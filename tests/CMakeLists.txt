cmake_minimum_required(VERSION 3.10)

include_directories(${CMAKE_SOURCE_DIR} ../ )

# Files common to all tests
set (COMMON_SOURCES ../common.h ../profiler.h ../profiler.cpp histVerificator.h histVerificator.cpp)

# Library setup/shutdown testing
set(TEST_MILLIS test_milliseconds)
add_executable(${TEST_MILLIS} test_millis.cpp ${COMMON_SOURCES})

set(TEST_MICROS test_microseconds)
add_executable(${TEST_MICROS} test_micros.cpp ${COMMON_SOURCES})

set(TEST_THREADS test_threads)
add_executable(${TEST_THREADS} test_threads.cpp ${COMMON_SOURCES})

set(exes ${TEST_THREADS} ${TEST_MICROS} ${TEST_MILLIS})

if (UNIX)
foreach (exe IN LISTS exes)
	target_link_libraries(${exe} pthread)
endforeach()
endif()

foreach (exe IN LISTS exes)
	add_test(${exe} ${exe})
endforeach()

