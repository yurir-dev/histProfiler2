#pragma once


/*
	Use these macros, if compiled without ENABLE_HIST_PROFILER flag,
	it won't add any penalty

	Example:

	1)	default profiler uses string labels("normal distribution 1") as histogram identifier   
		init two histograms with microseconds resolution and 100 buckets
		
	profiler::context ctx;
	HistProfiler_Init(ctx,
		{
			{"normal distribution 1", 1'000, 100},
			{"normal distribution 2", 1'000, 100},
		});

	// take samples
	loop {
		HistProfiler_Begin(ctx, "normal distribution 1");
		...
		HistProfiler_End(ctx, "normal distribution 1");
	}

	// dump the collected samples into a file or standard output
	std::ofstream outputFile = std::ofstream("test_res");
	if (outputFile)
		HistProfiler_DumpData(ctx, outputFile, profiler::outFormat::excel);
	else
		HistProfiler_DumpData(ctx, std::cout, profiler::outFormat::follow);


	2)	profiler uses enum labels as histogram identifier, begin&end operations will be faster.
		profiler::context<histTypes, hist2Types> ctx; first template parameter - label type 
		will be used by std::hash<label_t>::hash(label), so it must be a simple type.

		init two histograms with microseconds resolution and 100 buckets.

	enum class histTypes{hist1, hist2};
	struct hist2Types
	{
		std::string operator()(histTypes t)
		{
			switch (t)
			{
				case histTypes::hist1: return "enum hist1";
				case histTypes::hist2: return "enum hist2";
				default: return "invalid";
			}
		}
	};

	profiler::context<histTypes, hist2Types> ctx;

	// labels are enums, time units are microseconds
	HistProfiler_Init(ctx,
		{
			{histTypes::hist1, 1'000, 100},
			{histTypes::hist2, 1'000, 100},
		});

	loop {
		HistProfiler_Begin(ctx, histTypes::hist1);
		...
		HistProfiler_End(ctx, histTypes::hist1);
	}

	// dump the collected samples into a file or standard output
	std::ofstream outputFile = std::ofstream("test_res");
	if (outputFile)
		HistProfiler_DumpData(ctx, outputFile, profiler::outFormat::excel);
	else
		HistProfiler_DumpData(ctx, std::cout, profiler::outFormat::follow);
*/

#if defined (ENABLE_HIST_PROFILER)

#include "profiler.h"

#define HistProfiler_Init(ctx, ...) do { ctx.init(__VA_ARGS__); } while(false)
#define HistProfiler_DumpData(ctx, stream, format) do { ctx.getData(stream, format); } while(false)
#define HistProfiler_Begin(ctx, label) do { ctx.begin(label); } while(false)
#define HistProfiler_End(ctx, label) do { ctx.end(label); } while(false)
#define HistProfiler_BeginTid(ctx, label) do { ctx.beginTid(label); } while(false)
#define HistProfiler_EndTid(ctx, label) do { ctx.endTid(label); } while(false)

#define HistProfiled(ctx, label, func, ...) do { HistProfiler_Begin(ctx, label); func(__VA_ARGS__); HistProfiler_End(ctx, label); } while(false)

#pragma message ("compiled with histnamespace profiler")


#else

namespace profiler
{
	enum class outFormat { follow, excel };

	template <class Label_t = void, class Label2Str_t = void>
	class context {};
};
#define HistProfiler_Init(...) ((void)ctx)
#define HistProfiler_DumpData(...) ((void)0)
#define HistProfiler_Begin(...) ((void)0)
#define HistProfiler_End(...) ((void)0)
#define HistProfiler_BeginTid(...) ((void)0)
#define HistProfiler_EndTid(...) ((void)0)

#define HistProfiled(ctx, label, func, ...) do { func(__VA_ARGS__); } while(false)

#endif
