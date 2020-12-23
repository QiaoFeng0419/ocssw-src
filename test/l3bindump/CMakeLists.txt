enable_testing()

#include(PrintAllVariables)
#print_all_variables()

cdata(READ data_files "${CMAKE_CURRENT_LIST_DIR}/CDataList.txt"
  GENERATE "${CMAKE_CURRENT_BINARY_DIR}/l3bindump-data"
  VERBOSE
  TEST l3bindump-data
  SAVE_DIR dataDir)

# add a test that creates the output directory
add_test(NAME "l3bindump-output"
  WORKING_DIRECTORY ${dataDir}
  COMMAND mkdir -p output)

# get a list of the netCDF par files in this directory
file(GLOB files "${dataDir}/*.txt.par")

# loop through all netCDF par files found and make tests
foreach(filename ${files})

  GET_FILENAME_COMPONENT(parfile ${filename} NAME)
  STRING(REGEX REPLACE ".par" "" outfile ${parfile})

  add_test(NAME "l3bindump-${outfile}-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l3bindump par=${parfile})

  add_test(NAME "l3bindump-${outfile}-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND diff ${outfile} output/${outfile})

endforeach(filename)

