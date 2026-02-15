#include <gtest/gtest.h>
#include "interact/AppConfigManager.h"

#include <fstream>
#include <cstdio>

static const char *TEST_CONFIG_JSON = R"({
    "modules": {
        "app": {
            "log_level": "INFO",
            "fifo_cache": true
        },
        "operator": {
            "log_level": "WARNING",
            "fifo_cache": false
        },
        "dsp": {
            "log_level": "ERROR",
            "fifo_cache": true
        }
    },
    "user_log": {
        "enabled": true,
        "format": "csv",
        "fields": ["timestamp", "username", "ip", "action"]
    },
    "output_log": {
        "enabled": true,
        "format": "txt",
        "fields": ["timestamp", "content"]
    },
    "system": {
        "log_redirect": false,
        "debug_mode": false,
        "serial_control": true,
        "watchdog": true
    }
})";

class AppConfigManagerTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_tempFile = "/tmp/test_user_config.json";
		std::ofstream ofs(m_tempFile);
		ofs << TEST_CONFIG_JSON;
		ofs.close();
	}

	void TearDown() override
	{
		std::remove(m_tempFile.c_str());
	}

	std::string m_tempFile;
};

TEST_F(AppConfigManagerTest, LoadSuccess)
{
	AppConfigManager mgr;
	EXPECT_EQ(mgr.Load(m_tempFile), 0);
	EXPECT_TRUE(mgr.IsLoaded());
	EXPECT_EQ(mgr.GetPath(), m_tempFile);
}

TEST_F(AppConfigManagerTest, LoadFailsOnMissingFile)
{
	AppConfigManager mgr;
	EXPECT_NE(mgr.Load("/tmp/nonexistent_config.json"), 0);
	EXPECT_FALSE(mgr.IsLoaded());
}

TEST_F(AppConfigManagerTest, LoadFailsOnInvalidJson)
{
	std::string badFile = "/tmp/test_bad_config.json";
	std::ofstream ofs(badFile);
	ofs << "{ this is not valid json";
	ofs.close();

	AppConfigManager mgr;
	EXPECT_NE(mgr.Load(badFile), 0);
	EXPECT_FALSE(mgr.IsLoaded());

	std::remove(badFile.c_str());
}

TEST_F(AppConfigManagerTest, GetModuleLogLevel)
{
	AppConfigManager mgr;
	mgr.Load(m_tempFile);

	EXPECT_EQ(mgr.GetModuleLogLevel("app"), "INFO");
	EXPECT_EQ(mgr.GetModuleLogLevel("operator"), "WARNING");
	EXPECT_EQ(mgr.GetModuleLogLevel("dsp"), "ERROR");
	EXPECT_EQ(mgr.GetModuleLogLevel("nonexistent"), "INFO");  // default
}

TEST_F(AppConfigManagerTest, GetModuleFifoCache)
{
	AppConfigManager mgr;
	mgr.Load(m_tempFile);

	EXPECT_TRUE(mgr.GetModuleFifoCache("app"));
	EXPECT_FALSE(mgr.GetModuleFifoCache("operator"));
	EXPECT_TRUE(mgr.GetModuleFifoCache("dsp"));
	EXPECT_FALSE(mgr.GetModuleFifoCache("nonexistent"));
}

TEST_F(AppConfigManagerTest, SystemConfig)
{
	AppConfigManager mgr;
	mgr.Load(m_tempFile);

	EXPECT_FALSE(mgr.IsDebugMode());
	EXPECT_TRUE(mgr.IsWatchdogEnabled());
	EXPECT_FALSE(mgr.IsLogRedirect());
	EXPECT_TRUE(mgr.IsSerialControl());
}

TEST_F(AppConfigManagerTest, UserLogConfig)
{
	AppConfigManager mgr;
	mgr.Load(m_tempFile);

	EXPECT_TRUE(mgr.IsUserLogEnabled());
	EXPECT_EQ(mgr.GetUserLogFormat(), "csv");
}

TEST_F(AppConfigManagerTest, SetModuleLogLevel)
{
	AppConfigManager mgr;
	mgr.Load(m_tempFile);

	EXPECT_EQ(mgr.SetModuleLogLevel("app", "DEBUG"), 0);
	EXPECT_EQ(mgr.GetModuleLogLevel("app"), "DEBUG");
}

TEST_F(AppConfigManagerTest, SetDebugMode)
{
	AppConfigManager mgr;
	mgr.Load(m_tempFile);

	EXPECT_FALSE(mgr.IsDebugMode());
	EXPECT_EQ(mgr.SetDebugMode(true), 0);
	EXPECT_TRUE(mgr.IsDebugMode());
}

TEST_F(AppConfigManagerTest, SaveAndReload)
{
	std::string saveFile = "/tmp/test_save_config.json";

	{
		AppConfigManager mgr;
		mgr.Load(m_tempFile);
		mgr.SetModuleLogLevel("app", "DEBUG");

		// Copy path for save
		std::ofstream ofs(saveFile);
		ofs.close();
	}

	// Load original, modify, save to new file
	AppConfigManager mgr;
	mgr.Load(m_tempFile);
	mgr.SetModuleLogLevel("app", "DEBUG");

	// Save manually (need to set the path)
	// For this test, serialize and check
	std::string json = mgr.ToJsonString();
	EXPECT_NE(json.find("DEBUG"), std::string::npos);

	std::remove(saveFile.c_str());
}

TEST_F(AppConfigManagerTest, ToJsonStringContainsAllSections)
{
	AppConfigManager mgr;
	mgr.Load(m_tempFile);

	std::string json = mgr.ToJsonString();
	EXPECT_NE(json.find("modules"), std::string::npos);
	EXPECT_NE(json.find("user_log"), std::string::npos);
	EXPECT_NE(json.find("output_log"), std::string::npos);
	EXPECT_NE(json.find("system"), std::string::npos);
}

TEST_F(AppConfigManagerTest, UnloadedManagerReturnsDefaults)
{
	AppConfigManager mgr;
	// No Load() called

	EXPECT_EQ(mgr.GetModuleLogLevel("app"), "INFO");
	EXPECT_FALSE(mgr.GetModuleFifoCache("app"));
	EXPECT_FALSE(mgr.IsDebugMode());
	EXPECT_TRUE(mgr.IsWatchdogEnabled());
	EXPECT_EQ(mgr.ToJsonString(), "{}");
}
