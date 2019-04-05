cmake_minimum_required(VERSION 3.13)
project(hw2 C)

set(CMAKE_C_STANDARD 11)

add_executable(hw2 main.c)

target_link_libraries(hw2 m)
