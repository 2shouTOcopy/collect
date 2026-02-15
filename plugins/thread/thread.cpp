#include "core/IPlugin.h"
#include "utils/Logger.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>

static const char *TAG = "thread";
static const char *TARGET_PROCESS = "m320_app";

/// Thread plugin — flush-only, collects thread info for target process.
/// Reports: thread states, CPU usage, stack, FD counts to file.

class ThreadPlugin : public IPlugin
{
public:
	std::string Name() const override { return "thread"; }
	bool HasFlush() const override { return true; }

	int Configure(const std::string &key, const std::string &val) override
	{
		if (key == "BaseDir")
		{
			m_baseDir = val;
		}
		else if (key == "TargetProcess")
		{
			m_targetProcess = val;
		}
		return 0;
	}

	int Flush(CdTime /*timeout*/) override
	{
		if (m_baseDir.empty())
		{
			Logger::Error(TAG, "BaseDir not configured");
			return -1;
		}

		const std::string outPath = m_baseDir + "/thread.txt";

		// Find PID
		pid_t pid = FindPidByName(m_targetProcess);
		if (pid <= 0)
		{
			Logger::Error(TAG, "Process '" + m_targetProcess + "' not found");
			return -1;
		}

		// Initialize HZ
		if (m_hz == 0)
		{
			m_hz = sysconf(_SC_CLK_TCK);
			if (m_hz <= 0)
			{
				m_hz = 100;
			}
		}

		auto now = std::chrono::steady_clock::now();
		double timeDelta = 0.0;
		if (m_initialized)
		{
			timeDelta = std::chrono::duration<double>(now - m_prevTimestamp).count();
		}

		// Read all threads
		std::string taskDir = "/proc/" + std::to_string(pid) + "/task";
		DIR *dir = opendir(taskDir.c_str());
		if (!dir)
		{
			Logger::Error(TAG, "Cannot open " + taskDir);
			return -1;
		}

		struct ThreadInfo
		{
			pid_t tid = 0;
			std::string name;
			char state = '?';
			unsigned long utime = 0, stime = 0;
			double cpuUsage = 0.0;
			double userTime = 0.0, sysTime = 0.0;
			unsigned long vmStackKb = 0;
			int fdCount = 0;
		};

		std::vector<ThreadInfo> threads;
		std::set<pid_t> currentTids;

		struct dirent *entry;
		while ((entry = readdir(dir)) != nullptr)
		{
			if (entry->d_type != DT_DIR)
			{
				continue;
			}

			pid_t tid = static_cast<pid_t>(strtol(entry->d_name, nullptr, 10));
			if (tid <= 0)
			{
				continue;
			}

			currentTids.insert(tid);
			ThreadInfo info;
			info.tid = tid;

			// Thread name
			std::string commPath = taskDir + "/" + entry->d_name + "/comm";
			std::ifstream commFile(commPath);
			if (commFile)
			{
				std::getline(commFile, info.name);
			}

			// Parse stat
			std::string statPath = taskDir + "/" + entry->d_name + "/stat";
			std::ifstream statFile(statPath);
			if (statFile)
			{
				std::string line;
				std::getline(statFile, line);
				std::istringstream ss(line);
				std::vector<std::string> fields;
				std::string field;
				while (ss >> field)
				{
					fields.push_back(field);
				}
				if (fields.size() > 2) info.state = fields[2][0];
				if (fields.size() > 13) info.utime = std::stoul(fields[13]);
				if (fields.size() > 14) info.stime = std::stoul(fields[14]);
			}

			info.userTime = static_cast<double>(info.utime) / m_hz;
			info.sysTime = static_cast<double>(info.stime) / m_hz;

			// CPU usage delta
			if (m_initialized && timeDelta > 0)
			{
				auto prevIt = m_prevCpuTimes.find(tid);
				if (prevIt != m_prevCpuTimes.end())
				{
					unsigned long delta =
						(info.utime - prevIt->second.first) +
						(info.stime - prevIt->second.second);
					info.cpuUsage = (static_cast<double>(delta) / m_hz) / timeDelta * 100.0;
				}
			}

			// Stack size
			std::string statusPath = taskDir + "/" + entry->d_name + "/status";
			std::ifstream statusFile(statusPath);
			if (statusFile)
			{
				std::string line;
				while (std::getline(statusFile, line))
				{
					if (line.compare(0, 6, "VmStk:") == 0)
					{
						info.vmStackKb = std::stoul(line.substr(6));
						break;
					}
				}
			}

			// FD count
			std::string fdPath = taskDir + "/" + entry->d_name + "/fd";
			DIR *fdDir = opendir(fdPath.c_str());
			if (fdDir)
			{
				struct dirent *fdEntry;
				while ((fdEntry = readdir(fdDir)) != nullptr)
				{
					if (strcmp(fdEntry->d_name, ".") != 0 &&
					    strcmp(fdEntry->d_name, "..") != 0)
					{
						info.fdCount++;
					}
				}
				closedir(fdDir);
			}

			threads.push_back(info);
			m_prevCpuTimes[tid] = {info.utime, info.stime};
		}
		closedir(dir);

		// Cleanup stale entries
		for (auto it = m_prevCpuTimes.begin(); it != m_prevCpuTimes.end(); )
		{
			if (currentTids.count(it->first) == 0)
			{
				it = m_prevCpuTimes.erase(it);
			}
			else
			{
				++it;
			}
		}

		m_prevTimestamp = now;
		m_initialized = true;

		// Write report
		std::ofstream ofs(outPath, std::ios::out | std::ios::trunc);
		if (!ofs.is_open())
		{
			Logger::Error(TAG, "Cannot open output file: " + outPath);
			return -1;
		}

		ofs << std::fixed << std::setprecision(2);
		ofs << "--- Thread Monitor: " << m_targetProcess
		    << " (PID " << pid << ") ---\n";
		ofs << "Total threads: " << threads.size() << "\n\n";

		ofs << std::left
		    << std::setw(8)  << "TID"
		    << std::setw(20) << "Name"
		    << std::setw(8)  << "State"
		    << std::setw(10) << "CPU%"
		    << std::setw(10) << "User(s)"
		    << std::setw(10) << "Sys(s)"
		    << std::setw(10) << "Stack(KB)"
		    << std::setw(8)  << "FDs"
		    << "\n";

		for (const auto &t : threads)
		{
			ofs << std::left
			    << std::setw(8)  << t.tid
			    << std::setw(20) << t.name.substr(0, 19)
			    << std::setw(8)  << t.state
			    << std::right
			    << std::setw(9)  << t.cpuUsage << "%"
			    << std::setw(10) << t.userTime
			    << std::setw(10) << t.sysTime
			    << std::setw(10) << t.vmStackKb
			    << std::setw(8)  << t.fdCount
			    << "\n";
		}

		return 0;
	}

private:
	pid_t FindPidByName(const std::string &name)
	{
		DIR *dir = opendir("/proc");
		if (!dir)
		{
			return -1;
		}

		struct dirent *entry;
		while ((entry = readdir(dir)) != nullptr)
		{
			if (entry->d_type != DT_DIR)
			{
				continue;
			}

			pid_t pid = static_cast<pid_t>(strtol(entry->d_name, nullptr, 10));
			if (pid <= 0)
			{
				continue;
			}

			std::string commPath = "/proc/" + std::string(entry->d_name) + "/comm";
			std::ifstream commFile(commPath);
			if (commFile)
			{
				std::string comm;
				std::getline(commFile, comm);
				if (comm == name)
				{
					closedir(dir);
					return pid;
				}
			}
		}
		closedir(dir);
		return -1;
	}

	std::string m_baseDir;
	std::string m_targetProcess = TARGET_PROCESS;
	long m_hz = 0;
	bool m_initialized = false;
	std::chrono::steady_clock::time_point m_prevTimestamp;
	std::map<pid_t, std::pair<unsigned long, unsigned long>> m_prevCpuTimes;
};

extern "C"
{
	IPlugin *CreateModule() { return new ThreadPlugin(); }
	void DestroyModule(IPlugin *p) { delete p; }
}
