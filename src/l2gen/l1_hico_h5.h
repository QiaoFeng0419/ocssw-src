/*
 * l1_hico_h5.h
 */

#ifndef L1_HICO_H5_H_
#define L1_HICO_H5_H_

#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"

int closel1_hico_h5(filehandle *file);
int openl1_hico_h5(filehandle *file);
int readl1_hico_h5(filehandle *file, int32_t recnum, l1str *l1rec, int lonlat);

#endif /* L1_HICO_H5_H_ */

