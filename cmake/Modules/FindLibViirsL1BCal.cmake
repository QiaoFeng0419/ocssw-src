# LibViirsL1BCal_FOUND - true if library and headers were found
# LibViirsL1BCal_INCLUDE_DIRS - include directories
# LibViirsL1BCal_LIBRARIES - library directories

if(EXISTS ${VIIRS_LIB_SOURCE_DIR}/libViirsL1BCal)
    set(LibViirsL1BCal_LIBRARY ViirsL1BCal)
    set(LibViirsL1BCal_INCLUDE_DIR ${VIIRS_LIB_SOURCE_DIR}/libViirsL1BCal)
else()
    set(LibViirsL1BCal_LIBRARY $ENV{LIB3_DIR}/lib/libViirsL1BCal.a)
    set(LibViirsL1BCal_INCLUDE_DIR $ENV{LIB3_DIR}/include/ViirsL1BCal)
endif()

set(LibViirsL1BCal_LIBRARIES ${LibViirsL1BCal_LIBRARY})
set(LibViirsL1BCal_INCLUDE_DIRS ${LibViirsL1BCal_INCLUDE_DIR})

find_package(LibViirsCmn REQUIRED)
list(APPEND LibViirsL1BCal_LIBRARIES ${LibViirsCmn_LIBRARIES})
list(APPEND LibViirsL1BCal_INCLUDE_DIRS ${LibViirsCmn_INCLUDE_DIRS})

set(LibViirsL1BCal_FOUND TRUE)

mark_as_advanced(FORCE LibViirsL1BCal_INCLUDE_DIR LibViirsL1BCal_LIBRARY)
