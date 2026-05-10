#include "CsvFormatter.h"
#include "types/DataSet.h"
#include "types/ValueList.h"

#include <iomanip>
#include <sstream>

namespace
{
std::string EscapeCsvField(const std::string &field)
{
	bool needsQuotes = false;
	for (char ch : field)
	{
		if (ch == ',' || ch == '"' || ch == '\n' || ch == '\r')
		{
			needsQuotes = true;
			break;
		}
	}

	if (!needsQuotes)
	{
		return field;
	}

	std::string escaped;
	escaped.reserve(field.size() + 2);
	escaped.push_back('"');
	for (char ch : field)
	{
		if (ch == '"')
		{
			escaped.push_back('"');
		}
		escaped.push_back(ch);
	}
	escaped.push_back('"');
	return escaped;
}

void AppendValue(std::ostringstream &oss, const Value &value)
{
	if (value.IsGauge())
	{
		oss << value.AsGauge();
	}
	else if (value.IsDerive())
	{
		oss << value.AsDerive();
	}
	else if (value.IsCounter())
	{
		oss << value.AsCounter();
	}
	else if (value.IsAbsolute())
	{
		oss << value.AsAbsolute();
	}
}
}

std::string CsvFormatter::Format(const DataSet &ds, const ValueList &vl)
{
	(void)ds;

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(3) << vl.time.ToDouble();

	for (const auto &value : vl.values)
	{
		oss << ',';
		AppendValue(oss, value);
	}

	return oss.str();
}

std::string CsvFormatter::Header(const DataSet &ds)
{
	std::ostringstream oss;
	oss << "epoch";

	for (const auto &src : ds.sources)
	{
		oss << ',' << EscapeCsvField(src.name);
	}

	return oss.str();
}

std::string CsvFormatter::Header(const DataSet &ds, const ValueList &vl)
{
	std::ostringstream oss;
	oss << "epoch";

	for (size_t i = 0; i < vl.values.size(); ++i)
	{
		std::string name = (i < ds.sources.size())
			? ds.sources[i].name
			: "value_" + std::to_string(i);
		oss << ',' << EscapeCsvField(name);
	}

	return oss.str();
}
