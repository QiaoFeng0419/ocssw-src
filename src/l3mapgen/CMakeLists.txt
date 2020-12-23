cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

#
# Make a version file containing the current version from git.
#
set (VERSION_MAJOR 2)
set (VERSION_MINOR 2)
set (VERSION_PATCH 0)
include(GetOcsswVersion)

set(LIBS l2 bin)

find_package(NetCDF REQUIRED COMPONENTS C CXX)
include_directories(${NETCDF_INCLUDE_DIRS})
list(APPEND LIBS ${NETCDF_LIBRARIES})

find_package(GeoTIFF REQUIRED)
include_directories(${GeoTIFF_INCLUDE_DIRS})
list(APPEND LIBS ${GeoTIFF_LIBRARIES})

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})
list(APPEND LIBS ${PNG_LIBRARIES})

find_package(Proj4 REQUIRED)
include_directories(${Proj4_INCLUDE_DIRS})
list(APPEND LIBS ${Proj4_LIBRARIES})

add_executable(l3mapgen
  OutFile.cpp
  l3mapgen.cpp
  l3mapgen_input.cpp
  )

target_link_libraries(l3mapgen
  ${LIBS}
  )

install (TARGETS l3mapgen DESTINATION bin)
