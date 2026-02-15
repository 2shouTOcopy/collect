#include "core/IPlugin.h"
#include "utils/Logger.h"

#include <sys/statvfs.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cerrno>

static const char *TAG = "df";

/// DF plugin — reads disk free space via statvfs() for select mount points.
/// Reports: df_complex (free, used, reserved) as gauge (bytes).

class DfPlugin : public IPlugin
{
public:
	std::string Name() const override { return "df"; }
	bool HasRead() const override { return true; }

	int Configure(const std::string &key, const std::string &val) override
	{
		if (key == "MountPoint")
		{
			m_mountPoints.push_back(val);
		}
		else if (key == "ValuesAbsolute")
		{
			m_absolute = (val == "true" || val == "1" || val == "yes");
		}
		else if (key == "ValuesPercentage")
		{
			m_percentage = (val == "true" || val == "1" || val == "yes");
		}
		return 0;
	}

	int Init() override
	{
		// If no mount points configured, default to "/"
		if (m_mountPoints.empty())
		{
			m_mountPoints.push_back("/");
		}
		return 0;
	}

	int Read() override
	{
		CdTime now = CdTime::Now();

		for (const auto &mp : m_mountPoints)
		{
			struct statvfs st = {};
			if (statvfs(mp.c_str(), &st) < 0)
			{
				Logger::Error(TAG, "statvfs(\"" + mp + "\") failed: " +
				              std::string(strerror(errno)));
				continue;
			}

			if (st.f_blocks == 0)
			{
				continue;
			}

			// Build instance name from mount point
			std::string inst;
			if (mp == "/")
			{
				inst = "root";
			}
			else
			{
				inst = mp.substr(1);
				for (auto &c : inst)
				{
					if (c == '/')
					{
						c = '-';
					}
				}
			}

			uint64_t blkSize = st.f_frsize;
			double freeBytes = static_cast<double>(st.f_bavail) * blkSize;
			double reservedBytes = static_cast<double>(st.f_bfree - st.f_bavail) * blkSize;
			double usedBytes = static_cast<double>(st.f_blocks - st.f_bfree) * blkSize;

			if (m_absolute)
			{
				SubmitGauge(inst, "df_complex", "free", freeBytes, now);
				SubmitGauge(inst, "df_complex", "reserved", reservedBytes, now);
				SubmitGauge(inst, "df_complex", "used", usedBytes, now);
			}

			if (m_percentage && st.f_blocks > 0)
			{
				double total = static_cast<double>(st.f_blocks);
				SubmitGauge(inst, "percent_bytes", "free",
				            static_cast<double>(st.f_bavail) / total * 100.0, now);
				SubmitGauge(inst, "percent_bytes", "reserved",
				            static_cast<double>(st.f_bfree - st.f_bavail) / total * 100.0, now);
				SubmitGauge(inst, "percent_bytes", "used",
				            static_cast<double>(st.f_blocks - st.f_bfree) / total * 100.0, now);
			}
		}

		return 0;
	}

private:
	void SubmitGauge(const std::string &pluginInst,
	                 const std::string &type,
	                 const std::string &typeInst,
	                 double value, CdTime time)
	{
		DataSet ds;
		ds.type = type;
		DataSource src;
		src.name = "value";
		src.type = DataSourceType::Gauge;
		ds.sources.push_back(src);

		ValueList vl;
		vl.plugin = "df";
		vl.pluginInstance = pluginInst;
		vl.type = type;
		vl.typeInstance = typeInst;
		vl.time = time;
		vl.values.push_back(Value::Gauge(value));

		Dispatch(ds, vl);
	}

	std::vector<std::string> m_mountPoints;
	bool m_absolute = true;
	bool m_percentage = false;
};

extern "C"
{
	IPlugin *CreateModule() { return new DfPlugin(); }
	void DestroyModule(IPlugin *p) { delete p; }
}
