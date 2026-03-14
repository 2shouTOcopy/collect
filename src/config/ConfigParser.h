#pragma once

#include <string>
#include <vector>
#include <istream>

/// A single configuration item (key + values + optional children).
/// Represents both top-level directives and <Block> sections.
struct ConfigItem
{
	std::string key;
	std::vector<std::string> values;
	std::vector<ConfigItem> children;
};

/// Parser for collect.conf configuration files (oconfig format).
/// Supports:
///   - '#' comments
///   - Key value1 value2 ... (simple directives)
///   - <Block arg> ... </Block> (nested blocks)
///   - Quoted strings with '"' or '\''
class ConfigParser
{
public:
	/// Parse a configuration file, returning the root items.
	static std::vector<ConfigItem> Parse(const std::string &filePath);

	/// Parse a string buffer (useful for unit tests).
	static std::vector<ConfigItem> ParseString(const std::string &input);

private:
	/// Trim leading and trailing whitespace.
	static std::string Trim(const std::string &s);

	/// Remove enclosing quotes (" or ') if present.
	static std::string StripQuotes(const std::string &s);

	/// Split a line into tokens, respecting quoted strings.
	static std::vector<std::string> TokenizeLine(const std::string &line);

	/// Recursively parse a block from the stream into items.
	/// Returns 0 on success, -1 on error.
	static int ParseBlock(std::istream &stream,
	                       std::vector<ConfigItem> &items,
	                       int &lineNum);
};
