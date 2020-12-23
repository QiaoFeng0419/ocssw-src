#ifndef L1A_HAWKEYE_H
#define L1A_HAWKEYE_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"
    
// Call sequence
// openl1a_hawkeye      > qc_hawkeye_CCD_T      > nan_wmean                     // once
// readl1a_hawkeye      > read_cal_hawkeye                                      // once
// readl1a_hawkeye      > interp_hawkeye_CCD_T  > prep_for_interp_double        // per scan
// readl1a_hawkeye      > calibrate_hawkeye     > prep_for_interp_double        // per scan

// Open, read, close
int openl1a_hawkeye(filehandle *file);
int readl1a_hawkeye(filehandle *file, int32_t recnum, l1str *l1rec);
int closel1a_hawkeye(filehandle *file);

    
#ifdef __cplusplus
}
#endif
    
#endif
