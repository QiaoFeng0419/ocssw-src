/*
 * l1_olci.h
 */

#ifndef L1_OLCI_H_
#define L1_OLCI_H_

#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"
#include "olci.h"

#ifdef __cplusplus
extern "C" {
#endif

int closel1_olci(filehandle *file);
int openl1_olci(filehandle *file);
int readl1_olci(filehandle *file, int32_t recnum, l1str *l1rec);
olci_t* createPrivateData_olci();

#ifdef __cplusplus
}
#endif

#endif /* L1_OLCI_H_ */

