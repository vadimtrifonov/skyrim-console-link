#pragma once

#include <cctype>
#include <charconv>
#include <optional>
#include <string_view>

namespace LocalCommands
{
	[[nodiscard]] inline std::string_view TrimAscii(std::string_view value) noexcept
	{
		const auto isSpace = [](const unsigned char ch) noexcept {
			return std::isspace(ch) != 0;
		};

		while (!value.empty() && isSpace(static_cast<unsigned char>(value.front()))) {
			value.remove_prefix(1);
		}

		while (!value.empty() && isSpace(static_cast<unsigned char>(value.back()))) {
			value.remove_suffix(1);
		}

		return value;
	}

	inline std::string_view ConsumeToken(std::string_view& value) noexcept
	{
		value = TrimAscii(value);
		const auto tokenEnd = value.find_first_of(" \t");
		if (tokenEnd == std::string_view::npos) {
			const auto token = value;
			value = {};
			return token;
		}

		const auto token = value.substr(0, tokenEnd);
		value.remove_prefix(tokenEnd);
		value = TrimAscii(value);
		return token;
	}

	[[nodiscard]] inline std::optional<std::size_t> ParsePositiveLimit(std::string_view token) noexcept
	{
		if (token.empty()) {
			return std::nullopt;
		}

		std::size_t value = 0;
		const auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
		if (ec != std::errc() || ptr != token.data() + token.size() || value == 0) {
			return std::nullopt;
		}

		return value;
	}
}
