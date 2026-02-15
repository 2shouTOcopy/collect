#pragma once

#include <string>
#include <memory>

#include "core/PluginManager.h"
#include "core/Dispatcher.h"
#include "core/WriteQueue.h"
#include "core/PluginLoader.h"
#include "interact/AppConfigManager.h"
#include "interact/IpcServer.h"

/// Main application daemon — owns all core components via composition.
/// No singletons: all dependencies are created internally or injected via Configure().

class CollectDaemon
{
public:
	CollectDaemon();
	~CollectDaemon();

	CollectDaemon(const CollectDaemon &) = delete;
	CollectDaemon &operator=(const CollectDaemon &) = delete;

	/// Parse command-line arguments and load configuration.
	int Configure(int argc, char **argv);

	/// Enter main event loop (blocks until SIGINT/SIGTERM).
	int Run();

	/// Signal the loop to stop (called from signal handler).
	void RequestStop();

private:
	/// Load plugins from plugin directory.
	int LoadPlugins();

	/// Set dispatch callbacks on all read plugins.
	void WireDispatchCallbacks();

	/// Internal main loop: read → write → ipc → wait.
	int Loop();

	/// Graceful shutdown sequence.
	void Cleanup();

	// ─── Configuration ─────────────────────────────
	bool m_running;
	std::string m_configPath;
	std::string m_pluginDir;
	std::string m_userConfigPath;
	std::string m_ipcSocketPath;
	CdTime m_defaultInterval;

	// ─── Owned components (composition DI) ─────────
	PluginManager m_pluginManager;
	PluginLoader m_pluginLoader;
	Dispatcher m_dispatcher;
	WriteQueue m_writeQueue;
	AppConfigManager m_appConfig;

	// IpcServer created after Configure (needs AppConfigManager & PluginManager)
	std::unique_ptr<IpcServer> m_ipcServer;
};
