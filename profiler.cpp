#include "profiler.h"

#include <unordered_map>
#include <chrono>
#include <iostream>
#include <sstream>

using namespace profiler;

namespace profiler{




struct histogram
{
public:
	histogram() = default;
	histogram(const declaration& d)
	{
		m_declaration = d;
		numBuckets.resize(d.numBuckets);
		for (size_t i = 0; i < numBuckets.size(); ++i)
			numBuckets[i] = 0;
	}

	void setCurrentTS(std::chrono::time_point<std::chrono::system_clock> ts) { m_ts = ts; }
	std::chrono::time_point<std::chrono::system_clock> getCurrentTS() const { return m_ts; }

	void input(uint64_t s)
	{
		if (s > m_maxSample) m_maxSample = s;
		if (s < m_minSample) m_minSample = s;

		m_sum += s;
		++m_numSamples;

		const size_t bucket = s / m_declaration.samplesPerBucket;
		if (bucket < numBuckets.size())
			++numBuckets[bucket];
		else
		{
			++m_overfows;
		}
	}

	std::string toString() const;
	//std::string toJson();

	double average()const { return m_numSamples > 0 ? (double)m_sum / (double)m_numSamples : 0.0; }
	uint64_t median()const { return 0; }

private:
	std::vector<uint64_t> numBuckets;
	declaration m_declaration;
	uint64_t m_maxSample{0};
	uint64_t m_minSample{ 0xfffffffffffffffLL };
	uint64_t m_overfows{0};
	uint64_t m_sum{0};
	uint64_t m_numSamples{0};

	std::chrono::time_point<std::chrono::system_clock> m_ts;

};

struct context
{
	std::unordered_map<std::string, histogram> histograms;
};




context ctx;


void init(const std::vector<declaration>& declarations)
{
	ctx.histograms.reserve(declarations.size());
	for (auto& d : declarations)
		ctx.histograms[d.label] = histogram(d);
}

void startProfiler()
{
	// TODO, start taking samples only after this function
	// allow to warmup
}
void endProfiler()
{
	// dump everything to a file
	for (auto iter = ctx.histograms.cbegin(); iter != ctx.histograms.end(); ++iter)
		std::cout << iter->second.toString() << std::endl;
}

void start(const std::string& label)
{
	auto iter = ctx.histograms.find(label);
	if (iter != ctx.histograms.end())
		iter->second.setCurrentTS(std::chrono::system_clock::now());
}
void end(const std::string& label)
{
	const auto iter = ctx.histograms.find(label);
	if (iter != ctx.histograms.end())
	{
		const auto diffNanos = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - iter->second.getCurrentTS());
		iter->second.input(diffNanos.count());
	}
}



std::string histogram::toString() const
{
	std::ostringstream stringStream;

	for (size_t i = 0; i < numBuckets.size(); ++i)
		stringStream << numBuckets[i] << std::endl;

	stringStream << m_declaration.label << std::endl
		<< "num_buckets: " << numBuckets.size()
		<< " min_sample: " << m_minSample
		<< " max_sample: " << m_maxSample
		<< " overflows: " << m_overfows << std::endl
		<< "ns per bucket: " << m_declaration.samplesPerBucket
		<< " average: " << average()
		<< " median: " << median() << std::endl;

	return stringStream.str();
}


}