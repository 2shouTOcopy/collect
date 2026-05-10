#include "core/IPlugin.h"
#include "utils/Logger.h"

#include <fstream>
#include <cstring>
#include <cerrno>
#include <ctime>

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/time.h>
#else
#include <sys/sysinfo.h>
#endif

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
		double uptime = 0.0;
		if (GetUptime(uptime) != 0)
		{
			Logger::Error(TAG, "uptime read failed: " + std::string(strerror(errno)));
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
		vl.values.push_back(Value::Gauge(uptime));

		Dispatch(ds, vl);
		return 0;
	}

private:
	int GetUptime(double &uptime) const
	{
#ifdef __APPLE__
		struct timeval bootTime = {};
		size_t len = sizeof(bootTime);
		int mib[2] = {CTL_KERN, KERN_BOOTTIME};
		if (sysctl(mib, 2, &bootTime, &len, nullptr, 0) != 0)
		{
			return -1;
		}
		std::time_t now = std::time(nullptr);
		uptime = difftime(now, bootTime.tv_sec);
		return 0;
#else
		struct sysinfo info = {};
		if (sysinfo(&info) != 0)
		{
			return -1;
		}
		uptime = static_cast<double>(info.uptime);
		return 0;
#endif
	}
};

extern "C"
{
	IPlugin *CreateModule() { return new UptimePlugin(); }
	void DestroyModule(IPlugin *p) { delete p; }
}
