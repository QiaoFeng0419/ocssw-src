# LibOsmi_FOUND - true if library and headers were found
# LibOsmi_INCLUDE_DIRS - include directories
# LibOsmi_LIBRARIES - librarys

set(LibOsmi_LIBRARY osmi)
set(LibOsmi_INCLUDE_DIR ${OCSSW_SOURCE_DIR}/src/libosmi)

set(LibOsmi_LIBRARIES ${LibOsmi_LIBRARY})
set(LibOsmi_INCLUDE_DIRS ${LibOsmi_INCLUDE_DIR})

set(LibOsmi_FOUND TRUE)

mark_as_advanced(FORCE LibOsmi_INCLUDE_DIR LibOsmi_LIBRARY)

