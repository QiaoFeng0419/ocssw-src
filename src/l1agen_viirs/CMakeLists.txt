cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(LIBS netcdfutils)

find_package(NetCDF REQUIRED COMPONENTS C)
include_directories(${NETCDF_INCLUDE_DIRS})
list(APPEND LIBS ${NETCDF_LIBRARIES})

find_package(LibViirsRice REQUIRED)
if(LibViirsRice_INCLUDE_DIRS)
  include_directories(${LibViirsRice_INCLUDE_DIRS})
endif()
#list(APPEND LIBS ${LibViirsRice_LIBRARIES})

add_executable(l1agen_viirs
  l1agen_viirs.cpp
  l1aFile.cpp
  Usds.c
)

add_executable(l1aextract_viirs
  l1aextract_viirs.cpp
  l1aFile.cpp
)

add_executable(scpad_viirs
  scpad_viirs.cpp
  createNCDF_viirs.cpp
  l1aFile.cpp
)

add_executable(scpad_viirs_lsips
  scpad_viirs_lsips.cpp
  l1aFile.cpp
)

target_link_libraries(l1agen_viirs
  ${LibViirsRice_LIBRARIES}
  ${LIBS}
)

target_link_libraries(l1aextract_viirs
  ${LIBS}
)

target_link_libraries(scpad_viirs
  ${LIBS}
)

target_link_libraries(scpad_viirs_lsips
  ${LIBS}
)

install (TARGETS l1agen_viirs l1aextract_viirs scpad_viirs scpad_viirs_lsips DESTINATION bin)
