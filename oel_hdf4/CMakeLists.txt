cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

project (OEL_HDF4)

file(GLOB files "lib*")

foreach(dir ${files})
  add_subdirectory(${dir})
endforeach(dir)
