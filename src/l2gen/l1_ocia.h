#ifndef  _L1_OCIA_H
#define  _L1_OCIA_H

#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"

int closel1_ocia(filehandle *l1file);
int openl1_ocia(filehandle *l1file);
int readl1_ocia(filehandle *l1file, int32 recnum, l1str *l1rec);

#endif
