
#include "profiler.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cmath>
#include <tuple>

#include "histogram.h"
#include "shmManager.h"
#include "utils.h"

namespace profiler {

struct histWrapper
{
    histWrapper() = default;
    histWrapper(histogram* header, uint64_t* buckets): _header{header}, _buckets{buckets} {}
    histWrapper(histWrapper&& other) : _header{other._header}, _buckets{other._buckets}, _ts{std::move(other._ts)}
    {
        other._header = nullptr;
    }
    histWrapper& operator=(histWrapper&& other)
    {
        if (this != &other)
        {
            _header = other._header;
            other._header = nullptr;
            _buckets = other._buckets;
            other._buckets = nullptr;
            _ts = std::move(other._ts);
        }
        return *this;
    }
    histWrapper(histWrapper&) = delete;
    histWrapper& operator=(const histWrapper&) = delete;

	void setCurrentTS(std::chrono::time_point<std::chrono::system_clock> ts) { _ts = ts; }
	std::chrono::time_point<std::chrono::system_clock> getCurrentTS() const { return _ts; }

    void input(uint64_t s)
	{
		if (s > _header->_maxSample) _header->_maxSample = s;
		if (s < _header->_minSample) _header->_minSample = s;

		_header->_sum += s;
		++_header->_numSamples;

        auto divRes{static_cast<double>(s) / static_cast<double>(_header->_samplesPerBucket)};
		auto bucketInd{static_cast<size_t>(std::round(divRes))};
		if (bucketInd < _header->_numBuckets - 1)
			++_buckets[bucketInd];
		else
		{
			++_header->_overfows;
			++_buckets[_header->_numBuckets - 1];
		}
	}

    double mean()const
	{
        if (_header->_numSamples == 0)
			return 0.0;
        return static_cast<double>(_header->_sum) / static_cast<double>(_header->_numSamples);
	}
	double std()const
	{
		if (_header->_numSamples <= 0 || _header->_samplesPerBucket == 0)
			return 0.0;

		double average{mean() / static_cast<double>(_header->_samplesPerBucket)};
		double sum{ 0.0 };
		for (size_t i = 0; i < _header->_numBuckets; ++i)
		{
			double val{static_cast<double>(i) - average};
			val *= val;
			sum += val * _buckets[i];
		}

		return std::sqrt(sum / _header->_numSamples);
	}
	uint64_t median()const
	{
		const uint64_t halfSamples{ _header->_numSamples / 2 };
		uint64_t sum{ 0 };
		for (size_t i = 0; i < _header->_numBuckets; ++i)
		{
			sum += _buckets[i];
			if (sum >= halfSamples)
				return i;
		}
		return 0;
	}

	histogram* _header{nullptr};
    uint64_t* _buckets{nullptr};
	std::chrono::time_point<std::chrono::system_clock> _ts;
};

struct context::impl
{
    impl(const std::string& filename, const std::vector<declaration>& declarations)
    {
        size_t size{0};
	    for (auto& d : declarations)
	    {
	    	size += sizeof(histogram) + alignof(histogram);
	    	size += d.numBuckets * sizeof(uint64_t) + alignof(uint64_t);
	    }

	    _shmManager = shmManager{filename, size};

	    _histograms.resize(declarations.size());
	    histogram* header;
        uint64_t* buckets;
        for (auto& d : declarations)
	    {
	    	std::tie(header, buckets) = _shmManager.allocateHist(d.numBuckets, d.samplesPerBucket);
	    	_histograms[d.label] = histWrapper{header, buckets};
	    }
    }

    void beginImpl(const labelIndex& label, const std::thread::id& /*tid*/)
    {
        if (label < _histograms.size())
        {
            _histograms[label].setCurrentTS(std::chrono::system_clock::now());
        }
	    else
        {
	    	Throw(std::runtime_error) << " Label index : " << label 
                                      << " is greater then the expected: " << _histograms.size()
                                        << End;
        }
    }
	void endImpl(const labelIndex& label, const std::thread::id& /*tid*/)
    {
        if (label < _histograms.size())
        {
            auto& hist{_histograms[label]};
            const auto diffNanos{std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - hist.getCurrentTS())};
            hist.input(diffNanos.count());
        }
        else
        {
	    	Throw(std::runtime_error) << " Label index : " << label 
                                      << " is greater then the expected: " << _histograms.size()
                                        << End;
        }
    }
	void toXml(std::ostream& out)const
    {
        out << " not implemented" << std::endl;
		Throw(std::runtime_error) << " not implemented" << End;
    }
	void toStd(std::ostream& out)const
    {
        out << " not implemented" << std::endl;
		Throw(std::runtime_error) << " not implemented" << End;
    }

    shmManager _shmManager;
	std::vector<histWrapper> _histograms;
	std::mutex mtx;
};

context::context() = default;
context::context(const std::string& filename, const std::vector<declaration>& declarations)
: _impl{std::make_unique<context::impl>(filename, declarations)}
{}
context::context(context&&) = default;
context& context::operator=(context&&) = default;
context::~context() = default;


void context::begin(const labelIndex& label)
{
	_impl->beginImpl(label, std::thread::id());
}

void context::end(const labelIndex& label)
{
	_impl->endImpl(label, std::thread::id());
}


	void context::beginTid(const labelIndex& label)
	{
		Throw(std::runtime_error) << " not implemented: " << label << End;
		//std::thread::id tid = std::this_thread::get_id();
//
		//static thread_local std::once_flag f;
		//std::call_once(f, [this, &label, &tid]() {
		//	auto iter = histograms.find(std::tuple(std::thread::id(), label));
		//	if (iter != histograms.end())
		//	{
		//		auto pos{ histograms.end() };
		//		bool inserted;
		//		
		//		std::lock_guard<std::mutex> l{ mtx };
		//		std::tie(pos, inserted) = histograms.insert_or_assign(Key(tid, label), iter->second);
//
		//		auto vec_iter = std::find_if(ordered_hists.begin(), ordered_hists.end(), [&iter](auto& v)
		//			{
		//				return v.end() != std::find_if(v.begin(), v.end(), [&iter](auto& it) { return it == iter; });
		//			});
		//		if (vec_iter != ordered_hists.end())
		//		{
		//			vec_iter->push_back(pos);
//
		//		}
		//	}
		//	});
//
		//beginImpl(label, tid);
	}

	void context::endTid(const labelIndex& label)
	{
		Throw(std::runtime_error) << " not implemented: " << label << End;
		//endImpl(label, std::this_thread::get_id());
	}
}