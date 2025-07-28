cmake_minimum_required(VERSION 3.5)

project(LibDoit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(doit SHARED
    doit.h
    doit.cpp
)

target_include_directories(doit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
