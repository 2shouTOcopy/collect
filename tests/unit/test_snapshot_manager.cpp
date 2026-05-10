#include <gtest/gtest.h>

#include "snapshot/SnapshotManager.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

class SnapshotTestPlugin : public IPlugin
{
public:
	std::string Name() const override { return "snapshot_test"; }
	bool HasSnapshot() const override { return true; }

	int Snapshot(const SnapshotContext &ctx) override
	{
		called = true;
		receivedDir = ctx.snapshotDir;

		std::ofstream ofs(ctx.snapshotDir + "/plugin.txt");
		ofs << "plugin snapshot\n";
		return ofs.good() ? 0 : -1;
	}

	bool called = false;
	std::string receivedDir;
};

class FakeArchiver : public ISnapshotArchiver
{
public:
	int CreateArchive(const std::string &sourceDir,
	                  const std::string &archivePath) override
	{
		called = true;
		lastSourceDir = sourceDir;
		lastArchivePath = archivePath;

		std::ofstream ofs(archivePath);
		ofs << "archive placeholder\n";
		return ofs.good() ? 0 : -1;
	}

	bool called = false;
	std::string lastSourceDir;
	std::string lastArchivePath;
};

class SnapshotManagerTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		rootDir = "/tmp/collect_snapshot_manager_test";
		appLogDir = rootDir + "/app_logs_src";
		Cleanup();
		mkdir(rootDir.c_str(), 0755);
		mkdir(appLogDir.c_str(), 0755);

		std::ofstream ofs(appLogDir + "/app.log");
		ofs << "app log\n";
	}

	void TearDown() override
	{
		Cleanup();
	}

	void Cleanup()
	{
		std::string cmd = "rm -rf " + rootDir;
		(void)system(cmd.c_str());
	}

	bool Exists(const std::string &path) const
	{
		struct stat st = {};
		return stat(path.c_str(), &st) == 0;
	}

	std::string rootDir;
	std::string appLogDir;
};

TEST_F(SnapshotManagerTest, CreateSnapshotCopiesAppLogsRunsPluginsAndArchives)
{
	PluginManager pm;
	SnapshotTestPlugin plugin;
	pm.Register(&plugin);

	FakeArchiver archiver;
	SnapshotManager manager(pm, &archiver);

	SnapshotRequest request;
	request.reason = "manual";
	request.outputDir = rootDir;
	request.appLogDir = appLogDir;
	request.packArchive = true;

	SnapshotResult result = manager.CreateSnapshot(request);

	EXPECT_EQ(result.code, 0);
	EXPECT_TRUE(Exists(result.snapshotDir));
	EXPECT_TRUE(Exists(result.snapshotDir + "/summary.json"));
	EXPECT_TRUE(Exists(result.snapshotDir + "/plugin.txt"));
	EXPECT_TRUE(Exists(result.snapshotDir + "/app_logs/app.log"));
	EXPECT_TRUE(plugin.called);
	EXPECT_EQ(plugin.receivedDir, result.snapshotDir);
	EXPECT_TRUE(archiver.called);
	EXPECT_EQ(archiver.lastSourceDir, result.snapshotDir);
	EXPECT_EQ(archiver.lastArchivePath, result.archivePath);
	EXPECT_TRUE(Exists(result.archivePath));
}

TEST_F(SnapshotManagerTest, CreateSnapshotRecordsMissingAppLogsAsSuccess)
{
	PluginManager pm;
	FakeArchiver archiver;
	SnapshotManager manager(pm, &archiver);

	SnapshotRequest request;
	request.reason = "manual";
	request.outputDir = rootDir;
	request.appLogDir = rootDir + "/missing_logs";
	request.packArchive = false;

	SnapshotResult result = manager.CreateSnapshot(request);

	EXPECT_EQ(result.code, 0);
	EXPECT_TRUE(Exists(result.snapshotDir + "/summary.json"));
	EXPECT_FALSE(archiver.called);

	std::ifstream ifs(result.snapshotDir + "/summary.json");
	std::string summary((std::istreambuf_iterator<char>(ifs)),
	                    std::istreambuf_iterator<char>());
	EXPECT_NE(summary.find("\"app_logs\":\"missing\""), std::string::npos);
}
