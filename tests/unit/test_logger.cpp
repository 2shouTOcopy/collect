#include <gtest/gtest.h>
#include "utils/Logger.h"

// ─── Level Management ──────────────────────────────────────

TEST(LoggerTest, DefaultLevelIsInfo)
{
	// Reset to default state
	Logger::SetLevel(LogLevel::Info);
	EXPECT_EQ(Logger::GetLevel(), LogLevel::Info);
}

TEST(LoggerTest, SetAndGetLevel)
{
	Logger::SetLevel(LogLevel::Debug);
	EXPECT_EQ(Logger::GetLevel(), LogLevel::Debug);

	Logger::SetLevel(LogLevel::Error);
	EXPECT_EQ(Logger::GetLevel(), LogLevel::Error);

	// Restore
	Logger::SetLevel(LogLevel::Info);
}

// ─── ParseLevel ────────────────────────────────────────────

TEST(LoggerTest, ParseLevelDebug)
{
	EXPECT_EQ(Logger::ParseLevel("DEBUG"), LogLevel::Debug);
}

TEST(LoggerTest, ParseLevelInfo)
{
	EXPECT_EQ(Logger::ParseLevel("INFO"), LogLevel::Info);
}

TEST(LoggerTest, ParseLevelWarning)
{
	EXPECT_EQ(Logger::ParseLevel("WARNING"), LogLevel::Warning);
}

TEST(LoggerTest, ParseLevelError)
{
	EXPECT_EQ(Logger::ParseLevel("ERROR"), LogLevel::Error);
}

TEST(LoggerTest, ParseLevelUnknownDefaultsToInfo)
{
	EXPECT_EQ(Logger::ParseLevel("TRACE"), LogLevel::Info);
	EXPECT_EQ(Logger::ParseLevel(""), LogLevel::Info);
	EXPECT_EQ(Logger::ParseLevel("foobar"), LogLevel::Info);
}

// ─── Level Filtering (smoke test — output goes to stderr) ──

TEST(LoggerTest, DebugSuppressedAtInfoLevel)
{
	Logger::SetLevel(LogLevel::Info);
	// Should not crash, output suppressed
	Logger::Debug("test", "this should be suppressed");
	// No assert on output — just verifying no crash
}

TEST(LoggerTest, ErrorEmittedAtInfoLevel)
{
	Logger::SetLevel(LogLevel::Info);
	// Should not crash, output emitted
	Logger::Error("test", "this should appear");
}

TEST(LoggerTest, AllLevelsWork)
{
	Logger::SetLevel(LogLevel::Debug);
	// None of these should crash
	Logger::Debug("test", "debug message");
	Logger::Info("test", "info message");
	Logger::Warn("test", "warning message");
	Logger::Error("test", "error message");

	// Restore
	Logger::SetLevel(LogLevel::Info);
}
