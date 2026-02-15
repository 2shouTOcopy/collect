#pragma once

#include <string>
#include <vector>
#include <memory>

#include "IPlugin.h"

/// Plugin manager — registers plugins and classifies them by capability.
/// Replaces PluginService singleton. Maintains separate read/write/flush lists.

class PluginManager
{
public:
	PluginManager() = default;
	~PluginManager() = default;

	PluginManager(const PluginManager &) = delete;
	PluginManager &operator=(const PluginManager &) = delete;

	/// Register a plugin (auto-classified by HasRead/HasWrite/HasFlush).
	void Register(IPlugin *plugin);

	/// Configure a plugin by name with key-value pairs.
	int Configure(const std::string &pluginName,
	              const std::string &key,
	              const std::string &value);

	/// Lifecycle: initialize all registered plugins.
	int InitAll();

	/// Lifecycle: shutdown all registered plugins.
	int ShutdownAll();

	/// Flush all write plugins.
	int FlushAll(CdTime timeout);

	/// Get all read plugins (for Dispatcher registration).
	const std::vector<IPlugin *> &GetReadPlugins() const { return m_readPlugins; }

	/// Get all write plugins (for WriteQueue dispatch).
	const std::vector<IPlugin *> &GetWritePlugins() const { return m_writePlugins; }

	/// Find a plugin by name (or nullptr).
	IPlugin *FindPlugin(const std::string &name) const;

	/// Number of registered plugins total.
	size_t PluginCount() const { return m_allPlugins.size(); }

private:
	std::vector<IPlugin *> m_allPlugins;
	std::vector<IPlugin *> m_readPlugins;
	std::vector<IPlugin *> m_writePlugins;
	std::vector<IPlugin *> m_flushPlugins;
};
