#include "pch.h"

#include "settings/Settings.h"
#include <Windows.h>

namespace
{
	constexpr std::size_t kInitialIniBufferSize = 256;
	constexpr std::size_t kMaxIniBufferSize = 4096;
	constexpr std::size_t kMaxFileBackedPathLength = 512;

	template <class T>
	struct ParseResult
	{
		T value;
		bool malformed = false;
	};

	struct RawValueResult
	{
		std::wstring value;
		bool missing = false;
		bool truncated = false;
	};

	struct IniEntry
	{
		std::wstring_view section;
		std::wstring_view key;
	};

	struct ReadEntryResult
	{
		IniEntry entry{};
		RawValueResult raw{};
	};

	bool HasIniKey(const std::filesystem::path& path, IniEntry entry)
	{
		const std::wstring sectionName(entry.section);
		const std::wstring keyName = std::wstring(entry.key);
		std::size_t bufferSize = kInitialIniBufferSize;

		while (bufferSize <= kMaxIniBufferSize) {
			std::vector<wchar_t> buffer(bufferSize, L'\0');
			const auto length = GetPrivateProfileSectionW(
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
					const auto separator = line.find(L'=');
					const auto candidateKey = separator == std::wstring_view::npos ? line : line.substr(0, separator);
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
		std::size_t bufferSize = kInitialIniBufferSize;
		std::wstring value;

		while (bufferSize <= kMaxIniBufferSize) {
			std::vector<wchar_t> buffer(bufferSize, L'\0');
			const auto length = GetPrivateProfileStringW(
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

	ReadEntryResult ReadFirstPresentRawValue(
		const std::filesystem::path& path,
		std::initializer_list<IniEntry> entries)
	{
		const auto fallbackEntry = entries.begin() != entries.end() ? *entries.begin() : IniEntry{};
		for (const auto entry : entries) {
			const auto raw = ReadRawValue(path, entry);
			if (!raw.missing) {
				return { entry, raw };
			}
		}

		return { fallbackEntry, { L"", true, false } };
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
		IniEntry entry,
		const std::wstring& raw,
		std::string_view fallback)
	{
		warnings.push_back(
			"Invalid INI value [" + ToUtf8(entry.section) +
			"] " + ToUtf8(entry.key) +
			"='" + ToUtf8(raw) +
			"'; using fallback " + std::string(fallback));
	}

	void AddFeatureDisableWarning(
		std::vector<std::string>& warnings,
		IniEntry entry,
		const std::wstring& raw,
		std::string_view reason,
		std::string_view featureLabel)
	{
		warnings.push_back(
			"Rejected INI value [" + ToUtf8(entry.section) +
			"] " + ToUtf8(entry.key) +
			"='" + ToUtf8(raw) +
			"' (" + std::string(reason) +
			"); disabling " + std::string(featureLabel));
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

		const auto value = Trim(raw);
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

namespace Settings
{
	std::filesystem::path GetDefaultSettingsPath()
	{
		return L"Data/SKSE/Plugins/ConsoleLink.ini";
	}

	std::filesystem::path GetDefaultInboxPath()
	{
		return L"ConsoleLink.inbox.txt";
	}

	Values GetBuiltInDefaults()
	{
		Values settings{};
		settings.inboxPath = GetDefaultInboxPath();
		return settings;
	}

	LoadResult LoadSettingsWithDiagnostics(const std::filesystem::path& path)
	{
		LoadResult result{};
		result.settings = GetBuiltInDefaults();

		auto applyBool = [&](std::initializer_list<IniEntry> entries, bool Values::* member) {
			const auto rawResult = ReadFirstPresentRawValue(path, entries);
			const auto parsed = ParseBool(rawResult.raw.value, result.settings.*member);
			if (parsed.malformed) {
				AddMalformedWarning(result.warnings, rawResult.entry, rawResult.raw.value, parsed.value ? "true" : "false");
			}
			result.settings.*member = parsed.value;
		};

		auto applyUInt32 = [&](std::initializer_list<IniEntry> entries, std::uint32_t Values::* member) {
			const auto rawResult = ReadFirstPresentRawValue(path, entries);
			const auto parsed = ParseUInt32(rawResult.raw.value, result.settings.*member);
			if (parsed.malformed) {
				AddMalformedWarning(result.warnings, rawResult.entry, rawResult.raw.value, std::to_string(parsed.value));
			}
			result.settings.*member = parsed.value;
		};

		auto applyPath = [&](std::initializer_list<IniEntry> entries, std::filesystem::path Values::* pathMember, bool Values::* enabledMember, std::string_view featureLabel) {
			const auto rawResult = ReadFirstPresentRawValue(path, entries);
			if (rawResult.raw.missing) {
				return;
			}

			if (rawResult.raw.truncated) {
				result.settings.*pathMember = std::filesystem::path{};
				result.settings.*enabledMember = false;
				AddFeatureDisableWarning(result.warnings, rawResult.entry, rawResult.raw.value, "value exceeds the supported INI string length", featureLabel);
				return;
			}

			const auto value = Trim(rawResult.raw.value);
			if (value.empty()) {
				result.settings.*pathMember = std::filesystem::path{};
				result.settings.*enabledMember = false;
				AddFeatureDisableWarning(result.warnings, rawResult.entry, rawResult.raw.value, "value is empty", featureLabel);
				return;
			}
			if (value.size() > kMaxFileBackedPathLength) {
				result.settings.*pathMember = std::filesystem::path{};
				result.settings.*enabledMember = false;
				AddFeatureDisableWarning(result.warnings, rawResult.entry, rawResult.raw.value, "value exceeds the supported file path length", featureLabel);
				return;
			}

			result.settings.*pathMember = std::filesystem::path(value);
		};

		applyBool({ { L"Main", L"EnableConsoleInputLogging" } }, &Values::enableConsoleInputLogging);
		applyBool({ { L"Main", L"EnableConsoleOutputLogging" } }, &Values::enableConsoleOutputLogging);
		applyBool({ { L"Main", L"EnableInbox" } }, &Values::enableInbox);
		applyPath(
			{ { L"Inbox", L"Path" } },
			&Values::inboxPath,
			&Values::enableInbox,
			"inbox");
		applyUInt32({ { L"Inbox", L"MaxLinesPerTick" } }, &Values::maxInboxLinesPerTick);

		return result;
	}

	Values LoadSettings(const std::filesystem::path& path)
	{
		return LoadSettingsWithDiagnostics(path).settings;
	}

	Values LoadDefaultSettings()
	{
		return LoadSettings(GetDefaultSettingsPath());
	}
}
