#include "core/IPlugin.h"
#include "utils/Logger.h"

#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cerrno>

static const char *TAG = "memory";

/// Memory plugin — reads /proc/meminfo and dispatches memory metrics.
/// Reports: used, free, buffered, cached, slab, available (as gauge, bytes).

class MemoryPlugin : public IPlugin
{
public:
	std::string Name() const override { return "memory"; }
	bool HasRead() const override { return true; }

	int Configure(const std::string &key, const std::string &val) override
	{
		if (key == "ValuesAbsolute")
		{
			m_absolute = (val == "true" || val == "1" || val == "yes");
		}
		else if (key == "ValuesPercentage")
		{
			m_percentage = (val == "true" || val == "1" || val == "yes");
		}
		return 0;
	}

	int Read() override
	{
		std::ifstream fin("/proc/meminfo");
		if (!fin.is_open())
		{
			Logger::Error(TAG, "Failed to open /proc/meminfo");
			return -1;
		}

		double memTotal = 0, memFree = 0, memBuffered = 0;
		double memCached = 0, memSlab = 0, memAvailable = 0;
		bool hasAvailable = false;

		std::string line;
		while (std::getline(fin, line))
		{
			ParseLine(line, "MemTotal:", memTotal);
			ParseLine(line, "MemFree:", memFree);
			ParseLine(line, "Buffers:", memBuffered);
			ParseLine(line, "Cached:", memCached);
			ParseLine(line, "Slab:", memSlab);
			if (ParseLine(line, "MemAvailable:", memAvailable))
			{
				hasAvailable = true;
			}
		}

		double memUsed = memTotal - memFree - memBuffered - memCached - memSlab;
		if (memUsed < 0)
		{
			memUsed = 0;
		}

		CdTime now = CdTime::Now();

		if (m_absolute)
		{
			SubmitGauge("used", memUsed, now);
			SubmitGauge("free", memFree, now);
			SubmitGauge("buffered", memBuffered, now);
			SubmitGauge("cached", memCached, now);
			SubmitGauge("slab", memSlab, now);
			if (hasAvailable)
			{
				SubmitGauge("available", memAvailable, now);
			}
		}

		if (m_percentage && memTotal > 0)
		{
			SubmitPercent("used", memUsed / memTotal * 100.0, now);
			SubmitPercent("free", memFree / memTotal * 100.0, now);
			SubmitPercent("buffered", memBuffered / memTotal * 100.0, now);
			SubmitPercent("cached", memCached / memTotal * 100.0, now);
		}

		return 0;
	}

private:
	bool ParseLine(const std::string &line, const char *key, double &out)
	{
		if (line.compare(0, strlen(key), key) != 0)
		{
			return false;
		}

		size_t pos = line.find(':');
		if (pos == std::string::npos)
		{
			return false;
		}

		std::string numStr = line.substr(pos + 1);
		// Trim leading spaces
		size_t start = numStr.find_first_not_of(" \t");
		if (start == std::string::npos)
		{
			return false;
		}
		size_t end = numStr.find_first_of(" \t", start);
		if (end != std::string::npos)
		{
			numStr = numStr.substr(start, end - start);
		}
		else
		{
			numStr = numStr.substr(start);
		}

		try
		{
			out = std::stod(numStr) * 1024.0;  // kB → bytes
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	void SubmitGauge(const std::string &instance, double value, CdTime time)
	{
		DataSet ds;
		ds.type = "memory";
		DataSource src;
		src.name = "value";
		src.type = DataSourceType::Gauge;
		ds.sources.push_back(src);

		ValueList vl;
		vl.plugin = "memory";
		vl.type = "memory";
		vl.typeInstance = instance;
		vl.time = time;
		vl.values.push_back(Value::Gauge(value));

		Dispatch(ds, vl);
	}

	void SubmitPercent(const std::string &instance, double pct, CdTime time)
	{
		DataSet ds;
		ds.type = "percent";
		DataSource src;
		src.name = "value";
		src.type = DataSourceType::Gauge;
		ds.sources.push_back(src);

		ValueList vl;
		vl.plugin = "memory";
		vl.type = "percent";
		vl.typeInstance = instance;
		vl.time = time;
		vl.values.push_back(Value::Gauge(pct));

		Dispatch(ds, vl);
	}

	bool m_absolute = true;
	bool m_percentage = false;
};

extern "C"
{
	IPlugin *CreateModule() { return new MemoryPlugin(); }
	void DestroyModule(IPlugin *p) { delete p; }
}
