#include "pch.h"

#include <Windows.h>

#include "config/Settings.h"

namespace
{
	constexpr std::size_t kInitialIniBufferSize = 256;
	constexpr std::size_t kMaxIniBufferSize = 4096;
	constexpr std::size_t kMaxOutboxPathLength = 512;

	template <class T>
	struct ParseResult
	{
		T    value;
		bool malformed = false;
	};

	struct RawValueResult
	{
		std::wstring value;
		bool         missing = false;
		bool         truncated = false;
	};

	struct IniEntry
	{
		std::wstring_view section;
		std::wstring_view key;
	};

	bool HasIniKey(const std::filesystem::path& path, IniEntry entry)
	{
		const std::wstring sectionName(entry.section);
		const std::wstring keyName = std::wstring(entry.key);
		std::size_t        bufferSize = kInitialIniBufferSize;

		while (bufferSize <= kMaxIniBufferSize) {
			std::vector<wchar_t> buffer(bufferSize, L'\0');
			const auto           length = GetPrivateProfileSectionW(
                sectionName.c_str(),
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                path.c_str());

			if (length == 0) {
				return false;
			}

			if (length < buffer.size() - 2) {
				for (const auto* cursor = buffer.data(); *cursor != L'\0'; cursor += std::char_traits<wchar_t>::length(cursor) + 1) {
					const std::wstring_view line(cursor);
					const auto              separator = line.find(L'=');
					const auto              candidateKey = separator == std::wstring_view::npos ? line : line.substr(0, separator);
					if (candidateKey == keyName) {
						return true;
					}
				}

				return false;
			}

			bufferSize *= 2;
		}

		return false;
	}

	RawValueResult ReadRawValue(const std::filesystem::path& path, IniEntry entry)
	{
		if (!HasIniKey(path, entry)) {
			return { L"", true, false };
		}

		const std::wstring sectionName(entry.section);
		const std::wstring keyName(entry.key);
		std::size_t        bufferSize = kInitialIniBufferSize;
		std::wstring       value;

		while (bufferSize <= kMaxIniBufferSize) {
			std::vector<wchar_t> buffer(bufferSize, L'\0');
			const auto           length = GetPrivateProfileStringW(
                sectionName.c_str(),
                keyName.c_str(),
                L"",
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                path.c_str());

			value.assign(buffer.data(), length);
			if (length < buffer.size() - 1) {
				return { value, false, false };
			}

			bufferSize *= 2;
		}

		return { value, false, true };
	}

	std::wstring Trim(std::wstring value)
	{
		const auto notSpace = [](wchar_t ch) {
			return !std::iswspace(ch);
		};

		const auto begin = std::find_if(value.begin(), value.end(), notSpace);
		const auto end = std::find_if(value.rbegin(), value.rend(), notSpace).base();
		if (begin >= end) {
			return L"";
		}

		return std::wstring(begin, end);
	}

	std::wstring Lower(std::wstring value)
	{
		std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
			return static_cast<wchar_t>(std::towlower(ch));
		});
		return value;
	}

	std::string ToUtf8(std::wstring_view value)
	{
		return SKSE::stl::utf16_to_utf8(value).value_or(std::string("<unicode conversion error>"));
	}

	void AddMalformedWarning(
		std::vector<std::string>& warnings,
		IniEntry                  entry,
		const std::wstring&       raw,
		std::string_view          fallback)
	{
		warnings.push_back(
			"Invalid INI value [" + ToUtf8(entry.section) +
			"] " + ToUtf8(entry.key) +
			"='" + ToUtf8(raw) +
			"'; using fallback " + std::string(fallback));
	}

	void AddWriteBackDisableWarning(
		std::vector<std::string>& warnings,
		IniEntry                  entry,
		const std::wstring&       raw,
		std::string_view          reason)
	{
		warnings.push_back(
			"Rejected INI value [" + ToUtf8(entry.section) +
			"] " + ToUtf8(entry.key) +
			"='" + ToUtf8(raw) +
			"' (" + std::string(reason) +
			"); disabling console write-back");
	}

	ParseResult<bool> ParseBool(const std::wstring& raw, bool fallback)
	{
		if (raw.empty()) {
			return { fallback, false };
		}

		const auto value = Lower(Trim(raw));
		if (value == L"1" || value == L"true" || value == L"yes" || value == L"on") {
			return { true, false };
		}
		if (value == L"0" || value == L"false" || value == L"no" || value == L"off") {
			return { false, false };
		}

		return { fallback, true };
	}

	ParseResult<std::uint32_t> ParseUInt32(const std::wstring& raw, std::uint32_t fallback)
	{
		if (raw.empty()) {
			return { fallback, false };
		}

		const auto  value = Trim(raw);
		std::size_t index = 0;

		try {
			const auto parsed = std::stoul(value, &index, 10);
			return index == value.size() && parsed <= UINT32_MAX ?
			           ParseResult<std::uint32_t>{ static_cast<std::uint32_t>(parsed), false } :
			           ParseResult<std::uint32_t>{ fallback, true };
		} catch (...) {
			return { fallback, true };
		}
	}

}

namespace Config
{
	std::filesystem::path GetDefaultSettingsPath()
	{
		return L"Data/SKSE/Plugins/ConsoleDebugger.ini";
	}

	std::filesystem::path GetDefaultOutboxPath()
	{
		return L"ConsoleDebugger.outbox.txt";
	}

	Settings GetBuiltInDefaults()
	{
		Settings settings{};
		settings.outboxPath = GetDefaultOutboxPath();
		return settings;
	}

	LoadResult LoadSettingsWithDiagnostics(const std::filesystem::path& path)
	{
		LoadResult result{};
		result.settings = GetBuiltInDefaults();

		auto applyBool = [&](IniEntry entry, bool Settings::* member) {
			const auto raw = ReadRawValue(path, entry);
			const auto parsed = ParseBool(raw.value, result.settings.*member);
			if (parsed.malformed) {
				AddMalformedWarning(result.warnings, entry, raw.value, parsed.value ? "true" : "false");
			}
			result.settings.*member = parsed.value;
		};

		auto applyUInt32 = [&](IniEntry entry, std::uint32_t Settings::* member) {
			const auto raw = ReadRawValue(path, entry);
			const auto parsed = ParseUInt32(raw.value, result.settings.*member);
			if (parsed.malformed) {
				AddMalformedWarning(result.warnings, entry, raw.value, std::to_string(parsed.value));
			}
			result.settings.*member = parsed.value;
		};

		auto applyPath = [&](IniEntry entry, std::filesystem::path Settings::* member) {
			const auto raw = ReadRawValue(path, entry);
			if (raw.missing) {
				return;
			}

			if (raw.truncated) {
				result.settings.*member = std::filesystem::path{};
				result.settings.enableConsoleWriteBack = false;
				AddWriteBackDisableWarning(result.warnings, entry, raw.value, "value exceeds the supported INI string length");
				return;
			}

			const auto value = Trim(raw.value);
			if (value.empty()) {
				result.settings.*member = std::filesystem::path{};
				result.settings.enableConsoleWriteBack = false;
				AddWriteBackDisableWarning(result.warnings, entry, raw.value, "value is empty");
				return;
			}
			if (value.size() > kMaxOutboxPathLength) {
				result.settings.*member = std::filesystem::path{};
				result.settings.enableConsoleWriteBack = false;
				AddWriteBackDisableWarning(result.warnings, entry, raw.value, "value exceeds the supported outbox path length");
				return;
			}

			result.settings.*member = std::filesystem::path(value);
		};

		applyBool({ L"Main", L"EnableConsoleInputLogging" }, &Settings::enableConsoleInputLogging);
		applyBool({ L"Main", L"EnableConsoleOutputLogging" }, &Settings::enableConsoleOutputLogging);
		applyBool({ L"Main", L"EnableConsoleWriteBack" }, &Settings::enableConsoleWriteBack);
		applyBool({ L"Logging", L"MirrorToSeparateConsoleLog" }, &Settings::mirrorToSeparateConsoleLog);
		applyBool({ L"Logging", L"IncludeTimestamps" }, &Settings::includeTimestamps);
		applyPath({ L"WriteBack", L"OutboxPath" }, &Settings::outboxPath);
		applyUInt32({ L"WriteBack", L"MaxLinesPerTick" }, &Settings::maxLinesPerTick);
		applyBool({ L"Debug", L"TraceHookInstall" }, &Settings::traceHookInstall);

		return result;
	}

	Settings LoadSettings(const std::filesystem::path& path)
	{
		return LoadSettingsWithDiagnostics(path).settings;
	}

	Settings LoadDefaultSettings()
	{
		return LoadSettings(GetDefaultSettingsPath());
	}
}
