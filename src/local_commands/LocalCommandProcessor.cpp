#include "local_commands/LocalCommandProcessor.h"

#include "local_commands/LocalCommandReadinessState.h"
#include "local_commands/LookupCommand.h"
#include "local_commands/LookupPrefixCommand.h"
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
		kLookupPrefix,
		kUnknown
	};

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
	}

	LocalCommandKind ParseLocalCommandKind(std::string_view commandName) noexcept
	{
		if (commandName == "cdbg.lookup") {
			return LocalCommandKind::kLookup;
		}
		if (commandName == "cdbg.lookup-prefix") {
			return LocalCommandKind::kLookupPrefix;
		}

		return LocalCommandKind::kUnknown;
	}

	struct LocalCommandState
	{
		std::mutex lock;
		LocalCommands::LocalCommandReadinessState readiness;
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
		auto transition = LocalCommands::LocalCommandReadinessTransition::kNone;
		{
			std::scoped_lock lock(state.lock);
			transition = state.readiness.ObserveDataLoaded();
		}

		if (transition == LocalCommands::LocalCommandReadinessTransition::kBecameReady) {
			Logging::LogLocalActivityLine("local commands are ready");
		}
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
				return state.readiness.IsReady();
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
			case LocalCommandKind::kLookupPrefix:
				responseLines = ExecuteLookupPrefixCommand(arguments);
				break;
			case LocalCommandKind::kUnknown:
			default:
				responseLines.push_back(
					"unknown local command \"" + std::string(commandName) +
					"\"; supported: cdbg.lookup <EditorID>, cdbg.lookup-prefix <prefix> [limit]");
				break;
			}

			for (const auto& responseLine : responseLines) {
				EmitLocalResponseLine(responseLine);
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
