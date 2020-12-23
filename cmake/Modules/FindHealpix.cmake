# Healpix_FOUND - true if library and headers were found
# Healpix_INCLUDE_DIRS - include directories
# Healpix_LIBRARIES - library directories

find_path(Healpix_INCLUDE_DIR chealpix.h
  PATHS $ENV{LIB3_DIR}/include
  )

find_library(Healpix_LIBRARY NAMES healpix
  PATHS $ENV{LIB3_DIR}/lib
  )

find_library(Healpix_C_LIBRARY NAMES chealpix
  PATHS $ENV{LIB3_DIR}/lib
  )

find_library(Healpix_Cfitsio_LIBRARY NAMES cfitsio
  PATHS $ENV{LIB3_DIR}/lib
  )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Healpix DEFAULT_MSG Healpix_LIBRARY Healpix_C_LIBRARY Healpix_Cfitsio_LIBRARY Healpix_INCLUDE_DIR)
if(Healpix_FOUND)
  set(Healpix_LIBRARIES ${Healpix_C_LIBRARY} ${Healpix_LIBRARY} ${Healpix_Cfitsio_LIBRARY})
  set(Healpix_INCLUDE_DIRS ${Healpix_INCLUDE_DIR})
endif()
  
mark_as_advanced(Healpix_INCLUDE_DIR Healpix_LIBRARY Healpix_C_LIBRARY Healpix_Cfitsio_LIBRARY)
