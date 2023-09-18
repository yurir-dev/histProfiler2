#include "shmManager.h"

#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>
#include <iostream>

#include "utils.h"
#include "histogram.h"

namespace profiler {

struct shmManager::impl
{
	impl() = default;
	impl(std::string filename, int fd, uint8_t* addr, size_t size)
		: _filename{std::move(filename)}, _fd {fd}, _addr{ addr }, 
		_endAddr{_addr + size}, _size{ size }
	{}
	impl(const impl&) = delete;
	impl& operator=(const impl&) = delete;
	impl(impl&& other)
		: _filename{ std::move(other._filename) }, _fd{other._fd}, 
		_addr{ other._addr }, _size{other._size}
	{
		other = impl{};
	}
	impl& operator=(impl&& other)
	{
		if (this != &other)
		{
			_filename = std::move(other._filename);
			_fd = other._fd;
			_addr = other._addr;
			_size = other._size;
			other = impl{};
		}
		return *this;
	}

	bool valid()const { return _fd != -1 && _size != 0 && _addr != nullptr; }

	template <typename T, typename U>
	static size_t ptrdif(T* a, U* b)
	{
		return reinterpret_cast<uint8_t*>(a) - reinterpret_cast<uint8_t*>(b);
	}
	template <typename T, typename U>
	static T* ptradd(T* a, U add)
	{
		return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(a) + add);
	}

	std::pair<histogram*, uint64_t*> initHist(uint64_t numBuckets, uint64_t nanosPerBucket)
	{
		void *p{_addr};
		size_t sz{ptrdif(_endAddr, _addr)};
		auto* resHist = std::align(alignof(histogram), sizeof(histogram), p, sz);
		if (resHist == nullptr)
		{
			Throw(std::runtime_error) << " FAILED to align histogram" << *this << End;
		}

		auto* bucketPtr{ptradd(resHist, sizeof(histogram))};
		sz = ptrdif(_endAddr, bucketPtr);
		bucketPtr = std::align(alignof(uint64_t), sizeof(uint64_t), bucketPtr, sz);
		if (bucketPtr == nullptr)
		{
			Throw(std::runtime_error) << " FAILED to align buckets" << *this << End;
		}

		auto* hist{new (resHist) histogram(nanosPerBucket, numBuckets)};
		
		// move to next location
		_addr = reinterpret_cast<uint8_t*>(ptradd(bucketPtr, hist->_numBuckets * sizeof(uint64_t)));

		return std::pair<histogram*, uint64_t*>{hist, reinterpret_cast<uint64_t*>(bucketPtr)};
	}

	std::string _filename;
	int _fd{ -1 };
	uint8_t* _addr{nullptr};
	uint8_t* _endAddr{nullptr};
	size_t _size{0};
};

std::ostream& operator<<(std::ostream& stream, const shmManager::impl& obj)
{
	stream << "filename: " << obj._filename << ", fd: " << obj._fd
		<< ", address: " << obj._addr << ", end address: " << obj._endAddr << ", size: " << obj._size;
	return stream;
}

shmManager::shmManager() = default;
shmManager::shmManager(shmManager&&) = default;
shmManager& shmManager::operator=(shmManager&&) = default;
shmManager::shmManager(const std::string& filename, size_t size)
{
	auto fd = ::open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (-1 == fd)
	{
		const auto err{ errno };
		Throw(std::runtime_error) << " FAILED to open " << filename 
								  << ", errno: " << err << End;
	}

	lseek(fd, size - 1, SEEK_SET);
	write(fd, "\n", 1);

	auto* addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == reinterpret_cast<void*>(-1))
	{
		const auto err{ errno };
		Throw(std::runtime_error) << " FAILED to open " << filename
								  << ", size: " << size << ", errno: " << err << End;
	}

	_impl = std::make_unique<shmManager::impl>(filename, fd, reinterpret_cast<uint8_t*>(addr), size);
}

shmManager::~shmManager()
{
	if (_impl)
	{
		if (_impl->valid())
		{
			if (-1 == msync(_impl->_addr, _impl->_size, MS_SYNC))
			{
				const auto err{ errno };
				std::cerr << __FILE__ << ':' << __LINE__
					<< " FAILED to msync " << *_impl << ", errno: " << err << std::endl;
			}
			if (-1 == munmap(_impl->_addr, _impl->_size))
			{
				const auto err{ errno };
				std::cerr << __FILE__ << ':' << __LINE__
					<< " FAILED to munmap " << *_impl << ", errno: " << err << std::endl;
			}
			if (-1 == close(_impl->_fd))
			{
				const auto err{ errno };
				std::cerr << __FILE__ << ':' << __LINE__
					<< " FAILED to close " << *_impl << ", errno: " << err << std::endl;
			}
		}
		else
		{
			assert(false && "impl is invalid");
		}
		_impl.reset();
	}
}

std::pair<histogram*, uint64_t*> shmManager::allocateHist(uint64_t numBuckets, uint64_t nanosPerBucket)
{
	if (!_impl)
	{
		Throw(std::runtime_error) << " shmManager is not ready " << End;
	}

	return _impl->initHist(numBuckets, nanosPerBucket);
}
//bool shmManager::getHist(shmHistogram& hist, void&* dataPtr, size_t dataSize)
//{
//	return false;
//}


}
