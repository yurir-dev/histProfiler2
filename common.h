#pragma once

#include <chrono>
#include <thread>
#include <sstream>
#include <cmath>

struct busyMethods
{
	template< class Rep, class Period >
	static void getBusySleep(const std::chrono::duration<Rep, Period>& sleep_duration)
	{
		std::this_thread::sleep_for(sleep_duration);
	}
	template< class Rep, class Period >
	static void getBusySpin(const std::chrono::duration<Rep, Period>& sleep_duration)
	{
		const auto endTime = std::chrono::system_clock::now() + sleep_duration;
		while (auto now = std::chrono::system_clock::now() < endTime)
		{
		}
	}

	static void getBusySqrt(size_t cnt)
	{
		for (size_t i = 0; i < cnt; ++i)
		{
			volatile double s = std::sqrt(i + 1024 * 1024);
			s = s * s;
			//s = std::sqrt(i);
		}
	}
};

struct common
{
	template <typename T>
	static bool stoT(std::string str, T& res)
	{
		T n;
		std::stringstream ssLine{ str };
		ssLine >> n;
		if (ssLine.fail())
			return false;
		res = n;
		return true;
	}
};