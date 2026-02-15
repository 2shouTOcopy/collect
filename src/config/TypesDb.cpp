#include "TypesDb.h"

#include <iostream>

int TypesDb::Load(const std::string &filePath)
{
	// TODO Phase 1+: parse types.db file
	(void)filePath;
	std::cerr << "[TypesDb] Load: stub\n";
	return 0;
}

const DataSet *TypesDb::GetDataSet(const std::string &typeName) const
{
	auto it = m_types.find(typeName);
	if (it != m_types.end())
	{
		return &(it->second);
	}
	return nullptr;
}
