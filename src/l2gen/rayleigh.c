#include "l12_proto.h"

/* ----------------------------------------------------------------- */
/* ray_press_wang() - Wang (2004) pressure correction                */

/* ----------------------------------------------------------------- */
float ray_press_wang(float taur, float airmass, float pr) {
    static float p0 = STDPR;
    float x = (-(0.6543 - 1.608 * taur) + (0.8192 - 1.2541 * taur) * log(airmass)) * taur*airmass;
    return ( (1.0 - exp(-x * pr / p0)) / (1.0 - exp(-x)));
}

/* -------------------------------------------------------------------------------
 * rayleigh() - compute Rayleigh radiances with polarization, per band.
 *
 * Description:
 *
 *   This computes the Rayleigh scattering radiance (including 
 *   polarization) for a given solar, viewing, and relative azimuthal
 *   angles, as well as the wind speed for the ocean surface roughness
 *   effects.  This is a modification of M. Wangs rayleigh_i subroutine, 
 *   incorporating the code to read the formatted hdf files.
 *
 * Inputs:
 *
 *   l1rec     l1 data line
 *   ip        pixel # to process
 *
 * Outputs:
 *
 *   l1rec     l1 record with rayleigh radiances
 *
 * Hacked by:  B. Franz, December 2002.
 * C Version:  B. Franz, November 2004
 * W. Robinson, SAIC 20 May 2015  prevent solar zonith table index from 
 *               exceeding the table size
 * W. Robinson, SAIC 7 Feb 2017  Adapt for use of band-dependent viewing 
 *              geometry and deal with l1rec itself
 *
 * ------------------------------------------------------------------------------- */

void rayleigh(l1str *l1rec, int32_t ip) {

#define NSOL   45
#define NSEN   41
#define NORDER  3
#define NWIND   8

    static int firstCall = 1;

    static float *ray_tau;
    static float *ray_dep;
    static float ray_sen [NSEN];
    static float ray_sol [NSOL];
    static float ray_sigma[NWIND];
    typedef float ray_array[NWIND][NSOL][NORDER][NSEN];
    static ray_array *ray_for_i;
    static ray_array *ray_for_q;
    static ray_array *ray_for_u;

    static int nsol = NSOL;
    static int nsen = NSEN;
    static int norder = NORDER;
    static int nwind = NWIND;

    /* indices */
    int m;
    int iwind;
    int isigma;
    int isigma1;
    int isigma2;
    int isol1;
    int isol2;
    int isen1;
    int isen2;

    char *tmp_str;
    char name [H4_MAX_NC_NAME] = "";
    char sdsname[H4_MAX_NC_NAME] = "";
    char file [FILENAME_MAX] = "";
    char path [FILENAME_MAX] = "";
    char wavestr[10] = "";
    char sensorstr[32] = "";

    int32 sd_id;
    int32 sds_id;
    int32 status;
    int32 rank;
    int32 nt;
    int32 dims[H4_MAX_VAR_DIMS];
    int32 nattrs;
    int32 start[4] = {0, 0, 0, 0};

    /* working variables */
    float sigma_m;
    float p, q, h;
    float f00, f10, f01, f11;
    float cosd_phi[NORDER];
    float sind_phi[NORDER];
    float ray_i_sig[2];
    float ray_q_sig[2];
    float ray_u_sig[2];
    float ray_i, ray_q, ray_u;
    float airmass, fac;

    float *r_solz;
    float *r_senz;
    float *r_phi;
    float *r_csolz;
    float *r_csenz;

    int32_t iw, ipw, gmult, ix;

    int32_t sensorID = l1rec->l1file->sensorID;
    int32_t evalmask = input->evalmask;
    int32_t nwave = l1rec->l1file->nbands;
    int pol_opt = input->pol_opt;
    float *taur = l1rec->l1file->Tau_r;
    float *Fo = l1rec->Fo;
    float pr = l1rec->pr[ip];
    float ws = l1rec->ws[ip];
    /*
     *  Note that if band-dependent viewing geometry is enabled for this 
     *  instrument/run (geom_per_band != NULL), use that value in computing 
     *  the Rayleigh radiance.  Also, set the index to the angle for that 
     *  band - ix, based on whether the angles are band-dependent
     */
    ipw = ip * nwave;
    if (l1rec->geom_per_band != NULL) {
        r_solz = &l1rec->geom_per_band->solz[ipw];
        r_senz = &l1rec->geom_per_band->senz[ipw];
        r_phi = &l1rec->geom_per_band->delphi[ipw];
        r_csolz = &l1rec->geom_per_band->csolz[ipw];
        r_csenz = &l1rec->geom_per_band->csenz[ipw];
    } else {
        r_solz = &l1rec->solz[ip];
        r_senz = &l1rec->senz[ip];
        r_phi = &l1rec->delphi[ip];
        r_csolz = &l1rec->csolz[ip];
        r_csenz = &l1rec->csenz[ip];
    }

    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    if (firstCall) {

        int32_t *wave;

        nwave = rdsensorinfo(sensorID, evalmask, "Lambda", (void **) &wave);

        if ((tmp_str = getenv("OCDATAROOT")) == NULL) {
            printf("OCDATAROOT environment variable is not defined.\n");
            exit(1);
        }
        if ((evalmask & NEWRAYTAB) != 0) {
            strcpy(path, tmp_str);
            strcat(path, "/eval/");
            strcat(path, sensorId2SensorDir(sensorID));
            strcat(path, "/");
        } else {
            strcpy(path, tmp_str);
            strcat(path, "/");
            strcat(path, sensorId2SensorDir(sensorID));
            strcat(path, "/");
        }
        printf("\n");
        if ((ray_for_i = (ray_array *) calloc(nwave, sizeof (ray_array))) == NULL) {
            printf("-E- : Error allocating memory to ray_for_i\n");
            exit(FATAL_ERROR);
        }

        if ((ray_tau = (float *) calloc(nwave, sizeof (float))) == NULL) {
            printf("-E- : Error allocating memory to ray_for_q\n");
            exit(FATAL_ERROR);
        }
        if ((ray_dep = (float *) calloc(nwave, sizeof (float))) == NULL) {
            printf("-E- : Error allocating memory to ray_for_q\n");
            exit(FATAL_ERROR);
        }

        if ((ray_for_q = (ray_array *) calloc(nwave, sizeof (ray_array))) == NULL) {
            printf("-E- : Error allocating memory to ray_for_q\n");
            exit(FATAL_ERROR);
        }
        if ((ray_for_u = (ray_array *) calloc(nwave, sizeof (ray_array))) == NULL) {
            printf("-E- : Error allocating memory to ray_for_u\n");
            exit(FATAL_ERROR);
        }

        strcpy(sensorstr, sensorId2SensorName(sensorID));
        lowcase(sensorstr);
        
        for (iw = 0; iw < nwave; iw++) {

            sprintf(wavestr, "%d", (int) wave[iw]);
            file[0] = 0;
            
            // try subsensor dir first
            if(l1rec->l1file->subsensorID >= 0) {
                strcpy(file, path);
                strcat(file, subsensorId2SubsensorDir(l1rec->l1file->subsensorID));
                strcat(file, "/rayleigh/rayleigh_");
                strcat(file, sensorstr);
                strcat(file, "_");
                strcat(file, wavestr);
                strcat(file, "_iqu.hdf");
            }

            // then try sensor dir
            if(access(file, R_OK) == -1) {
                strcpy(file, path);
                strcat(file, "rayleigh/rayleigh_");
                strcat(file, sensorstr);
                strcat(file, "_");
                strcat(file, wavestr);
                strcat(file, "_iqu.hdf");
            }

            /* Open the file and initiate the SD interface */
            sd_id = SDstart(file, DFACC_RDONLY);
            if (sd_id == -1) {
                printf("-E- %s:  Error opening file %s.\n", __FILE__, file);
                exit(1);
            } else
                printf("Loading Rayleigh LUT %s\n", file);

            strcpy(sdsname, "taur");
            sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
            status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
            status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) ray_tau);
            if (status != 0) {
                printf("-E- %s:  Error reading SDS %s from %s.\n", __FILE__, sdsname, file);
                SDend(sd_id);
                exit(1);
            } else {
                status = SDendaccess(sds_id);
            }

            strcpy(sdsname, "depol");
            sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
            status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
            status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) ray_dep);
            if (status != 0) {
                printf("-E- %s:  Error reading SDS %s from %s.\n", __FILE__, sdsname, file);
                SDend(sd_id);
                exit(1);
            } else {
                status = SDendaccess(sds_id);
            }

            strcpy(sdsname, "senz");
            sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
            status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
            status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) ray_sen);
            if (status != 0) {
                printf("-E- %s:  Error reading SDS %s from %s.\n", __FILE__, sdsname, file);
                SDend(sd_id);
                exit(1);
            } else {
                status = SDendaccess(sds_id);
            }

            strcpy(sdsname, "solz");
            sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
            status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
            status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) ray_sol);
            if (status != 0) {
                printf("-E- %s:  Error reading SDS %s from %s.\n", __FILE__, sdsname, file);
                SDend(sd_id);
                exit(1);
            } else {
                status = SDendaccess(sds_id);
            }

            strcpy(sdsname, "sigma");
            sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
            status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
            status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) ray_sigma);
            if (status != 0) {
                printf("-E- %s:  Error reading SDS %s from %s.\n", __FILE__, sdsname, file);
                SDend(sd_id);
                exit(1);
            } else {
                status = SDendaccess(sds_id);
            }

            strcpy(sdsname, "i_ray");
            sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
            status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
            status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) ray_for_i[iw]);
            if (status != 0) {
                printf("-E- %s:  Error reading SDS %s from %s.\n", __FILE__, sdsname, file);
                SDend(sd_id);
                exit(1);
            } else {
                status = SDendaccess(sds_id);
            }

            if (pol_opt > 0) {

                strcpy(sdsname, "q_ray");
                sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
                status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
                status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) ray_for_q[iw]);
                if (status != 0) {
                    printf("-E- %s:  Error reading SDS %s from %s.\n", __FILE__, sdsname, file);
                    SDend(sd_id);
                    exit(1);
                } else {
                    status = SDendaccess(sds_id);
                }

                strcpy(sdsname, "u_ray");
                sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
                status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
                status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) ray_for_u[iw]);
                if (status != 0) {
                    printf("-E- %s:  Error reading SDS %s from %s.\n", __FILE__, sdsname, file);
                    SDend(sd_id);
                    exit(1);
                } else {
                    status = SDendaccess(sds_id);
                }
            }
            SDend(sd_id);
        }

        firstCall = 0;
    }


    /* windspeed index into tables */

    sigma_m = 0.0731 * sqrt(ws);
    isigma2 = 1;
    while (sigma_m > ray_sigma[isigma2] && isigma2 < nwind)
        isigma2++;
    isigma1 = isigma2 - 1;



    /* interpolate Rayleigh Stokes vectors for each band */
    gmult = 0;
    if (l1rec->geom_per_band != NULL) {
        gmult = 1;
    }
    for (iw = 0; iw < nwave; iw++) {

        ray_i = 0.0;
        ray_q = 0.0;
        ray_u = 0.0;

        /* geometry indices into tables */

        if ((iw == 0) || (gmult == 1)) {
            ix = iw * gmult;

            for (m = 0; m < norder; m++) {
                cosd_phi[m] = cos(r_phi[ix] * m / RADEG);
                sind_phi[m] = sin(r_phi[ix] * m / RADEG);
            }

            /* solar zenith indices */

            isol1 = ((int) r_solz[ix]) / 2;
            isol2 = isol1 + 1;

            /* sensor zenith indices */

            for (isen2 = 0; isen2 < nsen; isen2++) {
                if (r_senz[ix] < ray_sen[isen2])
                    break;
            }
            isen1 = isen2 - 1;

            /* interpolation coefficients */
            /*  for solz > 88 or isol1 >= nsol - 1, use coeffs at end index */
            if (isol1 >= nsol - 1) {
                isol1 = nsol - 1;
                isol2 = isol1;
                p = 1.;
            } else
                p = (r_solz[ix] - ray_sol[isol1]) / (ray_sol[isol2] - ray_sol[isol1]);
            q = (r_senz[ix] - ray_sen[isen1]) / (ray_sen[isen2] - ray_sen[isen1]);
            airmass = 1. / r_csolz[ix] + 1. / r_csenz[ix];
        }


        /* interpolate the Rayleigh coefficients for each windspeed */

        for (isigma = isigma1; isigma <= isigma2; isigma++) {

            iwind = isigma - isigma1; /* 0 or 1 */

            ray_i_sig [iwind] = 0.;
            ray_q_sig [iwind] = 0.;
            ray_u_sig [iwind] = 0.;

            /* I component */

            for (m = 0; m < norder; m++) {

                f00 = ray_for_i[iw][isigma][isol1][m][isen1];
                f10 = ray_for_i[iw][isigma][isol2][m][isen1];
                f01 = ray_for_i[iw][isigma][isol1][m][isen2];
                f11 = ray_for_i[iw][isigma][isol2][m][isen2];

                ray_i_sig[iwind] = ray_i_sig[iwind] +
                        ((1. - p)*(1. - q) * f00 + p * q * f11 + p * (1. - q) * f10 + q * (1. - p) * f01) * cosd_phi[m];
            }

            if (pol_opt <= 0)
                continue;

            /* Q component */

            for (m = 0; m < norder; m++) {

                f00 = ray_for_q[iw][isigma][isol1][m][isen1];
                f10 = ray_for_q[iw][isigma][isol2][m][isen1];
                f01 = ray_for_q[iw][isigma][isol1][m][isen2];
                f11 = ray_for_q[iw][isigma][isol2][m][isen2];

                ray_q_sig[iwind] = ray_q_sig[iwind] +
                        ((1. - p)*(1. - q) * f00 + p * q * f11 + p * (1. - q) * f10 + q * (1. - p) * f01) * cosd_phi[m];
            }


            /* U component */

            for (m = 0; m < norder; m++) {

                f00 = ray_for_u[iw][isigma][isol1][m][isen1];
                f10 = ray_for_u[iw][isigma][isol2][m][isen1];
                f01 = ray_for_u[iw][isigma][isol1][m][isen2];
                f11 = ray_for_u[iw][isigma][isol2][m][isen2];

                ray_u_sig[iwind] = ray_u_sig[iwind] +
                        ((1. - p)*(1. - q) * f00 + p * q * f11 + p * (1. - q) * f10 + q * (1. - p) * f01) * sind_phi[m];
            }

        }


        /* do linear interpolation between wind speeds */

        if (isigma1 == isigma2) {

            ray_i = ray_i_sig[0];

            if (pol_opt > 0) {
                ray_q = ray_q_sig[0];
                ray_u = ray_u_sig[0];
            }

        } else {

            h = (sigma_m - ray_sigma[isigma1]) / (ray_sigma[isigma2] - ray_sigma[isigma1]);

            ray_i = ray_i_sig[0] + (ray_i_sig[1] - ray_i_sig[0]) * h;

            if (pol_opt > 0) {
                ray_q = ray_q_sig[0] + (ray_q_sig[1] - ray_q_sig[0]) * h;
                ray_u = ray_u_sig[0] + (ray_u_sig[1] - ray_u_sig[0]) * h;
            }
        }
        /*  Compute the rayleigh radiance */
        fac = ray_press_wang(taur[iw], airmass, pr);
        ipw = ip * nwave + iw;
        l1rec->Lr[ipw] = Fo[iw] * ray_i*fac;
        l1rec->L_q[ipw] = Fo[iw] * ray_q*fac;
        l1rec->L_u[ipw] = Fo[iw] * ray_u*fac;
    }
}



