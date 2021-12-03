#include <iostream>
#include <random>
#include <cmath>
#include <chrono>
#include <thread>
#include <set>
#include <fstream>


#if defined(WIN32)
#include <windows.h>
#elif defined (LINUX)
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <pthread.h>
#endif

#include "profiler.h"

void testSeconds();
void testMillis();
void testMicros();
void testNanos();


std::string testType{};
std::set<std::string> testTypes{"seconds", "millis", "macros", "nanos"};
std::ostream& operator<<(std::ostream& os, const std::set<std::string>& dt)
{
	for (auto iter = dt.begin(); iter != dt.end(); ++iter)
		os << *iter << ",";
	os << "\b \b"; // rm the last ,
	return os;
}

std::string outputFileName;
size_t N{1000};
int cpuNum{-1};


void usage(const std::string& desc)
{
	std::cout << desc << std::endl
		<< "-h --help - help" << std::endl
		<< "-t [" << testTypes << "] - test type" << std::endl
		<< "-o --output [filename] - output file name" << std::endl
		<< "-n - number of iterations" << std::endl
		<< "-a --affinity - [cpu number]" << std::endl;

	exit(1);
}
void parseArgv(int argc, char* argv[])
{
	for (int i = 1; i < argc; i += 2)
	{
		if (std::string(argv[i]) == "-h" || std::string(argv[i]) == "--help")
		{
			usage("Usage");
		}
		else if (std::string(argv[i]) == "-t")
		{
			if (i + 1 < argc)
			{
				testType = argv[i + 1];
				if (testTypes.end() == testTypes.find(testType))
					usage("unexpected testType -t " + testType);
			}
			else
				usage("missing argument for " + std::string(argv[i]));
		}
		else if (std::string(argv[i]) == "-o" || std::string(argv[i]) == "--output")
		{
			if (i + 1 < argc)
			{
				outputFileName = argv[i + 1];
				std::ofstream f(outputFileName);
				if (!f)
					usage("FAILED to open " + outputFileName);
			}
			else
				usage("missing argument for " + std::string(argv[i]));
		}
		else if (std::string(argv[i]) == "-n")
		{
			if (i + 1 < argc)
			{
				std::string str = argv[i + 1];
				try
				{
					N = std::atoi(str.c_str());
				}
				catch (std::exception&)
				{
					usage("FAILED to convert -n " + str);
				}
			}
			else
				usage("missing argument for " + std::string(argv[i]));
		}
		else if (std::string(argv[i]) == "-a" || std::string(argv[i]) == "--affinity")
		{
			if (i + 1 < argc)
			{
				std::string str = argv[i + 1];
				try
				{
					cpuNum = std::atoi(str.c_str());
				}
				catch (std::exception&)
				{
					usage("FAILED to convert -a " + str);
				}
			}
			else
				usage("missing argument for " + std::string(argv[i]));
		}
		else
		{
			usage("unexpected argument " + std::string(argv[i]));
		}
	}
}

void pinCpu()
{
#if defined(WIN32)
	DWORD mask = (1 << cpuNum);
	HANDLE th = GetCurrentThread();
	DWORD_PTR prev_mask = SetThreadAffinityMask(th, mask);
#elif defined (LINUX)
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(i, &cpuset);
	int rc = pthread_setaffinity_np(threads[i].native_handle(),
		sizeof(cpu_set_t), &cpuset);
	if (rc != 0) {
		std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
	}
#endif
}

int main(int argc, char* argv[])
{
	parseArgv(argc, argv);

	std::cout << "testType: " << testType 
		<< ", outputFileName: " << outputFileName 
		<< ", N: " << N
		<< ", affinity: " << cpuNum
		<< std::endl;

	if (cpuNum > 0)
		pinCpu();

	//testSeconds();
	//testMillis();
	testMicros();
	//testNanos();

	return 0;
}

template< class Rep, class Period >
void getBusySleep(const std::chrono::duration<Rep, Period>& sleep_duration)
{
	std::this_thread::sleep_for(sleep_duration);
}
template< class Rep, class Period >
void getBusySpin(const std::chrono::duration<Rep, Period>& sleep_duration)
{
	const auto endTime = std::chrono::system_clock::now() + sleep_duration;
	while (auto now = std::chrono::system_clock::now() < endTime)
	{}
}

void getBusySqrt(size_t cnt)
{
	for (size_t i = 0; i < cnt; ++i)
	{
		volatile double s = std::sqrt(i);
		s = s * s;
		//s = std::sqrt(i);
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
		getBusySpin(std::chrono::milliseconds(timeToSleep));
		profiler::end("testMilliseconds");
	}

	profiler::endProfiler();
}
void testMicros()
{
	profiler::init({
		{"testMicrosecondsSpin1", 1'000, 100},
		{"testMicrosecondsSpin2", 1'000, 100},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	// values near the mean are the most likely
	// standard deviation affects the dispersion of generated values from the mean
	std::normal_distribution<> dist{ 10, 2 };

	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		timeToSleep += 10;
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::start("testMicrosecondsSpin1");
		getBusySpin(std::chrono::microseconds(timeToSleep));
		//getBusySqrt(timeToSleep);
		profiler::end("testMicrosecondsSpin1");
	}
	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::start("testMicrosecondsSpin2");
		getBusySpin(std::chrono::microseconds(timeToSleep));
		//getBusySqrt(timeToSleep);
		profiler::end("testMicrosecondsSpin2");
	}

	profiler::endProfiler(outputFileName);
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
	std::normal_distribution<> dist{ 50, 30 };

	for (size_t i = 0; i < N; i++)
	{
		auto s = std::round(dist(gen));
		size_t timeToSleep = s > 0 ? static_cast<size_t>(s) : 0;
		std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::start("testNanoseconds");
		getBusySpin(std::chrono::nanoseconds(timeToSleep));
		profiler::end("testNanoseconds");
	}

	profiler::endProfiler("test_res.txt");
}
