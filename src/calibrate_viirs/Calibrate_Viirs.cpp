/**************************************************************************
 *
 * NAME: Calibrate_Viirs_Driver.cpp
 *
 * DESCRIPTION: Application to read in L1A file, perform calibration, and
 * write out results in short format.
 *
 *
 * REFERENCES:
 *
 * REVISION HISTORY:
 *   DATE:       PR#         AUTHOR         Description
 *   --------    ------      --------       -----------------
 *   12-01-2014				S. Anderson
 *
 * NOTES (MISCELLANEOUS) SECTION:
 * none
 *
 **************************************************************************/
#include "calibrate_viirs.h"

#include <iostream>
#include <libgen.h>
#include <VcstViirsCal.h>
#include <VcstParamsReader.h>
#include <VcstCmnConsts.h>
#include <genutils.h>

//-----------------------------------------------------------------------------
//
// Main Function
//
//-----------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    int status = VCST_SUCCESS;

    clo_optionList_t* list = clo_createList();
    clo_setEnablePositionOptions(1);

    //char softwareVersion[200];
    //sprintf(softwareVersion, "%d.%d.%d-r%d", L3MAPGEN_VERSION_MAJOR,
    //L3MAPGEN_VERSION_MINOR, L3MAPGEN_VERSION_PATCH_LEVEL, SVN_REVISION);
    calibrate_viirs_init_options(list, "3.1");
    if (argc == 1) {
        clo_printUsage(list);
        exit(1);
    }
    calibrate_viirs_read_options(list, argc, argv);

    if (clo_getBool(list, "verbose"))
        want_verbose = 1;
    else
        want_verbose = 0;

    VcstViirsCal* vcstCal = new VcstViirsCal();
    vcstCal->history_ = VL1_get_history(argc, argv);
    vcstCal->source_files_ = VL1_get_source();

    bool bRad = clo_getBool(list, "radiance");
    bool bFilt = clo_getBool(list, "filters");
    bool bLunar = clo_getBool(list, "lunar");
    bool bCdg = !VL1_get_group(VIIRS_L1B_CDG).empty();

    status = vcstCal->initialize(bRad, bFilt, bCdg, bLunar);
    if (status != VCST_SUCCESS) {
        std::cerr << "Main:: Calibration initialization failure" << std::endl;
        exit(EXIT_FAILURE);
    }

    status = vcstCal->calibrate(ALL_BANDS);
    if (status != VCST_SUCCESS) {
        std::cerr << "Main:: Calibration processing failure for " << argv[1]
                << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cerr << "Main:: Calibration complete for " << VL1_get_option("ifile")
            << std::endl;

    delete vcstCal;

    return (status);
}

