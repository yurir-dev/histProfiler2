#include <iostream>
#include <random>
#include <cmath>
#include <chrono>
#include <thread>

#include "profiler.h"

void testSeconds();
void testMillis();
void testMicros();
void testNanos();


int main(int argc, char* argv[])
{
	std::cout << "hello world" << std::endl;

	//testSeconds();
	//testMillis();
	//testMicros();
	testNanos();

	return 0;
}

template< class Rep, class Period >
void getBusy(const std::chrono::duration<Rep, Period>& sleep_duration)
{
	//std::this_thread::sleep_for(std::chrono::nanoseconds(t));
	const auto endTime = std::chrono::system_clock::now() + sleep_duration;
	while (auto now = std::chrono::system_clock::now() < endTime)
	{}
}
void getBusy(size_t cnt)
{
	for (size_t i = 0; i < cnt; ++i)
	{
		volatile double s = std::sqrt(i);
		s = s * s;
		s = std::sqrt(i);
	}
}

void testSeconds()
{
	profiler::init({
		{"testSeconds", 1'000'000'000, 10},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	// values near the mean are the most likely
	// standard deviation affects the dispersion of generated values from the mean
	std::normal_distribution<> dist{ 5, 1 };

	for (size_t i = 0; i < 10; i++)
	{
		int timeToSleep = static_cast<int>(std::round(dist(gen)));
		std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::start("testSeconds");
		std::this_thread::sleep_for(std::chrono::seconds(timeToSleep));
		profiler::end("testSeconds");
	}

	profiler::endProfiler();
}
void testMillis()
{
	profiler::init({
		{"testMilliseconds", 1'000'000, 20},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	// values near the mean are the most likely
	// standard deviation affects the dispersion of generated values from the mean
	std::normal_distribution<> dist{ 5, 1 };

	for (size_t i = 0; i < 1000; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::start("testMilliseconds");
		getBusy(std::chrono::milliseconds(timeToSleep));
		profiler::end("testMilliseconds");
	}

	profiler::endProfiler();
}
void testMicros()
{
	profiler::init({
		{"testMicroseconds", 1'000, 20},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	// values near the mean are the most likely
	// standard deviation affects the dispersion of generated values from the mean
	std::normal_distribution<> dist{ 5, 1 };

	for (size_t i = 0; i < 1000; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::start("testMicroseconds");
		getBusy(std::chrono::microseconds(timeToSleep));
		profiler::end("testMicroseconds");
	}

	profiler::endProfiler();
}
void testNanos()
{
	profiler::init({
		{"testNanoseconds", 10, 100},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	// values near the mean are the most likely
	// standard deviation affects the dispersion of generated values from the mean
	std::normal_distribution<> dist{ 50, 15 };

	for (size_t i = 0; i < 100; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::start("testNanoseconds");
		getBusy(timeToSleep);
		profiler::end("testNanoseconds");
	}

	profiler::endProfiler();
}
