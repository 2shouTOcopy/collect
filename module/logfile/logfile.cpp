#include <iostream>
#include <assert.h>
#include <memory>
#include <time.h>
#include <sstream>
#include <mutex>

#include "logfile.h"
#include "../daemon/PluginService.h"
#include "../oconfig/configfile.h"

int CLogfileModule::config(const std::string &key, const std::string &val)
{
	return 0;
}

int CLogfileModule::flush()
{
	const std::string strDir = ConfigManager::Instance().GetGlobalOption("BaseDir");
	if (strDir.empty())
	{
		ERROR("dmesg plugin: BaseDir is not configured.");
		return -1;
	}

	const std::string command = "/mnt/app/toolbox log_record export " + strDir;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(
		popen(command.c_str(), "r"), pclose);
	if (!pipe)
	{
		ERROR("logfile plugin: Failed to execute command '%s'", command.c_str());
	}
	else
	{
		if (ferror(pipe.get()))
		{
			ERROR("logfile plugin: execute command '%s' output failed!", command.c_str());
		}
	}

	return 0;
}

int CLogfileModule::read()
{
	return 0;
}

int CLogfileModule::write(const data_set_t *ds, const value_list_t *vl)
{
	if (!ds || !vl || 0 != strcmp(ds->type, vl->type))
	{
		ERROR("logfile: 无效参数或类型不匹配 (%s/%s)", 
			ds ? ds->type : "null", vl ? vl->type : "null");
		return -1;
	}

	// 获取基础路径
	const std::string baseDir = ConfigManager::Instance().GetGlobalOption("BaseDir");
	if (baseDir.empty())
	{
		ERROR("logfile: BaseDir未配置");
		return -1;
	}

	// 创建日志文件路径
	const std::string logfile = baseDir + "/collect_data.log";

	// 获取当前时间格式化
	std::time_t rawtime = time(nullptr);
	struct tm timeinfo;
	localtime_r(&rawtime, &timeinfo);
	char timestr[64];
	strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &timeinfo);

	// 创建日志行
	std::ostringstream oss;
	oss.precision(3);
	oss << timestr << " [" << vl->plugin;
	
	if (strlen(vl->plugin_instance) > 0)
		oss << "." << vl->plugin_instance;
	
	oss << "] " << vl->type;
	
	if (strlen(vl->type_instance) > 0)
		oss << "." << vl->type_instance;
	
	oss << " = ";

	// 根据数据类型格式化值
	for (size_t i = 0; i < ds->ds_num; ++i)
	{
		if (i > 0)
			oss << ", ";
			
		const auto &dsrc = ds->ds[i];
		const auto &val = vl->values[i];
		
		oss << dsrc.name << ":";
		
		switch (dsrc.type)
		{
			case DS_TYPE_GAUGE:
				oss << val.gauge;
				break;
			case DS_TYPE_COUNTER:
				oss << static_cast<uint64_t>(val.counter);
				break;
			case DS_TYPE_DERIVE:
				oss << val.derive;
				break;
			case DS_TYPE_ABSOLUTE:
				oss << static_cast<uint64_t>(val.absolute);
				break;
			default:
				oss << "未知类型";
		}
	}

	// 线程安全写入文件
	static std::mutex ioMtx;
	std::lock_guard<std::mutex> lock(ioMtx);
	
	FILE *fp = fopen(logfile.c_str(), "a");
	if (!fp)
	{
		ERROR("logfile: 无法打开文件 %s: %s", logfile.c_str(), strerror(errno));
		return -1;
	}

	fprintf(fp, "%s\n", oss.str().c_str());
	fclose(fp);
	
	return 0;
}

CAbstractUserModule *CreateModule()
{
	return new CLogfileModule();
}

void DestroyModule(CAbstractUserModule *pUserModule)
{
	assert(pUserModule != NULL);
	
	delete pUserModule;
	pUserModule = NULL;
}

