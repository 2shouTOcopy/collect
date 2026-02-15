#pragma once

#include "types/CdTime.h"

#include <ctime>
#include <string>
#include <chrono>

/// Time utility functions — wraps CdTime conversions for common use cases.

namespace TimeUtils
{
	/// Get current wall-clock time as ISO 8601 string.
	inline std::string NowIso8601()
	{
		auto now = std::chrono::system_clock::now();
		auto timeT = std::chrono::system_clock::to_time_t(now);
		struct tm tmBuf = {};
		localtime_r(&timeT, &tmBuf);

		char buf[64] = {};
		strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", &tmBuf);
		return std::string(buf);
	}

	/// Convert CdTime to ISO 8601 string.
	inline std::string ToIso8601(CdTime t)
	{
		auto dur = t.ToDuration();
		auto tp = std::chrono::system_clock::time_point(
			std::chrono::duration_cast<std::chrono::system_clock::duration>(dur));
		auto timeT = std::chrono::system_clock::to_time_t(tp);
		struct tm tmBuf = {};
		localtime_r(&timeT, &tmBuf);

		char buf[64] = {};
		strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", &tmBuf);
		return std::string(buf);
	}
}
