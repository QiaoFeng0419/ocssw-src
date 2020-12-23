/*************************************************************/
/*               Sentinel 2A MSI L1C PROCESSING              */
/*                    Jiaying He 06/09/2016                  */
/*************************************************************/

#ifndef L1C_MSI_H
#define L1C_MSI_H

#include "filehandle.h"
#include "l1_struc.h"

/* -------------------------------------------------------------------------- */
/* l1c_msi Header */
/* Define */

#ifdef __cplusplus
extern "C" {
#endif
    /* Definition of all l1c_msi.c functions */

    int openl1c_msi(filehandle *file);
    int readl1c_msi(filehandle *file, int recnum, l1str *l1rec, int lonlat);
    int closel1c_msi(filehandle *file);
    int readl1c_msi_lonlat(filehandle *file, int recnum, l1str *l1rec);

#ifdef __cplusplus
}
#endif



#endif