#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <chrono>
#include <string.h> // memset

namespace profiler
{
	struct histogram
	{
		constexpr uint64_t magic() { return 0x0BADBABE1CEC0FFE; }
	public:
		histogram() = default;
		histogram(size_t samplesPerBucket, size_t numBuckets)
		: _magic{magic()}, _samplesPerBucket{samplesPerBucket}, _numBuckets{numBuckets}
		{}
		histogram(const histogram&) = delete;
		histogram& operator=(const histogram&) = delete;
		histogram(histogram&&) = delete;
		histogram& operator=(histogram&&) = delete;

		void clear()
		{
			_magic = _samplesPerBucket = _numBuckets = 
				_maxSample = _minSample = _overfows = _sum = _numSamples = 0;
		}

		uint64_t _magic{0};
		uint64_t _samplesPerBucket{1};
		uint64_t _numBuckets{0};
		uint64_t _maxSample{ 0 };
		uint64_t _minSample{ 0xfffffffffffffffLL };
		uint64_t _overfows{ 0 };
		uint64_t _sum{ 0 };
		uint64_t _numSamples{ 0 };
		// buckets will follow in uint64_t
	};

	

}