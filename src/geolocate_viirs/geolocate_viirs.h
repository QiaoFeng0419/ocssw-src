/* 
 * File:   calibrate_viirs.h
 * Author: dshea
 *
 * Created on March 31, 2016, 9:28 AM
 */

#ifndef GEOLOCATE_VIIRS_H
#define GEOLOCATE_VIIRS_H

#include <clo.h>
#include <VcstParamsReader.h>

void geolocate_viirs_init_options(clo_optionList_t* list, const char* softwareVersion);
void geolocate_viirs_read_options(clo_optionList_t* list, int argc, char* argv[]);

#endif /* GEOLOCATE_VIIRS_H */

