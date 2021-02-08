cmake_minimum_required(VERSION 2.8)

project(http-server)

add_executable(final main.cpp)

target_link_libraries(final -lpthread)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
