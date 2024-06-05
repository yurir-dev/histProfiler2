#pragma once

#include <ostream>
#include <sstream>
#include <string>
#include <string.h>

#include "shmFile.h"

namespace profiler
{

struct shmHistHeader
{
	static constexpr uint64_t magic() { return 0x0BADBABE1CEC0FFE; }
public:
	shmHistHeader() = default;
	shmHistHeader(size_t samplesPerBucket, size_t numBuckets, const std::string& desc)
	: _magic{magic()}, _samplesPerBucket{samplesPerBucket}, _numBuckets{numBuckets}
	{
		strncpy(_description, desc.c_str(), sizeof(_description) - 1);
	}
	void clear()
	{
		_magic = _samplesPerBucket = _numBuckets = _maxSample = _minSample = _overfows = _sum = _numSamples = 0;
	}
	uint64_t _magic{0};
	uint64_t _samplesPerBucket{1};
	uint64_t _numBuckets{0};
	uint64_t _maxSample{ 0 };
	uint64_t _minSample{ std::numeric_limits<uint64_t>::max() };
	uint64_t _overfows{ 0 };
	uint64_t _sum{ 0 };
	uint64_t _numSamples{ 0 };
	char _description[128] = {'\0'};
	
};

std::ostream& operator<<(std::ostream& stream, const shmHistHeader& obj)
{
	auto mean{obj._numSamples > 0 ? obj._sum / obj._numSamples : 0};
	stream << obj._description
		<< " : _numBuckets: " << obj._numBuckets << ", _samplesPerBucket: " << obj._samplesPerBucket
		<< ", _maxSample: " << obj._maxSample << ", _minSample: " << obj._minSample
        << ", _overfows: " << obj._overfows << ", mean: " << mean << ", _numSamples: " << obj._numSamples;
	return stream;
}

struct histogram
{
	// std::to_string(gettid())
	histogram(uint64_t numSamplesPerBucket, uint64_t numBuckets,
			const std::string& id, size_t cnt_, const std::string& desc)
	: _shmHist{"shmFile_" + id + "_" + std::to_string(cnt_) + ".shm", 
		  		shmHistHeader{numSamplesPerBucket, numBuckets, desc},
				numBuckets}
	{}

	void begin()
	{
		_begin = std::chrono::system_clock::now();
	}
	void end()
	{
		const auto end{std::chrono::system_clock::now()};
		const auto diffNanos{std::chrono::duration_cast<std::chrono::nanoseconds>(end - _begin)};
		sample(diffNanos.count());

	}

	void sample(uint64_t sample)
	{
		auto& header{_shmHist.header()};
		auto* data{_shmHist.data()};

		if (sample > header._maxSample)
			header._maxSample = sample;
		if (sample < header._minSample)
			header._minSample = sample;
		
		const auto bucket{header._samplesPerBucket > 1 ? sample / header._samplesPerBucket : sample};
		if (bucket < header._numBuckets - 1)
		{
			++data[bucket];
		}
		else
		{
			++header._overfows;
			++data[header._numBuckets - 1];
		}

		header._sum += bucket;
		++header._numSamples;

		_shmHist.sync();
	}

	std::chrono::time_point<std::chrono::system_clock> _begin;
	shmFile<shmHistHeader, uint64_t> _shmHist;
};

struct shmRateHeader
{
	static constexpr uint64_t magic() { return 0x0BADBABEBADC0FFE; }
public:
	shmRateHeader() = default;
	shmRateHeader(size_t nanosPerBucket, size_t numBuckets, const std::string& desc)
	: _magic{magic()}, _nanosPerBucket{nanosPerBucket}, _numBuckets{numBuckets}
	{
		strncpy(_description, desc.c_str(), sizeof(_description) - 1);
	}

	uint64_t _magic{0};
	uint64_t _nanosPerBucket{1}; // time unit - 1'000'000'000 - per 1 second
	uint64_t _numBuckets{0};
	uint64_t _currentIndex{0};
	char _description[128] = {'\0'};
};

std::ostream& operator<<(std::ostream& stream, const shmRateHeader& obj)
{
	stream << obj._description
		<< " : _numBuckets: " << obj._numBuckets << ", _currentIndex: " << obj._currentIndex << ", _nanosPerBucket: " << obj._nanosPerBucket;
	return stream;
}

struct rateCounter
{
	rateCounter(uint64_t nanosPerBucket, uint64_t numBuckets,
			const std::string& id, size_t cnt, const std::string& desc)
			:_shmRate{"shmFile_" + id + "_" + std::to_string(cnt) + ".shm",
					  shmRateHeader{nanosPerBucket, numBuckets, desc}, 
					  numBuckets}
			{}

	void sample(size_t num = 1)
	{
		auto& header{_shmRate.header()};
		auto* data{_shmRate.data()};

		const auto nanos{std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()};
		const auto index{static_cast<size_t>((nanos / header._nanosPerBucket) % header._numBuckets)};
		data[index] += num;
		header._currentIndex = index;
	}

	shmFile<shmRateHeader, uint64_t> _shmRate;
};

}