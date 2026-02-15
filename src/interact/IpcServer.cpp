#include "IpcServer.h"
#include "AppConfigManager.h"
#include "core/PluginManager.h"
#include "utils/cJSON.h"
#include "utils/Logger.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <algorithm>

static const char *TAG = "IpcServer";

// ─── Construction / Destruction ──────────────────────────────

IpcServer::IpcServer(const std::string &socketPath,
                     AppConfigManager &configMgr)
	: m_socketPath(socketPath)
	, m_configMgr(configMgr)
	, m_pluginMgr(nullptr)
	, m_listenFd(-1)
{
}

IpcServer::~IpcServer()
{
	Stop();
}

// ─── Start / Stop ────────────────────────────────────────────

int IpcServer::Start()
{
	// Remove stale socket file
	unlink(m_socketPath.c_str());

	// Create UNIX domain socket
	m_listenFd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (m_listenFd < 0)
	{
		Logger::Error(TAG, "socket() failed: " + std::string(strerror(errno)));
		return -1;
	}

	// Bind
	struct sockaddr_un addr = {};
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);

	if (bind(m_listenFd, reinterpret_cast<struct sockaddr *>(&addr),
	         sizeof(addr)) < 0)
	{
		Logger::Error(TAG, "bind() failed: " + std::string(strerror(errno)));
		close(m_listenFd);
		m_listenFd = -1;
		return -2;
	}

	// Listen
	if (listen(m_listenFd, MAX_CLIENTS) < 0)
	{
		Logger::Error(TAG, "listen() failed: " + std::string(strerror(errno)));
		close(m_listenFd);
		m_listenFd = -1;
		return -3;
	}

	// Set non-blocking
	int flags = fcntl(m_listenFd, F_GETFL, 0);
	if (flags >= 0)
	{
		fcntl(m_listenFd, F_SETFL, flags | O_NONBLOCK);
	}

	Logger::Info(TAG, "Listening on " + m_socketPath);
	return 0;
}

void IpcServer::Stop()
{
	// Close all client connections
	for (int fd : m_clientFds)
	{
		close(fd);
	}
	m_clientFds.clear();

	// Close listening socket
	if (m_listenFd >= 0)
	{
		close(m_listenFd);
		m_listenFd = -1;
		unlink(m_socketPath.c_str());
		Logger::Info(TAG, "Stopped");
	}
}

// ─── Poll ────────────────────────────────────────────────────

void IpcServer::Poll()
{
	if (m_listenFd < 0)
	{
		return;
	}

	// Accept new connections (non-blocking)
	AcceptClient();

	// Process existing clients
	std::vector<int> closedFds;

	for (int fd : m_clientFds)
	{
		char buf[RECV_BUF_SIZE] = {};
		ssize_t n = recv(fd, buf, sizeof(buf) - 1, MSG_DONTWAIT);

		if (n > 0)
		{
			buf[n] = '\0';
			std::string cmd(buf, static_cast<size_t>(n));
			std::string response = DispatchCommand(cmd);

			// Send response
			ssize_t sent = send(fd, response.c_str(), response.size(), MSG_NOSIGNAL);
			if (sent < 0)
			{
				Logger::Warn(TAG, "send() failed for client fd=" + std::to_string(fd));
			}

			// Close after response (request-response model)
			close(fd);
			closedFds.push_back(fd);
		}
		else if (n == 0)
		{
			// Client disconnected
			close(fd);
			closedFds.push_back(fd);
		}
		// n < 0 && errno == EAGAIN/EWOULDBLOCK → no data yet, skip
		else if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			Logger::Warn(TAG, "recv() error on fd=" + std::to_string(fd)
			             + ": " + strerror(errno));
			close(fd);
			closedFds.push_back(fd);
		}
	}

	// Remove closed fds
	for (int fd : closedFds)
	{
		m_clientFds.erase(
			std::remove(m_clientFds.begin(), m_clientFds.end(), fd),
			m_clientFds.end());
	}
}

void IpcServer::AcceptClient()
{
	struct sockaddr_un clientAddr = {};
	socklen_t addrLen = sizeof(clientAddr);

	int clientFd = accept(m_listenFd,
	                      reinterpret_cast<struct sockaddr *>(&clientAddr),
	                      &addrLen);
	if (clientFd < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			Logger::Warn(TAG, "accept() failed: " + std::string(strerror(errno)));
		}
		return;
	}

	if (static_cast<int>(m_clientFds.size()) >= MAX_CLIENTS)
	{
		Logger::Warn(TAG, "Max clients reached, rejecting connection");
		std::string err = MakeResponse(-1, "Too many connections");
		send(clientFd, err.c_str(), err.size(), MSG_NOSIGNAL);
		close(clientFd);
		return;
	}

	// Set non-blocking
	int flags = fcntl(clientFd, F_GETFL, 0);
	if (flags >= 0)
	{
		fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);
	}

	m_clientFds.push_back(clientFd);
}

// ─── Command dispatch ────────────────────────────────────────

std::string IpcServer::DispatchCommand(const std::string &jsonCmd)
{
	cJSON *root = cJSON_Parse(jsonCmd.c_str());
	if (root == nullptr)
	{
		return MakeResponse(-1, "Invalid JSON");
	}

	cJSON *cmdItem = cJSON_GetObjectItem(root, "cmd");
	if (cmdItem == nullptr || !cJSON_IsString(cmdItem))
	{
		cJSON_Delete(root);
		return MakeResponse(-2, "Missing 'cmd' field");
	}

	std::string cmd(cmdItem->valuestring);
	std::string response;

	if (cmd == "get_config")
	{
		response = CmdGetConfig();
	}
	else if (cmd == "set_log_level")
	{
		cJSON *modItem = cJSON_GetObjectItem(root, "module");
		cJSON *lvlItem = cJSON_GetObjectItem(root, "level");

		if (modItem == nullptr || !cJSON_IsString(modItem) ||
		    lvlItem == nullptr || !cJSON_IsString(lvlItem))
		{
			response = MakeResponse(-3, "Missing 'module' or 'level' field");
		}
		else
		{
			response = CmdSetLogLevel(modItem->valuestring, lvlItem->valuestring);
		}
	}
	else if (cmd == "set_debug_mode")
	{
		cJSON *enabledItem = cJSON_GetObjectItem(root, "enabled");
		if (enabledItem == nullptr || !cJSON_IsBool(enabledItem))
		{
			response = MakeResponse(-3, "Missing 'enabled' field");
		}
		else
		{
			response = CmdSetDebugMode(cJSON_IsTrue(enabledItem) != 0);
		}
	}
	else if (cmd == "reload_config")
	{
		response = CmdReloadConfig();
	}
	else if (cmd == "get_status")
	{
		response = CmdGetStatus();
	}
	else
	{
		response = MakeResponse(-4, "Unknown command: " + cmd);
	}

	cJSON_Delete(root);
	return response;
}

// ─── Command handlers ────────────────────────────────────────

std::string IpcServer::CmdGetConfig()
{
	return MakeResponse(0, "ok", m_configMgr.ToJsonString());
}

std::string IpcServer::CmdSetLogLevel(const std::string &module,
                                       const std::string &level)
{
	int ret = m_configMgr.SetModuleLogLevel(module, level);
	if (ret != 0)
	{
		return MakeResponse(ret, "Failed to set log level");
	}

	// Auto-save after modification
	m_configMgr.Save();
	return MakeResponse(0, "Log level updated: " + module + " = " + level);
}

std::string IpcServer::CmdSetDebugMode(bool enabled)
{
	int ret = m_configMgr.SetDebugMode(enabled);
	if (ret != 0)
	{
		return MakeResponse(ret, "Failed to set debug mode");
	}

	m_configMgr.Save();
	return MakeResponse(0, std::string("Debug mode ") + (enabled ? "enabled" : "disabled"));
}

std::string IpcServer::CmdReloadConfig()
{
	const std::string &path = m_configMgr.GetPath();
	if (path.empty())
	{
		return MakeResponse(-1, "No config path set");
	}

	int ret = m_configMgr.Load(path);
	if (ret != 0)
	{
		return MakeResponse(ret, "Failed to reload config");
	}

	return MakeResponse(0, "Config reloaded from: " + path);
}

std::string IpcServer::CmdGetStatus()
{
	cJSON *status = cJSON_CreateObject();
	cJSON_AddStringToObject(status, "state", "running");

	if (m_pluginMgr != nullptr)
	{
		cJSON_AddNumberToObject(status, "read_plugins",
		                        static_cast<double>(m_pluginMgr->GetReadPlugins().size()));
		cJSON_AddNumberToObject(status, "write_plugins",
		                        static_cast<double>(m_pluginMgr->GetWritePlugins().size()));
		cJSON_AddNumberToObject(status, "total_plugins",
		                        static_cast<double>(m_pluginMgr->PluginCount()));
	}

	char *str = cJSON_PrintUnformatted(status);
	std::string data(str != nullptr ? str : "{}");
	if (str != nullptr)
	{
		cJSON_free(str);
	}
	cJSON_Delete(status);

	return MakeResponse(0, "ok", data);
}

// ─── Response builder ────────────────────────────────────────

std::string IpcServer::MakeResponse(int code, const std::string &message,
                                     const std::string &data)
{
	cJSON *resp = cJSON_CreateObject();
	cJSON_AddNumberToObject(resp, "code", code);
	cJSON_AddStringToObject(resp, "message", message.c_str());

	if (!data.empty())
	{
		// Try to embed data as JSON object; if parse fails, embed as string
		cJSON *dataJson = cJSON_Parse(data.c_str());
		if (dataJson != nullptr)
		{
			cJSON_AddItemToObject(resp, "data", dataJson);
		}
		else
		{
			cJSON_AddStringToObject(resp, "data", data.c_str());
		}
	}

	char *str = cJSON_Print(resp);
	std::string result(str != nullptr ? str : "{}");
	if (str != nullptr)
	{
		cJSON_free(str);
	}
	cJSON_Delete(resp);

	return result;
}
