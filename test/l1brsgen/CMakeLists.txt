enable_testing()

#include(PrintAllVariables)
#print_all_variables()

cdata(READ data_files "${CMAKE_CURRENT_LIST_DIR}/CDataList.txt"
  GENERATE "${CMAKE_CURRENT_BINARY_DIR}/l1brsgen-data"
  VERBOSE
  TEST l1brsgen-data
  SAVE_DIR dataDir)

# add a test that creates the output directory
add_test(NAME "l1brsgen-output"
  WORKING_DIRECTORY ${dataDir}
  COMMAND mkdir -p output)

################################################################################
# l1brsgen PNG creation - Aqua
################################################################################
add_test(NAME "l1brsgen_A2008080195500.L1B_BRS_PNG-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1brsgen par=A2008080195500.L1B_BRS_PNG.par)

add_test(NAME "l1brsgen_A2008080195500.L1B_BRS_PNG-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND compare -metric AE -fuzz ${FUZZ} A2008080195500.L1B_BRS.png output/A2008080195500.L1B_BRS.png /dev/null )


################################################################################
#  l1brsgen HDF creation - Aqua
################################################################################
# modify the $ENV{OCTEST_TOLERANCE} for hdiff output

add_test(NAME "l1brsgen_A2008080195500.L1B_BRS-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1brsgen par=A2008080195500.L1B_BRS.par)

if(DEFINED ENV{OCTEST_TOLERANCE})
  set(TOLERANCE '')
  STRING(REGEX REPLACE "-T" "-p" TOLERANCE $ENV{OCTEST_TOLERANCE})
  add_test(NAME "l1brsgen_A2008080195500.L1B_BRS-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND compare_hdf4.py -G "Processing Time" ${TOLERANCE} A2008080195500.L1B_BRS.hdf output/A2008080195500.L1B_BRS.hdf)
else(DEFINED ENV{OCTEST_TOLERANCE})
  add_test(NAME "l1brsgen_A2008080195500.L1B_BRS-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND compare_hdf4.py -G "Processing Time" A2008080195500.L1B_BRS.hdf output/A2008080195500.L1B_BRS.hdf)
endif(DEFINED ENV{OCTEST_TOLERANCE})


################################################################################
#  l1brsgen HDF creation - 8bit Aqua
################################################################################
# modify the $ENV{OCTEST_TOLERANCE} for hdiff output

add_test(NAME "l1brsgen_A2008080195500.L1B_BRS.8bit-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1brsgen par=A2008080195500.L1B_BRS.8bit.par)

if(DEFINED ENV{OCTEST_TOLERANCE})
    set(TOLERANCE '')
    STRING(REGEX REPLACE "-T" "-p" TOLERANCE $ENV{OCTEST_TOLERANCE})
    add_test(NAME "l1brsgen_A2008080195500.L1B_BRS.8bit-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" ${TOLERANCE} A2008080195500.L1B_BRS.8bit.hdf output/A2008080195500.L1B_BRS.8bit.hdf)
else(DEFINED ENV{OCTEST_TOLERANCE})
    add_test(NAME "l1brsgen_A2008080195500.L1B_BRS.8bit-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" A2008080195500.L1B_BRS.8bit.hdf output/A2008080195500.L1B_BRS.8bit.hdf)
endif(DEFINED ENV{OCTEST_TOLERANCE})

################################################################################
# l1mapgen PPM creation - Aqua
################################################################################

add_test(NAME "l1mapgen_A2008080195500.L1B_MAP_PPM-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1mapgen par=A2008080195500.L1B_MAP_PPM.par)

add_test(NAME "l1mapgen_A2008080195500.L1B_MAP_PPM-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND compare -metric AE -fuzz ${FUZZ} A2008080195500.L1B_MAP.ppm output/A2008080195500.L1B_MAP.ppm output/A2002365234500.L1B_MAP.diff.ppm )

################################################################################
# l1mapgen PNG creation - Aqua
################################################################################
add_test(NAME "l1mapgen_A2008080195500.L1B_MAP_PNG-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1mapgen par=A2008080195500.L1B_MAP_PNG.par)

add_test(NAME "l1mapgen_A2008080195500.L1B_MAP_PNG-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND compare -metric AE -fuzz ${FUZZ} A2008080195500.L1B_MAP.png output/A2008080195500.L1B_MAP.png output/A2002365234500.L1B_MAP.diff.png )

################################################################################
# l1mapgen GeoTIFF creation - Aqua
################################################################################
add_test(NAME "l1mapgen_A2008080195500.L1B_MAP_TIFF-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1mapgen par=A2008080195500.L1B_MAP_TIFF.par)

add_test(NAME "l1mapgen_A2008080195500.L1B_MAP_TIFF-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND compare -metric AE -fuzz ${FUZZ} A2008080195500.L1B_MAP.tiff output/A2008080195500.L1B_MAP.tiff output/A2002365234500.L1B_MAP.diff.tiff)

################################################################################
# l1mapgen PNG creation - SeaWiFS
################################################################################
add_test(NAME "l1mapgen_S2002079071209.L1A_MLAC_MAP_PNG-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1mapgen par=S2002079071209.L1A_MLAC_MAP_PNG.par)

add_test(NAME "l1mapgen_S2002079071209.L1A_MLAC_MAP_PNG-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND compare -metric AE -fuzz ${FUZZ} S2002079071209.L1A_MLAC_MAP.png output/S2002079071209.L1A_MLAC_MAP.png output/S1998199173926.L1A_MLAC_MAP.diff.png )

################################################################################
#  ODPS TESTS
################################################################################

################################################################################
#  l1brsgen HDF creation - 8bit subsample=8 Aqua
################################################################################
# modify the $ENV{OCTEST_TOLERANCE} for hdiff output

add_test(NAME "l1brsgen_A2008080195500.L1B_BRS.subsample_8bit_ODPS-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1brsgen par=A2008080195500.L1B_BRS.subsample_8bit_ODPS.par)

if(DEFINED ENV{OCTEST_TOLERANCE})
    set(TOLERANCE '')
    STRING(REGEX REPLACE "-T" "-p" TOLERANCE $ENV{OCTEST_TOLERANCE})
    add_test(NAME "A2008080195500.L1B_BRS.subsample_8bit_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" ${TOLERANCE} A2008080195500.L1B_BRS.subsample_8bit_ODPS.hdf output/A2008080195500.L1B_BRS.subsample_8bit_ODPS.hdf)
else(DEFINED ENV{OCTEST_TOLERANCE})
    add_test(NAME "l1brsgen_A2008080195500.L1B_BRS.subsample_8bit_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" A2008080195500.L1B_BRS.subsample_8bit_ODPS.hdf output/A2008080195500.L1B_BRS.subsample_8bit_ODPS.hdf)
endif(DEFINED ENV{OCTEST_TOLERANCE})

################################################################################
#  l1brsgen HDF creation - subsample=8 CZCS
################################################################################
# modify the $ENV{OCTEST_TOLERANCE} for hdiff output

add_test(NAME "l1brsgen_C1982103032950.L1A_MLAC_BRS_ODPS-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1brsgen par=C1982103032950.L1A_MLAC_BRS_ODPS.par)

if(DEFINED ENV{OCTEST_TOLERANCE})
    set(TOLERANCE '')
    STRING(REGEX REPLACE "-T" "-p" TOLERANCE $ENV{OCTEST_TOLERANCE})
    add_test(NAME "C1982103032950.L1A_MLAC_BRS_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" ${TOLERANCE} C1982103032950.L1A_MLAC_BRS_ODPS.hdf output/C1982103032950.L1A_MLAC_BRS_ODPS.hdf)
else(DEFINED ENV{OCTEST_TOLERANCE})
    add_test(NAME "l1brsgen_C1982103032950.L1A_MLAC_BRS_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" C1982103032950.L1A_MLAC_BRS_ODPS.hdf output/C1982103032950.L1A_MLAC_BRS_ODPS.hdf)
endif(DEFINED ENV{OCTEST_TOLERANCE})

################################################################################
#  l1brsgen HDF creation - 8bit subsample=8 GOCI
################################################################################
# modify the $ENV{OCTEST_TOLERANCE} for hdiff output

add_test(NAME "l1brsgen_COMS_GOCI_L1B_GA_20121003041641.he5_BRS_ODPS-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1brsgen par=COMS_GOCI_L1B_GA_20121003041641.he5_BRS_ODPS.par)

if(DEFINED ENV{OCTEST_TOLERANCE})
    set(TOLERANCE '')
    STRING(REGEX REPLACE "-T" "-p" TOLERANCE $ENV{OCTEST_TOLERANCE})
    add_test(NAME "COMS_GOCI_L1B_GA_20121003041641.he5_BRS_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" ${TOLERANCE} COMS_GOCI_L1B_GA_20121003041641.he5_BRS_ODPS.hdf output/COMS_GOCI_L1B_GA_20121003041641.he5_BRS_ODPS.hdf)
else(DEFINED ENV{OCTEST_TOLERANCE})
    add_test(NAME "l1brsgen_COMS_GOCI_L1B_GA_20121003041641.he5_BRS_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" COMS_GOCI_L1B_GA_20121003041641.he5_BRS_ODPS.hdf output/COMS_GOCI_L1B_GA_20121003041641.he5_BRS_ODPS.hdf)
endif(DEFINED ENV{OCTEST_TOLERANCE})

################################################################################
#  l1brsgen HDF creation - 8bit subsample=8 OCTS
################################################################################
# modify the $ENV{OCTEST_TOLERANCE} for hdiff output

add_test(NAME "l1brsgen_O1997079195357.L1A_GAC_BRS_ODPS-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1brsgen par=O1997079195357.L1A_GAC_BRS_ODPS.par)

if(DEFINED ENV{OCTEST_TOLERANCE})
    set(TOLERANCE '')
    STRING(REGEX REPLACE "-T" "-p" TOLERANCE $ENV{OCTEST_TOLERANCE})
    add_test(NAME "O1997079195357.L1A_GAC_BRS_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" ${TOLERANCE} O1997079195357.L1A_GAC_BRS_ODPS.hdf output/O1997079195357.L1A_GAC_BRS_ODPS.hdf)
else(DEFINED ENV{OCTEST_TOLERANCE})
    add_test(NAME "l1brsgen_O1997079195357.L1A_GAC_BRS_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" O1997079195357.L1A_GAC_BRS_ODPS.hdf output/O1997079195357.L1A_GAC_BRS_ODPS.hdf)
endif(DEFINED ENV{OCTEST_TOLERANCE})

################################################################################
#  l1brsgen HDF creation - 8bit subsample=8 SeaWiFS
################################################################################
# modify the $ENV{OCTEST_TOLERANCE} for hdiff output

add_test(NAME "l1brsgen_S1998199173926.L1A_GAC_BRS_ODPS-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1brsgen par=S1998199173926.L1A_GAC_BRS_ODPS.par)

if(DEFINED ENV{OCTEST_TOLERANCE})
    set(TOLERANCE '')
    STRING(REGEX REPLACE "-T" "-p" TOLERANCE $ENV{OCTEST_TOLERANCE})
    add_test(NAME "S1998199173926.L1A_GAC_BRS_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" ${TOLERANCE} S1998199173926.L1A_GAC_BRS_ODPS.hdf output/S1998199173926.L1A_GAC_BRS_ODPS.hdf)
else(DEFINED ENV{OCTEST_TOLERANCE})
    add_test(NAME "l1brsgen_S1998199173926.L1A_GAC_BRS_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" S1998199173926.L1A_GAC_BRS_ODPS.hdf output/S1998199173926.L1A_GAC_BRS_ODPS.hdf)
endif(DEFINED ENV{OCTEST_TOLERANCE})

################################################################################
#  l1brsgen HDF creation - 8bit subsample=8 Terra
################################################################################
# modify the $ENV{OCTEST_TOLERANCE} for hdiff output

add_test(NAME "l1brsgen_T2008080122500.L1B_BRS.subsample_8bit_ODPS-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1brsgen par=T2008080122500.L1B_BRS.subsample_8bit_ODPS.par)

if(DEFINED ENV{OCTEST_TOLERANCE})
    set(TOLERANCE '')
    STRING(REGEX REPLACE "-T" "-p" TOLERANCE $ENV{OCTEST_TOLERANCE})
    add_test(NAME "T2008080122500.L1B_BRS.subsample_8bit_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" ${TOLERANCE} T2008080122500.L1B_BRS.subsample_8bit_ODPS.hdf output/T2008080122500.L1B_BRS.subsample_8bit_ODPS.hdf)
else(DEFINED ENV{OCTEST_TOLERANCE})
    add_test(NAME "l1brsgen_T2008080122500.L1B_BRS.subsample_8bit_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" T2008080122500.L1B_BRS.subsample_8bit_ODPS.hdf output/T2008080122500.L1B_BRS.subsample_8bit_ODPS.hdf)
endif(DEFINED ENV{OCTEST_TOLERANCE})

################################################################################
#  l1brsgen HDF creation - 8bit subsample=8 VIIRS
################################################################################
# modify the $ENV{OCTEST_TOLERANCE} for hdiff output

add_test(NAME "l1brsgen_V2015281181800.L1B_BRS.subsample_8bit_ODPS-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l1brsgen par=V2015281181800.L1B_BRS.subsample_8bit_ODPS.par)

if(DEFINED ENV{OCTEST_TOLERANCE})
    set(TOLERANCE '')
    STRING(REGEX REPLACE "-T" "-p" TOLERANCE $ENV{OCTEST_TOLERANCE})
    add_test(NAME "V2015281181800.L1B_BRS.subsample_8bit_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" ${TOLERANCE} V2015281181800.L1B_BRS.subsample_8bit_ODPS.hdf output/V2015281181800.L1B_BRS.subsample_8bit_ODPS.hdf)
else(DEFINED ENV{OCTEST_TOLERANCE})
    add_test(NAME "l1brsgen_V2015281181800.L1B_BRS.subsample_8bit_ODPS-check"
        WORKING_DIRECTORY ${dataDir}
        COMMAND compare_hdf4.py -G "Processing Time" V2015281181800.L1B_BRS.subsample_8bit_ODPS.hdf output/V2015281181800.L1B_BRS.subsample_8bit_ODPS.hdf)
endif(DEFINED ENV{OCTEST_TOLERANCE})







