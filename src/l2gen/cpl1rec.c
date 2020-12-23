#include "l12_proto.h"
#include "anc_acq.h"

/*  for SGLI, copy pointer to geom_per_band */

void cpl1rec(l1str *dest, l1str *src) {
    dest->npix = src->npix;
    dest->length = src->length;
    dest->iscan = src->iscan;
    dest->detnum = src->detnum;
    dest->mside = src->mside;
    dest->scantime = src->scantime;
    dest->margin_s = src->margin_s;
    dest->fsol = src->fsol;
    dest->tilt = src->tilt;
    dest->alt = src->alt;
    dest->is_l2 = src->is_l2;

    dest->scn_fmt = src->scn_fmt;
    dest->margin_s = src->margin_s;

    memcpy(dest->data, src->data, dest->length);

    if (src->geom_per_band) {
        int32_t nbands = src->l1file->nbands;
        int32_t npix = src->npix;

        if (!dest->geom_per_band)
            init_geom_per_band(dest);

        memcpy(dest->geom_per_band->senz, src->geom_per_band->senz, npix * nbands * sizeof (float));
        memcpy(dest->geom_per_band->sena, src->geom_per_band->sena, npix * nbands * sizeof (float));
        memcpy(dest->geom_per_band->csenz, src->geom_per_band->csenz, npix * nbands * sizeof (float));
        memcpy(dest->geom_per_band->solz, src->geom_per_band->solz, npix * nbands * sizeof (float));
        memcpy(dest->geom_per_band->sola, src->geom_per_band->sola, npix * nbands * sizeof (float));
        memcpy(dest->geom_per_band->csolz, src->geom_per_band->csolz, npix * nbands * sizeof (float));
        memcpy(dest->geom_per_band->scattang, src->geom_per_band->scattang, npix * nbands * sizeof (float));
        memcpy(dest->geom_per_band->delphi, src->geom_per_band->delphi, npix * nbands * sizeof (float));
    } else {
        if (dest->geom_per_band) {
            printf("-E- cpl1rec - Should never copy from empty to allocated geom_per_band\n");
            exit(EXIT_FAILURE);
        }
    }
  /* for the added ancillary data */
    if (src->anc_add) {
        int32_t npix = src->l1file->npix;
        int32_t nlvl = src->anc_add->nlvl;

        if (!dest->anc_add)
            init_anc_add(dest);

        memcpy(dest->anc_add->prof_temp, src->anc_add->prof_temp, 
          npix * nlvl * sizeof (float));
        memcpy(dest->anc_add->prof_rh, src->anc_add->prof_rh, 
          npix * nlvl * sizeof (float));
        memcpy(dest->anc_add->prof_height, src->anc_add->prof_height, 
          npix * nlvl * sizeof (float));
        memcpy(dest->anc_add->prof_q, src->anc_add->prof_q, 
          npix * nlvl * sizeof (float));
    } else {
        if (dest->anc_add) {
            printf("-E- cpl1rec - Should never copy from empty to allocated anc_add\n");
            exit(EXIT_FAILURE);
        }
    }
    /* for the ancillary aerosol data */
    if (src->anc_aerosol) {
        int32_t npix = src->l1file->npix;

        if (!dest->anc_aerosol)
            init_anc_aerosol(dest);

        memcpy(dest->anc_aerosol->black_carbon_ext, src->anc_aerosol->black_carbon_ext,
          npix * sizeof (float));
        memcpy(dest->anc_aerosol->black_carbon_scat, src->anc_aerosol->black_carbon_scat,
          npix * sizeof (float));
        memcpy(dest->anc_aerosol->dust_ext, src->anc_aerosol->dust_ext,
          npix * sizeof (float));
        memcpy(dest->anc_aerosol->dust_scat, src->anc_aerosol->dust_scat,
          npix * sizeof (float));
        memcpy(dest->anc_aerosol->organic_carbon_ext, src->anc_aerosol->organic_carbon_ext,
          npix * sizeof (float));
        memcpy(dest->anc_aerosol->organic_carbon_scat, src->anc_aerosol->organic_carbon_scat,
          npix * sizeof (float));
        memcpy(dest->anc_aerosol->sea_salt_ext, src->anc_aerosol->sea_salt_ext,
          npix * sizeof (float));
        memcpy(dest->anc_aerosol->sea_salt_scat, src->anc_aerosol->sea_salt_scat,
          npix * sizeof (float));
        memcpy(dest->anc_aerosol->sulphur_ext, src->anc_aerosol->sulphur_ext,
          npix * sizeof (float));
        memcpy(dest->anc_aerosol->sulphur_scat, src->anc_aerosol->sulphur_scat,
          npix * sizeof (float));
        memcpy(dest->anc_aerosol->total_aerosol_ext, src->anc_aerosol->total_aerosol_ext,
          npix * sizeof (float));
        memcpy(dest->anc_aerosol->total_aerosol_scat, src->anc_aerosol->total_aerosol_scat,
          npix * sizeof (float));

    } else {
        if (dest->anc_aerosol) {
            printf("-E- cpl1rec - Should never copy from empty to allocated anc_add\n");
            exit(EXIT_FAILURE);
        }
    }
}
