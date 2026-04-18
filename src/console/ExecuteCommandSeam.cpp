#include "console/ExecuteCommandSeam.h"

#include "pch.h"

#include "RE/F/FxDelegateArgs.h"

#include <Windows.h>

#include <cstdio>

namespace
{
	using ExecuteCommandCallback_t = void (*)(const RE::FxDelegateArgs&);

	struct ValidatedInputHookSite
	{
		const char* runtimeLabel;
		std::array<std::uint8_t, 17> expectedPrologue;
		std::size_t hookSize;
		std::uintptr_t callbackRva;
	};

	struct AbsoluteJumpWrite
	{
		std::uintptr_t patchSiteAddress;
		std::uintptr_t jumpTargetAddress;
		std::size_t patchSize;
	};

	enum class CommandExtractionStatus : std::uint8_t
	{
		kSuccess,
		kNoArgs,
		kNullString,
		kUnexpectedType,
		kUtf16ConversionFailed
	};

	struct CommandExtractionResult
	{
		CommandExtractionStatus status{ CommandExtractionStatus::kNoArgs };
		std::string command;
	};

	constexpr ValidatedInputHookSite kExecuteCommandHookSite{
		"Skyrim VR",
		{ 0x40, 0x53,
			0x56,
			0x57,
			0x48, 0x83, 0xEC, 0x30,
			0x48, 0xC7, 0x44, 0x24, 0x20, 0xFE, 0xFF, 0xFF, 0xFF },
		17,
		0x00885100
	};

	constexpr std::size_t kAbsoluteJumpSize = 14;

	ExecuteCommandCallback_t g_originalExecuteCommand = nullptr;
	ExecuteCommandCallback_t g_directExecuteCommand = nullptr;
	std::uintptr_t g_executeCommandTargetAddress = 0;
	bool g_executeCommandPathValidated = false;
	bool g_executeCommandValidationAttempted = false;
	bool g_acceptedCommandHookInstalled = false;
	bool g_acceptedCommandHookInstallAttempted = false;
	std::atomic_bool g_acceptedCommandInterceptionDisabled = false;
	std::atomic<Console::ExecuteCommandSeam::AcceptedCommandHandler> g_acceptedCommandHandler = nullptr;

	std::string FormatBytes(std::span<const std::uint8_t> bytes)
	{
		std::string formatted;
		formatted.reserve(bytes.size() * 3);
		for (std::size_t index = 0; index < bytes.size(); ++index) {
			if (index != 0) {
				formatted.push_back(' ');
			}
			char buffer[4]{};
			std::snprintf(buffer, sizeof(buffer), "%02X", bytes[index]);
			formatted.append(buffer);
		}
		return formatted;
	}

	[[nodiscard]] const void* AddressAsPointer(const std::uintptr_t address) noexcept
	{
		return reinterpret_cast<const void*>(address);
	}

	template <class T>
	[[nodiscard]] T AddressAsFunctionPointer(const std::uintptr_t address) noexcept
	{
		return reinterpret_cast<T>(address);
	}

	void WriteAbsoluteJump(const AbsoluteJumpWrite& write)
	{
		std::array<std::uint8_t, kExecuteCommandHookSite.hookSize> patch{};
		patch.fill(0x90);
		patch[0] = 0xFF;
		patch[1] = 0x25;
		patch[2] = 0x00;
		patch[3] = 0x00;
		patch[4] = 0x00;
		patch[5] = 0x00;
		std::memcpy(patch.data() + 6, &write.jumpTargetAddress, sizeof(write.jumpTargetAddress));
		REL::safe_write(write.patchSiteAddress, patch.data(), write.patchSize);
	}

	std::string TrimConsoleCommand(std::string value)
	{
		while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
			value.pop_back();
		}
		return value;
	}

	std::optional<std::string> WideToUtf8(std::wstring_view value)
	{
		if (value.empty()) {
			return std::string{};
		}

		const auto required = ::WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
		if (required <= 0) {
			return std::nullopt;
		}

		std::string converted(static_cast<std::size_t>(required), '\0');
		const auto written = ::WideCharToMultiByte(
			CP_UTF8,
			0,
			value.data(),
			static_cast<int>(value.size()),
			converted.data(),
			required,
			nullptr,
			nullptr);
		if (written <= 0) {
			return std::nullopt;
		}

		return converted;
	}

	CommandExtractionResult ExtractCommandText(const RE::FxDelegateArgs& args)
	{
		if (args.GetArgCount() == 0) {
			return { CommandExtractionStatus::kNoArgs, {} };
		}

		const auto& value = args[0];
		if (value.IsString()) {
			const auto* command = value.GetString();
			if (command == nullptr) {
				return { CommandExtractionStatus::kNullString, {} };
			}

			return { CommandExtractionStatus::kSuccess, TrimConsoleCommand(command) };
		}

		if (value.IsStringW()) {
			const auto* command = value.GetStringW();
			if (command == nullptr) {
				return { CommandExtractionStatus::kNullString, {} };
			}

			auto converted = WideToUtf8(command);
			if (!converted.has_value()) {
				return { CommandExtractionStatus::kUtf16ConversionFailed, {} };
			}

			return { CommandExtractionStatus::kSuccess, TrimConsoleCommand(std::move(*converted)) };
		}

		return { CommandExtractionStatus::kUnexpectedType, {} };
	}

	void ReportExtractionDrift(const CommandExtractionResult& result, const RE::FxDelegateArgs& args) noexcept
	{
		switch (result.status) {
		case CommandExtractionStatus::kSuccess:
			return;
		case CommandExtractionStatus::kNoArgs:
			logs::warn("ExecuteCommand seam interception saw a callback with zero args");
			return;
		case CommandExtractionStatus::kNullString:
			logs::warn("ExecuteCommand seam interception saw a null string pointer");
			return;
		case CommandExtractionStatus::kUnexpectedType:
			logs::warn(
				"ExecuteCommand seam interception saw callback args in an unexpected shape: arg_count={} arg0_is_string={} arg0_is_stringw={}",
				args.GetArgCount(),
				args.GetArgCount() > 0 && args[0].IsString(),
				args.GetArgCount() > 0 && args[0].IsStringW());
			return;
		case CommandExtractionStatus::kUtf16ConversionFailed:
			logs::warn("ExecuteCommand seam interception failed to convert a wide string to UTF-8");
			return;
		}
	}

	void HookedExecuteCommand(const RE::FxDelegateArgs& args)
	{
		std::string command;
		if (!g_acceptedCommandInterceptionDisabled.load()) {
			try {
				const auto extraction = ExtractCommandText(args);
				command = extraction.command;
				ReportExtractionDrift(extraction, args);
			} catch (const std::exception& exception) {
				g_acceptedCommandInterceptionDisabled = true;
				logs::warn("Disabling accepted ExecuteCommand interception after failure: {}", exception.what());
			} catch (...) {
				g_acceptedCommandInterceptionDisabled = true;
				logs::warn("Disabling accepted ExecuteCommand interception after failure: unknown exception");
			}
		}

		g_originalExecuteCommand(args);

		if (command.empty()) {
			return;
		}

		const auto handler = g_acceptedCommandHandler.load();
		if (handler != nullptr) {
			handler(command);
		}
	}

	[[nodiscard]] ExecuteCommandCallback_t GetInvocationCallback() noexcept
	{
		if (g_originalExecuteCommand != nullptr) {
			return g_originalExecuteCommand;
		}

		return g_executeCommandPathValidated ? g_directExecuteCommand : nullptr;
	}
}

namespace Console::ExecuteCommandSeam
{
	bool Validate() noexcept
	{
		if (g_executeCommandPathValidated) {
			return true;
		}

		if (g_executeCommandValidationAttempted) {
			return false;
		}
		g_executeCommandValidationAttempted = true;

		try {
			const auto target = REL::Module::get().base() + kExecuteCommandHookSite.callbackRva;

			std::array<std::uint8_t, kExecuteCommandHookSite.hookSize> actualBytes{};
			std::memcpy(actualBytes.data(), AddressAsPointer(target), actualBytes.size());
			if (!std::equal(actualBytes.begin(), actualBytes.end(), kExecuteCommandHookSite.expectedPrologue.begin(), kExecuteCommandHookSite.expectedPrologue.end())) {
				logs::warn(
					"Skipping ExecuteCommand seam features: {} ExecuteCommand callback prologue mismatch at 0x{:X}. expected=[{}] actual=[{}]",
					kExecuteCommandHookSite.runtimeLabel,
					target,
					FormatBytes(kExecuteCommandHookSite.expectedPrologue),
					FormatBytes(actualBytes));
				return false;
			}

			g_directExecuteCommand = AddressAsFunctionPointer<ExecuteCommandCallback_t>(target);
			g_executeCommandTargetAddress = target;
			g_executeCommandPathValidated = true;
			return true;
		} catch (const std::exception& exception) {
			logs::warn("Skipping ExecuteCommand seam features after validation failure: {}", exception.what());
		} catch (...) {
			logs::warn("Skipping ExecuteCommand seam features after an unknown validation failure");
		}

		return false;
	}

	bool InstallAcceptedCommandHook(const AcceptedCommandHandler handler) noexcept
	{
		if (g_acceptedCommandHookInstalled) {
			return true;
		}

		if (g_acceptedCommandHookInstallAttempted) {
			return false;
		}
		g_acceptedCommandHookInstallAttempted = true;

		if (!Validate()) {
			return false;
		}

		g_acceptedCommandHandler = handler;
		try {
			std::array<std::uint8_t, kExecuteCommandHookSite.hookSize> actualBytes{};
			std::memcpy(actualBytes.data(), AddressAsPointer(g_executeCommandTargetAddress), actualBytes.size());

			auto* trampoline = static_cast<std::uint8_t*>(SKSE::GetTrampoline().allocate(kExecuteCommandHookSite.hookSize + kAbsoluteJumpSize));
			if (trampoline == nullptr) {
				logs::warn("Skipping accepted ExecuteCommand interception: failed to allocate trampoline memory");
				return false;
			}

			std::memcpy(trampoline, actualBytes.data(), actualBytes.size());
			WriteAbsoluteJump(AbsoluteJumpWrite{
				.patchSiteAddress = reinterpret_cast<std::uintptr_t>(trampoline) + kExecuteCommandHookSite.hookSize,
				.jumpTargetAddress = g_executeCommandTargetAddress + kExecuteCommandHookSite.hookSize,
				.patchSize = kAbsoluteJumpSize });

			g_originalExecuteCommand = reinterpret_cast<ExecuteCommandCallback_t>(trampoline);
			WriteAbsoluteJump(AbsoluteJumpWrite{
				.patchSiteAddress = g_executeCommandTargetAddress,
				.jumpTargetAddress = reinterpret_cast<std::uintptr_t>(&HookedExecuteCommand),
				.patchSize = kExecuteCommandHookSite.hookSize });

			g_acceptedCommandHookInstalled = true;
			logs::info(
				"Installed accepted ExecuteCommand interception for {} at 0x{:X} with a validated {}-byte callback prologue",
				kExecuteCommandHookSite.runtimeLabel,
				g_executeCommandTargetAddress,
				kExecuteCommandHookSite.hookSize);
			return true;
		} catch (const std::exception& exception) {
			logs::warn("Skipping accepted ExecuteCommand interception after install failure: {}", exception.what());
		} catch (...) {
			logs::warn("Skipping accepted ExecuteCommand interception after an unknown install failure");
		}

		return false;
	}

	InvokeResult Invoke(const RE::FxDelegateArgs& args) noexcept
	{
		const auto callback = GetInvocationCallback();
		if (callback == nullptr) {
			logs::warn("ExecuteCommand seam invocation is unavailable because the validated callback path was not resolved");
			return InvokeResult::kTerminalFailure;
		}

		try {
			callback(args);
			return InvokeResult::kInvoked;
		} catch (const std::exception& exception) {
			logs::warn("ExecuteCommand seam invocation failed: {}", exception.what());
		} catch (...) {
			logs::warn("ExecuteCommand seam invocation failed: unknown exception");
		}

		return InvokeResult::kTerminalFailure;
	}
}
