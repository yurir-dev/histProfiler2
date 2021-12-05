#pragma once

#include <vector>
#include <string>

namespace profiler
{
	struct declaration
	{
		/* id string for current histogram, should be used in begin/end functions*/
		std::string label;

		/*
			it samples time in nanoseconds, every sample will be divided by samplesPerBucket
			so, if samplesPerBucket is 1, each bucket will contain 1 ns,
			if 10, each bucket will contain 10 ns,
			if 1'000, each bucket will contain 1000 ns = 1 micro,
			if 1'000'000, each bucket will contain 1'000'000 ns = 1 milli,
			if 1'000'000'000, each bucket will contain 1'000'000'000 ns = 1 second,
		*/
		size_t samplesPerBucket{1};

		/*	number of buckets, overflows will be counted and their respective weight will be stored
			in an additional bucket allocated at the end.
		*/
		size_t numBuckets{1};
	};

	/*
		inits everything, can be called several times.
		examples:
		1) test in seconds
			profiler::init({
				{"normal distribution", 1'000'000'000, 10},
			});
		2) test milliseconds
			profiler::init({
				{"normal distribution", 1'000'000, 20},
				{"gamma distribution", 1'000'000, 20},
			});
		2) test microseconds
			profiler::init({
				{"normal distribution 1", 1'000, 100},
				{"normal distribution 2", 1'000, 100},
			});
		3) test in 10th of nanoseconds
			profiler::init({
				{"normal distribution", 10, 500},
			});
	*/
	void init(const std::vector<declaration>& declarations);

	/* 
		dumps all the data to std::cout or a text file
		std::cout : histogram after histogram.
		text file : each histogram in a separate column, separated by a tab,
					copy&paste into excel, or just open the file from excel
	*/
	void getData();
	void getData(const std::string& fileName);

	/*
		take current timestamp in nanos (std::chrono::system_clock::now() is used)
		end() calculates the difference and stores it in the assosiated histogram.
		if label was not declared in init(), will do nothing.
		
		example:
		profiler::init({
			{"test my code in millis", 1'000'000, 20},
		});

		begin("test my code in millis");
		...
		end("test my code in millis");
	*/
	void begin(const std::string& label);
	void end(const std::string& label);
};
