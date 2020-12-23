#ifndef _L2_STRUC_H
#define _L2_STRUC_H

#include "input_struc.h"
#include "target_struc.h"
#include "filehandle.h"
#include "l1_struc.h"

typedef struct l2_struct {
    int32_t length;
    int32_t *bindx;

    char *data;
    int32_t *aermodmin;
    int32_t *aermodmax;
    float *aerratio2;
    int32_t *aermodmin2;
    int32_t *aermodmax2;
    float *aerratio;
    float *aerindex;
    float *eps; // NIR aerosol reflectance ratio (single scattering)
    float *taua; // aerosol optical thickness
    float *La; // aerosol radiance
    float *Lw; // water-leaving radiance
    float *nLw; // normalized water-leaving radiance
    float *nLw_unc;
    float *Rrs; //Remote sensing reflectance
    float *Rrs_unc;
    float *brdf; //bi-direction reflectance function
    float *a; //absoprtion coefficient
    float *bb; //backscattering coefficient
    float *chl;
    float *sst;
    int32_t *num_iter;

    tgstr *tgrec;
    l1str *l1rec;

    float *Rrs_raman;

} l2str;

#endif
