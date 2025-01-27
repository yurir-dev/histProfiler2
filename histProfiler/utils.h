#pragma once

#include <exception>
#include <sstream>

namespace profiler {

struct throwParam
{};

template <typename exception_t = std::runtime_error>
struct throwExceptionImpl final
{
    throwExceptionImpl(const std::string& file, int line)
    {
        _strm << file << ':' << line << ' ';
    }

    template<typename val_t>
    throwExceptionImpl& operator << (const val_t& v)
    {
        _strm << v;
        return *this;
    }
    throwExceptionImpl& operator<< (const throwParam&) noexcept(false)
    {
        throw exception_t{_strm.str()};
        return *this;
    }

    std::stringstream _strm;
};

#define Throw(type) throwExceptionImpl<type>{__FILE__, __LINE__}
#define End throwParam{}

}