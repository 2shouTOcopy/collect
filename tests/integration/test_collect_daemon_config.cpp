#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
std::string RunCommand(const std::string &cmd)
{
	std::array<char, 256> buffer = {};
	std::string output;
	FILE *pipe = popen(cmd.c_str(), "r");
	if (pipe == nullptr)
	{
		return output;
	}

	while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
	{
		output += buffer.data();
	}
	pclose(pipe);
	return output;
}

std::string TempPath(const std::string &name)
{
	std::ostringstream oss;
	oss << "/tmp/collect_test_" << getpid() << "_" << name;
	return oss.str();
}

void EnsureDir(const std::string &path)
{
	mkdir(path.c_str(), 0755);
}
}

TEST(CollectDaemonConfigTest, LoadPluginWhitelistAndPluginConfigAreApplied)
{
	const std::string root = TempPath("daemon_config");
	EnsureDir(root);

	const std::string configPath = root + "/collect.conf";
	const std::string socketPath = root + "/collect.sock";
	const std::string modulesDir = std::string(COLLECT_TEST_BINARY_DIR) + "/bin/modules";
	const std::string collectBin = std::string(COLLECT_TEST_BINARY_DIR) + "/bin/collect";

	std::ofstream cfg(configPath);
	const std::string outputFile = root + "/metrics.json";

	cfg << "PluginDir \"" << modulesDir << "\"\n"
	    << "LoadPlugin json_writer\n"
	    << "<Plugin json_writer>\n"
	    << "  OutputFile \"" << outputFile << "\"\n"
	    << "  StdOut false\n"
	    << "</Plugin>\n";
	cfg.close();

	const std::string cmd = collectBin +
		" -F -c " + configPath +
		" -u " + root + "/missing_user_config.json" +
		" -s " + socketPath +
		" 2>&1";
	std::string output = RunCommand(cmd);

	EXPECT_NE(output.find("Loaded 1/1 plugins"), std::string::npos) << output;
	EXPECT_EQ(output.find("Loaded 10/10 plugins"), std::string::npos) << output;
	EXPECT_NE(output.find("Writing to: " + outputFile), std::string::npos) << output;
}
