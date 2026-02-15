#pragma once

#include <string>
#include <map>
#include <functional>

/// Configuration manager — parses collect.conf and manages global settings.
/// Replaces the old ConfigManager singleton with a cleaner interface.

class ConfigManager
{
public:
	ConfigManager();
	~ConfigManager() = default;

	/// Load and parse the main configuration file.
	int Load(const std::string &configPath);

	/// Get a global string option (e.g. "PluginDir", "Interval").
	std::string GetGlobal(const std::string &key,
	                       const std::string &defaultValue = "") const;

	/// Get a global double option.
	double GetGlobalDouble(const std::string &key, double defaultValue = 0.0) const;

	/// Get the configured plugin directory.
	const std::string &GetPluginDir() const { return m_pluginDir; }

	/// Get the TypesDB path.
	const std::string &GetTypesDbPath() const { return m_typesDbPath; }

	/// Get the default read interval (seconds).
	double GetDefaultInterval() const { return m_defaultInterval; }

private:
	std::map<std::string, std::string> m_globals;
	std::string m_pluginDir;
	std::string m_typesDbPath;
	double m_defaultInterval;
};
