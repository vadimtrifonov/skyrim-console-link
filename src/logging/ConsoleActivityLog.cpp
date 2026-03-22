#include "logging/ConsoleActivityLog.h"

#include "pch.h"

#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace
{
	std::mutex                      g_consoleLogLock;
	std::shared_ptr<spdlog::logger> g_consoleActivityLogger;

	std::string BuildRecord(std::string_view channel, std::string_view line)
	{
		return "[" + std::string(channel) + "] " + std::string(line);
	}
}

namespace Logging
{
	void InitializeConsoleActivityLog(const Config::Settings& settings)
	{
		std::scoped_lock lock(g_consoleLogLock);
		g_consoleActivityLogger.reset();

		if (!settings.mirrorToSeparateConsoleLog) {
			return;
		}

		auto logPath = logs::log_directory();
		if (!logPath) {
			logs::warn("Could not resolve the standard SKSE log directory for the console activity log");
			return;
		}

		*logPath /= "ConsoleDebugger.console.log";

		try {
			auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath->string(), true);
			auto logger = std::make_shared<spdlog::logger>("console activity log", std::move(sink));
			logger->set_level(spdlog::level::info);
			logger->flush_on(spdlog::level::info);
			logger->set_pattern(settings.includeTimestamps ? "[%Y-%m-%d %H:%M:%S.%e] %v" : "%v");
			g_consoleActivityLogger = std::move(logger);
			logs::info("Console activity log: {}", logPath->string());
		} catch (const std::exception& exception) {
			logs::warn("Failed to initialize console activity log at {}: {}", logPath->string(), exception.what());
		}
	}

	void LogWriteBackLine(std::string_view line)
	{
		const auto record = BuildRecord("console-writeback", line);
		logs::info("{}", record);

		std::shared_ptr<spdlog::logger> logger;
		{
			std::scoped_lock lock(g_consoleLogLock);
			logger = g_consoleActivityLogger;
		}

		if (logger) {
			logger->info("{}", record);
		}
	}
}
