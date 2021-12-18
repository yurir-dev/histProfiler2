#include <iostream>
#include <random>
#include <cmath>
#include <chrono>
#include <thread>
#include <set>
#include <fstream>

#include "profiler.h"
#include "common.h"

void testMicros(size_t N)
{
	HistProfiler_Init({
		{"normal distribution 1", 1'000, 100, 10},
		{"normal distribution 2", 1'000, 100, 10},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 10, 2 };

	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		timeToSleep += 10;
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiler_Begin("normal distribution 1");
		busyMethods::getBusySpin(std::chrono::microseconds(timeToSleep));
		//getBusySqrt(timeToSleep);
		HistProfiler_End("normal distribution 1");
	}
	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiler_Begin("normal distribution 2");
		busyMethods::getBusySpin(std::chrono::microseconds(timeToSleep));
		//getBusySqrt(timeToSleep);
		HistProfiler_End("normal distribution 2");
	}

	std::ofstream outputFile = std::ofstream("test_res");
	if (outputFile)
		HistProfiler_DumpData(outputFile, profiler::outFormat::excel);
	else
		HistProfiler_DumpData(std::cout, profiler::outFormat::follow);
}

int main(int argc, char* argv[])
{
	testMicros(1024);

	// TODO add verification

	return 0;
};