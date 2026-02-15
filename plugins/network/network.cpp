#include "core/IPlugin.h"
#include "utils/Logger.h"

#include <fstream>
#include <string>
#include <cstdio>
#include <memory>

static const char *TAG = "network";

/// Network plugin — flush-only, captures network diagnostics to file.
/// Collects: /proc/net/dev, ip addr, ip route, resolv.conf, netstat, arp.

class NetworkPlugin : public IPlugin
{
public:
	std::string Name() const override { return "network"; }
	bool HasFlush() const override { return true; }

	int Configure(const std::string &key, const std::string &val) override
	{
		if (key == "BaseDir")
		{
			m_baseDir = val;
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

		const std::string outPath = m_baseDir + "/network_status.txt";

		std::ofstream ofs(outPath, std::ios::out | std::ios::trunc);
		if (!ofs.is_open())
		{
			Logger::Error(TAG, "Cannot open output file: " + outPath);
			return -1;
		}

		// Interface Statistics
		ofs << "=== Interface Statistics (/proc/net/dev) ===\n";
		AppendFile(ofs, "/proc/net/dev");

		// IP Addresses
		ofs << "=== IP Addresses (ip addr show) ===\n";
		AppendCommand(ofs, "ip addr show");

		// Routing Table
		ofs << "=== Routing Table (ip route show) ===\n";
		AppendCommand(ofs, "ip route show");

		// DNS Configuration
		ofs << "=== DNS Configuration (/etc/resolv.conf) ===\n";
		AppendFile(ofs, "/etc/resolv.conf");

		// Active Connections
		ofs << "=== Active Connections (netstat -anp) ===\n";
		AppendCommand(ofs, "netstat -anp");

		// ARP Cache
		ofs << "=== ARP Cache (arp -n) ===\n";
		AppendCommand(ofs, "arp -n");

		return 0;
	}

private:
	void AppendCommand(std::ofstream &ofs, const char *cmd)
	{
		std::unique_ptr<FILE, decltype(&pclose)> pipe(
			popen(cmd, "r"), pclose);
		if (!pipe)
		{
			ofs << "[Failed to execute: " << cmd << "]\n\n";
			return;
		}

		char buf[4096];
		while (fgets(buf, sizeof(buf), pipe.get()) != nullptr)
		{
			ofs << buf;
		}
		ofs << "\n";
	}

	void AppendFile(std::ofstream &ofs, const char *filepath)
	{
		std::ifstream ifs(filepath);
		if (ifs.is_open())
		{
			ofs << ifs.rdbuf();
			ofs << "\n\n";
		}
		else
		{
			ofs << "[Failed to read: " << filepath << "]\n\n";
		}
	}

	std::string m_baseDir;
};

extern "C"
{
	IPlugin *CreateModule() { return new NetworkPlugin(); }
	void DestroyModule(IPlugin *p) { delete p; }
}
