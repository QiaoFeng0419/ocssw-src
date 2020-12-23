cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project (OEL_UTIL)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

add_subdirectory("libpiutils")
add_subdirectory("libnetcdfutils")
add_subdirectory("libgenutils")
add_subdirectory("libtimeutils")

add_subdirectory("tests")

enable_testing()

