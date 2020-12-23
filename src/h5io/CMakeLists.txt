cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set_ocssw_policy()

set(LIBS "")

find_package(HDF5 REQUIRED COMPONENTS C)
include_directories(${HDF5_INCLUDE_DIRS})
list(APPEND LIBS ${HDF5_LIBRARIES})

add_library(h5io h5io.c)
target_link_libraries(h5io ${LIBS})
target_include_directories(h5io PUBLIC ".")

install(TARGETS h5io DESTINATION lib)
