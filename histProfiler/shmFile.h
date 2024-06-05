#pragma once

#include "utils.h"

#include <filesystem>
#include <memory>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>
#include <iostream>

namespace profiler {


/*
	keeps header and array for data in shared memory backup on a file
	0							4096
	+----------------+----------+-----------------------------------+
	| header		 | 			| data []							|
	+----------------+----------+-----------------------------------+
*/
template <typename HeaderType, typename DataType>
class shmFile final
{
public:
	shmFile() = default;
	shmFile(std::filesystem::path filename, HeaderType&& header, size_t dataSize);
	shmFile(shmFile&&) = default;
	shmFile& operator=(shmFile&&) = default;
	shmFile(shmFile&) = delete;
	shmFile& operator=(shmFile&) = delete;
	~shmFile();

    operator bool() const { return totalSize() > 0; }
    
    HeaderType& header() {return *reinterpret_cast<HeaderType*>(_headerAddr);}
    const HeaderType& header() const {return *reinterpret_cast<HeaderType*>(_headerAddr);}

    DataType* data() { return reinterpret_cast<DataType*>(_dataAddr);}
    const DataType* data() const { return reinterpret_cast<DataType*>(_dataAddr);}
    const DataType* endData() const { return reinterpret_cast<DataType*>(_endDataAddr);}
	size_t totalSize() const { return _endDataAddr - _headerAddr;}

	template<typename T>
	T dataAs()
	{
		static_assert(std::is_pointer<T>());
		return reinterpret_cast<T>(_dataAddr);
	}

	void sync();

	std::filesystem::path _filename;
	uint8_t* _headerAddr{nullptr};
    uint8_t* _dataAddr{nullptr};
	uint8_t* _endDataAddr{nullptr};
};

template <typename HeaderType, typename DataType>
void shmFile<HeaderType, DataType>::sync()
{
	auto rc{msync(_headerAddr, _endDataAddr - _headerAddr, MS_ASYNC)};
	if (0 != rc)
	{
		std::cerr << "failed to msync, err: " << rc << std::endl;
	}
}

template <typename HeaderType, typename DataType>
shmFile<HeaderType, DataType>::shmFile(std::filesystem::path filename, HeaderType&& hdr, size_t dataSize)
    :_filename{std::move(filename)}
{
	struct RAII final
	{
		int _fd{ -1 };
		~RAII() { if (_fd != -1) { close(_fd); } }
	};
	RAII raii;
    raii._fd = ::open(_filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (-1 == raii._fd)
	{
		const auto err{ errno };
		Throw(std::runtime_error) << " FAILED to open " << _filename 
								  << ", errno: " << err << End;
	}

    constexpr size_t pageSize{4096};
    constexpr auto headerSizeBytes{pageSize * ((sizeof(hdr) / pageSize) + 1)};
    const auto dataSizeBytes{dataSize * sizeof(DataType)};
    const auto totalSize{headerSizeBytes + dataSizeBytes};

	::lseek(raii._fd, totalSize - 1, SEEK_SET);
	::write(raii._fd, "0", 1);

	auto* beginAddr{mmap(nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, raii._fd, 0)};
	if (beginAddr == reinterpret_cast<void*>(-1))
	{
		const auto err{ errno };
		Throw(std::runtime_error) << " FAILED to mmap " << _filename
								  << ", size: " << totalSize << ", errno: " << err << End;
	}

    _headerAddr = reinterpret_cast<uint8_t*>(beginAddr);
    _dataAddr = _headerAddr + headerSizeBytes;
    _endDataAddr = _dataAddr + dataSizeBytes;

    header() = std::move(hdr);
    memset(dataAs<void*>(), 0, dataSizeBytes);

    std::cout << "Success to create shmFile: " << *this << std::endl;
}

template <typename HeaderType, typename DataType>
shmFile<HeaderType, DataType>::~shmFile()
{
	if (*this)
	{
    	if (-1 == msync(_headerAddr, totalSize(), MS_SYNC))
		{
			const auto err{ errno };
			std::cerr << __FILE__ << ':' << __LINE__
				<< " FAILED to msync " << *this << ", errno: " << err << std::endl;
		}
		if (-1 == munmap(_headerAddr, totalSize()))
		{
			const auto err{ errno };
			std::cerr << __FILE__ << ':' << __LINE__
				<< " FAILED to munmap " << *this << ", errno: " << err << std::endl;
		}
	}
}

template <typename HeaderType, typename DataType>
std::ostream& operator<<(std::ostream& stream, const shmFile<HeaderType, DataType>& obj)
{
	stream << "filename: " << obj._filename
		<< ", address: " << static_cast<void*>(obj._headerAddr) << ", end address: " << static_cast<void*>(obj._endDataAddr)
        << ", size: " << obj.totalSize();
	return stream;
}

}