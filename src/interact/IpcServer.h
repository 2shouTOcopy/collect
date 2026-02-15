#pragma once

#include <string>
#include <vector>
#include <functional>

class AppConfigManager;
class PluginManager;

/// UNIX Domain Socket IPC server — processes JSON commands from the App process.
/// Non-blocking: Poll() is called once per main loop iteration.
///
/// Supported commands:
///   {"cmd": "get_config"}                                  → returns full config
///   {"cmd": "set_log_level", "module": "app", "level": "DEBUG"} → modify log level
///   {"cmd": "set_debug_mode", "enabled": true}             → toggle debug mode
///   {"cmd": "reload_config"}                               → re-read user_config.json
///   {"cmd": "get_status"}                                  → returns plugin status

class IpcServer
{
public:
	IpcServer(const std::string &socketPath,
	          AppConfigManager &configMgr);
	~IpcServer();

	IpcServer(const IpcServer &) = delete;
	IpcServer &operator=(const IpcServer &) = delete;

	/// Set optional reference to PluginManager for get_status command.
	void SetPluginManager(PluginManager *pm) { m_pluginMgr = pm; }

	/// Start listening on the socket. Returns 0 on success.
	int Start();

	/// Non-blocking poll: accept new connections and handle pending commands.
	/// Should be called once per main loop iteration.
	void Poll();

	/// Stop the server, close socket, unlink socket file.
	void Stop();

	/// Whether the server is running.
	bool IsRunning() const { return m_listenFd >= 0; }

private:
	/// Accept a new client connection (non-blocking).
	void AcceptClient();

	/// Read command from a connected client, handle it, send response.
	void HandleClient(int clientFd);

	/// Parse JSON command and generate JSON response.
	std::string DispatchCommand(const std::string &jsonCmd);

	// ─── Individual command handlers ─────────────────────────

	std::string CmdGetConfig();
	std::string CmdSetLogLevel(const std::string &module, const std::string &level);
	std::string CmdSetDebugMode(bool enabled);
	std::string CmdReloadConfig();
	std::string CmdGetStatus();

	/// Build a JSON response string.
	static std::string MakeResponse(int code, const std::string &message,
	                                 const std::string &data = "");

	std::string m_socketPath;
	AppConfigManager &m_configMgr;
	PluginManager *m_pluginMgr;
	int m_listenFd;

	/// Connected client file descriptors (for multi-message sessions).
	std::vector<int> m_clientFds;

	static constexpr int MAX_CLIENTS = 4;
	static constexpr int RECV_BUF_SIZE = 4096;
};
