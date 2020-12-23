cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

#
# Make a version file containing the current version from git.
#
set (VERSION_MAJOR 9)
set (VERSION_MINOR 5)
set (VERSION_PATCH 0)
include(GetOcsswVersion)

#####version 3 of Atrem - no fortran libraries, just C code
# TODO: get rid of numerical.c
#set (ATREM_C_CODE numerical.c get_atrem_corl1v3.c rdatreminfo.c)
#set (ATREM_F_CODE )
#####version 2 of Atrem - mix with fortran libraries and C code
#set (ATREM_C_CODE numerical.c  get_atrem_corl1v2.c rdatreminfo.c)
#set (ATREM_F_CODE atrem_app_refl_plus_gas_removal_l2v2.f90 atrem_bndprms.f atrem_tpvmr_init.f)
#####version 1 of Atrem - need fortran libraries and code
set (ATREM_C_CODE numerical.c get_atrem_corl1.c rdatreminfo.c)
set (ATREM_F_CODE atrem_app_refl_plus_gas_removal_l2.f90 atrem_bndprms.f atrem_tpvmr_init.f)

set(LIBS l2 nav bin osmi oc)

find_package(Levmar REQUIRED)
include_directories(${Levmar_INCLUDE_DIRS})
list(APPEND LIBS ${Levmar_LIBRARIES})

find_package(LAPACK REQUIRED)
include_directories(${LAPACK_INCLUDE_DIRS})
list(APPEND LIBS ${LAPACK_LIBRARIES})

find_package(GeoTIFF REQUIRED)
include_directories(${GeoTIFF_INCLUDE_DIRS})
list(APPEND LIBS ${GeoTIFF_LIBRARIES})

find_package(PugiXML REQUIRED)
include_directories(${PUGIXML_INCLUDE_DIRS})
list(APPEND LIBS ${PUGIXML_LIBRARIES})

find_package(Fann REQUIRED)
include_directories(${Fann_INCLUDE_DIRS})
list(APPEND LIBS ${Fann_LIBRARIES})

find_package(OpenJP2 REQUIRED)
include_directories(${OpenJP2_INCLUDE_DIRS})
list(APPEND LIBS ${OpenJP2_LIBRARIES})

if (BUILD_VIIRS_L1)
    find_package(LibViirsCal REQUIRED)
    if(LibViirsCal_INCLUDE_DIRS)
        include_directories(${LibViirsCal_INCLUDE_DIRS})
    endif()
    list(APPEND LIBS ${LibViirsCal_LIBRARIES})
    set (VIIRS_L1_SRCS calibrate_viirs.c l1b_viirs_nc.c l1_viirs_nc.c)
else (BUILD_VIIRS_L1)
    set (VIIRS_L1_SRCS calibrate_viirs.c l1b_viirs_nc.c l1_viirs_nc_stub.c)
endif (BUILD_VIIRS_L1)

find_package(NetCDF REQUIRED COMPONENTS CXX F77)
include_directories(${NETCDF_INCLUDE_DIRS})
list(APPEND LIBS ${NETCDF_LIBRARIES})

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})


if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    list(APPEND LIBS ${X11_LIBRARIES})
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

list(APPEND LIBS timeutils)
list(APPEND LIBS h5io)
list(APPEND LIBS filetype)
list(APPEND LIBS bilio)

set(L2GEN_COMMON_FILES
  l1_io.c
  get_f0.c
  scene_meta.c
  alloc_l1.c
  init_l1.c
  l1subpix.c
  filehandle_init.c
  l1_hdf_generic_read.c
  l1_nc_generic_read.c
  l1_generic_write.c
  l1_xcal_hdf.c
  xcal.c
  filter.c
  windex.c
  ftrim.c
  ll2vec.c
  setflags.c
  cloud_flag.c
  viirs_pxcvt.c

# fortran files to get rid of
  etbpsub.f
  hmf8.f
  lenstr.f
  intpos.f
  getglint.f
  airmass_avhrr.f
  simpsn.f
  raygetpol.f
  avhrrsub5h.f

# L1 readers
  brightness.c
  l1_mos_hdf.c
  l1_octs_hdf.c
  l1_czcs_hdf.c
  l1a_seawifs.c
  l1_aci_hdf.c
  l1_meris_N1.c
  smile.c
  l1_meris_CC.c
  l1_ocia.c
  l1_aviris.c
  l1_aviris_nc.c
  l1_olci.c
  l1_prism.c
  l1_viirs_h5.c
  l1_hico_h5.c
  l1_goci.c
  l1_sgli.c
  geom_per_band.c
  viirs_utls.c
  l1_hmodis_hdf.c
  ${VIIRS_L1_SRCS}
  l1a_osmi.c
  l1_ocm_hdf.c
  l1_ocm2_hdf.c
  l1_ocmdb_hdf.c
  l1_oli.c
  l1_l5tm.c
  l1_l7etm.c
  l1c_msi.cpp
  l1a_hawkeye.c
  l1b_misr.c
  l1b_oci.c
  l1b_ocis.c
  l1_seabass.cpp
)

set(L2GEN_PRODUCT_FILES
  prodgen.c
  prodlist.c
  get_l2prod_index.c
  get_chl.c
  get_es.c
  get_habs.c
  get_Kd.c
  get_ndvi.c
  get_smoke.c
  get_depth.c
  get_poc.c
  get_par.c
  get_toa_refl.c
  get_tricho.c
  get_npp.c
  photic_depth.c
  cdom_morel.c
  calc_par.c
  cdom_mannino.c
  par_utils.c
  ipar.c
  fluorescence.c
  get_rhos.c
  calcite.c
  amoeba.c
  aph.c
  gsm.c
  giop.c
  mgiop.c
  carder.c
  las_iop.c
  pml.c
  get_pml.c
  pml_iop_calculate.c
  pml_iop_config.c
  pml_iop_tables.c
  qaa.c
  get_qaa.c
  flags_iop.c
  niwa_iop.c
  get_niwa_iop.c
  sst.c
  sst_dsdi.c
  mumm.c
  get_bsi.c
  swim.c
  get_pft_hirata.c
  get_pft_uitz.c
  get_psd_ksm.c
  get_nitrate.c

#  myprod.c
)

set(L2GEN_ANCILLARY_FILES
  gas_trans.c
  seawater.c
  seawater_get.c
  water_spectra.c
  setanc.c
  b128_msk_get.c
  b128_wd_bit.c
  b128_box_num.c
  read_mask.c
  get_dem_height.c
  get_nc_height.c
  get_height.c
  ice_mask.c
  get_ice_frac.c
  sstref.c
  sssref.c
  elev.c
  anc_acq.c
  met_cvt.c
  ncio.c
  get_mld.cpp
  get_zno3.c
  smi_climatology.c
)

add_executable(l2gen
  main_l2gen.c
  msl12_input.c
  getl1rec.c
  loadl1.c
  read_pixel_anc_file.cpp
  cpl1rec.c
  target_io.c
  bioOptBandShift.c
  l2_generic.c
  alloc_l2.c
  alloc_target.c
  alloc_vcal.c
  convl12.c
  convl21.c
  atmocor1.c
  atmocor1_land.c
  whitecaps.c
  rayleigh.c
  polcor_hawkeye.cpp
  polcor.c
  water_vapor.c
  airmass.c
  aerosol.c
  atmocor2.c
  glint.c
  get_rhown_nir.c
  nlw_outband.c
  brdf.c
  bin_climatology.c
  aer_io.c
  alloc_aer.c
  vcal.c
  dtran_brdf.f
  turbid.c
  init_l2.c
  convert_band.c
  fuzzy_func_v3.c
  gammln.c
  gcf.c
  gser.c
  nrutil.c
  sprsax.c
  sprsin.c
  covariance_inversion.c
  lubksb.c
  ludcmp.c
  owt.c
  get_owmc.c
  lonlat2pixline.cpp
  virtual_constellation.c
  bioOptBandShift.c
  read_l3bin.cpp
  ${ATREM_C_CODE}
  ${ATREM_F_CODE}
  raman.c
  ${L2GEN_ANCILLARY_FILES}
  ${L2GEN_PRODUCT_FILES}
  ${L2GEN_COMMON_FILES}
)

add_executable(l3gen
  main_l3gen.cpp
  msl12_input.c
  getl1rec.c
  loadl1.c
  read_pixel_anc_file.cpp
  cpl1rec.c
  target_io.c
  bioOptBandShift.c
  l2_generic.c
  alloc_l2.c
  alloc_target.c
  alloc_vcal.c
  convl12.c
  convl21.c
  atmocor1.c
  atmocor1_land.c
  whitecaps.c
  rayleigh.c
  polcor_hawkeye.cpp
  polcor.c
  water_vapor.c
  airmass.c
  aerosol.c
  atmocor2.c
  glint.c
  get_rhown_nir.c
  nlw_outband.c
  brdf.c
  bin_climatology.c
  aer_io.c
  alloc_aer.c
  vcal.c
  dtran_brdf.f
  turbid.c
  init_l2.c
  convert_band.c
  fuzzy_func_v3.c
  gammln.c
  gcf.c
  gser.c
  nrutil.c
  sprsax.c
  sprsin.c
  covariance_inversion.c
  lubksb.c
  ludcmp.c
  owt.c
  get_owmc.c
  virtual_constellation.c
  bioOptBandShift.c
  read_l3bin.cpp
  ${ATREM_C_CODE}
  ${ATREM_F_CODE}
  raman.c
  ${L2GEN_ANCILLARY_FILES}
  ${L2GEN_PRODUCT_FILES}
  ${L2GEN_COMMON_FILES}
)


add_executable(vcalmerge
  main_vcalmerge.c
  calfile_utils.c
  msl12_input.c
  l2_generic.c
  loadl1.c
  read_pixel_anc_file.cpp
  atmocor1.c
  atmocor1_land.c
  whitecaps.c
  rayleigh.c
  polcor_hawkeye.cpp
  polcor.c
  water_vapor.c
  bioOptBandShift.c
  convl12.c
  convl21.c
  alloc_l2.c
  getl1rec.c
  alloc_aer.c
  alloc_target.c
  init_l2.c
  alloc_vcal.c
  vcal.c
  virtual_constellation.c
  bioOptBandShift.c
  lubksb.c
  ludcmp.c
  covariance_inversion.c
  nrutil.c
  sprsax.c
  sprsin.c
  gammln.c
  gcf.c
  gser.c
  fuzzy_func_v3.c
  owt.c
  get_owmc.c
  lonlat2pixline.cpp
  atmocor2.c
  brdf.c
  cpl1rec.c
  convert_band.c
  flags_iop.c
  dtran_brdf.f
  turbid.c
  glint.c
  get_rhown_nir.c
  aerosol.c
  nlw_outband.c
  airmass.c
  bin_climatology.c
  ${ATREM_C_CODE}
  ${ATREM_F_CODE}
  raman.c
  ${L2GEN_ANCILLARY_FILES}
  ${L2GEN_PRODUCT_FILES}
  ${L2GEN_COMMON_FILES}
)


add_executable(l1det2det
  main_l1det2det.c
  calfile_utils.c
  msl12_input.c
  l2_generic.c
  loadl1.c
  read_pixel_anc_file.cpp
  atmocor1.c
  atmocor1_land.c
  whitecaps.c
  rayleigh.c
  polcor_hawkeye.cpp
  polcor.c
  water_vapor.c
  bioOptBandShift.c
  convl12.c
  convl21.c
  alloc_l2.c
  read9km_mask.c
  getl1rec.c
  alloc_aer.c
  alloc_target.c
  init_l2.c
  alloc_vcal.c
  vcal.c
  virtual_constellation.c
  bioOptBandShift.c
  lubksb.c
  ludcmp.c
  covariance_inversion.c
  nrutil.c
  sprsax.c
  sprsin.c
  gammln.c
  gcf.c
  gser.c
  fuzzy_func_v3.c
  owt.c
  get_owmc.c
  lonlat2pixline.cpp
  atmocor2.c
  brdf.c
  cpl1rec.c
  convert_band.c
  flags_iop.c
  dtran_brdf.f
  turbid.c
  glint.c
  get_rhown_nir.c
  aerosol.c
  nlw_outband.c
  airmass.c
  bin_climatology.c
  ${ATREM_C_CODE}
  ${ATREM_F_CODE}
  raman.c
  ${L2GEN_ANCILLARY_FILES}
  ${L2GEN_PRODUCT_FILES}
  ${L2GEN_COMMON_FILES}
  )

add_executable(l1brsgen
  main_l1brsgen.c
  l1_imgscale.c
  msl12_input.c
  loadl1.c
  read_pixel_anc_file.cpp
  atmocor1.c
  atmocor1_land.c
  whitecaps.c
  rayleigh.c
  polcor_hawkeye.cpp
  polcor.c
  water_vapor.c
  get_rhos.c
  ${ATREM_C_CODE}
  ${ATREM_F_CODE}
  ${L2GEN_ANCILLARY_FILES}
  ${L2GEN_COMMON_FILES}
  )

add_executable(l1mapgen
  main_l1mapgen.c
  l1_imgscale.c
  msl12_input.c
  loadl1.c
  read_pixel_anc_file.cpp
  atmocor1.c
  atmocor1_land.c
  whitecaps.c
  rayleigh.c
  polcor_hawkeye.cpp
  polcor.c
  water_vapor.c
  mipolyutil.c
  get_rhos.c
  ${ATREM_C_CODE}
  ${ATREM_F_CODE}
  ${L2GEN_ANCILLARY_FILES}
  ${L2GEN_COMMON_FILES}
  )

add_executable(l1bgen_generic
  main_l1bgen.c
  msl12_input.c
  ${L2GEN_COMMON_FILES}
)

add_executable(l1info
  main_l1info.cpp
  msl12_input.c
  gringHelper.cpp
  ${L2GEN_COMMON_FILES}
  )

add_executable(lonlat2pixline
  main_lonlat2pixline.c
  msl12_input.c
  lonlat2pixline.cpp
  ${L2GEN_COMMON_FILES}
  )

add_executable(l2binmatch
  main_l2binmatch.cpp
  l2binmatch_input.cpp
  get_l2prod_index.c
  calfile_utils.c
  elev.c
 )

target_link_libraries(l2gen
  anc
  czcs
  meris
  goci
  oli
  ${LIBS}
  )

target_link_libraries(l3gen
  anc
  czcs
  osmi
  meris
  goci
  oli
  ${LIBS}
  )

target_link_libraries(l1bgen_generic
  czcs
  osmi
  meris
  goci
  oli
  ${LIBS}
  )

target_link_libraries(l1brsgen
  anc
  czcs
  osmi
  meris
  goci
  oli
  imageutils
  ${LIBS}
  )

target_link_libraries(l1mapgen
  anc
  czcs
  osmi
  meris
  goci
  oli
  ${PNG_LIBRARIES}
  ${LIBS}
  )

target_link_libraries(l1info
  czcs
  osmi
  meris
  goci
  oli
  ${LIBS}
  )

target_link_libraries(lonlat2pixline
  czcs
  osmi
  meris
  goci
  oli
  ${LIBS}
  )

target_link_libraries(vcalmerge
  anc
  czcs
  osmi
  meris
  goci
  oli
  ${LIBS}
  )

target_link_libraries(l2binmatch
  ${LIBS}
  )

target_link_libraries(l1det2det
  anc
  czcs
  osmi
  meris
  goci
  oli
  ${LIBS}
  )

#set (CMAKE_C_FLAGS_RELEASE "-O1")
# Use -Wpadded -Wpacked to warn about align-double. Use  '#pragma GCC diagnostic ignored "-Wpadded"' in file if you don't want to see messages
# -malign-double needed for accessing fortran common blocks from C structs

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -malign-double -mpreferred-stack-boundary=8")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -malign-double -mpreferred-stack-boundary=8")

install (TARGETS l2gen l3gen l1bgen_generic l1brsgen l1mapgen l1info lonlat2pixline vcalmerge l2binmatch l1det2det DESTINATION bin)
