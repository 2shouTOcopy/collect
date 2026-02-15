#pragma once

#include <string>
#include <map>
#include "types/DataSet.h"

/// TypesDB loader — reads types.db and provides DataSet lookup.
/// Replaces linear search with O(1) map lookup.

class TypesDb
{
public:
	/// Load types definitions from a types.db file.
	int Load(const std::string &filePath);

	/// Look up a DataSet by type name. Returns nullptr if not found.
	const DataSet *GetDataSet(const std::string &typeName) const;

	/// Number of loaded types.
	size_t Size() const { return m_types.size(); }

private:
	std::map<std::string, DataSet> m_types;
};
