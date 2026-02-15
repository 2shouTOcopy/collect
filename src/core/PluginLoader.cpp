#include "PluginLoader.h"

#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <iostream>
#include <algorithm>

// ─── LibHandle RAII ────────────────────────────────────────

PluginLoader::LibHandle::~LibHandle()
{
	if (plugin != nullptr && destroyFn != nullptr)
	{
		destroyFn(plugin);
		plugin = nullptr;
	}
	if (handle != nullptr)
	{
		dlclose(handle);
		handle = nullptr;
	}
}

PluginLoader::LibHandle::LibHandle(LibHandle &&other) noexcept
	: handle(other.handle)
	, plugin(other.plugin)
	, destroyFn(other.destroyFn)
	, name(std::move(other.name))
{
	other.handle = nullptr;
	other.plugin = nullptr;
	other.destroyFn = nullptr;
}

PluginLoader::LibHandle &PluginLoader::LibHandle::operator=(LibHandle &&other) noexcept
{
	if (this != &other)
	{
		if (plugin != nullptr && destroyFn != nullptr)
		{
			destroyFn(plugin);
		}
		if (handle != nullptr)
		{
			dlclose(handle);
		}

		handle = other.handle;
		plugin = other.plugin;
		destroyFn = other.destroyFn;
		name = std::move(other.name);

		other.handle = nullptr;
		other.plugin = nullptr;
		other.destroyFn = nullptr;
	}
	return *this;
}

// ─── PluginLoader ──────────────────────────────────────────

PluginLoader::~PluginLoader()
{
	UnloadAll();
}

IPlugin *PluginLoader::Load(const std::string &pluginName)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto it = m_loaded.find(pluginName);
	if (it != m_loaded.end())
	{
		return it->second.plugin;
	}

	std::string soPath = m_pluginDir + "/" + pluginName + "/" + pluginName + ".so";

	void *handle = dlopen(soPath.c_str(), RTLD_NOW | RTLD_LOCAL);
	if (handle == nullptr)
	{
		std::cerr << "[PluginLoader] dlopen failed for '" << soPath
		          << "': " << dlerror() << "\n";
		return nullptr;
	}

	auto createFn = reinterpret_cast<PfnCreateModule>(dlsym(handle, "CreateModule"));
	if (createFn == nullptr)
	{
		std::cerr << "[PluginLoader] CreateModule not found in '"
		          << soPath << "': " << dlerror() << "\n";
		dlclose(handle);
		return nullptr;
	}

	auto destroyFn = reinterpret_cast<PfnDestroyModule>(dlsym(handle, "DestroyModule"));
	if (destroyFn == nullptr)
	{
		std::cerr << "[PluginLoader] DestroyModule not found in '"
		          << soPath << "': " << dlerror() << "\n";
		dlclose(handle);
		return nullptr;
	}

	IPlugin *plugin = createFn();
	if (plugin == nullptr)
	{
		std::cerr << "[PluginLoader] CreateModule() returned null for '"
		          << pluginName << "'\n";
		dlclose(handle);
		return nullptr;
	}

	LibHandle lib;
	lib.handle = handle;
	lib.plugin = plugin;
	lib.destroyFn = destroyFn;
	lib.name = pluginName;

	m_loaded.emplace(pluginName, std::move(lib));

	std::cerr << "[PluginLoader] Loaded: " << pluginName
	          << " (" << soPath << ")\n";

	return plugin;
}

int PluginLoader::Unload(const std::string &pluginName)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto it = m_loaded.find(pluginName);
	if (it == m_loaded.end())
	{
		std::cerr << "[PluginLoader] Plugin not loaded: " << pluginName << "\n";
		return -1;
	}

	m_loaded.erase(it);
	std::cerr << "[PluginLoader] Unloaded: " << pluginName << "\n";
	return 0;
}

void PluginLoader::UnloadAll()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_loaded.clear();
}

bool PluginLoader::IsLoaded(const std::string &pluginName) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_loaded.find(pluginName) != m_loaded.end();
}

std::vector<std::string> PluginLoader::GetLoadedNames() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::vector<std::string> names;
	names.reserve(m_loaded.size());
	for (const auto &pair : m_loaded)
	{
		names.push_back(pair.first);
	}
	return names;
}

std::vector<std::string> PluginLoader::ListPlugins() const
{
	std::vector<std::string> plugins;

	if (m_pluginDir.empty())
	{
		return plugins;
	}

	DIR *dir = opendir(m_pluginDir.c_str());
	if (dir == nullptr)
	{
		std::cerr << "[PluginLoader] Cannot open plugin dir: "
		          << m_pluginDir << "\n";
		return plugins;
	}

	struct dirent *entry;
	while ((entry = readdir(dir)) != nullptr)
	{
		if (entry->d_name[0] == '.')
		{
			continue;
		}

		std::string name(entry->d_name);
		std::string soPath = m_pluginDir + "/" + name + "/" + name + ".so";

		struct stat st = {};
		if (stat(soPath.c_str(), &st) == 0 && S_ISREG(st.st_mode))
		{
			plugins.push_back(name);
		}
	}

	closedir(dir);
	std::sort(plugins.begin(), plugins.end());
	return plugins;
}
