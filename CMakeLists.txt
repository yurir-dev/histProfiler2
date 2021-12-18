cmake_minimum_required(VERSION 3.10)

set (PROJECT_NAME "profiler")
set (EXE_NAME "profiledApp")

# set the project name
project(${PROJECT_NAME})

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set (SOURCES main.cpp common.h profiler.h profiler.cpp)

set(ENABLE_HIST_PROFILER "true" CACHE BOOL "enables the profiler")
if(ENABLE_HIST_PROFILER)
	add_compile_definitions(ENABLE_HIST_PROFILER)
endif()

# add the executable
add_executable(${EXE_NAME} ${SOURCES})

if (UNIX)
target_link_libraries(${EXE_NAME} pthread)
endif()

enable_testing()
add_subdirectory(tests)

