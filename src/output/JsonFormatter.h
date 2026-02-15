#pragma once

#include <string>
#include <vector>
#include <utility>

#include "IOutputFormatter.h"
#include "types/DataSet.h"
#include "types/ValueList.h"

/// JSON output formatter — generates AI/LLM friendly structured JSON.
/// Uses cJSON for serialization. Produces self-describing metrics with
/// timestamp, plugin metadata, unit, data_type, and source context.

class JsonFormatter : public IOutputFormatter
{
public:
	JsonFormatter() = default;
	~JsonFormatter() override = default;

	/// Format a single ValueList as a JSON object string.
	std::string Format(const DataSet &ds, const ValueList &vl) override;

	/// MIME content type.
	std::string ContentType() const override { return "application/json"; }

	/// Format multiple entries as a batch JSON array.
	/// Output: {"host":"...","timestamp":"...","metrics":[...]}
	std::string FormatBatch(const std::vector<std::pair<DataSet, ValueList>> &entries,
	                        const std::string &host);

	/// Set the hostname to include in output.
	void SetHost(const std::string &host) { m_host = host; }

private:
	std::string m_host;
};
