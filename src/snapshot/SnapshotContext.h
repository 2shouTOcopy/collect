#pragma once

#include <string>

/// Context passed to snapshot plugins for one diagnostic capture.
struct SnapshotContext
{
	std::string reason;
	std::string outputDir;
	std::string snapshotDir;
	std::string appLogDir;
	int targetPid = -1;
};

