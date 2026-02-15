#include "JsonFormatter.h"
#include "utils/cJSON.h"
#include "utils/TimeUtils.h"

#include <cstring>

/// Helper: convert DataSourceType enum to human-readable string.
static const char *DataTypeToString(DataSourceType type)
{
	switch (type)
	{
		case DataSourceType::Counter:  return "counter";
		case DataSourceType::Gauge:    return "gauge";
		case DataSourceType::Derive:   return "derive";
		case DataSourceType::Absolute: return "absolute";
	}
	return "unknown";
}

/// Helper: add a single Value to a cJSON object.
static void AddValueToJson(cJSON *obj, const Value &val, const DataSource &src)
{
	switch (val.type)
	{
		case DataSourceType::Counter:
			cJSON_AddNumberToObject(obj, "value",
				static_cast<double>(val.data.counter));
			break;
		case DataSourceType::Gauge:
			cJSON_AddNumberToObject(obj, "value", val.data.gauge);
			break;
		case DataSourceType::Derive:
			cJSON_AddNumberToObject(obj, "value",
				static_cast<double>(val.data.derive));
			break;
		case DataSourceType::Absolute:
			cJSON_AddNumberToObject(obj, "value",
				static_cast<double>(val.data.absolute));
			break;
	}

	cJSON_AddStringToObject(obj, "data_type", DataTypeToString(val.type));
	cJSON_AddStringToObject(obj, "ds_name", src.name.c_str());

	// Include min/max bounds from DataSource if meaningful
	if (src.min != 0.0 || src.max != 0.0)
	{
		cJSON_AddNumberToObject(obj, "min", src.min);
		cJSON_AddNumberToObject(obj, "max", src.max);
	}
}

// ─── Format single ValueList ────────────────────────────────

std::string JsonFormatter::Format(const DataSet &ds, const ValueList &vl)
{
	cJSON *root = cJSON_CreateObject();

	// Timestamp (ISO 8601)
	if (vl.time.Raw() != 0)
	{
		std::string ts = TimeUtils::ToIso8601(vl.time);
		cJSON_AddStringToObject(root, "timestamp", ts.c_str());
	}
	else
	{
		std::string ts = TimeUtils::NowIso8601();
		cJSON_AddStringToObject(root, "timestamp", ts.c_str());
	}

	// Host
	if (!m_host.empty())
	{
		cJSON_AddStringToObject(root, "host", m_host.c_str());
	}

	// Plugin metadata
	cJSON_AddStringToObject(root, "plugin", vl.plugin.c_str());
	if (!vl.pluginInstance.empty())
	{
		cJSON_AddStringToObject(root, "plugin_instance", vl.pluginInstance.c_str());
	}

	cJSON_AddStringToObject(root, "type", vl.type.c_str());
	if (!vl.typeInstance.empty())
	{
		cJSON_AddStringToObject(root, "type_instance", vl.typeInstance.c_str());
	}

	// Interval
	if (vl.interval.Raw() != 0)
	{
		cJSON_AddNumberToObject(root, "interval_sec", vl.interval.ToDouble());
	}

	// Values — if single value, flatten; if multiple, use array
	if (vl.values.size() == 1 && ds.sources.size() >= 1)
	{
		AddValueToJson(root, vl.values[0], ds.sources[0]);
	}
	else if (vl.values.size() > 1)
	{
		cJSON *valuesArr = cJSON_AddArrayToObject(root, "values");
		for (size_t i = 0; i < vl.values.size(); ++i)
		{
			cJSON *valObj = cJSON_CreateObject();

			if (i < ds.sources.size())
			{
				AddValueToJson(valObj, vl.values[i], ds.sources[i]);
			}
			else
			{
				// Fallback: no matching DataSource
				DataSource fallback;
				fallback.name = "value_" + std::to_string(i);
				fallback.type = vl.values[i].type;
				AddValueToJson(valObj, vl.values[i], fallback);
			}

			cJSON_AddItemToArray(valuesArr, valObj);
		}
	}

	// DataSet type info
	cJSON_AddStringToObject(root, "ds_type", ds.type.c_str());

	// Serialize
	char *str = cJSON_PrintUnformatted(root);
	std::string result(str != nullptr ? str : "{}");
	if (str != nullptr)
	{
		cJSON_free(str);
	}
	cJSON_Delete(root);

	return result;
}

// ─── Format batch ────────────────────────────────────────────

std::string JsonFormatter::FormatBatch(
	const std::vector<std::pair<DataSet, ValueList>> &entries,
	const std::string &host)
{
	cJSON *root = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "timestamp", TimeUtils::NowIso8601().c_str());
	cJSON_AddStringToObject(root, "host", host.c_str());
	cJSON_AddNumberToObject(root, "count", static_cast<double>(entries.size()));

	cJSON *metrics = cJSON_AddArrayToObject(root, "metrics");

	// Temporarily set host for individual formatting
	std::string savedHost = m_host;
	m_host = "";  // Don't duplicate host in each metric

	for (const auto &entry : entries)
	{
		std::string metricJson = Format(entry.first, entry.second);
		cJSON *metricObj = cJSON_Parse(metricJson.c_str());
		if (metricObj != nullptr)
		{
			cJSON_AddItemToArray(metrics, metricObj);
		}
	}

	m_host = savedHost;

	// Serialize
	char *str = cJSON_PrintUnformatted(root);
	std::string result(str != nullptr ? str : "{\"metrics\":[]}");
	if (str != nullptr)
	{
		cJSON_free(str);
	}
	cJSON_Delete(root);

	return result;
}
