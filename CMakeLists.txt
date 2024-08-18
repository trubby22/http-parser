cmake_minimum_required(VERSION 3.27)
project(http-parser)

set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 23)

find_package(Boost 1.85 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_libraries(${Boost_LIBRARIES})

add_executable(main main.cpp)
