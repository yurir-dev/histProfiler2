#pragma once

/*

TODO

1) global hist, begin() in one thread end() in another
2) create just hist , ThreadLocalTimeHist(id, perBucket=1, 100, desc)

*/


#if defined (ENABLE_HIST_PROFILER)

#include "histogram.h"

#define var(x) x##_cnt

/*
ThreadLocalTimeHist(histNum, 1, 100, "test hist with numbers");
std::random_device rd{};
std::mt19937 gen{ rd() };
std::normal_distribution<double> dist{ 40, 10 };
for (size_t i = 0; i < 1024 * 1024; i++)
{
	SampleHist(histNum, std::round(dist(gen)));
}
*/
#define ThreadLocalHist(id, num, description) \
	static size_t var(id);	\
	static thread_local profiler::histogram id{num, #id, ++var(id), description};

#define SampleHist(id, num) do { id.sample(num); } while(false)

#define ThreadLocalTimeHist(id, perBucket, num, description) \
	static size_t var(id);	\
	static thread_local profiler::timeHistogram id{perBucket, num, #id, ++var(id), description};

#define TimeHistBegin(id) do { id.begin(); } while(false)
#define TimeHistEnd(id) do { id.end(); } while(false)
#define TimeHistSample(id, beginTP, endTP) do { id.sample(beginTP, endTP); } while(false)

#define ThreadLocalRateCnt(id, perBucket, num, description) \
	static size_t var(id);	\
	static thread_local profiler::rateCounter id{perBucket, num, #id, ++var(id), description};

#define RateCntSample(id, num) do { id.sample(num); } while(false)

#pragma message "compiled with hist profiler enabled"

#else

#define ThreadLocalHist(id, num, description) do {;} while(false)
#define SampleHist(id, num) do {;} while(false)

#define ThreadLocalTimeHist(id, perBucket, num, description) do{;}while(false)
#define TimeHistBegin(id) do{;}while(false)
#define TimeHistEnd(id) do{;}while(false)
#define TimeHistSample(beginTP, endTP) do{;}while(false)

#define ThreadLocalRateCnt(id, perBucket, num, description) do{;}while(false)
#define RateCntSample(id, num) do {;} while(false)

#endif
