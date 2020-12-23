/* ============================================================================ */
/* module l1_olci.c - functions to read OLCI (coastal color) for MSL12          */
/* Written By:  Richard Healy (SAIC) July 29, 2015.                             */
/*                                                                              */
/* ============================================================================ */

#include <netcdf.h>

#include "l1_olci.h"
#include "l12_proto.h"
#include "olci.h"
#include "math.h"
#include <libnav.h>
#include <string.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>
#include <gsl/gsl_sort_double.h>

#define TIE_ROW_PTS      1
#define TIE_COL_PTS     64
#define NEW_CACHE_SIZE 10000000
#define NEW_CACHE_NELEMS 23
#define NEW_CACHE_PREEMPTION 1.0
#define BANDINFO_FILENAME "band_info_olci.txt"

static int16 npix;

static int32 spix = 0;
static int64_t *scan_start_tai;
static double lastvalidtime;
static int lastvalidscan = 0;
static int32_t olci_sd[MAXOLCI_RADFILES], geoFileID, coordFileID, tcoordFileID, instrumentFileID;

olci_t* createPrivateData_olci() {

    olci_t* data = (olci_t*) calloc(1, sizeof (olci_t));
    if (data == NULL) {
        fprintf(stderr, "-E- %s line %d: unable to allocate private data for olci\n",
                __FILE__, __LINE__);
        exit(1);
    }

    return data;
}

/*
 *  W. Robinson, SAIC, 18 Feb 2016  create correct name for radiance dataset 
 *  for I/O
 *
 *  Put olci_varname into olci struct instead of using static global. - rjh 2/29/2016
 */
int openl1_olci(filehandle * l1file) {
    olci_t *data = l1file->private_data;
    size_t source_w, source_h, source_b;
    int32 nscan;
    int timefileID, xid, yid, bid, retval, sds_id;
    int i;
    size_t start[3], count[3];
    unsigned short orbit_number;
    size_t attrLength;
    
    if (!data) {
        char* indir = strdup(l1file->name);
        char* ptr = strrchr(indir, '/');
        size_t nameLength = 30;  // bigger than largest filename
        if(ptr) {
            *ptr = 0;
            nameLength += strlen(indir);
        }
        
        data = l1file->private_data = createPrivateData_olci();
        if ((data->instrumentFile = (char*) malloc(nameLength)) == NULL) {
            printf("%s, %d - E - unable to allocate instrument data filename \n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if ((data->time_coordinatesFile = (char*) malloc(nameLength)) == NULL) {
            printf("%s, %d - E - unable to allocate time coordinates filename \n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if ((data->geoCoordinatesFile = (char*) malloc(nameLength)) == NULL) {
            printf("%s, %d - E - unable to allocate geo coordinates filename \n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if ((data->tieGeometriesFile = (char*) malloc(nameLength)) == NULL) {
            printf("%s, %d - E - unable to allocate tie geometries filename \n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if ((data->tieGeoCoordinatesFile = (char*) malloc(nameLength)) == NULL) {
            printf("%s, %d - E - unable to allocate tie geo coordinates filename \n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if ((data->tieMeteoFile = (char*) malloc(nameLength)) == NULL) {
            printf("%s, %d - E - unable to allocate tie meteo filename \n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }

        if(ptr) {
            sprintf(data->instrumentFile, "%s/instrument_data.nc", indir);
            sprintf(data->time_coordinatesFile, "%s/time_coordinates.nc", indir);
            sprintf(data->geoCoordinatesFile, "%s/geo_coordinates.nc", indir);
            sprintf(data->tieGeometriesFile, "%s/tie_geometries.nc", indir);
            sprintf(data->tieGeoCoordinatesFile, "%s/tie_geo_coordinates.nc", indir);
            sprintf(data->tieMeteoFile, "%s/tie_meteo.nc", indir);
        } else {
            strcpy(data->instrumentFile, "instrument_data.nc");
            strcpy(data->time_coordinatesFile, "time_coordinates.nc");
            strcpy(data->geoCoordinatesFile, "geo_coordinates.nc");
            strcpy(data->tieGeometriesFile, "tie_geometries.nc");
            strcpy(data->tieGeoCoordinatesFile, "tie_geo_coordinates.nc");
            strcpy(data->tieMeteoFile, "tie_meteo.nc");
        }

        for (i = 0; i < MAXOLCI_RADFILES; i++) {
            if ((data->olci_radfiles[i] = (char *) malloc(nameLength)) == NULL) {
                printf("%s, %d - E - unable to allocate radiance filename \n",
                        __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if(ptr)
                sprintf(data->olci_radfiles[i], "%s/Oa%2.2d_radiance.nc", indir, i + 1);
            else
                sprintf(data->olci_radfiles[i], "Oa%2.2d_radiance.nc", i + 1);
            printf("olci rad=%s\n", data->olci_radfiles[i]);
        }

        free(indir);
    }

    // Open the netcdf4 input file
    if (nc_set_chunk_cache(NEW_CACHE_SIZE, NEW_CACHE_NELEMS,
            NEW_CACHE_PREEMPTION) != NC_NOERR) {
        fprintf(stderr, "-E- %s line %d: nc_set_chunk_cache (%s) failed.\n",
                __FILE__, __LINE__, l1file->name);
        return (1);
    }
    retval = nc_open(data->instrumentFile, NC_NOWRITE, &instrumentFileID);
    if (retval != NC_NOERR) {
        fprintf(stderr, "-E- %s line %d: nc_open(%s) failed.\n",
                __FILE__, __LINE__, data->instrumentFile);
        return (1);
    }

    // Get pixel and scan dimensions
    DPTB(nc_inq_dimid(instrumentFileID, "rows", &yid));
    DPTB(nc_inq_dimid(instrumentFileID, "columns", &xid));
    DPTB(nc_inq_dimid(instrumentFileID, "bands", &bid));
    DPTB(nc_inq_dimlen(instrumentFileID, xid, &source_w));
    DPTB(nc_inq_dimlen(instrumentFileID, yid, &source_h));
    DPTB(nc_inq_dimlen(instrumentFileID, bid, &source_b));
    if (source_b != MAXOLCI_RADFILES) {
        fprintf(stderr, "-E- %s line %d: num bands (%d) does not match expected OLCI num bands (%d).\n",
                __FILE__, __LINE__, (int) source_b, MAXOLCI_RADFILES);
        exit(EXIT_FAILURE);
    }

    DPTB(nc_get_att_ushort(instrumentFileID, NC_GLOBAL, "absolute_orbit_number", &orbit_number));

    DPTB(nc_inq_attlen(instrumentFileID, NC_GLOBAL, "product_name", &attrLength));
    char* productName = (char*) allocateMemory(attrLength + 1, "product_name");
    DPTB(nc_get_att_text(instrumentFileID, NC_GLOBAL, "product_name", productName));
    if (strstr(productName, "OL_1_EFR")) {
        strcpy(l1file->spatialResolution, "300 m");
    } else if (strstr(productName, "OL_1_ERR")) {
        strcpy(l1file->spatialResolution, "1.2 km");
    } else {
        fprintf(stderr, "-E- %s line %d: nc_open(%s) illegal product = \"%s\".\n",
                __FILE__, __LINE__, l1file->name, productName);
        exit(EXIT_FAILURE);
    }
    free(productName);

    if (want_verbose) {
        printf("OLCI L1B Npix  :%d Nscans:%d\n", (int) source_w,
                (int) source_h);
    } // want_verbose
    npix = (int32) source_w;
    nscan = (int32) source_h;

    l1file->orbit_number = orbit_number;
    l1file->nbands = source_b;
    l1file->npix = npix;
    l1file->nscan = nscan;

    scan_start_tai = (int64_t *) calloc(nscan, sizeof (int64_t));

    start[0] = 0;
    count[0] = nscan;

    retval = nc_open(data->time_coordinatesFile, NC_NOWRITE, &timefileID);
    if (retval != NC_NOERR) {
        fprintf(stderr, "-E- %s line %d: nc_open(%s) failed.\n",
                __FILE__, __LINE__, data->time_coordinatesFile);
        return (1);
    }

    nc_inq_varid(timefileID, "time_stamp", &sds_id); //microseconds since 1/1/2000
    nc_get_vara_long(timefileID, sds_id, start, count, (long*) scan_start_tai);

    retval = nc_open(data->tieGeometriesFile, NC_NOWRITE, &geoFileID);
    if (retval != NC_NOERR) {
        fprintf(stderr, "-E- %s line %d: nc_open(%s) failed.\n",
                __FILE__, __LINE__, data->tieGeometriesFile);
        return (1);
    }

    retval = nc_open(data->geoCoordinatesFile, NC_NOWRITE, &coordFileID);
    if (retval != NC_NOERR) {
        fprintf(stderr, "-E- %s line %d: nc_open(%s) failed.\n",
                __FILE__, __LINE__, data->geoCoordinatesFile);
        return (1);
    }
    retval = nc_open(data->tieGeoCoordinatesFile, NC_NOWRITE, &tcoordFileID);
    if (retval != NC_NOERR) {
        fprintf(stderr, "-E- %s line %d: nc_open(%s) failed.\n",
                __FILE__, __LINE__, data->tieGeoCoordinatesFile);
        return (1);
    }

    for (i = 0; i < l1file->nbands; i++) {
        // Open each of the netcdf4 Lt files for each band
        retval = nc_open(data->olci_radfiles[i], NC_NOWRITE, &olci_sd[i]);
        if (retval != NC_NOERR) {

            fprintf(stderr,
                    "-E- %s line %d: nc_open failed for file, %s.\n",
                    __FILE__, __LINE__, data->olci_radfiles[i]);
            return (1);
        }
        // NOT so good  strcpy(olci_varname[i],data->olci_radfiles[i]);
        sprintf(data->olci_varname[i], "Oa%02d_radiance", i + 1);
    }


    return (LIFE_IS_GOOD);
}

/*
  W. Robinson, SAIC, 15jul2016  perform interpolations on vector 
         representations of the solar and satellite geometry tie points 
         to eliminate +-180 crossing instabilities
 */
int readl1_olci(filehandle *file, int32 scan, l1str *l1rec) {
    static int firstCall = 1;
    static double time_interval;
    static double tai2unixoffset; //=  946684800;
    static uint *tsolz, *tsenz;
    static int32_t *lon, *lat, *tsola, *tsena, *tlon, *tlat;
    static double xdist, *dist, *sdist, *tielat, *tielon;
    static double tiesolz, tiesenz, tiesola, tiesena, xvec, yvec, zvec, sinz;
    static double *solvecx, *solvecy, *solvecz, *senvecx, *senvecy, *senvecz;
    static float scale_lon, scale_lat, scale_solz, scale_senz, scale_sola, scale_sena, scale_rad;
    static int16_t *detector_index;
    static float *lambda0, *solar_flux;
    static size_t detectors;
    static unsigned int *rad_data;
    static size_t nctlpix, nctlscan;
    static unsigned short tie_row_pts, tie_col_pts;

    float *detectorTmpFloat;

    int retval;

    int32_t ip, ib, ipb;
    int32_t nbands = l1rec->l1file->nbands;
    size_t start[3], count[3];
    int i, j;

    int xid, yid, band_id;

    gsl_spline * spline[6];
    gsl_interp_accel *spl_acc;

    char varnam[14];
    olci_t *data = file->private_data;

    if (firstCall) {
        firstCall = 0;
        if (want_verbose)
            printf("file->nbands = %d, l1rec->nbands = %d\n",
                (int) file->nbands, (int) l1rec->l1file->nbands);

        for (ip = 0; ip < npix; ip++) {
            l1rec->pixnum[ip] = ip;
            l1rec->flags[ip] = 0;
        }

        // This is the time offset between what the time utilities need (1/1/1970) and what
        // the OLCI netcdf provides (1/1/2000)
        tai2unixoffset = yds2unix(2000, 1, 0);

        retval = nc_inq_dimid(tcoordFileID, "tie_columns", &xid);
        retval = nc_inq_dimlen(tcoordFileID, xid, &nctlpix);
        retval = nc_inq_dimid(tcoordFileID, "tie_rows", &yid);
        retval = nc_inq_dimlen(tcoordFileID, yid, &nctlscan);
        retval = nc_get_att_ushort(tcoordFileID, NC_GLOBAL, "ac_subsampling_factor", &tie_col_pts);
        if (retval != NC_NOERR) tie_col_pts = TIE_COL_PTS;
        retval = nc_get_att_ushort(tcoordFileID, NC_GLOBAL, "al_subsampling_factor", &tie_row_pts);
        if (retval != NC_NOERR) tie_row_pts = TIE_ROW_PTS;

        if (tie_row_pts != 1) {
            printf("-E- %s line %d: Sorry.  I can only handle tie_row_pts = 1 not tie_row_pts = %d\n",
                    __FILE__, __LINE__, tie_row_pts);
            exit(-1);
        }

        if (((nctlscan - 1) * tie_row_pts + 1) != file->nscan) {
            printf("-E- %s line %d: Sanity check failed - tie_rows (%d) x tie_row_pts (%d) != nscan (%d) in file %s\n",
                    __FILE__, __LINE__, (int) nctlscan, tie_row_pts, file->nscan, data->tieGeoCoordinatesFile);
            exit(-1);
        }
        if (((nctlpix - 1) * tie_col_pts + 1) != file->npix) {
            printf("-E- %s line %d: Sanity check failed - tie_cols (%d) x tie_col_pts (%d) != npix (%d) in file %s\n",
                    __FILE__, __LINE__, (int) nctlpix, tie_col_pts, file->nscan, data->tieGeoCoordinatesFile);
            exit(-1);
        }

        tlon = (int32_t *) calloc(nctlpix, sizeof (int32_t));
        tlat = (int32_t *) calloc(nctlpix, sizeof (int32_t));
        tsenz = (uint32_t *) calloc(nctlpix, sizeof (uint32_t));
        tsena = (int32_t *) calloc(nctlpix, sizeof (int32_t));
        tsolz = (uint32_t *) calloc(nctlpix, sizeof (uint32_t));
        tsola = (int32_t *) calloc(nctlpix, sizeof (int32_t));

        tielon = (double *) calloc(nctlpix, sizeof (double));
        tielat = (double *) calloc(nctlpix, sizeof (double));
        senvecx = (double *) calloc(nctlpix, sizeof (double));
        senvecy = (double *) calloc(nctlpix, sizeof (double));
        senvecz = (double *) calloc(nctlpix, sizeof (double));
        solvecx = (double *) calloc(nctlpix, sizeof (double));
        solvecy = (double *) calloc(nctlpix, sizeof (double));
        solvecz = (double *) calloc(nctlpix, sizeof (double));
        dist = (double *) calloc(nctlpix, sizeof (double));
        sdist = (double *) calloc(nctlpix, sizeof (double));

        lon = (int32_t *) calloc(npix, sizeof (int32_t));
        lat = (int32_t *) calloc(npix, sizeof (int32_t));

        detector_index = (int16_t*) calloc(npix, sizeof (int16_t));

        retval = nc_inq_dimid(instrumentFileID, "detectors", &xid);
        retval = nc_inq_dimlen(instrumentFileID, xid, &detectors);
        //printf("RJH: instrumentFile: detectors=%d \n",(int)detectors);

        detectorTmpFloat = (float*) allocateMemory(nbands * detectors * sizeof (float), "detectorTmpFloat");
        lambda0 = (float*) allocateMemory(nbands * detectors * sizeof (float), "lambda0");
        solar_flux = (float*) allocateMemory(nbands * detectors * sizeof (float), "solar_flux");

        rad_data = (unsigned int *) calloc(npix, sizeof (rad_data)); //BYSCAN

        start[0] = 0;
        start[1] = 0;
        start[2] = 0;
        count[0] = nbands;
        count[1] = detectors;
        count[2] = 0;

        if (input->rad_opt != 0) {
            retval = nc_inq_varid(instrumentFileID, "lambda0", &xid);
            if (retval != NC_NOERR) {
                fprintf(stderr,
                        "-E- %s line %d: nc_get_varid failed for file, %s  field, %s.\n",
                        __FILE__, __LINE__, data->instrumentFile, "lambda0");
                exit(FATAL_ERROR);
            }
            retval = nc_get_vara_float(instrumentFileID, xid, start, count, detectorTmpFloat);
            if (retval != NC_NOERR) {
                fprintf(stderr,
                        "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                        __FILE__, __LINE__, data->instrumentFile, "lambda0");
                exit(FATAL_ERROR);
            }
            // swap axis of the array
            for (i = 0; i < nbands; i++)
                for (j = 0; j < detectors; j++)
                    lambda0[j * nbands + i] = detectorTmpFloat[i * detectors + j];

            retval = nc_inq_varid(instrumentFileID, "solar_flux", &xid);
            if (retval != NC_NOERR) {
                fprintf(stderr,
                        "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                        __FILE__, __LINE__, data->instrumentFile, "solar_flux");
                exit(FATAL_ERROR);
            }
            retval = nc_get_vara_float(instrumentFileID, xid, start, count, detectorTmpFloat);
            if (retval != NC_NOERR) {
                fprintf(stderr,
                        "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                        __FILE__, __LINE__, data->instrumentFile, "solar_flux");
                exit(FATAL_ERROR);
            }
            // swap axis of the array
            for (i = 0; i < nbands; i++)
                for (j = 0; j < detectors; j++)
                    solar_flux[j * nbands + i] = detectorTmpFloat[i * detectors + j];
            free(detectorTmpFloat);

            char *tmp_str;
            char filename[FILENAME_MAX];
            if ((tmp_str = getenv("OCDATAROOT")) == NULL) {
                printf("OCDATAROOT environment variable is not defined.\n");
                exit(1);
            }
            strcpy(filename, tmp_str);
            strcat(filename, "/");
            strcat(filename, sensorId2SensorDir(l1rec->l1file->sensorID));
            strcat(filename, "/");
            strcat(filename, subsensorId2SubsensorDir(l1rec->l1file->subsensorID));
            strcat(filename, "/cal/");
            strcat(filename, BANDINFO_FILENAME);

            smile_init(nbands, detectors, filename, lambda0, solar_flux);
        } // if rad_opt

    } // first call


    //      set time for this scan - if scan_start_time value not properly set, estimate from scene start time.
    if (scan_start_tai[scan] > 0) {
        l1rec->scantime = scan_start_tai[scan] / 1000000.0 + tai2unixoffset;
        lastvalidtime = l1rec->scantime;
        if (scan > 0)
            time_interval = (scan_start_tai[scan] - lastvalidtime) / (scan - lastvalidscan);
        lastvalidscan = scan;
    } else {
        l1rec->scantime = lastvalidtime + (time_interval * (scan - lastvalidscan));
        lastvalidtime = l1rec->scantime;
    }

    start[0] = scan;
    start[1] = 0;
    start[2] = 0;
    count[0] = 1;
    count[1] = nctlpix;
    count[2] = 0;

    retval = nc_inq_varid(tcoordFileID, "longitude", &xid);
    retval = nc_get_att_float(tcoordFileID, xid, "scale_factor", &scale_lon);
    retval = nc_get_vara_int(tcoordFileID, xid, start, count, tlon);
    if (retval != NC_NOERR) {
        fprintf(stderr,
                "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                __FILE__, __LINE__, data->tieGeoCoordinatesFile, "lon");
        exit(FATAL_ERROR);
    }
    retval = nc_inq_varid(tcoordFileID, "latitude", &yid);
    retval = nc_get_att_float(tcoordFileID, yid, "scale_factor", &scale_lat);
    retval = nc_get_vara_int(tcoordFileID, yid, start, count, tlat);
    if (retval != NC_NOERR) {
        fprintf(stderr,
                "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                __FILE__, __LINE__, data->tieGeoCoordinatesFile, "lat");
        exit(FATAL_ERROR);
    }

    retval = nc_inq_varid(geoFileID, "OAA", &band_id);
    retval = nc_get_att_float(geoFileID, band_id, "scale_factor", &scale_sena);
    retval = nc_get_vara_int(geoFileID, band_id, start, count, tsena);
    if (retval != NC_NOERR) {
        fprintf(stderr,
                "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                __FILE__, __LINE__, data->tieGeoCoordinatesFile, "sena");
        exit(FATAL_ERROR);
    }
    retval = nc_inq_varid(geoFileID, "OZA", &band_id);
    retval = nc_get_att_float(geoFileID, band_id, "scale_factor", &scale_senz);
    retval = nc_get_vara_uint(geoFileID, band_id, start, count, tsenz);
    if (retval != NC_NOERR) {
        fprintf(stderr,
                "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                __FILE__, __LINE__, data->tieGeoCoordinatesFile, "senz");
        exit(FATAL_ERROR);
    }
    retval = nc_inq_varid(geoFileID, "SAA", &band_id);
    retval = nc_get_att_float(geoFileID, band_id, "scale_factor", &scale_sola);
    retval = nc_get_vara_int(geoFileID, band_id, start, count, tsola);
    if (retval != NC_NOERR) {
        fprintf(stderr,
                "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                __FILE__, __LINE__, data->tieGeoCoordinatesFile, "sola");
        exit(FATAL_ERROR);
    }
    retval = nc_inq_varid(geoFileID, "SZA", &band_id);
    retval = nc_get_att_float(geoFileID, band_id, "scale_factor", &scale_solz);
    retval = nc_get_vara_uint(geoFileID, band_id, start, count, tsolz);
    if (retval != NC_NOERR) {
        fprintf(stderr,
                "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                __FILE__, __LINE__, data->tieGeoCoordinatesFile, "solz");
        exit(FATAL_ERROR);
    }

    for (ip = 0; ip < nctlpix; ip++) {
        tielat[ip] = scale_lat * tlat[ip];
        tielon[ip] = scale_lon * tlon[ip];
        tiesena = scale_sena * tsena[ip];
        tiesenz = scale_senz * tsenz[ip];
        /*  */
        sinz = sin(deg2rad(tiesenz));
        senvecx[ip] = sinz * cos(deg2rad(tiesena));
        senvecy[ip] = sinz * sin(deg2rad(tiesena));
        senvecz[ip] = cos(deg2rad(tiesenz));
        /* */
        tiesola = scale_sola * tsola[ip];
        tiesolz = scale_solz * tsolz[ip];
        /*  */
        sinz = sin(deg2rad(tiesolz));
        solvecx[ip] = sinz * cos(deg2rad(tiesola));
        solvecy[ip] = sinz * sin(deg2rad(tiesola));
        solvecz[ip] = cos(deg2rad(tiesolz));

        dist[ip] = angular_distance(tielat[ip], tielon[ip], tielat[0], tielon[0]);
        //printf("RJH: TIE: ip=%d dist=%f lon=%lf lat=%lf %lf %lf solz=%lf sola=%lf senz=%lf sena=%lf\n",ip,dist[ip],tielon[ip],tielat[ip],tielon[0],tielat[0], tiesolz[ip],tiesola[ip],tiesenz[ip],tiesena[ip] );

    }

    start[0] = scan;
    start[1] = 0;
    start[2] = 0;
    count[0] = 1;
    count[1] = npix;
    count[2] = 0;

    //until geolocation is read, set fill values -
    for (ip = spix; ip < npix; ip++) {
        l1rec->lon[ip] = -999;
        l1rec->lat[ip] = -999;

        l1rec->solz[ip] = -999; //solz[scan * npix + ip];
        l1rec->sola[ip] = -999; //sola[scan * npix + ip];
        l1rec->senz[ip] = -999; //senz[scan * npix + ip];
        l1rec->sena[ip] = -999; //sena[scan * npix + ip];
    }

    retval = nc_inq_varid(coordFileID, "longitude", &xid);
    if (retval != NC_NOERR) {
        fprintf(stderr,
                "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                __FILE__, __LINE__, data->geoCoordinatesFile, "lon");
        exit(FATAL_ERROR);
    }
    retval = nc_get_vara_int(coordFileID, xid, start, count, lon);
    if (retval != NC_NOERR) {
        fprintf(stderr,
                "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                __FILE__, __LINE__, data->geoCoordinatesFile, "lat");
        exit(FATAL_ERROR);
    }
    retval = nc_inq_varid(coordFileID, "latitude", &yid);
    if (retval != NC_NOERR) {
        fprintf(stderr,
                "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                __FILE__, __LINE__, data->geoCoordinatesFile, "lat");
        exit(FATAL_ERROR);
    }
    retval = nc_get_vara_int(coordFileID, yid, start, count, lat);
    if (retval != NC_NOERR) {
        fprintf(stderr,
                "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                __FILE__, __LINE__, data->geoCoordinatesFile, "lat");
        exit(FATAL_ERROR);
    }

    retval = nc_inq_varid(instrumentFileID, "detector_index", &xid);
    if (retval != NC_NOERR) {
        fprintf(stderr,
                "-E- %s line %d: nc_get_varid failed for file, %s  field, %s.\n",
                __FILE__, __LINE__, data->instrumentFile, "detector_index");
        exit(FATAL_ERROR);
    }
    retval = nc_get_vara_short(instrumentFileID, xid, start, count, detector_index);
    if (retval != NC_NOERR) {
        fprintf(stderr,
                "-E- %s line %d: nc_get_vara_short failed for file, %s  field, %s.\n",
                __FILE__, __LINE__, data->instrumentFile, "detector_index");
        exit(FATAL_ERROR);
    }
    for (i = 0; i < npix; i++)
        l1rec->pixdet[i] = detector_index[i];


    for (i = 0; i < 6; i++)
        spline[i] = gsl_spline_alloc(gsl_interp_cspline, nctlpix);

    spl_acc = gsl_interp_accel_alloc();

    // Sort distances and corresponding elevation values
    memcpy(sdist, dist, sizeof (double)*nctlpix);
    gsl_sort2(sdist, 1, solvecx, 1, nctlpix);
    // Initiate spline
    gsl_spline_init(spline[0], sdist, solvecx, nctlpix);

    memcpy(sdist, dist, sizeof (double)*nctlpix);
    gsl_sort2(sdist, 1, solvecy, 1, nctlpix);
    gsl_spline_init(spline[1], sdist, solvecy, nctlpix);

    memcpy(sdist, dist, sizeof (double)*nctlpix);
    gsl_sort2(sdist, 1, solvecz, 1, nctlpix);
    gsl_spline_init(spline[2], sdist, solvecz, nctlpix);

    memcpy(sdist, dist, sizeof (double)*nctlpix);
    gsl_sort2(sdist, 1, senvecx, 1, nctlpix);
    gsl_spline_init(spline[3], sdist, senvecx, nctlpix);

    memcpy(sdist, dist, sizeof (double)*nctlpix);
    gsl_sort2(sdist, 1, senvecy, 1, nctlpix);
    gsl_spline_init(spline[4], sdist, senvecy, nctlpix);

    memcpy(sdist, dist, sizeof (double)*nctlpix);
    gsl_sort2(sdist, 1, senvecz, 1, nctlpix);
    gsl_spline_init(spline[5], sdist, senvecz, nctlpix);

    for (ip = spix; ip < npix; ip++) {

        l1rec->lon[ip] = lon[ip] * scale_lon;
        l1rec->lat[ip] = lat[ip] * scale_lat;

        xdist = angular_distance(l1rec->lat[ip], l1rec->lon[ip], tielat[0], tielon[0]);

        if (fabs(xdist) < 1.0e-7) {
            //printf("RJH:XDIST: ip=%d dist=%lf lon=%lf lat=%lf %lf %lf \n",ip,xdist,l1rec->lon[ip],l1rec->lat[ip],tielon[0],tielat[0]);
            l1rec->sola[ip] = scale_sola * tsola[ip];
            l1rec->solz[ip] = scale_solz * tsolz[ip];
            l1rec->sena[ip] = scale_sena * tsena[ip];
            l1rec->senz[ip] = scale_senz * tsenz[ip];
        } else if (xdist <= dist[nctlpix - 1]) {

            xvec = (float) gsl_spline_eval(spline[0], xdist, spl_acc);
            yvec = (float) gsl_spline_eval(spline[1], xdist, spl_acc);
            zvec = (float) gsl_spline_eval(spline[2], xdist, spl_acc);
            l1rec->solz[ip] = (float) rad2deg(acos(zvec));
            l1rec->sola[ip] = (float) rad2deg(atan2(yvec, xvec));

            xvec = (float) gsl_spline_eval(spline[3], xdist, spl_acc);
            yvec = (float) gsl_spline_eval(spline[4], xdist, spl_acc);
            zvec = (float) gsl_spline_eval(spline[5], xdist, spl_acc);
            l1rec->senz[ip] = (float) rad2deg(acos(zvec));
            l1rec->sena[ip] = (float) rad2deg(atan2(yvec, xvec));

            //printf("solz=%lf sola=%lf senz=%lf sena=%lf\n", l1rec->solz[ip],l1rec->sola[ip],l1rec->senz[ip],l1rec->sena[ip] );
        }
    }

    // read in radiance data

    for (ib = 0; ib < nbands; ib++) {
        retval = nc_inq_varid(olci_sd[ib], data->olci_varname[ib], &band_id);
        if (retval != NC_NOERR) {
            fprintf(stderr,
                    "-E- %s line %d: nc_inq_varid failed for file, %s  field, %s.\n",
                    __FILE__, __LINE__, data->olci_radfiles[ib], varnam);
            exit(FATAL_ERROR);
        }
        retval = nc_get_att_float(olci_sd[ib], band_id, "scale_factor", &scale_rad);
        if (retval != NC_NOERR) {
            fprintf(stderr,
                    "-E- %s line %d: nc_get_att_float failed for file, %s  field, %s.\n",
                    __FILE__, __LINE__, data->olci_radfiles[ib], varnam);
            exit(FATAL_ERROR);
        }
        retval = nc_get_vara_uint(olci_sd[ib], band_id, start, count, rad_data); //BYSCAN
        if (retval != NC_NOERR) {
            fprintf(stderr,
                    "-E- %s line %d: nc_get_vara_float failed for file, %s  field, %s.\n",
                    __FILE__, __LINE__, data->olci_radfiles[ib], varnam);
            exit(FATAL_ERROR);
        }
        // copy to Lt record.
        for (ip = spix; ip < npix; ip++) {
            ipb = ip * nbands + ib;
            l1rec->Lt[ipb] = rad_data[ip] * scale_rad / 10.; //BYSCAN

            // mark negative input data as HILT
            // navfail commented out for Lt < 0 - too limiting for hyperspectral - RJH
            if (l1rec->Lt[ipb] < 0.0)
                l1rec->Lt[ipb] = 0.0001;
            //                l1rec->navfail[ip] = 1;
        }
    } // for ib

    gsl_interp_accel_free(spl_acc);

    l1rec->npix = file->npix;
    l1rec->l1file->terrain_corrected = 1;

    return (LIFE_IS_GOOD);
}

int closel1_olci(filehandle *file) {
    int retval;
    int i;

    retval = nc_close(geoFileID);
    retval += nc_close(coordFileID);
    retval += nc_close(tcoordFileID);
    retval += nc_close(instrumentFileID);
    for (i = 0; i < file->nbands; i++)
        retval += nc_close(olci_sd[i]);

    if (retval != 0) {
        fprintf(stderr, "-E- %s line %d: nc_close failed for one or more OLCI files.\n",
                __FILE__, __LINE__);
        return (1);
    }
    free(scan_start_tai);

    return (LIFE_IS_GOOD);
}

