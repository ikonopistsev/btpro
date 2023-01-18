cmake_minimum_required(VERSION 3.18)

project(btpro VERSION 3.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
  add_definitions(-DNOMINMAX)
endif()

add_library(btpro INTERFACE)

target_include_directories(btpro INTERFACE include/)
