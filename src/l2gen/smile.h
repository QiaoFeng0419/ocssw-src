#ifndef  SMILE_H
#define  SMILE_H

#include "l1_struc.h"

void smile_init(int num_bands, int num_detectors, const char* bandinfo_filename,
        float* detectorWLs, float* detectorE0s);
void radcor(l1str *l1rec, int32_t ip, int land);


#endif
