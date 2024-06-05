
#include "profilerApi.h"

#include <random>
#include <fstream>
#include <string.h>
#include <chrono>
#include <vector>
#include <thread>
#include <iostream>

void wasteTime(size_t cnt)
{
	for (size_t i = 0; i < cnt; ++i)
	{
		volatile double s = std::sqrt(i + 1024 * 1024);
		s = s * s;
		s = std::sqrt(s);
	}
}

int testMacros()
{
	ThreadLocalHist(basic, 1000, 500, "basic test of macros");

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 2000, 100 };

	for (size_t i = 0; i < 1024 * 1024; i++)
	{
		size_t timeToWaist = static_cast<size_t>(std::round(dist(gen)));
	
		BeginProfiler(basic);
		wasteTime(timeToWaist);
		EndProfiler(basic);
		std::this_thread::sleep_for(std::chrono::milliseconds{10});
	}

	return 0;
}

int testMacrosMiltipleThreads()
{
	std::cout << "basic test of macros with threads" << std::endl;
	auto func{[](double stdev){
		std::random_device rd{};
		std::mt19937 gen{ rd() };
		std::normal_distribution<> dist{ 2000, 100 * stdev };

		for (size_t i = 0; i < 1024 * 1024; i++)
		{
			size_t timeToWaist = static_cast<size_t>(std::round(dist(gen)));

			ThreadLocalHist(basicThreads, 1000, 500, "basic test of macros with threads");

			BeginProfiler(basicThreads);
			wasteTime(timeToWaist);
			EndProfiler(basicThreads);
			std::this_thread::sleep_for(std::chrono::milliseconds{10});
		}
	}};
	
	std::vector<std::thread> threads;
	for (size_t i = 0 ; i < 4 ; ++i)
	{
		threads.emplace(threads.end(), func, i);
	}
	std::this_thread::sleep_for(std::chrono::seconds{1});
	for(auto& t : threads)
	{
		if(t.joinable())
			t.join();
	}
	
	return 0;
}

void testRateCnt()
{
	ThreadLocalRateCnt (rateEvents,
						1'000'000'000 /* events / second*/, 
						100, /* size of the array of last rates to keep*/ 
						"basic test of rate counter");

	size_t repeat{100000000};
	while(repeat-- != 0)
	{
		wasteTime(2000);
		RateCntSample(rateEvents, 1);
	}
}

int main(int /*argc*/, char* /*argv*/[])
{
	//testRateCnt();
	//testMacros();
	testMacrosMiltipleThreads();

	return 0;
}