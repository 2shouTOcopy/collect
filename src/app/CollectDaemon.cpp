#include "CollectDaemon.h"
#include "SignalHandler.h"
#include "utils/Logger.h"

#include <iostream>
#include <unistd.h>
#include <thread>
#include <chrono>

static const char *TAG = "CollectDaemon";

CollectDaemon::CollectDaemon()
	: m_running(false)
	, m_oneShot(false)
	, m_configPath("/etc/collect/collect.conf")
	, m_pluginDir("/usr/lib/collect/modules")
	, m_userConfigPath("/etc/collect/user_config.json")
	, m_ipcSocketPath("/tmp/collect.sock")
	, m_defaultInterval(CdTime::FromDouble(10.0))
	, m_writeQueue(WriteQueue::DEFAULT_CAPACITY)
{
}

CollectDaemon::~CollectDaemon()
{
	Cleanup();
}

void CollectDaemon::RequestStop()
{
	m_running = false;
}

int CollectDaemon::Configure(int argc, char **argv)
{
	// Parse command-line: -c config, -p plugin_dir, -u user_config, -s socket, -i interval
	int opt;
	while ((opt = getopt(argc, argv, "c:p:u:s:i:F")) != -1)
	{
		switch (opt)
		{
			case 'c':
				m_configPath = optarg;
				break;
			case 'p':
				m_pluginDir = optarg;
				break;
			case 'u':
				m_userConfigPath = optarg;
				break;
			case 's':
				m_ipcSocketPath = optarg;
				break;
			case 'i':
			{
				double interval = std::stod(optarg);
				if (interval > 0)
				{
					m_defaultInterval = CdTime::FromDouble(interval);
				}
				break;
			}
			case 'F':
				m_oneShot = true;
				break;
			default:
				Logger::Error(TAG, "Usage: collect [-c config] [-p plugin_dir] "
				              "[-u user_config] [-s socket] [-i interval_sec] [-F]");
				return -1;
		}
	}

	Logger::Info(TAG, "Config: " + m_configPath);
	Logger::Info(TAG, "Plugins: " + m_pluginDir);
	Logger::Info(TAG, "UserConfig: " + m_userConfigPath);
	Logger::Info(TAG, "IPC Socket: " + m_ipcSocketPath);
	Logger::Info(TAG, "Interval: " +
	             std::to_string(m_defaultInterval.ToDouble()) + "s");

	// Load main configuration (collect.conf)
	if (m_configManager.Load(m_configPath) == 0)
	{
		// Config file values override CLI defaults
		const std::string &cfgPluginDir = m_configManager.GetPluginDir();
		if (!cfgPluginDir.empty())
		{
			m_pluginDir = cfgPluginDir;
		}

		double cfgInterval = m_configManager.GetDefaultInterval();
		if (cfgInterval > 0.0)
		{
			m_defaultInterval = CdTime::FromDouble(cfgInterval);
		}

		Logger::Info(TAG, "Config loaded: " +
		             std::to_string(m_configManager.GetLoadPlugins().size()) +
		             " plugins configured");
	}
	else
	{
		Logger::Warn(TAG, "Config load failed, using defaults");
	}

	// Load types.db
	const std::string &typesDbPath = m_configManager.GetTypesDbPath();
	if (m_typesDb.Load(typesDbPath) != 0)
	{
		Logger::Warn(TAG, "TypesDB load failed: " + typesDbPath);
	}
	else
	{
		Logger::Info(TAG, "TypesDB loaded: " +
		             std::to_string(m_typesDb.Size()) + " types");
	}

	// Load user configuration
	int cfgRet = m_appConfig.Load(m_userConfigPath);
	if (cfgRet != 0)
	{
		Logger::Warn(TAG, "User config load failed (code=" +
		             std::to_string(cfgRet) + "), using defaults");
	}

	return 0;
}

int CollectDaemon::LoadPlugins()
{
	m_pluginLoader.SetDir(m_pluginDir);

	// Load all plugins found in the directory
	auto pluginNames = m_pluginLoader.ListPlugins();
	if (pluginNames.empty())
	{
		Logger::Warn(TAG, "No plugins found in: " + m_pluginDir);
		return 0;
	}

	int loaded = 0;
	for (const auto &name : pluginNames)
	{
		IPlugin *plugin = m_pluginLoader.Load(name);
		if (plugin != nullptr)
		{
			m_pluginManager.Register(plugin);
			loaded++;
			Logger::Info(TAG, "Loaded plugin: " + name);
		}
		else
		{
			Logger::Error(TAG, "Failed to load plugin: " + name);
		}
	}

	Logger::Info(TAG, "Loaded " + std::to_string(loaded) + "/" +
	             std::to_string(pluginNames.size()) + " plugins");

	return 0;
}

void CollectDaemon::WireDispatchCallbacks()
{
	// Set dispatch callback on all read plugins — routes to WriteQueue
	for (auto *plugin : m_pluginManager.GetReadPlugins())
	{
		plugin->SetDispatchCallback(
			[this](const DataSet &ds, const ValueList &vl)
			{
				m_writeQueue.Enqueue(ds, vl);
			}
		);
	}
}

int CollectDaemon::Run()
{
	// 1. Load & register plugins
	LoadPlugins();

	// 2. Initialize all plugins
	int initRet = m_pluginManager.InitAll();
	if (initRet != 0)
	{
		Logger::Error(TAG, "Plugin initialization failed");
	}

	// 3. Wire dispatch callbacks
	WireDispatchCallbacks();

	// 4. Register read plugins with dispatcher
	for (auto *plugin : m_pluginManager.GetReadPlugins())
	{
		m_dispatcher.RegisterRead(plugin, m_defaultInterval, m_oneShot);
	}

	// 5. Create IPC server
	m_ipcServer = std::make_unique<IpcServer>(
		m_ipcSocketPath, m_appConfig);
	m_ipcServer->SetPluginManager(&m_pluginManager);
	m_ipcServer->Start();

	// 6. Install signal handlers
	m_running = true;
	SignalHandler::Install(*this);

	Logger::Info(TAG, "Starting main loop — " +
	             std::to_string(m_dispatcher.TaskCount()) + " read tasks, " +
	             std::to_string(m_pluginManager.GetWritePlugins().size()) +
	             " writers");

	// 7. Run event loop
	int ret = Loop();

	// 8. Cleanup
	Cleanup();
	return ret;
}

int CollectDaemon::Loop()
{
	const auto &writers = m_pluginManager.GetWritePlugins();
	constexpr size_t DRAIN_BATCH_SIZE = 64;

	while (m_running)
	{
		// Phase 1: Execute due read tasks
		CdTime waitTime = m_dispatcher.RunOnce();

		// Phase 2: Drain write queue
		m_writeQueue.DrainBatch(writers, DRAIN_BATCH_SIZE);

		// Phase 3: Poll IPC
		if (m_ipcServer)
		{
			m_ipcServer->Poll();
		}

		// Phase 4: Sleep until next task (or interrupted by signal)
		double waitSec = waitTime.ToDouble();
		if (waitSec > 0.0)
		{
			// Cap maximum sleep at 1 second for signal responsiveness
			if (waitSec > 1.0)
			{
				waitSec = 1.0;
			}
			std::this_thread::sleep_for(
				std::chrono::microseconds(
					static_cast<long>(waitSec * 1000000)));
		}

		// One-shot mode: exit after first iteration
		if (m_oneShot)
		{
			m_running = false;
		}
	}

	// Drain remaining items on shutdown
	Logger::Info(TAG, "Draining remaining write queue entries...");
	m_writeQueue.DrainAll(writers);

	return 0;
}

void CollectDaemon::Cleanup()
{
	SignalHandler::Remove();

	// Shutdown all plugins
	m_pluginManager.ShutdownAll();

	// Close IPC server
	m_ipcServer.reset();

	Logger::Info(TAG, "Cleanup complete");
}
