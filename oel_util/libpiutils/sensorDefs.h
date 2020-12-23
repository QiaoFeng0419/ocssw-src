/*
 * sensorDefs.h
 *
 *  Created on: Oct 29, 2013
 *      Author: dshea
 */

#ifndef SENSOR_DEFS_H_
#define SENSOR_DEFS_H_

/* unique sensor IDs */
#define SEAWIFS        0
#define MOS            1
#define OCTS           2
#define AVHRR          3
#define OSMI           4
#define CZCS           5
#define MODIST         6
#define MODISA         7
#define OCM1           8
#define OCM2           9
#define MERIS         10
#define VIIRSN        11 /* VIIRS NPP */
#define OCRVC         12
#define HICO          13
#define GOCI          14
#define OLI           15
#define AQUARIUS      16
#define OCIA          17
#define AVIRIS        18
#define PRISM         19
#define OLCIS3A       20
#define SGLI          21
#define MSIS2A        22 /* MSI Sentinel-2A */
#define L5TM          23
#define L7ETMP        24
#define VIIRSJ1       25 /* VIIRS J1 */
#define MSIS2B        26 /* MSI Sentinel-2B */
#define HAWKEYE       27
#define MISR          28
#define OLCIS3B       29
#define OCI           30 
#define OCIS          31 /* OCI simulated data */
#define SENSOR_NUM    32

#define MAXWAVE_VIS  720
#define MINWAVE_IR  3000

/* subsensor IDs */
#define SEAWIFS_GAC    0
#define SEAWIFS_LAC    1
#define MODIS_TERRA    2
#define MODIS_AQUA     3
#define VIIRS_NPP      4
#define VIIRS_J1       5
#define MSI_S2A        6
#define MSI_S2B        7
#define OLCI_S3A       8
#define OLCI_S3B       9
#define SUBSENSOR_NUM  10

#endif /* SENSOR_DEFS_H_ */
