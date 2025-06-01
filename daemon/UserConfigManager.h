#pragma once

#include <string>
#include "utils/cJSON.h"

class UserConfigManager {
public:
    static UserConfigManager& Instance();
    
    // 加载并应用用户配置
    int loadAndApply(const std::string& configPath);
    
    // 设置默认配置路径
    void setConfigPath(const std::string& path) { m_configPath = path; }
    
    // 获取当前配置路径
    const std::string& getConfigPath() const { return m_configPath; }

private:
    UserConfigManager();
    ~UserConfigManager() = default;
    
    UserConfigManager(const UserConfigManager&) = delete;
    UserConfigManager& operator=(const UserConfigManager&) = delete;

    // 配置解析与应用方法
    int parseConfigFile(const std::string& configPath, cJSON** rootOut);
    void applyConfig(cJSON* root);
    
    // 各模块配置应用
    void applyModuleLogLevel(const cJSON* moduleConfig);
    void applyUserLogConfig(const cJSON* userLogConfig);
    void applyOutputLogConfig(const cJSON* outputLogConfig);
    void applySystemConfig(const cJSON* systemConfig);
    
    // 辅助函数
    int getLogLevelFromString(const char* levelStr);
    bool getBoolValue(const cJSON* json, const char* key, bool defaultValue);
    
    // 成员变量
    std::string m_configPath;
};