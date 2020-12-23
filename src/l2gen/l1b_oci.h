#ifndef L1B_OCI_H
#define L1B_OCI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"


// Open, read, close
int openl1b_oci(filehandle *file);
int readl1b_oci(filehandle *file, int32_t recnum, l1str *l1rec);
int closel1b_oci(filehandle *file);
    
#ifdef __cplusplus
}
#endif
    
#endif