
#include "histProfiler/histogram.h"
#include "histProfiler/utils.h"

#include <iostream>
#include <fstream>
#include <thread>

#if 1

void readFile(std::ifstream& fstream)
{
using namespace profiler;

	size_t numBuckets{0};
	uint64_t magic{0};
	fstream.read(reinterpret_cast<char*>(&magic), sizeof(magic));
	fstream.seekg(0, fstream.beg);

	if (magic == profiler::shmRateHeader::magic())
	{
		profiler::shmRateHeader header;
		fstream.read(reinterpret_cast<char*>(&header), sizeof(header));
		numBuckets = header._numBuckets;
		std::cout << header << std::endl;	
	}
	else if (magic == profiler::shmHistHeader::magic())
	{
		profiler::shmHistHeader header;
		fstream.read(reinterpret_cast<char*>(&header), sizeof(header));
		numBuckets = header._numBuckets;
		std::cout << header << std::endl;
	}
	else
	{
		profiler::Throw(std::runtime_error) << "unexpected magic: " << std::hex << magic << End;
	}

	fstream.seekg(4096, fstream.beg);
	for (size_t i = 0 ; i < numBuckets ; ++i)
	{
		uint64_t bucket{0};
		fstream.read(reinterpret_cast<char*>(&bucket), sizeof(bucket));
		std::cout << bucket << std::endl;
	}
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << "provide file name" << std::endl;
		return 1;
	}
	const char* fileName{argv[1]};
	std::ifstream fstream{fileName, std::ios::out | std::ios::binary};

	if (argc > 2)
	{
		while (true)
		{
			fstream.seekg(0, fstream.beg);
			readFile(fstream);
			std::this_thread::sleep_for(std::chrono::seconds{1});
		}
	}
	else
	{
		readFile(fstream);
	}
	
	return 0;
}
#else

#include <iostream>
#include <random>
#include <cmath>
#include <chrono>
#include <thread>
#include <set>
#include <fstream>


#if defined(_WIN32)
#include <windows.h>
#elif defined (linux)
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#endif
#include <pthread.h>
#else
#warning "thread pinning is not supported for this OS"
#endif


#include "profilerApi.h"
#include "common.h"

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
std::ofstream outputFile;
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
				outputFile = std::ofstream(outputFileName);
				if (!outputFile)
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
#if defined(_WIN32)
	DWORD mask = (1 << cpuNum);
	HANDLE th = GetCurrentThread();
	/*DWORD_PTR prev_mask = */ SetThreadAffinityMask(th, mask);
#elif defined (linux)
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

template <typename func, typename ... args_t>
auto profiled(func f, args_t ... args)
{
	return f(args...);
}

void printInt(int p1, int p2)
{
	if (p1 == p2)
		std::cout << "ret: " << 42 << std::endl;
	std::cout << "ret: " << 41 << std::endl;
}
int getInt(int p1, int p2)
{
	if (p1 == p2)
		std::cout << "ret: " << 52 << std::endl;
	std::cout << "ret: " << 51 << std::endl;
	return 52;
}

int main(int argc, char* argv[])
{
	profiled(printInt, 1, 2);
	int ret = profiled(getInt, 1, 2);
	std::cout << "ret: " << ret << std::endl;

	parseArgv(argc, argv);

#if defined (ENABLE_HIST_PROFILER)
	const std::string enableProfiler{"profiler is enabled"};
#else
	const std::string enableProfiler{ "profiler is NOT enabled" };
#endif

	std::cout << enableProfiler 
		<< ", testType: " << testType
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
	//else if (testType == "millis")
	//{
	//	testMillis();
	//}
	//else if (testType == "micros")
	//{
	//	testMicros();
	//}
	//else if (testType == "nanos")
	//{
	//	testNanos();
	//}
	else
	{
		usage("Unexpected test: " + testType);
	}

	std::cout << "finished: " << testType << " test, check the " << (!outputFileName.empty() ? outputFileName : "std:cout") << std::endl;

	return 1;
}

void testSeconds()
{
	profiler::context ctx {
		"/dev/shm/testseconds",
		{
			{profiler::label_1, 1'000'000'000, 10},
		}
	};

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 5, 1 };

	for (size_t i = 0; i < 10; i++)
	{
		int timeToSleep = static_cast<int>(std::round(dist(gen)));
		std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiler_Begin(ctx, profiler::label_1);
		std::this_thread::sleep_for(std::chrono::seconds(timeToSleep));
		HistProfiler_End(ctx, profiler::label_1);
	}

	if (outputFile)
		HistProfiler_DumpData(ctx, outputFile, profiler::outFormat::excel);
	else
		HistProfiler_DumpData(ctx, std::cout, profiler::outFormat::follow);
}
#endif

#if 0
void testMillis()
{
	profiler::context ctx {
		"/dev/shm/testMillis",
		{
			{profiler::label_1, 1'000'000, 20},
			{profiler::label_2, 1'000'000, 20},
		}
	};

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 10, 3 };

	for (size_t i = 0; i < 1000; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiled(ctx, profiler::label_1, busyMethods::getBusySpin, std::chrono::milliseconds(timeToSleep));

		//HistProfiler_Begin(ctx, "normal distribution");
		//busyMethods::getBusySpin(std::chrono::milliseconds(timeToSleep));
		//HistProfiler_End(ctx, "normal distribution");
	}

	// A gamma distribution with alpha=1, and beta=2
	// approximates an exponential distribution.
	std::gamma_distribution<> distGamma(1, 2);
	for (size_t i = 0; i < 1000; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(distGamma(gen)));
		std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiled(ctx, profiler::label_2, busyMethods::getBusySpin, std::chrono::milliseconds(timeToSleep));

		//HistProfiler_Begin(ctx, "gamma distribution");
		//busyMethods::getBusySpin(std::chrono::milliseconds(timeToSleep));
		//HistProfiler_End(ctx, "gamma distribution");
	}

	if (outputFile)
		HistProfiler_DumpData(ctx, outputFile, profiler::outFormat::excel);
	else
		HistProfiler_DumpData(ctx, std::cout, profiler::outFormat::follow);
}
void testMicros()
{
	profiler::context ctx {
		"/dev/shm/testMicros",
		{
			{profiler::label_1, 1'000, 100},
			{profiler::label_2, 1'000, 100},
		}
	};

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 10, 2 };

	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		timeToSleep += 10;
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiler_Begin(ctx, profiler::label_1);
		busyMethods::getBusySpin(std::chrono::microseconds(timeToSleep));
		//getBusySqrt(timeToSleep);
		HistProfiler_End(ctx, profiler::label_1);
	}
	for (size_t i = 0; i < N; i++)
	{
		size_t timeToSleep = static_cast<size_t>(std::round(dist(gen)));
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiler_Begin(ctx, profiler::label_2);
		busyMethods::getBusySpin(std::chrono::microseconds(timeToSleep));
		//getBusySqrt(timeToSleep);
		HistProfiler_End(ctx, profiler::label_2);
	}

	if (outputFile)
		HistProfiler_DumpData(ctx, outputFile, profiler::outFormat::excel);
	else
		HistProfiler_DumpData(ctx, std::cout, profiler::outFormat::follow);
}
void testNanos()
{
	profiler::context ctx {
		"/dev/shm/testNanos",
		{
			{profiler::label_1, 10, 500},
		}
	};

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 10, 2 };

	for (size_t i = 0; i < N; i++)
	{
		auto s = std::round(dist(gen));
		size_t timeToSleep = s > 0 ? static_cast<size_t>(s) : 0;
		//std::cout << "timeToSleep " << timeToSleep << std::endl;

		HistProfiler_Begin(ctx, {profiler::label_1);
		//getBusySpin(std::chrono::nanoseconds(timeToSleep));
		busyMethods::getBusySqrt(timeToSleep);
		HistProfiler_End(ctx, {profiler::label_1);
	}

	if (outputFile)
		HistProfiler_DumpData(ctx, outputFile, profiler::outFormat::excel);
	else
		HistProfiler_DumpData(ctx, std::cout, profiler::outFormat::follow);
}

#endif