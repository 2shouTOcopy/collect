#include "CsvFormatter.h"
#include "types/DataSet.h"
#include "types/ValueList.h"

std::string CsvFormatter::Format(const DataSet &ds, const ValueList &vl)
{
	// TODO Phase 4: implement CSV formatting
	(void)ds;
	(void)vl;
	return "";
}

std::string CsvFormatter::Header(const DataSet &ds)
{
	// TODO Phase 4: generate CSV header
	(void)ds;
	return "timestamp,plugin,type,value";
}
