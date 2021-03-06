#ifndef  _L1_HMODIS_HDF_H
#define  _L1_HMODIS_HDF_H

#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"

int closel1_hmodis_hdf();
int openl1_hmodis_hdf(filehandle *l1file);
int readl1_hmodis_hdf(filehandle *l1file, int32_t recnum, l1str *l1rec);
int readl1_lonlat_hmodis_hdf(filehandle *l1file, int32_t recnum, l1str *l1rec);

#endif
