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
#else
#warning "thread pinning is not supported for this OS"
#endif

#include "profiler.h"

void testSeconds();
void testMillis();
void testMicros();
void testNanos();


std::string testType{"micros"};
std::set<std::string> testTypes{"seconds", "millis", "micros", "nanos"};
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
	CPU_SET(cpuNum, &cpuset);
	int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	if (rc != 0) {
		std::cerr << "Error calling pthread_setaffinity_np: " << rc << std::endl;
	}
#else
	std::cerr << "thread pinning is not supported for this OS" << std::endl;
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

	if (testType == "seconds")
	{
		testSeconds();
	}
	else if (testType == "millis")
	{
		testMillis();
	}
	else if (testType == "micros")
	{
		testMicros();
	}
	else if (testType == "nanos")
	{
		testNanos();
	}
	else
	{
		usage("Unexpected test: " + testType);
	}

	std::cout << "finished: " << testType << " test, check the " << (!outputFileName.empty() ? outputFileName : "std:cout") << std::endl;

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
		volatile double s = std::sqrt(i + 1024 * 1024);
		s = s * s;
		//s = std::sqrt(i);
	}
}

void testSeconds()
{
	profiler::init({
		{"normal distribution", 1'000'000'000, 10},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 5, 1 };

	for (size_t i = 0; i < 10; i++)
	{
		int timeToSleep = static_cast<int>(std::round(dist(gen)));
		std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::begin("normal distribution");
		std::this_thread::sleep_for(std::chrono::seconds(timeToSleep));
		profiler::end("normal distribution");
	}

	if (!outputFileName.empty())
		profiler::getData(outputFileName);
	else
		profiler::getData();
}
void testMillis()
{
	profiler::init({
		{"normal distribution", 1'000'000, 20},
		{"gamma distribution", 1'000'000, 20},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 10, 3 };

	for (size_t i = 0; i < 1000; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::begin("normal distribution");
		getBusySpin(std::chrono::milliseconds(timeToSleep));
		profiler::end("normal distribution");
	}

	// A gamma distribution with alpha=1, and beta=2
	// approximates an exponential distribution.
	std::gamma_distribution<> distGamma(1, 2);
	for (size_t i = 0; i < 1000; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(distGamma(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::begin("gamma distribution");
		getBusySpin(std::chrono::milliseconds(timeToSleep));
		profiler::end("gamma distribution");
	}

	if (!outputFileName.empty())
		profiler::getData(outputFileName);
	else
		profiler::getData();
}
void testMicros()
{
	profiler::init({
		{"normal distribution 1", 1'000, 100},
		{"normal distribution 2", 1'000, 100},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 10, 2 };

	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		timeToSleep += 10;
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::begin("normal distribution 1");
		getBusySpin(std::chrono::microseconds(timeToSleep));
		//getBusySqrt(timeToSleep);
		profiler::end("normal distribution 1");
	}
	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::begin("normal distribution 2");
		getBusySpin(std::chrono::microseconds(timeToSleep));
		//getBusySqrt(timeToSleep);
		profiler::end("normal distribution 2");
	}

	if (!outputFileName.empty())
		profiler::getData(outputFileName);
	else
		profiler::getData();
}
void testNanos()
{
	profiler::init({
		{"normal distribution", 10, 500},
		});

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 10, 2 };

	for (size_t i = 0; i < N; i++)
	{
		auto s = std::round(dist(gen));
		size_t timeToSleep = s > 0 ? static_cast<size_t>(s) : 0;
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		profiler::begin("normal distribution");
		//getBusySpin(std::chrono::nanoseconds(timeToSleep));
		getBusySqrt(timeToSleep);
		profiler::end("normal distribution");
	}

	if (!outputFileName.empty())
		profiler::getData(outputFileName);
	else
		profiler::getData();
}
