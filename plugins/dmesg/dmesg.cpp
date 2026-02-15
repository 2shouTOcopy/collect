#include "core/IPlugin.h"
#include "utils/Logger.h"

#include <fstream>
#include <string>
#include <cstdio>
#include <memory>

static const char *TAG = "dmesg";

/// Dmesg plugin — flush-only, captures kernel ring buffer to file.

class DmesgPlugin : public IPlugin
{
public:
	std::string Name() const override { return "dmesg"; }
	bool HasFlush() const override { return true; }

	int Configure(const std::string &key, const std::string &val) override
	{
		if (key == "BaseDir")
		{
			m_baseDir = val;
		}
		else if (key == "OutputFile")
		{
			m_outputFile = val;
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

		const std::string outPath = m_baseDir + "/" + m_outputFile;

		std::unique_ptr<FILE, decltype(&pclose)> pipe(
			popen("dmesg", "r"), pclose);
		if (!pipe)
		{
			Logger::Error(TAG, "popen(dmesg) failed");
			return -1;
		}

		std::ofstream ofs(outPath, std::ios::out | std::ios::trunc);
		if (!ofs.is_open())
		{
			Logger::Error(TAG, "Cannot open output file: " + outPath);
			return -1;
		}

		char buffer[4096];
		while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr)
		{
			ofs << buffer;
		}

		return 0;
	}

private:
	std::string m_baseDir;
	std::string m_outputFile = "dmesg.txt";
};

extern "C"
{
	IPlugin *CreateModule() { return new DmesgPlugin(); }
	void DestroyModule(IPlugin *p) { delete p; }
}
