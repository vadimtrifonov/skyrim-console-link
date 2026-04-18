#include "local_commands/LocalCommandProcessor.h"

#include "local_commands/lookup/LookupCommand.h"
#include "local_commands/lookup/LookupFormCommand.h"
#include "local_commands/lookup_lvli/LookupLvliCommand.h"
#include "local_commands/lookup_lvli/LookupLvliFormCommand.h"
#include "local_commands/lookup_prefix/LookupPrefixCommand.h"
#include "local_commands/support/LocalCommandParsing.h"
#include "logging/ConsoleActivityLog.h"
#include "pch.h"

#include <cctype>
#include <mutex>

namespace
{
	void SwallowFailSoftBoundary() noexcept {}

	enum class LocalCommandKind : std::uint8_t
	{
		kLookup,
		kLookupForm,
		kLookupLvli,
		kLookupLvliForm,
		kLookupPrefix,
		kUnknown
	};

	using LocalCommands::ConsumeToken;
	using LocalCommands::TrimAscii;

	LocalCommandKind ParseLocalCommandKind(std::string_view commandName) noexcept
	{
		if (commandName == "cdbg.lookup") {
			return LocalCommandKind::kLookup;
		}
		if (commandName == "cdbg.lookup-prefix") {
			return LocalCommandKind::kLookupPrefix;
		}
		if (commandName == "cdbg.lookup-form") {
			return LocalCommandKind::kLookupForm;
		}
		if (commandName == "cdbg.lookup-lvli") {
			return LocalCommandKind::kLookupLvli;
		}
		if (commandName == "cdbg.lookup-lvli-form") {
			return LocalCommandKind::kLookupLvliForm;
		}

		return LocalCommandKind::kUnknown;
	}

	struct LocalCommandState
	{
		std::mutex lock;
		bool ready = false;
		bool initialized = false;
	};

	LocalCommandState& GetState()
	{
		static LocalCommandState state;
		return state;
	}

	void OnDataLoaded() noexcept
	{
		auto& state = GetState();
		{
			std::scoped_lock lock(state.lock);
			if (state.ready) {
				return;
			}
			state.ready = true;
		}

		Logging::LogLocalActivityLine("local commands are ready");
	}

	void MessageHandler(SKSE::MessagingInterface::Message* message)
	{
		if (message != nullptr && message->type == SKSE::MessagingInterface::kDataLoaded) {
			OnDataLoaded();
		}
	}
}

namespace LocalCommands
{
	void Initialize() noexcept
	{
		auto& state = GetState();
		{
			std::scoped_lock lock(state.lock);
			if (state.initialized) {
				return;
			}
			state.initialized = true;
		}

		const auto* messaging = SKSE::GetMessagingInterface();
		if (messaging == nullptr) {
			try {
				logs::warn("Local command processor could not register for kDataLoaded because SKSE messaging is unavailable");
			} catch (...) {
				SwallowFailSoftBoundary();
			}
			return;
		}

		if (!messaging->RegisterListener(MessageHandler)) {
			try {
				logs::warn("Local command processor could not register the SKSE messaging listener");
			} catch (...) {
				SwallowFailSoftBoundary();
			}
		}
	}

	bool IsLocalCommandLine(std::string_view line) noexcept
	{
		line = TrimAscii(line);
		const auto tokenEnd = line.find_first_of(" \t");
		const auto commandName = tokenEnd == std::string_view::npos ? line : line.substr(0, tokenEnd);
		return commandName.starts_with("cdbg.");
	}

	ProcessResult Process(std::string_view line) noexcept
	{
		try {
			const auto ready = [&] {
				auto& state = GetState();
				std::scoped_lock lock(state.lock);
				return state.ready;
			}();

			if (!ready) {
				Logging::LogLocalActivityLine("local commands are not ready");
				return ProcessResult::kRejected;
			}

			auto remaining = TrimAscii(line);
			const auto commandName = ConsumeToken(remaining);
			const auto arguments = TrimAscii(remaining);

			Logging::LogLocalActivityLine(line);

			std::vector<std::string> responseLines;
			switch (ParseLocalCommandKind(commandName)) {
			case LocalCommandKind::kLookup:
				responseLines = ExecuteLookupCommand(arguments);
				break;
			case LocalCommandKind::kLookupForm:
				responseLines = ExecuteLookupFormCommand(arguments);
				break;
			case LocalCommandKind::kLookupLvli:
				responseLines = ExecuteLookupLvliCommand(arguments);
				break;
			case LocalCommandKind::kLookupLvliForm:
				responseLines = ExecuteLookupLvliFormCommand(arguments);
				break;
			case LocalCommandKind::kLookupPrefix:
				responseLines = ExecuteLookupPrefixCommand(arguments);
				break;
			case LocalCommandKind::kUnknown:
			default:
				responseLines.push_back(
					"unknown local command \"" + std::string(commandName) +
					"\"; supported: cdbg.lookup <EditorID>, cdbg.lookup-prefix <prefix> [limit], cdbg.lookup-form <plugin>|<local-formid>, cdbg.lookup-lvli <EditorID> [limit], cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]");
				break;
			}

			for (const auto& responseLine : responseLines) {
				Logging::LogLocalActivityLine(responseLine);
			}

			return ProcessResult::kHandled;
		} catch (const std::exception& exception) {
			try {
				logs::warn("Local command processor failed: {}", exception.what());
			} catch (...) {
				SwallowFailSoftBoundary();
			}
		} catch (...) {
			try {
				logs::warn("Local command processor failed: unknown exception");
			} catch (...) {
				SwallowFailSoftBoundary();
			}
		}

		Logging::LogLocalActivityLine("local command failed");
		return ProcessResult::kRejected;
	}
}
