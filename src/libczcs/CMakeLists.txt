cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(LIBS nav)

ADD_LIBRARY(czcs
  czcscaleg.f
  czsuba.f
  satang.f
  subsatxy.f
)

target_link_libraries(czcs ${LIBS})
target_include_directories(czcs PUBLIC ".")

install (TARGETS czcs DESTINATION lib)
