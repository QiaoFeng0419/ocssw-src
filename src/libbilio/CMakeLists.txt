cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(LIBS nav timeutils genutils)

find_package(Proj4 REQUIRED)
include_directories(${Proj4_INCLUDE_DIRS})
list(APPEND LIBS ${Proj4_LIBRARIES})
  
ADD_LIBRARY(bilio
  jplaeriallib.c
  read_aviris.c
  read_prism.c
)

target_link_libraries(bilio ${LIBS})
target_include_directories(bilio PUBLIC ".")

install (TARGETS bilio DESTINATION lib)
