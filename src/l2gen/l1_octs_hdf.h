#ifndef  _L1_OCTS_HDF_H
#define  _L1_OCTS_HDF_H

#include <stdint.h>
#include "l1_struc.h"
#include "filehandle.h"


int openl1_read_octs_hdf(filehandle *l1file);
int readl1_octs_hdf(filehandle *l1file, int32_t recnum, l1str *l1rec);
int closel1_octs_hdf(filehandle *l1file);
int navigation(int32_t fileID);
int CalcViewAngle(float lon1, float lat1, float pos[3],
        float usun[]);
int LeapCheck(int yr);

#endif

