#include "ConfigManager.h"

#include <iostream>
#include <algorithm>
#include <cctype>

static const char *TAG = "ConfigManager";

/// Case-insensitive string comparison helper.
static bool EqualsIgnoreCase(const std::string &a, const std::string &b)
{
	if (a.size() != b.size())
	{
		return false;
	}
	for (size_t i = 0; i < a.size(); ++i)
	{
		if (std::tolower(static_cast<unsigned char>(a[i])) !=
		    std::tolower(static_cast<unsigned char>(b[i])))
		{
			return false;
		}
	}
	return true;
}

ConfigManager::ConfigManager()
	: m_pluginDir("/usr/lib/collect/modules")
	, m_typesDbPath("/etc/collect/types.db")
	, m_defaultInterval(10.0)
	, m_hasPluginDir(false)
	, m_hasDefaultInterval(false)
{
}

void ConfigManager::ProcessItem(const ConfigItem &item)
{
	const std::string &key = item.key;

	// LoadPlugin directive — value is the plugin name
	if (EqualsIgnoreCase(key, "LoadPlugin"))
	{
		if (!item.values.empty())
		{
			m_loadPlugins.push_back(item.values[0]);
		}
		return;
	}

	// <Plugin name> block — store for later retrieval
	if (EqualsIgnoreCase(key, "Plugin"))
	{
		m_pluginConfigs.push_back(item);
		return;
	}

	// Known global options with dedicated fields
	if (EqualsIgnoreCase(key, "PluginDir"))
	{
		if (!item.values.empty())
		{
			m_pluginDir = item.values[0];
			m_hasPluginDir = true;
		}
		return;
	}

	if (EqualsIgnoreCase(key, "TypesDB"))
	{
		if (!item.values.empty())
		{
			m_typesDbPath = item.values[0];
		}
		return;
	}

	if (EqualsIgnoreCase(key, "Interval"))
	{
		if (!item.values.empty())
		{
			try
				{
					m_defaultInterval = std::stod(item.values[0]);
					m_hasDefaultInterval = true;
				}
			catch (...)
			{
				std::cerr << "[" << TAG << "] Invalid Interval value: "
				          << item.values[0] << "\n";
			}
		}
		return;
	}

	// Generic global option — store first value in the globals map
	if (!item.values.empty() && item.children.empty())
	{
		m_globals[key] = item.values[0];
	}
}

int ConfigManager::Load(const std::string &configPath)
{
	m_globals.clear();
	m_pluginConfigs.clear();
	m_loadPlugins.clear();
	m_pluginDir = "/usr/lib/collect/modules";
	m_typesDbPath = "/etc/collect/types.db";
	m_defaultInterval = 10.0;
	m_hasPluginDir = false;
	m_hasDefaultInterval = false;

	auto items = ConfigParser::Parse(configPath);
	if (items.empty())
	{
		std::cerr << "[" << TAG << "] No configuration items parsed from "
		          << configPath << "\n";
		return -1;
	}

	for (const auto &item : items)
	{
		ProcessItem(item);
	}

	std::cerr << "[" << TAG << "] Loaded config: "
	          << m_loadPlugins.size() << " plugins, "
	          << m_pluginConfigs.size() << " plugin configs, "
	          << m_globals.size() << " globals\n";

	return 0;
}

std::string ConfigManager::GetGlobal(const std::string &key,
                                      const std::string &defaultValue) const
{
	auto it = m_globals.find(key);
	if (it != m_globals.end())
	{
		return it->second;
	}
	return defaultValue;
}

double ConfigManager::GetGlobalDouble(const std::string &key,
                                       double defaultValue) const
{
	auto it = m_globals.find(key);
	if (it != m_globals.end())
	{
		try
		{
			return std::stod(it->second);
		}
		catch (...)
		{
			std::cerr << "[" << TAG << "] Invalid double for key '"
			          << key << "'\n";
		}
	}
	return defaultValue;
}

std::vector<ConfigItem> ConfigManager::GetPluginConfig(
	const std::string &pluginName) const
{
	std::vector<ConfigItem> result;
	for (const auto &item : m_pluginConfigs)
	{
		if (!item.values.empty() && item.values[0] == pluginName)
		{
			result = item.children;
			break;
		}
	}
	return result;
}
