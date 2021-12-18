#include <iostream>
#include <random>
#include <cmath>
#include <chrono>
#include <thread>
#include <set>
#include <fstream>

#include "profiler.h"
#include "common.h"

void testMillis(size_t N)
{
	HistProfiler_Init({
		{"normal distribution", 1'000'000, 20},
		{"gamma distribution", 1'000'000, 20},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 10, 3 };

	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiler_Begin("normal distribution");
		busyMethods::getBusySpin(std::chrono::milliseconds(timeToSleep));
		HistProfiler_End("normal distribution");
	}

	// A gamma distribution with alpha=1, and beta=2
	// approximates an exponential distribution.
	std::gamma_distribution<> distGamma(1, 2);
	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(distGamma(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiler_Begin("gamma distribution");
		busyMethods::getBusySpin(std::chrono::milliseconds(timeToSleep));
		HistProfiler_End("gamma distribution");
	}

	std::ofstream outputFile = std::ofstream("test_res");
	if (outputFile)
		HistProfiler_DumpData(outputFile, profiler::outFormat::excel);
	else
		HistProfiler_DumpData(std::cout, profiler::outFormat::follow);
}

int main(int argc, char* argv[])
{
	testMillis(128);

	// TODO add verification

	return 0;
};