#include <iostream>
#include <random>
#include <cmath>
#include <chrono>
#include <thread>

#include "profiler.h"

int main(int argc, char* argv[])
{
	std::cout << "hello world" << std::endl;

	profiler::init({
		{"test1", 1'000'000'000, 10},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	// values near the mean are the most likely
	// standard deviation affects the dispersion of generated values from the mean
	std::normal_distribution<> dist{ 5, 1 };

	for (size_t i = 0; i < 10; i++)
	{
		int timeToSleep = std::round(dist(gen));
		std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::start("test1");
		std::this_thread::sleep_for(std::chrono::seconds(timeToSleep));
		profiler::end("test1");
	}

	profiler::endProfiler();

	return 0;
}