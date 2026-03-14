#pragma once

#include <string>
#include <map>
#include <vector>

#include "ConfigParser.h"

/// Configuration manager — parses collect.conf and manages global settings.
/// Extracts global options, LoadPlugin directives, and <Plugin> blocks.

class ConfigManager
{
public:
	ConfigManager();
	~ConfigManager() = default;

	/// Load and parse the main configuration file.
	int Load(const std::string &configPath);

	/// Get a global string option (e.g. "Hostname", "BaseDir").
	std::string GetGlobal(const std::string &key,
	                       const std::string &defaultValue = "") const;

	/// Get a global double option (e.g. "Interval").
	double GetGlobalDouble(const std::string &key, double defaultValue = 0.0) const;

	/// Get the configured plugin directory.
	const std::string &GetPluginDir() const { return m_pluginDir; }

	/// Get the TypesDB path.
	const std::string &GetTypesDbPath() const { return m_typesDbPath; }

	/// Get the default read interval (seconds).
	double GetDefaultInterval() const { return m_defaultInterval; }

	/// Get the list of plugins to load (from LoadPlugin directives).
	const std::vector<std::string> &GetLoadPlugins() const { return m_loadPlugins; }

	/// Get plugin-specific config items (from <Plugin name> blocks).
	std::vector<ConfigItem> GetPluginConfig(const std::string &pluginName) const;

private:
	/// Process a single top-level config item.
	void ProcessItem(const ConfigItem &item);

	std::map<std::string, std::string> m_globals;
	std::string m_pluginDir;
	std::string m_typesDbPath;
	double m_defaultInterval;

	std::vector<std::string> m_loadPlugins;
	std::vector<ConfigItem> m_pluginConfigs;
};
