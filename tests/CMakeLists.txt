cmake_minimum_required(VERSION 3.10)

include_directories(${CMAKE_SOURCE_DIR} ../ )

# Files common to all tests
set (COMMON_SOURCES ../common.h ../profiler.h ../profiler.cpp)

# Library setup/shutdown testing
set(TEST_MILLIS test_milliseconds)
add_executable(${TEST_MILLIS} test_millis.cpp ${COMMON_SOURCES})

set(TEST_MICROS test_microseconds)
add_executable(${TEST_MICROS} test_micros.cpp ${COMMON_SOURCES})


if (UNIX)
target_link_libraries(${TEST_MILLIS} pthread)
target_link_libraries(${TEST_MICROS} pthread)
endif()

add_test(${TEST_MILLIS} ${TEST_MILLIS})
add_test(${TEST_MICROS} ${TEST_MICROS})
