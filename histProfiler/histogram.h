#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

namespace profiler
{
	struct histogram
	{
	public:
		histogram() = default;
		histogram(const std::string& label, size_t samplesPerBucket, size_t numBuckets, size_t shift)
		{
			configure(label, samplesPerBucket, numBuckets, shift);
		}

		void configure(const std::string & label, size_t samplesPerBucket, size_t numBuckets, size_t shift)
		{
			m_label = label;
			m_samplesPerBucket = samplesPerBucket;
			m_numBuckets = numBuckets;
			m_shift = shift;
			m_buckets.resize(m_numBuckets);
			std::fill(m_buckets.begin(), m_buckets.end(), 0);
		}
		void clear()
		{
			m_label.clear();
			m_maxSample = m_minSample = m_overfows = m_underflows = m_sum = m_numSamples = 0;
			m_samplesPerBucket = m_numBuckets = m_shift = 0;
			m_buckets.resize(0);
		}

		void setCurrentTS(std::chrono::time_point<std::chrono::system_clock> ts) { m_ts = ts; }
		std::chrono::time_point<std::chrono::system_clock> getCurrentTS() const { return m_ts; }

		void input(const uint64_t s)
		{
			if (s > m_maxSample) m_maxSample = s;
			if (s < m_minSample) m_minSample = s;

			m_sum += s;
			++m_numSamples;

			size_t bucket = (size_t)std::round((double)s / (double)m_samplesPerBucket);
			if (m_shift > 0)
			{
				if (bucket < m_shift)
				{
					++m_underflows;
					return;
				}
				else
					bucket -= m_shift;
			}

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

		enum toString_t : uint32_t
		{
			label = 0x1,
			header = 0x2,
			data = 0x4,

			all = label | header | data
		};
		std::ostream& dump(uint32_t mask, std::ostream& out) const
		{
			if (mask & toString_t::label)
			{
				out << m_label;
			}

			if (mask & toString_t::header)
			{
				const auto average = mean();
				const auto deviation = std();

				out << ", #buckets: " << m_buckets.size()
					<< ", #samples: " << m_numSamples;

				if (m_shift > 0)
					out << ", shift: " << m_shift;

				out << ", #overflows: " << m_overfows << ", #underflows: " << m_underflows
					<< ", ns/bucket: " << m_samplesPerBucket
					<< ", mean: " << average / m_samplesPerBucket << "(" << average << " ns)"
					<< ", std: " << deviation << ", median: " << median()
					<< ", min: " << m_minSample << "ns, max: " << m_maxSample << "ns";
			}

			if (mask & toString_t::data)
			{
				out << std::endl;
				std::cout << 0 << std::endl;
				for (size_t i = 0; i < m_buckets.size(); ++i)
					out << m_buckets[i] << std::endl;
			}
			return out;
		}
		std::string toString(uint32_t mask) const
		{
			std::ostringstream stringStream;
			dump(mask, stringStream);
			return stringStream.str();
		}

		double mean()const { return m_numSamples > 0 ? (double)m_sum / (double)m_numSamples : 0.0; }
		double std()const
		{
			if (m_numSamples <= 0 || m_samplesPerBucket == 0)
				return 0.0;

			double m = mean() / m_samplesPerBucket;
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

		//private:
		std::vector<uint64_t> m_buckets;
		uint64_t m_maxSample{ 0 };
		uint64_t m_minSample{ 0xfffffffffffffffLL };
		uint64_t m_overfows{ 0 };
		uint64_t m_underflows{ 0 };
		uint64_t m_sum{ 0 };
		uint64_t m_numSamples{ 0 };
		size_t m_samplesPerBucket{0};
		size_t m_numBuckets{0};
		size_t m_shift{0};
		std::string m_label;

		std::chrono::time_point<std::chrono::system_clock> m_ts;
	};

	

}