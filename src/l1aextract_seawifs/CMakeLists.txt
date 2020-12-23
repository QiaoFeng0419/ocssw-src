cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_executable(l1aextract_seawifs
  main_l1aextract.c
  extract_sub.c
)

target_link_libraries(l1aextract_seawifs
  seawifs
)

install (TARGETS l1aextract_seawifs DESTINATION bin)
