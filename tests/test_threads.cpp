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


int main(int argc, char* argv[])
{
	const size_t N {1024};
	const std::string outputFileName{ "test_res_threads" };

	profiler::context ctx;
	HistProfiler_Init(ctx,
		{
			{"dist 1", 1'000, 90, 0},
			{"dist 2", 1'000, 100, 0}
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 10, 2 };

	auto task = [&](size_t offset, const std::string& label) {
		for (size_t i = 0; i < N; i++)
		{
			size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
			timeToSleep += offset;
			//std::cout << "timeToSleep " << timeToSleep << std::endl;

			HistProfiler_BeginTid(ctx, label);
			busyMethods::getBusySpin(std::chrono::microseconds(timeToSleep));
			//getBusySqrt(timeToSleep);
			HistProfiler_EndTid(ctx, label);
		}
	};

	std::vector<std::thread> threads;
	for (size_t i = 0; i < 20; i++)
		if ((i & 2) == 0)
			threads.push_back(std::thread(task, i * 5, "dist 1"));
		else
			threads.push_back(std::thread(task, i * 5, "dist 2"));

	for (auto& t : threads)
		if (t.joinable())
			t.join();

	std::ofstream outputFile = std::ofstream(outputFileName);
	if (outputFile)
		HistProfiler_DumpData(ctx, outputFile, profiler::outFormat::excel);
	else
		HistProfiler_DumpData(ctx, std::cout, profiler::outFormat::follow);

	return 0;
}