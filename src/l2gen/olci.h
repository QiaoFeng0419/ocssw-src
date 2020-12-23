/*
 * olci.h
 *
 *  Created on: Jul 29, 2015
 *      Author: rhealy
 */

#ifndef SRC_L2GEN_OLCI_H_
#define SRC_L2GEN_OLCI_H_

#define MAXOLCI_RADFILES 21

#ifdef __cplusplus
extern "C" {
#endif

typedef struct olci_struct {
    char *olci_radfiles[MAXOLCI_RADFILES + 1];
    char *geoCoordinatesFile, *tieGeoCoordinatesFile, *tieGeometriesFile, *instrumentFile, *time_coordinatesFile, *tieMeteoFile;
    char olci_varname[MAXOLCI_RADFILES][20];
} olci_t;

#ifdef __cplusplus
}
#endif

#endif /* SRC_L2GEN_OLCI_H_ */
