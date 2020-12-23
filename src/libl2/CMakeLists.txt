cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(LIBS seawifs dfutils)

#find_package(NetCDF REQUIRED)
#include_directories(${NETCDF_INCLUDE_DIRS})
#list(APPEND LIBS ${NETCDF_LIBRARIES})

ADD_LIBRARY(l2
  readL2scan.c
  setupflags.c
  get_product_table.c
)

target_link_libraries(l2 ${LIBS})
target_include_directories(l2 PUBLIC ".")

install (TARGETS l2 DESTINATION lib)
