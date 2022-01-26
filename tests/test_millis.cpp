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

void testMillis(size_t N, const std::string& outFilename, size_t offset = 0)
{
	profiler::context ctx;
	HistProfiler_Init(ctx, 
		{
			{"normal distribution", 1'000'000, 100, offset},
			{"gamma distribution", 1'000'000, 100, offset},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 10, 3 };

	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiler_Begin(ctx, "normal distribution");
		busyMethods::getBusySpin(std::chrono::milliseconds(timeToSleep));
		HistProfiler_End(ctx, "normal distribution");
	}

	// A gamma distribution with alpha=1, and beta=2
	// approximates an exponential distribution.
	std::gamma_distribution<> distGamma(1, 2);
	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(distGamma(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiler_Begin(ctx, "gamma distribution");
		busyMethods::getBusySpin(std::chrono::milliseconds(timeToSleep));
		HistProfiler_End(ctx, "gamma distribution");
	}

	std::ofstream outputFile = std::ofstream(outFilename);
	if (outputFile)
		HistProfiler_DumpData(ctx, outputFile, profiler::outFormat::excel);
	else
		HistProfiler_DumpData(ctx, std::cout, profiler::outFormat::follow);
}

int main(int /*argc*/, char* /*argv*/[])
{
	const size_t N{ 128 };
	const std::string outputFileName{ "test_res_millis" };
	std::vector<histVerificator::histInfo > infos{
		{"normal distribution", N},
		{"gamma distribution", N }
	};

	bool res{ true };
	{
		testMillis(N, outputFileName, 0);
		std::string error;
		bool rc = histVerificator::verify(infos, outputFileName, static_cast<int>(profiler::outFormat::excel), error);
		if (!rc)
		{
			std::cerr << error << std::endl;
			std::cout << error << std::endl;
		}
		res &= rc;
	}
	{
		testMillis(N, outputFileName, 10);
		std::string error;
		bool rc = histVerificator::verify(infos, outputFileName, static_cast<int>(profiler::outFormat::excel), error);
		if (!rc)
		{
			std::cerr << error << std::endl;
			std::cout << error << std::endl;
		}
		res &= rc;
	}
	return res ? 0 : 1;

	return 0;
};