# LibViirsGeo_FOUND - true if library and headers were found
# LibViirsGeo_INCLUDE_DIRS - include directories
# LibViirsGeo_LIBRARIES - library directories

if(EXISTS ${VIIRS_LIB_SOURCE_DIR}/libViirsGeo)
    set(LibViirsGeo_LIBRARY ViirsGeo)
    set(LibViirsGeo_INCLUDE_DIR "")
    set(LibViirsGeo_LIBRARIES ${LibViirsGeo_LIBRARY})
    set(LibViirsGeo_INCLUDE_DIRS "")
else()
    set(LibViirsGeo_LIBRARY $ENV{LIB3_DIR}/lib/libViirsGeo.a)
    set(LibViirsGeo_INCLUDE_DIR $ENV{LIB3_DIR}/include/ViirsGeo)
    set(LibViirsGeo_LIBRARIES ${LibViirsGeo_LIBRARY})
    set(LibViirsGeo_INCLUDE_DIRS ${LibViirsGeo_INCLUDE_DIR})

    find_package(LibViirsCmn REQUIRED)
    list(APPEND LibViirsGeo_LIBRARIES ${LibViirsCmn_LIBRARIES})
    list(APPEND LibViirsGeo_INCLUDE_DIRS ${LibViirsCmn_INCLUDE_DIRS})

    find_package(LibViirsCal REQUIRED)
    list(APPEND LibViirsGeo_LIBRARIES ${LibViirsCal_LIBRARIES})
    list(APPEND LibViirsGeo_INCLUDE_DIRS ${LibViirsCal_INCLUDE_DIRS})

    list(APPEND LibViirsGeo_LIBRARIES timeutils)
endif()

set(LibViirsGeo_FOUND TRUE)

mark_as_advanced(FORCE LibViirsGeo_INCLUDE_DIR LibViirsGeo_LIBRARY)
