cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(LIBS timeutils)

find_package(PGSTK REQUIRED)
include_directories(${PGSTK_INCLUDE_DIRS})
list(APPEND LIBS ${PGSTK_LIBRARIES})

add_executable(atteph_info_modis
  atteph_info_modis.c
)

target_link_libraries(atteph_info_modis
  ${LIBS}
)

install (TARGETS atteph_info_modis DESTINATION bin)

