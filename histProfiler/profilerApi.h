#pragma once

/*

TODO

1) global hist, begin() in one thread end() in another
2) create just hist , ThreadLocalHist(id, perBucket=1, 100, desc)

*/


#if defined (ENABLE_HIST_PROFILER)

#include "histogram.h"

#define var(x) x##_cnt

#define ThreadLocalHist(id, perBucket, num, description) \
	static size_t var(id);	\
	static thread_local profiler::histogram id{perBucket, num, #id, ++var(id), description};

#define BeginProfiler(id) do { id.begin(); } while(false)
#define EndProfiler(id) do { id.end(); } while(false)

#define ThreadLocalRateCnt(id, perBucket, num, description) \
	static size_t var(id);	\
	static thread_local profiler::rateCounter id{perBucket, num, #id, ++var(id), description};

#define RateCntSample(id, num) do { id.sample(num); } while(false)

#pragma message "compiled with hist profiler enabled"

#else

#define ThreadLocalHist(id, perBucket, num, description) do{;}while(false)
#define BeginProfiler(id) do{;}while(false)
#define EndProfiler(id) do{;}while(false)

#endif
