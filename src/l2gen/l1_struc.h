#ifndef _L1_STRUC_H
#define _L1_STRUC_H

#include <stdbool.h>
#include <stdint.h>

#include "l12_parms.h"
#include "input_struc.h"
#include "filehandle.h"

/* Notice: any changes to this structure may require modifications to the */

/* following routines: alloc_l1.c, cpl1rec.c, l1subpix.c.                 */

typedef struct geom_struc_def {
    float *senz;
    float *sena;
    float *csenz;
    float *solz;
    float *sola;
    float *csolz;
    float *delphi;
    float *scattang;
} geom_struc;

typedef struct anc_add_struc_def {
    int32_t nlvl;   /* number of profile levels */
    float *prof_temp;
    float *prof_rh;
    float *prof_height;
    float *prof_q;
} anc_struc;

// ancillary aerosol from GMAO MERRA model
typedef struct anc_aer_struc_def {
    float *black_carbon_ext;
    float *black_carbon_scat;
    float *dust_ext;
    float *dust_scat;
    float *sea_salt_ext;
    float *sea_salt_scat;
    float *sulphur_ext;
    float *sulphur_scat;
    float *organic_carbon_ext;
    float *organic_carbon_scat;
    float *total_aerosol_ext;
    float *total_aerosol_scat;
} anc_aer_struc;

typedef struct l1_struct {
    int32_t length; /* number of bytes allocated to data block */
    int32_t npix;

    int32_t iscan;
    int32_t detnum;
    int32_t mside;

    /* scan-time-specific data */
    double scantime;
    double fsol;

    bool is_l2; /**< Lt values are actually (above water?) reflectance; skip atmocor */

    /* scan attributes */

    float tilt;
    float alt; //altitude of sensor

    /* All parameters below are scan-length dependent */

    /* sensor band-pass-specific data */


    char *data; /* points to start of variable-length data block */

    int32_t *nobs;
    float *lon;
    float *lat;
    float *solz;
    float *sola;
    float *senz;
    float *sena;
    float *Lt;
    float *Lt_unc;

    float *Ltir;
    float *Bt;

    float *delphi;
    float *csolz;
    float *csenz;
    int32_t *pixnum;
    char *slot; /**< slot number                                */
    float *alpha;
    float *scattang;

    float *ws;
    float *wd;
    float *mw;
    float *zw;
    float *pr;
    float *oz;
    float *wv;
    float *rh;
    float *no2_tropo;
    float *no2_strat;
    float *no2_frac;
    float *sfcp;
    float *sfcrh;
    float *sfct;
    float *icefr;
    float *height;
    float *elev;

    float *ws_unc;
    float *wd_unc;
    float *mw_unc;
    float *zw_unc;
    float *pr_unc;
    float *oz_unc;
    float *wv_unc;
    float *rh_unc;
    float *no2_tropo_unc;
    float *no2_strat_unc;

    // TODO: can get rid of this.  Only used in setanc.c
    short *ancqc;


    short *ssttype; /* per pixel - reference type or climatology */

    int32_t *flags;
    char *mask; // this group of params is the flags expanded into a byte
    char *hilt;
    char *cloud;
    char *glint;
    char *land;
    char *swater;
    char *ice;
    char *solzmax;
    char *senzmax;
    char *stlight;
    char *absaer;
    char *navfail;
    char *navwarn;

    char *filter;

    float *t_h2o;
    float *t_o2;
    float *tg_sol;
    float *tg_sen;
    float *t_sol;
    float *t_sen;
    float *rhof;
    float *tLf;
    float *Lr;
    float *L_q;
    float *L_u;
    float *polcor;
    float *dpol;
    float *TLg;
    float *rhos;
    float *glint_coef;
    float *cloud_albedo;
    float *aerindex;
    float *sstref;
    float *sssref;
    float *sw_n;
    float *sw_a;
    float *sw_bb;
    float *sw_a_avg;
    float *sw_bb_avg;
    float *rho_cirrus;

    // TODO: move MERIS L1 to private_data pointer in l1rec
    /* for MERIS L1 */
    int32_t *pixdet; /* detector index of pixel */
    float *radcor; /* smile correction */


    float *Fo;

    // TODO: this needs to go into private_data pointer in filehandle
    /* for VIIRS unaggregated and superscan */
    int16_t scn_fmt; /* scan format of data, 0 std, else unaggregated */
    float margin_s; /* extra scan margin beyond actual samples */

    filehandle *l1file;

    // pointer to data needed by specific readers so far just meris
    void *private_data;

    // geometry per band
    geom_struc *geom_per_band;

    // added ancillary data, for CHIMAERA profiles, etc
    anc_struc *anc_add;

    // ancillary aerosol information from MERRA-2
    anc_aer_struc *anc_aerosol;

} l1str;
#endif




