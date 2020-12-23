# LibHdf5utils_FOUND - true if library and headers were found
# LibHdf5utils_INCLUDE_DIRS - include directories
# LibHdf5utils_LIBRARIES - library directories

set(LibHdf5utils_LIBRARY hdf5utils)
set(LibHdf5utils_INCLUDE_DIR ${OEL_HDF4_SOURCE_DIR}/libhdf5utils)

set(LibHdf5utils_LIBRARIES ${LibHdf5utils_LIBRARY})
set(LibHdf5utils_INCLUDE_DIRS ${LibHdf5utils_INCLUDE_DIR})

find_package(LibPiutils REQUIRED)
list(APPEND LibHdf5utils_LIBRARIES ${LibPiutils_LIBRARIES})
list(APPEND LibHdf5utils_INCLUDE_DIRS ${LibPiutils_INCLUDE_DIRS})

find_package(HDF5 REQUIRED COMPONENTS C HL)
list(APPEND LibHdf5utils_LIBRARIES ${HDF5_C_HL_LIBRARIES} ${HDF5_LIBRARIES})
list(APPEND LibHdf5utils_INCLUDE_DIRS ${HDF5_INCLUDE_DIRS})

set(LibHdf5utils_FOUND TRUE)

mark_as_advanced(FORCE LibHdf5utils_INCLUDE_DIR LibHdf5utils_LIBRARY)

