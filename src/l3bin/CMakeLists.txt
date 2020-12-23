cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_executable(l3bin
  l3bin.cpp
  l3bin_input.c
)

add_executable(l3binmerge
  l3binmerge.cpp
  l3bin_input.c
)

add_executable(l3rebin_meris
  l3rebin_meris.cpp
)

target_link_libraries(l3bin
  bin
)

target_link_libraries(l3binmerge
  bin
)

target_link_libraries(l3rebin_meris
  bin
)

install (TARGETS l3bin l3binmerge l3rebin_meris DESTINATION bin)
