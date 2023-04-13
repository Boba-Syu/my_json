cmake_minimum_required(VERSION 3.25)
project(my_json)

set(CMAKE_CXX_STANDARD 17)

add_executable(my_json my_json.h my_json.cpp test.cpp)
