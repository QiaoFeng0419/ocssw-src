#ifndef  _L1_OSMI_HDF_H
#define  _L1_OSMI_HDF_H

#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"

int closel1a_osmi(filehandle *l1file);
int openl1a_osmi(filehandle *l1file);
int readl1a_osmi(filehandle *l1file, int32_t recnum, l1str *l1rec);

#endif
