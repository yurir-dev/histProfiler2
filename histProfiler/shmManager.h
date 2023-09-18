#pragma once

#include <memory>
#include <string>

namespace profiler
{
	class histogram;

	class shmManager final
	{
	public:
		shmManager();
		shmManager(const std::string& filename, size_t size);
		shmManager(shmManager&&);
		shmManager& operator=(shmManager&&);
		~shmManager();

		// returns header* and buckets*
		std::pair<histogram*, uint64_t*> allocateHist(uint64_t numBuckets, uint64_t nanosPerBucket);
		//bool getHist(shmHistogram& hist, void&* dataPtr, size_t dataSize);
/*
		struct iterator
		{
			iterator()
		};
*/
	private:
		shmManager(shmManager&) = delete;
		shmManager& operator=(shmManager&) = delete;	

		struct impl;
		friend std::ostream& operator<<(std::ostream& stream, const shmManager::impl& obj);
		std::unique_ptr<impl> _impl;
	};
}