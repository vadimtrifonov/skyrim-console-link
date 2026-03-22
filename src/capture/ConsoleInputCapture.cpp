#include "capture/ConsoleInputCapture.h"

#include "logging/ConsoleActivityLog.h"
#include "pch.h"

#include "RE/F/FxDelegateArgs.h"

#include <Windows.h>

#include <cstdio>

namespace
{
	using ExecuteCommandCallback_t = void (*)(const RE::FxDelegateArgs&);

	struct ValidatedInputHookSite
	{
		const char*                  runtimeLabel;
		std::array<std::uint8_t, 17> expectedPrologue;
		std::size_t                  hookSize;
		std::uintptr_t               callbackRva;
	};

	enum class CommandExtractionStatus
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
		std::string             command;
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
	std::atomic_bool         g_pluginCaptureDisabled = false;
	std::atomic_bool         g_pluginCaptureFailureLogged = false;
	std::atomic_bool         g_loggedNoArgs = false;
	std::atomic_bool         g_loggedUnexpectedType = false;
	std::atomic_bool         g_loggedNullString = false;
	std::atomic_bool         g_loggedUtf16ConversionFailure = false;

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

	void WriteAbsoluteJump(std::uintptr_t source, std::uintptr_t destination, std::size_t writeSize)
	{
		std::array<std::uint8_t, kExecuteCommandHookSite.hookSize> patch{};
		patch.fill(0x90);
		patch[0] = 0xFF;
		patch[1] = 0x25;
		patch[2] = 0x00;
		patch[3] = 0x00;
		patch[4] = 0x00;
		patch[5] = 0x00;
		std::memcpy(patch.data() + 6, &destination, sizeof(destination));
		REL::safe_write(source, patch.data(), writeSize);
	}

	void DisablePluginCapture(std::string_view reason) noexcept
	{
		g_pluginCaptureDisabled = true;
		if (!g_pluginCaptureFailureLogged.exchange(true)) {
			try {
				logs::warn("Disabling passive console input plugin work after failure: {}", reason);
			} catch (...) {
				// Intentionally swallow logger failures so detoured paths remain fail-soft.
			}
		}
	}

	void WarnOnce(std::atomic_bool& flag, std::string_view message) noexcept
	{
		if (flag.exchange(true)) {
			return;
		}

		try {
			logs::warn("{}", message);
		} catch (...) {
			// Intentionally swallow logger failures so detoured paths remain fail-soft.
		}
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
			WarnOnce(
				g_loggedNoArgs,
				"Passive console input capture saw ExecuteCommand with zero args; later zero-arg warnings will be suppressed");
			return;
		case CommandExtractionStatus::kNullString:
			WarnOnce(
				g_loggedNullString,
				"Passive console input capture saw a null ExecuteCommand string pointer; later null-string warnings will be suppressed");
			return;
		case CommandExtractionStatus::kUnexpectedType:
			try {
				if (!g_loggedUnexpectedType.exchange(true)) {
					logs::warn(
						"Passive console input capture saw ExecuteCommand args in an unexpected shape: arg_count={} arg0_is_string={} arg0_is_stringw={}; later shape warnings will be suppressed",
						args.GetArgCount(),
						args.GetArgCount() > 0 && args[0].IsString(),
						args.GetArgCount() > 0 && args[0].IsStringW());
				}
			} catch (...) {
				// Intentionally swallow logger failures so detoured paths remain fail-soft.
			}
			return;
		case CommandExtractionStatus::kUtf16ConversionFailed:
			WarnOnce(
				g_loggedUtf16ConversionFailure,
				"Passive console input capture failed to convert a wide ExecuteCommand string to UTF-8; later conversion warnings will be suppressed");
			return;
		}
	}

	void HookedExecuteCommand(const RE::FxDelegateArgs& args)
	{
		CommandExtractionResult extraction;
		std::string command;
		if (!g_pluginCaptureDisabled.load()) {
			try {
				extraction = ExtractCommandText(args);
				command = extraction.command;
				ReportExtractionDrift(extraction, args);
			} catch (const std::exception& exception) {
				DisablePluginCapture(exception.what());
			} catch (...) {
				DisablePluginCapture("unknown exception");
			}
		}

		g_originalExecuteCommand(args);

		if (!command.empty() && !g_pluginCaptureDisabled.load()) {
			Logging::LogConsoleInputLine(command);
		}
	}
}

namespace Capture::Input
{
	void Install(const Config::Settings& settings)
	{
		static bool attempted = false;
		if (attempted) {
			return;
		}
		attempted = true;

		if (!settings.enableConsoleInputLogging) {
			logs::info("Console input capture is disabled by config");
			return;
		}

		if (!REL::Module::IsVR()) {
			logs::warn("Skipping passive console input capture: the first observational hook is VR-only");
			return;
		}

		try {
			const auto target = REL::Module::get().base() + kExecuteCommandHookSite.callbackRva;

			std::array<std::uint8_t, kExecuteCommandHookSite.hookSize> actualBytes{};
			std::memcpy(actualBytes.data(), reinterpret_cast<const void*>(target), actualBytes.size());
			if (!std::equal(actualBytes.begin(), actualBytes.end(), kExecuteCommandHookSite.expectedPrologue.begin(), kExecuteCommandHookSite.expectedPrologue.end())) {
				logs::warn(
					"Skipping passive console input capture: {} ExecuteCommand callback prologue mismatch at 0x{:X}. expected=[{}] actual=[{}]",
					kExecuteCommandHookSite.runtimeLabel,
					target,
					FormatBytes(kExecuteCommandHookSite.expectedPrologue),
					FormatBytes(actualBytes));
				return;
			}

			auto* trampoline = static_cast<std::uint8_t*>(SKSE::GetTrampoline().allocate(kExecuteCommandHookSite.hookSize + kAbsoluteJumpSize));
			if (trampoline == nullptr) {
				logs::warn("Skipping passive console input capture: failed to allocate trampoline memory for the ExecuteCommand callback");
				return;
			}

			std::memcpy(trampoline, actualBytes.data(), actualBytes.size());
			WriteAbsoluteJump(
				reinterpret_cast<std::uintptr_t>(trampoline) + kExecuteCommandHookSite.hookSize,
				target + kExecuteCommandHookSite.hookSize,
				kAbsoluteJumpSize);

			g_originalExecuteCommand = reinterpret_cast<ExecuteCommandCallback_t>(trampoline);
			WriteAbsoluteJump(target, reinterpret_cast<std::uintptr_t>(&HookedExecuteCommand), kExecuteCommandHookSite.hookSize);

			logs::info(
				"Installed passive console input capture for {} at 0x{:X} with a validated {}-byte ExecuteCommand callback prologue",
				kExecuteCommandHookSite.runtimeLabel,
				target,
				kExecuteCommandHookSite.hookSize);
		} catch (const std::exception& exception) {
			logs::warn("Skipping passive console input capture after install failure: {}", exception.what());
		} catch (...) {
			logs::warn("Skipping passive console input capture after an unknown install failure");
		}
	}
}
