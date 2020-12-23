# LibViirsRice_FOUND - true if library and headers were found
# LibViirsRice_INCLUDE_DIRS - include directories
# LibViirsRice_LIBRARIES - library directories

if(EXISTS ${VIIRS_LIB_SOURCE_DIR}/libViirsRice)
    set(LibViirsRice_LIBRARY ViirsRice)
    set(LibViirsRice_INCLUDE_DIR "")
else()
    set(LibViirsRice_LIBRARY $ENV{LIB3_DIR}/lib/libViirsRice.a)
    set(LibViirsRice_INCLUDE_DIR $ENV{LIB3_DIR}/include/ViirsRice)
endif()

set(LibViirsRice_FOUND TRUE)
set(LibViirsRice_LIBRARIES ${LibViirsRice_LIBRARY} m)
set(LibViirsRice_INCLUDE_DIRS ${LibViirsRice_INCLUDE_DIR})

mark_as_advanced(FORCE LibViirsRice_INCLUDE_DIR LibViirsRice_LIBRARY)
