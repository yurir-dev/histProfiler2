#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cmath>
#include <tuple>
#include <ostream>
#include "histogram.h"

namespace profiler
{
	enum class outFormat { follow, excel };

	template <typename Label_t = std::string>
	struct declaration
	{
		/* id for current histogram, should be used in begin/end functions*/
		Label_t label;

		/*
			it samples time in nanoseconds, every sample will be divided by samplesPerBucket
			so, if samplesPerBucket is 1, each bucket will contain 1 ns,
			if 10, each bucket will contain 10 ns,
			if 1'000, each bucket will contain 1000 ns = 1 micro,
			if 1'000'000, each bucket will contain 1'000'000 ns = 1 milli,
			if 1'000'000'000, each bucket will contain 1'000'000'000 ns = 1 second,
		*/
		size_t samplesPerBucket{ 1 };

		/*	number of buckets, overflows will be counted and their respective weight will be stored
			in an additional bucket allocated at the end.
		*/
		size_t numBuckets{ 1 };

		size_t shift{ 0 };
	};

	struct __defaultLabeltoString__
	{
		std::string operator()(const std::string& l) { return l; }
	};

	template <class Label_t = std::string, class Label2Str_t = __defaultLabeltoString__>
	class context final
	{
	public:
		using label_t = Label_t;

		context() = default;
		context(const context&) = delete;
		context& operator=(const context&) = delete;
		context(context&&) = delete;
		context& operator=(context&&) = delete;

		/*
			inits everything, can be called several times.
			examples:
			1) test in seconds
				init({
					{"normal distribution", 1'000'000'000, 10},
				});
			2) test milliseconds
				init({
					{"normal distribution", 1'000'000, 20},
					{"gamma distribution", 1'000'000, 20},
				});
			2) test microseconds
				init({
					{"normal distribution 1", 1'000, 100},
					{"normal distribution 2", 1'000, 100},
				});
			3) test in 10th of nanoseconds
				init({
					{"normal distribution", 10, 500},
				});
		*/
		void init(const std::vector<declaration<Label_t>>& declarations);


		/*
			dumps all the data to std::cout or a text file
			outFormat::follow : histogram after histogram.
			outFormat::excel : each histogram in a separate column, separated by a tab,
								copy&paste into excel, or just open the file from excel
		*/
		void getData(std::ostream& out, outFormat f);

		/*
			take current timestamp in nanos (std::chrono::system_clock::now() is used)
			end() calculates the difference and stores it in the assosiated histogram.
			if label was not declared in init(), will do nothing.

			example:
			profiler::init({
				{"test my code in millis", 1'000'000, 20},
			});

			begin("test my code in millis");
			...
			end("test my code in millis");
		*/
		void begin(const Label_t& label);
		void end(const Label_t& label);

		/*
			same as begin/end, can be used in different threads with the same labels.
			will create a separate histogram for each thread.
			t1					t2
			startTid(lbl1)		startTid(lbl1)
			...					...
			endTid(lbl1)		endTid(lbl1)
		*/
		void beginTid(const Label_t& label);
		void endTid(const Label_t& label);

	private:
		void beginImpl(const Label_t& label, const std::thread::id& tid);
		void endImpl(const Label_t& label, const std::thread::id& tid);
		void toXml(std::ostream& out)const;
		void toStd(std::ostream& out)const;

		using Key = std::tuple<std::thread::id, const Label_t>;
		struct hash
		{
			std::size_t operator()(const Key& k) const
			{
				return std::hash<std::thread::id>{}(std::get<0>(k)) ^ std::hash<Label_t>{}(std::get<1>(k));
			}
		};
		typedef std::unordered_map<Key, histogram, hash> HistMap_t;

		HistMap_t histograms;
		std::vector<std::vector<typename HistMap_t::iterator>> ordered_hists;
		std::mutex mtx;
	};



	template <typename Label_t, typename Label2Str_t>
	void context<Label_t, Label2Str_t>::init(const std::vector<declaration<Label_t>>& declarations)
	{
		histograms.reserve(declarations.size());
		ordered_hists.reserve(declarations.size());

		for (auto& d : declarations)
		{
			Label2Str_t convert;
			auto pos = histograms.end();
			bool inserted;
			std::tie(pos, inserted) = histograms.insert_or_assign(Key(std::thread::id(), d.label), 
				histogram{ convert(d.label), d.samplesPerBucket, d.numBuckets, d.shift });

			std::vector<typename HistMap_t::iterator> temp;
			temp.push_back(pos);
			ordered_hists.push_back(std::move(temp));
		}
	}

	template <typename Label_t, typename Label2Str_t>
	void context<Label_t, Label2Str_t>::getData(std::ostream& out, outFormat f)
	{
		switch (f)
		{
		case outFormat::excel:
			toXml(out);
			break;
		case outFormat::follow:
			toStd(out);
			break;
		default:
			std::cerr << "invalid format: " << static_cast<int>(f) << " , dump as std" << std::endl;
			toStd(out);
			break;
		}
	}

	template <typename Label_t, typename Label2Str_t>
	void context<Label_t, Label2Str_t>::beginImpl(const Label_t& label, const std::thread::id& tid)
	{
		auto iter = histograms.find(Key(tid, label));
		if (iter != histograms.end())
			iter->second.setCurrentTS(std::chrono::system_clock::now());
	}

	template <typename Label_t, typename Label2Str_t>
	void context<Label_t, Label2Str_t>::endImpl(const Label_t& label, const std::thread::id& tid)
	{
		const auto now = std::chrono::system_clock::now();

		const auto iter = histograms.find(Key(tid, label));
		if (iter != histograms.end())
		{
			const auto diffNanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now - iter->second.getCurrentTS());
			iter->second.input(diffNanos.count());
		}
	}

	template <typename Label_t, typename Label2Str_t>
	void context<Label_t, Label2Str_t>::begin(const Label_t& label)
	{
		beginImpl(label, std::thread::id());
	}

	template <typename Label_t, typename Label2Str_t>
	void context<Label_t, Label2Str_t>::end(const Label_t& label)
	{
		endImpl(label, std::thread::id());
	}

	template <typename Label_t, typename Label2Str_t>
	void context<Label_t, Label2Str_t>::beginTid(const Label_t& label)
	{
		std::thread::id tid = std::this_thread::get_id();

		static thread_local std::once_flag f;
		std::call_once(f, [this, &label, &tid]() {
			auto iter = histograms.find(std::tuple(std::thread::id(), label));
			if (iter != histograms.end())
			{
				auto pos{ histograms.end() };
				bool inserted;
				
				std::lock_guard<std::mutex> l{ mtx };
				std::tie(pos, inserted) = histograms.insert_or_assign(Key(tid, label), iter->second);

				auto vec_iter = std::find_if(ordered_hists.begin(), ordered_hists.end(), [&iter](auto& v)
					{
						return v.end() != std::find_if(v.begin(), v.end(), [&iter](auto& it) { return it == iter; });
					});
				if (vec_iter != ordered_hists.end())
				{
					vec_iter->push_back(pos);

				}
			}
			});

		beginImpl(label, tid);
	}

	template <typename Label_t, typename Label2Str_t>
	void context<Label_t, Label2Str_t>::endTid(const Label_t& label)
	{
		endImpl(label, std::this_thread::get_id());
	}

	template <typename Label_t, typename Label2Str_t>
	void context<Label_t, Label2Str_t>::toXml(std::ostream& out)const
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
	template <typename Label_t, typename Label2Str_t>
	void context<Label_t, Label2Str_t>::toStd(std::ostream & out)const
	{
		// TODO print ordered
		for (auto iter = histograms.cbegin(); iter != histograms.end(); ++iter)
			out << iter->second.toString(histogram::toString_t::all) << std::endl;
	}

};


