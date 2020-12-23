#ifndef L1B_OCIS_H
#define L1B_OCIS_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"

// Open, read, close
int openl1b_ocis(filehandle *file);
int readl1b_ocis(filehandle *file, int32_t recnum, l1str *l1rec);
int closel1b_ocis(filehandle *file);

    
#ifdef __cplusplus
}
#endif
    
#endif