#pragma once

#include <string>
#include <vector>

struct cJSON;

/// Manages user_config.json — provides API for IpcServer to query/modify config.
/// Uses cJSON internally. Replaces old UserConfigManager singleton.
/// The cJSON tree is owned by this class and immutable reads return copies.

class AppConfigManager
{
public:
	AppConfigManager();
	~AppConfigManager();

	AppConfigManager(const AppConfigManager &) = delete;
	AppConfigManager &operator=(const AppConfigManager &) = delete;

	/// Load user_config.json from file. Returns 0 on success.
	int Load(const std::string &path);

	/// Save current config back to file. Returns 0 on success.
	int Save() const;

	/// Get the loaded file path.
	const std::string &GetPath() const { return m_path; }

	/// Whether config has been loaded successfully.
	bool IsLoaded() const { return m_root != nullptr; }

	// ─── Module config queries ────────────────────────────────

	/// Get log level for a module (e.g. "app", "operator", "dsp").
	/// Returns "INFO" if not found.
	std::string GetModuleLogLevel(const std::string &moduleName) const;

	/// Get FIFO cache setting for a module. Returns false if not found.
	bool GetModuleFifoCache(const std::string &moduleName) const;

	// ─── System config queries ────────────────────────────────

	bool IsDebugMode() const;
	bool IsWatchdogEnabled() const;
	bool IsLogRedirect() const;
	bool IsSerialControl() const;

	// ─── User/Output log queries ──────────────────────────────

	bool IsUserLogEnabled() const;
	std::string GetUserLogFormat() const;

	bool IsOutputLogEnabled() const;
	std::string GetOutputLogFormat() const;

	// ─── Dynamic modification (triggered by IPC commands) ─────

	/// Set log level for a module. Returns 0 on success.
	int SetModuleLogLevel(const std::string &moduleName, const std::string &level);

	/// Set debug mode. Returns 0 on success.
	int SetDebugMode(bool enabled);

	// ─── Serialization ───────────────────────────────────────

	/// Get full config as JSON string (for get_config IPC command).
	std::string ToJsonString() const;

private:
	/// Read entire file into string.
	static std::string ReadFile(const std::string &path);

	/// Get a nested object: root → section → key.
	cJSON *GetSectionItem(const char *section, const char *key) const;

	/// Get a bool value from a section, with default.
	bool GetBool(const char *section, const char *key, bool defaultValue) const;

	/// Get a string value from a section, with default.
	std::string GetString(const char *section, const char *key,
	                       const std::string &defaultValue) const;

	std::string m_path;
	cJSON *m_root;
};
