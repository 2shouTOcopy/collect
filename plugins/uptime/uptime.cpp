#include "core/IPlugin.h"
#include "utils/Logger.h"

#include <fstream>
#include <sys/sysinfo.h>
#include <cstring>
#include <cerrno>

static const char *TAG = "uptime";

/// Uptime plugin — reads system uptime from sysinfo().
/// Reports as gauge (seconds since boot).

class UptimePlugin : public IPlugin
{
public:
	std::string Name() const override { return "uptime"; }
	bool HasRead() const override { return true; }

	int Read() override
	{
		struct sysinfo info = {};
		if (sysinfo(&info) != 0)
		{
			Logger::Error(TAG, "sysinfo() failed: " + std::string(strerror(errno)));
			return -1;
		}

		DataSet ds;
		ds.type = "uptime";
		DataSource src;
		src.name = "value";
		src.type = DataSourceType::Gauge;
		src.min = 0;
		src.max = 0;
		ds.sources.push_back(src);

		ValueList vl;
		vl.plugin = "uptime";
		vl.type = "uptime";
		vl.time = CdTime::Now();
		vl.values.push_back(Value::Gauge(static_cast<double>(info.uptime)));

		Dispatch(ds, vl);
		return 0;
	}
};

extern "C"
{
	IPlugin *CreateModule() { return new UptimePlugin(); }
	void DestroyModule(IPlugin *p) { delete p; }
}
