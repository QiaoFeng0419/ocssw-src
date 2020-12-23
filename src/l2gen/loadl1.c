/* =========================================================== */
/* Module loadl1.c                                             */
/*                                                             */
/* Functions to fill a level-1b file with precomputed          */
/* atmospheric and masking data.                               */
/*                                                             */
/* Note: due to filtering capabilities of MSl12, it can not be */
/* assumed that the same l1rec pointer will be passed in later */
/* calls to this function, so all fields must be reloaded. In  */
/* addition, it is possible for MSl12 to process a L1B file    */
/* containing data from different time periods, so earth-sun   */
/* distance can not be assumed to be constant.                 */
/*                                                             */
/* Written By:                                                 */
/*                                                             */
/*     B. A. Franz                                             */
/*     SAIC General Sciences Corp.                             */
/*     NASA/SIMBIOS Project                                    */
/*     February 1999                                           */
/*                                                             */
/* Perturbed By:                                               */
/* E.Karakoylu (SAIC)                                          */
/* Summer 2015                                                 */
/* =========================================================== */

#include <stdio.h>
#include "l12_proto.h"
#include "smi_climatology.h"
#include "read_pixel_anc_file.h"

// noise option ------------//
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <sys/time.h>
//--------------------------//

unsigned long int random_seed() {
    /* Seed generator for gsl. */
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (tv.tv_sec + tv.tv_usec);
}

float make_noise(float sigma) {
    unsigned long randSeed = random_seed();
    float noise;
    gsl_rng *rng;
    rng = gsl_rng_alloc(gsl_rng_mt19937);
    gsl_rng_set(rng, randSeed);
    noise = gsl_ran_gaussian(rng, sigma);
    gsl_rng_free(rng);
    return (noise);
}

float noise_model_hmodisa(float lt, int32_t iw, float snr_mult) {
    /*
    Noise coefficients entered in order: {C0,C1} such that C0 + C1 * lt
    If iw corresponds 667 or 678 and snr_mult = 42 then coefficients for
    667H and 678H are used. These are stored in the same order but in
    an alternative array
     */
    static int firstpassperband = 0;

    float sigma, noise, snr, scaled_lt;
    float ltSnrFitCoef[16][2] = {
        {/*412:C0,C1*/0.05499859, 0.00008340},
        {/*443:*/0.02939470, 0.00009380},
        {/*469:*/0.11931482, 0.00008195},
        {/*488:*/0.01927545, 0.00009450},
        {/*531:*/0.01397522, 0.00010040},
        {/*547:*/0.01139088, 0.00016480},
        {/*555*/0.08769538, 0.00007000},
        {/*645:*/0.10406925, 0.00008533},
        {/*667L*/0.00496291, 0.00014050},
        {/*678L*/0.00427147, 0.00013160},
        {/*748*/0.00416994, 0.00021250},
        {/*859*/0.04055895, 0.00019755},
        {/*869*/0.00312263, 0.00018600},
        {/*1240*/0.07877732, 0.00049940},
        {/*1640*/0.26743281, 0.01044864},
        {/*2130*/0.00628912, 0.00021160}
    };
    scaled_lt = lt;
    noise = ltSnrFitCoef[iw][0] + ltSnrFitCoef[iw][1] * scaled_lt;
    snr = scaled_lt * snr_mult / noise; //noise model based on
    sigma = 1 / snr;
    if (firstpassperband == iw) {
        printf("Band: %d - Lt=%f - Sigma=%f\n", iw, lt, sigma);
        firstpassperband++;
    }
    return sigma;
}

float noise_model_swf(float lt, int32_t iw, float snr_mult) {

    static int firstpassperband = 0;
    float sigma;
    int i, fitPolyOrder = 4;
    float snr = 0;
    float ltSnrFitCoef[8][5] = {
        {-8.28726301e-03, 3.85425664e-01, -9.10776926e+00, 1.65881862e+02, 4.54351582e-01},
        {-1.21871258e-02, 5.21579320e-01, -1.14574109e+01, 1.96509056e+02, 4.18921861e-01},
        {-2.99068165e-02, 1.05225457e-00, -1.90591166e+01, 2.66343986e+02, 6.67187489e-01},
        {-5.68939986e-02, 1.67950509e-00, -2.56915149e+01, 3.05832773e+02, 9.34468454e-01},
        {-1.31635902e-01, 3.09617393e-00, -3.73473556e+01, 3.52394751e+02, 3.54105899e-01},
        {-8.65458303e-01, 1.18857306e+01, -8.37771886e+01, 4.64496430e+02, 4.14633422e-02},
        {-4.96827099e+00, 4.50239057e+01, -2.10425126e+02, 7.75862055e+02, 5.18893137e-02},
        {-1.30487418e+01, 9.35407901e+01, -3.40988182e+02, 9.43414239e+02, 7.84956550e-01}
    };
    /*  Unused 2018-08-28 G. Fireman
    float sigmaLim[8][2] = {
        {0.00088, 0.059},
        {0.00078, 0.05012},
        {0.00073, 0.03689},
        {0.00074, 0.03199},
        {0.0008, 0.05593},
        {0.00108, 0.04337},
        {0.00093, 0.0261},
        {0.00109, 0.02123}
    };
    */


    for (i = 0; i < fitPolyOrder; i++)
        snr += ltSnrFitCoef[iw][i] * pow(lt, (fitPolyOrder - i));
    snr += ltSnrFitCoef[iw][fitPolyOrder];
    snr *= snr_mult;
    sigma = 1 / snr;
    if (firstpassperband == iw) {
        printf("Band: %d - Lt=%f - Sigma=%f\n", iw, lt, sigma);
        firstpassperband++;
    }
    return sigma;
}

float get_Lt_noise(float lt, int32_t iw, int32_t sensorID, float snr_fac) {

    float sigma;
    float noise;

    switch (sensorID) {

    case SEAWIFS:
        sigma = noise_model_swf(lt, iw, snr_fac);
        break;

    case MODISA:
        sigma = noise_model_hmodisa(lt, iw, snr_fac);
        break;
    default:
        printf("-E-: %s line %d: Sensor not  supported.\n", __FILE__, __LINE__);
        exit(1);
        break;
    }

    noise = make_noise(sigma);
    return noise;
}

/*
float get_ws_noise(float ws_unc, float input_factor){
    static int printPass = 0;

    float noise, sigma;
    if (input_factor == 0.0)
        sigma = fabs(ws_unc/2);
    else if (input_factor > 0.0)
        sigma = input_factor;
    else
        sigma = 0;
    noise = make_noise(sigma);
    if ((printPass % 1000000) == 0){
        printf("WSNOISE=>%d: ws_unc=%f --ws_noise_flag=%f --sigma=%f --noise=%f \n",
               printPass, ws_unc, input_factor, sigma, noise);
        printPass += 1000000;
    }
    return noise;
}
 */

float get_anc_noise(float anc_unc, float anc_data, float inp_factor, char anc_name[20]) {
    static int printPass = 0;

    float noise, sigma = 0;
    if (inp_factor == 0.0)
        sigma = fabs(anc_unc / anc_data / sqrt(2));
    else if (inp_factor > 0.0)
        sigma = fabs(inp_factor * anc_unc / anc_data / sqrt(2));
    noise = make_noise(sigma);
    if ((printPass % 1000000) == 0) {
        printf("ANCNOISE=>%d: anc_name=%s|-|anc_unc=%f|-|anc_data=%f|-|inp-fac=%f|-|sigma=%f|-|noise=%f \n",
                printPass, anc_name, anc_unc, anc_data, inp_factor, sigma, noise);
        printPass += 1000000;
    }
    return noise;
}

int loadl1(filehandle *l1file, l1str *l1rec) {
    static double radeg = RADEG;
    static int32_t sensorID = -999;
    static float *aw;
    static float *bbw;
    int navfail_cnt = 0;

    int32_t ip, ipb, ib, iw, ix;
    double esdist;
    int32_t nbands = l1rec->l1file->nbands;

    double *rvs;
    double temp;
    float lt_noise = input->add_lt_noise;
    float ws_noise = input->add_ws_noise;
    float wd_noise = input->add_wd_noise;
    float mw_noise = input->add_mw_noise;
    float zw_noise = input->add_zw_noise;
    float rh_noise = input->add_rh_noise;
    float pr_noise = input->add_pr_noise;
    float wv_noise = input->add_wv_noise;
    float oz_noise = input->add_oz_noise;
    float no2_tropo_noise = input->add_no2_tropo_noise;
    float no2_strat_noise = input->add_no2_strat_noise;
    float noise_factor;
    float snr_factor;
    static int firstpassperband = 0;
    int16_t year, day;
    double sec;
    unix2yds(l1rec->scantime, &year, &day, &sec);

    if (sensorID != l1file->sensorID) {

        sensorID = l1file->sensorID;
        aw = l1file->aw;
        bbw = l1file->bbw;


        printf("Loading land mask file from %s\n", input->land);
        if (land_mask_init(input->land) != 0) {
            printf("-E- %s : Unable to initialize land mask\n", __FILE__);
            exit(1);
        }

        printf("Loading bathymetry mask file from %s\n", input->water);
        if (bath_mask_init(input->water) != 0) {
            printf("-E- %s : Unable to initialize bath mask\n", __FILE__);
            exit(1);
        }

        printf("Loading ice mask file from %s\n", input->icefile);
        if (ice_mask_init(input->icefile, (int) year,
                (int) day, input->ice_threshold) != 0) {
            printf("-E- %s : Unable to initialize ice mask\n", __FILE__);
            exit(1);
        }

        if (input->elevfile[0])
            printf("Loading elevation file from %s\n", input->elevfile);
        if (input->elev_auxfile[0])
            printf("Loading auxiliary elevation file from %s\n", input->elev_auxfile);
        elev_init(input->elevfile, input->elev_auxfile);

    }

    /* Get correction for Earth-Sun distance and apply to Fo  */
    int32_t yr = (int32_t) year;
    int32_t dy = (int32_t) day;
    int32_t ms = (int32_t) (sec * 1.e3);
    esdist = esdist_(&yr, &dy, &ms);
    l1rec->fsol = pow(1.0 / esdist, 2);

    for (iw = 0; iw < nbands; iw++) {
        l1rec->Fo[iw] = l1file->Fobar[iw] * l1rec->fsol;
    }

    /* Apply vicarious cross-calibration gains */

    for (ix = 0; ix < input->xcal_nwave; ix++) {
        if ((input->xcal_opt[ix] & XCALRVS) != 0) {
            if ((ib = bindex_get(input->xcal_wave[ix])) < 0) {
                printf("-E- %sline %d: xcal wavelength %f does not match sensor\n",
                        __FILE__, __LINE__, input->xcal_wave[ix]);
                exit(1);
            };
            rvs = get_xcal(l1rec, XRVS, l1file->iwave[ib]);

            for (ip = 0; ip < l1rec->npix; ip++) {
                ipb = ip * nbands + ib;
                if (rvs == 0x0) {
                    l1rec->Lt[ipb] = -999.0;
                    continue;
                }
                if (l1rec->Lt[ipb] > 0.0 && l1rec->Lt[ipb] < 1000.0)
                    l1rec->Lt[ipb] /= rvs[ip];
            }
        }
    }

    for (ip = 0; ip < l1rec->npix; ip++) {
        /* Apply vicarious calibration */

        for (iw = 0; iw < nbands; iw++) {
            ipb = ip * nbands + iw;

            if (l1rec->Lt[ipb] > 0.0 && l1rec->Lt[ipb] < 1000.0) {
                l1rec->Lt[ipb] *= input->gain[iw];
                l1rec->Lt[ipb] += input->offset [iw];

            }
        }

        /*** Geolocation-based lookups ***/
        if (!l1rec->navfail[ip]) {

            /* Enforce longitude convention */
            if (l1rec->lon[ip] < -180.)
                l1rec->lon[ip] += 360.0;

            else if (l1rec->lon[ip] > 180.0)
                l1rec->lon[ip] -= 360.0;

            /* Get terrain height */
            if (input->proc_land) {
                if (get_height(input->demfile, l1rec, ip,
                        l1file->terrain_corrected) != 0) {
                    printf("-E- %s line %d: Error getting terrain height.\n",
                            __FILE__, __LINE__);
                    return (1);
                }
            } else
                l1rec->height[ip] = 0.0;

            /* Set land, bathymetry and ice flags */
            if (input->proc_ocean != 2 &&
                    l1file->format != FT_L3BIN &&
                    land_mask(l1rec->lat[ip], l1rec->lon[ip]) != 0) {
                l1rec->land[ip] = ON;
            }
            if (!l1rec->land[ip] &&
                    bath_mask(l1rec->lat[ip], l1rec->lon[ip]) != 0) {
                l1rec->swater[ip] = ON;
            }
            if (!l1rec->land[ip] &&
                    ice_mask(l1rec->lon[ip], l1rec->lat[ip]) != 0) {
                l1rec->ice[ip] = ON;
            }
        } else {
            navfail_cnt++;
        }
        /*** end Geolocation-based lookups ***/

        /* Set sea surface temperature and salinity, and seawater optical properties */
        for (iw = 0; iw < nbands; iw++) {
            ipb = ip * nbands + iw;
            l1rec->sw_n [ipb] = 1.334;
            // center band
            l1rec->sw_a [ipb] = aw_spectra(l1file->fwave[iw], BANDW);
            l1rec->sw_bb[ipb] = bbw_spectra(l1file->fwave[iw], BANDW);
            // band-averaged
            l1rec->sw_a_avg [ipb] = aw [iw];
            l1rec->sw_bb_avg[ipb] = bbw[iw];
        }

        l1rec->sstref[ip] = BAD_FLT;
        l1rec->sssref[ip] = BAD_FLT;

        if (!l1rec->land[ip]) {
            float bbw_fac;
            l1rec->sstref[ip] = get_sstref(input->sstreftype, input->sstfile, l1rec, ip);
            l1rec->sssref[ip] = get_sssref(input->sssfile, l1rec->lon[ip], l1rec->lat[ip], (int) day);
            if (l1rec->sstref[ip] > BAD_FLT && l1rec->sssref[ip] > BAD_FLT && input->seawater_opt > 0) {
                for (iw = 0; iw < nbands; iw++) {
                    ipb = ip * nbands + iw;
                    l1rec->sw_n [ipb] = seawater_nsw(l1file->fwave[iw], l1rec->sstref[ip], l1rec->sssref[ip], NULL);
                    // scale bbw based on ratio of center-band model results for actual sea state versus
                    // conditions of Morel measurements used to derive center and band-averaged bbw
                    bbw_fac = seawater_bb(l1file->fwave[iw], l1rec->sstref[ip], l1rec->sssref[ip]) / seawater_bb(l1file->fwave[iw], 20.0, 38.4);
                    l1rec->sw_bb[ipb] *= bbw_fac;
                    l1rec->sw_bb_avg[ipb] *= bbw_fac;
                }
            }
        }
        seawater_set(l1rec);

        /* Compute relative azimuth */
        /* CLASS AVHRR files contain relative azimuth so don't overwrite it */
        if (sensorID != AVHRR) {
            l1rec->delphi[ip] = l1rec->sena[ip] - 180.0 - l1rec->sola[ip];
        }
        if (l1rec->delphi[ip] < -180.)
            l1rec->delphi[ip] += 360.0;
        else if (l1rec->delphi[ip] > 180.0)
            l1rec->delphi[ip] -= 360.0;

        /* Precompute frequently used trig relations */
        l1rec->csolz[ip] = cos(l1rec->solz[ip] / radeg);
        l1rec->csenz[ip] = cos(l1rec->senz[ip] / radeg);

        /* Scattering angle */
        temp = sqrt((1.0 - l1rec->csenz[ip] * l1rec->csenz[ip])*(1.0 - l1rec->csolz[ip] * l1rec->csolz[ip]))
                * cos(l1rec->delphi[ip] / radeg);
        l1rec->scattang[ip] = acos(MAX(-l1rec->csenz[ip] * l1rec->csolz[ip] + temp, -1.0)) * radeg;

    }
    /* get derived quantities for band-dependent view angles */
    if (l1rec->geom_per_band != NULL)
        geom_per_band_deriv(l1rec);

    /* add ancillary data */
    // the navfail_cnt test is a kludge to prevent processing failures for scans that are entirely invalid.
    if (navfail_cnt != l1rec->npix) {
        if (setanc(l1rec) != 0)
            return (1);
    }
    
    if (input->pixel_anc_file[0])
        read_pixel_anc_file(input->pixel_anc_file, l1rec);

    if (input->windspeed > -999)
        for (ip = 0; ip < l1rec->npix; ip++)
            l1rec->ws[ip] = input->windspeed;
    if (input->windangle > -999)
        for (ip = 0; ip < l1rec->npix; ip++)
            l1rec->wd[ip] = input->windangle;
    if (input->pressure > -999)
        for (ip = 0; ip < l1rec->npix; ip++)
            l1rec->pr[ip] = input->pressure;
    if (input->ozone > -999)
        for (ip = 0; ip < l1rec->npix; ip++)
            l1rec->oz[ip] = input->ozone;
    if (input->watervapor > -999)
        for (ip = 0; ip < l1rec->npix; ip++)
            l1rec->wv[ip] = input->watervapor;
    if (input->relhumid > -999)
        for (ip = 0; ip < l1rec->npix; ip++)
            l1rec->rh[ip] = input->relhumid;

    /* SWIM bathymetry */
    for (ip = 0; ip < l1rec->npix; ip++)
        if (!l1rec->navfail[ip])
            l1rec->elev[ip] = get_elev(l1rec->lat[ip], l1rec->lon[ip]);

    /* add atmospheric cnomponents that do not depend on Lt */
    for (ip = 0; ip < l1rec->npix; ip++) {

        /* -------------------------------------------------------- */
        /*        Monte-Carlo and Other Uncertainty Processes       */
        /* -------------------------------------------------------- */

        if (ws_noise >= 0) {
            char name[] = "ws_noise";
            noise_factor = get_anc_noise(l1rec->ws_unc[ip], l1rec->ws[ip], ws_noise, name);
            l1rec->ws[ip] *= (1 + noise_factor);
        }
        if (wd_noise >= 0) {
            char name[] = "wd_noise";
            noise_factor = get_anc_noise(l1rec->wd_unc[ip], l1rec->wd[ip], wd_noise, name);
            l1rec->wd[ip] *= (1 + noise_factor);
        }
        if (mw_noise >= 0) {
            char name[] = "mw_noise";
            noise_factor = get_anc_noise(l1rec->mw_unc[ip], l1rec->mw[ip], mw_noise, name);
            l1rec->mw[ip] *= (1 + noise_factor);
        }
        if (zw_noise >= 0) {
            char name[] = "zw_noise";
            noise_factor = get_anc_noise(l1rec->zw_unc[ip], l1rec->zw[ip], zw_noise, name);
            l1rec->zw[ip] *= (1 + noise_factor);
        }
        if (rh_noise >= 0) {
            char name[] = "rh_noise";
            noise_factor = get_anc_noise(l1rec->rh_unc[ip], l1rec->rh[ip], rh_noise, name);
            l1rec->rh[ip] *= (1 + noise_factor);
        }
        if (pr_noise >= 0) {
            char name[] = "pr_noise";
            noise_factor = get_anc_noise(l1rec->pr_unc[ip], l1rec->pr[ip], pr_noise, name);
            l1rec->pr[ip] *= (1 + noise_factor);
        }
        if (wv_noise >= 0) {
            char name[] = "wv_noise";
            noise_factor = get_anc_noise(l1rec->wv_unc[ip], l1rec->wv[ip], wv_noise, name);
            l1rec->wv[ip] *= (1 + noise_factor);
        }
        if (oz_noise >= 0) {
            char name[] = "oz_noise";
            noise_factor = get_anc_noise(l1rec->oz_unc[ip], l1rec->oz[ip], oz_noise, name);
            l1rec->oz[ip] *= (1 + noise_factor);
        }
        if (no2_tropo_noise >= 0) {
            char name[] = "no2_tropo_noise";
            noise_factor = get_anc_noise(l1rec->no2_tropo_unc[ip], l1rec->no2_tropo[ip], no2_tropo_noise, name);
            l1rec->no2_tropo[ip] *= (1 + noise_factor);
        }
        if (no2_strat_noise >= 0) {
            char name[] = "no2_strat_noise";
            noise_factor = get_anc_noise(l1rec->no2_strat_unc[ip], l1rec->no2_strat[ip], no2_strat_noise, name);
            l1rec->no2_strat[ip] *= (1 + noise_factor);
        }

        if (lt_noise) {
            for (iw = 0; iw < nbands; iw++) {
                ipb = ip * nbands + iw;
                snr_factor = input->lt_noise_scale[iw];
                if (snr_factor < 0)
                    noise_factor = 0;
                else
                    noise_factor = get_Lt_noise(l1rec->Lt[ipb], iw, sensorID, snr_factor);
                l1rec->Lt[ipb] *= (1 + noise_factor);
                if (input->bias_frac[iw] > 0.0) {
                    l1rec->Lt[ipb] *= (1 + input->bias_frac[iw]);
                    if (firstpassperband == iw) {
                        if (iw == 0)
                            printf("-------Bias settings-------\n");
                        printf("Band: %d  - Bias frac: %.3f\n", iw, input->bias_frac[iw]);
                        firstpassperband++;
                    }
                }
            }
        }

        if (l1rec->is_l2){
            // l1rec->Lt is actually Rrs, so skip atmo, etc
        } else {
        /* ------------------------------------------------ */
        /* Ocean processing                                 */
        /* ------------------------------------------------ */
            if ((input->proc_ocean != 0) && !l1rec->land[ip] && !l1rec->navfail[ip]) {

                atmocor1(l1rec, ip);

                /* set the smile_delta field */
                radcor(l1rec, ip, 0);

                /* set polarization correction */
                polcor(l1rec, ip);

                /* add surface reflectance */
                get_rhos(l1rec, ip);

                /* assign uncertainty on Lt if not already set by sensor-specific i/o */
                for (iw = 0; iw < nbands; iw++) {
                    ipb = ip * nbands + iw;
                    if (l1rec->Lt_unc[ipb] < BAD_FLT + 1) {

                        l1rec->Lt_unc[ipb] = (l1rec->Lt[ipb] - l1rec->Lr[ipb]) * input->gain_unc[iw];
                    }
                }
            }
                /* ------------------------------------------------ */
                /* Land Processing                                  */
                /* ------------------------------------------------ */
            else if (input->proc_land && l1rec->land[ip] && !l1rec->navfail[ip]) {
                atmocor1_land(l1rec, ip);
                radcor(l1rec, ip, 1);
                get_rhos(l1rec, ip);
            }
                /* ------------------------------------------------ */
                /* General Processing                               */
                /* ------------------------------------------------ */
            else {
                for (ib = 0; ib < nbands; ib++) {
                    ipb = ip * nbands + ib;
                    l1rec->Lr [ipb] = 0.0;
                    l1rec->t_sol [ipb] = 1.0;
                    l1rec->t_sen [ipb] = 1.0;
                    l1rec->tg_sol[ipb] = 1.0;
                    l1rec->tg_sen[ipb] = 1.0;
                    l1rec->t_o2 [ipb] = 1.0;
                    l1rec->t_h2o [ipb] = 1.0;
                    l1rec->polcor [ipb] = 1.0;
                    l1rec->radcor [ipb] = 0.0;
                }
            }
        }

    }
    /* set masks and flags */
    if (setflags(l1rec) == 0)
        return (1);

    setflagbits(1, l1rec, NULL, -1);

    return (0);
}
