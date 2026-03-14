#include "TypesDb.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cctype>
#include <algorithm>

static const char *TAG = "TypesDb";

/// Trim leading and trailing whitespace from a string.
static std::string TrimStr(const std::string &s)
{
	size_t start = 0;
	while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
	{
		++start;
	}
	if (start == s.size())
	{
		return "";
	}

	size_t end = s.size() - 1;
	while (end > start && std::isspace(static_cast<unsigned char>(s[end])))
	{
		--end;
	}
	return s.substr(start, end - start + 1);
}

/// Parse a single data source spec: "name:TYPE:min:max"
/// Returns true on success.
static bool ParseDataSource(const std::string &spec, DataSource &ds)
{
	std::string trimmed = TrimStr(spec);
	if (trimmed.empty())
	{
		return false;
	}

	// Split by ':'
	std::vector<std::string> parts;
	std::istringstream stream(trimmed);
	std::string part;
	while (std::getline(stream, part, ':'))
	{
		parts.push_back(TrimStr(part));
	}

	if (parts.size() != 4)
	{
		std::cerr << "[" << TAG << "] Invalid data source spec: "
		          << trimmed << " (expected name:TYPE:min:max)\n";
		return false;
	}

	// Name
	ds.name = parts[0];

	// Type
	std::string typeStr = parts[1];
	std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(),
	               [](unsigned char c) { return std::toupper(c); });

	if (typeStr == "GAUGE")
	{
		ds.type = DataSourceType::Gauge;
	}
	else if (typeStr == "COUNTER")
	{
		ds.type = DataSourceType::Counter;
	}
	else if (typeStr == "DERIVE")
	{
		ds.type = DataSourceType::Derive;
	}
	else if (typeStr == "ABSOLUTE")
	{
		ds.type = DataSourceType::Absolute;
	}
	else
	{
		std::cerr << "[" << TAG << "] Unknown data source type: "
		          << typeStr << "\n";
		return false;
	}

	// Min: "U" means unbounded (NaN)
	if (parts[2] == "U" || parts[2] == "u")
	{
		ds.min = NAN;
	}
	else
	{
		try
		{
			ds.min = std::stod(parts[2]);
		}
		catch (...)
		{
			std::cerr << "[" << TAG << "] Invalid min value: "
			          << parts[2] << "\n";
			return false;
		}
	}

	// Max: "U" means unbounded (NaN)
	if (parts[3] == "U" || parts[3] == "u")
	{
		ds.max = NAN;
	}
	else
	{
		try
		{
			ds.max = std::stod(parts[3]);
		}
		catch (...)
		{
			std::cerr << "[" << TAG << "] Invalid max value: "
			          << parts[3] << "\n";
			return false;
		}
	}

	return true;
}

int TypesDb::Load(const std::string &filePath)
{
	std::ifstream file(filePath);
	if (!file.is_open())
	{
		std::cerr << "[" << TAG << "] Cannot open file: "
		          << filePath << "\n";
		return -1;
	}

	int lineNum = 0;
	int loaded = 0;
	std::string rawLine;

	while (std::getline(file, rawLine))
	{
		++lineNum;
		std::string line = TrimStr(rawLine);

		// Skip empty lines and comments
		if (line.empty() || line[0] == '#')
		{
			continue;
		}

		// Find first whitespace to split type name from data sources
		size_t splitPos = 0;
		while (splitPos < line.size() &&
		       !std::isspace(static_cast<unsigned char>(line[splitPos])))
		{
			++splitPos;
		}

		if (splitPos >= line.size())
		{
			std::cerr << "[" << TAG << "] No data sources on line "
			          << lineNum << ": " << line << "\n";
			continue;
		}

		std::string typeName = line.substr(0, splitPos);
		std::string remainder = TrimStr(line.substr(splitPos));

		// Split remainder by ',' to get individual data source specs
		DataSet ds;
		ds.type = typeName;

		std::istringstream dsStream(remainder);
		std::string dsSpec;
		bool valid = true;

		while (std::getline(dsStream, dsSpec, ','))
		{
			std::string trimmedSpec = TrimStr(dsSpec);
			if (trimmedSpec.empty())
			{
				continue;
			}

			DataSource source;
			if (!ParseDataSource(trimmedSpec, source))
			{
				std::cerr << "[" << TAG << "] Skipping invalid entry on line "
				          << lineNum << "\n";
				valid = false;
				break;
			}
			ds.sources.push_back(std::move(source));
		}

		if (valid && !ds.sources.empty())
		{
			m_types[typeName] = std::move(ds);
			++loaded;
		}
	}

	std::cerr << "[" << TAG << "] Loaded " << loaded << " type definitions from "
	          << filePath << "\n";
	return 0;
}

const DataSet *TypesDb::GetDataSet(const std::string &typeName) const
{
	auto it = m_types.find(typeName);
	if (it != m_types.end())
	{
		return &(it->second);
	}
	return nullptr;
}
