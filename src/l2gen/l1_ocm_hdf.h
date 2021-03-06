#ifndef  _L1_OCM_HDF_H
#define  _L1_OCM_HDF_H

#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"

int closel1_ocm_hdf(filehandle *l1file);
int openl1_ocm_hdf(filehandle *l1file);
int readl1_ocm_hdf(filehandle *l1file, int32_t recnum, l1str *l1rec);

#endif
