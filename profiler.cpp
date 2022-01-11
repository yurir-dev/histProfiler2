#include "profiler.h"

#include <unordered_map>
#include <chrono>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <thread>
#include <mutex>
#include <tuple>
#include <algorithm>

namespace profiler
{

struct impl;

struct histogram
{
	friend impl;
public:
	histogram() = default;
	histogram(const context::declaration& d)
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

		size_t bucket = (size_t)std::round((double)s / (double)m_declaration.samplesPerBucket);
		if (m_declaration.shift > 0)
		{
			if (bucket < m_declaration.shift)
			{
				++m_underflows;
				return;
			}
			else
				bucket -= m_declaration.shift;
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
	std::ostream& dump(uint32_t mask, std::ostream& out) const;
	std::string toString(uint32_t mask) const
	{
		std::ostringstream stringStream;
		dump(mask, stringStream);
		return stringStream.str();
	}

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
	context::declaration m_declaration;
	uint64_t m_maxSample{ 0 };
	uint64_t m_minSample{ 0xfffffffffffffffLL };
	uint64_t m_overfows{ 0 };
	uint64_t m_underflows{0};
	uint64_t m_sum{ 0 };
	uint64_t m_numSamples{ 0 };

	std::chrono::time_point<std::chrono::system_clock> m_ts;

};

std::ostream& histogram::dump(uint32_t mask, std::ostream& out) const
{
	if (mask & toString_t::label)
	{
		out << m_declaration.label;
	}

	if (mask & toString_t::header)
	{
		const auto average = mean();
		const auto deviation = std();

		out << ", #buckets: " << m_buckets.size()
			<< ", #samples: " << m_numSamples;

		if (m_declaration.shift > 0)
			out << ", shift: " << m_declaration.shift;

		out << ", #overflows: " << m_overfows << ", #underflows: " << m_underflows
			<< ", ns/bucket: " << m_declaration.samplesPerBucket
			<< ", mean: " << average / m_declaration.samplesPerBucket << "(" << average << " ns)"
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







struct impl: public context::implBase
{
	using Key = std::tuple<std::thread::id, const std::string>;
	struct hash
	{
		std::size_t operator()(const Key& k) const
		{
			return std::hash<std::thread::id>{}(std::get<0>(k)) ^ std::hash<std::string>{}(std::get<1>(k));
		}
	};

	using HistIterator_t = std::unordered_map<Key, histogram, hash>::iterator;

	std::unordered_map<Key, histogram, hash> histograms;
	std::vector<std::vector<HistIterator_t>> ordered_hists;
	std::mutex mtx;

	void toXml(std::ostream& out)const
	{
		std::ostringstream stringStream;

		for (auto& v : ordered_hists)
		{
			for (auto& iter : v)
			{
				iter->second.dump(histogram::toString_t::header | histogram::toString_t::label, out) << '\t';
			}
		}
		out << std::endl;

		for (size_t i = 0; true; ++i)
		{
			bool moreBuckets{ false };
			for (auto& v : ordered_hists)
			{
				for (auto& iter : v)
				{
					if (i < iter->second.m_buckets.size())
					{
						moreBuckets = true;
						out << iter->second.m_buckets[i];
					}
					out << '\t';

				}
			}
			out << std::endl;

			if (!moreBuckets)
				break;
		}
	}
	void toStd(std::ostream& out)const
	{
		for (auto iter = histograms.cbegin(); iter != histograms.end(); ++iter)
			out << iter->second.toString(histogram::toString_t::all) << std::endl;
	}

	void clear()
	{
		ordered_hists.clear();
		histograms.clear();
	}
	void init(const std::vector<context::declaration>& declarations);
};



context::context()
{
	m_impl = std::make_shared<impl>();
}
void context::init(const std::vector<declaration>& declarations)
{
	auto ctxImpl = std::reinterpret_pointer_cast<impl>(m_impl);
	ctxImpl->clear();

	ctxImpl->histograms.reserve(declarations.size());
	ctxImpl->ordered_hists.reserve(declarations.size());

	for (auto& d : declarations)
	{
		auto pos = ctxImpl->histograms.end();
		bool inserted;
		std::tie(pos, inserted) = ctxImpl->histograms.insert_or_assign(impl::Key(std::thread::id(), d.label), histogram(d));

		std::vector<impl::HistIterator_t> temp;
		temp.push_back(pos);
		ctxImpl->ordered_hists.push_back(std::move(temp));
	}
}


void context::getData(std::ostream& out, outFormat f)
{
	auto ctxImpl = std::reinterpret_pointer_cast<impl>(m_impl);
	switch (f)
	{
	case outFormat::excel:
		ctxImpl->toXml(out);
		break;
	case outFormat::follow:
		ctxImpl->toStd(out);
		break;
	default:
		std::cerr << "invalid format: " << static_cast<int>(f) << " , dump as std" << std::endl;
		ctxImpl->toStd(out);
		break;
	}
}
void context::beginImpl(const std::string& label, const std::thread::id& tid)
{
	auto ctxImpl = std::reinterpret_pointer_cast<impl>(m_impl);
	auto iter = ctxImpl->histograms.find(std::tuple(tid, label));
	if (iter != ctxImpl->histograms.end())
		iter->second.setCurrentTS(std::chrono::system_clock::now());
}
void context::endImpl(const std::string& label, const std::thread::id& tid)
{
	const auto now = std::chrono::system_clock::now();

	auto ctxImpl = std::reinterpret_pointer_cast<impl>(m_impl);
	const auto iter = ctxImpl->histograms.find(impl::Key(tid, label));
	if (iter != ctxImpl->histograms.end())
	{
		const auto diffNanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now - iter->second.getCurrentTS());
		iter->second.input(diffNanos.count());
	}
}

void context::begin(const std::string& label)
{
	beginImpl(label, std::thread::id());
}
void context::end(const std::string& label)
{
	endImpl(label, std::thread::id());
}
void context::beginTid(const std::string& label)
{
	std::thread::id tid = std::this_thread::get_id();

	static thread_local std::once_flag f;
	std::call_once(f, [this, &label, &tid]() {
		auto ctxImpl = std::reinterpret_pointer_cast<impl>(m_impl);
		auto iter = ctxImpl->histograms.find(std::tuple(std::thread::id(), label));
		if (iter != ctxImpl->histograms.end())
		{
			auto pos{ ctxImpl->histograms.end() };
			bool inserted;

			std::lock_guard<std::mutex> l{ ctxImpl->mtx };
			std::tie(pos, inserted) = ctxImpl->histograms.insert_or_assign(impl::Key(tid, label), iter->second);
			
			auto vec_iter = std::find_if(ctxImpl->ordered_hists.begin(), ctxImpl->ordered_hists.end(), [&iter](auto& v)
				{
					return v.end() != std::find_if(v.begin(), v.end(), [&iter](auto& it) { return it == iter; });
				});
			if (vec_iter != ctxImpl->ordered_hists.end())
			{
				vec_iter->push_back(pos);

			}
		}
	});
	
	beginImpl(label, tid);
}
void context::endTid(const std::string& label)
{
	endImpl(label, std::this_thread::get_id());
}

}; // namespace profiler