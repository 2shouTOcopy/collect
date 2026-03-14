#include "ConfigParser.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>

static const char *TAG = "ConfigParser";

std::string ConfigParser::Trim(const std::string &s)
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

std::string ConfigParser::StripQuotes(const std::string &s)
{
	if (s.size() >= 2)
	{
		char front = s.front();
		char back = s.back();
		if ((front == '"' && back == '"') ||
		    (front == '\'' && back == '\''))
		{
			return s.substr(1, s.size() - 2);
		}
	}
	return s;
}

std::vector<std::string> ConfigParser::TokenizeLine(const std::string &line)
{
	std::vector<std::string> tokens;
	bool inQuotes = false;
	char quoteChar = 0;
	std::ostringstream current;

	for (char c : line)
	{
		if (!inQuotes)
		{
			if (c == '"' || c == '\'')
			{
				inQuotes = true;
				quoteChar = c;
				current << c;
			}
			else if (std::isspace(static_cast<unsigned char>(c)))
			{
				if (current.tellp() > 0)
				{
					tokens.push_back(current.str());
					current.str("");
					current.clear();
				}
			}
			else
			{
				current << c;
			}
		}
		else
		{
			current << c;
			if (c == quoteChar)
			{
				inQuotes = false;
			}
		}
	}

	if (current.tellp() > 0)
	{
		tokens.push_back(current.str());
	}

	// Strip quotes from each token
	for (auto &t : tokens)
	{
		t = StripQuotes(Trim(t));
	}

	return tokens;
}

int ConfigParser::ParseBlock(std::istream &stream,
                              std::vector<ConfigItem> &items,
                              int &lineNum)
{
	std::string rawLine;
	while (std::getline(stream, rawLine))
	{
		++lineNum;
		std::string line = Trim(rawLine);

		// Skip empty lines and comments
		if (line.empty() || line[0] == '#')
		{
			continue;
		}

		// Block end: </...>
		if (line.size() >= 2 && line[0] == '<' && line[1] == '/')
		{
			return 0;
		}

		// Block start: <BlockName arg1 arg2 ...>
		if (line[0] == '<' && line[1] != '/')
		{
			size_t endPos = line.find('>');
			if (endPos == std::string::npos)
			{
				endPos = line.size();
			}

			std::string content = Trim(line.substr(1, endPos - 1));
			auto blockTokens = TokenizeLine(content);
			if (blockTokens.empty())
			{
				std::cerr << "[" << TAG << "] Empty block at line "
				          << lineNum << "\n";
				return -1;
			}

			ConfigItem blockItem;
			blockItem.key = blockTokens[0];
			for (size_t i = 1; i < blockTokens.size(); ++i)
			{
				blockItem.values.push_back(blockTokens[i]);
			}

			// Recursively parse block children
			int status = ParseBlock(stream, blockItem.children, lineNum);
			if (status != 0)
			{
				return status;
			}

			items.push_back(std::move(blockItem));
			continue;
		}

		// Simple directive: Key value1 value2 ...
		auto tokens = TokenizeLine(line);
		if (!tokens.empty())
		{
			ConfigItem item;
			item.key = tokens[0];
			for (size_t i = 1; i < tokens.size(); ++i)
			{
				item.values.push_back(tokens[i]);
			}
			items.push_back(std::move(item));
		}
	}

	return 0;
}

std::vector<ConfigItem> ConfigParser::Parse(const std::string &filePath)
{
	std::ifstream file(filePath);
	if (!file.is_open())
	{
		std::cerr << "[" << TAG << "] Cannot open file: "
		          << filePath << "\n";
		return {};
	}

	std::vector<ConfigItem> items;
	int lineNum = 0;

	int status = ParseBlock(file, items, lineNum);
	if (status != 0)
	{
		std::cerr << "[" << TAG << "] Parse error in " << filePath
		          << " at line " << lineNum << "\n";
		return {};
	}

	return items;
}

std::vector<ConfigItem> ConfigParser::ParseString(const std::string &input)
{
	std::istringstream stream(input);
	std::vector<ConfigItem> items;
	int lineNum = 0;

	int status = ParseBlock(stream, items, lineNum);
	if (status != 0)
	{
		return {};
	}

	return items;
}
