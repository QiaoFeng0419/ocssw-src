cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_executable(geogen_hico
  SUNCOR.f90
  SOLCOR.f90
  JULIAN.f90
  HAZEL.f90
  file_io_new.f90
  write_envi_header.f90
  envi_header.f90
  check_range.f90
  errors.f90
  auxtools.f90
  chartools.f90
  default_character_length.f90
  generic_info.f90
  ASTMATH.FOR
  ecef2latlon.f90
  solar_geometry.f90
  ASTREDUC.FOR
  ASTTIME.f
  q_to_r.f90
  params.f90
  proc_hico.f90
  get_astro_data.f90
  intercept.f90
  bore_site.f90
)

set_target_properties(geogen_hico PROPERTIES LINKER_LANGUAGE Fortran)

# Set up FORTRAN flags
set (CMAKE_Fortran_FLAGS "-DLINUX -Dlinux -ffixed-line-length-none -fno-automatic -fno-range-check -cpp")
set (CMAKE_Fortran_FLAGS_RELEASE "-O3 -funroll-all-loops")
set (CMAKE_Fortran_FLAGS_DEBUG "-g3 -gdwarf-2 -Wall -Wno-unused-variable -Wno-unused-label -Wno-unused-dummy-argument -Wno-conversion")

install (TARGETS geogen_hico DESTINATION bin)
