cmake_minimum_required(VERSION 3.10)

set (PROJECT_NAME "profiler")
set (EXE_NAME "profiledApp")

# set the project name
project(${PROJECT_NAME})

set (SOURCES main.cpp profiler.h profiler.cpp)

set(ENABLE_HIST_PROFILER "true" CACHE BOOL "enables the profiler")
if(ENABLE_HIST_PROFILER)
	add_compile_definitions(ENABLE_HIST_PROFILER)
endif()

# add the executable
add_executable(${EXE_NAME} ${SOURCES})

# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#enable_testing()
#add_test(NAME main COMMAND main)

#add_subdirectory (ctest)
