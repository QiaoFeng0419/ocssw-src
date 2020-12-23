/* ============================================================================ */
/* module l1b_oci.c - functions to read OCI L1B for MSL12                       */
/* Written By:  Steve Lockhart SAIC August 2018                                 */
/*      -Started with l1a_hawkeye.c and made changes to support OCI.            */
/*                                                                              */
/* ============================================================================ */

/* Issues:
        -Assume num_pixels = ccd_pixels = SWIR_pixels
        -Temporarily set tot_num_bands = 62 (since temporarily using copy of ocia share stuff)
        -No scan_delta_time_ms, so use secs of day (with wrap-around?)
        -Get Lt
        -Skip over first two red bands (600, 605), as they overlap with last two blue bands
         i.e. start at the third red band. So, this affects the total number of bands.
        -Combine the high-res and low-res SWIR bands?
        -Get sensor_band_parameters
        -Review Ltir: what I carried over from VIIRS reader may not be appropriate for OCI.
        -Make sure default (FILL) values are correct wrt what is in the input file.
*/

// Includes from l1_viirs_nc.c
#include <nc4utils.h>
#include "l12_proto.h"
#include "libnav.h"
#include <productInfo.h>

// Includes from demo_cal.c
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <math.h>
#include <timeutils.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_spline.h>
#include <gsl/gsl_sort.h>
#include <gsl/gsl_statistics.h>
#include <stdbool.h>

// New includes
#include "l1b_oci.h"


// MBAND_NUM_DETECTORS changed to 1 
#define MBAND_NUM_DETECTORS 1


// Declarations from l1_viirs_nc.c
static int32 prevScan = -1;

//static int geoFileId;
static int geoGeolocationGrp;
static int l1bScanLineGrp;
static int l1bObservationGrp;
static int lonId, latId, senzId, senaId, solzId, solaId, HAMSideId, scanQualityId;

static float *Fobar; // reflectance to radiance conversion factors
static int extract_pixel_start = 0;

static short *tmpShort;
static unsigned char *tmpByte;
static int nline;
static size_t num_scans, num_pixels;
static size_t num_blue_bands, num_red_bands, num_SWIR_bands, tot_num_bands;

static int firstCall = 1;
static double starttime;
static double lastvalidtime;
static int lastvalidscan = 0;
static double time_interval;

static float latGeoFillValue = -999.9;
static float lonGeoFillValue = -999.9;
static short senzGeoFillValue = -32768;
static short senaGeoFillValue = -32768;
static short solzGeoFillValue = -32768;
static short solaGeoFillValue = -32768;

static float latL2FillValue = -999.0;
static float lonL2FillValue = -999.0;
static float senzL2FillValue = -32767;
static float senaL2FillValue = -32767;
static float solzL2FillValue = -32767;
static float solaL2FillValue = -32767;

// Declarations added for OCI L1B
static int Lt_blue_varid, Lt_red_varid, Lt_SWIR_varid; 
static double **Lt_blue;                                            //[num_blue_bands][num_pixels], This scan
static double **Lt_red;                                             //[num_red_bands][num_pixels], This scan
static double **Lt_SWIR;                                            //[num_SWIR_bands][num_pixels], This scan
static double **Lt;                                                 //[tot_num_bands][num_pixels], This scan


/**
 * Open the OCI L1B file and perform some one-time tasks (as opposed to tasks that
 * are per scan), including:
 *   -Get L1B dimensions num_scans, num_bands, num_pixels (static).
 *    Allocate memory for some static arrays based upon these dimensions.
 *   -Get L1B group ids e.g. l1bScanLineGrp, l1bObservationGrp and it's 8 "band_%d" var ids (static)
 *   -Get L1B "time_coverage_start" and "time_coverage_end" to derive time_interval (static) etc.
 *

 * Get   
 * @param file
 * @return 
 */
int openl1b_oci(filehandle * file) {
    char *fltime;

    int ncid_L1B, dimid, status;
    size_t att_len;
    int orbit_number;
    
    // Open the netcdf4 input file
    printf("Opening oci l1b file\n");
    status = nc_open(file->name, NC_NOWRITE, &ncid_L1B);
    if (status != NC_NOERR) {
        fprintf(stderr, "-E- %s line %d: nc_open(%s) failed.\n",
                __FILE__, __LINE__, file->name);
        return (1);
    }
    
    // Get dims from L1B file: num_scans, num_bands, num_pixels
    // num_scans
    status = nc_inq_dimid(ncid_L1B, "number_of_scans", &dimid);
    if (status != NC_NOERR) {
        fprintf(stderr, "-E- Error reading num_scans.\n");
        exit(EXIT_FAILURE);
    };
    nc_inq_dimlen(ncid_L1B, dimid, &num_scans);
    // num_blue_bands
    status = nc_inq_dimid(ncid_L1B, "blue_bands", &dimid);
    if (status != NC_NOERR) {
        fprintf(stderr, "-E- Error reading num_blue_bands.\n");
        exit(EXIT_FAILURE);
    };
    nc_inq_dimlen(ncid_L1B, dimid, &num_blue_bands);
    // num_red_bands
    status = nc_inq_dimid(ncid_L1B, "red_bands", &dimid);
    if (status != NC_NOERR) {
        fprintf(stderr, "-E- Error reading num_red_bands.\n");
        exit(EXIT_FAILURE);
    };
    nc_inq_dimlen(ncid_L1B, dimid, &num_red_bands);
    // num_SWIR_bands
    status = nc_inq_dimid(ncid_L1B, "SWIR_bands", &dimid);
    if (status != NC_NOERR) {
        fprintf(stderr, "-E- Error reading num_SWIR_bands.\n");
        exit(EXIT_FAILURE);
    };
    nc_inq_dimlen(ncid_L1B, dimid, &num_SWIR_bands);
    // num_pixels
    status = nc_inq_dimid(ncid_L1B, "ccd_pixels", &dimid);
    if (status != NC_NOERR) {
        fprintf(stderr, "-E- Error reading num_pixels.\n");
        exit(EXIT_FAILURE);
    };
    nc_inq_dimlen(ncid_L1B, dimid, &num_pixels);
    
    // Derive nline (See issue above.) This line is from l1_viirs_nc.c.
    nline = num_scans * MBAND_NUM_DETECTORS;
    
    // Now that we know dims, prep for additional one-time reads by allocating memory
    // for arrays (declared static above). (Memory is freed in closel1b_oci.)
    //tot_num_bands = num_blue_bands + num_red_bands + num_SWIR_bands;
    tot_num_bands = 62;
    
    // Get group id from L1B file for GROUP scan_line_attributes.
    if ((nc_inq_grp_ncid(ncid_L1B, "scan_line_attributes", &l1bScanLineGrp)) == NC_NOERR) {
    } else {
        fprintf(stderr, "-E- Error finding scan_line_attributes.\n");
        exit(EXIT_FAILURE);
    }   
    
    // get mirror side from this GROUP
    status = nc_inq_varid(l1bScanLineGrp, "HAM_side", &HAMSideId);
    check_err(status, __LINE__, __FILE__);
    // get scan quality from this group
    status = nc_inq_varid(l1bScanLineGrp, "scan_quality_flags", &scanQualityId);
    check_err(status, __LINE__, __FILE__);    

    // Get attribute values (from l1_viirs_nc.c) e.g. "time_coverage_start" and "time_coverage_end" to derive
    // time_interval etc. Note that time_coverage_end is the START of the last scan.
    nc_type vr_type;            // attribute type 
    size_t vr_len;              // attribute length 
    // Comment out extract_pixel_start/stop stuff
    /*
    if ((nc_inq_att(fileID, NC_GLOBAL, "extract_pixel_start", &vr_type, &vr_len) == 0)) {
        status = nc_get_att_int(fileID, NC_GLOBAL, "extract_pixel_start", &extract_pixel_start);
        check_err(status, __LINE__, __FILE__);
        extract_pixel_start--; // Attribute is one-based
        status = nc_get_att_int(fileID, NC_GLOBAL, "extract_pixel_stop", &extract_pixel_stop);
        check_err(status, __LINE__, __FILE__);
        extract_pixel_stop--; // Attribute is one-based
        if (npix != (extract_pixel_stop - extract_pixel_start + 1)) {
            fprintf(stderr, "-E- Problem with the extracted L1B file pixel dimension.\n");
            printf("    npix(%d), extract_pixel_stop(%d), extract_pixel_start(%d) do not work together.\n",
                    npix, extract_pixel_stop, extract_pixel_start);
            exit(EXIT_FAILURE);
        }
    }
    */

    if (want_verbose) {
        printf("OCI L1B Npix  :%d Nlines:%d\n", (int)num_pixels, nline);
    } // want_verbose

    // get start and end time
    status = nc_inq_attlen(ncid_L1B, NC_GLOBAL, "time_coverage_start", &att_len);
    check_err(status, __LINE__, __FILE__);

    // allocate required space before retrieving values 
    fltime = (char *) malloc(att_len + 1); // + 1 for trailing null 

    // get attribute values 
    status = nc_get_att_text(ncid_L1B, NC_GLOBAL, "time_coverage_start", fltime);
    check_err(status, __LINE__, __FILE__);
    fltime[att_len] = '\0';
    //    isodate2ydmsec(fltime, (int32_t *) &year,(int32_t *) &day, (int32_t *) &msec);

    // Convert "time_coverage_start" ISO string to unix (seconds since 1/1/1970)
    starttime = lastvalidtime = isodate2unix(fltime);
    free(fltime);

    status = nc_inq_attlen(ncid_L1B, NC_GLOBAL, "time_coverage_end", &att_len);
    check_err(status, __LINE__, __FILE__);

    // allocate required space before retrieving values 
    fltime = (char *) malloc(att_len + 1); // + 1 for trailing null

    // get attribute values 
    status = nc_get_att_text(ncid_L1B, NC_GLOBAL, "time_coverage_end", fltime);
    check_err(status, __LINE__, __FILE__);
    fltime[att_len] = '\0';

    // Convert "time_coverage_stop" ISO string to unix (seconds since 1/1/1970)
    double stoptime = isodate2unix(fltime);
    free(fltime);

    // time_interval may be used in readl1b_oci if there is not a good scan time (per scan)
    time_interval = (stoptime - starttime)/(num_scans-1); // secs per scan

    if ((nc_inq_att(ncid_L1B, NC_GLOBAL, "orbit_number", &vr_type, &vr_len) == NC_NOERR)) {
        status = nc_get_att_int(ncid_L1B, NC_GLOBAL, "orbit_number", &orbit_number);
        check_err(status, __LINE__, __FILE__);
    } else {
        orbit_number = 0;
    }

    
    // Identify the "observation_data" GROUP and its "Lt_" vars, to be used later by readl1b_oci
    // Store the ids in static variables so we don't have to do nc_inq_grp_ncid per scan.
    if ((nc_inq_grp_ncid(ncid_L1B, "observation_data", &l1bObservationGrp)) == NC_NOERR) {
        // Get varids for each of the Lt_*
        status = nc_inq_varid(l1bObservationGrp, "Lt_blue", &Lt_blue_varid);
        check_err(status, __LINE__, __FILE__);
        status = nc_inq_varid(l1bObservationGrp, "Lt_red", &Lt_red_varid);
        check_err(status, __LINE__, __FILE__);
        status = nc_inq_varid(l1bObservationGrp, "Lt_SWIR", &Lt_SWIR_varid);
        check_err(status, __LINE__, __FILE__);
    } else {
        fprintf(stderr, "-E- Error finding observation_data.\n");
        exit(EXIT_FAILURE);
    }

    file->sd_id = ncid_L1B;
    file->nbands = tot_num_bands;
    file->npix = num_pixels;
    file->nscan = nline;
    file->ndets = MBAND_NUM_DETECTORS;
    file->terrain_corrected = 1; // presumed.
    file->orbit_number = orbit_number;
    strcpy(file->spatialResolution, "??? m");

    rdsensorinfo(file->sensorID, input->evalmask,
            "Fobar", (void **) &Fobar);

    if (want_verbose)
        printf("file->nbands = %d\n", (int) file->nbands);

    // Setup geofile pointers
    status = nc_inq_grp_ncid(ncid_L1B, "geolocation_data", &geoGeolocationGrp);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_varid(geoGeolocationGrp, "longitude", &lonId);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_var_fill(geoGeolocationGrp, lonId, NULL, &lonGeoFillValue);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_varid(geoGeolocationGrp, "latitude", &latId);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_var_fill(geoGeolocationGrp, latId, NULL, &latGeoFillValue);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_varid(geoGeolocationGrp, "sensor_zenith", &senzId);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_var_fill(geoGeolocationGrp, senzId, NULL, &senzGeoFillValue);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_varid(geoGeolocationGrp, "sensor_azimuth", &senaId);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_var_fill(geoGeolocationGrp, senaId, NULL, &senaGeoFillValue);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_varid(geoGeolocationGrp, "solar_zenith", &solzId);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_var_fill(geoGeolocationGrp, solzId, NULL, &solzGeoFillValue);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_varid(geoGeolocationGrp, "solar_azimuth", &solaId);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_var_fill(geoGeolocationGrp, solaId, NULL, &solaGeoFillValue);
    check_err(status, __LINE__, __FILE__);
    //status = nc_inq_varid(geoGeolocationGrp, "quality_flag", &pixelQualityId);
    //check_err(status, __LINE__, __FILE__);

    //status = nc_inq_grp_ncid(ncid_L1B, "navigation_data", &geoNavigationGrp);
    //check_err(status, __LINE__, __FILE__);
    //angId not yet in GEO file
    //status = nc_inq_varid(geoNavigationGrp, "att_ang_mid", &angId);
    //check_err(status, __LINE__, __FILE__);
    //status = nc_inq_varid(geoNavigationGrp, "orb_pos", &posId);
    //check_err(status, __LINE__, __FILE__);
    //status = nc_inq_varid(geoNavigationGrp, "orb_vel", &velId);
    //check_err(status, __LINE__, __FILE__);

    //status = nc_inq_grp_ncid(ncid_L1B, "scan_line_attributes", &geoScanLineGrp);
    //check_err(status, __LINE__, __FILE__);
    // The following field is not yet in the GEO file
    //status = nc_inq_varid(geoScanLineGrp, "scan_quality", &scanQualityId);
    //check_err(status, __LINE__, __FILE__);


    // Setup the fill values for the geo products
    // Replaced HAWKEYE with OCI. 
    productInfo_t* info = allocateProductInfo();
    status = findProductInfo("lat", OCI, info);
    if (status)
        latL2FillValue = info->fillValue;
    status = findProductInfo("lon", OCI, info);
    if (status)
        lonL2FillValue = info->fillValue;
    status = findProductInfo("sena", OCI, info);
    if (status)
        senaL2FillValue = info->fillValue;
    status = findProductInfo("senz", OCI, info);
    if (status)
        senzL2FillValue = info->fillValue;
    status = findProductInfo("sola", OCI, info);
    if (status)
        solaL2FillValue = info->fillValue;
    status = findProductInfo("solz", OCI, info);
    if (status)
        solzL2FillValue = info->fillValue;
    freeProductInfo(info);

    return (LIFE_IS_GOOD);
}


/**
 * Read the specified scan line from the specified L1B file. 
 * For each scan, get scan_delta_time_ms. 
 * Store Lt in l1rec->Lt.
 * Read GEO file.
 * 
 * 
 * @param file
 * @param line
 * @param l1rec
 * @return 
 */
int readl1b_oci(filehandle *file, int32_t line, l1str *l1rec) {

    // Declarations from l1_viirs_nc.c
    int32 ip;
    int i;
    double scan_sec;
    int16 scan_year, scan_day;

    int status;
    size_t start[] = { 0, 0, 0 };
    size_t count[] = { 1, 1, 1 };

    int32 scan = line / MBAND_NUM_DETECTORS;
    
    // Additional declarations
    int band_num, pixel_num;
    int blue_band_num, red_band_num, SWIR_band_num;
    int16 scan_month, scan_dom;
    double scan_delta_time_ms = 0;                                      // ms since start of image per scan

    //printf("reading oci l1b file\n");
    for (ip = 0; ip < num_pixels; ip++) {
        l1rec->pixnum[ip] = ip + extract_pixel_start;
    }

    // If first call, 
    if (firstCall) {
        firstCall = 0;  

        // One-time memory allocations
        tmpShort = (short *) malloc(num_pixels * sizeof(short));
        tmpByte = (unsigned char *) malloc(num_pixels);
        Lt = allocate2d_double(tot_num_bands, num_pixels);
        Lt_blue = allocate2d_double(num_blue_bands, num_pixels);
        Lt_red = allocate2d_double(num_red_bands, num_pixels);
        Lt_SWIR = allocate2d_double(num_SWIR_bands, num_pixels);
    }
    
    //    l1rec->sensorID = file->sensorID;
    l1rec->npix = file->npix;
     
    // Time
    // Get delta_time 
    //start[0] = line;
    //count[0] = 1;                                                                       // 1 scan at a time
    //count[1] = 0;
    //count[2] = 0;
    //status = nc_inq_varid(l1bScanLineGrp, "delta_time", &varid);
    //if (status != NC_NOERR) {
    //    fprintf(stderr, "-E- Error finding scan_delta_time_ms.\n");
    //    exit(EXIT_FAILURE);
    //} 
    //status = nc_get_vara_double(l1bScanLineGrp, varid, start, count, &scan_delta_time_ms);
    //if (status != NC_NOERR) {
    //    fprintf(stderr, "-E- Error reading scan_delta_time_ms.\n");
    //    exit(EXIT_FAILURE);
    //};
    // Set lastvalidtime, the start of this scan (in secs since 1/1/1970)
    if (scan_delta_time_ms > 0) {
        // starttime is the start of this granule (L1B "time_coverage_start"), converted to secs since 1/1/1970
        // scan_delta_time_ms is when this scan starts (number of milliseconds since the start of this granule)
        lastvalidtime = starttime + scan_delta_time_ms/1000;
    } else {
        lastvalidtime = lastvalidtime + (time_interval * (line - lastvalidscan));      
    }
    // Set scan_year, scan_day, scan_sec
    unix2yds(lastvalidtime, &scan_year, &scan_day, &scan_sec);
    // Store lastvalidtime in l1rec->scantime
    l1rec->scantime = lastvalidtime; 
    // Convert lastvalidtime to julian date
    yd2md(scan_year, scan_day, &scan_month, &scan_dom);
    
    // GROUP earth_view_data
    // Get Lt_blue[num_blue_bands][num_pixels] from this GROUP just for THIS scan  
    start[1] = line;
    count[1] = 1;     // 1 line at a time
    start[2] = 0;
    count[2] = num_pixels;
    for (blue_band_num=0; blue_band_num<num_blue_bands; blue_band_num++) {
        start[0] = blue_band_num;
        count[0] = 1; 
        status = nc_get_vara_double(l1bObservationGrp, Lt_blue_varid, start, count, &Lt_blue[blue_band_num][0]);
        if (status != NC_NOERR) {
            fprintf(stderr, "-E- Error reading Lt_blue.\n");
            exit(EXIT_FAILURE);
        }
    }
    
    
    // Get Lt_red[num_red_bands][num_pixels] from this GROUP just for THIS scan  
    start[1] = line;
    count[1] = 1;     // 1 line at a time
    start[2] = 0;
    count[2] = num_pixels;
    for (red_band_num=0; red_band_num<num_red_bands; red_band_num++) {
        start[0] = red_band_num;
        count[0] = 1; 
        status = nc_get_vara_double(l1bObservationGrp, Lt_red_varid, start, count, &Lt_red[red_band_num][0]);
        if (status != NC_NOERR) {
            fprintf(stderr, "-E- Error reading Lt_red.\n");
            exit(EXIT_FAILURE);
        }
    }
    
    // Get Lt_SWIR[num_SWIR_bands][num_pixels] from this GROUP just for THIS scan  
    start[1] = line;
    count[1] = 1;     // 1 line at a time
    start[2] = 0;
    count[2] = num_pixels;
    for (SWIR_band_num=0; SWIR_band_num<num_SWIR_bands; SWIR_band_num++) {
        start[0] = SWIR_band_num;
        count[0] = 1;
        status = nc_get_vara_double(l1bObservationGrp, Lt_SWIR_varid, start, count, &Lt_SWIR[SWIR_band_num][0]);
        if (status != NC_NOERR) {
            fprintf(stderr, "-E- Error reading Lt_SWIR.\n");
            exit(EXIT_FAILURE);
        }
    }
    
    // Set l1rec->Lt (fudged for now)
    for (pixel_num = 0; pixel_num < num_pixels; pixel_num++) {
        blue_band_num = 0;
        red_band_num = 0;
        SWIR_band_num = 0;
        for (band_num = 0; band_num < tot_num_bands; band_num++) {
            //ipb = ip * nbands + ib
            // Fudge for now the map from source bands to target bands e.g. take every other source band
            if (band_num < 30) {
                l1rec->Lt[pixel_num*tot_num_bands + band_num] = (float)Lt_blue[blue_band_num][pixel_num];
                blue_band_num = blue_band_num + 2;
            }
            if ((band_num >= 30) && (band_num < 60)) {
                l1rec->Lt[pixel_num*tot_num_bands + band_num] = (float)Lt_red[red_band_num][pixel_num];
                red_band_num = red_band_num + 2;
            }
            if (band_num >= 60) {
                l1rec->Lt[pixel_num*tot_num_bands + band_num] = (float)Lt_SWIR[SWIR_band_num][pixel_num];
                SWIR_band_num = SWIR_band_num + 2;
            }
        }
    }

    

    

    // first check the scan quality flag
    // 1   SCE_side_A_B
    // 2   SCE_side_invalid
    // 4   Sector_rotation
    // 8   Encoder_degraded
    // 16  SAA
    // 32  Solar_eclipse
    // 64  Lunar_eclipse
    // 128 HAM_side
    //
    // Sector_rotation
    short scanQualityWarnMask = 2 | 8 | 128;
    short scanQualityFailMask = 4;
    static short scanQualityFlag = 0;

    if (scan != prevScan) {
        start[0] = scan;
        status = nc_get_var1_short(l1bScanLineGrp, scanQualityId, start, &scanQualityFlag);
        check_err(status, __LINE__, __FILE__);
    }
    if (scanQualityFlag & scanQualityFailMask) {
        for (ip = 0; ip < num_pixels; ip++)
            l1rec->flags[ip] |= NAVFAIL;
        return 0;
    }
    if (scanQualityFlag & scanQualityWarnMask) {
        for (ip = 0; ip < num_pixels; ip++)
            l1rec->flags[ip] |= NAVWARN;
    }


    static unsigned char HAMSideVal = 0;
    if (scan != prevScan) {
        start[0] = scan;
        status = nc_get_var1_uchar(l1bScanLineGrp, HAMSideId, start, &HAMSideVal);
        check_err(status, __LINE__, __FILE__);
    }
    l1rec->mside = HAMSideVal;

    // set up to read all pixels of the line.
    start[0] = line;
    start[1] = 0;
    count[0] = 1;
    count[1] = num_pixels; // read all pixels

    status = nc_get_vara_float(geoGeolocationGrp, latId, start, count, l1rec->lat);
    check_err(status, __LINE__, __FILE__);
    for (i = 0; i < num_pixels; i++)
        if (l1rec->lat[i] == latGeoFillValue)
            l1rec->lat[i] = latL2FillValue;

    status = nc_get_vara_float(geoGeolocationGrp, lonId, start, count, l1rec->lon);
    check_err(status, __LINE__, __FILE__);
    for (i = 0; i < num_pixels; i++)
        if (l1rec->lon[i] == lonGeoFillValue)
            l1rec->lon[i] = lonL2FillValue;

    status = nc_get_vara_short(geoGeolocationGrp, solzId, start, count, tmpShort);
    check_err(status, __LINE__, __FILE__);
    for (i = 0; i < num_pixels; i++)
        if (tmpShort[i] == solzGeoFillValue)
            l1rec->solz[i] = solzL2FillValue;
        else
            l1rec->solz[i] = tmpShort[i] * 0.01;

    status = nc_get_vara_short(geoGeolocationGrp, solaId, start, count, tmpShort);
    check_err(status, __LINE__, __FILE__);
    for (i = 0; i < num_pixels; i++)
        if (tmpShort[i] == solaGeoFillValue)
            l1rec->sola[i] = solaL2FillValue;
        else
            l1rec->sola[i] = tmpShort[i] * 0.01;

    status = nc_get_vara_short(geoGeolocationGrp, senzId, start, count, tmpShort);
    check_err(status, __LINE__, __FILE__);
    for (i = 0; i < num_pixels; i++)
        if (tmpShort[i] == senzGeoFillValue)
            l1rec->senz[i] = senzL2FillValue;
        else
            l1rec->senz[i] = tmpShort[i] * 0.01;

    status = nc_get_vara_short(geoGeolocationGrp, senaId, start, count, tmpShort);
    check_err(status, __LINE__, __FILE__);
    for (i = 0; i < num_pixels; i++)
        if (tmpShort[i] == senaGeoFillValue)
            l1rec->sena[i] = senaL2FillValue;
        else
            l1rec->sena[i] = tmpShort[i] * 0.01;

    //status = nc_get_vara_float(geoNavigationGrp, angId, s3, c3, ang);
    //check_err(status, __LINE__, __FILE__);
    //status = nc_get_vara_float(geoNavigationGrp, posId, s3, c3, pos);
    //check_err(status, __LINE__, __FILE__);
    //status = nc_get_vara_float(geoNavigationGrp, velId, s3, c3, vel);
    //check_err(status, __LINE__, __FILE__);
    //for (i = 0; i < 3; i++) {
    //    pos[i] /= 1000.; // m   -> km
    //    vel[i] /= 1000.; // m/s -> km/s
    //}

    // Compute polarization rotation angles
    // Skipping for now since no ang [SBL]
    /* 
    float sen_mat[3][3], coeff[10];
    double mnorm[3];
    ocorient_(pos, vel, ang, sen_mat, coeff);
    for (i = 0; i < 3; i++)
        mnorm[i] = sen_mat[i][0];
    compute_alpha(l1rec->lon, l1rec->lat,
            l1rec->senz, l1rec->sena,
            mnorm, l1rec->npix, l1rec->alpha);
    */

    // Check pixel values 
    //status = nc_get_vara_uchar(geoGeolocationGrp, pixelQualityId, start, count, tmpByte);
    //check_err(status, __LINE__, __FILE__);
    // 1 Input_invalid
    // 2 Pointing_bad
    // 4 Terrain_bad
    unsigned char qualityFailMask = 1 || 2;
    unsigned char qualityWarnMask = 4;
    for (i = 0; i < num_pixels; i++) {
        if (tmpByte[i] & qualityFailMask)
            l1rec->flags[i] |= NAVFAIL;
        if (tmpByte[i] & qualityWarnMask)
            l1rec->flags[i] |= NAVWARN;
    }

    // Earth-sun distance correction for this scan
    static double esdist = -999.9;
    if (scan != prevScan) {
	int16_t year, day;
	double dsec;
	unix2yds(l1rec->scantime, &year, &day, &dsec);
	int32_t yr = (int32_t) year;
	int32_t dy = (int32_t) day;
	int32_t msec = (int32_t) (dsec * 1000.0);
        esdist = esdist_(&yr, &dy, &msec);
    }
    l1rec->fsol = pow(1.0 / esdist, 2);

    // Skipping this VIIRS-related stuff
    /* 
    int nbands = 16; //, nRSBbands = 10, nCIRbands = 1, nTEBbands = 5;

    // read in calibrated L1B data
    static float *l1bptrs[16] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    static char *bandType[16] = {"RSB", "RSB", "RSB", "RSB", "RSB", "RSB",
        "RSB", "RSB", "CIR", "RSB", "RSB", "TEB",
        "TEB", "TEB", "TEB", "TEB"};

    // Note: l1bptrs arrays are 3200 pixels wide
    int oldVerbose = want_verbose;
    want_verbose = 0;
    VcstViirsCal_calibrateMOD(line, nbands, l1bptrs);
    want_verbose = oldVerbose;
    */

    l1rec->detnum = line % file->ndets;

    // Skipping this VIIRS-related stuff
    /* 
    int irsb = 0, iteb = 0;
    int l1bptrs_scan_idx;
    for (ib = 0; ib < nbands; ib++) {

        // get specific f table cal correction  
        f_corr = (f_cal_corr == NULL) ? 1.0
                : f_cal_corr[ib][l1rec->detnum][l1rec->mside];

        if (strcmp(bandType[ib], "TEB") == 0) {

            for (ip = 0; ip < num_pixels; ip++) {
                ipb = ip * NBANDSIR + iteb;
                l1rec->Ltir[ipb] = 0;

                l1bptrs_scan_idx = l1rec->detnum * 3200 + ip + extract_pixel_start;

                if (l1bptrs[ib][l1bptrs_scan_idx] != -32767) {
                    l1rec->Ltir[ipb] = l1bptrs[ib][l1bptrs_scan_idx] / 10.0;

                    // Apply F-factor 
                    l1rec->Ltir[ipb] *= f_corr;
                }

            }
            iteb++;

        } else if (strcmp(bandType[ib], "CIR") == 0) {

            for (ip = 0; ip < num_pixels; ip++) {

                l1bptrs_scan_idx = l1rec->detnum * 3200 + ip + extract_pixel_start;

                if (l1bptrs[ib][l1bptrs_scan_idx] != -32767) {
                    l1rec->rho_cirrus[ip] = l1bptrs[ib][l1bptrs_scan_idx];

                    // Normalize reflectance by solar zenith angle 
                    l1rec->rho_cirrus[ip] /= cos(l1rec->solz[ip] / RADEG);

                    // Apply F-factor 
                    l1rec->rho_cirrus[ip] *= f_corr;
                }

            }

        } else if (strcmp(bandType[ib], "RSB") == 0) {

            l1rec->Fo[irsb] = Fobar[irsb] * l1rec->fsol;

            // copy to Lt record.
            for (ip = 0; ip < num_pixels; ip++) {
                ipb = ip * l1rec->l1file->nbands + irsb;

                l1bptrs_scan_idx = l1rec->detnum * 3200 + ip + extract_pixel_start;

                if (l1bptrs[ib][l1bptrs_scan_idx] != -32767) {
                    l1rec->Lt[ipb] = l1bptrs[ib][l1bptrs_scan_idx];

                    // convert from reflectance to radiance 
                    l1rec->Lt[ipb] *= l1rec->Fo[irsb] / PI;

                    // Apply F-factor 
                    l1rec->Lt[ipb] *= f_corr;
                }

            }
            irsb++;
        } // if RSB

    } // for ib
    */

    // Skipping for now to avoid "No brightness temperature conversion provided for this sensor" [SBL]
    //radiance2bt(l1rec, -1); // calculate brightness temperature

    // Bowtie stuff is specific to VIIRS, so skip it.
    //for (ip = 0; ip < num_pixels; ip++) {
    //    flag_bowtie_deleted(l1rec, ip, extract_pixel_start);
    //}

    prevScan = scan;
    
    
    return (LIFE_IS_GOOD);
}


/**
 * Close L1B file, GEO file, and free memory
 * @param file
 * @return 
 */
int closel1b_oci(filehandle *file) {
    int status;

    printf("Closing oci l1b file\n");
    status = nc_close(file->sd_id);
    check_err(status, __LINE__, __FILE__);

    // Free memory
    // From readl1b_oci
    if (tmpShort) free(tmpShort);
    if (tmpByte) free(tmpByte);                         
    if (Lt) free2d_double(Lt);  
    if (Lt_blue) free2d_double(Lt_blue);   
    if (Lt_red) free2d_double(Lt_red);  
    if (Lt_SWIR) free2d_double(Lt_SWIR);  
    // From openl1b_oci 


    return (LIFE_IS_GOOD);
}





