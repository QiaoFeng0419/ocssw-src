#ifndef  _L1_VIIRS_NC_H
#define  _L1_VIIRS_NC_H

#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"

int closel1_viirs_nc(filehandle *l1file);
int openl1_viirs_nc(filehandle *l1file);
int readl1_viirs_nc(filehandle *l1file, int32_t recnum, l1str *l1rec);

#endif
