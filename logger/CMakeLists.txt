cmake_minimum_required(VERSION 3.5)
project(Liblogger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

add_library(logger logger.cpp)
target_include_directories(logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
