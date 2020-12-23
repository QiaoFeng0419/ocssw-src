enable_testing()

cdata(READ data_files "${CMAKE_CURRENT_LIST_DIR}/CDataList.txt"
  GENERATE "${CMAKE_CURRENT_BINARY_DIR}/l1bextract_meris-data"
  VERBOSE
  TEST l1bextract_meris-data
  SAVE_DIR dataDir)

# add a test that creates the output directory
add_test(NAME "l1bextract_meris-output"
  WORKING_DIRECTORY ${dataDir}
  COMMAND mkdir -p output)

add_test(NAME "l1bextract_meris-extract-test"
  WORKING_DIRECTORY ${dataDir}
  COMMAND l1bextract_meris MER_RR__1PNACR20060921_150933_000026302051_00240_23841_0000.N1 400 900 5377 5900 output/extract.N1)

add_test(NAME "l1bextract_meris-extract-l2gen-test"
  WORKING_DIRECTORY ${dataDir}
  COMMAND l2gen par=extract.par)

add_test(NAME "l1bextract_meris-check"
  WORKING_DIRECTORY ${dataDir}
  COMMAND nccmp -m -g -d -f -C 10 -G date_created,software_version $ENV{OCTEST_TOLERANCE} extract.L2 output/extract.L2)
