#pragma once

#include "IOutputFormatter.h"

/// CSV output formatter — generates simple CSV lines with header.

class CsvFormatter : public IOutputFormatter
{
public:
	CsvFormatter() = default;
	~CsvFormatter() override = default;

	std::string Format(const DataSet &ds, const ValueList &vl) override;
	std::string ContentType() const override { return "text/csv"; }

	/// Generate CSV header line from a DataSet.
	static std::string Header(const DataSet &ds);
};
