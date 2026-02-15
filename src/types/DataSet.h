#pragma once

#include <cstdint>
#include <string>
#include <vector>

/// Data source type enum (replaces DS_TYPE_* macros).
enum class DataSourceType
{
	Counter  = 0,
	Gauge    = 1,
	Derive   = 2,
	Absolute = 3
};

/// A single data source definition within a DataSet.
struct DataSource
{
	std::string name;
	DataSourceType type = DataSourceType::Gauge;
	double min = 0.0;
	double max = 0.0;
};

/// A named collection of data sources (replaces data_set_s).
/// Loaded from types.db.
struct DataSet
{
	std::string type;
	std::vector<DataSource> sources;
};
