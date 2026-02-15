#include "ConfigManager.h"

#include <iostream>

ConfigManager::ConfigManager()
	: m_pluginDir("/usr/lib/collect/modules")
	, m_typesDbPath("/etc/collect/types.db")
	, m_defaultInterval(10.0)
{
}

int ConfigManager::Load(const std::string &configPath)
{
	// TODO Phase 1+: delegate to ConfigParser for actual .conf parsing
	(void)configPath;
	std::cerr << "[ConfigManager] Load: stub\n";
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
			std::cerr << "[ConfigManager] Invalid double for key '" << key << "'\n";
		}
	}
	return defaultValue;
}
