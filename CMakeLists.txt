cmake_minimum_required(VERSION 3.10)

set (PROJECT_NAME "profiler")
set (EXE_NAME "profiledApp")

project(${PROJECT_NAME})

include_directories(${CMAKE_SOURCE_DIR} histProfiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set (HIST_PROFILER histProfiler/profilerApi.h histProfiler/profiler.h histProfiler/histogram.h)
set (SOURCES main.cpp common.h ${HIST_PROFILER})

set(ENABLE_HIST_PROFILER "true" CACHE BOOL "enables the profiler")
if(ENABLE_HIST_PROFILER)
	add_compile_definitions(ENABLE_HIST_PROFILER)
endif()

add_executable(${EXE_NAME} ${SOURCES})

if (UNIX)
target_link_libraries(${EXE_NAME} pthread)
endif()

enable_testing()
add_subdirectory(tests)

