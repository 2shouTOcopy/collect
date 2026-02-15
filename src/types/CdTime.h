#pragma once

#include <cstdint>
#include <chrono>

/// Type-safe wrapper around cdtime_t (nanoseconds since epoch, stored as uint64_t).
/// Replaces the raw `typedef uint64_t cdtime_t` and utils_time.c helpers.

class CdTime
{
public:
	CdTime() : ns_(0) {}

	explicit CdTime(uint64_t ns) : ns_(ns) {}

	/// Factory: current wall-clock time.
	static CdTime Now()
	{
		using namespace std::chrono;
		auto tp = steady_clock::now().time_since_epoch();
		return CdTime(static_cast<uint64_t>(
			duration_cast<nanoseconds>(tp).count()));
	}

	/// Factory: from seconds (double).
	static CdTime FromDouble(double seconds)
	{
		return CdTime(static_cast<uint64_t>(seconds * 1e9));
	}

	/// Factory: from std::chrono duration.
	template <typename Rep, typename Period>
	static CdTime FromDuration(std::chrono::duration<Rep, Period> d)
	{
		return CdTime(static_cast<uint64_t>(
			std::chrono::duration_cast<std::chrono::nanoseconds>(d).count()));
	}

	/// Convert to seconds (double).
	double ToDouble() const
	{
		return static_cast<double>(ns_) / 1e9;
	}

	/// Convert to std::chrono::nanoseconds.
	std::chrono::nanoseconds ToDuration() const
	{
		return std::chrono::nanoseconds(ns_);
	}

	/// Raw nanosecond value.
	uint64_t Raw() const { return ns_; }

	/// Comparison operators.
	bool operator<(const CdTime &rhs) const { return ns_ < rhs.ns_; }
	bool operator<=(const CdTime &rhs) const { return ns_ <= rhs.ns_; }
	bool operator>(const CdTime &rhs) const { return ns_ > rhs.ns_; }
	bool operator>=(const CdTime &rhs) const { return ns_ >= rhs.ns_; }
	bool operator==(const CdTime &rhs) const { return ns_ == rhs.ns_; }
	bool operator!=(const CdTime &rhs) const { return ns_ != rhs.ns_; }

	/// Arithmetic.
	CdTime operator+(const CdTime &rhs) const { return CdTime(ns_ + rhs.ns_); }
	CdTime operator-(const CdTime &rhs) const { return CdTime(ns_ - rhs.ns_); }
	CdTime &operator+=(const CdTime &rhs) { ns_ += rhs.ns_; return *this; }

private:
	uint64_t ns_;
};
