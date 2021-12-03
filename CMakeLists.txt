cmake_minimum_required(VERSION 3.10)

set (PROJECT_NAME "profiler")
set (EXE_NAME "profiledApp")

# set the project name
project(${PROJECT_NAME})

set (SOURCES main.cpp profiler.h profiler.cpp)

# add the executable
add_executable(${EXE_NAME} ${SOURCES})

# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
