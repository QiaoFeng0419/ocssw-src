cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_executable(smitoppm
  smitoppm.c
)

target_link_libraries(smitoppm
  map
  bin
)

install (TARGETS smitoppm DESTINATION bin)

