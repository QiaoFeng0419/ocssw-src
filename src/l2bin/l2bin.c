#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "hdf.h"
#include "netcdf.h"

#include <seaproto.h>
#include <meta_l3b.h>

#include "readL2scan.h"
#include "l2bin_input.h"
#include <timeutils.h>
#include <genutils.h>
#include <setupflags.h>
#include "passthebuck.h"
#include "sensorInfo.h"
#include "sensorDefs.h"
#include <nc4utils.h>
#include <ncdfbin_utils.h>

//#ifdef GSL
#include <gsl/gsl_sort.h>
#include <gsl/gsl_statistics.h>
//#endif

#include "dataday.h"

#define PI      3.141592653589793
#define MTILT_DIMS_2    20
#define LTILT_DIMS_2    2
#define MAX32BITVALUE 4294967295
#define MAXALLOCPERBIN 20
#define BINCHECK -1
//#define BINCHECK -2
#define EARTH_RADIUS 6378.14

/* Global variables */
static instr input;
static l2_prod l2_str[MAXNFILES];
static int64_t *numbin;
static int64_t *basebin;
static int32_t nrows = -1;
static double *scan_frac;
static int16_t *nobs;
static float32 **data_values;
static int16_t **file_index;
static uint8_t **data_quality;
static float64 **time_value;
static char prod_avg[64];


#define VERSION "5.0.0"
#define PROGRAM "l2bin"

/*
 *
  Revision 4.4.1 2018-09-12
  Remove support for HEALPIX
  S. Bailey, G. Fireman

  Revision 4.4.0 11/29/2016
  Added clo library to replace command line processing functions
  R. Healy

  Revision 4.3.0 11/28/2016
  Added in xml code to read product min/max value from product.xml file.  Changed version to match l2bin64
  R. Healy

  Revision 4.1.2 08/04/2016
  Add composite_scheme parameter for land composite binning.
  J. Gales

  Revision 4.1.1 07/02/2016
  Add support for land composite binning.
  J. Gales

  Revision 4.1.0 06/16/2016
  Allocate and clear dolat/dolon arrays in dataday code to avoid time bombs.
  J. Gales

  Revision 4.0.9 01/06/2015
  Fixed issue introduced with 4.0.5 with default sday/eday which resulted
  in a change in behavior
 * NOTE: if this program is still in use in 2038...may need a tweak :)
  Modified test for "regional" prodtype to be case insensitive
  S. Bailey

  Revision 4.0.8 11/30/2015
  Fixed opening group id for multiple files.
  D. Shea

  Revision 4.0.7 10/26/2015
  Fix reading of year,day,time values for time_rec
  D. Shea

  Revision 4.0.6 10/26/2015
  Accumulate time_rec sums at double precision, store at float
  nature of crossing the pole.
  J. Gales

  Revision 4.0.5 10/01/2015
  Added logic to handle orbit based files that cross the dateline by
  nature of crossing the pole.
  S. Bailey

  Revision 4.0.4 09/01/15
  Add support for time_rec field in the binlist records
  J. Gales

 *
 * Revision 4.0
 * Replaced dateline crossing (dataday) with functions
 * based on Norman Kuring's brstobins7
 * Replaced diffday calculations using unix time
 * Added sensorID dependent equator crossing times
 * R. Healy

  Revision 3.1.2 03/10/14
  Remove paths from input filenames
  J. Gales

  Revision 3.1.1 03/06/14
  Add support for binlist and bindata compression
  J. Gales

  Revision 3.1.0 03/05/14
  Add support for CF metadata
  J. Gales

  Revision 2.5.1 05/13/13
  Add support for HEALPIX midaverage (despeckling)
  J. Gales

  Revision 2.5.0 04/08/13
  Add support for NETCDF4
  J. Gales

  Revision 2.4.9 10/14/12
  Put MERIS p1hr dataday parameter back to 19
  J. Gales

  Revision 2.4.8 10/14/12
  Adjust MERIS dataday parameters
  J. Gales

  Revision 2.4.7 10/10/12
  Switch MERIS from TERRA-like to SEAWIFS-like
  J. Gales

  Revision 2.4.6 07/30/12
  Incorporate cache allocation fix in readL2scan
  J. Gales

  Revision 2.4.5 09/12/11
  Put back "platformInformation" & "instrumentInformation" metadata
  J. Gales

  Revision 2.4.4 06/27/11
  Add MERIS to "TERRA-like" dataday
  J. Gales

  Revision 2.4.3 08/05/10
  Exit on non-existent parm/suite file
  J. Gales

  Revision 2.4.2 02/04/10
  Change parameter for cde=5 (dataday) to 0.92
  J. Gales

  Revision 2.4.0 02/04/10
  Add caching for better i/o performance.
  D. Shea

  Revision 2.3.2 08/31/09
  Determine bad value by testing for nan set in readL2 function.
  J. Gales

  Revision 2.3.2 08/25/09
  Add suite parameter
  Modify l2bin_input code to handle suite defaults
  J. Gales

  Revision 2.3.1 08/11/09
  Add pversion parameter
  Remove "Replacement Flag" metadata
  J. Gales

  Revision 2.3.0 11/15/07
  Add ability to "flag" on bad value defined in SDS metadata
  J. Gales

  Revision 2.2.3 05/02/07
  Check that no more than one delimiter is specified.
  J. Gales

  Revision 2.2.2 04/10/07
  Tweak ssec limit for TERRA night
  previous day/no scancross granules to 10.1*60*60
  (T2000138100000.L2_LAC_SST)
  J. Gales

  Revision 2.2.1 04/06/07
  Set bins with -32767 l2 pixel values to data_quality = 4
  J. Gales

  Revision 2.2.0 04/03/07
  Only throw out bad geolocated scans, not entire granules
  J. Gales

  Revision 2.1.9 04/02/07
  Fix memory leak when no good bins are left after qual check.
  J. Gales

  Revision 2.1.8 03/27/07
  Add check for non-navigatable file
  J. Gales

  Revision 2.1.7 03/02/07
  Remove previous revision (2.1.6)
  Use spline check in libl2 to catch bad lon/lat
  Skip pixels with bad lon/lat
  J. Gales

  Revision 2.1.6 03/01/07
  Skip swath scan lines with bad lon/lat
  J. Gales

  Revision 2.1.5 02/09/07
  Disable INTERP parameter
  Fix missing and incorrect entries in Input Parameters Attribute
  J. Gales

  Revision 2.1.4 12/12/06
  Add ',' and ' ' as product delimiters.
  Add '=' as min value delimiters.
  Trap bad minimum values.
  J. Gales

  Revision 2.1.3 12/01/06
  Add VERBOSE input parameter
  J. Gales

  Revision 2.1.2 11/10/06
  Fix interp_distance value definition
  J. Gales

  Revision 2.1.1 09/25/06
  Set MAXNFILES to 544 (Fix made in readL2scan.h)
  J. Gales

  Revision 2.1.0 09/05/06
  Fix problem with prodtype=regional and fileuse
  J. Gales

  Revision 2.0.9 05/12/06
  Fix problem when escan_row > bscan_row.
  J. Gales

  Revision 2.0.8 04/27/06
  Fix dataday problems for day granules
  J. Gales

  Revision 2.0.7 04/10/06
  Support for TERRA SST
  J. Gales

  Revision 2.0.6 03/31/06
  Add longitude boundary check
  (input.lonwest & input.loneast input parameters)
  J. Gales

  Revision 2.0.5 03/16/06
  Added support for HMODIST
  J. Gales
 */


int main(int argc, char **argv) {
    int i, j, k, ii;
    int status;
    intn ret_status = 0;
    int plusday = 0;

    int32_t ifile, jsrow, ipixl, iprod, jprod, kprod, krow;
    uint64_t bin;
    int32_t ibin;
    int32_t nfiles;
    int *fileused;
    int32_t n_active_files;
    int32_t nsamp;
    int32_t isamp;
    int32_t ncols;

    int32_t within_flag;
    int16_t *allocated_space;

    int32_t n_filled_bins;
    int64_t total_filled_bins = 0;
    int32_t noext = 0;
    int32_t date;

    int16_t brk_scan[MAXNFILES];

    float32 *slat = NULL;
    float32 *elat = NULL;
    float32 *clat = NULL;
    float32 *slon = NULL;
    float32 *elon = NULL;
    float32 *clon = NULL;

    float32 dlat;
    double latbin = 0.0;
    double lonbin = 0.0;

    int32_t igroup, ngroup;

    static int32_t * bscan_row[MAXNFILES];
    static int32_t * escan_row[MAXNFILES];
    static unsigned char *scan_in_rowgroup[MAXNFILES];
    int32_t row_group = -1;
    int32_t last_group;

    int32_t n_allocperbin;

    char **prodname;

    int8 isHDF4;
    int8 is64bit = 0;

    int32_t fileid_w;
    int32_t sd_id_w;
    int32_t vgid_w;
    int32_t index;
    int32_t grp1;

    idDS ds_id;

    int32_t zero = 0;
    int32_t five = 5;
    int32_t type[16];
    int32_t start[3] = {0, 0, 0};
    size_t start_nc[3] = {0, 0, 0};
    int32_t edges[3];
    size_t edges_nc[3];
    static int32_t stride[] = {20, 5, 1};
    static ptrdiff_t stride_nc[] = {20, 5, 1};
    int64_t *beg;
    int32_t *ext;
    int64_t *binnum_data;
    int32_t i32;
    int32_t n_bins_in_group;
    int32_t len;
    int32_t l3b_nprod;

    time_t diffday_beg, diffday_end;
    int32_t syear, sday, eyear, eday;

    int32_t tiltstate = 0;
    int32_t ntilts;
    int16_t tilt_flags[MTILT_DIMS_2];
    int16_t tilt_ranges[LTILT_DIMS_2][MTILT_DIMS_2];

    uint32_t flagusemask;
    uint32_t required;
    uint32_t flagcheck;

    int32_t proc_day_beg, proc_day_end;
    int32_t sd_id, sds_id, sd_gid, sd_scan_gid;

    double time_rec = 0;

    uint8_t selcat;
    uint8_t *best_qual, qual_max_allowed;

    int16_t * numer[MAXNFILES], *denom[MAXNFILES];
    int16_t qual_prod_index[MAXNFILES];
    int16_t composite_prod_index[MAXNFILES];
    int16_t composite_l3prod_index = -1;

    float32 *sum_data;
    float32 *sum2_data;
    float32 f32, wgt, sum, sum2;
    float32 *min_value;
    float32 northmost = -90.0, southmost = 90.0, eastmost = -180.0, westmost = 180.0;
    //float32 filter_radius, filter_cutoff;

    int32_t *bin_flag;
    int16_t *tilt, *qual, *nscenes, *lastfile;

    float64 radius = 6378.137000;
    float64 north = 90.0;
    float64 south = -90.0;
    float64 seam_lon = -180.0;
    float64 vsize;
    float64 hsize;

    int32_t bad_value;

    time_t tnow;

    time_t l2_stimes[MAXNFILES];
    int8_t **dorn;
    float onorth, osouth, owest, oeast, eqcross;
    int dateline;
    int32_t dataday0, dataday1, startdate, enddate;
    double ddstime, ddetime, dbldate;

    float **dolat, **dolon, *lonArray, *latArray;
    float *dnlat[2], *dnlon[2];
    int32_t n[2];
    int32_t *years, *days, *msecs;

    int32_t rank, nt, nattrs;
    int32_t dims[3];
    int dims_nc[3];
    int32_t small_dims[3];

    struct tm *tmnow;
    char sdsname[1024] = "";

    static meta_l2Type meta_l2;
    static meta_l3bType meta_l3b;

    static float lonRanges[4];
    lonRanges[0] = 0; // min, max in Western hemisphere
    lonRanges[1] = -180;
    lonRanges[2] = 180; // min, max in Eastern hemisphere
    lonRanges[3] = 0;

    int32_t off[MAXNFILES][100]; /* Byte offsets to each of the data channels */

    static char buf[LG_ATTRSZ];
    char small_buf[SM_ATTRSZ];

    int32_t sensorID[MAXNFILES];
    char units[1024];
    char *tmp_str;

    char *char_ptr1, *char_ptr2;

    char* fldname1[] = {"registration", "straddle", "bins", "radius", \
            "max_north", "max_south", "seam_lon"};

    char* fldname2[] = {"bin_num", "nobs", "nscenes", "time_rec", "weights", \
            "sel_cat", "flags_set"};

    char* fldname3[2];

    char* fldname4[] = {"row_num", "vsize", "hsize", "start_num", "begin", "extent", "max"};

    char* fldname5[] = {"qual_l3"};

    /* Function Prototypes */
    int64_t getbinnum(int32_t, int32_t, int32_t);
    int32_t compute_scanfrac(int32_t, int32_t, uint32_t, uint32_t);

    FILE *fp = NULL, *fp2 = NULL;

    char delim;
    int dnStatus;

    uint8_t *binList = NULL; // this one is for HDF4
    binListStruct_nc* binList32nc = NULL;
    binListStruct64_nc* binList64nc = NULL;

    uint8_t *bin_indx = NULL; // this one is for HDF4
    binIndexStruct_nc* binIndex32nc = NULL;
    binIndexStruct64_nc* binIndex64nc = NULL;

#ifdef MALLINFO
    struct mallinfo minfo;
#endif

    init_rowgroup_cache();

    /* From Fred Patt

        sum(data)    sum(data)*sqrt(n)
   s =  --------- =  -----------------  =  avg(data)*sqrt(n)
         sqrt(n)            n

     */


    setlinebuf(stdout);

    printf("%s %s (%s %s)\n", PROGRAM, VERSION, __DATE__, __TIME__);

    l2bin_input(argc, argv, &input, PROGRAM, VERSION);

    if (strncmp(input.average, "midaverage", 10) == 0) {
        printf("Midaverage no longer supported.\n");
        exit(EXIT_FAILURE);
    }

    cdata_(); //  initialize global FORTRAN common block data for l_sun call

    /* Single HDF input */
    /* ---------------- */
    int ncid;
    if (Hishdf(input.infile) == TRUE || nc_open(input.infile, 0, &ncid) == NC_NOERR) {
        nfiles = 1;
        status = openL2(input.infile, 0x0, &l2_str[0]);
        l2_stimes[0] = (time_t) yds2unix(l2_str[0].syear, l2_str[0].sday, l2_str[0].smsec / 1000); ///86400;
        closeL2(&l2_str[0], 0);
        input.noext = 1;
        printf("Single HDF input\n");

    } else {

        /* Filelist input - Determine number of input files */
        /* ------------------------------------------------ */
        nfiles = 0;
        fp = fopen(input.infile, "r");
        if (fp == NULL) {
            printf("Input listing file: \"%s\" not found.\n", input.infile);
            return -1;
        }
        while (fgets(buf, 256, fp) != NULL) {
            nfiles++;
        }
        fclose(fp);
        printf("%d input files\n", nfiles);


        /* Open L2 input files */
        /* ------------------- */
        fp = fopen(input.infile, "r");

        for (ifile = 0; ifile < nfiles; ifile++) {

            fgets(buf, 256, fp);
            buf[strlen(buf) - 1] = 0;

            status = openL2(buf, 0x0, &l2_str[ifile]);
            l2_stimes[ifile] = (time_t) yds2unix(l2_str[ifile].syear, l2_str[ifile].sday, l2_str[ifile].smsec / 1000); ///86400;
            closeL2(&l2_str[ifile], ifile);

        } /* ifile loop */
        fclose(fp);
    }

    if ((fileused = (int *) calloc(nfiles, sizeof (int))) == NULL) {
        printf("-E- Problem allocating memory for fileused element of metadata structure\n");
        exit(EXIT_FAILURE);
    }

    proc_day_beg = input.sday;
    proc_day_end = input.eday;
    syear = (int32_t) input.sday / 1000.;
    sday = input.sday - 1000 * syear;
    startdate = (int32_t) (yds2unix(syear, sday, 0) / 86400);
    eyear = (int32_t) input.eday / 1000.;
    eday = input.eday - 1000 * eyear;
    enddate = (int32_t) (yds2unix(eyear, eday, 0) / 86400);

    if (strcmp(input.resolve, "1D") == 0) {
        nrows = 180;
    } else if (strcmp(input.resolve, "HD") == 0) {
        nrows = 360;
    } else if (strcmp(input.resolve, "QD") == 0) {
        nrows = 720;
    } else if (strcmp(input.resolve, "36") == 0) {
        nrows = 2160 / 4;
    } else if (strcmp(input.resolve, "18") == 0) {
        nrows = 2160 / 2;
    } else if (strcmp(input.resolve, "9") == 0) {
        nrows = 2160;
    } else if (strcmp(input.resolve, "4") == 0) {
        nrows = 2160 * 2;
    } else if (strcmp(input.resolve, "2") == 0) {
        nrows = 2160 * 4;
    } else if (strcmp(input.resolve, "1") == 0) {
        nrows = 2160 * 8;
    } else if (strcmp(input.resolve, "H") == 0) {
        nrows = 2160 * 16;
    } else if (strcmp(input.resolve, "Q") == 0) {
        nrows = 2160 * 32;
        is64bit = 1;
    } else if (strcmp(input.resolve, "HQ") == 0) {
        nrows = 2160 * 80;
        is64bit = 1;
    } else if (strcmp(input.resolve, "HH") == 0) {
        nrows = 2160 * 160;
        is64bit = 1;
    }

    if (nrows == -1) {
        printf("Grid resolution not defined.\n");
        exit(EXIT_FAILURE);
    }

    dlat = 180. / nrows;

    noext = input.noext;
    qual_max_allowed = input.qual_max;

    printf("Averaging: %s\n", input.average);
    printf("Resolution: %s\n", input.resolve);
    printf("Max Qual Allowed: %d\n", input.qual_max);

    if (strchr(input.average, ':') != NULL) {
        i = 0;
        tmp_str = strtok(input.average, ":");
        while (tmp_str) {
            if (i == 1) strcpy(prod_avg, tmp_str);
            //if (i == 2) filter_radius = atof(tmp_str);
            //if (i == 3) filter_cutoff = atof(tmp_str);

            i++;
            printf("%s\n", tmp_str);
            tmp_str = strtok(NULL, ":");
        }
    }
    printf("prod_avg: %s\n", prod_avg);


    /* Fill offset array */
    /* ----------------- */
    for (ifile = 0; ifile < nfiles; ifile++) {
        for (ii = 0; ii < 100; ii++) {
            off[ifile][ii] = ii * l2_str[ifile].nsamp;
            if (off[ifile][ii] < 0) {
                fprintf(stderr, "Error getting band offset\n");
                exit(EXIT_FAILURE);
            }
        }
    }


    /* Setup flag mask */
    /* --------------- */
    strcpy(buf, l2_str[0].flagnames);
    setupflags(buf, input.flaguse, &flagusemask, &required, &status);
    printf("flagusemask: %d\n", flagusemask);
    printf("required: %d\n", required);


    /* Determine delimiter */
    /* ------------------- */
    if (strchr(input.l3bprod, ':') != NULL) delim = ':';
    if (strchr(input.l3bprod, ',') != NULL) delim = ',';
    if (strchr(input.l3bprod, ' ') != NULL) delim = ' ';

    if (strchr(input.l3bprod, ':') != NULL &&
            strchr(input.l3bprod, ',') != NULL) {
        printf("Both ':' and ',' used as delimiters.\n");
        exit(EXIT_FAILURE);
    }

    if (strchr(input.l3bprod, ':') != NULL &&
            strchr(input.l3bprod, ' ') != NULL) {
        printf("Both ':' and ' ' used as delimiters.\n");
        exit(EXIT_FAILURE);
    }

    if (strchr(input.l3bprod, ',') != NULL &&
            strchr(input.l3bprod, ' ') != NULL) {
        printf("Both ',' and ' ' used as delimiters.\n");
        exit(EXIT_FAILURE);
    }


    /* L3 Product List (ALL/all) */
    /* ------------------------- */
    /*  printf("%s\n", input.l3bprod);*/
    if (strcmp(input.l3bprod, ":ALL:") == 0 ||
            strcmp(input.l3bprod, ":all:") == 0) {
        strcpy(input.l3bprod, ":");
        strcat(input.l3bprod, l2_str[0].prodname[0]);
        strcat(input.l3bprod, ":");

        for (i = 1; i < l2_str[0].nprod; i++) {
            strcat(input.l3bprod, l2_str[0].prodname[i]);
            strcat(input.l3bprod, ":");
        }

        /* Set L3BPROD entry in Input Parameters Attribute */
        char_ptr1 = strstr(input.parms, "l3bprod");
        strcpy(small_buf, char_ptr1 + strlen("l3bprod = ALL"));
        sprintf(char_ptr1, "l3bprod = %s", &input.l3bprod[1]);
        strcat(input.parms, small_buf);
    }
    /*  printf("%s\n", input.l3bprod);*/


    /* Parse L3 Product list */
    /* --------------------- */
    len = strlen(input.l3bprod);
    l3b_nprod = 0;
    for (i = 1; i < len; i++) if (input.l3bprod[i] == delim) l3b_nprod++;

    prodname = (char **) calloc(l3b_nprod + 1, sizeof (char *));

    j = 0;
    for (i = 0; i < len; i++) {
        if (input.l3bprod[i] == delim) {
            prodname[j] = input.l3bprod + i + 1;
            input.l3bprod[i] = 0;
            j++;
        }
    }

    /* Get minimum value from product string */
    /* ------------------------------------- */
    min_value = (float32 *) calloc(l3b_nprod, sizeof (float32));
    for (i = 0; i < l3b_nprod; i++) {

        char_ptr1 = strchr(prodname[i], '/');
        char_ptr2 = strchr(prodname[i], ';');
        if (char_ptr2 == NULL)
            char_ptr2 = strchr(prodname[i], '=');
        if (char_ptr2 != NULL) {
            *char_ptr2 = 0;
            min_value[i] = (float32) strtod(char_ptr2 + 1, &tmp_str);
            if (strcmp(char_ptr2 + 1, tmp_str) == 0) {
                printf("Unable to convert min value: \"%s\"\n", char_ptr2 + 1);
                exit(EXIT_FAILURE);
            }
        } else {
            min_value[i] = BAD_FLT;
        }
    }

    /* Initialize bscan_row, escan_row, numer, denom */
    /* --------------------------------------------- */
    for (i = 0; i < MAXNFILES; i++) {
        bscan_row[i] = NULL;
        escan_row[i] = NULL;
        numer[i] = NULL;
        denom[i] = NULL;
        scan_in_rowgroup[i] = NULL;
    }


    /* Check whether L3 products exist in L2 */
    /* ------------------------------------- */
    for (ifile = 0; ifile < nfiles; ifile++) {

        numer[ifile] = (int16_t *) calloc(l3b_nprod, sizeof (int16_t));
        denom[ifile] = (int16_t *) calloc(l3b_nprod, sizeof (int16_t));

        for (jprod = 0; jprod < l3b_nprod; jprod++) {

            char_ptr1 = strchr(prodname[jprod], '/');
            if (char_ptr1 != NULL) *char_ptr1 = 0;

            for (i = 0; i < l2_str[ifile].nprod; i++)
                if (strcmp(prodname[jprod], l2_str[ifile].prodname[i]) == 0) break;

            numer[ifile][jprod] = i;
            denom[ifile][jprod] = -1;

            if (i == l2_str[ifile].nprod) {

                /* Check if FLAG product */
                /* --------------------- */
                if (strncmp(prodname[jprod], "FLAG_", 5) == 0) {

                    strcpy(small_buf, ",");
                    strcat(small_buf, prodname[jprod] + 5);
                    strcat(small_buf, ",");

                    strcpy(buf, ",");
                    strcat(buf, l2_str[ifile].flagnames);
                    strcat(buf, ",");

                    char_ptr2 = strstr(buf, small_buf);

                    if (char_ptr2 != NULL) {
                        numer[ifile][jprod] = 0;
                        while (char_ptr2 > buf) {
                            if (*char_ptr2 == ',') numer[ifile][jprod]++;
                            char_ptr2--;
                        }
                        denom[ifile][jprod] = -2;

                    } else {

                        printf("L3 product: \"%s\" not found in L2 flagnames.\n",
                                prodname[jprod]);
                        exit(EXIT_FAILURE);
                    }

                } else {

                    printf("L3 product: \"%s\" not found in L2 dataset \"%s\".\n",
                            prodname[jprod], l2_str[ifile].filename);
                    exit(EXIT_FAILURE);
                }
            }

            if (char_ptr1 != NULL) *char_ptr1 = '/';
            else continue;

            for (i = 0; i < l2_str[ifile].nprod; i++)
                if (strcmp(char_ptr1 + 1, l2_str[ifile].prodname[i]) == 0) break;
            denom[ifile][jprod] = i;

            if (i == l2_str[ifile].nprod) {
                printf("L3 product: \"%s\" not found in L2 dataset \"%s\".\n",
                        char_ptr1 + 1, l2_str[ifile].filename);
                exit(EXIT_FAILURE);
            }

        } /* jprod loop */

#if 0
        /* Print L3B product info */
        for (jprod = 0; jprod < l3b_nprod; jprod++)
            printf("%3d %-25s %3d %3d %8.3e\n",
                jprod, prodname[jprod], numer[ifile][jprod], denom[ifile][jprod],
                min_value[jprod]);
#endif
    } /* ifile loop */



    /* Check whether Quality product exists in L2 */
    /* ------------------------------------------ */
    if (input.qual_prod[0] != 0) {
        for (ifile = 0; ifile < nfiles; ifile++) {

            for (i = 0; i < l2_str[ifile].nprod; i++)
                if (strcmp(input.qual_prod, l2_str[ifile].prodname[i]) == 0) break;

            qual_prod_index[ifile] = i;

            if (i == l2_str[ifile].nprod) {
                printf("Quality product: \"%s\" not found in L2 dataset \"%s\".\n",
                        input.qual_prod, l2_str[ifile].filename);
                exit(EXIT_FAILURE);
            }
        }
    }

    /* Check whether Composite product exists in L2 */
    /* -------------------------------------------- */
    if (input.composite_prod[0] != 0) {
        for (ifile = 0; ifile < nfiles; ifile++) {

            for (i = 0; i < l2_str[ifile].nprod; i++)
                if (strcmp(input.composite_prod, l2_str[ifile].prodname[i]) == 0) break;

            composite_prod_index[ifile] = i;

            if (i == l2_str[ifile].nprod) {
                printf("Composite product: \"%s\" not found in L2 dataset \"%s\".\n",
                        input.composite_prod, l2_str[ifile].filename);
                exit(EXIT_FAILURE);
            }
        }
    }


    /* Check whether composite product exists in L3 product list */
    /* --------------------------------------------------------- */
    if (input.composite_prod[0] != 0) {
        for (jprod = 0; jprod < l3b_nprod; jprod++) {

            if (strcmp(input.composite_prod, prodname[jprod]) == 0) {
                composite_l3prod_index = jprod;
                break;
            }
        }
        if (composite_l3prod_index == -1) {
            printf("Composite product: \"%s\" not found in L3 product list.\n",
                    input.composite_prod);
            exit(EXIT_FAILURE);
        }
    }



    /* Find begin and end scan latitudes for each swath row */
    /* ---------------------------------------------------- */
    fp = NULL;
    if (Hishdf(input.infile) == FALSE) {
        if (nc_open(input.infile, NC_NOWRITE, &sd_id) == NC_NOERR)
            nc_close(sd_id);
        else
            fp = fopen(input.infile, "r");
    }

    for (ifile = 0; ifile < nfiles; ifile++) {

        slat = (float32 *) calloc(l2_str[ifile].nrec, sizeof (float32));
        elat = (float32 *) calloc(l2_str[ifile].nrec, sizeof (float32));

        bscan_row[ifile] = (int32_t *) calloc(l2_str[ifile].nrec, sizeof (int32_t));
        escan_row[ifile] = (int32_t *) calloc(l2_str[ifile].nrec, sizeof (int32_t));

        if (fp != NULL) {
            fgets(buf, 256, fp);
            buf[strlen(buf) - 1] = 0;
        } else strcpy(buf, input.infile);

        start[0] = start_nc[0] = 0;
        edges[0] = edges_nc[0] = l2_str[ifile].nrec;

        if (Hishdf(buf) == TRUE) {
            if (ifile == 0) printf("HDF4 input file\n");
            isHDF4 = 1;

            if ((nrows % 2) != 0) {
                printf("HDF4 input must use truncated sinosoidal resolutions\n");
                exit(EXIT_FAILURE);
            }
            sd_id = SDstart(buf, DFACC_RDONLY);

            index = SDnametoindex(sd_id, "slat");
            sds_id = SDselect(sd_id, index);

            status = SDreaddata(sds_id, start, NULL, edges, (VOIDP) slat);
            SDendaccess(sds_id);

            index = SDnametoindex(sd_id, "elat");
            sds_id = SDselect(sd_id, index);

            status = SDreaddata(sds_id, start, NULL, edges, (VOIDP) elat);
            SDendaccess(sds_id);

            SDreadattr(sd_id, SDfindattr(sd_id, "Sensor Name"), small_buf);
            sensorID[ifile] = sensorName2SensorId(small_buf);
            SDend(sd_id);
        } else {
            if (ifile == 0) printf("NetCDF4 input file\n");
            isHDF4 = 0;

            status = nc_open(buf, NC_NOWRITE, &sd_id);
            char instrument[256];
            char platform[256];

            DPTB(nc_get_att(sd_id, NC_GLOBAL, "instrument", instrument));
            DPTB(nc_get_att(sd_id, NC_GLOBAL, "platform", platform));
            sensorID[ifile] = instrumentPlatform2SensorId(instrument, platform);

            nc_inq_ncid(sd_id, "scan_line_attributes", &sd_scan_gid);

            status = nc_inq_varid(sd_scan_gid, "slat", &sds_id);
            status = nc_get_vara(sd_scan_gid, sds_id, start_nc, edges_nc, slat);

            status = nc_inq_varid(sd_scan_gid, "elat", &sds_id);
            status = nc_get_vara(sd_scan_gid, sds_id, start_nc, edges_nc, elat);

            nc_close(sd_id);
        }

        /* Get minimum value from product.xml */
        /* ---------------------------------- */
        productInfo_t* info = allocateProductInfo();
        for (i = 0; i < l3b_nprod; i++) {
            if (min_value[i] == BAD_FLT) {
                if (findProductInfo(prodname[i], sensorID[ifile], info)) {
                    min_value[i] = info->validMin;
                } else {
                    min_value[i] = 0;
                }
            }
        }
        freeProductInfo(info);

        /* Note: bscan > escan */

        for (jsrow = 0; jsrow < l2_str[ifile].nrec; jsrow++) {
            escan_row[ifile][jsrow] = (int32_t) ((90 + elat[jsrow]) / dlat);
            bscan_row[ifile][jsrow] = (int32_t) ((90 + slat[jsrow]) / dlat);

            if (escan_row[ifile][jsrow] > bscan_row[ifile][jsrow]) {
                k = escan_row[ifile][jsrow];
                escan_row[ifile][jsrow] = bscan_row[ifile][jsrow];
                bscan_row[ifile][jsrow] = k;
            }
            escan_row[ifile][jsrow] -= 10;
            bscan_row[ifile][jsrow] += 10;
        }

        free(slat);
        free(elat);

    } /* ifile loop */
    if (fp != NULL)
        fclose(fp);


    /* Find begin & end scans for each input file */
    /* ------------------------------------------ */
    fp = NULL;
    if (Hishdf(input.infile) == FALSE) {
        if (nc_open(input.infile, NC_NOWRITE, &sd_id) == NC_NOERR)
            nc_close(sd_id);
        else
            fp = fopen(input.infile, "r");
    }

    n_active_files = nfiles;
    for (ifile = 0; ifile < nfiles; ifile++) {
        switch (sensorID[ifile]) {
        case HICO:
            eqcross = 12.0; // Made this up - RJH
            break;
        case MODISA:
        case VIIRSN:
        case VIIRSJ1:
            eqcross = 13.5;
            break;
        case MODIST:
        case MERIS:
        case OCTS:
            eqcross = 10.5;
            break;
        case OLCIS3A:
        case OLCIS3B:
            eqcross = 10.0;
            break;
        case CZCS:
            eqcross = 12.0;
            break;
        case SEAWIFS:
            eqcross = 12.0;
            if (l2_str[ifile].syear > 2002) {
                // The constant 10957 makes d the number of days since 1 Jan. 2000
                int d = l2_stimes[ifile] / 86400 - 10957;
                /*
                 * On 10 July 2010 (d=3843) OrbView-2/SeaWiFS was nearing the
                 * end of its orbit-raising maneuvers.  Before these maneuvers
                 * the node-crossing time was drifting further into the afternoon
                 * (first equation below).  After the orbit raising, the node-crossing
                 * time was drifting back towards noon (second equation).

                 * Correction equations provided by Fred Patt.
                 */
                double deg;
                if (d < 3843) {

                    deg = 7.7517951e-10 * d * d * d
                            - 2.1692192e-06 * d * d
                            + 0.0070669241 * d
                            - 4.1300585;
                } else {
                    /*
                     * This equation may need to be replaced with a polynomial
                     * once we get more orbit data under our belts. (16-Jul-2010 N.Kuring)
                     * ...note...the above comment was written before the demise of SeaWiFS
                     * on 11-Dec-2010 ...no further modifications necessary...
                     */
                    deg = -0.024285181 * d + 128.86093;
                }

                //The above polynomials yield degrees; convert to hours.
                float hours = (float) (deg / 15.);

                eqcross = 12 + hours;
            }

            break;
        case HAWKEYE:
            eqcross = 12.0;
            break;
        default:
            fprintf(stderr, "-W- %s line %d: ", __FILE__, __LINE__);
            fprintf(stderr, "Unknown equator crossing time for sensorID=%d file=%s\n ", sensorID[ifile], buf);
            exit(EXIT_FAILURE);
            break;

        }
        // Shift the equatorial crossing time by 12 hours if doing night time binning
        // This is done so that if crossing the dateline the reference hour is being used
        // to move into a dataday in the same direction in function get_datadays
        if (input.night) {
            eqcross = eqcross - 12;
            if (eqcross < 0) {
                eqcross = eqcross + 24;
                plusday = 1; // if we do this, we're effectively going back
                // a day, and we so we need to add a day to the
                // output of get_datadays...
            }
        }

        slon = (float32 *) calloc(l2_str[ifile].nrec, sizeof (float32));
        elon = (float32 *) calloc(l2_str[ifile].nrec, sizeof (float32));
        clon = (float32 *) calloc(l2_str[ifile].nrec, sizeof (float32));
        elat = (float32 *) calloc(l2_str[ifile].nrec, sizeof (float32));
        slat = (float32 *) calloc(l2_str[ifile].nrec, sizeof (float32));
        clat = (float32 *) calloc(l2_str[ifile].nrec, sizeof (float32));
        years = (int32_t *) calloc(l2_str[ifile].nrec, sizeof (int32_t));
        days = (int32_t *) calloc(l2_str[ifile].nrec, sizeof (int32_t));
        msecs = (int32_t *) calloc(l2_str[ifile].nrec, sizeof (int32_t));
        lonArray = (float *) malloc(sizeof (float)*l2_str[ifile].nrec * l2_str[ifile].nsamp);
        latArray = (float *) malloc(sizeof (float)*l2_str[ifile].nrec * l2_str[ifile].nsamp);


        if (fp != NULL) {
            fgets(buf, 256, fp);
            buf[strlen(buf) - 1] = 0;
        } else strcpy(buf, input.infile);

        start[0] = start_nc[0] = 0;
        start[1] = start_nc[1] = 0;
        edges[0] = edges_nc[0] = l2_str[ifile].nrec;
        edges[1] = edges_nc[1] = 1;
        edges[2] = edges_nc[2] = 1;

        if (Hishdf(buf) == TRUE) {
            sd_id = SDstart(buf, DFACC_RDONLY);

            index = SDnametoindex(sd_id, "slon");
            sds_id = SDselect(sd_id, index);
            status = SDreaddata(sds_id, start, NULL, edges, (VOIDP) slon);
            SDendaccess(sds_id);

            index = SDnametoindex(sd_id, "elon");
            sds_id = SDselect(sd_id, index);
            status = SDreaddata(sds_id, start, NULL, edges, (VOIDP) elon);
            SDendaccess(sds_id);

            index = SDnametoindex(sd_id, "clon");
            sds_id = SDselect(sd_id, index);
            status = SDreaddata(sds_id, start, NULL, edges, (VOIDP) clon);
            SDendaccess(sds_id);

            index = SDnametoindex(sd_id, "slat");
            sds_id = SDselect(sd_id, index);
            status = SDreaddata(sds_id, start, NULL, edges, (VOIDP) slat);
            SDendaccess(sds_id);

            index = SDnametoindex(sd_id, "elat");
            sds_id = SDselect(sd_id, index);
            status = SDreaddata(sds_id, start, NULL, edges, (VOIDP) elat);
            SDendaccess(sds_id);

            index = SDnametoindex(sd_id, "clat");
            sds_id = SDselect(sd_id, index);
            status = SDreaddata(sds_id, start, NULL, edges, (VOIDP) clat);
            SDendaccess(sds_id);

            index = SDnametoindex(sd_id, "year");
            sds_id = SDselect(sd_id, index);
            status = SDreaddata(sds_id, start, NULL, edges, (VOIDP) years);
            SDendaccess(sds_id);

            index = SDnametoindex(sd_id, "day");
            sds_id = SDselect(sd_id, index);
            status = SDreaddata(sds_id, start, NULL, edges, (VOIDP) days);
            SDendaccess(sds_id);

            index = SDnametoindex(sd_id, "msec");
            sds_id = SDselect(sd_id, index);
            status = SDreaddata(sds_id, start, NULL, edges, (VOIDP) msecs);
            SDendaccess(sds_id);

            sds_id = SDselect(sd_id, SDnametoindex(sd_id, "latitude"));
            status = SDgetinfo(sds_id, sdsname, &rank, dims, &nt, &nattrs);
            if (dims[0] / stride[0] < 5)
                stride[0] = 1;
            if (dims[1] / stride[1] < 5)
                stride[1] = 1;
            edges[0] = dims[0] / stride[0];
            edges[1] = dims[1] / stride[1];
            status = SDreaddata(sds_id, start, stride, edges, (VOIDP) latArray);
            SDendaccess(sds_id);

            index = SDnametoindex(sd_id, "longitude");
            sds_id = SDselect(sd_id, index);
            status = SDreaddata(sds_id, start, stride, edges, (VOIDP) lonArray);
            SDendaccess(sds_id);
            SDend(sd_id);

            small_dims[0] = dims[0] / stride[0];
            small_dims[1] = dims[1] / stride[1];

        } else {
            status = nc_open(buf, NC_NOWRITE, &sd_id);

            nc_inq_ncid(sd_id, "navigation_data", &sd_gid);
            nc_inq_ncid(sd_id, "scan_line_attributes", &sd_scan_gid);

            status = nc_inq_varid(sd_scan_gid, "slon", &sds_id);
            status = nc_get_vara(sd_scan_gid, sds_id, start_nc, edges_nc, slon);

            status = nc_inq_varid(sd_scan_gid, "elon", &sds_id);
            status = nc_get_vara(sd_scan_gid, sds_id, start_nc, edges_nc, elon);

            status = nc_inq_varid(sd_scan_gid, "clon", &sds_id);
            status = nc_get_vara(sd_scan_gid, sds_id, start_nc, edges_nc, clon);

            status = nc_inq_varid(sd_scan_gid, "slat", &sds_id);
            status = nc_get_vara(sd_scan_gid, sds_id, start_nc, edges_nc, slat);

            status = nc_inq_varid(sd_scan_gid, "elat", &sds_id);
            status = nc_get_vara(sd_scan_gid, sds_id, start_nc, edges_nc, elat);

            status = nc_inq_varid(sd_scan_gid, "clat", &sds_id);
            status = nc_get_vara(sd_scan_gid, sds_id, start_nc, edges_nc, clat);

            status = nc_inq_varid(sd_scan_gid, "year", &sds_id);
            status = nc_get_vara(sd_scan_gid, sds_id, start_nc, edges_nc, years);

            status = nc_inq_varid(sd_scan_gid, "day", &sds_id);
            status = nc_get_vara(sd_scan_gid, sds_id, start_nc, edges_nc, days);

            status = nc_inq_varid(sd_scan_gid, "msec", &sds_id);
            status = nc_get_vara(sd_scan_gid, sds_id, start_nc, edges_nc, msecs);

            status = nc_inq_varid(sd_gid, "latitude", &sds_id);
            status = nc_inq_var(sd_gid, sds_id, 0, &nt, &rank, dims_nc, &nattrs);

            long tmpdim;
            status = ncdiminq(sd_gid, dims_nc[0], 0, &tmpdim);
            dims[0] = (int32_t) tmpdim;

            if (tmpdim / stride_nc[0] < 5)
                stride_nc[0] = 1;

            status = ncdiminq(sd_gid, dims_nc[1], 0, &tmpdim);
            dims[1] = (int32_t) tmpdim;

            if (tmpdim / stride_nc[1] < 5)
                stride_nc[1] = 1;

            edges_nc[0] = dims[0];
            edges_nc[1] = dims[1];

            status = nc_get_var_float(sd_gid, sds_id, latArray);
            status = nc_inq_varid(sd_gid, "longitude", &sds_id);
            status = nc_get_var_float(sd_gid, sds_id, lonArray);

            nc_close(sd_id);
            small_dims[0] = dims[0] / stride_nc[0];
            small_dims[1] = dims[1] / stride_nc[1];

        }

        dolat = (float **) malloc(sizeof (float *)*small_dims[0]);
        dolon = (float **) malloc(sizeof (float *)*small_dims[0]);

        // Allocate and clear dolat/dolon arrays
        for (i = 0; i < small_dims[0]; i++) {
            dolat[i] = (float *) calloc(small_dims[1], sizeof (float));
            dolon[i] = (float *) calloc(small_dims[1], sizeof (float));
        }

        k = 0;

        for (j = 0; j < small_dims[0]; j++) {
            k = dims[1] * (j * stride_nc[0]);
            for (i = 0; i < small_dims[1]; i++) {
                dolat[j][i] = latArray[k];
                dolon[j][i] = lonArray[k];
                k += stride_nc[1];
            }
        }

        // reset stride
        stride[0] = stride_nc[0] = 20;
        stride[1] = stride_nc[1] = 5;

        /* Determine brk_scan value */
        /* ------------------------ */
        brk_scan[ifile] = 0;

        /* Regional Product */
        /* ---------------- */
        if (strcasecmp(input.prodtype, "regional") == 0) {
            printf("%s   brk:%5d  %5d %3d %6d\n",
                    buf, brk_scan[ifile],
                    l2_str[ifile].nrec, l2_str[ifile].sday,
                    l2_str[ifile].smsec / 1000);

            free(slon);
            free(elon);
            free(clon);
            free(elat);
            free(slat);
            free(clat);
            for (i = 0; i < small_dims[0]; i++) {
                free(dolat[i]);
                free(dolon[i]);
            }
            free(dolat);
            free(dolon);

            free(years);
            free(days);
            free(msecs);
            free(lonArray);
            free(latArray);

            continue;
        }

        /* Allocate space for day or night array. */
        if ((dorn = (int8_t **) malloc(small_dims[0] * sizeof (int8_t *))) == NULL) {
            printf("%s -Error: Cannot allocate memory to dorn array\n",
                    __FILE__);
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < small_dims[0]; i++) {
            if ((dorn[i] = (int8_t *) calloc(small_dims[1], sizeof (int8_t))) == NULL) {
                printf("%s -Error: Cannot allocate memory to dorn array\n",
                        __FILE__);
                exit(EXIT_FAILURE);
            }
        }

        date = (time_t) yds2unix(l2_str[ifile].syear, l2_str[ifile].sday, l2_str[ifile].smsec / 1000) / 86400;
        diffday_beg = date - startdate;
        diffday_end = date - enddate;

        // get the vertices of the outline of the swath into one array for lat/lon
        // to pass into get_coord_extrema, which calculates whether the dateline is crossed
        dnlat[0] = NULL;
        dnlat[1] = NULL;
        dnlon[0] = NULL;
        dnlon[1] = NULL;
        dnStatus = daynight_outlines(years, days, msecs, small_dims[1], small_dims[0],
                dolat,
                dolon,
                n, dnlat, dnlon, dorn);

        if (dnStatus != 0) {
            fprintf(stderr, "Consider QC fail for file: %s\n...look at the file \
though, as I might be lying...\n", l2_str[ifile].filename);
            if (ret_status == 0) {
                ret_status = 120;

                // if fileuse is set, open a corresponding qcfail file
                if (input.fileuse[0] != 0) {
                    char *qcFailFile = (char*) malloc(strlen(input.fileuse) + 10);
                    strcpy(qcFailFile, input.fileuse);
                    strcat(qcFailFile, ".qcfail");
                    fp2 = fopen(qcFailFile, "w");
                    free(qcFailFile);
                }

            }
            fprintf(fp2, "%s\n", l2_str[ifile].filename);

        }

        //First argument, isnight, set to 0.  We want to use the dateline all the time
        if (dnStatus == 0 && n[input.night] > 0) {
            get_coord_extrema(0, n[input.night], dnlat[input.night], dnlon[input.night],
                    &onorth, &osouth, &owest, &oeast, &dateline);
            //Third argument, isnight, set to 0.  We want to use the dateline all the time
            //Determine dataday0 and dataday1 based on dateline determined from get_coord_extrema
            get_datadays(l2_stimes[ifile], eqcross, input.night, dateline, owest, oeast,
                    &dataday0, &dataday1);
            dataday0 += plusday;
            dataday1 += plusday;
            if (dataday1 == dataday0) {
                if (dataday0 < startdate || dataday1 > enddate)
                    brk_scan[ifile] = -9999;
                else
                    brk_scan[ifile] = 0; //-9999;
            } else {
                if (dataday1 < startdate) //startdate is dataday conversion of input sday
                    brk_scan[ifile] = -9999;
                else if (dataday0 > enddate) { //enddate is dataday conversion of input eday
                    brk_scan[ifile] = -9999;
                } else {
                    if (dataday1 > enddate)
                        brk_scan[ifile] = 1;
                    else
                        brk_scan[ifile] = -1;

                    //file is within sday and eday, brk_scan tells whether
                    //it went from the data day to the next day over the dateline (1),
                    //or data day to a previous day over the dateline (-1)
                }
            }

        } else {
            brk_scan[ifile] = -9999;
        }

        for (i = 0; i < small_dims[0]; i++) {
            free(dorn[i]);
        }
        free(dorn);
        if (dnlat[0])
            free(dnlat[0]);
        if (dnlat[1])
            free(dnlat[1]);
        if (dnlon[0])
            free(dnlon[0]);
        if (dnlon[1])
            free(dnlon[1]);


        if (brk_scan[ifile] == -9999) n_active_files--;

        free(slon);
        free(elon);
        free(clon);
        free(elat);
        free(slat);
        free(clat);

        free(lonArray);
        free(latArray);

        //for (i=0; i<l2_str[ifile].nrec; i++) {
        for (i = 0; i < small_dims[0]; i++) {
            free(dolat[i]);
            free(dolon[i]);
        }
        free(dolat);
        free(dolon);
        free(years);
        free(days);
        free(msecs);


    } /* ifile loop */
    if (ret_status == 120)
        fclose(fp2);
    // exit(-3);

    if (fp != NULL) fclose(fp);
    /* Compute numbin array (Number of bins in each row) */
    /* ------------------------------------------------- */
    numbin = (int64_t *) calloc(nrows, sizeof (int64_t));
    for (i = 0; i < nrows; i++) {
        latbin = (i + 0.5) * (180.0 / nrows) - 90.0;
        numbin[i] = (int64_t) (cos(latbin * PI / 180.0) * (2.0 * nrows) + 0.5);
    }

    /* Compute basebin array (Starting bin of each row [1-based]) */
    /* ---------------------------------------------------------- */
    basebin = (int64_t *) calloc(nrows + 1, sizeof (int64_t));
    basebin[0] = 1;
    for (i = 1; i <= nrows; i++) {
        basebin[i] = basebin[i - 1] + numbin[i - 1];
    }
    printf("total number of bins: %ld\n", (long int) basebin[nrows] - 1);


    /*
     * Create output file
     * If the user defined the output format use it
     *     (default is goesoutta == goesinta)
     */
    if (getFileFormatName(input.oformat) != NULL) {
        if (strcmp(getFileFormatName(input.oformat), "HDF4") == 0) {
            isHDF4 = 1;
        } else if (strcmp(getFileFormatName(input.oformat), "netCDF4") == 0) {
            isHDF4 = 0;
        } else {
            printf("Binfile output format not understood\n");
            exit(EXIT_FAILURE);
        }
    }

    if (isHDF4 == 1) {
        strcpy(buf, input.ofile);
        if (noext == 0) strcat(buf, ".main");
        fileid_w = Hopen(buf, DFACC_CREATE, 0);
        sd_id_w = SDstart(buf, DFACC_RDWR);
        ds_id.sid = sd_id_w;
        ds_id.fftype = DS_HDF; // FMT_L2HDF

        Vstart(fileid_w);

        vgid_w = Vattach(fileid_w, -1, "w");

        Vsetname(vgid_w, "Level-3 Binned Data");
        Vsetclass(vgid_w, "PlanetaryGrid");

    } else {
        strcpy(buf, input.ofile);
        status = nc_create(buf, NC_NETCDF4, &fileid_w);
        check_err(status, __LINE__, __FILE__);

        status = nc_def_grp(fileid_w, "level-3_binned_data", &grp1);
        check_err(status, __LINE__, __FILE__);

        ds_id.fid = fileid_w;
        ds_id.sid = -1;
        ds_id.fftype = DS_NCDF; // FMT_L2NCDF

        if(is64bit)
            status = defineBinList64_nc(input.deflate, grp1);
        else
            status = defineBinList_nc(input.deflate, grp1);
        if (status) {
            printf("-E- %s:%d Could not define binList variable in output file\n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        status = defineBinData_nc(input.deflate, grp1, l3b_nprod, prodname);
        if (status) {
            printf("-E- %s:%d Could not define binData variable in output file\n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if(is64bit)
            status = defineBinIndex64_nc(input.deflate, grp1);
        else
            status = defineBinIndex_nc(input.deflate, grp1);
        if (status) {
            printf("-E- %s:%d Could not define binIndex variable in output file\n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (input.qual_prod[0] != 0) {
            status = defineQuality_nc(input.deflate, grp1);
            if (status) {
                printf("-E- %s:%d Could not define quality variable in output file\n",
                        __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }

    if (isHDF4 == 1) {
        /* Write "SEAGrid" */
        /* --------------- */
        uint8_t *seagrid = (uint8_t *) malloc(44);

        ncols = 2 * nrows;

        memcpy(&seagrid[0], &five, 4);
        memcpy(&seagrid[4], &zero, 4);
        memcpy(&seagrid[8], &ncols, 4);
        memcpy(&seagrid[12], &radius, 8);
        memcpy(&seagrid[20], &north, 8);
        memcpy(&seagrid[28], &south, 8);
        memcpy(&seagrid[36], &seam_lon, 8);

        type[0] = DFNT_INT32;
        type[1] = DFNT_INT32;
        type[2] = DFNT_INT32;
        type[3] = DFNT_FLOAT64;
        type[4] = DFNT_FLOAT64;
        type[5] = DFNT_FLOAT64;
        type[6] = DFNT_FLOAT64;

        wr_vdata(input.ofile, fileid_w, vgid_w, "SEAGrid", "Geometry", 7, 1,
                fldname1, type, 0, seagrid, input.verbose);
        wr_vdata(input.ofile, fileid_w, vgid_w, "SEAGrid", "Geometry", 7, 0,
                NULL, NULL, 0, NULL, input.verbose);

        free(seagrid);
    }

    /* Allocate Arrays for Bin Index */
    /* ----------------------------- */
    beg = (int64_t *) calloc(nrows, sizeof (int64_t));
    ext = (int32_t *) calloc(nrows, sizeof (int32_t));
    if (isHDF4 == 1)
        bin_indx = (uint8_t *) calloc(36 * nrows, 1);
    else {
        if(is64bit)
            binIndex64nc = (binIndexStruct64_nc *) calloc(nrows, sizeof(binIndexStruct64_nc));
        else
            binIndex32nc = (binIndexStruct_nc *) calloc(nrows, sizeof(binIndexStruct_nc));
    }

    /* Initialize bin_indx array */
    /* ------------------------- */
    for (i = 0; i < nrows; i++) {

        i32 = i;

        if (i32 < 0 || i32 >= nrows) {
            printf("%d %d\n", i, nrows);
            exit(-1);
        }

        vsize = 180.0 / nrows;
        hsize = 360.0 / numbin[i32];

        if (isHDF4 == 1) {
            memcpy(&bin_indx[i * 36], &i32, 4);
            memcpy(&bin_indx[i * 36 + 4], &vsize, 8);
            memcpy(&bin_indx[i * 36 + 12], &hsize, 8);
            // JMG    memcpy(&bin_indx[i*36+20], &basebin[i32], 4);
            memcpy(&bin_indx[i * 36 + 24], &beg[i32], 4);
            memcpy(&bin_indx[i * 36 + 28], &ext[i32], 4);
            memcpy(&bin_indx[i * 36 + 32], &numbin[i32], 4);
        } else {
            if(is64bit) {
                binIndex64nc[i].begin = beg[i32];
                binIndex64nc[i].extent = ext[i32];
                binIndex64nc[i].max = numbin[i32];
            } else {
                binIndex32nc[i].begin = beg[i32];
                binIndex32nc[i].extent = ext[i32];
                binIndex32nc[i].max = numbin[i32];
            }
        }
    }


    // Row Group
    row_group = input.rowgroup;
    if (row_group <= 0) {
        printf("row_group not defined.\n");
        exit(EXIT_FAILURE);
    }
    printf("%d %d %d\n", proc_day_beg, proc_day_end, row_group);

    /* Find row_group that divides nrows */
    /* --------------------------------- */
    for (i = nrows; i > 0; i--) {
        if ((nrows % i) == 0) {
            if (i <= row_group) {
                row_group = i;
                break;
            }
        }
    }
    if (input.rowgroup != row_group) {
        printf("Input row_group: %d   Actual row_group: %d\n",
                input.rowgroup, row_group);
    }
    ngroup = nrows / row_group;


    /* Process each group of bin rows (Main Loop) */
    /* ========================================== */
    for (krow = 0, igroup = 0; igroup < ngroup; igroup++) {

        if (((double) ((krow + row_group) / nrows) * 180.0 - 90.0) < (double) input.latsouth) {
            krow += row_group;
            continue;
        }
        if ((double) (krow / nrows * 180.0 - 90.0) > (double) input.latnorth) {
            krow += row_group;
            continue;
        }


        /* Print info on rowgroup */
        /* ---------------------- */
        time(&tnow);
        tmnow = localtime(&tnow);
        printf("krow:%6d out of %6d  (%6.2f to %6.2f) ",
                krow, nrows,
                ((float32) (krow) / nrows) * 180 - 90,
                ((float32) (krow + row_group) / nrows) * 180 - 90);

        printf("%s", asctime(tmnow));

        n_bins_in_group = basebin[krow + row_group] - basebin[krow];

        within_flag = 0;


        /* Determine relevant swath rows for this bin row group for each file */
        /* ------------------------------------------------------------------ */
        for (ifile = 0; ifile < nfiles; ifile++) {

            /* add an extra 0 to the end of scan_in_rowgroup so the caching
             * code never reads past the end of the file */
            scan_in_rowgroup[ifile] = (unsigned char *)
                    calloc(l2_str[ifile].nrec + 1, sizeof (unsigned char));

            for (jsrow = 0; jsrow < l2_str[ifile].nrec; jsrow++) {
                scan_in_rowgroup[ifile][jsrow] = 1;
                if (bscan_row[ifile][jsrow] < krow ||
                        escan_row[ifile][jsrow] >= (krow + row_group - 1)) {
                    scan_in_rowgroup[ifile][jsrow] = 255;
                }
            } /* jsrow loop */


            /* Determine if within bin row group */
            /* --------------------------------- */
            for (jsrow = 0; jsrow < l2_str[ifile].nrec; jsrow++) {
                if (scan_in_rowgroup[ifile][jsrow] == 1 && within_flag == 0) {
                    within_flag = 1;
                    break;
                }
            } /* scan row loop */

        } /* ifile loop */


        /* If no swath rows within group then continue to next group */
        /* --------------------------------------------------------- */
        if (within_flag == 0) {

            for (ifile = 0; ifile < nfiles; ifile++) {
                if (scan_in_rowgroup[ifile] != NULL) {
                    free(scan_in_rowgroup[ifile]);
                    scan_in_rowgroup[ifile] = NULL;
                }
            }
            krow += row_group;
            continue;
        }


        /* Allocate # pixels in bin, bin_flag, tilt, qual, & nscenes arrays */
        /* ---------------------------------------------------------------- */
        n_filled_bins = 0;
        bin_flag = (int32_t *) calloc(n_bins_in_group, sizeof (int32_t));
        tilt = (int16_t *) calloc(n_bins_in_group, sizeof (int16_t));
        qual = (int16_t *) calloc(n_bins_in_group, sizeof (int16_t));
        nscenes = (int16_t *) calloc(n_bins_in_group, sizeof (int16_t));
        lastfile = (int16_t *) calloc(n_bins_in_group, sizeof (int16_t));

        for (i = 0; i < n_bins_in_group; i++) {
            tilt[i] = -1;
            qual[i] = 3;
            lastfile[i] = -1;
        }


        /* Allocate bin accumulator & data value arrays */
        /* -------------------------------------------- */
        nobs = (int16_t *) calloc(n_bins_in_group, sizeof (int16_t));
        allocated_space = (int16_t *) calloc(n_bins_in_group, sizeof (int16_t));
        data_values = (float32 **) calloc(n_bins_in_group, sizeof (float32 *));
        file_index = (int16_t **) calloc(n_bins_in_group, sizeof (int16_t *));
        data_quality = (uint8_t **) calloc(n_bins_in_group, sizeof (uint8_t *));
        time_value = (float64 **) calloc(n_bins_in_group, sizeof (float64 *));


        /* Initialize bin counters */
        /* ----------------------- */
        n_allocperbin =
                n_active_files * l2_str[0].nrec * l2_str[0].nsamp / 50000000;

        if (n_allocperbin < 2) n_allocperbin = 2;
        if (n_allocperbin > MAXALLOCPERBIN) n_allocperbin = MAXALLOCPERBIN;

        if (input.verbose == 1) {
            printf("%-20s:%8d\n", "# allocated per bin", n_allocperbin);
            printf("\n");
        }

        for (i = 0; i < n_bins_in_group; i++) {
            nobs[i] = 0;
            allocated_space[i] = 0;
            lastfile[i] = -1;
        }


        /* Read L2 files and fill data_values (L3b) array */
        /* ++++++++++++++++++++++++++++++++++++++++++++++ */
        for (ifile = 0; ifile < nfiles; ifile++) {

            free_rowgroup_cache();

            /* if "early" or "late" input file then skip */
            /* ----------------------------------------- */
            if (brk_scan[ifile] == -9999) continue;

            //      status = reopenL2(ifile, &l2_str[ifile]);

            /* if no scans in rowgroup for this file then skip */
            /* ----------------------------------------------- */
            for (jsrow = 0; jsrow < l2_str[ifile].nrec; jsrow++) {
                if (scan_in_rowgroup[ifile][jsrow] == 1) {
                    break;
                }
            }
            if (jsrow == l2_str[ifile].nrec) {
                //	closeL2(&l2_str[ifile], ifile);
                continue;
            }

            reopenL2(ifile, &l2_str[ifile]);

            /* Get tilt flags & ranges */
            /* ----------------------- */
            ntilts = l2_str[ifile].ntilts;
            for (i = 0; i < ntilts; i++) {
                tilt_flags[i] = l2_str[ifile].tilt_flags[i];
                tilt_ranges[0][i] = l2_str[ifile].tilt_ranges[0][i];
                tilt_ranges[1][i] = l2_str[ifile].tilt_ranges[1][i];
            }

            /* Get date stuff */
            /* -------------- */
            date = (time_t) yds2unix(l2_str[ifile].syear, l2_str[ifile].sday, l2_str[ifile].smsec / 1000) / 86400;
            diffday_beg = date - startdate;
            diffday_end = date - enddate;

            /* Loop over swath rows */
            /* ^^^^^^^^^^^^^^^^^^^^ */
            for (jsrow = 0; jsrow < l2_str[ifile].nrec; jsrow++) {

                /* if swath row not within group then continue */
                /* ------------------------------------------- */
                if (scan_in_rowgroup[ifile][jsrow] != 1) continue;


                /* Read swath record from L2 */
                /* ------------------------- */
                status = readL2(&l2_str[ifile], ifile, jsrow, -1,
                        scan_in_rowgroup[ifile]);

                if (status == 5) continue;
                /*
                 * The following bits were added to address the issue of orbit files
                 * (e.g. SeaWiFS) that cross the dateline because they cross the pole
                 * The intent is to prevent the dateline crossing code from being as
                 * draconian with the data it excludes for the portion of the orbit
                 * that doesn't cross the dateline.
                 */
                dbldate = yds2unix(l2_str[ifile].year, l2_str[ifile].day, l2_str[ifile].msec / 1000);
                ddstime = yds2unix(syear, sday - plusday, ((double) eqcross - 12) * 3600.);
                ddetime = yds2unix(eyear, eday - plusday, ((double) eqcross + 12) * 3600.);

                int scan_crosses_dateline = 0;
                if (brk_scan[ifile] != 0) {
                    int npixls = l2_str[ifile].nsamp - 1;
                    float slat = l2_str[ifile].longitude[0];
                    float elat = l2_str[ifile].longitude[npixls];
                    if (abs(slat - elat) > 180)
                        scan_crosses_dateline = 1;
                }

                if (scan_crosses_dateline == 0 && (strcasecmp(input.prodtype, "regional") != 0)) {
                    // if the scan does NOT cross the dateline and is outside
                    // the bounds of the dataday(s), move along...
                    if (dbldate < ddstime) continue;
                    if (dbldate > ddetime) continue;
                }

                /* Check tilt state */
                /* ---------------- */
                for (i = 0; i < ntilts; i++) {
                    if ((jsrow + 1) <= tilt_ranges[1][i]) {
                        tiltstate = (tilt_flags[i] & 0xFF);
                        break;
                    }
                }
                /*	if (tiltstate < 0 || tiltstate > 2) continue;*/


                /* Compute scan_frac */
                /* ----------------- */
                nsamp = compute_scanfrac(ifile, ipixl, flagusemask, required);
                if (nsamp == 0) continue;


                if ((jsrow % 100) == 0 && input.verbose == 1) {
                    printf("ifile:%4d  jsrow:%6d  nsamp:%8d\n", ifile, jsrow, nsamp);
                }

#if 0
                /* Check for bad lon/lat (2.1.6) */
                /* ----------------------------- */
                bad_lonlat = 0;
                for (isamp = 0; isamp < nsamp; isamp++) {
                    if (l2_str[ifile].longitude[isamp] < -180) bad_lonlat = 1;
                    if (l2_str[ifile].longitude[isamp] > +180) bad_lonlat = 1;
                    if (l2_str[ifile].latitude[isamp] < -90) bad_lonlat = 1;
                    if (l2_str[ifile].latitude[isamp] > +90) bad_lonlat = 1;
                }
                if (bad_lonlat == 1)
                    continue;
#endif

                /* ##### Loop over L2 pixels ##### */
                /* ------------------------------- */
                for (isamp = 0; isamp < nsamp; isamp++) {

                    ipixl = floor((double) scan_frac[isamp]);

                    /* if bin flagged then continue */
                    /* ---------------------------- */
                    flagcheck = (l2_str[ifile].l2_flags[ipixl] |
                            l2_str[ifile].l2_flags[ipixl + input.interp]);
                    if ((flagcheck & flagusemask) != 0)
                        continue;
                    if ((flagcheck & required) != required)
                        continue;


                    /* Check for dateline crossing */
                    /* --------------------------- */
                    if (scan_crosses_dateline == 1) {
                        // if the scan does cross the dateline, decide which
                        // pixels to bin based on longitude and which side of
                        // the dateline we should keep
                        if (input.night == 1) {

                            if ((brk_scan[ifile] == -1) &&
                                    (diffday_beg == -1) && (1) &&
                                    (l2_str[ifile].longitude[ipixl] < 0)) continue;

                            if ((brk_scan[ifile] == +1) &&
                                    (diffday_end == 0) && (1) &&
                                    (l2_str[ifile].longitude[ipixl] > 0)) continue;

                        } else {

                            if ((brk_scan[ifile] == -1) &&
                                    (diffday_beg <= 0) &&
                                    (l2_str[ifile].longitude[ipixl] < 0)) continue;

                            if ((brk_scan[ifile] == +1) &&
                                    (diffday_end >= 0) &&
                                    (l2_str[ifile].longitude[ipixl] > 0)) continue;
                        }
                    }
                    /* Check for bad value in any of the products */
                    /* ------------------------------------------ */
                    bad_value = 0;
                    for (jprod = 0; jprod < l3b_nprod; jprod++) {

                        f32 = l2_str[ifile].l2_data[ipixl + off[ifile][numer[ifile][jprod]]];
                        if (isnan(f32)) {
                            bad_value = 1;
                            break;
                        }
                    }
                    if (bad_value == 1) continue;


                    /* Check if within longitude boundaries */
                    /* ------------------------------------ */
                    if (input.lonwest != 0.0 || input.loneast != 0.0) {
                        if (l2_str[ifile].longitude[ipixl] < input.lonwest) continue;
                        if (l2_str[ifile].longitude[ipixl] > input.loneast) continue;
                    }


                    /* Get Bin Number for Pixel */
                    /* ------------------------ */
                    bin = getbinnum(ifile, ipixl, isamp); // bin is 1-based

                    if (bin == -1) {
                        printf("file: %s  ipixl: %d  jsrow: %d\n",
                                l2_str[ifile].filename, ipixl, jsrow);
                        continue;
                    }
                    ibin = bin - basebin[krow];


                    /* if bin not within bin row group then continue */
                    /* --------------------------------------------- */
                    if (ibin < 0 ||
                            ibin >= n_bins_in_group ||
                            bin >= basebin[krow + row_group] ||
                            bin < basebin[krow])
                        continue;



                    /* GOOD OBSERVATION FOUND */
                    /* ---------------------- */

#ifdef MALLINFO
                    if (input.dcinfo) {
                        if ((l2_str[ifile].longitude[ipixl] <= -160) ||
                                (l2_str[ifile].longitude[ipixl] >= +160)) {
                            printf("DC: %10ld %12d %8.2f %8.2f\n",
                                    (long int) bin,
                                    (int) dbldate,
                                    l2_str[ifile].longitude[ipixl],
                                    l2_str[ifile].latitude[ipixl]);
                        }
                    }
#endif
                    /*
          if (bin == BINCHECK) {
            printf("bin_af: %d  ifile: %d i: %d ipixl: %d lat: %f lon: %f\n",
                   bin,ifile,i,ipixl,
                   l2_str[ifile].latitude[ipixl],l2_str[ifile].longitude[ipixl]);
          }
                     */

                    /* "OR" flags in swath pixel & set tilt & increment nscenes */
                    /* -------------------------------------------------------- */
                    bin_flag[ibin] = bin_flag[ibin] | l2_str[ifile].l2_flags[ipixl];

                    tilt[ibin] = tiltstate;
                    if (ifile != lastfile[ibin]) {
                        nscenes[ibin]++;
                        lastfile[ibin] = ifile;
                    }


                    /* Allocate space for file index & bin data values */
                    /* ----------------------------------------------- */
                    if (file_index[ibin] == NULL) {
                        file_index[ibin] = (int16_t *) calloc(n_allocperbin, sizeof (int16_t));

                        data_values[ibin] =
                                (float32 *) calloc(n_allocperbin*l3b_nprod, sizeof (float32));

                        if (data_values[ibin] == 0x0) {
                            perror(buf);
                            printf("Allocation failed for data_values[ibin]: %d %s\n",
                                    ibin, buf);
                            exit(EXIT_FAILURE);
                        }

                        data_quality[ibin] =
                                (uint8_t *) calloc(n_allocperbin, sizeof (uint8_t));

                        if (data_quality[ibin] == 0x0) {
                            perror(buf);
                            printf("Allocation failed for data_quality[ibin]: %d %s\n",
                                    ibin, buf);
                            exit(EXIT_FAILURE);
                        }

                        time_value[ibin] =
                                (float64 *) calloc(n_allocperbin, sizeof (float64));

                        if (time_value[ibin] == 0x0) {
                            perror(buf);
                            printf("Allocation failed for time_value[ibin]: %d %s\n",
                                    ibin, buf);
                            exit(EXIT_FAILURE);
                        }

                        allocated_space[ibin] = n_allocperbin;
                    }


                    /* Set file_index for each observation */
                    /* ----------------------------------- */
                    file_index[ibin][nobs[ibin]] = ifile;


                    /* Get data quality */
                    /* ---------------- */
                    if (input.qual_prod[0] != 0) {
                        data_quality[ibin][nobs[ibin]] =
                                l2_str[ifile].l2_data[ipixl + off[ifile][qual_prod_index[ifile]]];
                        // a temporary fix to mask the last 4 pixels of a MODIS Terra scan
                        // for SST product
                        if (strncmp(input.suite, "SST", 3) == 0 && sensorID[ifile] == MODIST && ipixl > 1349) {
                            data_quality[ibin][nobs[ibin]] = 3;
                        }
                    }

                    /* Get time_value (TAI93) */
                    /* ---------------------- */
                    double time_tai;
                    double dbl_sec = (double) (l2_str[ifile].msec / 1000.0);
                    time_tai = yds2tai93(l2_str[ifile].year, l2_str[ifile].day, dbl_sec);

                    time_value[ibin][nobs[ibin]] = time_tai;

                    /* Get composite data */
                    /* ------------------- */
                    if (input.composite_prod[0] != 0) {
                        int idx = composite_prod_index[ifile];
                        f32 = l2_str[ifile].l2_data[ipixl + off[ifile][idx]];
                        if (f32 == -32767) continue;

                        if (nobs[ibin] != 0) {
                            if (strcmp(input.composite_scheme, "max") == 0) {
                                if (f32 < data_values[ibin][composite_l3prod_index]) continue;
                            } else {
                                if (f32 > data_values[ibin][composite_l3prod_index]) continue;
                            }
                            nobs[ibin] = 0;
                        }
                    }

                    /* Get data values for all L3 products */
                    /* ----------------------------------- */
                    for (jprod = 0; jprod < l3b_nprod; jprod++) {

                        f32 = l2_str[ifile].l2_data[ipixl + off[ifile][numer[ifile][jprod]]];

                        /* Set -32767 value to "bad" quality */
                        if (f32 == -32767)
                            if (input.qual_prod[0] != 0)
                                data_quality[ibin][nobs[ibin]] = 4;

                        if (input.interp == 1) {
                            f32 += (scan_frac[isamp] - ipixl) *
                                    (l2_str[ifile].l2_data[ipixl + 1 +
                                    off[ifile][numer[ifile][jprod]]] -
                                    l2_str[ifile].l2_data[ipixl +
                                    off[ifile][numer[ifile][jprod]]]);
                        }

                        if (input.composite_prod[0] != 0) {
                            data_values[ibin][l3b_nprod * nobs[ibin] + jprod] = f32;
                        } else {
                            if (denom[ifile][jprod] == -1 && f32 >= min_value[jprod])
                                data_values[ibin][l3b_nprod * nobs[ibin] + jprod] = f32;

                            if (denom[ifile][jprod] == -1 && f32 < min_value[jprod])
                                data_values[ibin][l3b_nprod * nobs[ibin] + jprod] = min_value[jprod];

                            if (denom[ifile][jprod] == -2)
                                data_values[ibin][l3b_nprod * nobs[ibin] + jprod] =
                                    (l2_str[ifile].l2_flags[ipixl] >> numer[ifile][jprod]) & 1;
                        }

                        /* ratio product */
                        /* ------------- */
                        if (denom[ifile][jprod] >= 0) {

                            data_values[ibin][l3b_nprod * nobs[ibin] + jprod] = f32;

                            f32 = l2_str[ifile].l2_data[ipixl +
                                    off[ifile][denom[ifile][jprod]]];

                            if (input.interp == 1) {
                                f32 += (scan_frac[isamp] - ipixl) *
                                        (l2_str[ifile].l2_data[ipixl + 1 +
                                        off[ifile][denom[ifile][jprod]]] -
                                        l2_str[ifile].l2_data[ipixl +
                                        off[ifile][denom[ifile][jprod]]]);
                            }

                            if (f32 >= min_value[jprod])
                                data_values[ibin][l3b_nprod * nobs[ibin] + jprod] /= f32;
                            else
                                data_values[ibin][l3b_nprod * nobs[ibin] + jprod] /=
                                    min_value[jprod];
                        }

                    } /* jprod loop */


                    /* Increment number of observations in bin */
                    /* --------------------------------------- */
                    nobs[ibin]++;
                    if (input.composite_prod[0] != 0) nobs[ibin] = 1;


                    /* Reallocate if necessary */
                    /* ----------------------- */
                    if (nobs[ibin] == allocated_space[ibin]) {

                        file_index[ibin] =
                                (int16_t *) realloc(file_index[ibin],
                                (nobs[ibin] + n_allocperbin) * sizeof (int16_t));

                        data_values[ibin] =
                                (float32 *) realloc(data_values[ibin],
                                (nobs[ibin] + n_allocperbin) * l3b_nprod *
                                sizeof (float32));
                        if (data_values[ibin] == 0x0) {
                            perror(buf);
                            printf("Reallocation failed for data_values[ibin]: %d %s\n",
                                    ibin, buf);
                            exit(EXIT_FAILURE);
                        }

                        data_quality[ibin] =
                                (uint8_t *) realloc(data_quality[ibin],
                                (nobs[ibin] + n_allocperbin) * sizeof (uint8_t));
                        if (data_quality[ibin] == 0x0) {
                            perror(buf);
                            printf("Reallocation failed for data_quality[ibin]: %d %s\n",
                                    ibin, buf);
                            exit(EXIT_FAILURE);
                        }

                        time_value[ibin] =
                                (float64 *) realloc(time_value[ibin],
                                (nobs[ibin] + n_allocperbin) * sizeof (float64));
                        if (time_value[ibin] == 0x0) {
                            perror(buf);
                            printf("Reallocation failed for time_value[ibin]: %d %s\n",
                                    ibin, buf);
                            exit(EXIT_FAILURE);
                        }

                        allocated_space[ibin] += n_allocperbin;
                    } /* end reallocate */

                } /* ##### i (ipixl) loop ##### */

                free(scan_frac);


            } /* ^^^^^^^^^^ jsrow loop ^^^^^^^^^^ */

            closeL2(&l2_str[ifile], ifile);


#ifdef MALLINFO
            if (input.meminfo) {
 		int32_t total_alloc_space;
                /*      malloc_stats();*/
                minfo = mallinfo();
                total_alloc_space = 0;
                for (i = 0; i < n_bins_in_group; i++) {
                    total_alloc_space += allocated_space[i];
                }
                printf("Used space: %10d\n", minfo.uordblks);
                printf("Allo space: %10d\n", total_alloc_space * (2 + l3b_nprod * 4));
            }
#endif


        } /* ++++++++++ ifile loop ++++++++++ */

        time(&tnow);
        tmnow = localtime(&tnow);
        if (input.verbose == 1)
            printf("krow:%5d After data_value fill: %s\n", krow, asctime(tmnow));

        /* Compute Total # of filled bins */
        /* ------------------------------ */
        for (ibin = 0; ibin < n_bins_in_group; ibin++) {
            // JMG
            if (nobs[ibin] > 0 && nobs[ibin] < input.minobs)
                nobs[ibin] = 0;

            if (nobs[ibin] != 0) n_filled_bins++;
        } /* ibin loop */


        best_qual = (uint8_t *) calloc(n_bins_in_group, sizeof (uint8_t));
        memset(best_qual, 255, n_bins_in_group * sizeof (uint8_t));


        /* ********** If filled bins ********** */
        /* ------------------------------------ */
        if (n_filled_bins > 0) {

            last_group = 1;

            /* Fill "Bin List" vdata array */
            /* --------------------------- */
            if (isHDF4 == 1)
                binList = (uint8_t *) calloc(20 * n_filled_bins, 1);
            else {
                if(is64bit)
                    binList64nc = (binListStruct64_nc*) calloc(n_filled_bins, sizeof(binListStruct64_nc));
                else
                    binList32nc = (binListStruct_nc*) calloc(n_filled_bins, sizeof(binListStruct_nc));
            }

            i = 0;
            for (ibin = 0; ibin < n_bins_in_group; ibin++) {

                bin = (uint64_t) ibin + basebin[krow];

                /*
        if (bin == BINCHECK) {
          for (j=0; j<nobs[ibin]; j++)
            printf("qual: %d %d %d\n",
                   bin, nobs[ibin], data_quality[ibin][j]);
        }
                 */

                /* Adjust for bins with "bad" quality values */
                /* ----------------------------------------- */
                if (input.qual_prod[0] != 0 && nobs[ibin] > 0) {
                    best_qual[ibin] = 255;
                    for (j = 0; j < nobs[ibin]; j++)
                        if (data_quality[ibin][j] < best_qual[ibin])
                            best_qual[ibin] = data_quality[ibin][j];

                    k = 0;
                    for (j = 0; j < nobs[ibin]; j++) {
                        if ((data_quality[ibin][j] <= best_qual[ibin]) &&
                                (data_quality[ibin][j] <= qual_max_allowed)) {
                            if (k < j) {
                                for (iprod = 0; iprod < l3b_nprod; iprod++) {
                                    data_values[ibin][k * l3b_nprod + iprod] =
                                            data_values[ibin][j * l3b_nprod + iprod];
                                }
                            }
                            k++;
                        }
                    }
                    nobs[ibin] = k;

                    if (nobs[ibin] == 0) n_filled_bins--;
                }

                if (nobs[ibin] != 0) {

                    bin = (uint64_t) ibin + basebin[krow];
                    if (isHDF4 == 1) {
                        i32 = bin;
                        memcpy(&binList[i * 19], &i32, 4);
                        memcpy(&binList[i * 19 + 4], &nobs[ibin], 2);
                        memcpy(&binList[i * 19 + 6], &nscenes[ibin], 2);
                    } else {
                        if(is64bit) {
                            binList64nc[i].binnum = bin;
                            binList64nc[i].nobs = nobs[ibin];
                            binList64nc[i].nscenes = nscenes[ibin];
                        } else {
                            binList32nc[i].binnum = bin;
                            binList32nc[i].nobs = nobs[ibin];
                            binList32nc[i].nscenes = nscenes[ibin];
                        }
                    }
                    if (bin == BINCHECK) {
                        printf("%ld %d %d\n", (long)bin, nobs[ibin], best_qual[ibin]);
                    }

                    /* weights {=sqrt(# of L2 files in given bin)} */
                    /* ------------------------------------------- */
                    wgt = 0.0;
                    for (ifile = 0; ifile <= nfiles; ifile++) {
                        i32 = 0;
                        for (j = 0; j < nobs[ibin]; j++) {
                            if (file_index[ibin][j] == ifile) i32++;
                        }
                        wgt += sqrt(i32);
                    }

                    time_rec = 0.0;
                    for (j = 0; j < nobs[ibin]; j++) time_rec += time_value[ibin][j];

                    //	  if (bin == BINCHECK) printf("%d %d %f\n", i32, i16, wgt);

                    if (isHDF4 == 1) {
                        memcpy(&binList[i * 19 + 10], &wgt, 4);
                        selcat = (tilt[ibin] << 2) | qual[ibin];
                        memcpy(&binList[i * 19 + 14], &selcat, 1);
                        memcpy(&binList[i * 19 + 15], &bin_flag[ibin], 4);
                    } else {
                        if(is64bit) {
                            binList64nc[i].weights = wgt;
                            binList64nc[i].time_rec = time_rec;
                        } else {
                            binList32nc[i].weights = wgt;
                            binList32nc[i].time_rec = time_rec;
                        }
                    }

                    i++;


                    /* Update Max/Min Lon/Lat */
                    /* ---------------------- */
                    for (j = last_group; j <= row_group; j++) {
                        if ((ibin + basebin[krow]) < basebin[krow + j]) {
                            latbin = (j + krow + 0.5) * (180.0 / nrows) - 90.0;
                            lonbin = 360.0 * (ibin + basebin[krow] - basebin[krow + j - 1] + 0.5) /
                                    numbin[krow + j - 1];
                            last_group = j;

                            break;
                        }
                    }
                    if (latbin > northmost) northmost = latbin;
                    if (latbin < southmost) southmost = latbin;

                    lonbin += seam_lon;
                    //                    if (lonbin > eastmost) eastmost = lonbin;
                    //                    if (lonbin < westmost) westmost = lonbin;
                    float minNeg = lonRanges[0]; // min, max in Western hemisphere
                    float maxNeg = lonRanges[1];
                    float minPos = lonRanges[2]; // min, max in Eastern hemisphere
                    float maxPos = lonRanges[3];
                    if (lonbin < 0) { // Western hemisphere
                        minNeg = fmin(minNeg, lonbin);
                        maxNeg = fmax(maxNeg, lonbin);

                    } else { // Eastern hemisphere
                        minPos = fmin(minPos, lonbin);
                        maxPos = fmax(maxPos, lonbin);
                    }
                    // Adjust east and west granule bounding coordinates
                    float max_angle = 90.0;
                    int8_t lon000 = ((minPos - maxNeg) < max_angle);
                    int8_t lon180 = ((maxPos - minNeg) > (360.0 - max_angle));
                    int8_t polar = (lon000 && lon180) ||
                            (90.0 - fmax(northmost, -1 * southmost) <= FLT_EPSILON);

                    if (minNeg >= maxNeg) { // Entirely in Eastern hemisphere
                        eastmost = maxPos;
                        westmost = minPos;
                    } else if (minPos >= maxPos) { // Entirely in Western hemisphere
                        eastmost = maxNeg;
                        westmost = minNeg;
                    } else if (polar) { // Polar granule
                        eastmost = 180;
                        westmost = -180;
                        if (northmost > 0)
                            northmost = 90; // North pole
                        else southmost = -90; // South pole
                    } else if (lon000) { // Granule crosses Longitude 0
                        eastmost = maxPos;
                        westmost = minNeg;
                    } else if (lon180) { // Granule crosses Longitude 180
                        eastmost = maxNeg;
                        westmost = minPos;
                    }
                    lonRanges[0] = minNeg;
                    lonRanges[1] = maxNeg;
                    lonRanges[2] = minPos;
                    lonRanges[3] = maxPos;
                } /* nobs[ibin] != 0 */
            } /* ibin loop */


            /* if no good obs left than bail */
            /* ----------------------------- */
            if (n_filled_bins == 0) goto freemem;


            /* Print info on filled row group */
            /* ------------------------------ */
            printf("%-20s:%8d\n", "# bins in row group", n_bins_in_group);
            printf("%-20s:%8d\n", "# filled bins", n_filled_bins);


            /* Write "Bin List" vdata */
            /* ---------------------- */
            if (isHDF4 == 1) {
                type[0] = DFNT_INT32;
                type[1] = DFNT_INT16;
                type[2] = DFNT_INT16;
                type[3] = DFNT_INT16;
                type[4] = DFNT_FLOAT32;
                type[5] = DFNT_INT8;
                type[6] = DFNT_INT32;

                wr_vdata(input.ofile, fileid_w, vgid_w, "BinList", "DataMain", 7,
                        n_filled_bins, fldname2, type, 0, binList, input.verbose);
                free(binList);
            } else {
                if(is64bit) {
                    writeBinList_nc(grp1, n_filled_bins, (VOIDP) binList64nc);
                    free(binList64nc);
                } else {
                    writeBinList_nc(grp1, n_filled_bins, (VOIDP) binList32nc);
                    free(binList32nc);
                }
            }


            /* Allocate sum & sum-squared arrays */
            /* --------------------------------- */
            sum_data = (float32 *) calloc(n_bins_in_group, sizeof (float32));
            sum2_data = (float32 *) calloc(n_bins_in_group, sizeof (float32));

            /* Loop over all L3 products to fill sum arrays */
            /* -------------------------------------------- */
            for (iprod = 0; iprod < l3b_nprod; iprod++) {

                memset(sum_data, 0, n_bins_in_group * sizeof (float32));
                memset(sum2_data, 0, n_bins_in_group * sizeof (float32));

                fldname3[0] = (char *) calloc(strlen(prodname[iprod]) + 5, sizeof (char));
                fldname3[1] = (char *) calloc(strlen(prodname[iprod]) + 8, sizeof (char));

                char_ptr1 = strchr(prodname[iprod], '/');
                if (char_ptr1 != NULL) *char_ptr1 = '_';

                strcpy(fldname3[0], prodname[iprod]);
                strcpy(fldname3[1], prodname[iprod]);
                strcat(fldname3[0], "_sum");
                strcat(fldname3[1], "_sum_sq");

                if (char_ptr1 != NULL) *char_ptr1 = '/';

                /* Process bins */
                /* ------------ */
                for (ibin = 0; ibin < n_bins_in_group; ibin++) {

                    if ((ibin % 10000) == 0) {
                        time(&tnow);
                        tmnow = localtime(&tnow);
                    }

                    if (nobs[ibin] == 0) continue;

                    /* Display data values if BINCHECK */
                    bin = ibin + basebin[krow];
                    if (bin == BINCHECK) {
                        kprod = 0;
                        for (j = 0; j < nobs[ibin]; j++)
                            printf("value: %10ld %3d %4d %10.4f\n",
                                (long int) bin, nobs[ibin], file_index[ibin][j],
                                data_values[ibin][j * l3b_nprod + kprod]);
                    }

                    // Dump data
                    if (BINCHECK == -2) {
                        for (j = 0; j < nobs[ibin]; j++) {
                            printf("iprod: %3d bin: %8ld f#: %3d %14.7f\n",
                                    iprod, (long int) bin, file_index[ibin][j],
                                    data_values[ibin][j * l3b_nprod + iprod]);
                        }
                    }

                    /* Process data file by file */
                    /* ------------------------- */
                    i32 = 1;
                    sum = data_values[ibin][0 * l3b_nprod + iprod];
                    sum2 = sum * sum;
                    for (j = 1; j < nobs[ibin]; j++) {
                        if (file_index[ibin][j] == file_index[ibin][j - 1]) {
                            i32++;
                            sum += data_values[ibin][j * l3b_nprod + iprod];
                            sum2 += data_values[ibin][j * l3b_nprod + iprod] *
                                    data_values[ibin][j * l3b_nprod + iprod];
                        } else {
                            sum_data[ibin] += (sum / sqrt(i32));
                            sum2_data[ibin] += (sum2 / sqrt(i32));

                            i32 = 1;
                            sum = data_values[ibin][j * l3b_nprod + iprod];
                            sum2 = sum * sum;
                        }
                    } /* observation loop */
                    sum_data[ibin] += (sum / sqrt(i32));
                    sum2_data[ibin] += (sum2 / sqrt(i32));

                } /* ibin loop */


                /* Write Product Vdatas */
                /* -------------------- */
                uint8_t *productData = (uint8_t *) calloc(8 * n_filled_bins, 1);

                /* Fill bin data array */
                /* ------------------- */
                i = 0;
                for (ibin = 0; ibin < n_bins_in_group; ibin++) {
                    if (nobs[ibin] != 0) {

                        memcpy(&productData[i * 8 ], &sum_data[ibin], 4);
                        memcpy(&productData[i * 8 + 4], &sum2_data[ibin], 4);
                        i++;
                    }
                }

                char_ptr1 = strchr(prodname[iprod], '/');
                if (char_ptr1 != NULL) *char_ptr1 = '_';

                if (isHDF4 == 1) {
                    type[0] = DFNT_FLOAT32;
                    type[1] = DFNT_FLOAT32;

                    wr_vdata(input.ofile, fileid_w, vgid_w, prodname[iprod],
                            "DataSubordinate", 2, i, fldname3, type, noext, productData,
                            input.verbose);
                } else {
                    writeBinData_nc(grp1, i, iprod, productData);
                }

                if (char_ptr1 != NULL) *char_ptr1 = '/';


                free(productData);

                free(fldname3[0]);
                free(fldname3[1]);

            } /* iprod loop */


            /* Write Quality vdata */
            /* ------------------- */
            if (input.qual_prod[0] != 0) {
                uint8_t *qualityData = (uint8_t *) calloc(n_filled_bins, 1);

                i = 0;
                for (ibin = 0; ibin < n_bins_in_group; ibin++) {
                    if (nobs[ibin] != 0) {
                        memcpy(&qualityData[i], &best_qual[ibin], 1);
                        i++;
                    }
                }

                if (isHDF4 == 1) {
                    type[0] = DFNT_UINT8;
                    wr_vdata(input.ofile, fileid_w, vgid_w, "qual_l3", "DataQuality", 1,
                            i, fldname5, type, 0, qualityData, input.verbose);
                } else {
                    writeQuality_nc(grp1, n_filled_bins, (VOIDP) qualityData);
                }

                free(qualityData);
            }


            /* Free dynamic memory */
            /* ------------------- */
            if (sum_data != NULL) free(sum_data);
            if (sum2_data != NULL) free(sum2_data);

            /* Compute "begin" & "extent" vdata entries */
            /* ---------------------------------------- */
            binnum_data = (int64_t *) calloc(n_filled_bins, sizeof (int64_t));

            i = 0;
            for (ibin = 0; ibin < n_bins_in_group; ibin++) {
                if (nobs[ibin] != 0) {
                    binnum_data[i] = (int64_t) ibin + basebin[krow];

                    if (i < 0 || i >= n_filled_bins) {
                        printf("Error: %d %d %d %d\n",
                                i, ibin, n_filled_bins, n_bins_in_group);
                    }
                    i++;
                }
            }

            get_beg_ext(n_filled_bins, binnum_data, basebin, nrows, beg, ext);

            free(binnum_data);

            total_filled_bins += n_filled_bins;


        } /* ********** n_filled_bin > 0 ********** */

        free(best_qual);

        time(&tnow);
        tmnow = localtime(&tnow);
        if (input.verbose == 1)
            printf("krow:%5d After bin processing:  %s", krow, asctime(tmnow));


        /* Fill BinIndex Vdata */
        /* ------------------- */
        for (i = 0; i < row_group; i++) {

            i32 = i + krow;

            if (i32 < 0 || i32 >= nrows) {
                printf("Error: %d %d\n", i, krow);
                exit(-1);
            }

            if (isHDF4 == 1) {
                vsize = 180.0 / nrows;
                hsize = 360.0 / numbin[i32];

                memcpy(&bin_indx[(i + krow)*36], &i32, 4);
                memcpy(&bin_indx[(i + krow)*36 + 4], &vsize, 8);
                memcpy(&bin_indx[(i + krow)*36 + 12], &hsize, 8);
                memcpy(&bin_indx[(i + krow)*36 + 20], &basebin[i32], 4);
                memcpy(&bin_indx[(i + krow)*36 + 24], &beg[i32], 4);
                memcpy(&bin_indx[(i + krow)*36 + 28], &ext[i32], 4);
                memcpy(&bin_indx[(i + krow)*36 + 32], &numbin[i32], 4);
            } else {
                if(is64bit) {
                    binIndex64nc[i + krow].start_num = basebin[i32];
                    binIndex64nc[i + krow].begin = beg[i32];
                    binIndex64nc[i + krow].extent = ext[i32];
                    binIndex64nc[i + krow].max = numbin[i32];
                } else {
                    binIndex32nc[i + krow].start_num = basebin[i32];
                    binIndex32nc[i + krow].begin = beg[i32];
                    binIndex32nc[i + krow].extent = ext[i32];
                    binIndex32nc[i + krow].max = numbin[i32];
                }
            }
        } /* row_group loop */

        /* End Bin Index Fill */


    freemem:
        /* Free Dynamic Memory */
        /* ------------------- */
        if (bin_flag != NULL) free(bin_flag);
        if (tilt != NULL) free(tilt);
        if (qual != NULL) free(qual);
        if (nscenes != NULL) free(nscenes);
        if (lastfile != NULL) free(lastfile);

        for (ifile = 0; ifile < nfiles; ifile++) {
            if (scan_in_rowgroup[ifile] != NULL) free(scan_in_rowgroup[ifile]);
        }

        time(&tnow);
        tmnow = localtime(&tnow);
        if (input.verbose == 1)
            printf("krow:%5d Befre free dynic mem:  %s", krow, asctime(tmnow));

        for (i = 0; i < n_bins_in_group; i++) {
            if (file_index[i] != NULL) free(file_index[i]);
            if (data_values[i] != NULL) free(data_values[i]);
            if (data_quality[i] != NULL) free(data_quality[i]);
            if (time_value[i] != NULL) free(time_value[i]);
        }

        time(&tnow);
        tmnow = localtime(&tnow);
        if (input.verbose == 1)
            printf("krow:%5d After free dynic mem:  %s", krow, asctime(tmnow));

        free(data_values);
        free(data_quality);
        free(time_value);
        free(nobs);
        free(allocated_space);
        free(file_index);

        krow += row_group;

    } /* ========== End krow (Main) loop ========== */
    time(&tnow);
    tmnow = localtime(&tnow);
    printf("krow:%5d %s", krow, asctime(tmnow));

    printf("total_filled_bins: %ld\n", (long int) total_filled_bins);

    if (total_filled_bins == 0) {
        strcpy(buf, "rm -f ");
        strcat(buf, input.ofile);
        strcat(buf, "*");
        printf("%s\n", buf);
        system(buf);
        ret_status = 110;
        exit(ret_status);
    }



    /* Close BinList Vdata */
    /* ------------------- */
    if (isHDF4 == 1) {
        wr_vdata(input.ofile, fileid_w, vgid_w, "BinList", "DataMain",
                7, 0, NULL, NULL, 0, NULL, input.verbose);
    }

    /* Close Product Vdatas */
    /* -------------------- */
    for (iprod = 0; iprod < l3b_nprod; iprod++) {

        char_ptr1 = strchr(prodname[iprod], '/');
        if (char_ptr1 != NULL) *char_ptr1 = '_';

        if (isHDF4 == 1) {
            wr_vdata(input.ofile, fileid_w, vgid_w, prodname[iprod],
                    "DataSubordinate",
                    2, 0, NULL, NULL, 0, NULL, input.verbose);
        }
        if (char_ptr1 != NULL) *char_ptr1 = '/';
    }


    /* Write and Close BinIndex Vdata */
    /* ------------------------------ */
    if (isHDF4 == 1) {

        type[0] = DFNT_INT32;
        type[1] = DFNT_FLOAT64;
        type[2] = DFNT_FLOAT64;
        type[3] = DFNT_INT32;
        type[4] = DFNT_INT32;
        type[5] = DFNT_INT32;
        type[6] = DFNT_INT32;

        wr_vdata(input.ofile, fileid_w, vgid_w, "BinIndex", "Index", 7, nrows,
                fldname4, type, 0, bin_indx, input.verbose);
        wr_vdata(input.ofile, fileid_w, vgid_w, "BinIndex", "Index", 7, 0,
                NULL, NULL, 0, NULL, input.verbose);
    } else {
        if(is64bit)
            writeBinIndex_nc(grp1, nrows, binIndex64nc);
        else
            writeBinIndex_nc(grp1, nrows, binIndex32nc);
    }

    /* Close Quality Vdata */
    /* ------------------- */
    if (input.qual_prod[0] != 0) {
        if (isHDF4 == 1) {
            wr_vdata(input.ofile, fileid_w, vgid_w, "qual_l3", "DataQuality", 1,
                    0, NULL, NULL, 0, NULL, input.verbose);
        }
    }
    /*
     * determine list of files actually used in the bin output
     */
    if (input.fileuse[0] != 0) {
        fp2 = fopen(input.fileuse, "w");
        for (ifile = 0; ifile < nfiles; ifile++) {
            if (brk_scan[ifile] != -9999) {
                fileused[ifile] = 1;
                fprintf(fp2, "%s\n", l2_str[ifile].filename);
            }
        }
        fclose(fp2);
    } else {
        for (ifile = 0; ifile < nfiles; ifile++) {
            if (brk_scan[ifile] != -9999) {
                fileused[ifile] = 1;
            }
        }
    }

    /* Read and write global attributes */
    /* -------------------------------- */
    printf("Writing Global Attributes\n");

    status = reopenL2(0, &l2_str[0]);
    status = readL2meta(&meta_l2, 0);

    strcpy(meta_l3b.product_name, input.ofile);
    if (meta_l2.sensor_name != NULL)
        strcpy(meta_l3b.sensor_name, meta_l2.sensor_name);
    strcpy(meta_l3b.sensor, meta_l2.sensor);
    meta_l3b.sensorID = sensorID[0];
    strcpy(meta_l3b.title, meta_l3b.sensor_name);
    strcat(meta_l3b.title, " Level-3 Binned Data");
    if (meta_l2.mission != NULL)
        strcpy(meta_l3b.mission, meta_l2.mission);
    strcpy(meta_l3b.prod_type, input.prodtype);

    strcat(meta_l3b.pversion, input.pversion);
    strcat(meta_l3b.soft_name, "l2bin");
    strcat(meta_l3b.soft_ver, VERSION);

    /*
     * loop through the input files
     *
     * set some ridiculous start and end times to get the ball rolling...
     * if this code is still in use in the 22nd century, damn we're good!
     * ...but this line will need tweaking.
     *
     */
    meta_l3b.end_orb = 0;
    meta_l3b.start_orb = 1000000;
    double startTime = yds2unix(2100, 1, 0);
    double endTime = yds2unix(1900, 1, 0);
    double tmpTime;
    strcpy(meta_l3b.infiles, basename(l2_str[0].filename));
    for (i = 0; i < nfiles; i++) {
        if (fileused[i] == 1) {
            // update orbit numbers
            if (l2_str[i].orbit > meta_l3b.end_orb)
                meta_l3b.end_orb = l2_str[i].orbit;
            if (l2_str[i].orbit < meta_l3b.start_orb)
                meta_l3b.start_orb = l2_str[i].orbit;

            // update start/end times
            tmpTime = yds2unix(l2_str[i].syear, l2_str[i].sday, (float64) (l2_str[i].smsec / 1000.0));
            if (tmpTime < startTime)
                startTime = tmpTime;
            tmpTime = yds2unix(l2_str[i].eyear, l2_str[i].eday, (float64) (l2_str[i].emsec / 1000.0));
            if (tmpTime > endTime)
                endTime = tmpTime;
        }
        // append to the file list - all files input, not just those used in the binning.
        if (i > 0) {
            strcat(meta_l3b.infiles, ",");
            strcat(meta_l3b.infiles, basename(l2_str[i].filename));
        }
    }

    meta_l3b.startTime = startTime;
    meta_l3b.endTime = endTime;

    if (strcmp(input.resolve, "1D") == 0) {
        strcpy(meta_l3b.bin_resolution, "1 degree");
    } else if (strcmp(input.resolve, "HD") == 0) {
        strcpy(meta_l3b.bin_resolution, "0.5 degree");
    } else if (strcmp(input.resolve, "QD") == 0) {
        strcpy(meta_l3b.bin_resolution, "0.25 degree");
    } else if (strcmp(input.resolve, "36") == 0) {
        strcpy(meta_l3b.bin_resolution, "36 km");
    } else if (strcmp(input.resolve, "18") == 0) {
        strcpy(meta_l3b.bin_resolution, "18.5 km");
    } else if (strcmp(input.resolve, "12") == 0) {
        strcpy(meta_l3b.bin_resolution, "12.7 km");
    } else if (strcmp(input.resolve, "9") == 0) {
        strcpy(meta_l3b.bin_resolution, "9.2 km");
    } else if (strcmp(input.resolve, "6") == 0) {
        strcpy(meta_l3b.bin_resolution, "6.4 km");
    } else if (strcmp(input.resolve, "4") == 0) {
        strcpy(meta_l3b.bin_resolution, "4.6 km");
    } else if (strcmp(input.resolve, "3") == 0) {
        strcpy(meta_l3b.bin_resolution, "3.2 km");
    } else if (strcmp(input.resolve, "3H") == 0) {
        strcpy(meta_l3b.bin_resolution, "1.6 km");
    } else if (strcmp(input.resolve, "2") == 0) {
        strcpy(meta_l3b.bin_resolution, "2.3 km");
    } else if (strcmp(input.resolve, "1") == 0) {
        strcpy(meta_l3b.bin_resolution, "1.1 km");
    } else if (strcmp(input.resolve, "H") == 0) {
        strcpy(meta_l3b.bin_resolution, "575 m");
    } else if (strcmp(input.resolve, "Q") == 0) {
        strcpy(meta_l3b.bin_resolution, "288 m");
    } else if (strcmp(input.resolve, "HQ") == 0) {
        strcpy(meta_l3b.bin_resolution, "100 m");
    } else if (strcmp(input.resolve, "HH") == 0) {
        strcpy(meta_l3b.bin_resolution, "50 m");
    } else {
        strcpy(meta_l3b.bin_resolution, "unknown");
    }

    meta_l3b.geospatial_resolution = (double) (360.0 / (nrows * 2.0));

    strcpy(meta_l3b.binning_scheme, "Integerized Sinusoidal Grid");

    meta_l3b.north = northmost;
    meta_l3b.south = southmost;
    meta_l3b.east = eastmost;
    meta_l3b.west = westmost;

    strcpy(buf, argv[0]);
    for (i = 1; i < argc; i++) {
        strcat(buf, " ");
        strcat(buf, argv[i]);
    }
    strcpy(meta_l3b.proc_con, buf);
    strcpy(meta_l3b.input_parms, input.parms);
    strcpy(meta_l3b.flag_names, input.flaguse);

    buf[0] = 0;
    for (iprod = 0; iprod < l3b_nprod; iprod++) {
        getL3units(&l2_str[0], 0, prodname[iprod], units);
        strcat(buf, prodname[iprod]);
        strcat(buf, ":");
        strcat(buf, units);
        strcat(buf, ",");
        if (strlen(buf) >= MD_ATTRSZ) {
            printf("-E- units metadata is too long.  Remove some products from product list.\n");
            exit(EXIT_FAILURE);
        }
    }
    buf[strlen(buf) - 1] = 0;
    strcpy(meta_l3b.units, buf);

    meta_l3b.data_bins = total_filled_bins;
    int64_t totalNumBins;
    totalNumBins = basebin[nrows] - 1;
    meta_l3b.pct_databins = (float) (total_filled_bins) / totalNumBins * 100.0;

    const char* doiStr = getDOI(meta_l3b.mission, meta_l3b.sensor,
                "L3B", input.suite, input.pversion);
    if(doiStr)
        strcpy(meta_l3b.doi, doiStr);

    const char* keywordStr = getGCMDKeywords(input.suite);
    if(keywordStr)
        strcpy(meta_l3b.keywords, keywordStr);

    if (isHDF4 == 1) {
        strcpy(meta_l3b.ptime, ydhmsf(tnow, 'L'));
        write_l3b_meta_hdf4(ds_id.sid, &meta_l3b);

    } else {
        strcpy(meta_l3b.ptime, unix2isodate(tnow, 'G'));
        strcpy(meta_l3b.mission, sensorId2PlatformName(sensorID[0]));
        write_l3b_meta_netcdf4(ds_id, &meta_l3b, is64bit);

    }


    freeL2meta(&meta_l2);

    /* Free Dynamic Memory */
    /* ------------------- */
    printf("Freeing Dynamic Memory\n");
    free(fileused);

    if(isHDF4)
    free(bin_indx);
    else {
        if(is64bit)
            free(binIndex64nc);
        else
            free(binIndex32nc);
    }

    free(ext);
    free(beg);

    free(numbin);
    free(basebin);

    free(prodname);
    free(min_value);

    for (ifile = 0; ifile < nfiles; ifile++) {
        if (bscan_row[ifile] != NULL) free(bscan_row[ifile]);
        if (escan_row[ifile] != NULL) free(escan_row[ifile]);

        if (numer[ifile] != NULL) free(numer[ifile]);
        if (denom[ifile] != NULL) free(denom[ifile]);
    }


    /* Close L2 input files and free allocated L2 memory */
    /* ------------------------------------------------- */
    printf("Freeing L2 arrays\n");
    closeL2(&l2_str[0], 0);
    for (ifile = 0; ifile < nfiles; ifile++) {
        //closeL2(&l2_str[ifile], ifile);
        freeL2(&l2_str[ifile]);
    }
    freeL2(NULL);


    /* Close L3B output file */
    /* --------------------- */
    if (isHDF4 == 1) {
        printf("Detaching L3B Vgroup\n");
        Vdetach(vgid_w);
        printf("End L3B Vgroup interface\n");
        Vend(fileid_w);
        printf("End L3B SD interface\n");
        SDend(sd_id_w);
        printf("Close L3B file\n");
        Hclose(fileid_w);
    } else {
        status = nc_close(fileid_w);
    }

    return ret_status;
}

int32_t compute_scanfrac(int32_t ifile, int32_t ipixl,
        uint32_t flagusemask, uint32_t required) {

    int32_t i, j;
    int32_t nsamp;
    double fdbl;
    float32 total_distance;
    float32 interp_distance;
    float32 *distance;
    float32 dtheta, dphi;
    uint32_t flagcheck;

    /* Determine distances between pixels if interpolating */
    /* --------------------------------------------------- */
    if (input.interp == 1) {

        if (strcmp(input.resolve, "1D") == 0) interp_distance = 111 * 0.75;
        if (strcmp(input.resolve, "HD") == 0) interp_distance = 56 * 0.75;
        if (strcmp(input.resolve, "36") == 0) interp_distance = 36 * 0.75;
        if (strcmp(input.resolve, "QD") == 0) interp_distance = 27 * 0.75;
        if (strcmp(input.resolve, "18") == 0) interp_distance = 18 * 0.75;
        if (strcmp(input.resolve, "9") == 0) interp_distance = 9 * 0.75;
        if (strcmp(input.resolve, "4") == 0) interp_distance = 4 * 0.75;
        if (strcmp(input.resolve, "2") == 0) interp_distance = 2 * 0.75;
        if (strcmp(input.resolve, "1") == 0) interp_distance = 1 * 0.75;
        if (strcmp(input.resolve, "H") == 0) interp_distance = 0.5 * 0.75;
        if (strcmp(input.resolve, "Q") == 0) interp_distance = 0.25 * 0.75;
        if (strcmp(input.resolve, "HQ") == 0) interp_distance = 0.1 * 0.75;
        if (strcmp(input.resolve, "HH") == 0) interp_distance = 0.05 * 0.75;

        distance = (float32 *) calloc(l2_str[ifile].nsamp, sizeof (float32));
        total_distance = 0.0;
        for (ipixl = 1; ipixl < l2_str[ifile].nsamp; ipixl++) {


            /* Don't process flagged pixels */
            /* ---------------------------- */
            flagcheck = (l2_str[ifile].l2_flags[ipixl] |
                    l2_str[ifile].l2_flags[ipixl - 1]);
            if ((flagcheck & flagusemask) != 0)
                continue;
            if ((flagcheck & required) != required)
                continue;


            /* Compute delta theta & phi */
            /* ------------------------- */
            dtheta = l2_str[ifile].latitude[ipixl] -
                    l2_str[ifile].latitude[ipixl - 1];
            dphi = l2_str[ifile].longitude[ipixl] -
                    l2_str[ifile].longitude[ipixl - 1];


            /* Correct for dateline crossing */
            /* ----------------------------- */
            if (dphi < -90) dphi += 360;
            if (dphi > +90) dphi -= 360;

            fdbl = dphi * cos((PI / 180) * l2_str[ifile].latitude[ipixl]);
            distance[ipixl] = fdbl * fdbl;

            fdbl = dtheta * dtheta;
            distance[ipixl] += fdbl;
            distance[ipixl] = (PI / 180) * sqrt(distance[ipixl]) * EARTH_RADIUS;

            total_distance += distance[ipixl];
        } /* ipixl loop */

        /* # of samples = total scan distance / width of "middle" pixel */
        nsamp = rint((float64) (total_distance / interp_distance));

        fdbl = total_distance / nsamp;

        distance[0] = 0.0;
        for (ipixl = 1; ipixl < l2_str[ifile].nsamp; ipixl++) {
            distance[ipixl] = distance[ipixl - 1] + distance[ipixl];
        }

    } else {
        /* No interp */
        /* --------- */
        nsamp = l2_str[ifile].nsamp;
    } /* 	if (input.interp == 1) ... */



    /* Compute scan_frac */
    /* ----------------- */
    scan_frac = (double *) calloc(nsamp, sizeof (double));

    if (input.interp == 1) {
        ipixl = 0;
        for (i = 0; i < nsamp; i++) {
            for (j = ipixl; j < l2_str[ifile].nsamp; j++) {
                if ((i + 0.5) * fdbl >= distance[j] && (i + 0.5) * fdbl < distance[j + 1]) {
                    ipixl = j;
                    /*printf("%f %f %f\n", distance[j], i*fdbl, distance[j+1]);*/
                    break;
                }
            }
            scan_frac[i] = j +
                    ((i + 0.5) * fdbl - distance[j]) / (distance[j + 1] - distance[j]);
        }
    } else {
        /* No interp */
        /* --------- */
        for (i = 0; i < nsamp; i++) scan_frac[i] = (float32) i;
    }

    if (input.interp == 1) free(distance);

    return (nsamp);
}

int64_t getbinnum(int32_t ifile, int32_t ipixl, int32_t isamp) {

    int64_t bin_row;
    int64_t bin_col;
    double fdbl;
    float scan_lon;
    float scan_lat;


    if (input.interp == 1) {
        scan_lat = l2_str[ifile].latitude[ipixl] +
                (scan_frac[isamp] - ipixl) *
                (l2_str[ifile].latitude[ipixl + 1] - l2_str[ifile].latitude[ipixl]);

        /* Handle dateline crossing for longitude */
        /* -------------------------------------- */
        fdbl = l2_str[ifile].longitude[ipixl + 1] - l2_str[ifile].longitude[ipixl];
        if (fabs(fdbl) < 90) {
            scan_lon = l2_str[ifile].longitude[ipixl] +
                    (scan_frac[isamp] - ipixl) * fdbl;
        } else if (fdbl < 0) {
            scan_lon = l2_str[ifile].longitude[ipixl] + (scan_frac[isamp] - ipixl) *
                    (360.0 + fdbl);
            if (scan_lon > +180.0) scan_lon -= 360.0;
        } else if (fdbl > 0) {
            scan_lon = l2_str[ifile].longitude[ipixl] + (scan_frac[isamp] - ipixl) *
                    (fdbl - 360.0);
            if (scan_lon < -180.0) scan_lon += 360.0;
        }

    } else {
        /* No interp */
        /* --------- */
        scan_lat = l2_str[ifile].latitude[ipixl];
        scan_lon = l2_str[ifile].longitude[ipixl];
    }


    bin_row = (int64_t) ((double)(90.0 + scan_lat) *
            ((double) nrows / (double) 180.0));

    if (bin_row < 0 || bin_row >= nrows) {
        //    printf("bin_row: %d out of bounds. (%f)\n", bin_row, scan_lat);
        return -1;
    }

    bin_col = (int64_t) ((double) numbin[bin_row] *
            (double)(scan_lon + 180.0) / (double) 360.0);

    return (basebin[bin_row] + bin_col);
}
