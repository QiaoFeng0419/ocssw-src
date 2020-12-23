/*---------------------------------------------------------------------*/
/* get_ndvi.c - vegetation index classification for MSl12.             */
/*                                                                     */
/* Inputs:                                                             */
/*     l2rec - level-2 structure containing one complete scan after    */
/*             atmospheric correction.                                 */
/* Outputs:                                                            */
/*     ndvi  - vegetation index for land, 1 value per pixel.           */
/*                                                                     */
/* Written by: Bryan Franz, SAIC-GSC, February 2000                    */
/*                                                                     */
/*---------------------------------------------------------------------*/

#include <stdlib.h>
#include <math.h>
#include "l12_proto.h"

static float undef = -2.0;
static int ibblue = -1;
static int ibred = -1;
static int ibnir = -1;

//static float minval = -2.0;
//static float maxval =  2.0;

// as per C. Tucker (11/2014), no cut-off at -2
static float minval = -1000.0;
static float maxval = 1000.0;
static int32_t mask = LAND;

void get_ndvi(l1str *l1rec, float ndvi[]) {
    int32_t ip, ipb;
    float red;
    float nir;

    if (ibred < 0 || ibnir < 0) {
        printf("NDVI requires bands near 670 and 865nm\n");
        exit(1);
    }

    for (ip = 0; ip < l1rec->npix; ip++) {
        ndvi[ip] = undef;

        ipb = l1rec->l1file->nbands*ip;
        red = l1rec->rhos[ipb + ibred];
        nir = l1rec->rhos[ipb + ibnir];

        if ((l1rec->height[ip] == 0 && l1rec->elev[ip] < -500) ||
                (l1rec->flags[ip] & mask) == 0 ||
                red <= 0.0 || nir <= 0.0 ||
                red > 1.0 || nir > 1.0) {
            l1rec->flags[ip] |= PRODFAIL;
            continue;
        } else {
            ndvi[ip] = (nir - red)
                    / (nir + red);

            ndvi[ip] = MIN(MAX(ndvi[ip], minval), maxval);
        }
    }
}

void get_evi(l1str *l1rec, float evi[]) {
    static float L = 1.0, c1 = 6.0, c2 = 7.5;

    int32_t ip, ipb;
    float blu;
    float red;
    float nir;
    double val;

    if (ibblue < 0 || ibred < 0 || ibnir < 0) {
        printf("EVI requires bands near 412, 670, and 865nm\n");
        exit(1);
    }

    for (ip = 0; ip < l1rec->npix; ip++) {
        evi[ip] = undef;

        ipb = l1rec->l1file->nbands*ip;
        blu = l1rec->rhos[ipb + ibblue];
        red = l1rec->rhos[ipb + ibred];
        nir = l1rec->rhos[ipb + ibnir];

        if ((l1rec->height[ip] == 0 && l1rec->elev[ip] < -500) ||
                (l1rec->flags[ip] & mask) == 0 ||
                red <= 0.0 || nir <= 0.0 ||
                red > 1.0 || nir > 1.0) {
            l1rec->flags[ip] |= PRODFAIL;
            continue;

        } else {

            if (blu > 0.0 && (blu <= red || red <= nir)) {

                /* Most cases - EVI formula */

                if ((val = L + nir + c1 * red - c2 * blu) == 0)
                    continue;
                else
                    evi[ip] = 2.5 * (nir - red) / val;

            } else {

                /* Backup - SAVI formula */

                if ((val = 0.5 + nir + red) == 0)
                    continue;
                else
                    evi[ip] = 1.5 * (nir - red) / val;
            }
            evi[ip] = MIN(MAX(evi[ip], minval), maxval);

        }
    }
}

// From Compton Tucker 07/30/2016
//EVI3=2.5*(nir-red)/(nir+6*red-7.5*blue+1)                 
//EVI2=2.5*(NIR-Red)/(NIR+2.4*Red+1)

void get_evi2(l1str *l1rec, float evi2[]) {
    int32_t ip, ipb;
    float red;
    float nir;

    if (ibred < 0 || ibnir < 0) {
        printf("EVI2 requires bands near 670, and 865nm\n");
        exit(1);
    }

    for (ip = 0; ip < l1rec->npix; ip++) {
        evi2[ip] = undef;

        ipb = l1rec->l1file->nbands*ip;
        red = l1rec->rhos[ipb + ibred];
        nir = l1rec->rhos[ipb + ibnir];

        if ((l1rec->height[ip] == 0 && l1rec->elev[ip] < -500) ||
                (l1rec->flags[ip] & mask) == 0 ||
                red <= 0.0 || nir <= 0.0 ||
                red > 1.0 || nir > 1.0) {
            l1rec->flags[ip] |= PRODFAIL;
            continue;

        } else {

            if (red <= nir) {
                evi2[ip] = 2.5 * (nir - red) / (nir + 2.4 * red + 1);
            }
            evi2[ip] = MIN(MAX(evi2[ip], minval), maxval);
        }
    }
}

void get_evi3(l1str *l1rec, float evi3[]) {
    static float L = 1.0, c1 = 6.0, c2 = 7.5;

    int32_t ip, ipb;
    float blu;
    float red;
    float nir;
    double val;

    if (ibblue < 0 || ibred < 0 || ibnir < 0) {
        printf("EVI requires bands near 412, 670, and 865nm\n");
        exit(1);
    }

    for (ip = 0; ip < l1rec->npix; ip++) {
        evi3[ip] = undef;

        ipb = l1rec->l1file->nbands*ip;
        blu = l1rec->rhos[ipb + ibblue];
        red = l1rec->rhos[ipb + ibred];
        nir = l1rec->rhos[ipb + ibnir];

        if ((l1rec->height[ip] == 0 && l1rec->elev[ip] < -500) ||
                (l1rec->flags[ip] & mask) == 0 ||
                red <= 0.0 || nir <= 0.0 ||
                red > 1.0 || nir > 1.0) {
            l1rec->flags[ip] |= PRODFAIL;
            continue;

        } else {


            if (blu > 0.0 && (blu <= red || red <= nir)) {

                /* Most cases - EVI formula */

                if ((val = L + nir + c1 * red - c2 * blu) == 0)
                    continue;
                else
                    evi3[ip] = 2.5 * (nir - red) / val;

            }
            evi3[ip] = MIN(MAX(evi3[ip], minval), maxval);
        }
    }
}

void get_ndvi_evi(l1str *l1rec, int prodnum, float prod[]) {

    static int firstCall = 1;
    if (firstCall) {
        ibblue = bindex_get(412);
        ibred = bindex_get(670);
        ibnir = bindex_get(865);

        if (l1rec->l1file->sensorID == MODISA ||
                l1rec->l1file->sensorID == MODIST) {
            ibred = bindex_get(645);
            ibnir = bindex_get(859);
        }
    }

    switch (prodnum) {
    case CAT_ndvi:
        get_ndvi(l1rec, prod);
        break;
    case CAT_evi:
        get_evi(l1rec, prod);
        break;
    case CAT_evi2:
        get_evi2(l1rec, prod);
        break;
    case CAT_evi3:
        get_evi3(l1rec, prod);
        break;
    default:
        printf("Error: %s : Unknown product specifier: %d\n", __FILE__, prodnum);
        exit(FATAL_ERROR);
        break;
    }

}