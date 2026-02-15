#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>

#include "IPlugin.h"

/// Dynamic .so plugin loader with RAII handle management.
/// Replaces ModuleLoader — fixes race conditions and memory leaks.

class PluginLoader
{
public:
	PluginLoader() = default;
	~PluginLoader();

	PluginLoader(const PluginLoader &) = delete;
	PluginLoader &operator=(const PluginLoader &) = delete;

	/// Set the base directory for plugin .so files.
	void SetDir(const std::string &dir) { m_pluginDir = dir; }

	/// Get the plugin directory.
	const std::string &GetDir() const { return m_pluginDir; }

	/// Load a plugin. Returns the plugin interface pointer, or nullptr on error.
	/// The PluginLoader retains ownership of the IPlugin.
	IPlugin *Load(const std::string &pluginName);

	/// Unload a specific plugin.
	int Unload(const std::string &pluginName);

	/// Unload all plugins (for shutdown).
	void UnloadAll();

	/// Check if a plugin is loaded.
	bool IsLoaded(const std::string &pluginName) const;

	/// Get names of all loaded plugins.
	std::vector<std::string> GetLoadedNames() const;

	/// Scan plugin directory for available plugins (subdirectories containing .so).
	std::vector<std::string> ListPlugins() const;

private:
	/// RAII wrapper for a loaded .so library.
	struct LibHandle
	{
		void *handle = nullptr;
		IPlugin *plugin = nullptr;
		PfnDestroyModule destroyFn = nullptr;
		std::string name;

		~LibHandle();

		LibHandle() = default;
		LibHandle(const LibHandle &) = delete;
		LibHandle &operator=(const LibHandle &) = delete;
		LibHandle(LibHandle &&other) noexcept;
		LibHandle &operator=(LibHandle &&other) noexcept;
	};

	std::string m_pluginDir;
	std::map<std::string, LibHandle> m_loaded;
	mutable std::mutex m_mutex;
};
