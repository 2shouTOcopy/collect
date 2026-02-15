#include "Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>

LogLevel Logger::s_level = LogLevel::Info;

void Logger::SetLevel(LogLevel level)
{
	s_level = level;
}

LogLevel Logger::GetLevel()
{
	return s_level;
}

void Logger::Log(LogLevel level, const std::string &tag, const std::string &msg)
{
	if (level < s_level)
	{
		return;
	}

	// Timestamp
	auto now = std::chrono::system_clock::now();
	auto timeT = std::chrono::system_clock::to_time_t(now);
	struct tm tmBuf = {};
	localtime_r(&timeT, &tmBuf);

	const char *levelStr = "???";
	switch (level)
	{
		case LogLevel::Debug:   levelStr = "DBG"; break;
		case LogLevel::Info:    levelStr = "INF"; break;
		case LogLevel::Warning: levelStr = "WRN"; break;
		case LogLevel::Error:   levelStr = "ERR"; break;
	}

	std::cerr << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S")
	          << " [" << levelStr << "] [" << tag << "] " << msg << "\n";
}

void Logger::Debug(const std::string &tag, const std::string &msg)
{
	Log(LogLevel::Debug, tag, msg);
}

void Logger::Info(const std::string &tag, const std::string &msg)
{
	Log(LogLevel::Info, tag, msg);
}

void Logger::Warn(const std::string &tag, const std::string &msg)
{
	Log(LogLevel::Warning, tag, msg);
}

void Logger::Error(const std::string &tag, const std::string &msg)
{
	Log(LogLevel::Error, tag, msg);
}

LogLevel Logger::ParseLevel(const std::string &levelStr)
{
	if (levelStr == "DEBUG")   return LogLevel::Debug;
	if (levelStr == "INFO")    return LogLevel::Info;
	if (levelStr == "WARNING") return LogLevel::Warning;
	if (levelStr == "ERROR")   return LogLevel::Error;
	return LogLevel::Info;
}
