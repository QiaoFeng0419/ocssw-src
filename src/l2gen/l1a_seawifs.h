#ifndef L1A_SEAWIFS_H
#define L1A_SEAWIFS_H

#include <stdint.h>

int openl1a_seawifs(filehandle *file);
int readl1a_seawifs(filehandle *file, int32_t recnum, l1str *l1rec);
int readl1a_lonlat_seawifs(filehandle *file, int32_t recnum, l1str *l1rec);
int closel1a_seawifs(filehandle *file);


#endif
