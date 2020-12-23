# LibH4metadata_FOUND - true if library and headers were found
# LibH4metadata_INCLUDE_DIRS - include directories
# LibH4metadata_LIBRARIES - library directories

set(LibH4metadata_LIBRARY h4metadata)
set(LibH4metadata_INCLUDE_DIR ${OEL_HDF4_SOURCE_DIR}/libh4metadata)

set(LibH4metadata_LIBRARIES ${LibH4metadata_LIBRARY})
set(LibH4metadata_INCLUDE_DIRS ${LibH4metadata_INCLUDE_DIR})

find_package(HDF4 REQUIRED)
list(APPEND LibH4metadata_LIBRARIES ${HDF4_LIBRARIES})
list(APPEND LibH4metadata_INCLUDE_DIRS ${HDF4_INCLUDE_DIRS})

set(LibH4metadata_FOUND TRUE)

mark_as_advanced(FORCE LibH4metadata_INCLUDE_DIR LibH4metadata_LIBRARY)

