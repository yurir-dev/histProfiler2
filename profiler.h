#pragma once

#include <vector>
#include <string>

namespace profiler
{
	struct declaration
	{
		std::string label;
		size_t samplesPerBucket;
		size_t numBuckets;
	};
	void init(const std::vector<declaration>& declarations);

	void startProfiler();
	void endProfiler();

	void start(const std::string& label);
	void end(const std::string& label);
};