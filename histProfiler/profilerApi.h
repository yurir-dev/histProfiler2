#pragma once

/*

*/


#if defined (ENABLE_HIST_PROFILER)

#include "histogram.h"

#define var(x) x##_cnt


/*
	simple histogram

	ThreadLocalHist(histNum, - shmFile_histNum.shm
					100, - number of buckets
					"$", - X axis description
					"test hist with numbers");
	
	SampleHist(histNum, std::round(dist(gen)));
*/
#define ThreadLocalHist(id, num, XAxisDesc, description) \
	static size_t var(id);	\
	static thread_local profiler::histogram id{num, #id, ++var(id), XAxisDesc, description};

#define SampleHist(id, num) do { id.sample(num); } while(false)

/*
	used to measure code execution in specified time units,

ThreadLocalTimeHist(basic, - shmFile_basic.shm 
					1000, - 1000 nanos per bucket - microseconds
					500, - number of buckets  
					"basic test of macros");

TimeHistBegin(basic);
...
TimeHistEnd(basic);
*/
#define ThreadLocalTimeHist(id, perBucket, num, description) \
	static size_t var(id);	\
	static thread_local profiler::timeHistogram id{perBucket, num, #id, ++var(id), description};

#define TimeHistBegin(id) do { id.begin(); } while(false)
#define TimeHistEnd(id) do { id.end(); } while(false)
#define TimeHistSample(id, beginTP, endTP) do { id.sample(beginTP, endTP); } while(false)

/*
	used to measure rate per  specified time units - second or less than a second

	ThreadLocalRateCnt (rateEvents,
						1'000'000'000 - events / second, 
						100, - size of the array of last rates to keep
						"basic test of rate counter");
	
	RateCntSample(rateEvents, 1);
*/
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
