cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set_ocssw_policy()

find_package(HDF5 REQUIRED COMPONENTS C HL)
include_directories(${HDF5_INCLUDE_DIRS})
set(LIBS ${HDF5_HL_LIBRARIES} ${HDF5_LIBRARIES})

find_package(Proj4 REQUIRED)
include_directories(${Proj4_INCLUDE_DIRS})
list(APPEND LIBS ${Proj4_LIBRARIES})

ADD_LIBRARY(goci
  goci.c
  goci_slot.c
)

target_link_libraries(goci ${LIBS})
target_include_directories(goci PUBLIC ".")

install (TARGETS goci DESTINATION lib)
