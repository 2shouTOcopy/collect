#include "core/IPlugin.h"
#include "utils/Logger.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <cstdint>

static const char *TAG = "cpu";

/// CPU plugin — reads /proc/stat, computes per-CPU usage percentages.
/// Reports: user, system, idle, wait, interrupt, softirq, steal, nice, active.

class CpuPlugin : public IPlugin
{
public:
	std::string Name() const override { return "cpu"; }
	bool HasRead() const override { return true; }

	int Configure(const std::string &key, const std::string &val) override
	{
		bool flag = (val == "true" || val == "1" || val == "yes");
		if (key == "ReportByCpu")        m_reportByCpu = flag;
		else if (key == "ReportByState") m_reportByState = flag;
		else if (key == "ValuesPercentage") m_reportPercent = flag;
		return 0;
	}

	int Read() override
	{
		std::ifstream fin("/proc/stat");
		if (!fin.is_open())
		{
			Logger::Error(TAG, "Failed to open /proc/stat");
			return -1;
		}

		std::vector<CpuData> currentCpus;
		std::string line;
		while (std::getline(fin, line))
		{
			// Skip aggregate "cpu " line, only parse "cpuN"
			if (line.compare(0, 3, "cpu") != 0 || !isdigit(line[3]))
			{
				continue;
			}

			std::istringstream iss(line);
			std::string label;
			iss >> label;

			CpuData cpu = {};
			iss >> cpu.user >> cpu.nice >> cpu.system >> cpu.idle
			    >> cpu.iowait >> cpu.irq >> cpu.softirq >> cpu.steal;

			// Compute total and active
			cpu.total = cpu.user + cpu.nice + cpu.system + cpu.idle +
			            cpu.iowait + cpu.irq + cpu.softirq + cpu.steal;
			cpu.active = cpu.total - cpu.idle - cpu.iowait;

			currentCpus.push_back(cpu);
		}

		CdTime now = CdTime::Now();

		// Compute deltas and percentages
		if (!m_prevCpus.empty() && m_prevCpus.size() == currentCpus.size())
		{
			for (size_t i = 0; i < currentCpus.size(); ++i)
			{
				const auto &prev = m_prevCpus[i];
				const auto &curr = currentCpus[i];

				uint64_t totalDelta = curr.total - prev.total;
				if (totalDelta == 0)
				{
					continue;
				}

				double scale = 100.0 / static_cast<double>(totalDelta);

				if (m_reportByCpu)
				{
					std::string inst = std::to_string(i);

					if (m_reportByState)
					{
						SubmitPercent(inst, "user", (curr.user - prev.user) * scale, now);
						SubmitPercent(inst, "system", (curr.system - prev.system) * scale, now);
						SubmitPercent(inst, "idle", (curr.idle - prev.idle) * scale, now);
						SubmitPercent(inst, "wait", (curr.iowait - prev.iowait) * scale, now);
						SubmitPercent(inst, "interrupt", (curr.irq - prev.irq) * scale, now);
						SubmitPercent(inst, "softirq", (curr.softirq - prev.softirq) * scale, now);
						SubmitPercent(inst, "steal", (curr.steal - prev.steal) * scale, now);
						SubmitPercent(inst, "nice", (curr.nice - prev.nice) * scale, now);
					}
					else
					{
						SubmitPercent(inst, "active", (curr.active - prev.active) * scale, now);
					}
				}
				else
				{
					// Aggregate all CPUs — accumulate
					// For simplicity, report per-CPU; aggregate in formatter
					std::string inst = std::to_string(i);
					SubmitPercent(inst, "active",
					              (curr.active - prev.active) * scale, now);
				}
			}
		}

		// Submit CPU count
		{
			DataSet ds;
			ds.type = "count";
			DataSource src;
			src.name = "value";
			src.type = DataSourceType::Gauge;
			ds.sources.push_back(src);

			ValueList vl;
			vl.plugin = "cpu";
			vl.type = "count";
			vl.time = now;
			vl.values.push_back(Value::Gauge(static_cast<double>(currentCpus.size())));

			Dispatch(ds, vl);
		}

		// Save for next cycle
		m_prevCpus = currentCpus;
		return 0;
	}

private:
	struct CpuData
	{
		uint64_t user = 0, nice = 0, system = 0, idle = 0;
		uint64_t iowait = 0, irq = 0, softirq = 0, steal = 0;
		uint64_t total = 0, active = 0;
	};

	void SubmitPercent(const std::string &cpuInst, const std::string &state,
	                   double pct, CdTime time)
	{
		if (std::isnan(pct))
		{
			return;
		}

		DataSet ds;
		ds.type = "percent";
		DataSource src;
		src.name = "value";
		src.type = DataSourceType::Gauge;
		ds.sources.push_back(src);

		ValueList vl;
		vl.plugin = "cpu";
		vl.pluginInstance = cpuInst;
		vl.type = "percent";
		vl.typeInstance = state;
		vl.time = time;
		vl.values.push_back(Value::Gauge(pct));

		Dispatch(ds, vl);
	}

	std::vector<CpuData> m_prevCpus;
	bool m_reportByCpu = true;
	bool m_reportByState = true;
	bool m_reportPercent = true;
};

extern "C"
{
	IPlugin *CreateModule() { return new CpuPlugin(); }
	void DestroyModule(IPlugin *p) { delete p; }
}
