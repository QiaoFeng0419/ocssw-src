#ifndef  _L1_XCAL_HDF_H
#define  _L1_XCAL_HDF_H

#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"

int closel1_xcal_hdf(filehandle *l1file);
int openl1_xcal_hdf(filehandle *l1file);
int readl1_xcal_hdf(filehandle *l1file, int32_t recnum, l1str *l1rec);

#endif
