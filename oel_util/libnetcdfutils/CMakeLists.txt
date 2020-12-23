cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(LIBS piutils timeutils)

find_package(NetCDF REQUIRED)
include_directories(${NETCDF_INCLUDE_DIRS})
list(APPEND LIBS ${NETCDF_LIBRARIES})
  
add_library (netcdfutils
  nc_gridutils.c
  nc_init_compress.c
  nc4utils.c
  cdl_utils.cpp
  createNCDF.c
)

target_link_libraries(netcdfutils ${LIBS})
target_include_directories(netcdfutils PUBLIC ".")

install (TARGETS netcdfutils DESTINATION lib)
