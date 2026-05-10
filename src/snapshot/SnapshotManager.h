#pragma once

#include "core/PluginManager.h"
#include "snapshot/SnapshotContext.h"

#include <string>

/// Request options for creating one diagnostic snapshot.
struct SnapshotRequest
{
	std::string reason = "manual";
	std::string outputDir = "/tmp/collect_snapshots";
	std::string appLogDir;
	int targetPid = -1;
	bool packArchive = true;
};

/// Result of one snapshot creation attempt.
struct SnapshotResult
{
	int code = 0;
	std::string snapshotDir;
	std::string archivePath;
	int pluginFailures = 0;
	std::string appLogsStatus;
};

/// Interface around archive creation; tests can inject a fake implementation.
class ISnapshotArchiver
{
public:
	virtual ~ISnapshotArchiver() = default;
	virtual int CreateArchive(const std::string &sourceDir,
	                          const std::string &archivePath) = 0;
};

/// tar.gz archiver backed by the system tar command.
class TarSnapshotArchiver : public ISnapshotArchiver
{
public:
	int CreateArchive(const std::string &sourceDir,
	                  const std::string &archivePath) override;
};

/// Creates snapshot directories, invokes snapshot plugins, copies app logs,
/// writes summary.json, and optionally creates a tar.gz archive.
class SnapshotManager
{
public:
	explicit SnapshotManager(PluginManager &pluginManager,
	                         ISnapshotArchiver *archiver = nullptr);

	SnapshotResult CreateSnapshot(const SnapshotRequest &request);

private:
	std::string MakeSnapshotDir(const std::string &outputDir) const;
	int WriteSummary(const SnapshotRequest &request,
	                 const SnapshotResult &result) const;
	std::string CopyAppLogs(const SnapshotRequest &request,
	                        const std::string &snapshotDir) const;

	PluginManager &m_pluginManager;
	TarSnapshotArchiver m_defaultArchiver;
	ISnapshotArchiver *m_archiver;
};

