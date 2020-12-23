/**************************************************************************
 *
 * NAME: Geolocate_Viirs_Driver.cpp
 *
 * DESCRIPTION: Source file for calibration wrapper application designed to read in L1A file, perform
 * calibration, and write out results.
 *
 *
 * REFERENCES:
 *
 * REVISION HISTORY:
 *   DATE:       PR#         AUTHOR         Description
 *   --------    ------      --------       -----------------
 *   01-29-2015				S. Anderson
 *
 * NOTES (MISCELLANEOUS) SECTION:
 * none
 *
 **************************************************************************/
#include "geolocate_viirs.h"

#include <VcstViirsGeo.h>
#include <VcstParamsReader.h>
#include <VcstCmnConsts.h>
#include <VcstLogger.h>
#include <libgen.h>
#include <genutils.h>

//-----------------------------------------------------------------------------
//
// Main Function
//
//-----------------------------------------------------------------------------

int main(int argc, char* argv[]) {

    int status = 0;

    clo_optionList_t* list = clo_createList();
    clo_setEnablePositionOptions(1);

    //char softwareVersion[200];
    //sprintf(softwareVersion, "%d.%d.%d-r%d", L3MAPGEN_VERSION_MAJOR, L3MAPGEN_VERSION_MINOR,
    //        L3MAPGEN_VERSION_PATCH_LEVEL, SVN_REVISION);
    geolocate_viirs_init_options(list, "3.1");
    if (argc == 1) {
        clo_printUsage(list);
        exit(1);
    }

    geolocate_viirs_read_options(list, argc, argv);

    if (!clo_isSet(list, "geofile_img")
            && !clo_isSet(list, "geofile_mod")
            && !clo_isSet(list, "geofile_dnb")) {
        std::cerr << "-E- At least one output geofile (geofile_img, geofile_mod or geofile_dnb)\n    should be specified." << endl;
        exit(EXIT_FAILURE);
    }
    if (clo_isSet(list, "geofile_dnb") && !clo_isSet(list, "geofile_img")) {
        std::cerr << "-E- Sorry, you must also output a geofile_img to make a geofile_dnb." << endl;
        exit(EXIT_FAILURE);
    }

    if (clo_getBool(list, "verbose"))
        want_verbose = 1;
    else
        want_verbose = 0;

    std::cerr << "\nMain:: Geolocating L1A " << VL1_get_option("ifile") << std::endl;

    VcstViirsGeo* vcstGeo = new VcstViirsGeo();
    vcstGeo->history_ = VL1_get_history(argc, argv);
    vcstGeo->source_files_ = VL1_get_source();

    status = vcstGeo->initialize();
    if (status != VCST_SUCCESS) {
        std::cerr << "Main::Geolocation initialization failure" << std::endl;
        exit(EXIT_FAILURE);
    }

    status = vcstGeo->geolocate();
    if (status != VCST_SUCCESS) {
        std::cerr << "Main:: Geolocation processing failure for " << argv[1] << std::endl;
        exit(EXIT_FAILURE);
    }

    delete vcstGeo;

    std::cerr << "Geolocation completed.\n" << std::endl;

    return (status);
}

