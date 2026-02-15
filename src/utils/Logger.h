#pragma once

#include <string>
#include <iostream>

/// Simple logger — replaces printf-based logging with tagged output.
/// Outputs to stderr with [tag] prefix and severity level.

enum class LogLevel
{
	Debug   = 0,
	Info    = 1,
	Warning = 2,
	Error   = 3
};

class Logger
{
public:
	/// Set the minimum log level (messages below this are suppressed).
	static void SetLevel(LogLevel level);

	/// Get the current log level.
	static LogLevel GetLevel();

	/// Log a message with the given tag and level.
	static void Log(LogLevel level, const std::string &tag, const std::string &msg);

	/// Convenience helpers.
	static void Debug(const std::string &tag, const std::string &msg);
	static void Info(const std::string &tag, const std::string &msg);
	static void Warn(const std::string &tag, const std::string &msg);
	static void Error(const std::string &tag, const std::string &msg);

	/// Parse a log level string ("DEBUG", "INFO", "WARNING", "ERROR").
	static LogLevel ParseLevel(const std::string &levelStr);

private:
	static LogLevel s_level;
};
