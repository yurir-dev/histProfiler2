#include "profiler.h"

#include <unordered_map>
#include <chrono>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>

namespace profiler
{

struct context;

struct histogram
{
	friend context;
public:
	histogram() = default;
	histogram(const profiler::declaration& d)
	{
		m_declaration = d;
		m_buckets.resize(d.numBuckets + 1);
		for (size_t i = 0; i < m_buckets.size(); ++i)
			m_buckets[i] = 0;
	}

	void setCurrentTS(std::chrono::time_point<std::chrono::system_clock> ts) { m_ts = ts; }
	std::chrono::time_point<std::chrono::system_clock> getCurrentTS() const { return m_ts; }

	void input(const uint64_t s)
	{
		if (s > m_maxSample) m_maxSample = s;
		if (s < m_minSample) m_minSample = s;

		m_sum += s;
		++m_numSamples;

		const size_t bucket = (size_t)std::round((double)s / (double)m_declaration.samplesPerBucket);
		//std::cout << "s: " << s << " bucket: " << bucket << std::endl;
		if (bucket < m_buckets.size())
			++m_buckets[bucket];
		else
		{
			++m_overfows;

			// it this overflow to the last special bucket, it will preserve it's weight on the average.
			// if twice greater than buckets then will increment by 2
			m_buckets[m_buckets.size() - 1] += (uint64_t)std::round((double)bucket / (double)m_buckets.size());
			//std::cout << "s: " << s << " val: " << (uint64_t)std::round((double)bucket / (double)m_buckets.size()) << std::endl;
		}
	}

	std::string toString(bool header = true, bool data = true) const;

	double mean()const { return m_numSamples > 0 ? (double)m_sum / (double)m_numSamples : 0.0; }
	double std()const
	{
		if (m_numSamples <= 0 || m_declaration.samplesPerBucket == 0)
			return 0.0;

		double m = mean() / m_declaration.samplesPerBucket;
		double sum{ 0 };
		for (size_t i = 0; i < m_buckets.size(); ++i)
		{
			double d = ((double)i - m);
			d *= d;
			sum += d * m_buckets[i];
		}

		return std::sqrt(sum / m_numSamples);
	}
	uint64_t median()const
	{
		uint64_t halfSamples{ m_numSamples / 2 };
		uint64_t sum{ 0 };
		for (size_t i = 0; i < m_buckets.size(); ++i)
		{
			sum += m_buckets[i];
			if (sum >= halfSamples)
				return i;
		}
		return 0;
	}

private:
	std::vector<uint64_t> m_buckets;
	profiler::declaration m_declaration;
	uint64_t m_maxSample{ 0 };
	uint64_t m_minSample{ 0xfffffffffffffffLL };
	uint64_t m_overfows{ 0 };
	uint64_t m_sum{ 0 };
	uint64_t m_numSamples{ 0 };

	std::chrono::time_point<std::chrono::system_clock> m_ts;

};

std::string histogram::toString(bool header, bool data) const
{
	std::ostringstream stringStream;

	if (header)
	{
		const auto average = mean();
		const auto deviation = std();

		stringStream << m_declaration.label
			<< " #buckets: " << m_buckets.size()
			<< ", #samples: " << m_numSamples
			<< ", #overflows: " << m_overfows
			<< ", ns/bucket: " << m_declaration.samplesPerBucket
			<< ", mean: " << average / m_declaration.samplesPerBucket << ",(" << average << " ns)"
			<< ", std: " << deviation << ", median: " << median()
			<< ", min: " << m_minSample << "ns, max: " << m_maxSample << "ns";
	}

	if (data)
	{
		stringStream << std::endl;
		std::cout << 0 << std::endl;
		for (size_t i = 0; i < m_buckets.size(); ++i)
			stringStream << m_buckets[i] << std::endl;
	}

	return stringStream.str();
}






struct context
{
	std::unordered_map<std::string, histogram> histograms;

	void toXml(std::ostream& out)const
	{
		std::ostringstream stringStream;

		for (auto iter = histograms.cbegin(); iter != histograms.end(); ++iter)
		{
			out << iter->second.toString(true, false) << '\t';
		}
		out << std::endl;

		//for (auto iter = ctx.histograms.cbegin(); iter != ctx.histograms.end(); ++iter)
		//	out << 0 << '\t';
		//out << std::endl;

		bool finished{ false };
		for (size_t i = 0; !finished; ++i)
		{
			bool moreBuckets{ false };
			for (auto iter = histograms.cbegin(); iter != histograms.end(); ++iter)
			{
				if (i < iter->second.m_buckets.size())
				{
					moreBuckets = true;
					out << iter->second.m_buckets[i] << '\t';
				}
			}
			out << std::endl;

			if (!moreBuckets)
				finished = true;
		}
	}
	void toStd(std::ostream& out)const
	{
		for (auto iter = histograms.cbegin(); iter != histograms.end(); ++iter)
			out << iter->second.toString() << std::endl;
	}
};
context ctx;


void init(const std::vector<declaration>& declarations)
{
	ctx.histograms.reserve(declarations.size());
	for (auto& d : declarations)
		ctx.histograms[d.label] = histogram(d);
}

void getData(std::ostream& out, outFormat f)
{
	switch (f)
	{
	case outFormat::excel:
		ctx.toXml(out);
		break;
	case outFormat::follow:
		ctx.toStd(out);
		break;
	default:
		std::cerr << "invalid format: " << static_cast<int>(f) << " , dump as std" << std::endl;
		ctx.toStd(out);
		break;
	}
}

void begin(const std::string& label)
{
	auto iter = ctx.histograms.find(label);
	if (iter != ctx.histograms.end())
		iter->second.setCurrentTS(std::chrono::system_clock::now());
}
void end(const std::string& label)
{
	const auto now = std::chrono::system_clock::now();

	const auto iter = ctx.histograms.find(label);
	if (iter != ctx.histograms.end())
	{
		const auto diffNanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now - iter->second.getCurrentTS());
		iter->second.input(diffNanos.count());
	}
}

}; // namespace profiler