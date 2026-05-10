#include "CollectDaemon.h"
#include "SignalHandler.h"
#include "utils/Logger.h"

#include <iostream>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <cstdlib>

static const char *TAG = "CollectDaemon";

CollectDaemon::CollectDaemon()
	: m_running(false)
	, m_oneShot(false)
	, m_cleaned(false)
	, m_snapshotRequested(0)
	, m_configPath("/etc/collect/collect.conf")
	, m_pluginDir("/usr/lib/collect/modules")
	, m_userConfigPath("/etc/collect/user_config.json")
	, m_ipcSocketPath("/tmp/collect.sock")
	, m_snapshotDir("/tmp/collect_snapshots")
	, m_appLogDir("")
	, m_snapshotPack(true)
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

void CollectDaemon::RequestSnapshot()
{
	m_snapshotRequested = 1;
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

	LoadRuntimeConfig();
	return 0;
}

void CollectDaemon::LoadRuntimeConfig()
{
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

		std::string cfgSnapshotDir =
			m_configManager.GetGlobal("SnapshotDir", m_snapshotDir);
		if (!cfgSnapshotDir.empty())
		{
			m_snapshotDir = cfgSnapshotDir;
		}

		std::string cfgAppLogDir =
			m_configManager.GetGlobal("AppLogDir", m_appLogDir);
		if (!cfgAppLogDir.empty())
		{
			m_appLogDir = cfgAppLogDir;
		}

		std::string pack = m_configManager.GetGlobal("SnapshotPack",
		                                             m_snapshotPack ? "true" : "false");
		m_snapshotPack = (pack == "true" || pack == "1" || pack == "yes");

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
}

int CollectDaemon::RunSnapshotCommand(int argc, char **argv)
{
	std::string reason = "manual";
	std::string appLogDirOverride;
	std::string outputDirOverride;
	int targetPid = -1;
	bool packArchive = true;

	for (int i = 2; i < argc; ++i)
	{
		std::string arg(argv[i]);
		auto requireValue = [&](const std::string &name) -> const char *
		{
			if (i + 1 >= argc)
			{
				Logger::Error(TAG, "Missing value for " + name);
				return nullptr;
			}
			++i;
			return argv[i];
		};

		if (arg == "--reason")
		{
			const char *value = requireValue(arg);
			if (value == nullptr) return -1;
			reason = value;
		}
		else if (arg == "--app-log-dir")
		{
			const char *value = requireValue(arg);
			if (value == nullptr) return -1;
			appLogDirOverride = value;
		}
		else if (arg == "--out")
		{
			const char *value = requireValue(arg);
			if (value == nullptr) return -1;
			outputDirOverride = value;
		}
		else if (arg == "--pid")
		{
			const char *value = requireValue(arg);
			if (value == nullptr) return -1;
			targetPid = std::atoi(value);
		}
		else if (arg == "--no-pack")
		{
			packArchive = false;
		}
		else if (arg == "-c" || arg == "--config")
		{
			const char *value = requireValue(arg);
			if (value == nullptr) return -1;
			m_configPath = value;
		}
		else if (arg == "-p" || arg == "--plugin-dir")
		{
			const char *value = requireValue(arg);
			if (value == nullptr) return -1;
			m_pluginDir = value;
		}
		else if (arg == "-u" || arg == "--user-config")
		{
			const char *value = requireValue(arg);
			if (value == nullptr) return -1;
			m_userConfigPath = value;
		}
		else
		{
			Logger::Error(TAG, "Unknown snapshot option: " + arg);
			return -1;
		}
	}

	LoadRuntimeConfig();
	LoadPlugins();
	m_pluginManager.InitAll();

	int ret = CreateSnapshot(
		reason,
		appLogDirOverride.empty() ? m_appLogDir : appLogDirOverride,
		outputDirOverride.empty() ? m_snapshotDir : outputDirOverride,
		targetPid,
		packArchive);

	Cleanup();
	return ret;
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

		if (m_snapshotRequested != 0)
		{
			m_snapshotRequested = 0;
			CreateSnapshot("sigusr1", m_appLogDir, m_snapshotDir, -1, m_snapshotPack);
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

int CollectDaemon::CreateSnapshot(const std::string &reason,
                                  const std::string &appLogDir,
                                  const std::string &outputDir,
                                  int targetPid,
                                  bool packArchive)
{
	SnapshotManager snapshotManager(m_pluginManager);

	SnapshotRequest request;
	request.reason = reason;
	request.outputDir = outputDir;
	request.appLogDir = appLogDir;
	request.targetPid = targetPid;
	request.packArchive = packArchive;

	SnapshotResult result = snapshotManager.CreateSnapshot(request);
	if (result.code == 0)
	{
		Logger::Info(TAG, "Snapshot created: " + result.snapshotDir);
		if (!result.archivePath.empty())
		{
			Logger::Info(TAG, "Snapshot archive: " + result.archivePath);
		}
	}
	else
	{
		Logger::Error(TAG, "Snapshot failed, code=" + std::to_string(result.code));
	}
	return result.code;
}

void CollectDaemon::Cleanup()
{
	if (m_cleaned)
	{
		return;
	}
	m_cleaned = true;

	SignalHandler::Remove();

	// Shutdown all plugins
	m_pluginManager.ShutdownAll();

	// Close IPC server
	m_ipcServer.reset();

	Logger::Info(TAG, "Cleanup complete");
}
