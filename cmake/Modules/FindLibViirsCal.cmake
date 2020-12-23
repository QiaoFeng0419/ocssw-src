# ViirsCal_FOUND - true if library and headers were found
# LibViirsCal_INCLUDE_DIRS - include directories
# LibViirsCal_LIBRARIES - library directories

if(EXISTS ${VIIRS_LIB_SOURCE_DIR}/libViirsCal)
    set(LibViirsCal_LIBRARY ViirsCal)
    set(LibViirsCal_INCLUDE_DIR "")
    set(LibViirsCal_LIBRARIES ${LibViirsCal_LIBRARY})
    set(LibViirsCal_INCLUDE_DIRS "")
else()
    set(LibViirsCal_LIBRARY $ENV{LIB3_DIR}/lib/libViirsCal.a)
    set(LibViirsCal_INCLUDE_DIR $ENV{LIB3_DIR}/include/ViirsCal)
    set(LibViirsCal_LIBRARIES ${LibViirsCal_LIBRARY})
    set(LibViirsCal_INCLUDE_DIRS ${LibViirsCal_INCLUDE_DIR})

    find_package(LibViirsCmn REQUIRED)
    list(APPEND LibViirsCal_LIBRARIES ${LibViirsCmn_LIBRARIES})
    list(APPEND LibViirsCal_INCLUDE_DIRS ${LibViirsCmn_INCLUDE_DIRS})
endif()

set(LibViirsCal_FOUND TRUE)

mark_as_advanced(FORCE LibViirsCal_INCLUDE_DIR LibViirsCal_LIBRARY)
