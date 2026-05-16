#include "console/ConsoleOutputCapture.h"

#include "logging/ConsoleActivityLog.h"
#include "pch.h"

#include "RE/C/ConsoleLog.h"

#include <cstdarg>
#include <cstdio>

namespace
{
	using VPrint_t = void (*)(RE::ConsoleLog*, const char*, std::va_list);

	constexpr std::size_t kVPrintHookSize = 16;

	struct ValidatedOutputHookSite
	{
		std::string_view label;
		std::array<std::uint8_t, kVPrintHookSize> expectedPrologue;
	};

	// vr_address_tools marks ConsoleLog::VPrint as bit-for-bit identical for SE/VR.
	// AE 1.6.1170 was live-disassembled before enabling this site.
	constexpr std::array<ValidatedOutputHookSite, 2> kVPrintHookSites{
		ValidatedOutputHookSite{
			.label = "SE/VR",
			.expectedPrologue = { 0x48, 0x8B, 0xC4, 0x57, 0x41, 0x54, 0x41, 0x55,
				0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x40 },
		},
		ValidatedOutputHookSite{
			.label = "AE 1.6.1170",
			.expectedPrologue = { 0x4C, 0x89, 0x44, 0x24, 0x18, 0x48, 0x89, 0x54,
				0x24, 0x10, 0x53, 0x55, 0x56, 0x57, 0x41, 0x54 },
		},
	};

	constexpr std::size_t kAbsoluteJumpSize = 14;
	constexpr std::size_t kMaxCapturedLineBytes = 4096;

	VPrint_t g_originalVPrint = nullptr;
	std::atomic_bool g_outputCaptureDisabled = false;

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

	std::string FormatExpectedHookSites()
	{
		std::string formatted;
		for (const auto& site : kVPrintHookSites) {
			if (!formatted.empty()) {
				formatted.append("; ");
			}
			formatted.append(site.label);
			formatted.append("=[");
			formatted.append(FormatBytes(site.expectedPrologue));
			formatted.push_back(']');
		}
		return formatted;
	}

	const ValidatedOutputHookSite* FindValidatedOutputHookSite(std::span<const std::uint8_t> actualBytes) noexcept
	{
		for (const auto& site : kVPrintHookSites) {
			if (std::equal(actualBytes.begin(), actualBytes.end(), site.expectedPrologue.begin(), site.expectedPrologue.end())) {
				return &site;
			}
		}
		return nullptr;
	}

	std::string TrimTrailingLineTerminators(std::string value)
	{
		while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
			value.pop_back();
		}
		return value;
	}

	[[nodiscard]] std::string_view GetRuntimeName() noexcept
	{
		if (REL::Module::IsVR()) {
			return "Skyrim VR";
		}
		if (REL::Module::IsAE()) {
			return "Skyrim AE";
		}
		return "Skyrim SE";
	}

	void WriteAbsoluteJump(
		const std::uintptr_t patchSiteAddress,
		const std::uintptr_t jumpTargetAddress,
		const std::size_t patchSize)
	{
		std::array<std::uint8_t, kVPrintHookSize> patch{};
		patch.fill(0x90);
		patch[0] = 0xFF;
		patch[1] = 0x25;
		patch[2] = 0x00;
		patch[3] = 0x00;
		patch[4] = 0x00;
		patch[5] = 0x00;
		std::memcpy(patch.data() + 6, &jumpTargetAddress, sizeof(jumpTargetAddress));
		REL::safe_write(patchSiteAddress, patch.data(), patchSize);
	}

	std::string CaptureFormattedLine(const char* format, std::va_list args)
	{
		if (format == nullptr) {
			return {};
		}

		std::array<char, kMaxCapturedLineBytes + 1> buffer{};
		const auto written = std::vsnprintf(buffer.data(), buffer.size(), format, args);
		if (written < 0) {
			logs::warn("Passive console output capture failed to format a ConsoleLog::VPrint line");
			return {};
		}

		if (written > static_cast<int>(kMaxCapturedLineBytes)) {
			logs::warn(
				"Passive console output capture truncated a ConsoleLog::VPrint line to {} bytes",
				kMaxCapturedLineBytes);
		}

		return TrimTrailingLineTerminators(std::string(buffer.data(), static_cast<std::size_t>(std::min<int>(written, kMaxCapturedLineBytes))));
	}

	void HookedVPrint(RE::ConsoleLog* consoleLog, const char* format, std::va_list args)
	{
		std::string line;
		if (!g_outputCaptureDisabled.load()) {
			std::va_list copy;
			va_copy(copy, args);
			try {
				line = CaptureFormattedLine(format, copy);
			} catch (const std::exception& exception) {
				g_outputCaptureDisabled = true;
				logs::warn("Disabling passive console output capture after failure: {}", exception.what());
			} catch (...) {
				g_outputCaptureDisabled = true;
				logs::warn("Disabling passive console output capture after failure: unknown exception");
			}
			va_end(copy);
		}

		g_originalVPrint(consoleLog, format, args);

		if (!line.empty() && !g_outputCaptureDisabled.load()) {
			Logging::LogConsoleOutputLine(line);
		}
	}
}

namespace Console::OutputCapture
{
	std::size_t RequiredTrampolineSize(const Settings::Values& settings) noexcept
	{
		return settings.enableConsoleOutputLogging ? kVPrintHookSize + kAbsoluteJumpSize : 0;
	}

	void Install(const Settings::Values& settings)
	{
		static bool attempted = false;
		if (attempted) {
			return;
		}
		attempted = true;

		if (!settings.enableConsoleOutputLogging) {
			logs::info("Console output capture is disabled by settings");
			return;
		}

		try {
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(50180, 51110) };
			if (target.address() == 0) {
				logs::warn("Skipping passive console output capture: failed to resolve ConsoleLog::VPrint");
				return;
			}

			std::array<std::uint8_t, kVPrintHookSize> actualBytes{};
			std::memcpy(actualBytes.data(), reinterpret_cast<const void*>(target.address()), actualBytes.size());
			const auto* hookSite = FindValidatedOutputHookSite(actualBytes);
			if (hookSite == nullptr) {
				logs::warn(
					"Skipping passive console output capture: {} ConsoleLog::VPrint prologue mismatch at 0x{:X}. expected_one_of=[{}] actual=[{}]",
					GetRuntimeName(),
					target.address(),
					FormatExpectedHookSites(),
					FormatBytes(actualBytes));
				return;
			}

			auto* trampoline = static_cast<std::uint8_t*>(SKSE::GetTrampoline().allocate(kVPrintHookSize + kAbsoluteJumpSize));

			std::memcpy(trampoline, actualBytes.data(), actualBytes.size());
			WriteAbsoluteJump(
				reinterpret_cast<std::uintptr_t>(trampoline) + kVPrintHookSize,
				target.address() + kVPrintHookSize,
				kAbsoluteJumpSize);

			g_originalVPrint = reinterpret_cast<VPrint_t>(trampoline);
			WriteAbsoluteJump(target.address(), reinterpret_cast<std::uintptr_t>(&HookedVPrint), kVPrintHookSize);

			logs::info(
				"Installed passive console output capture for {} at 0x{:X} with validated {} {}-byte ConsoleLog::VPrint prologue",
				GetRuntimeName(),
				target.address(),
				hookSite->label,
				kVPrintHookSize);
		} catch (const std::exception& exception) {
			logs::warn("Skipping passive console output capture after install failure: {}", exception.what());
		} catch (...) {
			logs::warn("Skipping passive console output capture after an unknown install failure");
		}
	}
}
