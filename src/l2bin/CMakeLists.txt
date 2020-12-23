cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(LIBS l2 nav bin)

find_package(GSL REQUIRED)
include_directories(${GSL_INCLUDE_DIRS})
list(APPEND LIBS ${GSL_LIBRARIES})

add_executable(l2bin
  l2bin.c
  l2bin_input.c
  dataday.c
)

target_link_libraries(l2bin
  ${LIBS}
  ${LibBin_LIBRARIES}
)

install (TARGETS l2bin DESTINATION bin)
