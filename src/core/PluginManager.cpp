#include "PluginManager.h"

#include <iostream>
#include <algorithm>

void PluginManager::Register(IPlugin *plugin)
{
	if (plugin == nullptr)
	{
		std::cerr << "[PluginManager] Cannot register null plugin\n";
		return;
	}

	m_allPlugins.push_back(plugin);

	if (plugin->HasRead())
	{
		m_readPlugins.push_back(plugin);
		std::cerr << "[PluginManager] Registered read plugin: "
		          << plugin->Name() << "\n";
	}

	if (plugin->HasWrite())
	{
		m_writePlugins.push_back(plugin);
		std::cerr << "[PluginManager] Registered write plugin: "
		          << plugin->Name() << "\n";
	}

	if (plugin->HasFlush())
	{
		m_flushPlugins.push_back(plugin);
	}
}

int PluginManager::Configure(const std::string &pluginName,
                              const std::string &key,
                              const std::string &value)
{
	IPlugin *plugin = FindPlugin(pluginName);
	if (plugin == nullptr)
	{
		std::cerr << "[PluginManager] Plugin not found: " << pluginName << "\n";
		return -1;
	}
	return plugin->Configure(key, value);
}

int PluginManager::InitAll()
{
	int failures = 0;
	for (auto *plugin : m_allPlugins)
	{
		int ret = plugin->Init();
		if (ret != 0)
		{
			std::cerr << "[PluginManager] Init failed for: "
			          << plugin->Name() << " (ret=" << ret << ")\n";
			++failures;
		}
	}
	return failures;
}

int PluginManager::ShutdownAll()
{
	int failures = 0;
	for (auto *plugin : m_allPlugins)
	{
		int ret = plugin->Shutdown();
		if (ret != 0)
		{
			std::cerr << "[PluginManager] Shutdown failed for: "
			          << plugin->Name() << "\n";
			++failures;
		}
	}
	return failures;
}

int PluginManager::FlushAll(CdTime timeout)
{
	int failures = 0;
	for (auto *plugin : m_flushPlugins)
	{
		int ret = plugin->Flush(timeout);
		if (ret != 0)
		{
			std::cerr << "[PluginManager] Flush failed for: "
			          << plugin->Name() << "\n";
			++failures;
		}
	}
	return failures;
}

IPlugin *PluginManager::FindPlugin(const std::string &name) const
{
	auto it = std::find_if(m_allPlugins.begin(), m_allPlugins.end(),
		[&name](const IPlugin *p) { return p->Name() == name; });

	return (it != m_allPlugins.end()) ? *it : nullptr;
}
