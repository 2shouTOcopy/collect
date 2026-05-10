#include "core/IPlugin.h"
#include "output/CsvFormatter.h"
#include "utils/Logger.h"

#include <fstream>
#include <string>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static const char *TAG = "csv";

/// CSV writer plugin — writes ValueList data as CSV lines to files.
/// Each plugin+type combination gets its own CSV file with header.

class CsvPlugin : public IPlugin
{
public:
	std::string Name() const override { return "csv"; }
	bool HasWrite() const override { return true; }

	int Configure(const std::string &key, const std::string &val) override
	{
		if (key == "DataDir")
		{
			m_useStdout = false;
			if (val == "stdout")
			{
				m_useStdout = true;
			}
			else
			{
				m_dataDir = val;
				// Trim trailing slashes
				while (!m_dataDir.empty() && m_dataDir.back() == '/')
				{
					m_dataDir.pop_back();
				}
			}
		}
		else if (key == "FileDate")
		{
			m_withDate = (val == "true" || val == "1" || val == "yes");
		}
		return 0;
	}

	int Write(const DataSet &ds, const ValueList &vl) override
	{
		std::string line = m_formatter.Format(ds, vl);

		if (m_useStdout)
		{
			std::string id = BuildId(vl);
			std::cout << "PUTVAL " << id << " " << line << "\n";
			return 0;
		}

		// Build file path
		std::string filePath = BuildFilePath(vl);
		if (filePath.empty())
		{
			return -1;
		}

		// Touch file with header if first time
		TouchCsvFile(filePath, ds, vl);

		// Append data line
		FILE *fp = fopen(filePath.c_str(), "a");
		if (!fp)
		{
			Logger::Error(TAG, "fopen(" + filePath + ") failed: " +
			              std::string(strerror(errno)));
			return -1;
		}

		fprintf(fp, "%s\n", line.c_str());
		fclose(fp);

		return 0;
	}

private:
	std::string BuildId(const ValueList &vl) const
	{
		std::string id = vl.plugin;
		if (!vl.pluginInstance.empty())
		{
			id += "-" + vl.pluginInstance;
		}
		id += "/" + vl.type;
		if (!vl.typeInstance.empty())
		{
			id += "-" + vl.typeInstance;
		}
		return id;
	}

	std::string BuildFilePath(const ValueList &vl) const
	{
		std::string path;
		if (!m_dataDir.empty())
		{
			path = m_dataDir + "/";
		}

		path += BuildId(vl);

		if (m_withDate)
		{
			std::time_t now = std::time(nullptr);
			struct tm tmv = {};
			if (localtime_r(&now, &tmv) != nullptr)
			{
				char dateBuf[16];
				if (strftime(dateBuf, sizeof(dateBuf), "-%Y-%m-%d", &tmv) > 0)
				{
					path += dateBuf;
				}
			}
		}

		return path;
	}

	void TouchCsvFile(const std::string &filePath,
	                  const DataSet &ds,
	                  const ValueList &vl)
	{
		struct stat st = {};
		if (stat(filePath.c_str(), &st) == 0 && S_ISREG(st.st_mode))
		{
			return;  // Already exists
		}

		FILE *fp = fopen(filePath.c_str(), "w");
		if (!fp)
		{
			Logger::Error(TAG, "Cannot create: " + filePath);
			return;
		}

		std::string header = CsvFormatter::Header(ds, vl);
		fprintf(fp, "%s\n", header.c_str());
		fclose(fp);
	}

	CsvFormatter m_formatter;
	std::string m_dataDir;
	bool m_useStdout = false;
	bool m_withDate = false;
};

extern "C"
{
	IPlugin *CreateModule() { return new CsvPlugin(); }
	void DestroyModule(IPlugin *p) { delete p; }
}
