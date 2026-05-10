#include <gtest/gtest.h>

#include "config/ConfigManager.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>

namespace
{
std::string WriteConfig(const std::string &content, const std::string &name)
{
	std::string path = "/tmp/collect_config_manager_" +
		std::to_string(getpid()) + "_" + name + ".conf";
	std::ofstream out(path);
	out << content;
	return path;
}
}

TEST(ConfigManagerTest, TracksOnlyExplicitPluginDirAndInterval)
{
	std::string path = WriteConfig(
		"LoadPlugin cpu\n"
		"<Plugin cpu>\n"
		"  ReportByCpu false\n"
		"</Plugin>\n",
		"explicit_flags");

	ConfigManager cfg;
	ASSERT_EQ(cfg.Load(path), 0);

	EXPECT_FALSE(cfg.HasPluginDir());
	EXPECT_FALSE(cfg.HasDefaultInterval());
	EXPECT_EQ(cfg.GetLoadPlugins().size(), 1u);
	EXPECT_EQ(cfg.GetPluginConfig("cpu").size(), 1u);

	std::remove(path.c_str());
}

TEST(ConfigManagerTest, ResetsStateOnReload)
{
	std::string first = WriteConfig(
		"PluginDir \"/tmp/plugins\"\n"
		"Interval 3\n"
		"LoadPlugin cpu\n",
		"first");
	std::string second = WriteConfig("LoadPlugin memory\n", "second");

	ConfigManager cfg;
	ASSERT_EQ(cfg.Load(first), 0);
	EXPECT_TRUE(cfg.HasPluginDir());
	EXPECT_TRUE(cfg.HasDefaultInterval());

	ASSERT_EQ(cfg.Load(second), 0);
	EXPECT_FALSE(cfg.HasPluginDir());
	EXPECT_FALSE(cfg.HasDefaultInterval());
	ASSERT_EQ(cfg.GetLoadPlugins().size(), 1u);
	EXPECT_EQ(cfg.GetLoadPlugins()[0], "memory");

	std::remove(first.c_str());
	std::remove(second.c_str());
}
