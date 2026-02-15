#include "core/IPlugin.h"
#include "utils/Logger.h"

#include <fstream>
#include <string>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <memory>

static const char *TAG = "logfile";

/// Logfile plugin — flush-only, exports logs via external tool.
/// Also has write capability for plain-text log output.

class LogfilePlugin : public IPlugin
{
public:
	std::string Name() const override { return "logfile"; }
	bool HasWrite() const override { return true; }
	bool HasFlush() const override { return true; }

	int Configure(const std::string &key, const std::string &val) override
	{
		if (key == "BaseDir")
		{
			m_baseDir = val;
		}
		else if (key == "LogFile")
		{
			m_logFile = val;
		}
		return 0;
	}

	int Write(const DataSet &ds, const ValueList &vl) override
	{
		if (m_baseDir.empty())
		{
			Logger::Error(TAG, "BaseDir not configured");
			return -1;
		}

		const std::string logPath = m_baseDir + "/" + m_logFile;

		// Build log line
		std::string logLine = "[" + vl.plugin;
		if (!vl.pluginInstance.empty())
		{
			logLine += "." + vl.pluginInstance;
		}
		logLine += "] " + vl.type;
		if (!vl.typeInstance.empty())
		{
			logLine += "." + vl.typeInstance;
		}
		logLine += " = ";

		for (size_t i = 0; i < vl.values.size(); ++i)
		{
			if (i > 0)
			{
				logLine += ", ";
			}

			const auto &val = vl.values[i];
			std::string dsName = (i < ds.sources.size()) ? ds.sources[i].name : "?";
			logLine += dsName + ":";

			if (val.IsGauge())
			{
				logLine += std::to_string(val.AsGauge());
			}
			else if (val.IsDerive())
			{
				logLine += std::to_string(val.AsDerive());
			}
			else if (val.IsCounter())
			{
				logLine += std::to_string(val.AsCounter());
			}
		}

		FILE *fp = fopen(logPath.c_str(), "a");
		if (!fp)
		{
			Logger::Error(TAG, "Cannot open " + logPath + ": " +
			              std::string(strerror(errno)));
			return -1;
		}

		fprintf(fp, "%s\n", logLine.c_str());
		fclose(fp);

		return 0;
	}

	int Flush(CdTime /*timeout*/) override
	{
		if (m_baseDir.empty())
		{
			Logger::Error(TAG, "BaseDir not configured");
			return -1;
		}

		const std::string command =
			"/mnt/app/toolbox log_record export " + m_baseDir;

		std::unique_ptr<FILE, decltype(&pclose)> pipe(
			popen(command.c_str(), "r"), pclose);
		if (!pipe)
		{
			Logger::Error(TAG, "Failed to execute: " + command);
			return -1;
		}

		return 0;
	}

private:
	std::string m_baseDir;
	std::string m_logFile = "collect_data.log";
};

extern "C"
{
	IPlugin *CreateModule() { return new LogfilePlugin(); }
	void DestroyModule(IPlugin *p) { delete p; }
}
