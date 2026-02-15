#include "AppConfigManager.h"
#include "utils/cJSON.h"
#include "utils/Logger.h"

#include <fstream>
#include <sstream>

static const char *TAG = "AppConfigManager";

// ─── Construction / Destruction ──────────────────────────────

AppConfigManager::AppConfigManager()
	: m_root(nullptr)
{
}

AppConfigManager::~AppConfigManager()
{
	if (m_root != nullptr)
	{
		cJSON_Delete(m_root);
		m_root = nullptr;
	}
}

// ─── Load / Save ─────────────────────────────────────────────

int AppConfigManager::Load(const std::string &path)
{
	std::string content = ReadFile(path);
	if (content.empty())
	{
		Logger::Error(TAG, "Failed to read config file: " + path);
		return -1;
	}

	cJSON *parsed = cJSON_Parse(content.c_str());
	if (parsed == nullptr)
	{
		const char *errPtr = cJSON_GetErrorPtr();
		std::string errMsg = "JSON parse error";
		if (errPtr != nullptr)
		{
			errMsg += " near: ";
			errMsg += errPtr;
		}
		Logger::Error(TAG, errMsg);
		return -2;
	}

	// Replace old tree (immutable swap)
	if (m_root != nullptr)
	{
		cJSON_Delete(m_root);
	}
	m_root = parsed;
	m_path = path;

	Logger::Info(TAG, "Config loaded: " + path);
	return 0;
}

int AppConfigManager::Save() const
{
	if (m_root == nullptr)
	{
		Logger::Error(TAG, "No config loaded, cannot save");
		return -1;
	}
	if (m_path.empty())
	{
		Logger::Error(TAG, "No file path set, cannot save");
		return -2;
	}

	char *jsonStr = cJSON_Print(m_root);
	if (jsonStr == nullptr)
	{
		Logger::Error(TAG, "Failed to serialize JSON");
		return -3;
	}

	std::ofstream ofs(m_path);
	if (!ofs.is_open())
	{
		Logger::Error(TAG, "Failed to open file for writing: " + m_path);
		cJSON_free(jsonStr);
		return -4;
	}

	ofs << jsonStr;
	ofs.close();
	cJSON_free(jsonStr);

	Logger::Info(TAG, "Config saved: " + m_path);
	return 0;
}

// ─── Module config queries ───────────────────────────────────

std::string AppConfigManager::GetModuleLogLevel(const std::string &moduleName) const
{
	if (m_root == nullptr)
	{
		return "INFO";
	}

	cJSON *modules = cJSON_GetObjectItem(m_root, "modules");
	if (modules == nullptr)
	{
		return "INFO";
	}

	cJSON *mod = cJSON_GetObjectItem(modules, moduleName.c_str());
	if (mod == nullptr)
	{
		return "INFO";
	}

	cJSON *level = cJSON_GetObjectItem(mod, "log_level");
	if (level != nullptr && cJSON_IsString(level) && level->valuestring != nullptr)
	{
		return std::string(level->valuestring);
	}

	return "INFO";
}

bool AppConfigManager::GetModuleFifoCache(const std::string &moduleName) const
{
	if (m_root == nullptr)
	{
		return false;
	}

	cJSON *modules = cJSON_GetObjectItem(m_root, "modules");
	if (modules == nullptr)
	{
		return false;
	}

	cJSON *mod = cJSON_GetObjectItem(modules, moduleName.c_str());
	if (mod == nullptr)
	{
		return false;
	}

	cJSON *fifo = cJSON_GetObjectItem(mod, "fifo_cache");
	if (fifo != nullptr && cJSON_IsBool(fifo))
	{
		return cJSON_IsTrue(fifo) != 0;
	}

	return false;
}

// ─── System config queries ───────────────────────────────────

bool AppConfigManager::IsDebugMode() const
{
	return GetBool("system", "debug_mode", false);
}

bool AppConfigManager::IsWatchdogEnabled() const
{
	return GetBool("system", "watchdog", true);
}

bool AppConfigManager::IsLogRedirect() const
{
	return GetBool("system", "log_redirect", false);
}

bool AppConfigManager::IsSerialControl() const
{
	return GetBool("system", "serial_control", true);
}

// ─── User/Output log queries ─────────────────────────────────

bool AppConfigManager::IsUserLogEnabled() const
{
	return GetBool("user_log", "enabled", false);
}

std::string AppConfigManager::GetUserLogFormat() const
{
	return GetString("user_log", "format", "csv");
}

bool AppConfigManager::IsOutputLogEnabled() const
{
	return GetBool("output_log", "enabled", false);
}

std::string AppConfigManager::GetOutputLogFormat() const
{
	return GetString("output_log", "format", "txt");
}

// ─── Dynamic modification ────────────────────────────────────

int AppConfigManager::SetModuleLogLevel(const std::string &moduleName,
                                         const std::string &level)
{
	if (m_root == nullptr)
	{
		Logger::Error(TAG, "No config loaded");
		return -1;
	}

	// Navigate to modules → moduleName
	cJSON *modules = cJSON_GetObjectItem(m_root, "modules");
	if (modules == nullptr)
	{
		// Create "modules" section
		modules = cJSON_AddObjectToObject(m_root, "modules");
	}

	cJSON *mod = cJSON_GetObjectItem(modules, moduleName.c_str());
	if (mod == nullptr)
	{
		// Create module section
		mod = cJSON_AddObjectToObject(modules, moduleName.c_str());
	}

	// Replace or add "log_level"
	cJSON *existing = cJSON_GetObjectItem(mod, "log_level");
	if (existing != nullptr)
	{
		cJSON_ReplaceItemInObject(mod, "log_level",
		                          cJSON_CreateString(level.c_str()));
	}
	else
	{
		cJSON_AddStringToObject(mod, "log_level", level.c_str());
	}

	Logger::Info(TAG, "Set " + moduleName + " log_level = " + level);
	return 0;
}

int AppConfigManager::SetDebugMode(bool enabled)
{
	if (m_root == nullptr)
	{
		Logger::Error(TAG, "No config loaded");
		return -1;
	}

	cJSON *system = cJSON_GetObjectItem(m_root, "system");
	if (system == nullptr)
	{
		system = cJSON_AddObjectToObject(m_root, "system");
	}

	cJSON *existing = cJSON_GetObjectItem(system, "debug_mode");
	if (existing != nullptr)
	{
		cJSON_ReplaceItemInObject(system, "debug_mode",
		                          cJSON_CreateBool(enabled ? 1 : 0));
	}
	else
	{
		cJSON_AddBoolToObject(system, "debug_mode", enabled ? 1 : 0);
	}

	Logger::Info(TAG, std::string("Set debug_mode = ") + (enabled ? "true" : "false"));
	return 0;
}

// ─── Serialization ───────────────────────────────────────────

std::string AppConfigManager::ToJsonString() const
{
	if (m_root == nullptr)
	{
		return "{}";
	}

	char *str = cJSON_Print(m_root);
	if (str == nullptr)
	{
		return "{}";
	}

	std::string result(str);
	cJSON_free(str);
	return result;
}

// ─── Private helpers ─────────────────────────────────────────

std::string AppConfigManager::ReadFile(const std::string &path)
{
	std::ifstream ifs(path);
	if (!ifs.is_open())
	{
		return "";
	}

	std::ostringstream oss;
	oss << ifs.rdbuf();
	return oss.str();
}

cJSON *AppConfigManager::GetSectionItem(const char *section, const char *key) const
{
	if (m_root == nullptr)
	{
		return nullptr;
	}

	cJSON *sec = cJSON_GetObjectItem(m_root, section);
	if (sec == nullptr)
	{
		return nullptr;
	}

	return cJSON_GetObjectItem(sec, key);
}

bool AppConfigManager::GetBool(const char *section, const char *key,
                                bool defaultValue) const
{
	cJSON *item = GetSectionItem(section, key);
	if (item != nullptr && cJSON_IsBool(item))
	{
		return cJSON_IsTrue(item) != 0;
	}
	return defaultValue;
}

std::string AppConfigManager::GetString(const char *section, const char *key,
                                         const std::string &defaultValue) const
{
	cJSON *item = GetSectionItem(section, key);
	if (item != nullptr && cJSON_IsString(item) && item->valuestring != nullptr)
	{
		return std::string(item->valuestring);
	}
	return defaultValue;
}
