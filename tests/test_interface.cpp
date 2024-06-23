
#include "profilerApi.h"

#include <random>
#include <fstream>
#include <string.h>
#include <chrono>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>

void wasteTime(size_t cnt)
{
	for (size_t i = 0; i < cnt; ++i)
	{
		volatile double s = std::sqrt(i + 1024 * 1024);
		s = s * s;
		s = std::sqrt(s);
	}
}

int testMacros()
{
	ThreadLocalTimeHist(basic, 1000, 500, "basic test of macros");

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<> dist{ 2000, 100 };

	for (size_t i = 0; i < 1024 * 1024; i++)
	{
		size_t timeToWaist = static_cast<size_t>(std::round(dist(gen)));
	
		TimeHistBegin(basic);
		wasteTime(timeToWaist);
		TimeHistEnd(basic);
		std::this_thread::sleep_for(std::chrono::milliseconds{10});
	}

	return 0;
}

int testThreadComm()
{
	struct Node
	{
		int _id;
		std::chrono::time_point<std::chrono::system_clock> _sentTP;
	};
	std::queue<Node> queue;
	std::mutex mtx;
	std::condition_variable cv;

	std::atomic<bool> end{false};

	auto producer{[&queue, &mtx, &cv, &end](){
		int cnt{0};
		while(!end.load(std::memory_order_acquire))
		{
			{
				std::unique_lock<std::mutex> l{mtx};
				queue.push(Node{cnt++, std::chrono::system_clock::now()});
			}
			cv.notify_all();
			std::this_thread::sleep_for(std::chrono::milliseconds{1});
		}

		std::cout << "send: " << cnt << " nodes" << std::endl;
	}};

	auto consumer{[&queue, &mtx, &cv, &end](){
		ThreadLocalTimeHist(threadComm, 1000, 300, "passing messages between threads");	
		
		int cnt{0};
		while(!end.load(std::memory_order_acquire))
		{
			std::unique_lock<std::mutex> l(mtx);
			cv.wait(l, [&end, &queue]{ return end.load(std::memory_order_acquire) || !queue.empty(); });
			
			if (!queue.empty())
			{
				auto& node{queue.front()};
				TimeHistSample(threadComm, node._sentTP, std::chrono::system_clock::now());
				queue.pop();
				cnt++;
			}
		}

		std::cout << "recv: " << cnt << " nodes, queue.size: " << queue.size() << std::endl;
	}};

	std::vector<std::thread> threads;
	threads.emplace_back(consumer);
	threads.emplace_back(producer);

	std::this_thread::sleep_for(std::chrono::seconds{60});

	end.store(true, std::memory_order_acquire);
	{
		std::unique_lock<std::mutex> l{mtx};
		queue.push(Node{0, std::chrono::system_clock::now()}); // release the consumer
	}
	cv.notify_all();

	for(auto& t : threads)
	{
		t.join();
	}

	return 0;
}

int testMacrosMiltipleThreads()
{
	std::cout << "basic test of macros with threads" << std::endl;
	auto func{[](double stdev){
		std::random_device rd{};
		std::mt19937 gen{ rd() };
		std::normal_distribution<> dist{ 2000 + (stdev * 200), 100 * stdev };

		for (size_t i = 0; i < 1024 * 1024; i++)
		{
			size_t timeToWaist = static_cast<size_t>(std::round(dist(gen)));

			ThreadLocalTimeHist(basicThreads, 1000, 200, "basic test of macros with threads");

			TimeHistBegin(basicThreads);
			wasteTime(timeToWaist);
			TimeHistEnd(basicThreads);
			//std::this_thread::sleep_for(std::chrono::milliseconds{1});
		}
	}};
	
	std::vector<std::thread> threads;
	for (size_t i = 0 ; i < 4 ; ++i)
	{
		threads.emplace(threads.end(), func, i);
	}
	std::this_thread::sleep_for(std::chrono::seconds{1});
	for(auto& t : threads)
	{
		if(t.joinable())
			t.join();
	}
	
	return 0;
}

void testRateCnt()
{
	ThreadLocalRateCnt (rateEvents,
						1'000'000'000 /* events / second*/, 
						100, /* size of the array of last rates to keep*/ 
						"basic test of rate counter");

	size_t repeat{100000000};
	while(repeat-- != 0)
	{
		wasteTime(2000);
		RateCntSample(rateEvents, 1);
	}
}

void testHistAsNumbers()
{
	ThreadLocalTimeHist(histNum, 1, 100, "test hist with numbers");

	std::random_device rd{};
	std::mt19937 gen{ rd() };
	std::normal_distribution<double> dist{ 40, 10 };

	for (size_t i = 0; i < 1024 * 1024; i++)
	{
		SampleHist(histNum, std::round(dist(gen)));
	}
}

int main(int /*argc*/, char* /*argv*/[])
{
	std::vector<std::thread> threads;
	//testRateCnt();
	//testMacros();
	
	threads.emplace_back(testMacros);
	threads.emplace_back(testThreadComm);
	threads.emplace_back(testHistAsNumbers);
	threads.emplace_back(testRateCnt);
	
	for(auto& t : threads)
	{
		if(t.joinable())
			t.join();
	}

	return 0;
}