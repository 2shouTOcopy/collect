#include "core/IPlugin.h"
#include "output/JsonFormatter.h"
#include "utils/Logger.h"

#include <fstream>
#include <iostream>
#include <string>

static const char *TAG = "json_writer";

/// JSON Writer plugin — writes AI/LLM friendly structured JSON to file or stdout.
/// Supports batch mode for efficient output.

class JsonWriterPlugin : public IPlugin
{
public:
	std::string Name() const override { return "json_writer"; }
	bool HasWrite() const override { return true; }
	bool HasFlush() const override { return true; }

	int Configure(const std::string &key, const std::string &val) override
	{
		if (key == "OutputFile")
		{
			m_outputFile = val;
		}
		else if (key == "StdOut")
		{
			m_useStdout = (val == "true" || val == "1" || val == "yes");
		}
		else if (key == "Host")
		{
			m_formatter.SetHost(val);
		}
		return 0;
	}

	int Init() override
	{
		if (!m_outputFile.empty())
		{
			m_ofs.open(m_outputFile, std::ios::app);
			if (!m_ofs.is_open())
			{
				Logger::Error(TAG, "Failed to open output file: " + m_outputFile);
				return -1;
			}
			Logger::Info(TAG, "Writing to: " + m_outputFile);
		}

		if (m_useStdout)
		{
			Logger::Info(TAG, "Writing to stdout");
		}

		return 0;
	}

	int Write(const DataSet &ds, const ValueList &vl) override
	{
		std::string json = m_formatter.Format(ds, vl);
		json += "\n";

		if (m_ofs.is_open())
		{
			m_ofs << json;
		}

		if (m_useStdout)
		{
			std::cout << json;
		}

		return 0;
	}

	int Flush(CdTime timeout) override
	{
		(void)timeout;

		if (m_ofs.is_open())
		{
			m_ofs.flush();
		}

		if (m_useStdout)
		{
			std::cout.flush();
		}

		return 0;
	}

	int Shutdown() override
	{
		if (m_ofs.is_open())
		{
			m_ofs.close();
		}
		Logger::Info(TAG, "Shutdown");
		return 0;
	}

private:
	JsonFormatter m_formatter;
	std::string m_outputFile;
	std::ofstream m_ofs;
	bool m_useStdout = true;  // Default to stdout
};

extern "C"
{
	IPlugin *CreateModule()
	{
		return new JsonWriterPlugin();
	}

	void DestroyModule(IPlugin *p)
	{
		delete p;
	}
}
