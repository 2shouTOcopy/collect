#pragma once

#include <string>
#include <memory>
#include <csignal>

#include "core/PluginManager.h"
#include "core/Dispatcher.h"
#include "core/WriteQueue.h"
#include "core/PluginLoader.h"
#include "config/ConfigManager.h"
#include "config/TypesDb.h"
#include "interact/AppConfigManager.h"
#include "interact/IpcServer.h"
#include "snapshot/SnapshotManager.h"

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

	/// Signal the loop to create a snapshot (called from SIGUSR1 handler).
	void RequestSnapshot();

	/// Run one snapshot from the command line and exit.
	int RunSnapshotCommand(int argc, char **argv);

private:
	/// Load collect.conf, types.db, and user_config.json after path parsing.
	void LoadRuntimeConfig();

	/// Load plugins from plugin directory.
	int LoadPlugins();

	/// Apply <Plugin name> config blocks to a loaded plugin.
	int ConfigurePlugin(const std::string &pluginName);

	/// Set dispatch callbacks on all read plugins.
	void WireDispatchCallbacks();

	/// Internal main loop: read → write → ipc → wait.
	int Loop();

	/// Graceful shutdown sequence.
	void Cleanup();

	/// Create a snapshot using current configuration.
	int CreateSnapshot(const std::string &reason,
	                   const std::string &appLogDir,
	                   const std::string &outputDir,
	                   int targetPid,
	                   bool packArchive);

	// ─── Configuration ─────────────────────────────
	bool m_running;
	bool m_oneShot;
	bool m_cleaned;
	volatile std::sig_atomic_t m_snapshotRequested;
	std::string m_configPath;
	std::string m_pluginDir;
	std::string m_userConfigPath;
	std::string m_ipcSocketPath;
	std::string m_snapshotDir;
	std::string m_appLogDir;
	bool m_snapshotPack;
	CdTime m_defaultInterval;

	// ─── Owned components (composition DI) ─────────
	PluginManager m_pluginManager;
	PluginLoader m_pluginLoader;
	Dispatcher m_dispatcher;
	WriteQueue m_writeQueue;
	AppConfigManager m_appConfig;
	ConfigManager m_configManager;
	TypesDb m_typesDb;

	// IpcServer created after Configure (needs AppConfigManager & PluginManager)
	std::unique_ptr<IpcServer> m_ipcServer;
};
