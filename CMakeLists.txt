cmake_minimum_required(VERSION 3.10)
project(CountingBloomFilter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

include_directories(include)

add_library(bloomfilter
    src/MurmurHash3.cpp
    src/CountingBloomFilter.cpp
)

add_executable(demo src/main.cpp)
target_link_libraries(demo bloomfilter)

add_executable(benchmark benchmark/benchmark.cpp)
target_link_libraries(benchmark bloomfilter)
