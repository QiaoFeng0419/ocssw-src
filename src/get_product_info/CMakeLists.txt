cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_executable(get_product_info
  get_product_info.c
)

target_link_libraries(get_product_info
  l2
)

install (TARGETS get_product_info DESTINATION bin)
