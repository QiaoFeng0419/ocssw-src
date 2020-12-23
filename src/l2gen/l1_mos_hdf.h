#ifndef  _L1_MOS_HDF_H
#define  _L1_MOS_HDF_H

#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"

int openl1_read_mos_hdf(filehandle *l1file);
int readl1_mos_hdf(filehandle *l1file, int32_t recnum, l1str *l1rec);
int closel1_mos_hdf(filehandle *l1file);

#endif
