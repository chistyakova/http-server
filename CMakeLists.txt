cmake_minimum_required(VERSION 2.8)

project(http-server)

add_executable(final main.cpp)

target_link_libraries(final -lpthread)
