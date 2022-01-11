cmake_minimum_required(VERSION 3.10)

project(btpro VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
  add_definitions(-DNOMINMAX)
endif()

add_library(btpro INTERFACE ./) 
