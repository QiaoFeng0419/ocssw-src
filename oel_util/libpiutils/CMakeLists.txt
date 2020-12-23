cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(LIBS genutils)

find_package(PugiXML REQUIRED)
include_directories(${PUGIXML_INCLUDE_DIRS})
list(APPEND LIBS ${PUGIXML_LIBRARIES})

ADD_LIBRARY(piutils
  productInfo.cpp
  sensorInfo.c
  rdsensorinfo.c
  getDOI.cpp
  getGCMDKeywords.cpp
)

target_link_libraries(piutils ${LIBS})
target_include_directories(piutils PUBLIC ".")

install (TARGETS piutils DESTINATION lib)

