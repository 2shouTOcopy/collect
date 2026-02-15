#pragma once

#include <string>

/// Output formatter interface for serializing ValueList to different formats.
/// Implementations: JsonFormatter (AI/LLM friendly), CsvFormatter.

class DataSet;
class ValueList;

class IOutputFormatter
{
public:
	virtual ~IOutputFormatter() = default;

	/// Format a single ValueList + DataSet into a string representation.
	virtual std::string Format(const DataSet &ds, const ValueList &vl) = 0;

	/// MIME content type (e.g. "application/json", "text/csv").
	virtual std::string ContentType() const = 0;
};
