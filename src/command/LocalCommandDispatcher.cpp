#include "command/LocalCommandDispatcher.h"

#include "command/LookupCommand.h"
#include "command/LookupPrefixCommand.h"
#include "logging/ConsoleActivityLog.h"
#include "pch.h"
#include "print/PrintService.h"

#include <cctype>

namespace
{
	enum class LocalCommandKind : std::uint8_t
	{
		kNotHandled,
		kLookup,
		kLookupPrefix,
		kUnknown
	};

	void SwallowFailSoftBoundary() noexcept {}

	std::string_view TrimAscii(std::string_view value) noexcept
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

	std::string_view ConsumeToken(std::string_view& value) noexcept
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

	void EmitLocalResponseLine(std::string_view line) noexcept
	{
		Logging::LogLocalActivityLine(line);
		static_cast<void>(Print::TryPrintImmediateLine(line));
	}

	void WarnLocalDispatchFailure(std::string_view message) noexcept
	{
		try {
			logs::warn("{}", message);
		} catch (...) {
			SwallowFailSoftBoundary();
		}
	}

	LocalCommandKind ParseLocalCommandKind(std::string_view commandName) noexcept
	{
		if (!commandName.starts_with("cdbg.")) {
			return LocalCommandKind::kNotHandled;
		}

		if (commandName == "cdbg.lookup") {
			return LocalCommandKind::kLookup;
		}
		if (commandName == "cdbg.lookup-prefix") {
			return LocalCommandKind::kLookupPrefix;
		}

		return LocalCommandKind::kUnknown;
	}
}

namespace Command
{
	LocalCommandDispatchResult TryDispatchLocalCommand(std::string_view commandLine, const bool dataLoaded) noexcept
	{
		try {
			auto       remaining = TrimAscii(commandLine);
			const auto commandName = ConsumeToken(remaining);
			const auto arguments = TrimAscii(remaining);
			const auto commandKind = ParseLocalCommandKind(commandName);
			if (commandKind == LocalCommandKind::kNotHandled) {
				return LocalCommandDispatchResult::kNotHandled;
			}

			if (!dataLoaded) {
				static std::atomic_bool loggedWaitingForDataLoaded = false;
				if (!loggedWaitingForDataLoaded.exchange(true)) {
					WarnLocalDispatchFailure("Local console commands are waiting for kDataLoaded before execution");
				}
				return LocalCommandDispatchResult::kDeferred;
			}

			Logging::LogLocalActivityLine(commandLine);

			std::vector<std::string> responseLines;
			switch (commandKind) {
			case LocalCommandKind::kLookup:
				responseLines = ExecuteLookupCommand(arguments);
				break;
			case LocalCommandKind::kLookupPrefix:
				responseLines = ExecuteLookupPrefixCommand(arguments);
				break;
			case LocalCommandKind::kUnknown:
				responseLines.push_back(
					"unknown local command \"" + std::string(commandName) +
					"\"; supported: cdbg.lookup <EditorID>, cdbg.lookup-prefix <prefix> [limit]");
				break;
			case LocalCommandKind::kNotHandled:
			default:
				return LocalCommandDispatchResult::kNotHandled;
			}

			for (const auto& line : responseLines) {
				EmitLocalResponseLine(line);
			}

			return LocalCommandDispatchResult::kHandled;
		} catch (const std::exception& exception) {
			try {
				logs::warn("Local console command execution failed: {}", exception.what());
			} catch (...) {
				SwallowFailSoftBoundary();
			}
		} catch (...) {
			WarnLocalDispatchFailure("Local console command execution failed: unknown exception");
		}

		return LocalCommandDispatchResult::kTerminalFailure;
	}
}
