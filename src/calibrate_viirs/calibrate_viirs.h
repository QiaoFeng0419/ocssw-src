/* 
 * File:   calibrate_viirs.h
 * Author: dshea
 *
 * Created on March 31, 2016, 9:28 AM
 */

#ifndef CALIBRATE_VIIRS_H
#define CALIBRATE_VIIRS_H

#include <clo.h>
#include <VcstParamsReader.h>

void calibrate_viirs_init_options(clo_optionList_t* list, const char* softwareVersion);
void calibrate_viirs_read_options(clo_optionList_t* list, int argc, char* argv[]);

#endif /* CALIBRATE_VIIRS_H */

