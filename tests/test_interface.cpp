#include "profilerApi.h"
#include <random>
#include <fstream>

void wasteTime(size_t cnt)
{
	for (size_t i = 0; i < cnt; ++i)
	{
		volatile double s = std::sqrt(i + 1024 * 1024);
		s = s * s;
		s = std::sqrt(s);
	}
}

int testStrings()
{
	profiler::context ctx;

	// labels are strings, time units are microseconds
	HistProfiler_Init(ctx,
		{
			{"hist 1", 1'000, 100},
			{"hist 2", 1'000, 100},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 1000, 100 };
	for (size_t i = 0; i < 1024; i++)
	{
		size_t timeToWaist = static_cast<size_t>(std::round(dist(gen)));
	
		HistProfiler_Begin(ctx, "hist 1");
		wasteTime(timeToWaist);
		HistProfiler_End(ctx, "hist 1");

		timeToWaist += 100;

		HistProfiled(ctx, "hist 2", wasteTime, timeToWaist);
	}

	std::ofstream outputFile = std::ofstream("test_res_strings");
	HistProfiler_DumpData(ctx, outputFile, profiler::outFormat::excel);

	return 0;
}


enum class histTypes
{
	hist1,
	hist2
};
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
int testEnum()
{
	profiler::context<histTypes, hist2Types> ctx;

	// labels are enums, time units are microseconds
	HistProfiler_Init(ctx,
		{
			{histTypes::hist1, 1'000, 100},
			{histTypes::hist2, 1'000, 100},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 1000, 100 };
	for (size_t i = 0; i < 1024; i++)
	{
		size_t timeToWaist = static_cast<size_t>(std::round(dist(gen)));

		HistProfiler_Begin(ctx, histTypes::hist1);
		wasteTime(timeToWaist);
		HistProfiler_End(ctx, histTypes::hist1);

		timeToWaist += 100;

		HistProfiled(ctx, histTypes::hist2, wasteTime, timeToWaist);
	}

	std::ofstream outputFile = std::ofstream("test_res_enum");
	HistProfiler_DumpData(ctx, outputFile, profiler::outFormat::excel);

	return 0;
}

int main(int /*argc*/, char* /*argv*/[])
{
	testEnum();
	testStrings();
	
	return 0;
}