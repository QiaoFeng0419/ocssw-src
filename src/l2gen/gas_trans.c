/* =========================================================== */
/* Module gaseous_transmittance.c                              */
/*                                                             */
/* Computes sensor-specific transmittance through various      */
/* atmospheric gases.                                          */
/*                                                             */
/* B. Franz, NASA/OBPG, July 2006                              */
/* =========================================================== */

#include "l12_proto.h"
#include <allocate3d.h>

// TODO: need to get this table off of the stack
size_t num_models, num_wavelengths, num_water_vapors;
float ***wvtbl = NULL;
float *cwv_all = NULL;


int32_t get_wvindex(float *wvtable,int32_t nwv, double wv)
{
	int32_t index;
	int32_t i;

	if(wv>wvtable[nwv-1])
		index=nwv-1;
	else
	{
		for(i=0;i<nwv;i++)
			if(wv<wvtable[i])
				break;
	}

	index=i;
	if( (wv-wvtable[i-1])<  (wvtable[i]-wv) )
		index=i-1;

	return index;
}


void ozone_transmittance(l1str *l1rec, int32_t ip) {
    float tau_oz;
    int32_t iw;

    filehandle *l1file = l1rec->l1file;
    int32_t nwave = l1file->nbands;
    int32_t ipb = ip*nwave;

    for (iw = 0; iw < nwave; iw++) {
        tau_oz = l1rec->oz[ip] * l1file->k_oz[iw];
        l1rec->tg_sol[ipb + iw] *= exp(-(tau_oz / l1rec->csolz[ip]));
        l1rec->tg_sen[ipb + iw] *= exp(-(tau_oz / l1rec->csenz[ip]));
    }
}

void co2_transmittance(l1str *l1rec, int32_t ip) {
    static float *t_co2 = NULL;
    int32_t iw;

    filehandle *l1file = l1rec->l1file;
    int32_t nwave = l1file->nbands;
    int32_t ipb = ip*nwave;

    if (t_co2 == NULL) {
        rdsensorinfo(l1file->sensorID, input->evalmask, "t_co2", (void **) &t_co2);
    }

    for (iw = 0; iw < nwave; iw++) {
        l1rec->tg_sol[ipb + iw] *= pow(t_co2[iw], 1.0 / l1rec->csolz[ip]);
        l1rec->tg_sen[ipb + iw] *= pow(t_co2[iw], 1.0 / l1rec->csenz[ip]);
    }
}

void no2_transmittance(l1str *l1rec, int32_t ip) {

    float a_285, a_225;
    float tau_to200;
    float no2_tr200;
    int32_t iw;

    filehandle *l1file = l1rec->l1file;
    int32_t nwave = l1file->nbands;
    int32_t ipb = ip*nwave;

    float sec0 = 1.0 / l1rec->csolz[ip];
    float sec = 1.0 / l1rec->csenz[ip];

    if (l1rec->no2_tropo[ip] > 0.0)
        /* compute tropo no2 above 200m (Z.Ahmad)    
        no2_tr200 = exp(12.6615 + 0.61676*log(no2_tropo));
           new, location-dependent method */
        no2_tr200 = l1rec->no2_frac[ip] * l1rec->no2_tropo[ip];
    else
        no2_tr200 = 0.0;


    for (iw = 0; iw < nwave; iw++) {

        if (l1file->k_no2[iw] > 0.0) {

            a_285 = l1file->k_no2[iw] * (1.0 - 0.003 * (285.0 - 294.0));
            a_225 = l1file->k_no2[iw] * (1.0 - 0.003 * (225.0 - 294.0));

            tau_to200 = a_285 * no2_tr200 + a_225 * l1rec->no2_strat[ip];

            l1rec->tg_sol[ipb + iw] *= exp(-(tau_to200 * sec0));
            l1rec->tg_sen[ipb + iw] *= exp(-(tau_to200 * sec));

        }
    }
}

void h2o_transmittance(l1str *l1rec, int32_t ip) {
    static float *a_h2o = NULL;
    static float *b_h2o = NULL;
    static float *c_h2o = NULL;
    static float *d_h2o = NULL;
    static float *e_h2o = NULL;
    static float *f_h2o = NULL;
    static float *g_h2o = NULL;
    int model = 5;

    static int firstcall=1;

    float t_h2o;
    int32_t iw;

    filehandle *l1file = l1rec->l1file;
    int32_t nwave = l1file->nbands;
    int32_t ipb = ip*nwave;
    float wv = l1rec->wv[ip];


    // Apply water vapor transmittance only for the MSE and multi-band AC from the netcdf
    if (input->aer_opt == AERRHMSEPS || input->aer_opt == AERRHMSEPS_lin || input->aer_opt == AERRHSM) {
        int ja_sol = 0;
        int ja_sen = 0;


        if (firstcall) {

            // netcdf file that has water vapor transmittance as a function of wavelength and cwv (6 profiles x number of bands x 220 water vapor value)
            // filename = /OCDATAROOT/sensor[/subsensor]/gas_trans_<lowercase_sensorname>.nc
            char *filedir;
            char filename[FILENAME_MAX];
            if ((filedir = getenv("OCDATAROOT")) == NULL) {
                printf("-E- %s: OCDATAROOT env variable undefined.\n", __FILE__);
                return;
            }
            strcpy(filename, filedir);

            strcat(filename, "/");
            strcat(filename, sensorId2SensorDir(l1rec->l1file->sensorID));
            
            if(l1rec->l1file->subsensorID != -1) {
                strcat(filename, "/");
                strcat(filename, subsensorId2SubsensorDir(l1rec->l1file->subsensorID));
            }

            char *sensorName = strdup(sensorId2SensorName(l1rec->l1file->sensorID));
            lowcase(sensorName);
            strcat(filename, "/");
            strcat(filename, sensorName);
            strcat(filename, "_gas_transmittance.nc");
            free(sensorName);

            /* Handle errors by printing an error message and exiting with a
             * non-zero status. */
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

            /* This will be the netCDF ID for the file and data variable. */
            int ncid, varid1, varid2;

            /* error handling. */
            int retval;

            //model = 5; //Force to model US standard atmosphere

            if(want_verbose)
                printf("Water vapor model = %d\n", model);


            /* Open the file. NC_NOWRITE tells netCDF we want read-only access
             * to the file.*/
            if (retval = nc_open(filename, NC_NOWRITE, &ncid))
                ERR(retval);

            /* Get the varid of the data variable, based on its name. */
            if ((retval = nc_inq_varid(ncid, "water_vapor_transmittance", &varid1)) || (retval = nc_inq_varid(ncid, "water_vapor", &varid2)))
                ERR(retval);

            int dimids[3];
            if(retval = nc_inq_vardimid(ncid, varid1, dimids)) 	
                ERR(retval);
            if(retval = nc_inq_dimlen(ncid, dimids[0], &num_models))
                ERR(retval);
            if(retval = nc_inq_dimlen(ncid, dimids[1], &num_wavelengths))
                ERR(retval);
            if(retval = nc_inq_dimlen(ncid, dimids[2], &num_water_vapors))
                ERR(retval);

            wvtbl = allocate3d_float(num_models, num_wavelengths, num_water_vapors);
            if(!wvtbl) {
                printf("Error: allocating memory for water_vapor_transmittance\n");
                exit(EXIT_FAILURE);
            }
            cwv_all = malloc(num_water_vapors * sizeof(float));
            if(!cwv_all) {
                printf("Error: allocating memory for water_vapor\n");
                exit(EXIT_FAILURE);
            }

            /* Read the data. */
            if ((retval = nc_get_var_float(ncid, varid1, &wvtbl[0][0][0])) || (retval = nc_get_var_float(ncid, varid2, &cwv_all[0])))
                ERR(retval);

            /* Close the file, freeing all resources. */
            if ((retval = nc_close(ncid)))
                ERR(retval);

            firstcall = 0;
        }

        ja_sol = get_wvindex(cwv_all,num_water_vapors,wv / l1rec->csolz[ip]);
        ja_sen = get_wvindex(cwv_all,num_water_vapors,wv / l1rec->csenz[ip]);

        for (iw = 0; iw < nwave; iw++) {
            l1rec->tg_sol[ipb + iw] *= wvtbl[model][iw][ja_sol];
            l1rec->tg_sen[ipb + iw] *= wvtbl[model][iw][ja_sen];
        }
    }        // otherwise apply Zia's tabel from Bo-cai
    else {
        if (a_h2o == NULL) {
            rdsensorinfo(l1file->sensorID, input->evalmask, "a_h2o", (void **) &a_h2o);
            rdsensorinfo(l1file->sensorID, input->evalmask, "b_h2o", (void **) &b_h2o);
            rdsensorinfo(l1file->sensorID, input->evalmask, "c_h2o", (void **) &c_h2o);
            rdsensorinfo(l1file->sensorID, input->evalmask, "d_h2o", (void **) &d_h2o);
            rdsensorinfo(l1file->sensorID, input->evalmask, "e_h2o", (void **) &e_h2o);
            rdsensorinfo(l1file->sensorID, input->evalmask, "f_h2o", (void **) &f_h2o);
            rdsensorinfo(l1file->sensorID, input->evalmask, "g_h2o", (void **) &g_h2o);
        }

        for (iw = 0; iw < nwave; iw++) {
            t_h2o = a_h2o[iw] + wv * (b_h2o[iw] + wv * (c_h2o[iw] + wv * (d_h2o[iw]
                    + wv * (e_h2o[iw] + wv * (f_h2o[iw] + wv * g_h2o[iw])))));
            l1rec->tg_sol[ipb + iw] *= pow(t_h2o, 1.0 / l1rec->csolz[ip]);
            l1rec->tg_sen[ipb + iw] *= pow(t_h2o, 1.0 / l1rec->csenz[ip]);
        }
    }
    return;
}

void gaseous_transmittance(l1str *l1rec, int32_t ip) {

    if ((input->gas_opt & O3_BIT) != 0) {
        ozone_transmittance(l1rec, ip);
    }

    if ((input->gas_opt & CO2_BIT) != 0) {
        co2_transmittance(l1rec, ip);
    }

    if ((input->gas_opt & NO2_BIT) != 0) {
        no2_transmittance(l1rec, ip);
    }

    if ((input->gas_opt & H2O_BIT) != 0) {
        h2o_transmittance(l1rec, ip);
    }
}
