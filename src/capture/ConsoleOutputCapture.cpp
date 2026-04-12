#include "capture/ConsoleOutputCapture.h"

#include "logging/ConsoleActivityLog.h"
#include "pch.h"

#include "RE/C/ConsoleLog.h"

#include <cstdarg>
#include <cstdio>

namespace
{
	using VPrint_t = void (*)(RE::ConsoleLog*, const char*, std::va_list);

	struct ValidatedOutputHookSite
	{
		const char* runtimeLabel;
		std::array<std::uint8_t, 16> expectedPrologue;
		std::size_t hookSize;
	};

	struct AbsoluteJumpWrite
	{
		std::uintptr_t patchSiteAddress;
		std::uintptr_t jumpTargetAddress;
		std::size_t patchSize;
	};

	constexpr ValidatedOutputHookSite kVPrintHookSite{
		"Skyrim VR",
		{ 0x48, 0x8B, 0xC4, 0x57, 0x41, 0x54, 0x41, 0x55,
			0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x40 },
		16
	};

	constexpr std::size_t kAbsoluteJumpSize = 14;
	constexpr std::size_t kMaxCapturedLineBytes = 4096;

	VPrint_t g_originalVPrint = nullptr;
	std::atomic_bool g_loggedFormatFailure = false;
	std::atomic_bool g_loggedTruncation = false;
	std::atomic_bool g_pluginCaptureDisabled = false;
	std::atomic_bool g_pluginCaptureFailureLogged = false;

	void SwallowFailSoftBoundary() noexcept {}

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

	std::string TrimConsoleLine(std::string value)
	{
		while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
			value.pop_back();
		}
		return value;
	}

	[[nodiscard]] const void* AddressAsPointer(const std::uintptr_t address) noexcept
	{
		return reinterpret_cast<const void*>(address);
	}

	void WriteAbsoluteJump(const AbsoluteJumpWrite& write)
	{
		std::array<std::uint8_t, kVPrintHookSite.hookSize> patch{};
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

	void WarnBestEffort(std::string_view message) noexcept
	{
		try {
			logs::warn("{}", message);
		} catch (...) {
			SwallowFailSoftBoundary();
		}
	}

	void DisablePluginCapture(std::string_view reason) noexcept
	{
		g_pluginCaptureDisabled = true;
		if (!g_pluginCaptureFailureLogged.exchange(true)) {
			try {
				logs::warn("Disabling passive console output plugin work after failure: {}", reason);
			} catch (...) {
				SwallowFailSoftBoundary();
			}
		}
	}

	std::string CaptureFormattedLine(const char* format, std::va_list args)
	{
		if (format == nullptr) {
			return {};
		}

		std::array<char, kMaxCapturedLineBytes + 1> buffer{};
		const auto written = std::vsnprintf(buffer.data(), buffer.size(), format, args);
		if (written < 0) {
			if (!g_loggedFormatFailure.exchange(true)) {
				WarnBestEffort("Passive console output capture failed to format a ConsoleLog::VPrint line; later failures will be suppressed");
			}
			return {};
		}

		if (written > static_cast<int>(kMaxCapturedLineBytes) && !g_loggedTruncation.exchange(true)) {
			try {
				logs::warn(
					"Passive console output capture truncated a ConsoleLog::VPrint line to {} bytes; later truncation warnings will be suppressed",
					kMaxCapturedLineBytes);
			} catch (...) {
				SwallowFailSoftBoundary();
			}
		}

		return TrimConsoleLine(std::string(buffer.data(), static_cast<std::size_t>(std::min<int>(written, kMaxCapturedLineBytes))));
	}

	void HookedVPrint(RE::ConsoleLog* consoleLog, const char* format, std::va_list args)
	{
		std::string line;
		if (!g_pluginCaptureDisabled.load()) {
			std::va_list copy;
			va_copy(copy, args);
			try {
				line = CaptureFormattedLine(format, copy);
			} catch (const std::exception& exception) {
				DisablePluginCapture(exception.what());
			} catch (...) {
				DisablePluginCapture("unknown exception");
			}
			va_end(copy);
		}

		g_originalVPrint(consoleLog, format, args);

		if (!line.empty() && !g_pluginCaptureDisabled.load()) {
			Logging::LogConsoleOutputLine(line);
		}
	}
}

namespace Capture::Output
{
	void Install(const Config::Settings& settings)
	{
		static bool attempted = false;
		if (attempted) {
			return;
		}
		attempted = true;

		if (!settings.enableConsoleOutputLogging) {
			logs::info("Console output capture is disabled by config");
			return;
		}

		if (!REL::Module::IsVR()) {
			logs::warn("Skipping passive console output capture: the first observational hook is VR-only");
			return;
		}

		try {
			REL::Relocation<std::uintptr_t> target{ RE::Offset::ConsoleLog::VPrint };
			if (target.address() == 0) {
				logs::warn("Skipping passive console output capture: failed to resolve ConsoleLog::VPrint");
				return;
			}

			std::array<std::uint8_t, kVPrintHookSite.hookSize> actualBytes{};
			std::memcpy(actualBytes.data(), AddressAsPointer(target.address()), actualBytes.size());
			if (!std::equal(actualBytes.begin(), actualBytes.end(), kVPrintHookSite.expectedPrologue.begin(), kVPrintHookSite.expectedPrologue.end())) {
				logs::warn(
					"Skipping passive console output capture: {} ConsoleLog::VPrint prologue mismatch at 0x{:X}. expected=[{}] actual=[{}]",
					kVPrintHookSite.runtimeLabel,
					target.address(),
					FormatBytes(kVPrintHookSite.expectedPrologue),
					FormatBytes(actualBytes));
				return;
			}

			auto* trampoline = static_cast<std::uint8_t*>(SKSE::GetTrampoline().allocate(kVPrintHookSite.hookSize + kAbsoluteJumpSize));
			if (trampoline == nullptr) {
				logs::warn("Skipping passive console output capture: failed to allocate trampoline memory");
				return;
			}

			std::memcpy(trampoline, actualBytes.data(), actualBytes.size());
			WriteAbsoluteJump(AbsoluteJumpWrite{
				.patchSiteAddress = reinterpret_cast<std::uintptr_t>(trampoline) + kVPrintHookSite.hookSize,
				.jumpTargetAddress = target.address() + kVPrintHookSite.hookSize,
				.patchSize = kAbsoluteJumpSize });

			g_originalVPrint = reinterpret_cast<VPrint_t>(trampoline);
			WriteAbsoluteJump(AbsoluteJumpWrite{
				.patchSiteAddress = target.address(),
				.jumpTargetAddress = reinterpret_cast<std::uintptr_t>(&HookedVPrint),
				.patchSize = kVPrintHookSite.hookSize });

			logs::info(
				"Installed passive console output capture for {} at 0x{:X} with a validated {}-byte ConsoleLog::VPrint prologue",
				kVPrintHookSite.runtimeLabel,
				target.address(),
				kVPrintHookSite.hookSize);
		} catch (const std::exception& exception) {
			logs::warn("Skipping passive console output capture after install failure: {}", exception.what());
		} catch (...) {
			logs::warn("Skipping passive console output capture after an unknown install failure");
		}
	}
}
