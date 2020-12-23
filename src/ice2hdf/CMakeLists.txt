cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

find_package(LibHdf4utils REQUIRED)
include_directories(${LibHdf4utils_INCLUDE_DIRS})
set(LIBS ${LibHdf4utils_LIBRARIES})

add_executable(ice2hdf
  ice2hdf.c
)

target_link_libraries(ice2hdf ${LIBS})

install (TARGETS ice2hdf DESTINATION bin)
