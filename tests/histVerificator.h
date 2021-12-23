#pragma once

#include "profiler.h"
#include <string>
#include <vector>


class histVerificator
{
public:
	struct histInfo
	{
		std::string label;
		size_t numEntries{ 0 };
	};

	/*
		return: empty string on success
				error string on failures
	*/
	static bool verify(const std::vector<histInfo>& histInfos, const std::string& fileName, profiler::outFormat frmt, std::string& errors);
};
