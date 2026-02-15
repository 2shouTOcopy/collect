#pragma once

#include <cstdint>
#include <cmath>
#include <string>
#include <vector>

#include "CdTime.h"
#include "DataSet.h"

/// Tagged union for a single metric value (replaces the C union value_u).
/// C++14 doesn't have std::variant, so we use an explicit tag.
struct Value
{
	DataSourceType type = DataSourceType::Gauge;

	union
	{
		uint64_t counter;
		double   gauge;
		int64_t  derive;
		uint64_t absolute;
	} data = {};

	/// Factory helpers.
	static Value Gauge(double v)
	{
		Value val;
		val.type = DataSourceType::Gauge;
		val.data.gauge = v;
		return val;
	}

	static Value Derive(int64_t v)
	{
		Value val;
		val.type = DataSourceType::Derive;
		val.data.derive = v;
		return val;
	}

	static Value Counter(uint64_t v)
	{
		Value val;
		val.type = DataSourceType::Counter;
		val.data.counter = v;
		return val;
	}

	static Value Absolute(uint64_t v)
	{
		Value val;
		val.type = DataSourceType::Absolute;
		val.data.absolute = v;
		return val;
	}

	/// Type query helpers.
	bool IsGauge()    const { return type == DataSourceType::Gauge; }
	bool IsDerive()   const { return type == DataSourceType::Derive; }
	bool IsCounter()  const { return type == DataSourceType::Counter; }
	bool IsAbsolute() const { return type == DataSourceType::Absolute; }

	/// Get typed value (returns default if wrong type).
	double   AsGauge()    const { return IsGauge()   ? data.gauge   : NAN; }
	int64_t  AsDerive()   const { return IsDerive()  ? data.derive  : 0; }
	uint64_t AsCounter()  const { return IsCounter() ? data.counter : 0; }
	uint64_t AsAbsolute() const { return IsAbsolute()? data.absolute: 0; }
};

/// A single data submission from a plugin (replaces value_list_s).
/// Uses RAII: std::string instead of char[128], std::vector instead of raw ptr.
struct ValueList
{
	std::string plugin;
	std::string pluginInstance;
	std::string type;
	std::string typeInstance;

	std::vector<Value> values;

	CdTime time;
	CdTime interval;
};
