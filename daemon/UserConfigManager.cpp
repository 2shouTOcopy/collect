#include "UserConfigManager.h"
#include <cstring>
#include "ModuleDef.h"

UserConfigManager& UserConfigManager::Instance()
{
    static UserConfigManager instance;
    return instance;
}

UserConfigManager::UserConfigManager()
    : m_configPath(PREFIX "/share/user_config.json")
{
}

int UserConfigManager::loadAndApply(const std::string& configPath)
{
    cJSON* root = nullptr;
    int result = parseConfigFile(configPath, &root);
    
    if (result == 0) {
        applyConfig(root);
        cJSON_Delete(root);
        INFO("用户配置加载成功并已应用");
    }
    
    return result;
}

int UserConfigManager::parseConfigFile(const std::string& configPath, cJSON** rootOut)
{
    // 打开并读取配置文件
    FILE* fp = fopen(configPath.c_str(), "r");
    if (!fp) {
        ERROR("无法打开用户配置文件: %s", configPath.c_str());
        return -1;
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // 读取文件内容
    char* buffer = (char*)malloc(fileSize + 1);
    if (!buffer) {
        fclose(fp);
        ERROR("内存分配失败");
        return -2;
    }
    
    size_t readSize = fread(buffer, 1, fileSize, fp);
    fclose(fp);
    
    if (readSize != (size_t)fileSize) {
        free(buffer);
        ERROR("读取文件失败");
        return -3;
    }
    
    buffer[fileSize] = '\0';
    
    // 解析JSON
    *rootOut = cJSON_Parse(buffer);
    free(buffer);
    
    if (!*rootOut) {
        ERROR("JSON解析失败: %s", cJSON_GetErrorPtr());
        return -4;
    }
    
    return 0;
}

void UserConfigManager::applyConfig(cJSON* root)
{
    // 处理各部分配置
    cJSON* modules = cJSON_GetObjectItem(root, "modules");
    if (modules) {
        applyModuleLogLevel(modules);
    }
    
    cJSON* userLog = cJSON_GetObjectItem(root, "user_log");
    if (userLog) {
        applyUserLogConfig(userLog);
    }
    
    cJSON* outputLog = cJSON_GetObjectItem(root, "output_log");
    if (outputLog) {
        applyOutputLogConfig(outputLog);
    }
    
    cJSON* system = cJSON_GetObjectItem(root, "system");
    if (system) {
        applySystemConfig(system);
    }
}

void UserConfigManager::applyModuleLogLevel(const cJSON* moduleConfig)
{
    const char* modules[] = {"app", "operator", "dsp"};
    
    for (const char* module : modules) {
        cJSON* moduleJson = cJSON_GetObjectItem(moduleConfig, module);
        if (moduleJson) {
            // 设置日志级别
            cJSON* logLevel = cJSON_GetObjectItem(moduleJson, "log_level");
            if (logLevel && cJSON_IsString(logLevel)) {
                int level = getLogLevelFromString(logLevel->valuestring);
                INFO("设置模块 %s 日志级别: %s (%d)", module, logLevel->valuestring, level);
                
                // 这里应用配置到相应模块
                // 示例: ModuleManager::Instance().setLogLevel(module, level);
            }
            
            // 设置FIFO缓存
            cJSON* fifoCache = cJSON_GetObjectItem(moduleJson, "fifo_cache");
            if (fifoCache && cJSON_IsBool(fifoCache)) {
                bool enabled = cJSON_IsTrue(fifoCache);
                INFO("设置模块 %s FIFO缓存: %s", module, enabled ? "启用" : "禁用");
                
                // 这里应用配置到相应模块
                // 示例: ModuleManager::Instance().setFifoCache(module, enabled);
            }
        }
    }
}

void UserConfigManager::applyUserLogConfig(const cJSON* userLogConfig)
{
    bool enabled = getBoolValue(userLogConfig, "enabled", false);
    
    if (enabled) {
        cJSON* format = cJSON_GetObjectItem(userLogConfig, "format");
        const char* formatStr = format && cJSON_IsString(format) ? format->valuestring : "csv";
        
        INFO("用户日志记录: 启用, 格式: %s", formatStr);
        
        // 这里应用用户日志配置
        // 示例: LogManager::Instance().enableUserLog(formatStr);
    } else {
        INFO("用户日志记录: 禁用");
        // 示例: LogManager::Instance().disableUserLog();
    }
}

void UserConfigManager::applyOutputLogConfig(const cJSON* outputLogConfig)
{
    bool enabled = getBoolValue(outputLogConfig, "enabled", false);
    
    if (enabled) {
        cJSON* format = cJSON_GetObjectItem(outputLogConfig, "format");
        const char* formatStr = format && cJSON_IsString(format) ? format->valuestring : "txt";
        
        INFO("输出日志记录: 启用, 格式: %s", formatStr);
        
        // 这里应用输出日志配置
        // 示例: LogManager::Instance().enableOutputLog(formatStr);
    } else {
        INFO("输出日志记录: 禁用");
        // 示例: LogManager::Instance().disableOutputLog();
    }
}

void UserConfigManager::applySystemConfig(const cJSON* systemConfig)
{
    // 日志重定向
    bool logRedirect = getBoolValue(systemConfig, "log_redirect", false);
    INFO("系统日志重定向: %s", logRedirect ? "启用" : "禁用");
    
    // 调试模式
    bool debugMode = getBoolValue(systemConfig, "debug_mode", false);
    INFO("调试模式: %s", debugMode ? "启用" : "禁用");
    
    // 串口控制
    bool serialControl = getBoolValue(systemConfig, "serial_control", true);
    INFO("串口控制: %s", serialControl ? "启用" : "禁用");
    
    // 看门狗
    bool watchdog = getBoolValue(systemConfig, "watchdog", true);
    INFO("看门狗: %s", watchdog ? "启用" : "禁用");
    
    // 这里应用系统配置
    // 示例: SystemManager::Instance().configure(logRedirect, debugMode, serialControl, watchdog);
}

int UserConfigManager::getLogLevelFromString(const char* levelStr)
{
    if (strcmp(levelStr, "DEBUG") == 0) return LOG_DEBUG;
    if (strcmp(levelStr, "INFO") == 0) return LOG_INFO;
    if (strcmp(levelStr, "WARNING") == 0) return LOG_WARNING;
    if (strcmp(levelStr, "ERROR") == 0) return LOG_ERR;
    return LOG_INFO; // 默认级别
}

bool UserConfigManager::getBoolValue(const cJSON* json, const char* key, bool defaultValue)
{
    cJSON* item = cJSON_GetObjectItem(json, key);
    if (item && cJSON_IsBool(item)) {
        return cJSON_IsTrue(item);
    }
    return defaultValue;
}
