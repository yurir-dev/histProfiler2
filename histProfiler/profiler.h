#pragma once

#include <memory>
#include <ostream>
#include <vector>

namespace profiler
{
	enum class outFormat { follow, excel };

	enum labelIndex
	{
		label_1 = 0,
		label_2 = 0,
		label_3 = 0,
		// ...
	};
	
	struct declaration
	{
		/* id for current histogram, should be used in begin/end functions*/
		labelIndex label;

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
	};

	class context final
	{
	public:
		context();
		context(const std::string& filename, const std::vector<declaration>& declarations);
		context(context&&);
		context& operator=(context&&);
		~context();

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


		/*
			dumps all the data to std::cout or a text file
			outFormat::follow : histogram after histogram.
			outFormat::excel : each histogram in a separate column, separated by a tab,
								copy&paste into excel, or just open the file from excel
		*/
		void getData(std::ostream& /*out*/, outFormat /*f*/){}

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
		void begin(const labelIndex& label);
		void end(const labelIndex& label);

		/*
			same as begin/end, can be used in different threads with the same labels.
			will create a separate histogram for each thread.
			t1					t2
			startTid(lbl1)		startTid(lbl1)
			...					...
			endTid(lbl1)		endTid(lbl1)
		*/
		void beginTid(const labelIndex& label);
		void endTid(const labelIndex& label);

	private:
		context(const context&) = delete;
		context& operator=(const context&) = delete;

		struct impl;
		std::unique_ptr<impl> _impl;
	};
}


