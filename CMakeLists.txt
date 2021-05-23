cmake_minimum_required(VERSION 2.6)
project(vnx)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -fopenmp -Wall")

include_directories(
	include
)

add_executable(test_disk_sort test/test_disk_sort.cpp)
