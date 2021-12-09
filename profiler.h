#pragma once

#include <vector>
#include <string>

#include <iostream>

namespace profiler
{
	struct declaration
	{
		//~declaration()
		//{
		//	std::cout << "declaration dtor: " << label << std::endl;
		//}

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
		outFormat::follow : histogram after histogram.
		outFormat::excel : each histogram in a separate column, separated by a tab,
							copy&paste into excel, or just open the file from excel
	*/
	enum class outFormat {follow, excel};
	void getData(std::ostream& out, outFormat f);

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



/*
	Use these macros, if compiled without ENABLE_HIST_PROFILER flag,
	it won't add any penalty

	Example:

	// init two histograms with microseconds resolution and 100 buckets
	HistProfiler_Init({
		{"normal distribution 1", 1'000, 100},
		{"normal distribution 2", 1'000, 100},
		});

	// take samples
	loop {
		HistProfiler_Begin("normal distribution 1");
		...
		HistProfiler_End("normal distribution 1");
	}

	// dump the collected samples into a file or standard output
	if (outputFile)
		HistProfiler_DumpData(outputFile, profiler::outFormat::excel);
	else
		HistProfiler_DumpData(std::cout, profiler::outFormat::follow);

*/

#if defined (ENABLE_HIST_PROFILER)
#define HistProfiler_Init(...) do { profiler::init(__VA_ARGS__); } while(false)
#define HistProfiler_DumpData(stream, format) do { profiler::getData(stream, format); } while(false)
#define HistProfiler_Begin(label) do { profiler::begin(label); } while(false)
#define HistProfiler_End(label) do { profiler::end(label); } while(false)
#else
#define HistProfiler_Init(...) ((void)0)
#define HistProfiler_DumpData(stream, format) ((void)0)
#define HistProfiler_Begin(label) ((void)0)
#define HistProfiler_End(label) ((void)0)
#endif

