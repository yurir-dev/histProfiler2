#include <iostream>
#include <random>
#include <cmath>
#include <chrono>
#include <thread>
#include <set>
#include <fstream>

#include "profilerApi.h"
#include "common.h"
#include "histVerificator.h"

void testMicros(size_t N, const std::string& outFilename, size_t offset = 0)
{
	profiler::context ctx;
	HistProfiler_Init(ctx, 
		{
			{"normal distribution 1", 1'000, 100, offset},
			{"normal distribution 2", 1'000, 100, offset},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 10, 2 };

	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		timeToSleep += 10;
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiler_Begin(ctx, "normal distribution 1");
		busyMethods::getBusySpin(std::chrono::microseconds(timeToSleep));
		//getBusySqrt(timeToSleep);
		HistProfiler_End(ctx, "normal distribution 1");
	}
	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiler_Begin(ctx, "normal distribution 2");
		busyMethods::getBusySpin(std::chrono::microseconds(timeToSleep));
		//getBusySqrt(timeToSleep);
		HistProfiler_End(ctx, "normal distribution 2");
	}

	std::ofstream outputFile = std::ofstream(outFilename);
	if (outputFile)
		HistProfiler_DumpData(ctx, outputFile, profiler::outFormat::excel);
	else
		HistProfiler_DumpData(ctx, std::cout, profiler::outFormat::follow);
}

int main(int argc, char* argv[])
{
	const size_t N{1024};
	const std::string outputFileName{"test_res_micros"};
	std::vector<histVerificator::histInfo > infos{
		{"normal distribution 1", N},
		{"normal distribution 2", N }
	};

	bool res{ true };
	{
		testMicros(N, outputFileName, 0);
		std::string error;
		bool rc = histVerificator::verify(infos, outputFileName, static_cast<int>(profiler::outFormat::excel), error);
		if (!rc)
		{
			std::cerr << __FILE__ << ":" << __LINE__ << " - " << error << std::endl;
		}
		res &= rc;
	}
	{
		testMicros(N, outputFileName, 10);
		std::string error;
		bool rc = histVerificator::verify(infos, outputFileName, static_cast<int>(profiler::outFormat::excel), error);
		if (!rc)
		{
			std::cerr << __FILE__ << ":" << __LINE__ << " - " << error << std::endl;
		}
		res &= rc;
	}
	return res ? 0 : 1;
};
