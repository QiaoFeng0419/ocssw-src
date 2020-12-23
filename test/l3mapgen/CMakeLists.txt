enable_testing()

#include(PrintAllVariables)
#print_all_variables()

cdata(READ data_files "${CMAKE_CURRENT_LIST_DIR}/CDataList.txt"
  GENERATE "${CMAKE_CURRENT_BINARY_DIR}/l3mapgen-data"
  VERBOSE
  TEST l3mapgen-data
  SAVE_DIR dataDir)

# add a test that creates the output directory
add_test(NAME "l3mapgen-output"
  WORKING_DIRECTORY ${dataDir}
  COMMAND mkdir -p output)

# hdf.input.nc.par - HDF4 input, netCDF output, one product
# multiple.nc.par - multiple output products, area interp, smi projection
# out.hdf.par - netCDF input, HDF4 output
# out.nc.par - netCDF output
# out.png.par - PNG output
# out.ppm.par - PPM output
# out.tiff.par - GEO TIFF output
# projection.nc.par - mollweide projection
# resolution.nc.par - 90km resolution pixel
# rgb.png.par - make a RGB true color image
#


# get a list of the netCDF par files in this directory
file(GLOB files "${dataDir}/*.nc.par")

# loop through all netCDF par files found and make tests
foreach(filename ${files})

  GET_FILENAME_COMPONENT(parfile ${filename} NAME)
  STRING(REGEX REPLACE ".par" "" outfile ${parfile})

  add_test(NAME "l3mapgen-${outfile}-test"
    WORKING_DIRECTORY ${dataDir}
    COMMAND l3mapgen par=${parfile})

  add_test(NAME "l3mapgen-${outfile}-check"
    WORKING_DIRECTORY ${dataDir}
    COMMAND nccmp -m -g -d -f -C 10 -G date_created,software_version,_lastModified $ENV{OCTEST_TOLERANCE} ${outfile} output/${outfile})

endforeach(filename)


# test for HDF4 SMI output
add_test(NAME "l3mapgen-hdf4-test"
  WORKING_DIRECTORY ${dataDir}
  COMMAND l3mapgen par=out.hdf.par)

add_test(NAME "l3mapgen-hdf4-check"
  WORKING_DIRECTORY ${dataDir}
  COMMAND compare_hdf4.py -G "Software Version,Processing Time,Processing Control,_lastModified" out.hdf output/out.hdf)


# test PNG output
add_test(NAME "l3mapgen-png-test"
  WORKING_DIRECTORY ${dataDir}
  COMMAND l3mapgen par=out.png.par)

add_test(NAME "l3mapgen-png-check"
  WORKING_DIRECTORY ${dataDir}
  COMMAND compare -metric AE -fuzz ${FUZZ} out.png output/out.png /dev/null )


# test PNG transparent missing data output
add_test(NAME "l3mapgen-transparent-png-test"
  WORKING_DIRECTORY ${dataDir}
  COMMAND l3mapgen par=out.transparent.png.par)

add_test(NAME "l3mapgen-transparent-png-check"
  WORKING_DIRECTORY ${dataDir}
  COMMAND compare -metric AE -fuzz ${FUZZ} out.transparent.png output/out.transparent.png /dev/null )


# test PPM output
add_test(NAME "l3mapgen-ppm-test"
  WORKING_DIRECTORY ${dataDir}
  COMMAND l3mapgen par=out.ppm.par)

add_test(NAME "l3mapgen-ppm-check"
  WORKING_DIRECTORY ${dataDir}
  COMMAND compare -metric AE -fuzz ${FUZZ} out.ppm output/out.ppm /dev/null )


# need to add a test for the geotiff metadata tags
#
add_test(NAME "l3mapgen-tiff-test"
  WORKING_DIRECTORY ${dataDir}
  COMMAND l3mapgen par=out.tiff.par)

add_test(NAME "l3mapgen-tiff-check"
  WORKING_DIRECTORY ${dataDir}
  COMMAND tiffcmp out.tiff output/out.tiff )


# test geotiff transparent missing data
add_test(NAME "l3mapgen-transparent-tiff-test"
  WORKING_DIRECTORY ${dataDir}
  COMMAND l3mapgen par=out.transparent.tiff.par)

add_test(NAME "l3mapgen-transparent-tiff-check"
  WORKING_DIRECTORY ${dataDir}
  COMMAND tiffcmp out.transparent.tiff output/out.transparent.tiff )


# test RGB PNG output
add_test(NAME "l3mapgen-rgb-test"
  WORKING_DIRECTORY ${dataDir}
  COMMAND l3mapgen par=rgb.png.par)

add_test(NAME "l3mapgen-rgb-check"
  WORKING_DIRECTORY ${dataDir}
  COMMAND compare -metric AE -fuzz ${FUZZ} rgb.png output/rgb.png /dev/null )

# add a test for multiple tiff files
add_test(NAME "l3mapgen-multiple-tiff-test"
  WORKING_DIRECTORY ${dataDir}
  COMMAND l3mapgen par=multiple_PRODUCT.tiff.par)

add_test(NAME "l3mapgen-multiple-tiff1-check"
  WORKING_DIRECTORY ${dataDir}
  COMMAND tiffcmp multiple_Rrs_443.tiff output/multiple_Rrs_443.tiff )

add_test(NAME "l3mapgen-multiple-tiff2-check"
  WORKING_DIRECTORY ${dataDir}
  COMMAND tiffcmp multiple_Rrs_555.tiff output/multiple_Rrs_555.tiff )

