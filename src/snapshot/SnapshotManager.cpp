#include "SnapshotManager.h"

#include "utils/Logger.h"
#include "utils/cJSON.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

static const char *TAG = "SnapshotManager";

namespace
{
	bool Exists(const std::string &path)
	{
		struct stat st = {};
		return stat(path.c_str(), &st) == 0;
	}

	bool IsDir(const std::string &path)
	{
		struct stat st = {};
		return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
	}

	int EnsureDir(const std::string &path)
	{
		if (path.empty())
		{
			return -1;
		}

		std::string current;
		for (size_t i = 0; i < path.size(); ++i)
		{
			current.push_back(path[i]);
			if (path[i] != '/' && i + 1 != path.size())
			{
				continue;
			}

			if (current == "/")
			{
				continue;
			}

			if (mkdir(current.c_str(), 0755) != 0 && errno != EEXIST)
			{
				Logger::Error(TAG, "mkdir failed: " + current + ": " +
				              std::string(strerror(errno)));
				return -1;
			}
		}
		return 0;
	}

	std::string JoinPath(const std::string &left, const std::string &right)
	{
		if (left.empty())
		{
			return right;
		}
		if (left.back() == '/')
		{
			return left + right;
		}
		return left + "/" + right;
	}

	std::string BaseName(const std::string &path)
	{
		size_t pos = path.find_last_of('/');
		if (pos == std::string::npos)
		{
			return path;
		}
		return path.substr(pos + 1);
	}

	std::string ShellQuote(const std::string &value)
	{
		std::string quoted = "'";
		for (char c : value)
		{
			if (c == '\'')
			{
				quoted += "'\\''";
			}
			else
			{
				quoted.push_back(c);
			}
		}
		quoted.push_back('\'');
		return quoted;
	}

	std::string TimestampForPath()
	{
		std::time_t now = std::time(nullptr);
		struct tm tmv = {};
		localtime_r(&now, &tmv);

		char buf[32] = {};
		strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tmv);
		return std::string(buf);
	}

	int CopyFile(const std::string &src, const std::string &dst)
	{
		std::ifstream ifs(src, std::ios::binary);
		if (!ifs.is_open())
		{
			Logger::Warn(TAG, "Cannot open app log file: " + src);
			return -1;
		}

		std::ofstream ofs(dst, std::ios::binary | std::ios::trunc);
		if (!ofs.is_open())
		{
			Logger::Warn(TAG, "Cannot create app log copy: " + dst);
			return -1;
		}

		ofs << ifs.rdbuf();
		return ofs.good() ? 0 : -1;
	}

	int CopyDirRecursive(const std::string &src, const std::string &dst)
	{
		if (EnsureDir(dst) != 0)
		{
			return -1;
		}

		DIR *dir = opendir(src.c_str());
		if (dir == nullptr)
		{
			return -1;
		}

		int failures = 0;
		struct dirent *entry;
		while ((entry = readdir(dir)) != nullptr)
		{
			std::string name(entry->d_name);
			if (name == "." || name == "..")
			{
				continue;
			}

			std::string srcPath = JoinPath(src, name);
			std::string dstPath = JoinPath(dst, name);

			struct stat st = {};
			if (lstat(srcPath.c_str(), &st) != 0)
			{
				++failures;
				continue;
			}

			if (S_ISDIR(st.st_mode))
			{
				if (CopyDirRecursive(srcPath, dstPath) != 0)
				{
					++failures;
				}
			}
			else if (S_ISREG(st.st_mode))
			{
				if (CopyFile(srcPath, dstPath) != 0)
				{
					++failures;
				}
			}
		}

		closedir(dir);
		return failures == 0 ? 0 : -1;
	}
}

int TarSnapshotArchiver::CreateArchive(const std::string &sourceDir,
                                       const std::string &archivePath)
{
	std::string parent = sourceDir;
	size_t pos = parent.find_last_of('/');
	if (pos == std::string::npos)
	{
		parent = ".";
	}
	else
	{
		parent = parent.substr(0, pos);
	}

	std::string command = "tar -czf " + ShellQuote(archivePath) +
	                      " -C " + ShellQuote(parent) + " " +
	                      ShellQuote(BaseName(sourceDir));
	int ret = std::system(command.c_str());
	return ret == 0 ? 0 : -1;
}

SnapshotManager::SnapshotManager(PluginManager &pluginManager,
                                 ISnapshotArchiver *archiver)
	: m_pluginManager(pluginManager)
	, m_archiver(archiver != nullptr ? archiver : &m_defaultArchiver)
{
}

SnapshotResult SnapshotManager::CreateSnapshot(const SnapshotRequest &request)
{
	SnapshotResult result;
	result.snapshotDir = MakeSnapshotDir(request.outputDir);
	result.archivePath = result.snapshotDir + ".tar.gz";

	if (EnsureDir(request.outputDir) != 0 || EnsureDir(result.snapshotDir) != 0)
	{
		result.code = -1;
		return result;
	}

	result.appLogsStatus = CopyAppLogs(request, result.snapshotDir);

	SnapshotContext ctx;
	ctx.reason = request.reason;
	ctx.outputDir = request.outputDir;
	ctx.snapshotDir = result.snapshotDir;
	ctx.appLogDir = request.appLogDir;
	ctx.targetPid = request.targetPid;

	result.pluginFailures = m_pluginManager.SnapshotAll(ctx);
	if (result.pluginFailures != 0)
	{
		result.code = -2;
	}

	if (request.packArchive)
	{
		if (m_archiver->CreateArchive(result.snapshotDir, result.archivePath) != 0)
		{
			result.code = -3;
		}
	}
	else
	{
		result.archivePath.clear();
	}

	if (WriteSummary(request, result) != 0 && result.code == 0)
	{
		result.code = -4;
	}

	return result;
}

std::string SnapshotManager::MakeSnapshotDir(const std::string &outputDir) const
{
	std::string base = JoinPath(outputDir, "snapshot_" + TimestampForPath());
	if (!Exists(base))
	{
		return base;
	}

	for (int i = 1; i < 100; ++i)
	{
		std::ostringstream oss;
		oss << base << "_" << i;
		if (!Exists(oss.str()))
		{
			return oss.str();
		}
	}
	return base + "_overflow";
}

std::string SnapshotManager::CopyAppLogs(const SnapshotRequest &request,
                                         const std::string &snapshotDir) const
{
	if (request.appLogDir.empty())
	{
		return "not_requested";
	}

	if (!IsDir(request.appLogDir))
	{
		return "missing";
	}

	std::string dst = JoinPath(snapshotDir, "app_logs");
	if (CopyDirRecursive(request.appLogDir, dst) != 0)
	{
		return "copy_failed";
	}
	return "copied";
}

int SnapshotManager::WriteSummary(const SnapshotRequest &request,
                                  const SnapshotResult &result) const
{
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "reason", request.reason.c_str());
	cJSON_AddStringToObject(root, "snapshot_dir", result.snapshotDir.c_str());
	cJSON_AddStringToObject(root, "archive_path", result.archivePath.c_str());
	cJSON_AddStringToObject(root, "app_logs", result.appLogsStatus.c_str());
	cJSON_AddNumberToObject(root, "target_pid",
	                        static_cast<double>(request.targetPid));
	cJSON_AddNumberToObject(root, "plugin_failures",
	                        static_cast<double>(result.pluginFailures));
	cJSON_AddNumberToObject(root, "code", static_cast<double>(result.code));

	char *str = cJSON_PrintUnformatted(root);
	std::string json(str != nullptr ? str : "{}");
	if (str != nullptr)
	{
		cJSON_free(str);
	}
	cJSON_Delete(root);

	std::ofstream ofs(JoinPath(result.snapshotDir, "summary.json"),
	                  std::ios::out | std::ios::trunc);
	if (!ofs.is_open())
	{
		return -1;
	}

	ofs << json << "\n";
	return ofs.good() ? 0 : -1;
}
