enable_testing()

cdata(READ data_files "${CMAKE_CURRENT_LIST_DIR}/CDataList.txt"
  GENERATE "${CMAKE_CURRENT_BINARY_DIR}/l2gen-data"
  VERBOSE
  TEST geogen_modis-data
  SAVE_DIR dataDir)

# add a test that creates the output directory
add_test(NAME "geogen_modis-output"
  WORKING_DIRECTORY ${dataDir}
  COMMAND mkdir -p output)

################################################################################
# loop through l1A files making GEO files
################################################################################
file(GLOB files "${dataDir}/*L1A_LAC*")
foreach(filename ${files})

  GET_FILENAME_COMPONENT(l1afile ${filename} NAME)
  STRING(REGEX REPLACE "L1A_LAC" "GEO" geofile ${l1afile})

  add_test(NAME "geogen_modis_${geofile}-copy"
    WORKING_DIRECTORY ${dataDir}
    COMMAND cp ${l1afile} output)

  add_test(NAME "geogen_modis-${geofile}-test"
    WORKING_DIRECTORY ${dataDir}/output
    COMMAND modis_GEO.py --refreshDB -o ${geofile} ${l1afile})
  
  add_test(NAME "geogen_modis-${geofile}-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND compare_hdf4.py -G "CoreMetadata.0,utcpole File Header" ${geofile} output/${geofile})

endforeach(filename)

################################################################################
# loop through l1A files making L1B files
################################################################################
foreach(filename ${files})

  GET_FILENAME_COMPONENT(l1afile ${filename} NAME)
  STRING(REGEX REPLACE "L1A_LAC" "L1B_LAC" okmfile ${l1afile})
  STRING(REGEX REPLACE "L1A_LAC" "L1B_HKM" hkmfile ${l1afile})
  STRING(REGEX REPLACE "L1A_LAC" "L1B_QKM" qkmfile ${l1afile})
  STRING(REGEX REPLACE "L1A_LAC" "L1B_OBC" obcfile ${l1afile})
  STRING(REGEX REPLACE "L1A_LAC" "GEO" geofile ${l1afile})
  if(l1afile MATCHES "^A")
    set(lutdir "../luts/aqua")
  else()
    set(lutdir "../luts/terra")
  endif()
  
  add_test(NAME "l1bgen_modis-${okmfile}-test"
    WORKING_DIRECTORY ${dataDir}/output
    COMMAND modis_L1B.py --keep-obc --lutdir=${lutdir} ${l1afile} ../${geofile})

  add_test(NAME "l1bgen_modis-${okmfile}-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND compare_hdf4.py -G "CoreMetadata.0,ArchiveMetadata.0" ${okmfile} output/${okmfile})

  add_test(NAME "l1bgen_modis-${hkmfile}-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND compare_hdf4.py -G "CoreMetadata.0,ArchiveMetadata.0" ${hkmfile} output/${hkmfile})

  add_test(NAME "l1bgen_modis-${qkmfile}-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND compare_hdf4.py -G "CoreMetadata.0,ArchiveMetadata.0" ${qkmfile} output/${qkmfile})

  add_test(NAME "l1bgen_modis-${obcfile}-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND compare_hdf4.py -G "CoreMetadata.0,ArchiveMetadata.0" ${obcfile} output/${obcfile})

endforeach(filename)
