#pragma once

#include <string>
#include <vector>

/// Parser for collect.conf configuration files (oconfig format).
/// Replaces the old oconfig/ directory.

struct ConfigItem
{
	std::string key;
	std::vector<std::string> values;
	std::vector<ConfigItem> children;
};

class ConfigParser
{
public:
	/// Parse a configuration file, returning the root items.
	static std::vector<ConfigItem> Parse(const std::string &filePath);
};
