#include "logging/ConsoleActivityLog.h"

#include "pch.h"

#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace
{
	std::mutex                      g_consoleLogLock;
	std::shared_ptr<spdlog::logger> g_consoleActivityLogger;
	std::atomic_bool                g_consoleActivityDisabled = false;
	std::atomic_bool                g_consoleActivityFailureLogged = false;

	std::string BuildRecord(std::string_view channel, std::string_view line)
	{
		return "[" + std::string(channel) + "] " + std::string(line);
	}

	void DisableConsoleActivityLogging(std::string_view reason) noexcept
	{
		g_consoleActivityDisabled = true;
		{
			std::scoped_lock lock(g_consoleLogLock);
			g_consoleActivityLogger.reset();
		}

		if (!g_consoleActivityFailureLogged.exchange(true)) {
			try {
				logs::warn("Disabling console activity logging after a plugin-side failure: {}", reason);
			} catch (...) {
				// Intentionally swallow logger failures so detoured paths remain fail-soft.
			}
		}
	}

	void LogConsoleActivityLine(std::string_view channel, std::string_view line) noexcept
	{
		if (g_consoleActivityDisabled.load()) {
			return;
		}

		try {
			const auto record = BuildRecord(channel, line);
			std::shared_ptr<spdlog::logger> logger;
			{
				std::scoped_lock lock(g_consoleLogLock);
				logger = g_consoleActivityLogger;
			}

			if (logger) {
				logger->info("{}", record);
			}
		} catch (const std::exception& exception) {
			DisableConsoleActivityLogging(exception.what());
		} catch (...) {
			DisableConsoleActivityLogging("unknown exception");
		}
	}
}

namespace Logging
{
	std::optional<std::string> InitializeConsoleActivityLog(const Config::Settings& settings)
	{
		std::scoped_lock lock(g_consoleLogLock);
		g_consoleActivityLogger.reset();
		g_consoleActivityDisabled = false;
		g_consoleActivityFailureLogged = false;

		auto logPath = logs::log_directory();
		if (!logPath) {
			g_consoleActivityDisabled = true;
			return "Could not resolve the standard SKSE log directory for the console activity log";
		}

		*logPath /= "ConsoleLink.console.log";

		try {
			auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath->string(), true);
			auto logger = std::make_shared<spdlog::logger>("console activity log", std::move(sink));
			logger->set_level(spdlog::level::info);
			logger->flush_on(spdlog::level::info);
			logger->set_pattern(settings.includeTimestamps ? "[%Y-%m-%d %H:%M:%S.%e] %v" : "%v");
			g_consoleActivityLogger = std::move(logger);
			logs::info("Console activity log: {}", logPath->string());
			return std::nullopt;
		} catch (const std::exception& exception) {
			g_consoleActivityDisabled = true;
			return "Failed to initialize console activity log at " + logPath->string() + ": " + exception.what();
		} catch (...) {
			g_consoleActivityDisabled = true;
			return "Failed to initialize console activity log at " + logPath->string() + ": unknown exception";
		}
	}

	void LogConsoleOutputLine(std::string_view line) noexcept
	{
		LogConsoleActivityLine("console-out", line);
	}

	void LogConsoleInputLine(std::string_view line) noexcept
	{
		LogConsoleActivityLine("console-in", line);
	}

	void LogConsoleSubmitLine(std::string_view line) noexcept
	{
		LogConsoleActivityLine("console-submit", line);
	}

	void LogLocalLine(std::string_view line) noexcept
	{
		LogConsoleActivityLine("console-local", line);
	}

	void LogDisplayLine(std::string_view line) noexcept
	{
		LogConsoleActivityLine("console-display", line);
	}
}

