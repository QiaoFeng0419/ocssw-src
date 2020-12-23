/*
 * l1_sgli.h
 */

#ifndef L1_SGLI_H_
#define L1_SGLI_H_

#include <stdint.h>
#include "h5io.h"
#include "sgli.h"
#include "l1_struc.h"
#include "filehandle.h"

int closel1_sgli(filehandle *file);
int openl1_sgli(filehandle *file);
int readl1_sgli(filehandle *file, int32_t recnum, l1str *l1rec);
int sgli_file_ver(h5io_str *fid);
sgli_t* createPrivateData_sgli();

#endif /* L1_SGLI_H_ */
