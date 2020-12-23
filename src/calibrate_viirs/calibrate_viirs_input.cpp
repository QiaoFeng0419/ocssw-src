#include "calibrate_viirs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <genutils.h>
#include <sensorInfo.h>
#include <VcstGetSensorId.h>

static const char *help_optionKeys[] = {
    "help",
    "version",
    "dump_options",
    "dump_options_paramfile",
    "dump_options_xmlfile",
    "par",
    "ifile",
    "l1bfile_img",
    "l1bfile_mod",
    "l1bfile_dnb",
    "l1bfile_cdg",
    "static_lut_file",
    "rsb_dynamic_lut_file ",
    "dnb_dynamic_lut_file",
    "straylight_lut_file",
    "cmn_lut_file",
    "polar_wander_file",
    "leapsec_file",
    "geo_lut_file",
    "terrain_path",
    "verbose",
    "pversion",
    "pcf_help",
    NULL
};

/** add all of the accepted command line options to list */
void calibrate_viirs_init_options(clo_optionList_t* list, const char* softwareVersion) {
    char tmpStr[2048];
    clo_option_t* option;
    int i;

    clo_setVersion2("calibrate_viirs", softwareVersion);

    sprintf(tmpStr, "Usage: calibrate_viirs argument-list\n\n");

    strcat(tmpStr, "  This program takes a VIIRS L1A file and outputs an L1B file.\n\n");

    strcat(tmpStr, "  The argument-list is a set of keyword=value pairs. The arguments can\n");
    strcat(tmpStr, "  be specified on the commandline, or put into a parameter file, or the\n");
    strcat(tmpStr, "  two methods can be used together, with commandline over-riding.\n\n");
    strcat(tmpStr, "The list of valid keywords follows:\n");
    clo_setHelpStr(tmpStr);

    // add the parameters common to all VIIRS programs
    VL1_add_options(list);

    option = clo_addOption(list, "verbose", CLO_TYPE_BOOL, "false", "turn on verbose output");
    clo_addOptionAlias(option, "v");

    clo_setSelectOptionKeys((char**) help_optionKeys);
}

/* 
   Read the command line option and all of the default parameter files.

   This is the order for loading the options:
    - load the main program defaults file
    - load the command line (including specified par files)
    - re-load the command line disabling file descending so command
       line arguments will over ride

 */
void calibrate_viirs_read_options(clo_optionList_t* list, int argc, char* argv[]) {
    char *dataRoot;
    char tmpStr[FILENAME_MAX];
    int i;

    assert(list);

    if ((dataRoot = getenv("OCDATAROOT")) == NULL) {
        fprintf(stderr, "-E- OCDATAROOT environment variable is not defined.\n");
        exit(EXIT_FAILURE);
    }

    // disable the dump option until we have read all of the default files
    clo_setEnableDumpOptions(0);

    // load program defaults
    sprintf(tmpStr, "%s/common/calibrate_viirs_defaults.par", dataRoot);
    clo_readFile(list, tmpStr);

    // read all arguments including descending par files
    clo_readArgs(list, argc, argv);

    // handle PCF file on command line
    if (clo_getPositionNumOptions(list) > 1) {
        fprintf(stderr, "-E- Too many command line parameters.  Only one par file name allowed.\n");
        exit(EXIT_FAILURE);
    }

    if (clo_getPositionNumOptions(list) == 1) {
        clo_readFile(list, clo_getPositionString(list, 0));
    }

    VL1_copy_options();

    if (clo_getPositionNumOptions(list) != 1) {
        // get sensor directory for this ifile
        int sensorId = VcstGetSensorId(clo_getString(list, "ifile"));
        const char* sensorDir = sensorId2SensorDir(sensorId);
        if (sensorDir == NULL) {
            fprintf(stderr, "-E- %s:%d - Could not find sensor directory for file %s\n",
                    __FILE__, __LINE__, clo_getString(list, "ifile"));
            exit(EXIT_FAILURE);
        }

        const char* subsensorDir = subsensorId2SubsensorDir(sensorId2SubsensorId(sensorId));
        if (subsensorDir == NULL) {
            fprintf(stderr, "-E- %s:%d - Could not find subsensor directory for sensor %s\n",
                    __FILE__, __LINE__, sensorId2SensorName(sensorId));
            exit(EXIT_FAILURE);
        }

        // load the subsensor specific defaults file
        sprintf(tmpStr, "%s/%s/%s/instrument_defaults.par", dataRoot, sensorDir, subsensorDir);
        clo_readFile(list, tmpStr);
    }
    // enable the dump option
    clo_setEnableDumpOptions(1);

    clo_readArgs(list, argc, argv);

}
