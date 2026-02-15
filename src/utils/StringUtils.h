#pragma once

#include <string>
#include <sstream>
#include <vector>

/// String utility functions.

namespace StringUtils
{
	/// Trim whitespace from both ends.
	inline std::string Trim(const std::string &s)
	{
		size_t start = s.find_first_not_of(" \t\r\n");
		if (start == std::string::npos)
		{
			return "";
		}
		size_t end = s.find_last_not_of(" \t\r\n");
		return s.substr(start, end - start + 1);
	}

	/// Split a string by delimiter.
	inline std::vector<std::string> Split(const std::string &s, char delim)
	{
		std::vector<std::string> parts;
		std::istringstream stream(s);
		std::string part;
		while (std::getline(stream, part, delim))
		{
			parts.push_back(part);
		}
		return parts;
	}

	/// Case-insensitive comparison.
	inline bool EqualsIgnoreCase(const std::string &a, const std::string &b)
	{
		if (a.size() != b.size())
		{
			return false;
		}
		for (size_t i = 0; i < a.size(); ++i)
		{
			if (std::tolower(a[i]) != std::tolower(b[i]))
			{
				return false;
			}
		}
		return true;
	}
}
