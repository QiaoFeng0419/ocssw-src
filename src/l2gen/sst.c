#include <sys/types.h>
#include <unistd.h>

/* ============================================================================ */
/* sst.c - functions for retrieval of sea surface temperature                   */
/*                                                                              */
/* Written By: B. Franz, NASA/SIMBIOS, August 2003.                             */
/*                                                                              */
/* ============================================================================ */

/*
 * nightcube input parameter eliminated - hardcoded to 1 in calls to the sses
 * functions.  This is due to the note in comp_sses_sst:
 *    "GHRSST sses defintion is that the sses are based on night stats only"
 *  Since the SSES products are only for GHRSST, silly to make it an option...
 * But July 2017, GHRSST definition has changed and viirs now has day and night sses
 * so hardcoding it in (for now?  Why was there an option before?)
 */
#include "l12_proto.h"
#include "flags_sst.h"
#include "d3940tref.h"
#include "l1_aci_hdf.h"
#include "sst_dsdi.h"

static float sstbad = BAD_FLT;
static int32 evalmask = 0;


typedef struct stat_struct {
    float min;
    float max;
    float avg;
    float med;
    float sqr;
    float sd;
    float cen;
    int cnt;
} statstr;

#define CtoK         273.15    /* conversion between degree C and Kelvin*/

/* sst sensor-specific error table (SSES) v6 viirs */
#define NSSTMAXv6v   7 /* was 8 */
#define NDAYMAXv6v   2 /* viirs sst now has day and night sses */
#define NDAYMAXv6v3  1 /* viirs sst3 is night only */
#define NQUARMAXv6v  4
#define NSENZMAXv6v  4
#define NDIFFMAXv6v  4
#define NDIFFMAXv6v3 4
#define NLATMAXv6v   6 /* was 5 */
#define NQUALMAXv6v  5 /* was 4, then 5 */
#define NSSESDIMv6v  7
/* sst sensor-specific error table (SSES) v6 modis */
#define NSSTMAXv6m   7 /* was 8 */
#define NDAYMAXv6m   2 /* sst now has day and night sses */
#define NDAYMAXv6m4  1 /* sst4 is night only */
#define NQUARMAXv6m  4
#define NSENZMAXv6m  4
#define NDIFFMAXv6m  4
#define NDIFFMAXv6m4 4
#define NLATMAXv6m   7 /* was 5 */
#define NQUALMAXv6m  5 /* was 4, then 5 */
#define NSSESDIMv6m  7
/* sst sensor-specific error table (SSES) v6 avhrr */
#define NSSTMAXv6a   7
#define NQUARMAXv6a  4
#define NSENZMAXv6a  4
#define NDIFFMAXv6a  4
#define NLATMAXv6a   6
#define NQUALMAXv6a  9
#define NSSESDIMv6a  6
/* avhrr sensor-specific error table (SSES)
 * -2 to 3C, 3+ to 8C, 8+ to 13C, 13+ to 18C, 18+ to 23C, 23+ to 28C, > 28C
 *	set hdf limits to: -2, 3, 8, 13, 18, 23, 28
 * Day, Night
 * 1Q, 2Q, 3Q, 4Q
 * 0 to 30 deg, 30+ to 40 deg, 40+ to 50 deg, 50+ deg
 *	set hdf limits to : 0, 30, 40, 50
 * sst: < 0.0C, 0.0 to 0.7C, 0.7+ to 2.0C, > 2.0C
 * sst:   set hdf limits to -1000, 0.0, 0.7, 2.0
 * sst4:< -0.5C, -0.5 to 0.0C, 0.0+ to 0.5C, > 0.5C
 * sst4:  set hdf limits to -1000, -0.5, 0.0, 0.5
 * 90S to 40S, 40S+ to 20S, 20S+ to Eq, Eq+ to 20N, 20N+ to 40N, 40N+ to 90N
 *	set hdf lower limits to -90 -40, -20, 0, 20, 40
 * avhrr: 0, 1, 2, 3, 4, 5, 6, 7, 8, modis: 0, 1, 2, 3, 4
 *	set hdf lower limits to avhrr: 0, 1, 2, 3, 4, modis: 0, 1, 2, 3, 4
 * use bias=-10, stdv=5 for avhrr qual 8, and modis qual 4
 */

/* this v6 structure is for viirs */

typedef struct ssestab_structv6v {
    int nsst;
    int nday;
    int nquar;
    int nsenz;
    int ndiff;
    int nlat;
    int nqual;

    float sst[NSSTMAXv6v];
    float senz[NSENZMAXv6v];
    float diff[NDIFFMAXv6v];
    float lat[NLATMAXv6v];
    float qual[NQUALMAXv6v];

    float bias[NQUALMAXv6v][NLATMAXv6v][NDIFFMAXv6v][NSENZMAXv6v][NQUARMAXv6v][NDAYMAXv6v][NSSTMAXv6v];
    float stdv[NQUALMAXv6v][NLATMAXv6v][NDIFFMAXv6v][NSENZMAXv6v][NQUARMAXv6v][NDAYMAXv6v][NSSTMAXv6v];
    float bias_mean[NQUALMAXv6v][NLATMAXv6v][NDIFFMAXv6v][NSENZMAXv6v][NQUARMAXv6v][NDAYMAXv6v][NSSTMAXv6v];
    int16 counts[NQUALMAXv6v][NLATMAXv6v][NDIFFMAXv6v][NSENZMAXv6v][NQUARMAXv6v][NDAYMAXv6v][NSSTMAXv6v];

} ssestabstrv6v;

/* structure for viirs sst3 stats, with different NDAY and NDIFF sizes */
typedef struct ssestab_structv6v3 {
    int nsst;
    int nday;
    int nquar;
    int nsenz;
    int ndiff;
    int nlat;
    int nqual;

    float sst[NSSTMAXv6v];
    float senz[NSENZMAXv6v];
    float diff[NDIFFMAXv6v3];
    float lat[NLATMAXv6v];
    float qual[NQUALMAXv6v];

    float bias[NQUALMAXv6v][NLATMAXv6v][NDIFFMAXv6v3][NSENZMAXv6v][NQUARMAXv6v][NDAYMAXv6v3][NSSTMAXv6v];
    float stdv[NQUALMAXv6v][NLATMAXv6v][NDIFFMAXv6v3][NSENZMAXv6v][NQUARMAXv6v][NDAYMAXv6v3][NSSTMAXv6v];
    float bias_mean[NQUALMAXv6v][NLATMAXv6v][NDIFFMAXv6v3][NSENZMAXv6v][NQUARMAXv6v][NDAYMAXv6v3][NSSTMAXv6v];
    int16 counts[NQUALMAXv6v][NLATMAXv6v][NDIFFMAXv6v3][NSENZMAXv6v][NQUARMAXv6v][NDAYMAXv6v3][NSSTMAXv6v];

} ssestabstrv6v3;

static ssestabstrv6v sses_sstv6v;
static ssestabstrv6v3 sses_sst3v6v3;

/* this v6 structure is for modis */

typedef struct ssestab_structv6m {
    int nsst;
    int nday;
    int nquar;
    int nsenz;
    int ndiff;
    int nlat;
    int nqual;

    float sst[NSSTMAXv6m];
    float senz[NSENZMAXv6m];
    float diff[NDIFFMAXv6m];
    float lat[NLATMAXv6m];
    float qual[NQUALMAXv6m];

    float bias[NQUALMAXv6m][NLATMAXv6m][NDIFFMAXv6m][NSENZMAXv6m][NQUARMAXv6m][NDAYMAXv6m][NSSTMAXv6m];
    float stdv[NQUALMAXv6m][NLATMAXv6m][NDIFFMAXv6m][NSENZMAXv6m][NQUARMAXv6m][NDAYMAXv6m][NSSTMAXv6m];
    float bias_mean[NQUALMAXv6m][NLATMAXv6m][NDIFFMAXv6m][NSENZMAXv6m][NQUARMAXv6m][NSSTMAXv6m];
    int16 counts[NQUALMAXv6m][NLATMAXv6m][NDIFFMAXv6m][NSENZMAXv6m][NQUARMAXv6m][NSSTMAXv6m];

} ssestabstrv6m;

/* structure for modis sst4 stats, with different NDAY and NDIFF sizes */
typedef struct ssestab_structv6m4 {
    int nsst;
    int nday;
    int nquar;
    int nsenz;
    int ndiff;
    int nlat;
    int nqual;

    float sst[NSSTMAXv6m];
    float senz[NSENZMAXv6m];
    float diff[NDIFFMAXv6m4];
    float lat[NLATMAXv6m];
    float qual[NQUALMAXv6m];

    float bias[NQUALMAXv6m][NLATMAXv6m][NDIFFMAXv6m4][NSENZMAXv6m][NQUARMAXv6m][NDAYMAXv6m4][NSSTMAXv6m];
    float stdv[NQUALMAXv6m][NLATMAXv6m][NDIFFMAXv6m4][NSENZMAXv6m][NQUARMAXv6m][NDAYMAXv6m4][NSSTMAXv6m];
    float bias_mean[NQUALMAXv6m][NLATMAXv6m][NDIFFMAXv6m4][NSENZMAXv6m][NQUARMAXv6m][NDAYMAXv6m4][NSSTMAXv6m];
    int16 counts[NQUALMAXv6m][NLATMAXv6m][NDIFFMAXv6m4][NSENZMAXv6m][NQUARMAXv6m][NDAYMAXv6m4][NSSTMAXv6m];

} ssestabstrv6m4;
static ssestabstrv6m sses_sstv6m;
static ssestabstrv6m4 sses_sst4v6m;

/* this sses structure is for avhhr which has more quality levels and modis and viirs */
typedef struct ssestab_structv6a {
    int nsst;
    int nquar;
    int nsenz;
    int ndiff;
    int nlat;
    int nqual;

    float sst[NSSTMAXv6a];
    float senz[NSENZMAXv6a];
    float diff[NDIFFMAXv6a];
    float lat[NLATMAXv6a];
    float qual[NQUALMAXv6a];

    float bias[NQUALMAXv6a][NLATMAXv6a][NDIFFMAXv6a][NSENZMAXv6a][NQUARMAXv6a][NSSTMAXv6a];
    float stdv[NQUALMAXv6a][NLATMAXv6a][NDIFFMAXv6a][NSENZMAXv6a][NQUARMAXv6a][NSSTMAXv6a];
    float bias_mean[NQUALMAXv6a][NLATMAXv6a][NDIFFMAXv6a][NSENZMAXv6a][NQUARMAXv6a][NSSTMAXv6a];
    int16 counts[NQUALMAXv6a][NLATMAXv6a][NDIFFMAXv6a][NSENZMAXv6a][NQUARMAXv6a][NSSTMAXv6a];

} ssestabstrv6a;

static ssestabstrv6a sses_sstv6a;

/* scans of computed quantities */
typedef float s_array[MAXPIX];
static s_array *sstq, *sst4q, *sst3q;
//static float *sst = NULL;
//static float *sst4 = NULL;
//static float *sst3 = NULL;
static float *treesum = NULL;
static int16 *flags_sst = NULL;
static int16 *flags_sst4 = NULL;
static int16 *flags_sst3 = NULL;
static int8 *qual_sst = NULL;
static int8 *qual_sst4 = NULL;
static int8 *qual_sst3 = NULL;
static float *bias_sst = NULL;
static float *bias_sst4 = NULL;
static float *bias_sst3 = NULL;
static float *stdv_sst = NULL;
static float *stdv_sst4 = NULL;
static float *stdv_sst3 = NULL;
static float *bias_mean_sst = NULL;
static float *bias_mean_sst4 = NULL;
static float *bias_mean_sst3 = NULL;
static int16 *bias_counts_sst = NULL;
static int16 *bias_counts_sst4 = NULL;
static int16 *bias_counts_sst3 = NULL;
static float *dsdi_correction = NULL;

static float *d3940ref = NULL;

// set up arrays for box
static float *LtRED_maxmin = NULL;
static float *Bt11_maxmin = NULL;
static float *Bt11_max = NULL;
static float *Bt11_min = NULL;
static float *Bt11_stdev = NULL;
static float *Bt12_maxmin = NULL;
static float *Bt12_min = NULL;
static float *Bt37_maxmin = NULL;
static float *Bt37_stdev = NULL;
static float *Bt39_maxmin = NULL;
static float *Bt40_maxmin = NULL;
//static float *Bt40_avg = NULL;
static float *Bt40_stdev = NULL;
static float *Bt85_min = NULL;
static float *Bt73_max = NULL;
static float *rhoCirrus_maxmin = NULL;
static float *rhoCirrus_min = NULL;
static float *rhoCirrus_max = NULL;
static float *rhotRED_maxmin = NULL;
static float *rhotRED_min = NULL;
static float *rhotRED_max = NULL;
static float *rhotNIR7_min = NULL;
static float *rhot16_min = NULL;
static float *rhotRED = NULL;
static float *rhotNIR7 = NULL;
static float *rhot16 = NULL;
static float *sst_stdev = NULL;

/* precomputed indices */
static int32_t recnumSST = -1;
static int haveSST4 = 0;
static int haveSST = 0;
static int haveSSES = 1;
static int haveRed = 0;
static int ib07 = -1;
static int ib08 = -1;
static int ib16 = -1;
static int ib37 = -1;
static int ib39 = -1;
static int ib40 = -1;
static int ib67 = -1;
static int ib73 = -1;
static int ib85 = -1;
static int ib11 = -1;
static int ib12 = -1;
static int ibred = -1;
static int nbvis = -1;
static int nbir = -1;
static float satred;
static int isV5 = 0;

/* quality test parameters */
static int fullscanpix = 1354; // intialize to modis, will get set in init_sst for others
static int32_t cldbox = 3;
static int32_t cldboxv = 5;
static int sstboxcscan = -1;
static float cldthresh = 0.01; /* modis */
static float cldthreshv = 0.04; /* viirs */
/* Sue read the comment before changing btbox for aqua !!!!!!!!*/
static int32_t btbox = 3; /* SUE: read the next comment!!!!! */
/* if btbox changes to 5 for aqua then change modisa/msl12_filter.dat
 * to keep 7 lines for btavg (Bt40 detector zero replacement) to work
 */
static int32_t btboxv = 5;
static int32_t csstbox = -1;
static float hisenz = 55.0;
static float hisenza = 45.0;
static float vhisenz = 75.0;
static float vhisenza = 55.0; /* make this higher?  ask Bob */
static float vhisenzv2 = 65.0; /* for VIIRS v6.4 sst2b May 2015 */
static float solznight = SOLZNIGHT; /* becomes SOLZNIGHTA for AVHRR */
static float Btmin = -4.0;
static float Btmina = -10.0;
//static float Btminv     = -4.0;//+CtoK;
static float Btmax = 37.0; /* pre Nov 2012 was 33.0 */
static float Btmaxa = 37.0; /* pre Nov 2012 was 35.0 */
//static float Btmaxv     = 37.0;//+CtoK;	/* pre Nov 2012 was 33.0+CtoK */
static float Btmax40 = 35.0;
//static float Btmax40v   = 35.0;//+CtoK;
static float SSTmin = -1.8; /* Peter Minnett, 2017: should be -1.8 instead of -2.0 */
static float SSTmax = 40.0; /* pre Nov 2012 was 45.0 */
static float SSTmaxa = 40.0; /* pre Nov 2012 was 35.0 */
static float SSTmaxn = 37.0; /* night sst max */
static float glintmax = 0.005;
static float dBtmin = 0.0;
static float dBtmax = 3.6;
static float dBt4min = 0.0;
static float dBt4max = 8.0;
static float SSTdiffa = 2.0;
static float SSTdiff = 3.0;
/* Sue: test Nov 2016 - use sstvdiff instead of sstdiff */
/*
 * Kay discovered an issue...
 * 1. The two thresholds, SST4diff1,and SST4diff2,  should be negative, -0.8, and -1.0,
respectively
 * 2. The comparisons should be less than  eg. dSST< SST4diff1  and dSST < SSTdiff2
 */
//static float SSTdiff = 5.0;
static float SSTvdiff = 5.0;
static float SST4diff1 = -0.8;
static float SST4diff2 = -1.0;
static float SST3diff1 = 0.8;
static float SST3diff2 = 1.0;
/* Bt11unif1 changes in run_avhrr_sst because it varies by satellite */
static float Bt11unif1 = 0.7;
static float Bt12unif1 = 0.7;
static float Bt11unif2 = 1.2;
static float Bt12unif2 = 1.2;
static float Bt37unif1 = 0.7;
static float Bt37unif2 = 1.2;
static float Bt39unif1 = 0.7;
static float Bt40unif1 = 0.7;
static float Bt39unif2 = 1.2;
static float Bt40unif2 = 1.2;
static float dBtrefmin = -1.1;
static float dBtrefmax = 10.0;
/* tighter test between 10S and 30N and -105 to 105 longitude */
static float equatorialNorth = 30.0;
static float equatorialSouth = -10.0;
static float equatorialWest = -105.0;
static float equatorialEast = 105.0;

/* NOTE: flag bit settings are now in flags_sst.h */
/* flag bit settings */

static float latwin = 2.5; /* half the overlap for lat band coeffs */

static int32 tmonth = -1;

static int StartOfMonth[2][12] = {
    { 0, 31, 59, 90, 120, 151, 181, 212, 243,
        273, 304, 334},
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
};

/* ----------------------------------------------------------------------------------- */
/* btrefdiffv6() - scan-dependent test on 4um BT temps to reference (RSMAS)            */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
float btrefdiffv6(int32_t ip, float BT39, float BT40, l2str *l2rec) {
    /* Gui's new table is by senz, which goes from - to + */
    /* sza and trefv6 are in the d3940tref.h include file */

    float diff;
    float satzdir;
    float senz;
    float tref;

    /* this WILL now work if working on a subset of the data (spixl or epixl were specified) */
    /* viirs has 3200 pixels per scan, pixnum's start at zero
     * so we want pixels 0..1599 in the first half and 1600..3199 in the 2nd */
    satzdir = (l2rec->l1rec->pixnum[ip] < fullscanpix / 2) ? -1.0 : 1.0;
    senz = l2rec->l1rec->senz[ip] * satzdir;
    tref = linterp(sza, trefv6, NSENZ, senz);
    diff = BT39 - BT40 - tref;

    return (diff);
}

/* ----------------------------------------------------------------------------------------------- */
/* load_sses_sstv6v() - loads the specified SSES (sensor-specific error stats) table for VIIRS sst */

/* ----------------------------------------------------------------------------------------------- */
void load_sses_sstv6v(int32_t sensorID, char *file, ssestabstrv6v *sses) {
    int32 sd_id;
    int32 sds_id;
    int32 status;
    int32 rank;
    int32 nt;
    int32 nattrs;
    int32 dims[NSSESDIMv6v];
    int32 start[NSSESDIMv6v];

    char name[H4_MAX_NC_NAME] = "";
    char sdsname[H4_MAX_NC_NAME] = "";

    memset(start, 0, NSSESDIMv6v * sizeof (int32));

    if (strcmp(file, "") == 0) {
        printf("\nNo SSES data provided for this sensor.\n");
        haveSSES = 0;
        return;
    }

    printf("\nLoading SSES table from %s\n", file);

    /* open the file and initiate the SD interface */
    sd_id = SDstart(file, DFACC_RDONLY);
    if (sd_id == -1) {
        printf("-E- %s line %d:  Error opening file %s.\n", __FILE__, __LINE__,
                file);
        exit(1);
    }

    /* read the bias and standard deviation */

    strcpy(sdsname, "bias");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation. got: %d expected %d\n",
                __FILE__, __LINE__, sdsname, rank, NSSESDIMv6v);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6v || dims[1] != NLATMAXv6v
            || dims[2] != NDIFFMAXv6v || dims[3] != NSENZMAXv6v
            || dims[4] != NQUARMAXv6v || dims[5] != NDAYMAXv6v
            || dims[6] != NSSTMAXv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->bias);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    strcpy(sdsname, "stdv");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6v || dims[1] != NLATMAXv6v
            || dims[2] != NDIFFMAXv6v || dims[3] != NSENZMAXv6v
            || dims[4] != NQUARMAXv6v || dims[5] != NDAYMAXv6v
            || dims[6] != NSSTMAXv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->stdv);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    /* new hypercubes have bias_mean and counts also */

    strcpy(sdsname, "bias_mean");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6v || dims[1] != NLATMAXv6v
            || dims[2] != NDIFFMAXv6v || dims[3] != NSENZMAXv6v
            || dims[4] != NQUARMAXv6v || dims[5] != NDAYMAXv6v
            || dims[6] != NSSTMAXv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->bias_mean);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    strcpy(sdsname, "counts");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6v || dims[1] != NLATMAXv6v
            || dims[2] != NDIFFMAXv6v || dims[3] != NSENZMAXv6v
            || dims[4] != NQUARMAXv6v || dims[5] != NDAYMAXv6v
            || dims[6] != NSSTMAXv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->counts);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    /* save the table dimensions */
    sses->nqual = dims[0];
    sses->nlat = dims[1];
    sses->ndiff = dims[2];
    sses->nsenz = dims[3];
    sses->nquar = dims[4];
    sses->nday = dims[5];
    sses->nsst = dims[6];

    /* read the table indice ranges */

    strcpy(sdsname, "sst");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->sst);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nsst = dims[0];

    strcpy(sdsname, "senz");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->senz);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nsenz = dims[0];

    strcpy(sdsname, "BTdiff");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->diff);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->ndiff = dims[0];

    strcpy(sdsname, "lat");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->lat);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nlat = dims[0];

    /* terminate access to the SD interface and close the file */
    status = SDend(sd_id);
}

/* ------------------------------------------------------------------------------------------------ */
/* load_sses_sstv6v3() - loads the specified SSES (sensor-specific error stats) table for VIIRS sst3 */

/* ------------------------------------------------------------------------------------------------ */
void load_sses_sstv6v3(int32_t sensorID, char *file, ssestabstrv6v3 *sses) {
    int32 sd_id;
    int32 sds_id;
    int32 status;
    int32 rank;
    int32 nt;
    int32 nattrs;
    int32 dims[NSSESDIMv6v];
    int32 start[NSSESDIMv6v];

    char name[H4_MAX_NC_NAME] = "";
    char sdsname[H4_MAX_NC_NAME] = "";

    memset(start, 0, NSSESDIMv6v * sizeof (int32));

    if (strcmp(file, "") == 0) {
        printf("\nNo SSES data provided for this sensor.\n");
        haveSSES = 0;
        return;
    }

    printf("\nLoading SSES table from %s\n", file);

    /* open the file and initiate the SD interface */
    sd_id = SDstart(file, DFACC_RDONLY);
    if (sd_id == -1) {
        printf("-E- %s line %d:  Error opening file %s.\n", __FILE__, __LINE__,
                file);
        exit(1);
    }

    /* read the bias and standard deviation */

    strcpy(sdsname, "bias");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation. got: %d expected %d\n",
                __FILE__, __LINE__, sdsname, rank, NSSESDIMv6v);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6v || dims[1] != NLATMAXv6v
            || dims[2] != NDIFFMAXv6v3 || dims[3] != NSENZMAXv6v
            || dims[4] != NQUARMAXv6v || dims[5] != NDAYMAXv6v3
            || dims[6] != NSSTMAXv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->bias);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    strcpy(sdsname, "stdv");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6v || dims[1] != NLATMAXv6v
            || dims[2] != NDIFFMAXv6v3 || dims[3] != NSENZMAXv6v
            || dims[4] != NQUARMAXv6v || dims[5] != NDAYMAXv6v3
            || dims[6] != NSSTMAXv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->stdv);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    /* new hypercubes have bias_mean and counts also */

    strcpy(sdsname, "bias_mean");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6v || dims[1] != NLATMAXv6v
            || dims[2] != NDIFFMAXv6v3 || dims[3] != NSENZMAXv6v
            || dims[4] != NQUARMAXv6v || dims[5] != NDAYMAXv6v3
            || dims[6] != NSSTMAXv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->bias_mean);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    strcpy(sdsname, "counts");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6v || dims[1] != NLATMAXv6v
            || dims[2] != NDIFFMAXv6v3 || dims[3] != NSENZMAXv6v
            || dims[4] != NQUARMAXv6v || dims[5] != NDAYMAXv6v3
            || dims[6] != NSSTMAXv6v) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->counts);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    /* save the table dimensions */
    sses->nqual = dims[0];
    sses->nlat = dims[1];
    sses->ndiff = dims[2];
    sses->nsenz = dims[3];
    sses->nquar = dims[4];
    sses->nday = dims[5];
    sses->nsst = dims[6];

    /* read the table indice ranges */

    strcpy(sdsname, "sst");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->sst);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nsst = dims[0];

    strcpy(sdsname, "senz");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->senz);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nsenz = dims[0];

    strcpy(sdsname, "BTdiff");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->diff);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->ndiff = dims[0];

    strcpy(sdsname, "lat");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->lat);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nlat = dims[0];

    /* terminate access to the SD interface and close the file */
    status = SDend(sd_id);
}

/* ---------------------------------------------------------------------------------------- */
/* load_sses_sstv6m() - loads the specified SSES (sensor-specific error stats) table           */

/* ---------------------------------------------------------------------------------------- */
void load_sses_sstv6m(int32_t sensorID, char *file, ssestabstrv6m *sses) {
    int32 sd_id;
    int32 sds_id;
    int32 status;
    int32 rank;
    int32 nt;
    int32 nattrs;
    int32 dims[NSSESDIMv6m];
    int32 start[NSSESDIMv6m];

    char name[H4_MAX_NC_NAME] = "";
    char sdsname[H4_MAX_NC_NAME] = "";

    memset(start, 0, NSSESDIMv6m * sizeof (int32));

    if (strcmp(file, "") == 0) {
        printf("\nNo SSES data provided for this sensor.\n");
        haveSSES = 0;
        return;
    }

    printf("\nLoading SSES table from %s\n", file);

    /* open the file and initiate the SD interface */
    sd_id = SDstart(file, DFACC_RDONLY);
    if (sd_id == -1) {
        printf("-E- %s line %d:  Error opening file %s.\n", __FILE__, __LINE__,
                file);
        exit(1);
    }

    /* read the bias and standard deviation */

    strcpy(sdsname, "bias");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6m) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation. got: %d expected %d\n",
                __FILE__, __LINE__, sdsname, rank, NSSESDIMv6m);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6m || dims[1] != NLATMAXv6m
            || dims[2] != NDIFFMAXv6m || dims[3] != NSENZMAXv6m
            || dims[4] != NQUARMAXv6m || dims[5] > NDAYMAXv6m
            || dims[6] != NSSTMAXv6m) {
        printf("%d,%d,%d,%d,%d,%d,%d\n",dims[0],dims[1],dims[2],dims[3],dims[4],dims[5],dims[6]);
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->bias);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    strcpy(sdsname, "stdv");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6m) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6m || dims[1] != NLATMAXv6m
            || dims[2] != NDIFFMAXv6m || dims[3] != NSENZMAXv6m
            || dims[4] != NQUARMAXv6m || dims[5] > NDAYMAXv6m
            || dims[6] != NSSTMAXv6m) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->stdv);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    /* new hypercubes have bias_mean and counts also */

    strcpy(sdsname, "bias_mean");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6m) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6m || dims[1] != NLATMAXv6m
            || dims[2] != NDIFFMAXv6m || dims[3] != NSENZMAXv6m
            || dims[4] != NQUARMAXv6m || dims[5] > NDAYMAXv6m
            || dims[6] != NSSTMAXv6m) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
//    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->bias_mean);
//    if (status != 0) {
//        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
//                __LINE__, sdsname, file);
//        exit(1);
//    }
//    status = SDendaccess(sds_id);
//
//    strcpy(sdsname, "counts");
//    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
//    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
//    if (rank != NSSESDIMv6m) {
//        printf(
//                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
//                __FILE__, __LINE__, sdsname);
//        exit(1);
//    }
//    if (dims[0] != NQUALMAXv6m || dims[1] != NLATMAXv6m
//            || dims[2] != NDIFFMAXv6m || dims[3] != NSENZMAXv6m
//            || dims[4] != NQUARMAXv6m || dims[5] != NDAYMAXv6m
//            || dims[6] != NSSTMAXv6m) {
//            printf(
//                    "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
//                __FILE__, __LINE__, sdsname);
//        exit(1);
//    }
//    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->counts);
//    if (status != 0) {
//        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
//                __LINE__, sdsname, file);
//        exit(1);
//    }
    status = SDendaccess(sds_id);

    /* save the table dimensions */
    sses->nqual = dims[0];
    sses->nlat = dims[1];
    sses->ndiff = dims[2];
    sses->nsenz = dims[3];
    sses->nquar = dims[4];
    sses->nday = dims[5];
    sses->nsst = dims[6];

    /* read the table indice ranges */

    strcpy(sdsname, "sst");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->sst);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nsst = dims[0];

    strcpy(sdsname, "senz");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->senz);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nsenz = dims[0];

    strcpy(sdsname, "BTdiff");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->diff);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->ndiff = dims[0];

    strcpy(sdsname, "lat");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->lat);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nlat = dims[0];

    strcpy(sdsname, "lat");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->lat);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nlat = dims[0];

    /* terminate access to the SD interface and close the file */
    status = SDend(sd_id);
}

/* ---------------------------------------------------------------------------------------- */
/* load_sses_sstv6m4() - loads the specified SST4 SSES (sensor-specific error stats) table           */

/* ---------------------------------------------------------------------------------------- */
void load_sses_sstv6m4(int32_t sensorID, char *file, ssestabstrv6m4 *sses) {
    int32 sd_id;
    int32 sds_id;
    int32 status;
    int32 rank;
    int32 nt;
    int32 nattrs;
    int32 dims[NSSESDIMv6m];
    int32 start[NSSESDIMv6m];

    char name[H4_MAX_NC_NAME] = "";
    char sdsname[H4_MAX_NC_NAME] = "";

    memset(start, 0, NSSESDIMv6m * sizeof (int32));

    if (strcmp(file, "") == 0) {
        printf("\nNo SSES data provided for this sensor.\n");
        haveSSES = 0;
        return;
    }

    printf("\nLoading SSES table from %s\n", file);

    /* open the file and initiate the SD interface */
    sd_id = SDstart(file, DFACC_RDONLY);
    if (sd_id == -1) {
        printf("-E- %s line %d:  Error opening file %s.\n", __FILE__, __LINE__,
                file);
        exit(1);
    }

    /* read the bias and standard deviation */

    strcpy(sdsname, "bias");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6m) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation. got: %d expected %d\n",
                __FILE__, __LINE__, sdsname, rank, NSSESDIMv6m);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6m || dims[1] != NLATMAXv6m
            || dims[2] != NDIFFMAXv6m4 || dims[3] != NSENZMAXv6m
            || dims[4] != NQUARMAXv6m || dims[5] > NDAYMAXv6m4
            || dims[6] != NSSTMAXv6m) {
        printf("%d,%d,%d,%d,%d,%d,%d\n",dims[0],dims[1],dims[2],dims[3],dims[4],dims[5],dims[6]);
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->bias);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    strcpy(sdsname, "stdv");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6m) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6m || dims[1] != NLATMAXv6m
            || dims[2] != NDIFFMAXv6m4 || dims[3] != NSENZMAXv6m
            || dims[4] != NQUARMAXv6m || dims[5] > NDAYMAXv6m4
            || dims[6] != NSSTMAXv6m) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->stdv);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    /* new hypercubes have bias_mean and counts also */

    strcpy(sdsname, "bias_mean");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6m) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6m || dims[1] != NLATMAXv6m
            || dims[2] != NDIFFMAXv6m || dims[3] != NSENZMAXv6m
            || dims[4] != NQUARMAXv6m || dims[5] > NDAYMAXv6m
            || dims[6] != NSSTMAXv6m) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
//    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->bias_mean);
//    if (status != 0) {
//        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
//                __LINE__, sdsname, file);
//        exit(1);
//    }
//    status = SDendaccess(sds_id);
//
//    strcpy(sdsname, "counts");
//    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
//    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
//    if (rank != NSSESDIMv6m) {
//        printf(
//                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
//                __FILE__, __LINE__, sdsname);
//        exit(1);
//    }
//    if (dims[0] != NQUALMAXv6m || dims[1] != NLATMAXv6m
//            || dims[2] != NDIFFMAXv6m4 || dims[3] != NSENZMAXv6m
//            || dims[4] != NQUARMAXv6m || dims[5] != NDAYMAXv6m4
//            || dims[6] != NSSTMAXv6m) {
//            printf(
//                    "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
//                __FILE__, __LINE__, sdsname);
//        exit(1);
//    }
//    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->counts);
//    if (status != 0) {
//        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
//                __LINE__, sdsname, file);
//        exit(1);
//    }
    status = SDendaccess(sds_id);

    /* save the table dimensions */
    sses->nqual = dims[0];
    sses->nlat = dims[1];
    sses->ndiff = dims[2];
    sses->nsenz = dims[3];
    sses->nquar = dims[4];
    sses->nday = dims[5];
    sses->nsst = dims[6];

    /* read the table indice ranges */

    strcpy(sdsname, "sst");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->sst);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nsst = dims[0];

    strcpy(sdsname, "senz");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->senz);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nsenz = dims[0];

    strcpy(sdsname, "BTdiff");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->diff);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->ndiff = dims[0];

    strcpy(sdsname, "lat");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->lat);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nlat = dims[0];

    strcpy(sdsname, "lat");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->lat);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nlat = dims[0];

    /* terminate access to the SD interface and close the file */
    status = SDend(sd_id);
}

/* ---------------------------------------------------------------------------------------- */
/* load_sses_sstv6a() - loads the specified AVHRR SSES (sensor-specific error stats) table  */

/* ---------------------------------------------------------------------------------------- */
void load_sses_sstv6a(int32_t sensorID, char *file, ssestabstrv6a *sses) {
    int32 sd_id;
    int32 sds_id;
    int32 status;
    int32 rank;
    int32 nt;
    int32 nattrs;
    int32 dims[NSSESDIMv6a];
    int32 start[NSSESDIMv6a];

    char name[H4_MAX_NC_NAME] = "";
    char sdsname[H4_MAX_NC_NAME] = "";

    memset(start, 0, NSSESDIMv6a * sizeof (int32));

    if (strcmp(file, "") == 0) {
        printf("\nNo SSES data provided for this sensor.\n");
        haveSSES = 0;
        return;
    }

    printf("\nLoading SSES table from %s\n", file);

    /* open the file and initiate the SD interface */
    sd_id = SDstart(file, DFACC_RDONLY);
    if (sd_id == -1) {
        printf("-E- %s line %d:  Error opening file %s.\n", __FILE__, __LINE__,
                file);
        exit(1);
    }

    /* read the bias and standard deviation */

    strcpy(sdsname, "bias");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6a) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation. got: %d expected %d\n",
                __FILE__, __LINE__, sdsname, rank, NSSESDIMv6a);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6a || dims[1] != NLATMAXv6a
            || dims[2] != NDIFFMAXv6a || dims[3] != NSENZMAXv6a
            || dims[4] != NQUARMAXv6a || dims[5] != NSSTMAXv6a) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->bias);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    strcpy(sdsname, "stdv");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6a) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6a || dims[1] != NLATMAXv6a
            || dims[2] != NDIFFMAXv6a || dims[3] != NSENZMAXv6a
            || dims[4] != NQUARMAXv6a || dims[5] != NSSTMAXv6a) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->stdv);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    /* new hypercubes have bias_mean and counts also */

    strcpy(sdsname, "bias_mean");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6a) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6a || dims[1] != NLATMAXv6a
            || dims[2] != NDIFFMAXv6a || dims[3] != NSENZMAXv6a
            || dims[4] != NQUARMAXv6a || dims[5] != NSSTMAXv6a) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->bias_mean);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    strcpy(sdsname, "counts");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    if (rank != NSSESDIMv6a) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    if (dims[0] != NQUALMAXv6a || dims[1] != NLATMAXv6a
            || dims[2] != NDIFFMAXv6a || dims[3] != NSENZMAXv6a
            || dims[4] != NQUARMAXv6a || dims[5] != NSSTMAXv6a) {
        printf(
                "-E- %s line %d: Table dimensions for %s do not match expectation.\n",
                __FILE__, __LINE__, sdsname);
        exit(1);
    }
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->counts);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);

    /* save the table dimensions */
    sses->nqual = dims[0];
    sses->nlat = dims[1];
    sses->ndiff = dims[2];
    sses->nsenz = dims[3];
    sses->nquar = dims[4];
    sses->nsst = dims[5];

    /* read the table indice ranges */

    strcpy(sdsname, "sst");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->sst);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nsst = dims[0];

    strcpy(sdsname, "senz");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->senz);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nsenz = dims[0];

    strcpy(sdsname, "BTdiff");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->diff);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->ndiff = dims[0];

    strcpy(sdsname, "lat");
    sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
    status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);
    status = SDreaddata(sds_id, start, NULL, dims, (VOIDP) sses->lat);
    if (status != 0) {
        printf("-E- %s line %d:  Error reading SDS %s from %s.\n", __FILE__,
                __LINE__, sdsname, file);
        exit(1);
    }
    status = SDendaccess(sds_id);
    sses->nlat = dims[0];

    /* terminate access to the SD interface and close the file */
    status = SDend(sd_id);
}

void isV5coef(l2str *l2rec) {
    FILE *fp;
    char line [200] = "";
    char mission[5];
    int datechk;
    int32_t ncid;

    /* Open the coefficient file */
    if (nc_open(input->sstcoeffile, NC_NOWRITE, &ncid) != NC_NOERR) {

        if ((fp = fopen(input->sstcoeffile, "r")) == NULL) {
            fprintf(stderr,
                    "-E- %s line %d: unable to open sst coef file %s for reading\n",
                    __FILE__, __LINE__, input->sstcoeffile);
            exit(1);
        }

        while (fgets(line, 200, fp)) {
            if (line[0] == '#') {
                continue;
            } else {
                sscanf(line, "%4s %d", mission, &datechk);
                break;
            }
        }
        fclose(fp);
        /*
         * If it's not a VIIRS file, and the datechk is true, it's a V5 table
         * VIIRS still uses dates, but we don't have v5 coef for VIIRS, so for now,
         * this simple check should work -
         */
        if ((strcmp(mission, "VIIR") != 0) && (datechk > 1000)) {
            isV5 = 1;
            if (want_verbose)
                printf("Using V5 coefficients to compute SST\n");
        }
    } else {
        nc_close(ncid);
    }
}

/* ----------------------------------------------------------------------------------- */
/* init_sst() - Initialize for SST processing                                          */
/* B. Franz, SAIC, August 2005                                                         */

/* ----------------------------------------------------------------------------------- */
void init_sst(l2str *l2rec) {
    extern l1qstr l1que;
    int32_t npix = l2rec->l1rec->npix;

    evalmask = input->evalmask;

    nbvis = l2rec->l1rec->l1file->nbands;
    nbir = NBANDSIR;
    ibred = bindex_get(678); /* 9 for modisa */
    ib07 = bindex_get(748); /* 10 for modisa */
    ib08 = bindex_get(850);
    if (ib08 < 0) {
        ib08 = bindex_get(865);
    }
    ib16 = bindex_get(1640); /* 14 for modisa */
    ib37 = bindex_get(3750) - l2rec->l1rec->l1file->nbands; /* 0 for modisa */
    ib39 = bindex_get(3959) - l2rec->l1rec->l1file->nbands; /* 1 for modisa */
    ib40 = bindex_get(4050) - l2rec->l1rec->l1file->nbands; /* 2 for modisa */
    ib67 = bindex_get(6715) - l2rec->l1rec->l1file->nbands;
    ib73 = bindex_get(7325) - l2rec->l1rec->l1file->nbands;
    ib85 = bindex_get(8550) - l2rec->l1rec->l1file->nbands; /* 5 for modisa */
    ib11 = bindex_get(11000) - l2rec->l1rec->l1file->nbands; /* 6 for modisa */
    ib12 = bindex_get(12000) - l2rec->l1rec->l1file->nbands; /* 7 for modisa */

    if (l2rec->l1rec->l1file->sensorID == AVHRR) {
        if (strncmp(l2rec->l1rec->l1file->spatialResolution, "4.6km", 5) == 0) {
            fullscanpix = 409;
        } else {
            fullscanpix = 2048;
        }
        /* avhrr wavelenths are 630, 855, 3700, 11000, 12000 */
        /* or, are they different for different satellites? */
        /* ibred should be index 0 for channel 1 data */
        ibred = bindex_get(630);
        //Hide ib39, and ib40 as 3700 is put in here from above...
        ib39 = -1;
        ib40 = -1;
        /* channel 3a, 1610, exists for daytime NO16, NO17, NO18, NO19 */
        /* all other cases it's 3700 so msl12_sensor.dat has 3700 for channel 3 */
        // ib16  = ib08 + 1;
        /* some limits are different for AVHRR */
        solznight = SOLZNIGHTA;
        hisenz = hisenza;
        vhisenz = vhisenza;
        Btmin = Btmina;
        Btmax = Btmaxa;
        SSTmax = SSTmaxa;
        SSTdiff = SSTdiffa;
        switch (l2rec->l1rec->l1file->subsensorID) {
            case NO07:
            case NO11:
            case NO14:
            case NO15:
                Bt11unif1 = 0.6;
                break;
            case NO09:
                Bt11unif1 = 0.7;
                break;
            default:
                Bt11unif1 = 1.0;
                break;
        }
    }

    if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
        fullscanpix = 3200;
        cldbox = cldboxv;
        btbox = btboxv;
        // Red and NIR bands in VIIRS is far enough from MODIS to require a separate call
        ibred = bindex_get(672); /* SVM05 */
        ib16 = bindex_get(1601); /* SVM10 */

        //Hide ib39, as 3700 is put in here from above...
        ib39 = -1;

        cldthresh = cldthreshv;
        /* viirs Bt's are in K, not Deg C (not so using OBPG reader - we read the radiances and create the BTs)*/
        //                Btmin = Btminv;
        //                Btmax = Btmaxv;
        //                Btmax40 = Btmax40v;
    }

    if (l2rec->l1rec->l1file->sensorID == MODIST || l2rec->l1rec->l1file->sensorID == MODISA) {
        if (input->resolution == 250) {
            fullscanpix = 5416;
        } else if (input->resolution == 500) {
            fullscanpix = 2708;
        }
    }

    if (ib11 < 0 || ib12 < 0)
        haveSST = 0;
    else
        haveSST = 1;

    if (ib39 < 0 || ib40 < 0)
        haveSST4 = 0;
    else
        haveSST4 = 1;

    if (!haveSST && !haveSST4) {
        fprintf(stderr, "-E- %s line %d: no SST bands found.\n",
                __FILE__, __LINE__);
        exit(1);
    }

    /* this is the value that a saturated radiance would be assigned */
    if (ibred < 0)
        haveRed = 0;
    else {
        haveRed = 1;
        // satred = 1000.0/input->gain[ibred]-1.0;
        satred = 1000.0 - 1.0;
    }
    /* make sure l1que.nq is not smaller than btbox, can only calc sst's if have Bt's */
    if (l1que.nq < btbox) {
        fprintf(stderr, "-E- %s line %d: filter queue (l1que) is too small.  Have %d need %d.\n",
                __FILE__, __LINE__, l1que.nq, btbox);
        exit(1);
    }

    /* allocate private arrays for a single scan line */
    if (haveSST) {
        sstq = (s_array*) calloc(l1que.nq, sizeof (s_array));
        flags_sst = (int16*) calloc(npix, sizeof (int16));
        qual_sst = (int8*) calloc(npix, sizeof (int8));
        bias_sst = (float*) calloc(npix, sizeof (float));
        stdv_sst = (float*) calloc(npix, sizeof (float));
        bias_mean_sst = (float*) calloc(npix, sizeof (float));
        bias_counts_sst = (int16*) calloc(npix, sizeof (int16));
        dsdi_correction = (float*) calloc(npix, sizeof (float));

        treesum = (float*) calloc(npix, sizeof (float));

        if (l2rec->l1rec->l1file->sensorID == AVHRR) {
            load_sses_sstv6a(l2rec->l1rec->l1file->sensorID, input->sstssesfile,
                    &sses_sstv6a);
        } else {
            if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
                load_sses_sstv6v(l2rec->l1rec->l1file->sensorID, input->sstssesfile,
                        &sses_sstv6v);
            } else {
                /* modis */
                load_sses_sstv6m(l2rec->l1rec->l1file->sensorID, input->sstssesfile,
                        &sses_sstv6m);
            }
        }

        // VIIRSN can do a triple window sst algorithm...
        if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
            sst3q = (s_array*) calloc(l1que.nq, sizeof (s_array));
            flags_sst3 = (int16*) calloc(npix, sizeof (int16));
            qual_sst3 = (int8*) calloc(npix, sizeof (int8));
            bias_sst3 = (float*) calloc(npix, sizeof (float));
            stdv_sst3 = (float*) calloc(npix, sizeof (float));
            bias_mean_sst3 = (float*) calloc(npix, sizeof (float));
            bias_counts_sst3 = (int16*) calloc(npix, sizeof (int16));
            load_sses_sstv6v3(l2rec->l1rec->l1file->sensorID, input->sst3ssesfile, &sses_sst3v6v3);

        }

    }

    if (haveSST4) {
        sst4q = (s_array*) calloc(l1que.nq, sizeof (s_array));
        flags_sst4 = (int16*) calloc(npix, sizeof (int16));
        qual_sst4 = (int8*) calloc(npix, sizeof (int8));
        bias_sst4 = (float*) calloc(npix, sizeof (float));
        stdv_sst4 = (float*) calloc(npix, sizeof (float));
        bias_mean_sst4 = (float*) calloc(npix, sizeof (float));
        bias_counts_sst4 = (int16*) calloc(npix, sizeof (int16));
        load_sses_sstv6m4(l2rec->l1rec->l1file->sensorID, input->sst4ssesfile,
                &sses_sst4v6m);

        d3940ref = (float*) calloc(npix, sizeof (float));
    }
    rhoCirrus_maxmin = (float*) calloc(npix, sizeof (float));
    rhoCirrus_min = (float*) calloc(npix, sizeof (float));
    rhoCirrus_max = (float*) calloc(npix, sizeof (float));

    if (haveRed) {
        LtRED_maxmin = (float*) calloc(npix, sizeof (float));
        rhotRED_maxmin = (float*) calloc(npix, sizeof (float));
        rhotRED_min = (float*) calloc(npix, sizeof (float));
        rhotRED_max = (float*) calloc(npix, sizeof (float));
        rhotRED = (float*) calloc(npix, sizeof (float));
    }
    if (ib11 >= 0) {
        Bt11_maxmin = (float*) calloc(npix, sizeof (float));
        Bt11_max = (float*) calloc(npix, sizeof (float));
        Bt11_min = (float*) calloc(npix, sizeof (float));
        Bt11_stdev = (float*) calloc(npix, sizeof (float));
    }
    if (ib12 >= 0) {
        Bt12_maxmin = (float*) calloc(npix, sizeof (float));
        Bt12_min = (float*) calloc(npix, sizeof (float));
    }
    if (ib37 >= 0) {
        Bt37_maxmin = (float*) calloc(npix, sizeof (float));
        Bt37_stdev = (float*) calloc(npix, sizeof (float));
    }
    if (ib73 >= 0)
        Bt73_max = (float*) calloc(npix, sizeof (float));

    if (ib85 >= 0)
        Bt85_min = (float*) calloc(npix, sizeof (float));

    if (ib07 >= 0) {
        rhotNIR7_min = (float*) calloc(npix, sizeof (float));
        rhotNIR7 = (float*) calloc(npix, sizeof (float));
    }
    if (ib16 >= 0) {
        rhot16_min = (float*) calloc(npix, sizeof (float));
        rhot16 = (float*) calloc(npix, sizeof (float));
    }
    if (ib39 >= 0)
        Bt39_maxmin = (float*) calloc(npix, sizeof (float));

    if (ib40 >= 0) {
        Bt40_maxmin = (float*) calloc(npix, sizeof (float));
        Bt40_stdev = (float*) calloc(npix, sizeof (float));
        //	if (l2rec->l1rec->l1file->sensorID == MODISA)
        //	    Bt40_avg = (float*) calloc(npix, sizeof(float));
    }

    sst_stdev = (float*) calloc(npix, sizeof (float));

    csstbox = l1que.nq / 2; /* csstbox is the center of the box of sst's */
    isV5coef(l2rec);
}

/* ----------------------------------------------------------------------------------- */
/* sst_ran() - Determine if we have already run sst for this scan line                 */

/* ----------------------------------------------------------------------------------- */
int sst_ran(int recnum) {
    if (recnum == recnumSST)
        return 1;
    else
        return 0;
}

/* ----------------------------------------------------------------------------------- */
/* read_v5_sst_coeff() - reads sst coefficients for the specified date                    */
/*                                                                                     */
/* B. Franz, SAIC, 11 August 2005                                                      */

/* ----------------------------------------------------------------------------------- */
void read_v5_sst_coeff(l2str *l2rec, float **bounds, float **coef, float *sstrefoffday, float *sstrefoffnight) {
    char mission[5] = "";
    char mission2[5] = "";

    FILE *fp;
    char line [200] = "";
    char odates [8] = ""; /* yyyyddd */
    char sdates [8] = "";
    char edates [8] = "";
    char name [80];
    char value [80];
    char *p;
    char *p1;
    char *p2;

    int32 found = 0;
    int32 indx = 0;
    int32 sensorID = l2rec->l1rec->l1file->sensorID;
    double pasutime = l2rec->l1rec->scantime;
    int16_t year, day;
    double sec;
    unix2yds(pasutime, &year, &day, &sec);

    switch (sensorID) {
        case MODISA:
            strcpy(mission, "AQUA");
            break;
        case MODIST:
            strcpy(mission, "TERR");
            break;
        case AVHRR:
            strcpy(mission, xsatid2name(l2rec->l1rec->l1file->subsensorID));
            break;
        default:
            strcpy(mission, "AQUA");
            break;
    }

    /* get v5 coeffs for AVHRR or MODIS */
    fp = fopen(input->sstcoeffile, "r");
    /* Form date string */

uselastyear:
    sprintf(odates, "%4d%03d", year, day);

    /* Loop through to find bounding times, for 2 sets of coeffs */

    indx = 0;
    while (fgets(line, 200, fp)) {
        if (line[0] == '#') {
            /* look for lines with: # variable = value */
            if (!(p = strchr(line, '=')))
                continue;
            p1 = line + 1; /* look for white space after # and before variable name */
            while (isspace(*p1))
                p1++;
            p2 = p - 1; /* look for white space before = and after variable name */
            while (isspace(*p2))
                p2--;
            /* get variable name from input line */
            strncpy(name, p1, p2 - p1 + 1);
            name[p2 - p1 + 1] = '\0';

            /*
             * Parse parameter value string
             */
            /* start at character after = and ignore white space before value */
            p1 = p + 1;
            while (isspace(*p1))
                p1++;
            p2 = p1;
            /* look for white space to determine end of value */
            while (!isspace(*p2))
                p2++;
            /* get value from input line */
            strncpy(value, p1, p2 - p1);
            value[p2 - p1] = '\0';

        } else {
            /* read sst v5 coeffs for AVHRR */
            if (strncmp(line, mission, 4) == 0) {
                sscanf(line, "%4s %7s %7s %f %f %f %f",
                        mission2, sdates, edates, &coef[indx][0], &coef[indx][1], &coef[indx][2], &coef[indx][3]);
                coef[indx][4] = 0.0;
                if (strcmp(odates, sdates) >= 0 && (strcmp(odates, edates) <= 0
                        || strcmp(edates, "0000000") == 0
                        || strcmp(edates, "") == 0)) {
                    indx++;
                    if (indx == 2) {
                        found = 1;
                        break;
                    }
                }
            }
        }
    }

    if (found == 0 && year > 2004) {
        printf("Warning: No SST coefficients available for %s, reverting to previous year.\n", odates);
        year--;
        rewind(fp);
        goto uselastyear;
    }


    fclose(fp);

    if (found == 1) {
        printf("Loading SST coefficients from %s:\n", input->sstcoeffile);
        printf("%s %s %6.3f %6.3f %6.3f %6.3f %6.3f\n", sdates, edates, coef[0][0], coef[0][1], coef[0][2], coef[0][3], coef[0][4]);
        printf("%s %s %6.3f %6.3f %6.3f %6.3f %6.3f\n\n", sdates, edates, coef[1][0], coef[1][1], coef[1][2], coef[1][3], coef[1][4]);

        printf(" sst reference day offset = %f\n", *sstrefoffday);
        printf(" sst reference night offset = %f\n", *sstrefoffnight);
    } else {
        fprintf(stderr,
                "-E- %s line %d: unable to locate valid SST coefficients for %s in %s\n",
                __FILE__, __LINE__, odates, input->sstcoeffile);
        exit(1);
    }

    return;
}

/* ----------------------------------------------------------------------------------- */
/* read_sst_coeff() - reads sst coefficients for the specified date                    */
/*                                                                                     */
/* B. Franz, SAIC, 11 August 2005                                                      */

/* ----------------------------------------------------------------------------------- */
void read_sst_coeff(l2str *l2rec, float **bounds, float **coef,
        float *sstrefoffday, float *sstrefoffnight) {
    char mission[5] = "";
    char mission2[5] = "";

    FILE *fp;
    char line[200] = "";
    char odatel[14] = ""; /* yyyydddhhmmss */
    char sdatel[14] = ""; /* yyyydddhhmmss */
    char edatel[14] = ""; /* yyyydddhhmmss */
    char odates[8] = ""; /* yyyyddd */
    char sdates[8] = "";
    char edates[8] = "";
    char stime[7] = "";
    char etime[7] = "";
    char *coeflabel[] ={"day dry ", "day moist ", "night dry ", "night moist "};
    char dorn[2] = ""; /* day or night, we assume DDNN, so don't really need it */
    char *ztime;
    char name[80];
    char value[80];
    char *p;
    char *p1;
    char *p2;
    int32 gotsstrefoffday = 0;
    int32 gotsstrefoffnight = 0;
    int32 found = 0;
    int32 indx = 0;
    //    int32 tmonth = 0;
    int32 month;
    int32 leap;
    int32 sensorID = l2rec->l1rec->l1file->sensorID;
    double pasutime = l2rec->l1rec->scantime;
    int16_t year, day;
    double sec;
    unix2yds(pasutime, &year, &day, &sec);

//    int32_t year = *l2rec->year;
//    int32_t day = *l2rec->day;
//    int32_t msec = *l2rec->msec;
    int32 tmp1;
    float tmp2, tmp3;
    //float64 pasutime;

    switch (sensorID) {
        case MODISA:
            strcpy(mission, "AQUA");
            break;
        case MODIST:
            strcpy(mission, "TERR");
            break;
        case AVHRR:
            strcpy(mission, xsatid2name(l2rec->l1rec->l1file->subsensorID));
            break;
        case VIIRSN:
            strcpy(mission, "VIIR");
            break;
        default:
            strcpy(mission, "AQUA");
            break;
    }

    /* Open the file */
    if ((fp = fopen(input->sstcoeffile, "r")) == NULL) {
        fprintf(stderr,
                "-E- %s line %d: unable to open sst coef file %s for reading\n",
                __FILE__, __LINE__, input->sstcoeffile);
        exit(1);
    }
    /* Find month */
    leap = (isleap(year) == TRUE ? 1 : 0);
    if (tmonth < 0) {
        for (tmonth = 11; tmonth >= 0; tmonth--) {
            /* day is one based, StartOfMonth is zero based */
            if (day > StartOfMonth[leap][tmonth]) {
                break;
            }
        }
    }

    if (sensorID == VIIRSN) {
        /* VIIRS latband coeffs are this format: */
        /* sat, start yyyyddd, start hhmmss, end yyyyddd, end hhmmss, min lat, max lat, c0, c1, c2, c3 */
        /* VIIR 2012001 000000 2012069 235900  -90 -40 -4.202194 1.0196764 0.002352195 1.206611 */

        /* Form date string */

        /* msec is the scan line start time */
        /* should calculate the actual pixel time (extrapolate from msec) for field 57? */
        /* modis, viirs, and avhhr: year, day are per scan line, not just time of first scan */
 //       pasutime = yds2unix(year, day, ((double) (msec)) / 1000.0);
        ztime = ydhmsf(pasutime, 'G');
        /* ztime is yyyydddhhmmssfff */
        strncpy(odatel, ztime, 13);
        odatel[13] = '\0';

        /* Loop through to find bounding times */

        indx = 0;
        while (fgets(line, 200, fp)) {
            if (line[0] == '#') {
                /* look for lines with: # variable = value */
                if (!(p = strchr(line, '=')))
                    continue;
                p1 = line + 1; /* look for white space after # and before variable name */
                while (isspace(*p1))
                    p1++;
                p2 = p - 1; /* look for white space before = and after variable name */
                while (isspace(*p2))
                    p2--;
                /* get variable name from input line */
                strncpy(name, p1, p2 - p1 + 1);
                name[p2 - p1 + 1] = '\0';

                /*
                 * Parse parameter value string
                 */
                /* start at character after = and ignore white space before value */
                p1 = p + 1;
                while (isspace(*p1))
                    p1++;
                p2 = p1;
                /* look for white space to determine end of value */
                while (!isspace(*p2))
                    p2++;
                /* get value from input line */
                strncpy(value, p1, p2 - p1);
                value[p2 - p1] = '\0';
                /*
                 * Copy value to appropriate variable
                 */
                if (strcmp(name, "sstref_day_offset") == 0) {
                    *sstrefoffday = (float) atof(value);
                    gotsstrefoffday = 1;
                }
                if (strcmp(name, "sstref_night_offset") == 0) {
                    *sstrefoffnight = (float) atof(value);
                    gotsstrefoffnight = 1;
                }

            } else {
                /* read viirs sst latband coeffs */
                if (strncmp(line, mission, 4) == 0) {
                    if (input->viirsnosisaf == 1) {
                        sscanf(line, "%4s %7s %7s %1s %f %f %f %f %f %f %f",
                                mission2, sdates, edates, dorn,
                                &coef[indx][0], &coef[indx][1], &coef[indx][2],
                                &coef[indx][3], &coef[indx][4], &coef[indx][5],
                                &coef[indx][6]);
                    } else if (input->viirsnv7 >= 0) {
                        /* all latband versions except v6.4.1 */
                        sscanf(line,
                                "%4s %7s %6s %7s %6s %f %f %f %f %f %f %f %f %f",
                                mission2, sdates, stime, edates, etime,
                                &bounds[indx][0], &bounds[indx][1],
                                &coef[indx][0], &coef[indx][1], &coef[indx][2],
                                &coef[indx][3], &coef[indx][4], &coef[indx][5],
                                &coef[indx][6]);
                        sprintf(sdatel, "%s%s", sdates, stime);
                        sprintf(edatel, "%s%s", edates, etime);
                        if (strcmp(odatel, sdatel) >= 0
                                && (strcmp(odatel, edatel) <= 0
                                || strcmp(edates, "0000000") == 0
                                || strcmp(edates, "") == 0)) {
                            indx++;
                            if (indx == 7) {
                                found = 1;
                                break;
                            }
                        }
                    } else {
                        /* viirs nlsst v6.4.1 has extra satz terms, but not mirror side */
                        /*    and is by month, not start and end dates */
                        sscanf(line, "%4s %d %f %f %f %f %f %f %f %f",
                                mission2, &month, &bounds[indx][0], &bounds[indx][1],
                                &coef[indx][0], &coef[indx][1], &coef[indx][2],
                                &coef[indx][3], &coef[indx][5], &coef[indx][6]);
                        /* no mirror side for viirs */
                        coef[indx][4] = 0.0;
                        if (month == tmonth + 1) {
                            indx++;
                            if (indx == 7) {
                                found = 1;
                                break;
                            }
                        }
                    }

                }
            }
        }

    } else {
        /* get latband coeffs for AVHRR or MODIS */

        /* Loop through to find 6 sets of coeffs for this month */
        indx = 0;
        while (fgets(line, 200, fp)) {
            if (line[0] == '#') {
                /* look for lines with: # variable = value */
                if (!(p = strchr(line, '=')))
                    continue;
                p1 = line + 1; /* look for white space after # and before variable name */
                while (isspace(*p1))
                    p1++;
                p2 = p - 1; /* look for white space before = and after variable name */
                while (isspace(*p2))
                    p2--;
                /* get variable name from input line */
                strncpy(name, p1, p2 - p1 + 1);
                name[p2 - p1 + 1] = '\0';

                /*
                 * Parse parameter value string
                 */
                /* start at character after = and ignore white space before value */
                p1 = p + 1;
                while (isspace(*p1))
                    p1++;
                p2 = p1;
                /* look for white space to determine end of value */
                while (!isspace(*p2))
                    p2++;
                /* get value from input line */
                strncpy(value, p1, p2 - p1);
                value[p2 - p1] = '\0';
                /*
                 * Copy value to appropriate variable
                 */
                if (strcmp(name, "sstref_day_offset") == 0) {
                    *sstrefoffday = (float) atof(value);
                    gotsstrefoffday = 1;
                }
                if (strcmp(name, "sstref_night_offset") == 0) {
                    *sstrefoffnight = (float) atof(value);
                    gotsstrefoffnight = 1;
                }

            } else {
                /* read sst latband coeffs for AVHRR or MODIS */
                if (strncmp(line, mission, 4) == 0) {
                    if (l2rec->l1rec->l1file->sensorID == MODIST || l2rec->l1rec->l1file->sensorID == MODISA) {
                        /* terra and aqua nlsst has extra mirror side and satz terms */
                        sscanf(line,
                                "%4s %d %f %f %f %f %f %f %f %f %f %d %f %f",
                                mission2, &month, &bounds[indx][0],
                                &bounds[indx][1], &coef[indx][0],
                                &coef[indx][1], &coef[indx][2], &coef[indx][3],
                                &coef[indx][4], &coef[indx][5], &coef[indx][6],
                                &tmp1, &tmp2, &tmp3);
                    } else {
                        /* don't have extra terms for avhrr nlsst */
                        sscanf(line, "%4s %d %f %f %f %f %f %f", mission2,
                                &month, &bounds[indx][0], &bounds[indx][1],
                                &coef[indx][0], &coef[indx][1], &coef[indx][2],
                                &coef[indx][3]);
                        coef[indx][4] = 0.0;
                        coef[indx][5] = 0.0;
                        coef[indx][6] = 0.0;
                    }
                    if (month == tmonth + 1) {
                        indx++;
                        if (indx == 7){
                            found = 1;
                            break;
                        }
                    }
                }
            }
        }
    }

    fclose(fp);

    if (found == 1) {

        if (sensorID == VIIRSN
                && (gotsstrefoffday == 0 || gotsstrefoffnight == 0)) {
            fprintf(stderr,
                    "-E- %s line %d: Day and night sst reference offsets not found in %s\n",
                    __FILE__, __LINE__, input->sstcoeffile);
            exit(1);
        }

        printf("Loading SST lat band coefficients from %s:\n",
                input->sstcoeffile);

        if (sensorID == VIIRSN && input->viirsnosisaf == 1) {
            /* viirs osi-saf coefs are not latband */
            for (indx = 0; indx < 4; indx++) {
                printf("%s %s %s %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f\n",
                        coeflabel[indx], sdates, edates, coef[indx][0], coef[indx][1], coef[indx][2],
                        coef[indx][3], coef[indx][4], coef[indx][5], coef[indx][6]);
            }
        } else {
            for (indx = 0; indx < 7; indx++) {
                if (sensorID == VIIRSN && input->viirsnv7 >= 1) {
                    /* v7 viirs have start and end dates, not months */
                    printf(
                            "%s %s %s %s %6.1f %6.1f %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f\n",
                            sdates, stime, edates, etime, bounds[indx][0], bounds[indx][1],
                            coef[indx][0], coef[indx][1], coef[indx][2], coef[indx][3],
                            coef[indx][4], coef[indx][5], coef[indx][6]);
                } else {
                    /* most latband coeffs are by month */
                    printf(
                            "%d %6.1f %6.1f %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f\n",
                            month, bounds[indx][0], bounds[indx][1], coef[indx][0],
                            coef[indx][1], coef[indx][2], coef[indx][3],
                            coef[indx][4], coef[indx][5], coef[indx][6]);
                }
            }
        }
        printf(" sst reference day offset = %f\n", *sstrefoffday);
        printf(" sst reference night offset = %f\n", *sstrefoffnight);

    } else {
        fprintf(stderr,
                "-E- %s line %d: unable to locate valid SST coefficients for %s in %s\n",
                __FILE__, __LINE__, odates, input->sstcoeffile);
        exit(1);
    }

    return;
}

/* ----------------------------------------------------------------------------------- */
/* read_sst4_coeff() - reads 4um sst coefficients for the specified date               */
/*                                                                                     */
/* B. Franz, SAIC, 11 August 2005                                                      */

/* ----------------------------------------------------------------------------------- */
void read_sst4_coeff(int32 sensorID, char *filename, double scantime,
        float **bounds, float **coef) {
    char mission[5];
    char mission2[5];

    FILE *fp;
    char *line;
    line = (char *) calloc(200, sizeof (char));
    char odate[8];
    int found = 0;
    //    int tmonth;
    int16_t year, day;
    double sec;
    unix2yds(scantime, &year, &day, &sec);

    int month;
    int indx;
    //    char tmp1[200] = "\0"; /* random length to slurp up whatevers after the coefs */
    int32 leap;

    switch (sensorID) {
        case MODISA:
            strcpy(mission, "AQUA");
            break;
        case MODIST:
            strcpy(mission, "TERR");
            break;
        case VIIRSN:
            strcpy(mission, "VIIR");
            break;
        default:
            strcpy(mission, "AQUA");
            break;
    }

    /* Open the file */
    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "-E- %s line %d: unable to open %s for reading\n",
                __FILE__, __LINE__, filename);
        exit(1);
    }

    sprintf(odate, "%4d%03d", year, day);

    /* Find month */
    leap = (isleap(year) == TRUE ? 1 : 0);
    if (tmonth < 0) {
        for (tmonth = 11; tmonth >= 0; tmonth--) {
            if (day > StartOfMonth[leap][tmonth]) {
                break;
            }
        }
    }
    /* Loop through to find 6 sets of coeffs for this month */
    indx = 0;
    fprintf(stderr, "  looking for month %d mission %s\n", tmonth + 1, mission);
    while (fgets(line, 200, fp)) {
        /* read sst4 latband coeffs */
        if (strncmp(line, mission, 4) == 0) {
            sscanf(line, "%4s %d %f %f %f %f %f %f %f %f %f", &mission2[0],
                    &month, &bounds[indx][0], &bounds[indx][1], &coef[indx][0],
                    &coef[indx][1], &coef[indx][2], &coef[indx][3],
                    &coef[indx][4], &coef[indx][5], &coef[indx][6]);
            if (month == tmonth + 1) {
                indx++;
                if (indx == 7) {
                    found = 1;
                    break;
                }
            }
        }
    }

    fclose(fp);

    if (found == 1) {

        printf("Loading SST4 lat band coefficients from %s:\n", filename);
        for (indx = 0; indx < 7; indx++) {
            printf("%d %6.1f %6.1f %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f\n",
                    month, bounds[indx][0], bounds[indx][1], coef[indx][0], coef[indx][1], coef[indx][2],
                    coef[indx][3], coef[indx][4], coef[indx][5], coef[indx][6]);
        }

    } else {
        fprintf(stderr,
                "-E- %s line %d: unable to locate valid SST4 coefficients for %s in %s\n",
                __FILE__, __LINE__, odate, filename);
        exit(1);
    }
    free(line);

    return;
}

/* ----------------------------------------------------------------------------------- */
/* sstcloud() - uses red band to test homogeneity in nx x ny box (RSMAS)               */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
int sstcloud(int32_t ip, int32_t nx, int32_t ny, float thresh) {
    extern l1qstr l1que;

    int32_t nscan = l1que.nq;
    int32_t npix = l1que.r[0].npix;
    int32_t is = nscan / 2;
    int32_t ip1, ip2;
    int32_t is1, is2;
    int32_t i, j;
    int32_t ipb;
    float rhot;
    int flag = 0;
    int cnt = 0;
    float maxv = BAD_FLT;
    float minv = -1.0 * BAD_FLT; /* initial minimum has to be large positive number */

    if (!haveRed)
        return (flag);

    /* make sure code is not inconsistent NQMIN in l12_parms.h */
    if (nscan < ny) {
        printf(
                "-E- %s line %d: L1 queue size of %d is too small for requested homogeneity test of %d x %d.\n",
                __FILE__, __LINE__, l1que.nq, nx, ny);
        exit(1);
    }

    /* algorithm is only valid for daytime and in non glint area */
    if (l1que.r[is].solz[ip] >= solznight
            || l1que.r[is].glint_coef[ip] > glintmax)
        return (flag);

    /* compute pix/scan limits for the Row Of Interest (ROI) */
    is1 = MIN(MAX(0, is - ny / 2), nscan - 1);
    is2 = MAX(MIN(nscan - 1, is + ny / 2), 0);
    //    if ((l1que.r[is].l1file->sensorID == VIIRS) && (l1que.r[is].scn_fmt == 0)) {
    //        ip_scan = ip + l1que.r[is].spix; /* scan pixel */
    //        filt_dist = -nx / 2;
    //        viirs_pxcvt_agdel(ip_scan, filt_dist, &ipmod);
    //        ipmod -= l1que.r[is].spix;
    //        ip1 = MIN( MAX( 0, ipmod ), npix-1 );
    //
    //        filt_dist = nx / 2;
    //        viirs_pxcvt_agdel(ip_scan, filt_dist, &ipmod);
    //        ipmod -= l1que.r[is].spix;
    //        ip2 = MAX( MIN( npix-1, ipmod ), 0 );
    //    } else {
    ip1 = MIN(MAX(0, ip - nx / 2), npix - 1);
    ip2 = MAX(MIN(npix - 1, ip + nx / 2), 0);
    //    }

    /* compute max and min normalized reflectance in window */
    /* l1_hmodis_hdf.c converts visible bands, i.e. red, to radiance */
    /* so we need to convert it to reflectance */
    /* atmospheric transmittance and path radiance due to scattering and attenuation by
     * atmospheric gases and aerosols is a problem for ocean color, but not for sst
     * so, while we probably don't need the extra terms here, we definitely don't need
     * them for the cloud decision trees */
    for (j = is1; j <= is2; j++)
        for (i = ip1; i <= ip2; i++) {
            ipb = i * nbvis + ibred;
            if (l1que.r[j].Lt[ipb] > 0.0 && l1que.r[j].Lt[ipb] < satred) {
                rhot = PI * l1que.r[j].Lt[ipb] / l1que.r[j].Fo[ibred]
                        / l1que.r[j].tg_sol[ipb] / l1que.r[j].tg_sen[ipb]
                        / l1que.r[j].t_sol[ipb] / l1que.r[j].t_sen[ipb]
                        / cos(l1que.r[j].solz[ip] / RADEG);
                maxv = MAX(maxv, rhot);
                minv = MIN(minv, rhot);
                cnt++;
            }
        }

    /* added for collect 5 (if all saturated but 1, then cloud) */
    if (cnt < 2 || (maxv - minv) > thresh) {
        flag = 1;
    }

    return (flag);
}

/* ----------------------------------------------------------------------------------- */
/* rhoCboxstats() - test homogeneity in nx x ny box of rho_cirrus (RSMAS)               */

/* ----------------------------------------------------------------------------------- */
int32_t rhoCboxstats(int32_t ip, int32_t nx, int32_t ny, statstr *statrec) {
    extern l1qstr l1que;

    static int32_t len = 0;
    int32_t nscan = l1que.nq;
    int32_t npix = l1que.r[0].npix;
    int32_t ip1, ip2;
    int32_t is1, is2;
    int32_t is, i, j, ix;
    float Bt;
    static float *x = NULL;


    /* make sure code is not inconsistent NQMIN in l12_parms.h */
    if (nscan < ny) {
        printf("-E- %s line %d: L1 queue size of %d is too small for  %d x %d box stats.\n",
                __FILE__, __LINE__, l1que.nq, nx, ny);
        exit(1);
    }

    /* allocate sufficient workspace for the filter size */
    if (x == NULL || len < nx * ny) {
        len = nx*ny;
        if (x != NULL) free(x);
        if ((x = (float *) malloc(len * sizeof (float))) == NULL) {
            printf("-E- %s line %d: Unable to allocate workspace for median and stdev\n",
                    __FILE__, __LINE__);
            exit(1);
        }
    }

    /* Compute queue scan limits for the Row Of Interest (ROI) */
    is = nscan / 2;
    is1 = MIN(MAX(0, is - ny / 2), nscan - 1);
    is2 = MAX(MIN(nscan - 1, is + ny / 2), 0);

    /* compute pixel neighbor limits for the Row Of Interest (ROI) */
    /* for aggregated VIIRS, limits are aggregation zone dependent */
    //    if( ( l1que.r[is].l1file->sensorID == VIIRS ) && ( l1que.r[is].scn_fmt == 0 ) ) {
    //    ip_scan = ip + l1que.r[is].spix;  /* scan pixel */
    //    filt_dist = -nx / 2;
    //    viirs_pxcvt_agdel( ip_scan, filt_dist, &ipmod );
    //    ipmod -= l1que.r[is].spix;
    //    ip1 = MIN( MAX( 0, ipmod ), npix-1 );
    //
    //    filt_dist = nx / 2;
    //    viirs_pxcvt_agdel( ip_scan, filt_dist, &ipmod );
    //    ipmod -= l1que.r[is].spix;
    //    ip2 = MAX( MIN( npix-1, ipmod ), 0 );
    //    } else {
    ip1 = MIN(MAX(0, ip - nx / 2), npix - 1);
    ip2 = MAX(MIN(npix - 1, ip + nx / 2), 0);
    //    }

    /* initialize output rec - only min,max,avg,cnt are actually used here*/
    statrec->min = -1.0 * BAD_FLT;
    statrec->max = BAD_FLT;
    statrec->avg = BAD_FLT;
    statrec->med = BAD_FLT;
    statrec->cen = BAD_FLT;
    statrec->sqr = BAD_FLT;
    statrec->sd = BAD_FLT;
    statrec->cnt = 0;
    for (ix = 0; ix < len; ix++) {
        x[ix] = 0.0;
    }

    /* compute stats for window */
    for (j = is1; j <= is2; j++) for (i = ip1; i <= ip2; i++) {
            Bt = l1que.r[j].rho_cirrus[i];
            if (Bt > (BAD_FLT + 0.1)) {
                statrec->max = MAX(statrec->max, Bt);
                statrec->min = MIN(statrec->min, Bt);
                if (statrec->cnt == 0) {
                    /* first value - start the summing for the average at zero */
                    statrec->avg = 0.0;
                }
                statrec->avg += Bt;
                statrec->sqr += Bt * Bt;
                x[statrec->cnt] = Bt;
                statrec->cnt++;
            }
        }
    if (statrec->cnt > 2) {
        qsort(x, statrec->cnt, sizeof (float), (int (*)(const void *, const void *)) compfloat);
        statrec->max = x[statrec->cnt - 1];
        statrec->min = x[0];
        if (statrec->cnt > 1) {
            statrec->sd = (statrec->sqr - statrec->avg * statrec->avg / statrec->cnt) / (statrec->cnt - 1);
            if (statrec->sd > 0.0) {
                statrec->sd = sqrt(statrec->sd);
            } else {
                statrec->sd = BAD_FLT;
            }
        }

        statrec->med = x[statrec->cnt / 2];
    }
    if (statrec->cnt > 0)
        statrec->avg /= statrec->cnt;

    return (statrec->cnt);
}

/* ----------------------------------------------------------------------------------- */
/* rhotboxstats() - test homogeneity in nx x ny box of rhot's (RSMAS)                  */
/*                                                                                     */

/* ----------------------------------------------------------------------------------- */
int32_t rhotboxstats(int32_t ip, int ib, int nbands, int32_t nx, int32_t ny,
        statstr *statrec) {
    extern l1qstr l1que;

    static int32_t len = 0;
    int32_t nscan = l1que.nq;
    int32_t npix = l1que.r[0].npix;
    int32_t ip1, ip2;
    int32_t is1, is2;
    int32_t is, i, j, ix;
    int32_t ipb;
    //    int32_t ip_scan, filt_dist, ipmod;
    float rhot;
    static float *x = NULL;

    /* make sure code is not inconsistent NQMIN in l12_parms.h */
    if (nscan < ny) {
        printf(
                "-E- %s line %d: L1 queue size of %d is too small for  %d x %d box stats.\n",
                __FILE__, __LINE__, l1que.nq, nx, ny);
        exit(1);
    }

    /* allocate sufficient workspace for the filter size */
    if (x == NULL || len < nx * ny) {
        len = nx*ny;
        if (x != NULL) free(x);
        if ((x = (float *) malloc(len * sizeof (float))) == NULL) {
            printf("-E- %s line %d: Unable to allocate workspace for median and stdev\n",
                    __FILE__, __LINE__);
            exit(1);
        }
    }

    /* Compute queue scan limits for the Row Of Interest (ROI) */
    is = nscan / 2;
    is1 = MIN(MAX(0, is - ny / 2), nscan - 1);
    is2 = MAX(MIN(nscan - 1, is + ny / 2), 0);

    /* compute pixel neighbor limits for the Row Of Interest (ROI) */
    /* for aggregated VIIRS, limits are aggregation zone dependent */
    //    if ((l1que.r[is].l1file->sensorID == VIIRS) && (l1que.r[is].scn_fmt == 0)) {
    //        ip_scan = ip + l1que.r[is].spix; /* scan pixel */
    //        filt_dist = -nx / 2;
    //        viirs_pxcvt_agdel(ip_scan, filt_dist, &ipmod);
    //        ipmod -= l1que.r[is].spix;
    //        ip1 = MIN( MAX( 0, ipmod ), npix-1 );
    //
    //        filt_dist = nx / 2;
    //        viirs_pxcvt_agdel(ip_scan, filt_dist, &ipmod);
    //        ipmod -= l1que.r[is].spix;
    //        ip2 = MAX( MIN( npix-1, ipmod ), 0 );
    //    } else {
    ip1 = MIN(MAX(0, ip - nx / 2), npix - 1);
    ip2 = MAX(MIN(npix - 1, ip + nx / 2), 0);
    //    }

    /* initialize output rec - only min,max,avg,cnt are actually used here*/
    statrec->min = -1.0 * BAD_FLT;
    statrec->max = BAD_FLT;
    statrec->avg = BAD_FLT;
    statrec->med = BAD_FLT;
    statrec->cen = BAD_FLT;
    statrec->sqr = BAD_FLT;
    statrec->sd = BAD_FLT;
    statrec->cnt = 0;
    for (ix = 0; ix < len; ix++) {
        x[ix] = 0.0;
    }

    /* compute stats for window */
    for (j = is1; j <= is2; j++)
        for (i = ip1; i <= ip2; i++) {
            ipb = i * nbands + ib;
            /* saturated Lt is 1000.0 or sstbad=-32767 (class bad could be 0.0, but don't care?) */
            if (l1que.r[j].Lt[ipb] > sstbad + 1.0 && l1que.r[j].Lt[ipb] < 1000.0 - 1.0) {
                rhot = M_PI * l1que.r[j].Lt[ipb] / l1que.r[j].Fo[ib] / l1que.r[j].csolz[i];
                statrec->max = MAX(statrec->max, rhot);
                statrec->min = MIN(statrec->min, rhot);
                if (statrec->cnt == 0) {
                    /* first value - start the summing for the average at zero */
                    statrec->avg = 0.0;
                }
                statrec->avg += rhot;
                statrec->sqr += rhot * rhot;
                x[statrec->cnt] = rhot;
                statrec->cnt++;
            }
        }
    if (statrec->cnt > 2) {
        qsort(x, statrec->cnt, sizeof (float), (int (*)(const void *, const void *)) compfloat);
        statrec->max = x[statrec->cnt - 1];
        statrec->min = x[0];
        if (statrec->cnt > 1) {
            statrec->sd = (statrec->sqr - statrec->avg * statrec->avg / statrec->cnt) / (statrec->cnt - 1);
            if (statrec->sd > 0.0) {
                statrec->sd = sqrt(statrec->sd);
            } else {
                statrec->sd = BAD_FLT;
            }
        }

        statrec->med = x[statrec->cnt / 2];
    }
    if (statrec->cnt > 0)
        statrec->avg /= statrec->cnt;

    return (statrec->cnt);
}

/* ----------------------------------------------------------------------------------- */
/* btboxstats() - test homogeneity in nx x ny box of Bt's (RSMAS)                      */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
int32_t btboxstats(int32_t ip, int ib, int nbands, int32_t nx, int32_t ny,
        statstr *statrec) {
    extern l1qstr l1que;

    static int32_t len = 0;
    int32_t nscan = l1que.nq;
    int32_t npix = l1que.r[0].npix;
    int32_t ip1, ip2;
    int32_t is1, is2;
    int32_t is, i, j, ix;
    int32_t ipb;
    float Bt;
    static float *x = NULL;

    /* make sure code is not inconsistent NQMIN in l12_parms.h */
    if (nscan < ny) {
        printf(
                "-E- %s line %d: L1 queue size of %d is too small for  %d x %d box stats.\n",
                __FILE__, __LINE__, l1que.nq, nx, ny);
        exit(1);
    }

    /* allocate sufficient workspace for the filter size */
    if (x == NULL || len < nx * ny) {
        len = nx*ny;
        if (x != NULL) free(x);
        if ((x = (float *) malloc(len * sizeof (float))) == NULL) {
            printf("-E- %s line %d: Unable to allocate workspace for median and stdev\n",
                    __FILE__, __LINE__);
            exit(1);
        }
    }

    /* Compute queue scan limits for the Row Of Interest (ROI) */
    is = nscan / 2;
    is1 = MIN(MAX(0, is - ny / 2), nscan - 1);
    is2 = MAX(MIN(nscan - 1, is + ny / 2), 0);

    /* compute pixel neighbor limits for the Row Of Interest (ROI) */
    /* for aggregated VIIRS, limits are aggregation zone dependent */
    //    if ((l1que.r[is].l1file->sensorID == VIIRS) && (l1que.r[is].scn_fmt == 0)) {
    //        ip_scan = ip + l1que.r[is].spix; /* scan pixel */
    //        filt_dist = -nx / 2;
    //        viirs_pxcvt_agdel(ip_scan, filt_dist, &ipmod);
    //        ipmod -= l1que.r[is].spix;
    //        ip1 = MIN( MAX( 0, ipmod ), npix-1 );
    //
    //        filt_dist = nx / 2;
    //        viirs_pxcvt_agdel(ip_scan, filt_dist, &ipmod);
    //        ipmod -= l1que.r[is].spix;
    //        ip2 = MAX( MIN( npix-1, ipmod ), 0 );
    //    } else {
    ip1 = MIN(MAX(0, ip - nx / 2), npix - 1);
    ip2 = MAX(MIN(npix - 1, ip + nx / 2), 0);
    //    }

    /* initialize output rec - only min,max,avg,cnt are actually used here*/
    statrec->min = -1.0 * BAD_FLT;
    statrec->max = BAD_FLT;
    statrec->avg = BAD_FLT;
    statrec->med = BAD_FLT;
    statrec->cen = BAD_FLT;
    statrec->sqr = BAD_FLT;
    statrec->sd = BAD_FLT;
    statrec->cnt = 0;
    for (ix = 0; ix < len; ix++) {
        x[ix] = 0.0;
    }

    /* compute stats for window */
    for (j = is1; j <= is2; j++) {
        /* don't use values from  aqua detector zero Bt40 */
        /* Peter and Kay say to not use average, just skip detector zero */
        if ((l1que.r[j].l1file->sensorID != MODISA)
                || (ib != ib40)
                || (l1que.r[j].detnum != 0)) {

            for (i = ip1; i <= ip2; i++) {
                ipb = i * nbands + ib;
                Bt = l1que.r[j].Bt[ipb];
                if (Bt > BT_LO + 0.1 && Bt < BT_HI - 0.1) {
                    statrec->max = MAX(statrec->max, Bt);
                    statrec->min = MIN(statrec->min, Bt);
                    if (statrec->cnt == 0) {
                        /* first value - start the summing for the average at zero */
                        statrec->avg = 0.0;
                        statrec->sqr = 0.0;
                    }
                    statrec->avg += Bt;
                    statrec->sqr += Bt * Bt;
                    x[statrec->cnt] = Bt;
                    statrec->cnt++;
                }
            }
        }
    }
    if (statrec->cnt > 2) {
        qsort(x, statrec->cnt, sizeof (float), (int (*)(const void *, const void *)) compfloat);
        statrec->max = x[statrec->cnt - 1];
        statrec->min = x[0];
        if (statrec->cnt > 1) {
            statrec->sd = (statrec->sqr - statrec->avg * statrec->avg / statrec->cnt) / (statrec->cnt - 1);
            if (statrec->sd > 0.0) {
                statrec->sd = sqrt(statrec->sd);
            } else {
                statrec->sd = BAD_FLT;
            }
        }

        statrec->med = x[statrec->cnt / 2];
    }
    if (statrec->cnt > 0) {
        statrec->avg /= statrec->cnt;
    }

    return (statrec->cnt);
}

/* ----------------------------------------------------------------------------------- */
/* sstboxstats() - test homogeneity in nx x ny box of Bt's (RSMAS)                      */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
int32_t sstboxstats(int32_t ip, int32_t nx, int32_t ny, statstr *statrec) {
    extern l1qstr l1que;

    static int32_t len = 0;
    int32_t nscan = l1que.nq;
    int32_t npix = l1que.r[0].npix;
    int32_t ip1, ip2;
    int32_t is1, is2;
    int32_t is, i, j, ix;
    //    int32_t ip_scan, filt_dist, ipmod;
    float sst;
    static float *x = NULL;

    /* make sure code is not inconsistent */
    if (nscan < ny) {
        printf(
                "-E- %s line %d: sst queue size of %d is too small for  %d x %d box stats.\n",
                __FILE__, __LINE__, nscan, nx, ny);
        exit(1);
    }

    /* allocate sufficient workspace for the filter size */
    if (x == NULL || len < nx * ny) {
        len = nx*ny;
        if (x != NULL) free(x);
        if ((x = (float *) malloc(len * sizeof (float))) == NULL) {
            printf("-E- %s line %d: Unable to allocate workspace for median and stdev\n",
                    __FILE__, __LINE__);
            exit(1);
        }
    }

    /* Compute queue scan limits for the Row Of Interest (ROI) */
    is = nscan / 2;
    is1 = MIN(MAX(0, is - ny / 2), nscan - 1);
    is2 = MAX(MIN(nscan - 1, is + ny / 2), 0);

    /* compute pixel neighbor limits for the Row Of Interest (ROI) */
    /* for aggregated VIIRS, limits are aggregation zone dependent */
    //    if ((l1que.r[is].l1file->sensorID == VIIRS) && (l1que.r[is].scn_fmt == 0)) {
    //        ip_scan = ip + l1que.r[is].spix; /* scan pixel */
    //        filt_dist = -nx / 2;
    //        viirs_pxcvt_agdel(ip_scan, filt_dist, &ipmod);
    //        ipmod -= l1que.r[is].spix;
    //        ip1 = MIN( MAX( 0, ipmod ), npix-1 );
    //
    //        filt_dist = nx / 2;
    //        viirs_pxcvt_agdel(ip_scan, filt_dist, &ipmod);
    //        ipmod -= l1que.r[is].spix;
    //        ip2 = MAX( MIN( npix-1, ipmod ), 0 );
    //    } else {
    ip1 = MIN(MAX(0, ip - nx / 2), npix - 1);
    ip2 = MAX(MIN(npix - 1, ip + nx / 2), 0);
    //    }

    /* initialize output rec - only min,max,avg,cnt are actually used here*/
    statrec->min = -1.0 * BAD_FLT;
    statrec->max = BAD_FLT;
    statrec->avg = BAD_FLT;
    statrec->med = BAD_FLT;
    statrec->cen = BAD_FLT;
    statrec->sqr = BAD_FLT;
    statrec->sd = BAD_FLT;
    statrec->cnt = 0;
    for (ix = 0; ix < len; ix++) {
        x[ix] = 0.0;
    }

    /* compute stats for window */
    for (j = is1; j <= is2; j++) {
        for (i = ip1; i <= ip2; i++) {
            sst = sstq[j][i];
            if (sst >= SSTmin && sst <= SSTmax) {
                statrec->max = MAX(statrec->max, sst);
                statrec->min = MIN(statrec->min, sst);
                if (statrec->cnt == 0) {
                    /* first value - start the summing for the average at zero */
                    statrec->avg = 0.0;
                    statrec->sqr = 0.0;
                }
                statrec->avg += sst;
                statrec->sqr += sst * sst;
                x[statrec->cnt] = sst;
                statrec->cnt++;
            }
        }
    }
    if (statrec->cnt > 2) {
        qsort(x, statrec->cnt, sizeof (float), (int (*)(const void *, const void *)) compfloat);
        statrec->max = x[statrec->cnt - 1];
        statrec->min = x[0];
        if (statrec->cnt > 1) {
            statrec->sd = (statrec->sqr - statrec->avg * statrec->avg / statrec->cnt) / (statrec->cnt - 1);
            if (statrec->sd > 0.0) {
                statrec->sd = sqrt(statrec->sd);
            } else {
                statrec->sd = BAD_FLT;
            }
        }

        statrec->med = x[statrec->cnt / 2];
    }
    if (statrec->cnt > 0) {
        statrec->avg /= statrec->cnt;
    }

    return (statrec->cnt);
}
/* ----------------------------------------------------------------------------------- */
/* run_sstboxstdev() - calculate the sst standard deviation                               */

/* ----------------------------------------------------------------------------------- */
void run_sstboxstdev(int npix, float *sst_stdev) {

    int ip;
    statstr statrec;
    for (ip = 0; ip < npix; ip++) {
        sst_stdev[ip] = sstbad;
        if (sstboxstats(ip, btbox, btbox, &statrec) > 0) {
            sst_stdev[ip] = statrec.sd;
        }
    }
}

/* ----------------------------------------------------------------------------------- */
/* run_rhotboxmin() - initialize the static min arrays                             */

/* ----------------------------------------------------------------------------------- */
void run_rhotboxmin(int npix, int btidx, int nbands, float *minarr) {

    int ip;
    statstr statrec;
    for (ip = 0; ip < npix; ip++) {
        minarr[ip] = sstbad;
        if (rhotboxstats(ip, btidx, nbands, btbox, btbox, &statrec) > 0) {
            minarr[ip] = statrec.min;
        }
    }
}

/* ----------------------------------------------------------------------------------- */
/* run_rhotboxmaxmin() - initialize the static maxmin arrays                             */

/* ----------------------------------------------------------------------------------- */
void run_rhotboxmaxmin(int npix, int btidx, int nbands, float *maxminarr, float *maxarr) {

    int ip;
    statstr statrec;
    for (ip = 0; ip < npix; ip++) {
        maxminarr[ip] = sstbad;
        maxarr[ip] = sstbad;
        if (rhotboxstats(ip, btidx, nbands, btbox, btbox, &statrec) > 0) {
            maxminarr[ip] = statrec.max - statrec.min;
            maxarr[ip] = statrec.max;
        }
    }
}

/* ----------------------------------------------------------------------------------- */
/* run_btboxavg() - initialize the static avg arrays                                   */

/* ----------------------------------------------------------------------------------- */
void run_btboxavg(int npix, int btidx, float *avgarr) {

    int ip;
    statstr statrec;
    for (ip = 0; ip < npix; ip++) {
        avgarr[ip] = sstbad;
        if (btboxstats(ip, btidx, NBANDSIR, btbox, btbox, &statrec) > 0) {
            avgarr[ip] = statrec.avg;
        }
    }
}

/* ----------------------------------------------------------------------------------- */
/* run_btboxmaxmin() - initialize the static maxmin arrays                             */

/* ----------------------------------------------------------------------------------- */
void run_btboxmaxmin(int npix, int btidx, float *maxminarr) {

    int ip;
    statstr statrec;
    for (ip = 0; ip < npix; ip++) {
        maxminarr[ip] = sstbad;
        if (btboxstats(ip, btidx, NBANDSIR, btbox, btbox, &statrec) > 0) {
            maxminarr[ip] = statrec.max - statrec.min;
        }
    }
}
/* ----------------------------------------------------------------------------------- */
/* run_btboxmin() - initialize the static min arrays                             */

/* ----------------------------------------------------------------------------------- */
void run_btboxmin(int npix, int btidx, float *minarr) {

    int ip;
    statstr statrec;
    for (ip = 0; ip < npix; ip++) {
        minarr[ip] = sstbad;
        if (btboxstats(ip, btidx, NBANDSIR, btbox, btbox, &statrec) > 0) {
            minarr[ip] = statrec.min;
        }
    }
}
/* ----------------------------------------------------------------------------------- */
/* run_btboxmax() - initialize the static max arrays                             */

/* ----------------------------------------------------------------------------------- */
void run_btboxmax(int npix, int btidx, float *maxarr) {

    int ip;
    statstr statrec;
    for (ip = 0; ip < npix; ip++) {
        maxarr[ip] = sstbad;
        if (btboxstats(ip, btidx, NBANDSIR, btbox, btbox, &statrec) > 0) {
            maxarr[ip] = statrec.max;
        }
    }
}

void run_rhoCboxmaxmin(int npix, float *maxminarr, float *minarr, float *maxarr) {

    int ip;
    statstr statrec;
    for (ip = 0; ip < npix; ip++) {
        maxminarr[ip] = sstbad;
        minarr[ip] = sstbad;
        maxarr[ip] = sstbad;
        if (rhoCboxstats(ip, btbox, btbox, &statrec) > 0) {
            maxminarr[ip] = statrec.max - statrec.min;
            minarr[ip] = statrec.min;
            maxarr[ip] = statrec.max;
        }
    }
}

void run_btboxstdev(int npix, int btidx, float *stdevarr) {
    int ip;
    statstr statrec;
    for (ip = 0; ip < npix; ip++) {
        stdevarr[ip] = sstbad;
        if (btboxstats(ip, btidx, NBANDSIR, btbox, btbox, &statrec) > 0) {
            stdevarr[ip] = statrec.sd;
        }
    }
}
/* ----------------------------------------------------------------------------------- */
/* btavg() - Average detector 9 with detector 1 to replace aqua channel 23 detector 0  */
/*                                                                                     */
/* S. Walsh, RSMAS, August 2013.                                                       */

/* ----------------------------------------------------------------------------------- */
int32_t btavg(int32_t ip, int is, int ib, int nbands, statstr *statrec) {
    extern l1qstr l1que;

    int32_t nscan = l1que.nq;
    int32_t is1, is2;
    int32_t j;
    int32_t ipb;
    float Bt;

    /* this routine should only be used to fake Bt40's for aqua detector zero */
    if ((l1que.r[is].l1file->sensorID != MODISA)
            || (ib != ib40)
            || (l1que.r[is].detnum != 0)) {
        printf(
                "-E- %s line %d: btavg should only be used for AQUA detector zero Bt40.\n",
                __FILE__, __LINE__);
        exit(1);
    }

    /* make sure code is not inconsistent NQMIN in l12_parms.h */
    /* Right now modisa uses 3x3 boxes for aqua.
     * When we switch to 5x5 then modisa/msl12_filter.dat will have to change to keep a 7x7 box
     * so that we have the line before and after the 5x5 box to calculate the Bt40 average for
     * detector 0 in whichever line of the box is being calculated
     */
    if ((nscan < (btbox + 2)) && (is == 0 || is == btbox)) {
        printf(
                "-E- %s line %d: L1 queue size of %d is too small for 3 line Bt40 average around line %d.\n",
                __FILE__, __LINE__, l1que.nq, is);
        exit(1);
    }

    /* Compute queue scan limits for the Row Of Interest (ROI) in 'is' */
    is1 = MIN(MAX(0, is - 1), nscan - 1);
    is2 = MAX(MIN(nscan - 1, is + 1), 0);

    /* initialize output rec - only min,max,avg,cnt are actually used here*/
    statrec->min = -1.0 * BAD_FLT;
    statrec->max = BAD_FLT;
    statrec->avg = BAD_FLT;
    statrec->med = BAD_FLT;
    statrec->cen = BAD_FLT;
    statrec->sqr = BAD_FLT;
    statrec->sd = BAD_FLT;
    statrec->cnt = 0;

    /* compute stats for window */
    for (j = is1; j <= is2; j += 2) {
        /* don't use bad middle line - aqua detector zero channel 23 */
        if (j != is) {
            ipb = ip * nbands + ib;
            Bt = l1que.r[j].Bt[ipb];
            if (Bt > BT_LO + 0.1 && Bt < BT_HI - 0.1) {
                if (statrec->cnt == 0) {
                    /* first value - start the summing for the average at zero */
                    statrec->avg = 0.0;
                }
                statrec->avg += Bt;
                statrec->cnt++;
            }
        }
    }
    if (statrec->cnt > 0)
        statrec->avg /= statrec->cnt;

    return (statrec->cnt);
}

/* ----------------------------------------------------------------------------------- */
/* sstmasked() - returns 1 if pixel was already masked (SST processing skipped)        */

/* ----------------------------------------------------------------------------------- */
int sstmasked(int32_t *flags, int32_t ip) {
    if ((flags[ip] & LAND) != 0 || (flags[ip] & NAVFAIL) != 0)

        return (1);
    else
        return (0);
}

/* ------------------------------------------------------------------- */
/* avhrr_ascend() - compare center lat of this line and  previous line */

/* ------------------------------------------------------------------- */
int32_t avhrr_ascend(int32_t ny, float *diflat) {
    extern l1qstr l1que;

    int32_t nscan = l1que.nq;
    int32_t is, cpix;
    int32_t jj;
    //float diflat;
    float diflats[FILTMAX];

    /* make sure code is not inconsistent NQMIN in l12_parms.h */
    if (nscan < ny) {
        printf(
                "-E- %s line %d: L1 queue size of %d is too small for %d line box stats.\n",
                __FILE__, __LINE__, l1que.nq, ny);
        exit(-1); /* exit value changed from 1 to -1 */
    }

    /* current row is center of box */
    is = nscan / 2; /* is =1 for boxsize (nscan=ny=) 3 */

    /* this WILL now work if working on a subset of the data (spixl or epixl were specified) */
    /* 204/409 or 1023/2048 or 1599/3200 (zero based) */
    /* pixel number start at 0 so first half of scan lines are:
     *        0..204, 0..1023, 0..1599 */
    cpix = (fullscanpix - 1) / 2;
    //    if (l1que.r[is - 1].lat[cpix] < l1que.r[is].lat[cpix]) {
    //    }
    //    I don't know if I ever checked this for avhrr, but modis and viirs seem to wobble near the north pole (I didn't check south)
    // Their nadir lat's don't always increase when approaching the north pole, sometimes they decrease from one scan to the next.
    //    so check for average difference between nadir lats to find split between ascending and descending
    //    average didn't work.  in V2016183000600, which ascends near the dateline and descends over Russia, 
    //    every 16th line (the difference between detector 15 and detector 0) is a greater change in latitude
    //    than the difference between the lines in one scan.  Hard to describe, but easy to see if plot clat[2900:3246]
    //    Try assuming flip from ascending to descending (and reverse in the south) is only when two in a row match.
    *diflat = 0.0;
    //*diflat=*diflat+l1que.r[jj].lat[cpix] - l1que.r[jj+1].lat[cpix];
    //*diflat=*diflat/(nscan-1);
    // check the diff in clat between the lines in the box
    for (jj = 0; jj < nscan - 1; jj++) {
        diflats[jj] = l1que.r[jj].lat[cpix] - l1que.r[jj + 1].lat[cpix];
    }
    // check for no change in direction
    if ((diflats[is - 1] > 0.0 && diflats[is] > 0.0) ||
            (diflats[is - 1] < 0.0 && diflats[is] < 0.0)) {
        *diflat = diflats[is];
    } else {
        // there was a change.  But really only if the next dif matches the same direction, otherwise it's just the flaky
        // change between swaths (detector 15 to detector 0)
        if ((diflats[is] > 0.0 && diflats[is + 1] > 0.0) ||
                (diflats[is] < 0.0 && diflats[is + 1] < 0.0)) {
            *diflat = diflats[is];
        } else {
            // not really a change, return the previous diff (or next, but not this one)
            *diflat = diflats[is - 1];
        }
    }

    //if (l1que.cscan == 156) {
    //	printf(" in ascend, diflat avg = %f\n", *diflat);
    //}
    if (*diflat < 0.0) {
        /* satellite is ascending */
        return (1);
    } else {
        /* satellite is not ascending (usually descending) */
        return (0);
    }
}

/* ----------------------------------------------------------------------------------- */
/* set_flags_sst() - set quality flags for long wave sea surface temperature           */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
void set_flags_sst(l2str *l2rec) {

    extern l1qstr l1que;
    int32_t npix = l2rec->l1rec->npix;
    int32_t cpix;
    int32_t ip, ipb, ipbir;
    int32_t yyyyddd;
    int32_t ASCEND;
    int32_t v5viirs;
    float LtRED;
    float LtNIR7; // change m6 to LtNIR7, as it's VIIRS 746nm;
    float rhoCirrus; // change m9 to rhoCirrus (VIIRS 1.38um);
    float Lt16; // change m10 to Lt16, as it's VIIRS 1.61 micron band;
    float LtNIR8;
    float Bt37;
    float Bt39;
    float Bt40;
    float Bt67;
    float Bt85; /* aqua trees - don't use 67, 73, 85 because of cross talk, until fixed 2016? */
    float Bt11;
    float Bt12;
    float dSST_ref; /* sst - ref */
    float dSST_SST3; /* sst - sst3 */
    float dSST_SST4; /* sst - sst4 */
    float dBt_11_12, dBt_37_11, dBt_37_12, dBt_11_37, dBt_67_11; //brightness temp diffs
    float dBt_40_11, dBt_85_11;
    float Tdeflong;
    float subsolar, xdoy, xrad;
    float diflat;
    statstr statrec;

    /* this WILL now work if working on a subset of the data (spixl or epixl were specified) */
    cpix = (fullscanpix - 1) / 2; /* 204/409 or 1023/2048 (zero based) */

    get_toa_refl(l2rec, ibred, rhotRED);
    get_toa_refl(l2rec, ib07, rhotNIR7);
    get_toa_refl(l2rec, ib16, rhot16);

    /* check each pixel in scan */
    for (ip = 0; ip < npix; ip++) {

        /* SST not processed */
        if (sstmasked(l2rec->l1rec->flags, ip)) {
            flags_sst[ip] |= SSTF_ISMASKED;
            continue;
        }

        ipb = ip * l2rec->l1rec->l1file->nbands;
        ipbir = ip * NBANDSIR;

        treesum[ip] = 0.0;

        Bt37 = l2rec->l1rec->Bt[ipbir + ib37]; /* modis chan 20, avhrr cen3, viirs SVM12 */
        Bt85 = l2rec->l1rec->Bt[ipbir + ib85]; /* modis chan 29,             viirs SVM14 */
        Bt11 = l2rec->l1rec->Bt[ipbir + ib11]; /* modis chan 31, avhrr cen4, viirs SVM15 */
        Bt12 = l2rec->l1rec->Bt[ipbir + ib12]; /* modis chan 32, avhrr cen5, viirs SVM16 */

        LtRED = l2rec->l1rec->Lt[ipb + ibred];

        // some bands really are sensor specific...
        if (l2rec->l1rec->l1file->sensorID == AVHRR) {
            LtNIR8 = l2rec->l1rec->Lt[ipb + ib08];
        } else {
            rhoCirrus = l2rec->l1rec->rho_cirrus[ip];
            if (l2rec->l1rec->l1file->sensorID == MODIST || l2rec->l1rec->l1file->sensorID == MODISA) {
                Bt39 = l2rec->l1rec->Bt[ipbir + ib39];
                /* don't use aqua Bt40 detector 0, average the previous and next scans instead */
                if (l2rec->l1rec->l1file->sensorID == MODISA && l2rec->l1rec->detnum == 0) {
                    //Bt40 = Bt40_avg[ip];
                    /* current row is center of box */
                    int32_t is = l1que.nq / 2;
                    if (btavg(ip, is, ib40, NBANDSIR, &statrec) > 0) {
                        Bt40 = statrec.avg;
                    }
                } else
                    Bt40 = l2rec->l1rec->Bt[ipbir + ib40];
                Bt67 = l2rec->l1rec->Bt[ipbir + ib67];
            } else if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
                Bt40 = l2rec->l1rec->Bt[ipbir + ib40];
                LtNIR7 = l2rec->l1rec->Lt[ipb + ib07];
                Lt16 = l2rec->l1rec->Lt[ipb + ib16];
            }
        }

        /* BT could not be computed (radiance out-of-range) */
        if (Bt11 < BT_LO + 0.1 || Bt11 > BT_HI - 0.1 || Bt12 < BT_LO + 0.1
                || Bt12 > BT_HI - 0.1) {
            flags_sst[ip] |= SSTF_BTBAD;
            continue;
        }

        /* check BT range */
        if (Bt11 < Btmin || Bt11 > Btmax || Bt12 < Btmin || Bt12 > Btmax)
            flags_sst[ip] |= SSTF_BTRANGE;

        if (l2rec->l1rec->l1file->sensorID == AVHRR) {
            if ((ASCEND = avhrr_ascend(btbox, &diflat)) == 1) {
                flags_sst[ip] |= SSTF_ASCEND;
            }
            /* if day and glint > thresh set bit M2B8 (shared with BT4REFDIFF) */
            if (l2rec->l1rec->solz[ip] < solznight) {
                if (l2rec->l1rec->glint_coef[ip] > glintmax)
                    flags_sst[ip] |= SSTF_GLINT;
            }

            /* ------------------------------------------------------------------------- */
            /* Gross Cloud contamination test for tree test BTRANGE (M1B1)               */
            /*   Based on Kilpatrick et al:                                              */
            /*   Overfiew of NOAA/NASA AVHRR Pathfinder Algorithm. Figure 6.             */
            /* ------------------------------------------------------------------------- */
            /* check BT range: BT must be ge -10 C and le 35 C, M1B1 */
            /* some noaa satellites need channel 3, and some only at night */
            /* if n7-15 at night or at day and not glint or
             * if n16-19 at night */
            /* Nov 2012, day only check too cold except older avhrr need too warm check also */

            /* if older avhrr sat and night or day and not glint check for Bt37 too warm or too cold */
            if ((l2rec->l1rec->l1file->subsensorID == NO07
                    || l2rec->l1rec->l1file->subsensorID == NO09
                    || l2rec->l1rec->l1file->subsensorID == NO10
                    || l2rec->l1rec->l1file->subsensorID == NO11
                    || l2rec->l1rec->l1file->subsensorID == NO12
                    || l2rec->l1rec->l1file->subsensorID == NO14
                    || l2rec->l1rec->l1file->subsensorID == NO15)
                    && (l2rec->l1rec->solz[ip] >= solznight
                    || (l2rec->l1rec->solz[ip] < solznight
                    && l2rec->l1rec->glint_coef[ip] <= glintmax))) {
                if (Bt37 < Btmin || Bt37 > Btmax) {
                    flags_sst[ip] |= SSTF_BTRANGE;
                }
            }

            /* if n16-19 Bt37 at night (channel 3 is a different wavelength during the day) is too warm or too cold */
            if ((l2rec->l1rec->l1file->subsensorID == NO16
                    || l2rec->l1rec->l1file->subsensorID == NO17
                    || l2rec->l1rec->l1file->subsensorID == NO18
                    || l2rec->l1rec->l1file->subsensorID == NO19)
                    && l2rec->l1rec->solz[ip] >= solznight) {
                if (Bt37 < Btmin || Bt37 > Btmax) {
                    flags_sst[ip] |= SSTF_BTRANGE;
                }
            }
        } else {
            /* for viirs (and modis), to test l2bin dataday splitting and not mixing ascending and descending, set spare flag */
            if ((ASCEND = avhrr_ascend(btbox, &diflat)) == 1) {
//                l2rec->l1rec->flags[ip] |= ASCFLG; //NEED THIS FLAG DEFINITION FROM SOMEWHERE
            }
            //if (ip == cpix) {
            //    printf(" pix=%d line=%d diflat=%f\n",ip,l2rec->l1rec->iscan,diflat);
            //}
        }
        if ((l2rec->l1rec->l1file->sensorID == MODIST || l2rec->l1rec->l1file->sensorID == MODISA)) {
            /* if modis Bt37 or Bt39 or Bt40 at night is too warm or too cold */

            if (l2rec->l1rec->solz[ip] >= solznight
                    && (Bt37 < Btmin || Bt37 > Btmax || Bt39 < Btmin
                    || Bt39 > Btmax || Bt40 < Btmin || Bt40 > Btmax40)) {
                flags_sst[ip] |= SSTF_BTRANGE;
            }
            /* if modis Bt37 or Bt39 or Bt40 is during the day too cold */
            if ((l2rec->l1rec->solz[ip] < solznight
                    && l2rec->l1rec->glint_coef[ip] <= glintmax)
                    && (Bt37 < Btmin || Bt39 < Btmin || Bt40 < Btmin)) {
                flags_sst[ip] |= SSTF_BTRANGE;
            }
        }

        if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
            /* if viirs Bt37 or Bt40 at night is too warm or too cold */
            if (l2rec->l1rec->solz[ip] >= solznight
                    && (Bt37 < Btmin || Bt37 > Btmax || Bt40 < Btmin
                    || Bt40 > Btmax40)) {
                flags_sst[ip] |= SSTF_BTRANGE;
            }

            /* if viirs Bt37 or Bt40 during the day is too cold */
            if ((l2rec->l1rec->solz[ip] < solznight
                    && l2rec->l1rec->glint_coef[ip] <= glintmax)
                    && (Bt37 < Btmin || Bt40 < Btmin)) {
                flags_sst[ip] |= SSTF_BTRANGE;
            }
        }

        /* check BT diff */
        dBt_11_12 = Bt11 - Bt12;
        if (dBt_11_12 < dBtmin || dBt_11_12 > dBtmax)
            flags_sst[ip] |= SSTF_BTDIFF;

        /* check SST range, using different max limits for day and night */
        if ((sstq[csstbox][ip] < SSTmin)
                || (l2rec->l1rec->solz[ip] < solznight && sstq[csstbox][ip] > SSTmax)
                || (l2rec->l1rec->solz[ip] >= solznight && sstq[csstbox][ip] > SSTmaxn)) {
            flags_sst[ip] |= SSTF_SSTRANGE;
        }

        /* check SST difference with references */
        /* check for both bad, or don't bother checking anywhere?
         * if either are bad then SSTF_SSTRANGE will be set so
         * no other flags matter? */
        dSST_ref = sstq[csstbox][ip] - l2rec->l1rec->sstref[ip];
        // the "coldonly" and equatorial aerosol tests are to be run by default, but if SSTMODS is set, don't
        if ((evalmask & SSTMODS) == 0) {
            /* evaluate change to cold-test only */
            /* set the flag bit if sst is too much colder than reference */
            if (dSST_ref < -SSTdiff || l2rec->l1rec->sstref[ip] < sstbad + 1.0)
                flags_sst[ip] |= SSTF_SSTREFDIFF;
            if (dSST_ref < -input->sstrefdif &&
                    l2rec->l1rec->lat[ip] >= equatorialSouth && l2rec->l1rec->lat[ip] <= equatorialNorth &&
                    l2rec->l1rec->lon[ip] >= equatorialWest && l2rec->l1rec->lon[ip] <= equatorialEast) {
                /* tighter test between 10S and 30N and -105 to 105 longitude */
                /* equatorial aerosol test */
                flags_sst[ip] |= SSTF_SSTREFDIFF;
            }
            /* set the flag bit if sst is too much warmer than reference at night */
            if (l2rec->l1rec->solz[ip] >= solznight) {
                if (fabs(dSST_ref) > SSTdiff)
                    flags_sst[ip] |= SSTF_SSTREFDIFF;
            }
            /* is sst way too much colder than reference? */
            if (dSST_ref < -SSTvdiff || l2rec->l1rec->sstref[ip] < sstbad + 1.0)
                flags_sst[ip] |= SSTF_SSTREFVDIFF;
            /* set the flag bit if sst is way too much warmer than reference at night */
            if (l2rec->l1rec->solz[ip] >= solznight) {
                if (fabs(dSST_ref) > SSTvdiff)
                    flags_sst[ip] |= SSTF_SSTREFVDIFF;
            }
        } else {
            if (l2rec->l1rec->solz[ip] >= SOLZNIGHT) {
                if (fabs(dSST_ref) > SSTdiff)
                    flags_sst[ip] |= SSTF_SSTREFDIFF;
            } else {
                if (dSST_ref < -SSTdiff || dSST_ref > (SSTdiff + 1))
                    flags_sst[ip] |= SSTF_SSTREFDIFF;
            }
            if (fabs(dSST_ref) > SSTvdiff)
                flags_sst[ip] |= SSTF_SSTREFVDIFF;
        } //end of to-do-or-not-to-do coldonly block

        /* check SST difference with 4um SST only at night */
        /* set BT4REFDIFF flag based on sst4 flags */
        if (haveSST4) {
            dSST_SST4 = sstq[csstbox][ip] - sst4q[csstbox][ip];
            if (sst4q[csstbox][ip] > sstbad + 1.0 && l2rec->l1rec->solz[ip] >= SOLZNIGHT) {
                if (fabs(dSST_SST4) < SST4diff1)
                    flags_sst[ip] |= SSTF_SST4DIFF;
                if (fabs(dSST_SST4) < SST4diff2)
                    flags_sst[ip] |= SSTF_SST4VDIFF;
                flags_sst[ip] |= (flags_sst4[ip] & SSTF_BT4REFDIFF);
            }
        }

        /* for viirs, check SST difference with jpss triple window SST only at night  */
        if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
            dSST_SST3 = sstq[csstbox][ip]-sst3q[csstbox][ip];
            if (sst3q[csstbox][ip] > sstbad+1.0 && l2rec->l1rec->solz[ip] >= SOLZNIGHT) {
                if (fabs(dSST_SST3) > SST3diff1)
                    flags_sst[ip] |= SSTF_SST3DIFF;
                if (fabs(dSST_SST3) > SST3diff2)
                    flags_sst[ip] |= SSTF_SST3VDIFF;
            }
        }

        /* check sensor zenith limits */
        if (l2rec->l1rec->senz[ip] > hisenz)
            flags_sst[ip] |= SSTF_HISENZ;
        if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
            if (l2rec->l1rec->senz[ip] > vhisenzv2) {
                /* different limit only for viirs sst (not sst3) */
                flags_sst[ip] |= SSTF_VHISENZ;
            }
        } else {
            /* not viirs */
            if (l2rec->l1rec->senz[ip] > vhisenz)
                flags_sst[ip] |= SSTF_VHISENZ;
        }

        // flag 2 edge pixels as SSTF_VHISENZ so quality gets set to 3
        if (l2rec->l1rec->pixnum[ip] < 2 || l2rec->l1rec->pixnum[ip] > (fullscanpix - 3))
            flags_sst[ip] |= SSTF_VHISENZ;
        // set the last 4 pixels of the scan for Terra to VHISENZ 
        // as there is an apparent calibration issue with the BT12 for those pixels
        if ((l2rec->l1rec->l1file->sensorID == MODIST) && (l2rec->l1rec->pixnum[ip] > 1349))
            flags_sst[ip] |= SSTF_VHISENZ;

        /* if SST is cold (collect 5) */
        /* I suppose it doesn't hurt to do this for AVHRR since it isn't used to determine quality? */
        /* do this for all satellites */
        /* sstcloud checks to make sure it's day and not glint */
        if (sstq[csstbox][ip] > sstbad + 1.0 && l2rec->l1rec->sstref[ip] > sstbad + 1.0
                && sstq[csstbox][ip] - l2rec->l1rec->sstref[ip] <= -1.0)
            if (sstcloud(ip, cldbox, cldbox, cldthresh) == 1)
                flags_sst[ip] |= SSTF_REDNONUNIF;

        /* check homogeneity of BT */
        if (Bt11_maxmin[ip] > Bt11unif1)
            flags_sst[ip] |= SSTF_BTNONUNIF;
        if (Bt11_maxmin[ip] > Bt11unif2)
            flags_sst[ip] |= SSTF_BTVNONUNIF;

        if (Bt12_maxmin[ip] > Bt12unif1)
            flags_sst[ip] |= SSTF_BTNONUNIF;
        if (Bt12_maxmin[ip] > Bt12unif2)
            flags_sst[ip] |= SSTF_BTVNONUNIF;

        /* end of homogeneity checks */

        dBt_37_11 = Bt37 - Bt11;
        dBt_37_12 = Bt37 - Bt12;
        dBt_11_37 = Bt11 - Bt37;
        dBt_67_11 = Bt67 - Bt11;
        dBt_40_11 = Bt40 - Bt11;
        dBt_85_11 = Bt85 - Bt11;

        /* values from Kay's VIIRS trees */
        /* Bt's are BT_LO = -1000 or BT_HI = 1000 when bad */
        /* If a Bt is bad then the pixel will be bad so it doesn't matter what happens in the trees */
        if (Bt11 != 0.0) {
            Tdeflong = (Bt11 - Bt12) / Bt11;
        } else {
            Tdeflong = (Bt11 - Bt12) / (Bt11 + 0.00001); /* don't divide by zero */
        }
        double pasutime = l2rec->l1rec->scantime;
        int16_t year, day;
        double sec;
        unix2yds(pasutime, &year, &day, &sec);

        yyyyddd = year * 1000 + day;

        /* --------------------------------------------------------------------------*/
        /* decision_tree() - Tree models are based on binary recursive partitioning  */
        /*                   to indicate whether data are potentially contaminated.  */
        /*                   based on charts sent to NODC from Kay Kilpatrick        */
        /*                   worked on by  Vicky Lin, SAIC, October 2007.            */
        /*                   V6 trees from Guillermo Podesta.                        */
        /* ------------------------------------------------------------------------- */
        switch (l2rec->l1rec->l1file->subsensorID) {
            case NO07:
                /* use the noaa-9 tree for now (Kay: Mar 2013) */
                //		break;
            case NO09:
                if (yyyyddd < 1994001) {
                    /* 9-Jun-97 matchups - NOAA-9 before Jan 1994 */
                    if ((l2rec->l1rec->solz[ip] >= solznight
                            || (l2rec->l1rec->solz[ip] < solznight
                            && l2rec->l1rec->glint_coef[ip] <= glintmax))
                            && dBt_37_12 < -0.4805) {
                        if (dBt_37_12 < -1.249) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        } else if (dBt_11_12 >= 0.239) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        } else if (Bt37_maxmin[ip] >= 0.7975
                                && dBt_11_12 >= -0.1805) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        }
                    } else {
                        if (dBt_11_12 < 0.307
                                && ((l2rec->l1rec->solz[ip] >= solznight
                                || (l2rec->l1rec->solz[ip] < solznight
                                && l2rec->l1rec->glint_coef[ip] <= glintmax))
                                && Bt37_maxmin[ip] >= 1.6215)) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        }
                    }
                } else {
                    /* 9-Jun-97 matchups - NOAA-9 after Jan 1994 */
                    if ((l2rec->l1rec->solz[ip] >= solznight
                            || (l2rec->l1rec->solz[ip] < solznight
                            && l2rec->l1rec->glint_coef[ip] <= glintmax))
                            && dBt_37_12 < 0.62) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    } else if (dBt_11_12 < 0.5055
                            && ((l2rec->l1rec->solz[ip] >= solznight
                            || (l2rec->l1rec->solz[ip] < solznight
                            && l2rec->l1rec->glint_coef[ip] <= glintmax))
                            && Bt37_maxmin[ip] >= 1.033)
                            && Bt11_maxmin[ip] >= 0.2435) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                }
                break;
            case NO10:
                break;
            case NO11:
                if ((l2rec->l1rec->solz[ip] >= solznight
                        || (l2rec->l1rec->solz[ip] < solznight && l2rec->l1rec->glint_coef[ip] <= glintmax))
                        && dBt_37_12 < -0.581) {
                    flags_sst[ip] |= SSTF_CLOUD;
                } else {
                    if (dBt_11_12 < 0.315) {
                        if ((l2rec->l1rec->solz[ip] >= solznight
                                || (l2rec->l1rec->solz[ip] < solznight
                                && l2rec->l1rec->glint_coef[ip] <= glintmax))
                                && dBt_11_37 < -0.1855) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        } else if ((l2rec->l1rec->solz[ip] >= solznight
                                || (l2rec->l1rec->solz[ip] < solznight
                                && l2rec->l1rec->glint_coef[ip] <= glintmax))
                                && Bt37_maxmin[ip] >= 0.9555) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        }
                    } else if ((l2rec->l1rec->solz[ip] >= solznight
                            || (l2rec->l1rec->solz[ip] < solznight
                            && l2rec->l1rec->glint_coef[ip] <= glintmax))
                            && dBt_11_37 >= 0.86 && Bt37_maxmin[ip] >= 0.5035) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                }
                break;
            case NO12:
                break;
            case NO14:
                if (yyyyddd >= 1995001 && yyyyddd < 1996001) {
                    if ((l2rec->l1rec->solz[ip] >= solznight
                            || (l2rec->l1rec->solz[ip] < solznight
                            && l2rec->l1rec->glint_coef[ip] <= glintmax))
                            && dBt_11_37 >= 0.8015) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    } else if ((l2rec->l1rec->solz[ip] >= solznight
                            || (l2rec->l1rec->solz[ip] < solznight
                            && l2rec->l1rec->glint_coef[ip] <= glintmax))
                            && Bt37_maxmin[ip] >= 1.216) {
                        if (dBt_11_12 < 1.342) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        }
                    } else if (dBt_11_12 < 1.3225
                            && ((l2rec->l1rec->solz[ip] >= solznight
                            || (l2rec->l1rec->solz[ip] < solznight
                            && l2rec->l1rec->glint_coef[ip] <= glintmax))
                            && dBt_11_37 < 0.994)) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                } else if (yyyyddd >= 1996001) {
                    if (dBt_11_12 >= 0.755) {
                        if ((l2rec->l1rec->solz[ip] >= solznight
                                || (l2rec->l1rec->solz[ip] < solznight
                                && l2rec->l1rec->glint_coef[ip] <= glintmax))
                                && (dBt_11_37 >= 0.7995 || LtNIR8 >= 0.7225)) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        }
                    } else {
                        if ((l2rec->l1rec->solz[ip] >= solznight
                                || (l2rec->l1rec->solz[ip] < solznight
                                && l2rec->l1rec->glint_coef[ip] <= glintmax))
                                && (dBt_11_37 >= 1.1475 || LtNIR8 >= 1.2645
                                || Bt37_maxmin[ip] >= 0.966)) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        } else {
                            if (((l2rec->l1rec->solz[ip] >= solznight
                                    || (l2rec->l1rec->solz[ip] < solznight
                                    && l2rec->l1rec->glint_coef[ip] <= glintmax))
                                    && dBt_11_37 >= 0.869)
                                    || (dBt_11_12 < 0.462
                                    && ((l2rec->l1rec->solz[ip] >= solznight
                                    || (l2rec->l1rec->solz[ip] < solznight
                                    && l2rec->l1rec->glint_coef[ip] <= glintmax))
                                    && LtNIR8 >= 0.4865))) {
                                flags_sst[ip] |= SSTF_CLOUD;
                            }
                        }
                    }
                }
                break;
            case NO15:
                break;
            case NO16:
                /* pathfinder v6 trees */
                /* patfinder v6 2001-2003 */
                if (l2rec->l1rec->solz[ip] < solznight) {
                    /* day */
                    if ((l2rec->l1rec->glint_coef[ip] <= glintmax && LtRED >= 0.7315)
                            || dBt_11_12 < 0.5375) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                } else {
                    /* night */
                    if (dBt_37_11 < -0.1025 || Bt37_maxmin[ip] >= 1.035
                            || dBt_11_12 < 0.7385) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    } else if (Bt37_maxmin[ip] < 0.7145) {
                        if (dBt_11_12 < 1.454) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        }
                    } else if (dBt_11_12 < 1.271) {
                        if (dBt_37_11 >= 0.9195) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        }
                    } else if (dBt_37_11 >= 2.408 && dBt_11_12 < 2.272) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                }
                break;
            case NO17:
                /* pathfinder v6 trees */
                /* NO17 matchups 2003-2006 - pathfinder v6 */
                if (l2rec->l1rec->solz[ip] < solznight) {
                    /* day */
                    if (l2rec->l1rec->glint_coef[ip] <= glintmax && LtRED >= 0.7045) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    } else if (l2rec->l1rec->lat[ip] >= -20.0 && dBt_11_12 < 0.4355) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                } else {
                    /* night */
                    if (dBt_37_12 < 0.0355 || Bt37_maxmin[ip] >= 0.9035
                            || dBt_11_12 < 0.4565 || dBt_37_11 < -0.5515) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    } else if (dBt_37_11 >= 1.635 && dBt_11_12 < 1.693) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                }
                break;
            case NO18:
                /* pathfinder v6 trees */
                if (l2rec->l1rec->solz[ip] < solznight) {
                    /* day */
                    /* NOA18 matchups 2005-2009 tree - Pathfinder version 6 */
                    if (l2rec->l1rec->glint_coef[ip] <= glintmax && LtRED >= 0.7045) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    } else if (dBt_11_12 < 0.4995) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                } else {
                    /* night  */
                    /* NOA18 matchups 2005-2009 tree - Pathfinder version 6 */
                    if (dBt_37_12 < 0.3185) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    } else if (Bt37_maxmin[ip] >= 0.8455) {
                        if (dBt_11_12 < 1.477) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        }
                    } else if (dBt_11_12 < 0.6605) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    } else if (dBt_37_11 < -0.5745) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    } else if (dBt_37_11 >= 2.1015 && dBt_11_12 < 2.206) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                }
                break;
            case NO19:
                if (l2rec->l1rec->solz[ip] < solznight) {
                    /* day */
                    if ((l2rec->l1rec->glint_coef[ip] <= glintmax && LtRED >= 0.84648)
                            || dBt_11_12 < 0.168655
                            || abs(sstq[csstbox][ip] - l2rec->l1rec->sstref[ip]) >= 1.237905) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                } else {
                    /* night  */
                    if (dBt_11_12 < 0.99571) {
                        if (dBt_37_12 < -0.26932 || dBt_11_12 < 0.126555
                                || Bt37_maxmin[ip] >= 1.02994
                                || abs(sstq[csstbox][ip] - l2rec->l1rec->sstref[ip]) >= 0.758065) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        }
                    } else if (dBt_37_11 < -0.387815
                            || abs(sstq[csstbox][ip] - l2rec->l1rec->sstref[ip]) >= 0.857825) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                }
                break;
            default:
                /* modis trees are only v6 */

                /* modis trees were built using gsfc extractions with rho for the vis bands */
                /* but l2gen puts radiance in LtRED so need to use rhotRED in the tree tests */

                if (l2rec->l1rec->l1file->sensorID == MODIST) {
                    if (l2rec->l1rec->solz[ip] < solznight
                            && l2rec->l1rec->glint_coef[ip] <= glintmax) {
                        /* day not glint TERRA SST tree test */
                        treesum[ip] = 0.0;
                        /* seadas (594,662) is outside glint for comparison to glint pixels that are wrong quality */
                        /* Bad Lt's could be -32767, 0 (class only so ignore), or 1000. */
                        /* rho is the same sign so really could check rhotRED instead of LtRED,
                           except where we have to check for 1000.0 */
                        if (rhotRED[ip] < 0.206 && LtRED > (BAD_FLT + 0.1) && LtRED < (1000.0 - 0.1)) {
                            treesum[ip] += 0.894;
                            if (rhotRED[ip] < 0.051 && LtRED > (BAD_FLT + 0.1) && LtRED < (1000.0 - 0.1)) {
                                treesum[ip] += 0.540;
                                if (l2rec->l1rec->senz[ip] < 64.915) {
                                    treesum[ip] += -0.057;
                                    if (rhoCirrus_max[ip] < 0.003 && rhoCirrus_max[ip] > (BAD_FLT + 0.1)) {
                                        treesum[ip] += 0.087;
                                        if (l2rec->l1rec->senz[ip] < 54.529) {
                                            treesum[ip] += 0.030;
                                        } else {
                                            treesum[ip] += -0.573;
                                        }
                                    } else {
                                        treesum[ip] += -0.828;
                                    }
                                } else {
                                    treesum[ip] += -2.700;
                                }
                            } else {
                                treesum[ip] += -0.490;
                            }
                            /* default min is 32767, but gets set to -32767 if no valid values in box */
                            if (rhoCirrus_min[ip] < 0.004 && rhoCirrus_min[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.210;
                                if (rhotRED[ip] < 0.073 && LtRED > (BAD_FLT + 0.1) && LtRED < (1000.0 - 0.1)) {
                                    treesum[ip] += 0.074;
                                } else {
                                    treesum[ip] += -0.467;
                                }
                            } else {
                                treesum[ip] += -1.008;
                            }
                        } else {
                            treesum[ip] += -1.402;
                            /* don't need LtRED_min: if Lt is -32767, 0.0 or
                               1000.0 (which makes rho large) it will go to
                               the negative treesum */
                            /* default min is 32767, but gets set to -32767 if no valid values in box */
                            if (rhotRED_min[ip] < 0.239 && rhotRED_min[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.821;
                            } else {
                                treesum[ip] += -0.033;
                                if (dBt_67_11 < -42.423) {
                                    treesum[ip] += 0.283;
                                } else {
                                    treesum[ip] += -0.184;
                                }
                            }
                        }
                        if (dBt_37_11 < 3.493) {
                            treesum[ip] += 0.530;
                            if (dBt_11_12 < 0.178) {
                                treesum[ip] += -0.589;
                            } else {
                                treesum[ip] += 0.174;
                            }
                        } else {
                            treesum[ip] += -0.169;
                            /* default min is 32767, but gets set to -32767 if no valid values in box */
                            if (rhoCirrus_min[ip] < 0.008 && rhoCirrus_min[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.097;
                            } else {
                                treesum[ip] += -0.761;
                            }
                        }
                        if (sst_stdev[ip] < 0.273 && sst_stdev[ip] > (BAD_FLT + 0.1)) {
                            treesum[ip] += 0.258;
                        } else {
                            treesum[ip] += -0.194;
                        }
                        if (sst_stdev[ip] < 0.119 && sst_stdev[ip] > (BAD_FLT + 0.1)) {
                            treesum[ip] += 0.459;
                        } else {
                            treesum[ip] += -0.037;
                            if (sstq[csstbox][ip] < 27.293) {
                                treesum[ip] += -0.054;
                            } else {
                                treesum[ip] += 0.411;
                            }
                        }
                    } else if (l2rec->l1rec->solz[ip] < solznight) {
                        /* day in high and low glint TERRA SST tree tests */
                        if (LtRED < (1000.0 - 0.1)) {
                            /* RED is not saturated */
                            /* day in low glint TERRA SST tree test */
                            treesum[ip] = 0.0;
                            /* seadas (600,663) should be qual 1 not 3; terra 20141701425 */
                            /* seadas (611,698) should be qual 3 not 1; descending so not flipped */
                            /* don't check for > 0.0 here because we want the negative
                               treesum if it's < 0.0  or = 1000.0 */
                            //if (rhotRED[ip] < 0.098 || LtRED >= (1000.0-0.1)) {}
                            /* Jun 2017, don't know why the above comment and test, but want positive tree sum only if Lt was valid */
                            if (rhotRED[ip] < 0.098 && LtRED > (BAD_FLT + 0.1) && LtRED < (1000.0 - 0.1)) {
                                treesum[ip] += 1.026;
                                if (rhotRED[ip] < 0.069 && LtRED > (BAD_FLT + 0.1) && LtRED < (1000.0 - 0.1)) {
                                    treesum[ip] += 0.290;
                                } else {
                                    treesum[ip] += -0.312;
                                }
                                /* rhoCirrus is -32767 when bad */
                                if (rhoCirrus < 0.002 && rhoCirrus > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.154;
                                } else {
                                    treesum[ip] += -0.574;
                                }
                                /* if bad then it will go negative */
                                if (Bt73_max[ip] < -10.303) {
                                    treesum[ip] += -0.150;
                                } else {
                                    treesum[ip] += 0.393;
                                }
                            } else {
                                treesum[ip] += -0.761;
                                if (Bt11 < 16.122) {
                                    treesum[ip] += -0.538;
                                } else {
                                    treesum[ip] += 0.707;
                                }
                            }
                            /* rhoCirrus is -32767 when bad so make sure it goes negative if bad */
                            if (rhoCirrus_max[ip] < 0.005 && rhoCirrus_max[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.357;
                                if (Tdeflong < 0.037) {
                                    treesum[ip] += -0.450;
                                    if (l2rec->l1rec->lat[ip] < 28.270) {
                                        treesum[ip] += -0.383;
                                    } else {
                                        treesum[ip] += 0.560;
                                    }
                                } else {
                                    treesum[ip] += 0.134;
                                }
                                if (l2rec->l1rec->senz[ip] < 34.756) {
                                    treesum[ip] += 0.098;
                                } else {
                                    treesum[ip] += -0.424;
                                }
                            } else {
                                treesum[ip] += -1.399;
                                /* default min is 32767, but gets set to -32767 if no valid values in box */
                                if (rhoCirrus_min[ip] < 0.01 && rhoCirrus_min[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.326;
                                } else {
                                    treesum[ip] += -1.404;
                                }
                            }
                            /* sst_stdev is -32767 if bad so make sure it goes negative if bad */
                            if (sst_stdev[ip] < 0.382 && sst_stdev[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.130;
                                if (l2rec->l1rec->lat[ip] < -5.005) {
                                    treesum[ip] += 0.400;
                                } else {
                                    treesum[ip] += -0.107;
                                }
                                /* sst_stdev is -32767 if bad so make sure it goes negative if bad */
                                if (sst_stdev[ip] < 0.224 && sst_stdev[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.182;
                                } else {
                                    treesum[ip] += -0.216;
                                }
                            } else {
                                treesum[ip] += -0.632;
                            }
                            if (dBt_85_11 < -1.641) {
                                treesum[ip] += 0.268;
                            } else {
                                treesum[ip] += -0.201;
                                if (sstq[csstbox][ip] < 27.723) {
                                    treesum[ip] += -0.069;
                                } else {
                                    treesum[ip] += 0.903;
                                }
                            }
                        } else {
                            /* RED is saturated */
                            /* day high glint terra sst tree test */
                            treesum[ip] = 0.0;
                            /* default min is 32767, but gets set to -32767 if no valid values in box */
                            if (Bt12_min[ip] < 15.376) {
                                treesum[ip] += -0.908;
                                if (dBt_11_12 < 0.27) {
                                    treesum[ip] += -0.446;
                                } else {
                                    treesum[ip] += 0.568;
                                }
                                /* default min is 32767, but gets set to -32767 if no valid values in box */
                                if (rhoCirrus_min[ip] < -0.001) {
                                    treesum[ip] += -0.981;
                                } else {
                                    treesum[ip] += 0.137;
                                }
                            } else {
                                treesum[ip] += 0.819;
                                if (Tdeflong < 0.031) {
                                    treesum[ip] += -0.969;
                                } else {
                                    treesum[ip] += 0.025;
                                }
                                /* bad is -32767 so it will go negative if bad */
                                if (Bt73_max[ip] < -9.485) {
                                    treesum[ip] += -0.208;
                                } else {
                                    treesum[ip] += 0.428;
                                }
                            }
                            if (rhoCirrus_max[ip] < 0.005 && rhoCirrus_max[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.235;
                                if (rhoCirrus < 0.002 && rhoCirrus > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.100;
                                } else {
                                    treesum[ip] += -0.506;
                                }
                            } else {
                                treesum[ip] += -1.176;
                            }
                            if (sst_stdev[ip] < 0.36 && sst_stdev[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.222;
                            } else {
                                treesum[ip] += -0.657;
                            }
                            if (sstq[csstbox][ip] < 27.443) {
                                treesum[ip] += -0.077;
                                if (sst_stdev[ip] < 0.36 && sst_stdev[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.355;
                                } else {
                                    treesum[ip] += -0.179;
                                }
                                /* default min is 32767, but gets set to -32767 if no valid values in box */
                                if (rhoCirrus_min[ip] < 0.01 && rhoCirrus_min[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.022;
                                    if (l2rec->l1rec->lat[ip] < 29.775) {
                                        treesum[ip] += -0.128;
                                        if (l2rec->l1rec->lat[ip] < -15.305) {
                                            treesum[ip] += 0.431;
                                        } else {
                                            treesum[ip] += -0.202;
                                            /* bad max is -32767 so it will go negative without a specific test */
                                            if (Bt11_max[ip] < 14.722) {
                                                treesum[ip] += -1.906;
                                            } else {
                                                treesum[ip] += 0.087;
                                            }
                                        }
                                        if (Tdeflong < 0.04) {
                                            treesum[ip] += -0.336;
                                        } else {
                                            treesum[ip] += 0.156;
                                        }
                                    } else {
                                        treesum[ip] += 0.379;
                                    }
                                } else {
                                    treesum[ip] += -1.948;
                                }
                            } else {
                                treesum[ip] += 0.688;
                            }
                        }
                    } else {
                        /* night TERRA SST tree test */
                        treesum[ip] = 0.0;
                        if (dBt_37_12 < -0.053) {
                            treesum[ip] += -1.257;
                            if (l2rec->l1rec->lat[ip] < 33.195) {
                                treesum[ip] += -0.278;
                                if (l2rec->l1rec->lat[ip] < -40.185) {
                                    treesum[ip] += 0.619;
                                } else {
                                    treesum[ip] += -0.711;
                                    if (Bt37 < 6.477) {
                                        treesum[ip] += -3.733;
                                    } else {
                                        treesum[ip] += -0.111;
                                    }
                                }
                            } else {
                                treesum[ip] += 0.333;
                            }
                            if (Bt37 < 9.372) {
                                treesum[ip] += -0.292;
                            } else {
                                treesum[ip] += 0.764;
                            }
                        } else {
                            treesum[ip] += 0.430;
                            if (Bt11_maxmin[ip] < 0.486 && Bt11_maxmin[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.628;
                                if (Bt40_stdev[ip] < 0.146 && Bt40_stdev[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.177;
                                } else {
                                    treesum[ip] += -0.723;
                                }
                            } else {
                                treesum[ip] += -0.450;
                            }
                            if (dSST_SST4 < -0.878) {
                                treesum[ip] += -1.353;
                                if (dSST_SST4 < -1.533) {
                                    treesum[ip] += -1.439;
                                } else {
                                    treesum[ip] += 0.346;
                                }
                            } else {
                                treesum[ip] += 0.219;
                                if (Bt37_stdev[ip] < 0.448 && Bt37_stdev[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.290;
                                    if (dSST_SST4 < -0.422) {
                                        treesum[ip] += -0.504;
                                    } else {
                                        treesum[ip] += 0.268;
                                    }
                                } else {
                                    treesum[ip] += -0.484;
                                }
                                if (Bt12 < 16.736) {
                                    treesum[ip] += -0.285;
                                    if (dBt_40_11 < -2.199) {
                                        treesum[ip] += 0.518;
                                    } else {
                                        treesum[ip] += -0.316;
                                        if (Bt12 < 11.896) {
                                            treesum[ip] += -0.527;
                                        } else {
                                            treesum[ip] += 0.400;
                                        }
                                    }
                                } else {
                                    treesum[ip] += 0.500;
                                }
                                if (dSST_SST4 < 1.183) {
                                    treesum[ip] += 0.051;
                                } else {
                                    treesum[ip] += -0.898;
                                }
                            }
                        }
                    }
                    if (treesum[ip] <= 0.0) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                }/* end case for modis terra */
                else if (l2rec->l1rec->l1file->sensorID == MODISA) {
                    if (l2rec->l1rec->solz[ip] < solznight && l2rec->l1rec->glint_coef[ip] <= glintmax) {
                        /* day not glint AQUA SST tree test */
                        treesum[ip] = 0.0;
                        /* seadas (131,1112) should be qual 1 not 3; in l2gen it's (1353-131,2029-1112)=(1222,917) */
                        /* seadas (1086,1375) should be qual 1 not 3; in l2gen it's (1353-1086,2029-1375)=(1264,654) */
                        /* seadas says valid rhotRED is >= -0.1 & <= 1.0 */
                        /* seadas says valid cirrus is >= 0.0 & <= 1.1 */
                        if (rhotRED[ip] < 0.204 && LtRED > (BAD_FLT + 0.1) && LtRED < (1000.0 - 0.1)) {
                            treesum[ip] += 0.982;
                            if (rhotRED[ip] < 0.056 && LtRED > (BAD_FLT + 0.1) && LtRED < (1000.0 - 0.1)) {
                                treesum[ip] += 0.328;
                                /* bad cirrus is -32767 */
                                if (rhoCirrus_max[ip] < 0.006 && rhoCirrus_max[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.151;
                                    if (l2rec->l1rec->senz[ip] < 59.043) {
                                        treesum[ip] += 0.029;
                                    } else {
                                        treesum[ip] += -0.618;
                                    }
                                } else {
                                    treesum[ip] += -1.071;
                                }
                            } else {
                                treesum[ip] += -0.565;
                            }
                        } else {
                            treesum[ip] += -1.325;
                            /* if Lt is 1000 then rhot will be large so this will go negative */
                            /* default min is 32767, but gets set to -32767 if no valid values in box */
                            if (rhotRED_min[ip] < 0.251 && rhotRED_min[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.81;
                            } else {
                                treesum[ip] += -0.021;
                                if (l2rec->l1rec->lat[ip] < 48.685) {
                                    treesum[ip] += 0.605;
                                } else {
                                    treesum[ip] += -0.088;
                                    if (l2rec->l1rec->lat[ip] < 37.755) {
                                        treesum[ip] += -0.311;
                                    } else {
                                        treesum[ip] += 0.189;
                                    }
                                    if (dBt_11_12 < 0.752) {
                                        treesum[ip] += -0.091;
                                    } else {
                                        treesum[ip] += 0.581;
                                    }
                                }
                            }
                            // maybe add check to make sure Bt67 is valid? if ((Bt67 +/- 1000)
                            if (dBt_67_11 < -38.023) {
                                treesum[ip] += 0.221;
                            } else {
                                treesum[ip] += -0.201;
                            }
                        }
                        /* default min is 32767, but gets set to -32767 if no valid values in box */
                        if (rhoCirrus_min[ip] < 0.003 && rhoCirrus_min[ip] > (BAD_FLT + 0.1)) {
                            treesum[ip] += 0.22;
                            if (rhoCirrus_max[ip] < 0.003 && rhoCirrus_max[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.069;
                            } else {
                                treesum[ip] += -0.422;
                            }
                        } else {
                            treesum[ip] += -0.891;
                        }
                        if (dBt_37_11 < 3.577) {
                            treesum[ip] += 0.719;
                        } else {
                            treesum[ip] += -0.227;
                            if (sstq[csstbox][ip] < 27.668) {
                                treesum[ip] += -0.017;
                            } else {
                                treesum[ip] += 0.472;
                            }
                            if (sst_stdev[ip] < 0.481 && sst_stdev[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.094;
                            } else {
                                treesum[ip] += -0.322;
                            }
                        }
                        if (sst_stdev[ip] < 0.263 && sst_stdev[ip] > (BAD_FLT + 0.1)) {
                            treesum[ip] += 0.193;
                        } else {
                            treesum[ip] += -0.276;
                        }
                    } else if (l2rec->l1rec->solz[ip] < solznight) {
                        /* day in high and low glint AQUA SST tree tests */
                        if (LtRED < (1000.0 - 0.1)) {
                            /* RED is not saturated */
                            /* day in low glint AQUA SST tree test */
                            /* v1 adtree */
                            /* seadas (403,469) should be qual 1 not 3; in l2gen it's (1353-403,2029-469)=(950,1560) */
                            treesum[ip] = 0.0;
                            /* default min is 32767, but gets set to -32767 if no valid values in box */
                            if (rhotRED_min[ip] < 0.087 && rhotRED_min[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 1.017;
                            } else {
                                treesum[ip] += -0.644;
                                /* default min is 32767, but gets set to -32767 if no valid values in box */
                                if (Bt11_min[ip] < 15.22) {
                                    treesum[ip] += -0.837;
                                    if (dBt_67_11 < -40.834) {
                                        treesum[ip] += 0.510;
                                    } else {
                                        treesum[ip] += -0.181;
                                    }
                                } else {
                                    treesum[ip] += 0.870;
                                    if (Tdeflong < 0.036) {
                                        treesum[ip] += -0.859;
                                    } else {
                                        treesum[ip] += 0.215;
                                    }
                                }
                                if (l2rec->l1rec->lat[ip] < 29.675) {
                                    treesum[ip] += -0.099;
                                } else {
                                    treesum[ip] += 0.467;
                                }
                                if (dBt_37_11 < 10.053) {
                                    treesum[ip] += -0.743;
                                } else {
                                    treesum[ip] += 0.137;
                                }
                            }
                            if (rhoCirrus_max[ip] < 0.004 && rhoCirrus_max[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.227;
                                if (rhotRED[ip] < 0.068 && LtRED > (BAD_FLT + 0.1) && LtRED < (1000.0 - 0.1)) {
                                    treesum[ip] += 0.824;
                                } else {
                                    treesum[ip] += -0.113;
                                }
                                if (rhoCirrus_maxmin[ip] < 0.001 && rhoCirrus_max[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.102;
                                } else {
                                    treesum[ip] += -0.454;
                                }
                            } else {
                                treesum[ip] += -1.096;
                                if (rhoCirrus < 0.006 && rhoCirrus > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.282;
                                } else {
                                    treesum[ip] += -0.810;
                                }
                            }
                            if (sst_stdev[ip] < 0.397 && sst_stdev[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.235;
                                if (l2rec->l1rec->lat[ip] < -0.365) {
                                    treesum[ip] += 0.240;
                                } else {
                                    treesum[ip] += -0.194;
                                    if (l2rec->l1rec->lon[ip] < -67.480) {
                                        treesum[ip] += 0.241;
                                    } else {
                                        treesum[ip] += -0.229;
                                    }
                                }
                                if (sst_stdev[ip] < 0.188 && sst_stdev[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.187;
                                } else {
                                    treesum[ip] += -0.258;
                                }
                            } else {
                                treesum[ip] += -0.848;
                            }
                            if (sstq[csstbox][ip] < 27.278) {
                                treesum[ip] += -0.066;
                            } else {
                                treesum[ip] += 0.509;
                            }
                            //		    /* v2 adtree, might not be needed, try v1 first */
                            //                    /* day in glint AQUA SST tree test */
                            //		    treesum[ip] = 0.0;
                            //		    if (rhotRED_min[ip] < 15.119 && rhotRED_min[ip] > 0.0) {
                            //			treesum[ip] += -0.759;
                            //		    } else {
                            //			treesum[ip] += 0.794;
                            //		    }
                            //		    if (sst_stdev[ip] < 0.254) {
                            //			treesum[ip] += 0.304;
                            //			if (sst_stdev[ip] < 0.14) {
                            //			    treesum[ip] += 0.237;
                            //			} else {
                            //			    treesum[ip] += -0.276;
                            //			}
                            //			if (l2rec->l1rec->lat[ip] < -4.225){
                            //			    treesum[ip] += 0.401;
                            //			} else {
                            //			    treesum[ip] += -0.144;
                            //			}
                            //		    } else {
                            //			treesum[ip] += -0.508;
                            //		    }
                            //		    if (dBt_11_12 < 0.303) {
                            //			treesum[ip] += -0.715;
                            //		    } else {
                            //			treesum[ip] += 0.184;
                            //			if (l2rec->l1rec->lat[ip] < -23.905) {
                            //			    treesum[ip] += 0.662;
                            //			} else {
                            //			    treesum[ip] += -0.087;
                            //			}
                            //			if (sst_stdev[ip] < 0.494) {
                            //			    treesum[ip] += 0.114;
                            //			} else {
                            //			    treesum[ip] += -0.550;
                            //			}
                            //		    }
                            //		    if (l2rec->l1rec->lat[ip] < 32.415) {
                            //			treesum[ip] += -0.137;
                            //			if (sstq[csstbox][ip] < 26.408) {
                            //			    treesum[ip] += -0.245;
                            //			} else {
                            //			    treesum[ip] += 0.499;
                            //			}
                            //			if (Tdeflong < 0.038) {
                            //			    treesum[ip] += -0.415;
                            //			} else {
                            //			    treesum[ip] += 0.038;
                            //			}
                            //		    } else {
                            //			treesum[ip] += 0.506;
                            //		    }
                        } else {
                            /* RED is saturated */
                            /* day high glint aqua sst tree test */
                            treesum[ip] = 0.0;
                            /* default min is 32767, but gets set to -32767 if no valid values in box */
                            /* if min is bad this will go negative without a specific test */
                            if (Bt12_min[ip] < 15.119) {
                                treesum[ip] += -0.759;
                                if (dBt_37_11 < 12.733) {
                                    treesum[ip] += 0.652;
                                    /* default min is 32767, but gets set to -32767 if no valid values in box */
                                    /* if min is bad this will go negative without a specific test */
                                    if (rhoCirrus_min[ip] < 0.001) {
                                        treesum[ip] += -1.096;
                                    } else {
                                        treesum[ip] += 0.130;
                                    }
                                } else {
                                    treesum[ip] += -0.288;
                                }
                                if (Bt37_maxmin[ip] < 0.529 && Bt37_maxmin[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.926;
                                } else {
                                    treesum[ip] += 0.026;
                                }
                            } else {
                                treesum[ip] += 0.794;
                                if (dBt_37_11 < 8.37) {
                                    treesum[ip] += 0.522;
                                } else {
                                    treesum[ip] += -0.268;
                                }
                            }
                            if (rhoCirrus_maxmin[ip] < 0.001 && rhoCirrus_max[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.381;
                                if (rhoCirrus_max[ip] < 0.002 && rhoCirrus_max[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.006;
                                } else {
                                    treesum[ip] += -0.584;
                                }
                            } else {
                                treesum[ip] += -0.619;
                            }
                            if (sst_stdev[ip] < 0.397 && sst_stdev[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.101;
                            } else {
                                treesum[ip] += -0.684;
                            }
                            if (Tdeflong < 0.036) {
                                treesum[ip] += -0.471;
                                if (l2rec->l1rec->lat[ip] < 32.335) {
                                    treesum[ip] += -0.481;
                                } else {
                                    treesum[ip] += 0.684;
                                }
                            } else {
                                treesum[ip] += 0.193;
                            }
                            /* default min is 32767, but gets set to -32767 if no valid values in box */
                            if (rhoCirrus_min[ip] < 0.003 && rhoCirrus_min[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.108;
                                if (l2rec->l1rec->lat[ip] < -25.425) {
                                    treesum[ip] += 0.517;
                                } else {
                                    treesum[ip] += -0.082;
                                    if (l2rec->l1rec->lat[ip] < 27.215) {
                                        treesum[ip] += -0.077;
                                        if (Bt11 < 14.734) {
                                            treesum[ip] += -2.012;
                                        } else {
                                            treesum[ip] += 0.089;
                                        }
                                    } else {
                                        treesum[ip] += 0.383;
                                    }
                                }
                            } else {
                                treesum[ip] += -0.913;
                            }
                            if (sstq[csstbox][ip] < 27.583) {
                                treesum[ip] += -0.074;
                            } else {
                                treesum[ip] += 0.691;
                            }
                        }
                    } else {
                        /* night AQUA SST tree test */
                        /* use tree that starts with 0, not -0.029 */
                        treesum[ip] = 0.0;
                        if (dBt_37_12 < 0.117) {
                            treesum[ip] += -1.279;
                            if (l2rec->l1rec->lat[ip] < 33.035) {
                                treesum[ip] += -0.289;
                                if (l2rec->l1rec->lat[ip] < -42.235) {
                                    treesum[ip] += 0.747;
                                } else {
                                    treesum[ip] += -0.564;
                                    if (Bt37 < 13.117) {
                                        treesum[ip] += -0.580;
                                    } else {
                                        treesum[ip] += 0.638;
                                    }
                                }
                            } else {
                                treesum[ip] += 0.355;
                            }
                            if (Bt37 < 9.447) {
                                treesum[ip] += -0.307;
                            } else {
                                treesum[ip] += 0.747;
                            }
                        } else {
                            treesum[ip] += 0.470;
                            if (Bt11_stdev[ip] < 0.155 && Bt11_stdev[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.690;
                                if (Bt37_maxmin[ip] < 0.524 && Bt37_maxmin[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.150;
                                } else {
                                    treesum[ip] += -0.794;
                                }
                            } else {
                                treesum[ip] += -0.430;
                            }
                            if (dSST_SST4 < -0.787) {
                                treesum[ip] += -1.404;
                                if (dSST_SST4 < -1.253) {
                                    treesum[ip] += -1.086;
                                } else {
                                    treesum[ip] += 0.388;
                                }
                            } else {
                                treesum[ip] += 0.197;
                                if (Bt37_maxmin[ip] < 1.229 && Bt37_maxmin[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.287;
                                    if (dSST_SST4 < -0.383) {
                                        treesum[ip] += -0.531;
                                    } else {
                                        treesum[ip] += 0.279;
                                    }
                                } else {
                                    treesum[ip] += -0.497;
                                }
                                if (Bt12 < 16.353) {
                                    treesum[ip] += -0.300;
                                    if (dBt_40_11 < -2.171) {
                                        treesum[ip] += 0.516;
                                    } else {
                                        treesum[ip] += -0.395;
                                        if (sst4q[csstbox][ip] < 16.212) {
                                            treesum[ip] += -0.694;
                                        } else {
                                            treesum[ip] += 0.392;
                                        }
                                    }
                                } else {
                                    treesum[ip] += 0.451;
                                }
                            }
                            // maybe add check to make sure Bt85 is valid? if ((Bt85 +/- 1000)
                            if (dBt_85_11 < -1.577) {
                                treesum[ip] += 0.088;
                            } else {
                                treesum[ip] += -0.408;
                            }
                        }
                    }
                    if (treesum[ip] <= 0.0) {
                        flags_sst[ip] |= SSTF_CLOUD;
                    }
                }/* end case for modis */

                else if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
                    /* if we run this more than once I'll add a switch, but I think it's just here to make a 'previous' picture */
                    v5viirs = 0;
                    /* v5 viirs */
                    if (v5viirs == 1) {
                        if (l2rec->l1rec->solz[ip] < solznight) {
                            /* day VIIRS SST tree test */
                            /* v5.33 day tree */

                            /* sd6.4 version used class files that contained values in K */
                            /* sd7.4 version uses gsfc files with values in C */
                            if (dBt_11_12 < 0.23435 ||
                                    Bt12 < (270.78 - CtoK)) {
                                flags_sst[ip] |= SSTF_CLOUD;
                            } else if (Bt37_maxmin[ip] >= 0.40919 &&
                                    (Bt37_maxmin[ip] >= 1.0765 ||
                                    Bt12_maxmin[ip] >= 0.8412 ||
                                    Bt12_maxmin[ip] < 0.3193)) {
                                flags_sst[ip] |= SSTF_CLOUD;
                            }
                        } else {
                            /* night VIIRS SST tree test */
                            /* v5.33 night tree */
                            if (dBt_37_11 < 0.18654) {
                                flags_sst[ip] |= SSTF_CLOUD;
                            } else if (dSST_SST3 < -1.021674) {
                                if (dBt_11_12 < 2.187215 || dBt_37_11 >= 5.46243) {
                                    flags_sst[ip] |= SSTF_CLOUD;
                                }
                            } else if (Bt37_maxmin[ip] >= 0.48349) {
                                if (dBt_11_12 < 0.32251 || dSST_SST3 >= 0.4414579) {
                                    flags_sst[ip] |= SSTF_CLOUD;
                                } else if (Bt37_maxmin[ip] >= 1.012265) {
                                    if (dBt_11_12 < 0.951295 || Bt12_maxmin[ip] < 0.69827) {
                                        flags_sst[ip] |= SSTF_CLOUD;
                                    }
                                } else {
                                    if (l2rec->l1rec->senz[ip] > 56.3) {
                                        if (dBt_37_11 < 1.101845 || dBt_11_12 >= 1.308475) {
                                            flags_sst[ip] |= SSTF_CLOUD;
                                        }
                                    }
                                }
                            }
                        }
                    } else {

                        /* v6 viirs tree */
                        if (l2rec->l1rec->solz[ip] >= solznight) {
                            /* night VIIRS v6 SST tree test */
                            treesum[ip] = 0.413;
                            if (dBt_37_11 < 0.115) {
                                treesum[ip] += -1.931;
                            } else {
                                treesum[ip] += 0.496;
                                if (dSST_SST3 < -0.465) {
                                    treesum[ip] += -0.942;
                                    if (dSST_SST3 < -1.102) {
                                        treesum[ip] += -0.735;
                                        if (dBt_37_11 < 1.686) {
                                            treesum[ip] += 0.651;
                                        } else {
                                            treesum[ip] += -0.82;
                                        }
                                    } else {
                                        treesum[ip] += 0.207;
                                    }
                                } else {
                                    treesum[ip] += 0.309;
                                    if (dSST_SST3 < 0.78) {
                                        treesum[ip] += 0.043;
                                        if (dSST_SST3 < -0.158) {
                                            treesum[ip] += -0.392;
                                        } else {
                                            treesum[ip] += 0.194;
                                        }
                                    } else {
                                        treesum[ip] += -1.069;
                                    }
                                    if (dBt_37_11 < 0.306) {
                                        treesum[ip] += -0.692;
                                    } else {
                                        treesum[ip] += 0.077;
                                    }
                                }
                                if (Bt12 < 287.407) {
                                    treesum[ip] += -0.339;
                                } else {
                                    treesum[ip] += 0.426;
                                }
                            }
                            if (sstq[csstbox][ip] < (270.414 - CtoK)) {
                                treesum[ip] += -3.879;
                            } else {
                                treesum[ip] += 0.047;
                                if (Bt37_stdev[ip] < 0.247 && Bt37_stdev[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.176;
                                } else {
                                    treesum[ip] += -0.187;
                                }
                            }
                            if (treesum[ip] <= 0.0) {
                                /* set bit to show failed tree test */
                                flags_sst[ip] |= SSTF_CLOUD;
                            }
                            /* end case for v6 viirs night */
                        } else if (l2rec->l1rec->glint_coef[ip] <= glintmax) {
                            /* day not glint VIIRS v6 SST tree test */
                            treesum[ip] = 0.0;

                            /* add ice test here for now.  May want to add it to set_qual instead of setting cloud flag */
                            /* only for viirs, day, not glint */
                            // check for bad rhotred? if (rhotRED[ip] > 0.5 && LtRED > (BAD_FLT+0.1) && LtRED < (1000.0-0.1) &&

                            /* instead of lat > 40, calc sub solar point and +/- 30 degrees to get lat bounds that vary by season */
                            /* units of x=radians=(deg/day)*day/(deg/rad)
                             * subsolar is negative in the southern hemisphere
                             */
                            xdoy = day + 284.0;
                            xrad = (360.0 / 365.0) * xdoy / RADEG;
                            subsolar = 23.45 * sin(xrad);

                            if (rhotRED[ip] > 0.3 &&
                                    rhot16[ip] >= 0.006 && rhot16[ip] < 0.1 &&
                                    ((l2rec->l1rec->lat[ip] > (subsolar + 30.0)) || (l2rec->l1rec->lat[ip] < (subsolar - 30.0)))) {
                                /* set cloud bit to show ice (or wispy clouds?) */
                                flags_sst[ip] |= SSTF_CLOUD;
                                /* don't really need to do the tree test now, but it doesn't hurt */
                            }

                            /* rho's are bad when Lt's are -32767 or 0.0 (make rho <=0) or 1000 (make rho large) */
                            // don't adjust for different f0 in Kays R code was 1.0 (from sd6.4) should have been 25.084 (what it is in sd7)
                            /* seadas (1907,3148) should be qual 0 not 3; in l2gen it's (3199-1907,3231-3148)=(1292,83) */
                            /* seadas (1935,3147) should be qual 0 not 0; in l2gen it's (3199-1935,3231-3147)=(1264,84) */
                            if (rhot16[ip] < 0.16 && Lt16 > (BAD_FLT + 0.1) && Lt16 < (1000.0 - 0.1)) {
                                treesum[ip] += 0.805;
                                if (rhotNIR7[ip] < 0.062 && LtNIR7 > (BAD_FLT + 0.1) && LtNIR7 < (1000.0 - 0.1)) {
                                    treesum[ip] += 0.393;
                                    if (rhoCirrus < 0.004 && rhoCirrus > (BAD_FLT + 0.1)) {
                                        treesum[ip] += 0.287;
                                        if (Tdeflong < 0.002) {
                                            treesum[ip] += -0.681;
                                        } else {
                                            treesum[ip] += 0.026;
                                            if (rhotNIR7[ip] < 0.039 && LtNIR7 > (BAD_FLT + 0.1) && LtNIR7 < (1000.0 - 0.1)) {
                                                treesum[ip] += 0.364;
                                            } else {
                                                treesum[ip] += -0.21;
                                            }
                                        }
                                    } else {
                                        treesum[ip] += -1.244;
                                    }
                                } else {
                                    treesum[ip] += -0.0572;
                                    /* default min is 32767, but gets set to -32767 if no valid values in box */
                                    if (rhot16_min[ip] < 0.032 && rhot16_min[ip] > (BAD_FLT + 0.1)) {
                                        treesum[ip] += 0.455;
                                    } else {
                                        treesum[ip] += -0.395;
                                    }
                                }
                                if (l2rec->l1rec->senz[ip] < 64.994) {
                                    treesum[ip] += 0.216;
                                    if (rhoCirrus < 0.007 && rhoCirrus > (BAD_FLT + 0.1)) {
                                        treesum[ip] += 0.065;
                                    } else {
                                        treesum[ip] += -1.077;
                                    }
                                } else {
                                    treesum[ip] += -0.708;
                                }
                            } else {
                                treesum[ip] += -1.755;
                                if (rhot16[ip] < 0.266 && Lt16 > (BAD_FLT + 0.1) && Lt16 < (1000.0 - 0.1)) {
                                    treesum[ip] += 0.642;
                                } else {
                                    treesum[ip] += -0.19;
                                    if (rhotRED_maxmin[ip] < 0.103 && rhotRED_maxmin[ip] > (BAD_FLT + 0.1)) {
                                        treesum[ip] += 0.425;
                                    } else {
                                        treesum[ip] += -0.195;
                                    }
                                }
                                if (dBt_11_12 < 0.235) {
                                    treesum[ip] += -0.189;
                                } else {
                                    treesum[ip] += 0.411;
                                }
                                if (l2rec->l1rec->wv[ip] < 2.946) {
                                    treesum[ip] += 0.038;
                                } else {
                                    treesum[ip] += -1.137;
                                }
                            }
                            if (Bt11_maxmin[ip] < 0.762 && Bt11_maxmin[ip] > (BAD_FLT + 0.1)) {
                                treesum[ip] += 0.156;
                            } else {
                                treesum[ip] += -0.188;
                            }
                            if (l2rec->l1rec->wv[ip] < 1.315) {
                                treesum[ip] += 0.327;
                            } else {
                                treesum[ip] += -0.054;
                                if (sstq[csstbox][ip] < (278.171 - CtoK)) {
                                    treesum[ip] += -0.679;
                                } else {
                                    treesum[ip] += 0.05;
                                }
                            }
                        } else {
                            /* day glint VIIRS v6 SST tree test */
                            treesum[ip] = 0.0;
                            /* seadas (1907,3148) should be qual 0 not 3; in l2gen it's (3199-1907,3231-3148)=(1292,83) */
                            /* seadas (1935,3147) should be qual 0 not 0; in l2gen it's (3199-1935,3231-3147)=(1264,84) */
                            if (rhotRED[ip] > 0.065) {
                                /* high glint tree */
                                treesum[ip] = 0.0;
                                /* default min is 32767, but gets set to -32767 if no valid values in box */
                                if (Bt85_min[ip] < (287.451 - CtoK)) {
                                    treesum[ip] += -0.812;
                                    if (l2rec->l1rec->lat[ip] < 32.315) {
                                        treesum[ip] += -0.296;
                                        if (Tdeflong < 0.001) {
                                            treesum[ip] += -1.109;
                                            if (l2rec->l1rec->lat[ip] < -30.0) {
                                                treesum[ip] += 0.525;
                                            } else {
                                                treesum[ip] += -1.827;
                                            }
                                        } else {
                                            treesum[ip] += 0.44;
                                        }
                                        if (l2rec->l1rec->wv[ip] < 2.065) {
                                            treesum[ip] += 0.49;
                                        } else {
                                            treesum[ip] += -1.104;
                                            if (Bt85 < (287.059 - CtoK)) {
                                                treesum[ip] += -1.071;
                                            } else {
                                                treesum[ip] += 0.727;
                                            }
                                        }
                                    } else {
                                        treesum[ip] += 0.669;
                                    }
                                    if (dBt_37_11 < 17.594) {
                                        treesum[ip] += 0.452;
                                    } else {
                                        treesum[ip] += -0.76;
                                    }
                                } else {
                                    treesum[ip] += 0.858;
                                    if (l2rec->l1rec->lon[ip] < -69.475) {
                                        treesum[ip] += 0.512;
                                    } else {
                                        treesum[ip] += -0.176;
                                        if (l2rec->l1rec->lat[ip] < 1.01) {
                                            treesum[ip] += 0.64;
                                        } else {
                                            treesum[ip] += -0.176;
                                            if (tmonth < 6) {
                                                treesum[ip] += 0.69;
                                            } else {
                                                treesum[ip] += -0.305;
                                            }
                                        }
                                    }
                                    if (dBt_37_11 < 9.655) {
                                        treesum[ip] += 0.562;
                                    } else {
                                        treesum[ip] += -0.173;
                                    }
                                    if (Bt85 < (290.343 - CtoK)) {
                                        treesum[ip] += -0.39;
                                    } else {
                                        treesum[ip] += 0.243;
                                        if (Tdeflong < 0.003) {
                                            treesum[ip] += -0.817;
                                        } else {
                                            treesum[ip] += 0.267;
                                        }
                                    }
                                    if (l2rec->l1rec->lat[ip] < 22.465) {
                                        treesum[ip] += -0.206;
                                    } else {
                                        treesum[ip] += 0.523;
                                    }
                                }
                                if (Bt11_maxmin[ip] < 1.189 && Bt11_maxmin[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.059;
                                } else {
                                    treesum[ip] += -1.22;
                                }
                            } else {
                                /* low glint tree */
                                treesum[ip] = 0.0;
                                /* default min is 32767, but gets set to -32767 if no valid values in box */
                                if (rhotNIR7_min[ip] < 0.104 && rhotNIR7_min[ip] > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.91;
                                    if (rhotRED[ip] < 0.086 && LtRED > (BAD_FLT + 0.1) && LtRED < (1000.0 - 0.1)) {
                                        treesum[ip] += 0.518;
                                        /* default min is 32767, but gets set to -32767 if no valid values in box */
                                        if (rhotRED_min[ip] < 0.067 && rhotRED_min[ip] > (BAD_FLT + 0.1)) {
                                            treesum[ip] += 0.558;
                                        } else {
                                            treesum[ip] += -0.263;
                                            /* don't check for > BAD_FLT here because we want the negative treesum if Lt < 0.0 (or =1000.0) */
                                            if (rhot16[ip] < 0.06 || Lt16 > (1000.0 - 0.1)) {
                                                treesum[ip] += -0.231;
                                            } else {
                                                treesum[ip] += 1.712;
                                            }
                                            /* don't check for > BAD_FLT here because we want the negative treesum if Lt < 0.0 (or =1000.0) */
                                            if (rhot16[ip] < 0.046 || Lt16 > (1000.0 - 0.1)) {
                                                treesum[ip] += -1.353;
                                            } else {
                                                treesum[ip] += 0.352;
                                            }
                                        }
                                    } else {
                                        treesum[ip] += -0.585;
                                        if (dBt_37_12 < 12.951) {
                                            treesum[ip] += -0.905;
                                        } else {
                                            treesum[ip] += 0.187;
                                            if (rhotRED[ip] < 0.098 && LtRED > (BAD_FLT + 0.1) && LtRED < (1000.0 - 0.1)) {
                                                treesum[ip] += 0.549;
                                            } else {
                                                treesum[ip] += -0.484;
                                            }
                                        }
                                    }
                                } else {
                                    treesum[ip] += -1.819;
                                    /* default min is 32767, but gets set to -32767 if no valid values in box */
                                    if (rhotRED_min[ip] < 0.206 && rhotRED_min[ip] > (BAD_FLT + 0.1)) {
                                        treesum[ip] += 0.467;
                                    } else {
                                        treesum[ip] += -1.18;
                                        if (rhotRED_maxmin[ip] < 0.037 && rhotRED_maxmin[ip] > (BAD_FLT + 0.1)) {
                                            treesum[ip] += 1.747;
                                        } else {
                                            treesum[ip] += -1.79;
                                        }
                                    }
                                    if (l2rec->l1rec->wv[ip] < 1.705) {
                                        treesum[ip] += 0.434;
                                    } else {
                                        treesum[ip] += -0.645;
                                    }
                                }
                                if (rhoCirrus < 0.002 && rhoCirrus > (BAD_FLT + 0.1)) {
                                    treesum[ip] += 0.23;
                                    if (l2rec->l1rec->lat[ip] < 32.13) {
                                        treesum[ip] += -0.067;
                                        if (sstq[csstbox][ip] < (300.034 - CtoK)) {
                                            treesum[ip] += -0.146;
                                        } else {
                                            treesum[ip] += 0.824;
                                        }
                                    } else {
                                        treesum[ip] += 0.758;
                                    }
                                } else {
                                    treesum[ip] += -1.153;
                                    /* default min is 32767, but gets set to -32767 if no valid values in box */
                                    if (rhoCirrus_min[ip] < 0.005 && rhoCirrus_min[ip] > (BAD_FLT + 0.1)) {
                                        treesum[ip] += 0.28;
                                    } else {
                                        treesum[ip] += -0.939;
                                    }
                                }
                                if (l2rec->l1rec->senz[ip] < 33.204) {
                                    treesum[ip] += 0.2;
                                } else {
                                    treesum[ip] += -0.331;
                                }
                            }
                        }
                        if (treesum[ip] <= 0.0) {
                            flags_sst[ip] |= SSTF_CLOUD;
                        }

                    } /* end case for viirs v5 vs v6 */

                }/* end case for viirs */
                break;
        } /* end switch statement for tree tests */

        if (l2rec->l1rec->l1file->sensorID == AVHRR) {
            /* Check stray sunlight test, M2B2 (shared bit with SST4DIFF) */
            /* this WILL now work if working on a subset of the data (spixl or epixl were specified) */

            if (l2rec->l1rec->lat[ip] < 0.0
                    && ((ASCEND == 1 && l2rec->l1rec->pixnum[ip] <= cpix)
                    || (ASCEND == 0 && l2rec->l1rec->pixnum[ip] > cpix))
                    && l2rec->l1rec->senz[ip] > 45.00) {
                flags_sst[ip] |= SSTF_SUNLIGHT;
            }
        }

    } /* End of pixel loop */
    return;
}

/* ----------------------------------------------------------------------------------- */
/* set_flags_sst4() - set quality flags for short wave sea surface temperature         */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
void set_flags_sst4(l2str *l2rec) {

    extern l1qstr l1que;
    int32_t npix = l2rec->l1rec->npix;
    int32_t ip, ipbir;
    float Bt37, Bt39, Bt40, Bt85;
    float Bt11, Bt12;
    float dBt_37_12, dBt_40_11, dBt_85_11;
    float dBt_34;
    float dSST4_ref; /* sst4 - ref */
    float dSST_SST4; /* sst - sst4 */
    statstr statrec;

    /* check each pixel in scan */
    for (ip = 0; ip < npix; ip++) {

        d3940ref[ip] = sstbad;

        /* SST not processed */
        if (sstmasked(l2rec->l1rec->flags, ip)) {
            flags_sst4[ip] |= SSTF_ISMASKED;
            continue;
        }

        ipbir = ip * NBANDSIR;

        Bt37 = l2rec->l1rec->Bt[ipbir + ib37]; /* modis chan 20, avhrr cen3, viirs m12 */
        Bt39 = l2rec->l1rec->Bt[ipbir + ib39]; /* modis chan 22                        */
        Bt40 = l2rec->l1rec->Bt[ipbir + ib40]; /* modis chan 23,             viirs m13 */
        Bt85 = l2rec->l1rec->Bt[ipbir + ib85]; /* modis chan 29,             viirs m14 */
        Bt11 = l2rec->l1rec->Bt[ipbir + ib11]; /* modis chan 31, avhrr cen4, viirs m15 */
        Bt12 = l2rec->l1rec->Bt[ipbir + ib12]; /* modis chan 32, avhrr cen5, viirs m16 */

        /* aqua detector 0 channel 23 has a problem so average detectors 9 and 1 instead */
        if ((l2rec->l1rec->l1file->sensorID == MODISA)
                && l2rec->l1rec->detnum == 0) {
            /* current row is center of box */
            int32_t is = l1que.nq / 2;
            if (btavg(ip, is, ib40, NBANDSIR, &statrec) > 0) {
                Bt40 = statrec.avg;
            }
        }
        dBt_37_12 = Bt37 - Bt12;
        dBt_40_11 = Bt40 - Bt11;
        dBt_85_11 = Bt85 - Bt11;

        /* if BT could not be computed (radiance out-of-range) */
        if (Bt39 < BT_LO + 0.1 || Bt39 > BT_HI - 0.1 || Bt40 < BT_LO + 0.1
                || Bt40 > BT_HI - 0.1) {
            flags_sst4[ip] |= SSTF_BTBAD;
            continue;
        }

        /* check BT range (don't care about Bt37 for sst4) */
        if (Bt39 < Btmin || Bt39 > Btmax || Bt40 < Btmin || Bt40 > Btmax40)
            flags_sst4[ip] |= SSTF_BTRANGE;

        /* check BT diff */
        dBt_34 = Bt39 - Bt40;
        if (dBt_34 < dBt4min || dBt_34 > dBt4max)
            flags_sst4[ip] |= SSTF_BTDIFF;

        /* check BT diff against BT reference */

        /* pathfinder v6 */
        d3940ref[ip] = btrefdiffv6(ip, Bt39, Bt40, l2rec);

        if (d3940ref[ip] < dBtrefmin || d3940ref[ip] > dBtrefmax)
            flags_sst4[ip] |= SSTF_BT4REFDIFF;

        /* check SST range */
        if (sst4q[csstbox][ip] < SSTmin || sst4q[csstbox][ip] > SSTmaxn)
            flags_sst4[ip] |= SSTF_SSTRANGE;

        /* check SST difference with reference */
        if (sst4q[csstbox][ip] < sstbad + 1.0 || l2rec->l1rec->sstref[ip] < sstbad + 1.0) {
            dSST4_ref = sstbad;
        } else {
            dSST4_ref = sst4q[csstbox][ip] - l2rec->l1rec->sstref[ip];
        }
        // the "coldonly" and equatorial aerosol tests are to be run by default, but if SSTMODS is set, don't
        if ((evalmask & SSTMODS) == 0) {
            /* evaluate change to cold-test only */
            /* set the flag bit if sst4 is too much colder than reference */
            if (dSST4_ref < -SSTdiff || l2rec->l1rec->sstref[ip] < sstbad + 1.0)
                flags_sst4[ip] |= SSTF_SSTREFDIFF;
            if (dSST4_ref < -input->sstrefdif &&
                    l2rec->l1rec->lat[ip] >= equatorialSouth && l2rec->l1rec->lat[ip] <= equatorialNorth &&
                    l2rec->l1rec->lon[ip] >= equatorialWest && l2rec->l1rec->lon[ip] <= equatorialEast) {
                /* tighter test for avhrr between 10S and 30N and -105 to 105 longitude */
                /* equatorial aerosol test */
                flags_sst4[ip] |= SSTF_SSTREFDIFF;
            }
            /* set the flag bit if sst4 is too much warmer than reference at night */
            if (l2rec->l1rec->solz[ip] >= solznight) {
                if (fabs(dSST4_ref) > SSTdiff)
                    flags_sst4[ip] |= SSTF_SSTREFDIFF;
                if (fabs(dSST4_ref) > SSTvdiff)
                    flags_sst4[ip] |= SSTF_SSTREFVDIFF;
            }
            /* is sst4 way too much colder than reference? */
            if (dSST4_ref < -SSTvdiff || l2rec->l1rec->sstref[ip] < sstbad + 1.0)
                flags_sst4[ip] |= SSTF_SSTREFVDIFF;
        } else {
            if (l2rec->l1rec->solz[ip] >= SOLZNIGHT) {
                if (fabs(dSST4_ref) > SSTdiff)
                    flags_sst4[ip] |= SSTF_SSTREFDIFF;
            } else {
                if (dSST4_ref < -SSTdiff || dSST4_ref > (SSTdiff + 1))
                    flags_sst4[ip] |= SSTF_SSTREFDIFF;
            }
            if (fabs(dSST4_ref) > SSTvdiff)
                flags_sst4[ip] |= SSTF_SSTREFVDIFF;
        }
        /* check SST4 difference with 11um SST */
        dSST_SST4 = sstq[csstbox][ip] - sst4q[csstbox][ip];
        if (sstq[csstbox][ip] > sstbad + 1.0) {
            if (fabs(dSST_SST4) < SST4diff1)
                flags_sst4[ip] |= SSTF_SST4DIFF;
            if (fabs(dSST_SST4) < SST4diff2)
                flags_sst4[ip] |= SSTF_SST4VDIFF;
        }

        /* check sensor zenith limits */
        if (l2rec->l1rec->senz[ip] > hisenz)
            flags_sst4[ip] |= SSTF_HISENZ;
        if (l2rec->l1rec->senz[ip] > vhisenz)
            flags_sst4[ip] |= SSTF_VHISENZ;
        // flag 2 edge pixels as SSTF_VHISENZ so quality gets set to 3
        if (l2rec->l1rec->pixnum[ip] < 2 || l2rec->l1rec->pixnum[ip] > (fullscanpix - 3))
            flags_sst4[ip] |= SSTF_VHISENZ;
        // set the last 4 pixels of the scan for Terra to VHISENZ 
        // as there is an apparent calibration issue with the BT12 for those pixels
        if ((l2rec->l1rec->l1file->sensorID == MODIST) && (l2rec->l1rec->pixnum[ip] > 1349))
            flags_sst4[ip] |= SSTF_VHISENZ;

        //        /* if 11um SST is cold, check for clouds (collect 5) */
        //	/* sstcloud checks to make sure it's day and not glint */
        //        if (sstq[csstbox][ip] > sstbad+1.0 && l2rec->l1rec->sstref[ip] > sstbad+1.0 &&
        //	      sstq[csstbox][ip]-l2rec->l1rec->sstref[ip] <= -1.0)
        //	    if (sstcloud(ip,cldbox,cldbox,cldthresh) == 1)
        //	        flags_sst4[ip] |= SSTF_REDNONUNIF;

        /* check homogeneity of BT */

        if (Bt39_maxmin[ip] > Bt39unif1)
            flags_sst4[ip] |= SSTF_BTNONUNIF;
        if (Bt39_maxmin[ip] > Bt39unif2)
            flags_sst4[ip] |= SSTF_BTVNONUNIF;

        if (Bt40_maxmin[ip] > Bt40unif1)
            flags_sst4[ip] |= SSTF_BTNONUNIF;
        if (Bt40_maxmin[ip] > Bt40unif2)
            flags_sst4[ip] |= SSTF_BTVNONUNIF;
        /* end of homogeneity checks */

        if (l2rec->l1rec->solz[ip] >= solznight) {
            if (l2rec->l1rec->l1file->sensorID == MODIST) {
                /* night TERRA SST tree test */
                treesum[ip] = 0.0;
                if (dBt_37_12 < -0.053) {
                    treesum[ip] += -1.257;
                    if (l2rec->l1rec->lat[ip] < 33.195) {
                        treesum[ip] += -0.278;
                        if (l2rec->l1rec->lat[ip] < -40.185) {
                            treesum[ip] += 0.619;
                        } else {
                            treesum[ip] += -0.711;
                            if (Bt37 < 6.477) {
                                treesum[ip] += -3.733;
                            } else {
                                treesum[ip] += -0.111;
                            }
                        }
                    } else {
                        treesum[ip] += 0.333;
                    }
                    if (Bt37 < 9.372) {
                        treesum[ip] += -0.292;
                    } else {
                        treesum[ip] += 0.764;
                    }
                } else {
                    treesum[ip] += 0.430;
                    if (Bt11_maxmin[ip] < 0.486 && Bt11_maxmin[ip] > (BAD_FLT + 0.1)) {
                        treesum[ip] += 0.628;
                        if (Bt40_stdev[ip] < 0.146) {
                            treesum[ip] += 0.177;
                        } else {
                            treesum[ip] += -0.723;
                        }
                    } else {
                        treesum[ip] += -0.450;
                    }
                    if (dSST_SST4 < -0.878) {
                        treesum[ip] += -1.353;
                        if (dSST_SST4 < -1.533) {
                            treesum[ip] += -1.439;
                        } else {
                            treesum[ip] += 0.346;
                        }
                    } else {
                        treesum[ip] += 0.219;
                        if (Bt37_stdev[ip] < 0.448) {
                            treesum[ip] += 0.290;
                            if (dSST_SST4 < -0.422) {
                                treesum[ip] += -0.504;
                            } else {
                                treesum[ip] += 0.268;
                            }
                        } else {
                            treesum[ip] += -0.484;
                        }
                        if (Bt12 < 16.736) {
                            treesum[ip] += -0.285;
                            if (dBt_40_11 < -2.199) {
                                treesum[ip] += 0.518;
                            } else {
                                treesum[ip] += -0.316;
                                if (Bt12 < 11.896) {
                                    treesum[ip] += -0.527;
                                } else {
                                    treesum[ip] += 0.400;
                                }
                            }
                        } else {
                            treesum[ip] += 0.500;
                        }
                        if (dSST_SST4 < 1.183) {
                            treesum[ip] += 0.051;
                        } else {
                            treesum[ip] += -0.898;
                        }
                    }
                }

            } else if (l2rec->l1rec->l1file->sensorID == MODISA) {
                /* MODIS AQUA SST4 Night decision Tree test */
                /* night AQUA SST tree test */
                /* use tree that starts with 0, not -0.029 */
                treesum[ip] += 0.0;
                if (dBt_37_12 < 0.117) {
                    treesum[ip] += -1.279;
                    if (l2rec->l1rec->lat[ip] < 33.035) {
                        treesum[ip] += -0.289;
                        if (l2rec->l1rec->lat[ip] < -42.235) {
                            treesum[ip] += 0.747;
                        } else {
                            treesum[ip] += -0.564;
                            if (Bt37 < 13.117) {
                                treesum[ip] += -0.580;
                            } else {
                                treesum[ip] += 0.638;
                            }
                        }
                    } else {
                        treesum[ip] += 0.355;
                    }
                    if (Bt37 < 9.447) {
                        treesum[ip] += -0.307;
                    } else {
                        treesum[ip] += 0.747;
                    }
                } else {
                    treesum[ip] += 0.470;
                    if (Bt11_stdev[ip] < 0.155) {
                        treesum[ip] += 0.690;
                        if (Bt37_maxmin[ip] < 0.524 && Bt37_maxmin[ip] > (BAD_FLT + 0.1)) {
                            treesum[ip] += 0.150;
                        } else {
                            treesum[ip] += -0.794;
                        }
                    } else {
                        treesum[ip] += -0.430;
                    }
                    if (dSST_SST4 < -0.787) {
                        treesum[ip] += -1.404;
                        if (dSST_SST4 < -1.253) {
                            treesum[ip] += -1.086;
                        } else {
                            treesum[ip] += 0.388;
                        }
                    } else {
                        treesum[ip] += 0.197;
                        if (Bt37_maxmin[ip] < 1.229 && Bt37_maxmin[ip] > (BAD_FLT + 0.1)) {
                            treesum[ip] += 0.287;
                            if (dSST_SST4 < -0.383) {
                                treesum[ip] += -0.531;
                            } else {
                                treesum[ip] += 0.279;
                            }
                        } else {
                            treesum[ip] += -0.497;
                        }
                        if (Bt12 < 16.353) {
                            treesum[ip] += -0.300;
                            if (dBt_40_11 < -2.171) {
                                treesum[ip] += 0.516;
                            } else {
                                treesum[ip] += -0.395;
                                if (sst4q[csstbox][ip] < 16.212) {
                                    treesum[ip] += -0.694;
                                } else {
                                    treesum[ip] += 0.392;
                                }
                            }
                        } else {
                            treesum[ip] += 0.451;
                        }
                    }
                    // maybe add check to make sure Bt85 is valid? if ((Bt85 +/- 1000)
                    if (dBt_85_11 < -1.577) {
                        treesum[ip] += 0.088;
                    } else {
                        treesum[ip] += -0.408;
                    }
                }
            }
            if (treesum[ip] <= 0.0) {
                flags_sst4[ip] |= SSTF_CLOUD;
            }
        } /* end case for sst4 */
    } /* end pixel loop */
    return;
}

/* ----------------------------------------------------------------------------------- */
/* set_qual_sst() - set quality levels for long wave sea surface temperature           */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
void set_qual_sst(l2str *l2rec) {
    int32_t ip, ipb;
    float rhot;

    for (ip = 0; ip < l2rec->l1rec->npix; ip++) {

        if (l2rec->l1rec->l1file->sensorID == AVHRR) {
            if ((flags_sst[ip] & SSTF_ISMASKED) > 0
                    || (flags_sst[ip] & SSTF_BTBAD) > 0) {

                qual_sst[ip] = 8;

            } else if ((flags_sst[ip] & SSTF_BTRANGE) == 0
                    && (flags_sst[ip] & SSTF_BTVNONUNIF) == 0
                    && (flags_sst[ip] & SSTF_VHISENZ) == 0
                    && (flags_sst[ip] & SSTF_SSTRANGE) == 0
                    && (flags_sst[ip] & SSTF_SUNLIGHT) == 0) {

                if ((flags_sst[ip] & SSTF_SSTREFDIFF) == 0
                        && (flags_sst[ip] & SSTF_BTNONUNIF) == 0
                        && (flags_sst[ip] & SSTF_HISENZ) == 0
                        && (flags_sst[ip] & SSTF_GLINT) == 0
                        && (flags_sst[ip] & SSTF_CLOUD) == 0) {
                    qual_sst[ip] = 0;
                } else if ((flags_sst[ip] & SSTF_BTNONUNIF) == 0
                        && (flags_sst[ip] & SSTF_SSTREFDIFF) == 0
                        && (flags_sst[ip] & SSTF_CLOUD) == 0) {
                    qual_sst[ip] = 1;
                } else if ((flags_sst[ip] & SSTF_HISENZ) == 0
                        && (flags_sst[ip] & SSTF_SSTREFDIFF) == 0
                        && (flags_sst[ip] & SSTF_CLOUD) == 0) {
                    qual_sst[ip] = 2;
                } else if ((flags_sst[ip] & SSTF_CLOUD) == 0
                        && (flags_sst[ip] & SSTF_SSTREFDIFF) == 0) {
                    qual_sst[ip] = 3;
                } else if ((flags_sst[ip] & SSTF_BTNONUNIF) == 0
                        && (flags_sst[ip] & SSTF_HISENZ) == 0) {
                    qual_sst[ip] = 4;
                } else if ((flags_sst[ip] & SSTF_BTNONUNIF) == 0) {
                    qual_sst[ip] = 5;
                } else if ((flags_sst[ip] & SSTF_HISENZ) == 0) {
                    qual_sst[ip] = 6;
                } else {
                    qual_sst[ip] = 7;
                }
                if (l2rec->l1rec->ssttype[ip] == 0 && qual_sst[ip] < 4) {
                    qual_sst[ip] = 4;
                }
            } else {
                qual_sst[ip] = 7;
            }
        } else {

            if (l2rec->l1rec->solz[ip] < solznight) {

                /* daytime 11um SST quality level */

                if ((flags_sst[ip] & SSTF_ISMASKED) > 0
                        || (flags_sst[ip] & SSTF_BTBAD) > 0) {

                    qual_sst[ip] = 4;

                } else if ((flags_sst[ip] & SSTF_VHISENZ) > 0
                        || (flags_sst[ip] & SSTF_BTRANGE) > 0
                        || (flags_sst[ip] & SSTF_SSTRANGE) > 0
                        || (flags_sst[ip] & SSTF_BTVNONUNIF) > 0
                        || (flags_sst[ip] & SSTF_SSTREFVDIFF) > 0
                        || (flags_sst[ip] & SSTF_CLOUD) > 0
                        || l2rec->l1rec->ssttype[ip] == 0) {

                    qual_sst[ip] = 3;

                } else if (((input->viirsnv7 >= 1)
                        && ((flags_sst[ip] & SSTF_BTNONUNIF) > 0))
                        || (flags_sst[ip] & SSTF_SSTREFDIFF) > 0
                        || (flags_sst[ip] & SSTF_REDNONUNIF) > 0) {

                    qual_sst[ip] = 2;

                } else if (((input->viirsnv7 <= 0)
                        && ((flags_sst[ip] & SSTF_BTNONUNIF) > 0))
                        || (l2rec->l1rec->glint_coef[ip] > glintmax)
                        || (flags_sst[ip] & SSTF_HISENZ) > 0) {
                    /* if this changes then change comp_sses_sstv6mv */
                    /* because we use qual 0 sses stats for qual 1 due to glint only */

                    qual_sst[ip] = 1;

                } else {

                    qual_sst[ip] = 0;

                }

                // Kay thinks this was supposed to be taken out for modis also, but definitely don't want it for viirs?
                //if (l2rec->l1rec->l1file->sensorID == MODIST || l2rec->l1rec->l1file->sensorID == MODISA) {
                /* Reduce quality if red-band reflectance is high and sst is cold (collect 5) */
                /* only in non glint areas */
                if (haveRed && l2rec-> l1rec->glint_coef[ip] <= glintmax) {
                    ipb = ip * nbvis + ibred;
                    /* Sue: should it divide by csolz here as it does everywhere else?:
                     * rhot = PI * l2rec->l1rec->Lt[ipb] / l2rec->Fo[ibred] / l2rec->csolz[ip];
                     * but then the 0.05 limit needs to change?
                     */
                    rhot = PI * l2rec->l1rec->Lt[ipb] / l2rec->l1rec->Fo[ibred];
                    if (qual_sst[ip] < 3 && sstq[csstbox][ip] - l2rec->l1rec->sstref[ip] < -1.0
                            && rhot > 0.05) {
                        qual_sst[ip]++;
                        if ((input->viirsnv7 >= 1)
                                && (qual_sst[ip] == 1)) {
                            /* viirsnv7 test want only hisenz to be qual 1 so degrade q0 to q2 */
                            qual_sst[ip]++;
                        }

                    }
                }
                //}
            } else {
                /* nighttime 11um SST quality level */

                if ((flags_sst[ip] & SSTF_ISMASKED) > 0
                        || (flags_sst[ip] & SSTF_BTBAD) > 0) {

                    qual_sst[ip] = 4;

                } else if ((flags_sst[ip] & SSTF_BTRANGE) > 0
                        || (flags_sst[ip] & SSTF_SSTRANGE) > 0
                        || (flags_sst[ip] & SSTF_BT4REFDIFF) > 0
                        || (flags_sst[ip] & SSTF_BTVNONUNIF) > 0
                        || (flags_sst[ip] & SSTF_CLOUD) > 0
                        || (flags_sst[ip] & SSTF_VHISENZ) > 0
                        || (flags_sst[ip] & SSTF_SSTREFVDIFF) > 0) {
                    /* BTVNONUNIF should be qual 3 as it is for day */
                    /* don't need to check for haveSST4 because BT4REFDIF is only set if haveSST4 */

                    qual_sst[ip] = 3;

                } else if ((flags_sst[ip] & SSTF_SSTREFDIFF) > 0
                        || (flags_sst[ip] & SSTF_SST4VDIFF) > 0
                        || ((input->viirsnv7 >= 1)
                        && ((flags_sst[ip] & SSTF_BTNONUNIF) > 0))
                        || ((input->viirsnv7 >= 1)
                        && ((flags_sst[ip] & SSTF_SST4DIFF) > 0))
                        || ((input->viirsnv7 >= 1)
                        && ((flags_sst[ip] & SSTF_SST3DIFF) > 0))
                        || (flags_sst[ip] & SSTF_SST3VDIFF) > 0) {
                    /* for now sst4vdiff and sst3vdiff are the same bit */
                    /* BTNONUNIF should be qual 2 as it is for day */

                    qual_sst[ip] = 2;

                } else if (((input->viirsnv7 <= 0)
                        && ((flags_sst[ip] & SSTF_BTNONUNIF) > 0))
                        || ((input->viirsnv7 <= 0)
                        && ((flags_sst[ip] & SSTF_SST4DIFF) > 0))
                        || ((input->viirsnv7 <= 0)
                        && ((flags_sst[ip] & SSTF_SST3DIFF) > 0))
                        || (flags_sst[ip] & SSTF_HISENZ) > 0) {
                    /* for now sst4diff and sst3diff are the same bit */

                    qual_sst[ip] = 1;

                } else {

                    qual_sst[ip] = 0;
                }

                /* reduce quality if 4um BTs show non-uniformity (collect 5) */
                if (haveSST4 && sst4q[csstbox][ip] > sstbad + 1.0) {
                    if (qual_sst[ip] < 3
                            && (flags_sst4[ip] & SSTF_BTNONUNIF) > 0)
                        qual_sst[ip]++;
                    if ((input->viirsnv7 >= 1) && (qual_sst[ip] == 1)) {
                        /* viirsnv7 test want only hisenz to be qual 1 so degrade q0 to q2 */
                        qual_sst[ip]++;
                    }
                }

            } /* end night sst quality */

        } /* end modis and viirs sst quality */
    } /* end pixel loop */
    return;
}

/* ----------------------------------------------------------------------------------- */
/* set_qual_sst4() - set quality levels for short wave sea surface temperature         */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
void set_qual_sst4(l2str *l2rec) {
    int32_t ip;

    for (ip = 0; ip < l2rec->l1rec->npix; ip++) {

        if (l2rec->l1rec->solz[ip] < solznight) {

            /* daytime 4um SST quality level */

            if ((flags_sst4[ip] & SSTF_ISMASKED) > 0
                    || (flags_sst4[ip] & SSTF_BTBAD) > 0) {

                qual_sst4[ip] = 4;

            } else {

                qual_sst4[ip] = 3;

            }

        } else {

            /* nighttime 4um SST quality level */

            if ((flags_sst4[ip] & SSTF_ISMASKED) > 0
                    || (flags_sst4[ip] & SSTF_BTBAD) > 0) {

                qual_sst4[ip] = 4;

            } else if ((flags_sst4[ip] & SSTF_BTRANGE) > 0
                    || (flags_sst4[ip] & SSTF_SSTRANGE) > 0
                    || (flags_sst4[ip] & SSTF_BT4REFDIFF) > 0
                    || (flags_sst4[ip] & SSTF_BTVNONUNIF) > 0
                    || (flags_sst4[ip] & SSTF_CLOUD) > 0
                    || (flags_sst4[ip] & SSTF_VHISENZ) > 0
                    || (flags_sst4[ip] & SSTF_SSTREFVDIFF) > 0) {
                /* BTVNONUNIF and VHISENZ are very bad */

                qual_sst4[ip] = 3;

            } else if ((flags_sst4[ip] & SSTF_SSTREFDIFF) > 0
                    || (flags_sst4[ip] & SSTF_SST4VDIFF) > 0) {
                /* I'm pretty sure this SSTF_SSTREFDIFF should be in flags_sst4, not flags_sst */

                qual_sst4[ip] = 2;

            } else if ((flags_sst4[ip] & SSTF_BTNONUNIF) > 0
                    || (flags_sst4[ip] & SSTF_SST4DIFF) > 0
                    || (flags_sst4[ip] & SSTF_HISENZ) > 0) {

                qual_sst4[ip] = 1;

            } else {

                qual_sst4[ip] = 0;
            }
        }

    }
    return;
}

/* ----------------------------------------------------------------------------------- */
/* nlsst() - nonlinear sst, long wave sea surface temperature                          */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
float nlsst(float Bt11, float Bt12, int32_t is, float sstref,
        float **bounds, float **coef, float sstrefoffday,
        float sstrefoffnight, int32_t ip, int32_t xsatid, 
        size_t nlatbands, size_t ncoefficients) {
    extern l1qstr l1que;
    static float dBtlo = 0.5;
    static float dBthi = 0.9;

    float lat = l1que.r[is].lat[ip];
    float mu = l1que.r[is].csenz[ip];

    int32_t sensorID = l1que.r[is].l1file->sensorID;
    int mside = l1que.r[is].mside;

    float dBt = Bt11 - Bt12; /* aka "thing" */
    float sstlo, ssthi, lsst;
    float satzdir;
    int i, j;

    /* it doesn't hurt, but V5 and viirs osi-saf do not have latband coeffs so they don't use 'ic' */
    /* bounds go from -90 to 90 so we want the first set */
    /* that has max lat greater than pixel lat           */
    int ic = nlatbands - 1;
    while ((ic > 0) && (lat <= bounds[ic - 1][1] + latwin)) {
        ic--;
    }

    /* convert sstref to K or whatever offset is required for the coeffs - usually 0.0 except for VIIRS */
    if (l1que.r[is].solz[ip] >= solznight)
        sstref = sstref + sstrefoffnight;
    else
        sstref = sstref + sstrefoffday;

    /* sstref is supposed to warm the retrieved temperature, not cool it */
    /* this is the scaling factor for temp deficit term */
    /* not a problem for deg F or K, but deg C can go negative and we don't want that */
    if (sstref < 0.0) {
        sstref = 0.0;
    }
    // VIIRS coefficients made for brightness temperatures are in Kelvin...so add the offset
    if (sensorID == VIIRSN) {
        Bt11 += CtoK
                ;
    }
    /* this WILL now work if working on a subset of the data (spixl or epixl were specified) */
    satzdir = (l1que.r[is].pixnum[ip] < fullscanpix / 2) ? -1.0 : 1.0;

    if (isV5) {
        /* Noaa-16 uses different bounds than all the other AVHRR satellites */
        if (xsatid == NO16) {
            dBtlo = 1.0;
            dBthi = 1.4;
        }
        if(nlatbands < 2) {
            printf("ERROR - need nlatbands >= 2, found %ld\n", nlatbands);
            exit(EXIT_FAILURE);
        }
        if(ncoefficients < 4) {
            printf("ERROR - need ncoefficients >= 4, found %ld\n", ncoefficients);
            exit(EXIT_FAILURE);
        }
        if (dBt <= dBtlo)
            lsst = coef[0][0] + coef[0][1] * Bt11 + coef[0][2] * dBt * sstref + coef[0][3] * dBt * ((1.0 / mu) - 1.0);
        else if (dBt >= dBthi)
            lsst = coef[1][0] + coef[1][1] * Bt11 + coef[1][2] * dBt * sstref + coef[1][3] * dBt * ((1.0 / mu) - 1.0);
        else {
            sstlo = coef[0][0] + coef[0][1] * Bt11 + coef[0][2] * dBt * sstref + coef[0][3] * dBt * ((1.0 / mu) - 1.0);
            ssthi = coef[1][0] + coef[1][1] * Bt11 + coef[1][2] * dBt * sstref + coef[1][3] * dBt * ((1.0 / mu) - 1.0);
            lsst = sstlo + ((dBt - dBtlo) / (dBthi - dBtlo))*(ssthi - sstlo);
        }

    } else if (input->viirsnosisaf == 1) {
        if (l1que.r[is].solz[ip] >= solznight)
            ic = 2;
        else
            ic = 0;
        /* OSI-SAF only has one set of coeffs for day and one for night, not two */
        /* OSI-SAF equation: Ts = b0 + (b1 + bt2 Stheta) T11 + [b3 + b4 (Ts0 - 273.15) + b5 Stheta] dT11-12 + b6 Stheta */
        /* Ts0 is in degrees Kelvin.  Our sstref is in Deg C so don't need to subract 273.15 */
        if(ncoefficients < 7) {
            printf("ERROR - need ncoefficients >= 7, found %ld\n", ncoefficients);
            exit(EXIT_FAILURE);
        }
        lsst = coef[ic][0]
                + (coef[ic][1] + coef[ic][2] * ((1.0 / mu) - 1.0)) * Bt11
                + (coef[ic][3] + coef[ic][4] * sstref + coef[ic][5] * ((1.0 / mu) - 1.0)) * dBt
                + coef[ic][6] * ((1.0 / mu) - 1.0);

    } else if (input->viirsnv7 >= 1) {
        /* v7 high senz latband coeffs */
        /* choose coeffs by latband */

        if (ncoefficients < 7) {
            printf("ERROR - need ncoefficients >= 7, found %ld\n", ncoefficients);
            exit(EXIT_FAILURE);
        }
        if (lat < bounds[ic][1] - latwin || ic == (nlatbands - 1)) {
            lsst = coef[ic][0] + coef[ic][1] * Bt11 + coef[ic][2] * dBt * sstref
                    + coef[ic][3] * dBt * ((1.0 / mu) - 1.0)
                    + coef[ic][4] * ((1.0 / mu) - 1.0)
                    + coef[ic][5] * pow(((1.0 / mu) - 1.0), coef[ic][6]);

        } else {
            dBtlo = bounds[ic][1] - latwin;
            dBthi = bounds[ic][1] + latwin;
            sstlo = coef[ic][0] + coef[ic][1] * Bt11 + coef[ic][2] * dBt * sstref
                    + coef[ic][3] * dBt * ((1.0 / mu) - 1.0)
                    + coef[ic][4] * ((1.0 / mu) - 1.0)
                    + coef[ic][5] * pow(((1.0 / mu) - 1.0), coef[ic][6]);
            ssthi = coef[ic + 1][0] + coef[ic + 1][1] * Bt11
                    + coef[ic + 1][2] * dBt * sstref
                    + coef[ic + 1][3] * dBt * ((1.0 / mu) - 1.0)
                    + coef[ic + 1][4] * ((1.0 / mu) - 1.0)
                    + coef[ic + 1][5] * pow(((1.0 / mu) - 1.0), coef[ic + 1][6]);

            lsst = sstlo + ((lat - dBtlo) / (dBthi - dBtlo)) * (ssthi - sstlo);

        }

    } else {
        /* v6 latband coeffs */

        /* Only coeff 4 for terra and aqua, they're zeroed for others */
        /* Only coeffs 5-6 for terra, aqua and viirs; they're zeroed for avhrr */
        static float* C0;
        static float* C1;
        static int last_ic = -1;

        if (C0 == NULL) {
            C0 = (float *) calloc(7, sizeof(float));
            C1 = (float *) calloc(7, sizeof(float));
        }
        if (last_ic != ic) {
            for (i = 0; i < ncoefficients; i++){
                j = i;
                if (((sensorID == VIIRSN) || (sensorID == AVHRR)) && (i > 3)){
                    j += 1;
                }
                C0[j] =  coef[ic][i];
                if (ic < nlatbands -1)
                    C1[j] =  coef[ic + 1][i];
            }
        }
        if (lat < bounds[ic][1] - latwin || ic == (nlatbands - 1)) {
            lsst = C0[0]
                 + C0[1] * Bt11
                 + C0[2] * dBt * sstref
                 + C0[3] * dBt * ((1.0 / mu) - 1.0)
                 + C0[4] * mside
                 + C0[5] * (l1que.r[is].senz[ip] * satzdir)
                 + C0[6] * pow((l1que.r[is].senz[ip] * satzdir), 2);
        } else {
            dBtlo = bounds[ic][1] - latwin;
            dBthi = bounds[ic][1] + latwin;
            sstlo = C0[0]
                  + C0[1] * Bt11
                  + C0[2] * dBt * sstref
                  + C0[3] * dBt * ((1.0 / mu) - 1.0)
                  + C0[4] * mside
                  + C0[5] * (l1que.r[is].senz[ip] * satzdir)
                  + C0[6] * pow((l1que.r[is].senz[ip] * satzdir), 2);
            ssthi = C1[0]
                  + C1[1] * Bt11
                  + C1[2] * dBt * sstref
                  + C1[3] * dBt * ((1.0 / mu) - 1.0)
                  + C1[4] * mside
                  + C1[5] * (l1que.r[is].senz[ip] * satzdir)
                  + C1[6] * pow((l1que.r[is].senz[ip] * satzdir), 2);

            lsst = sstlo + ((lat - dBtlo) / (dBthi - dBtlo)) * (ssthi - sstlo);

        }
    }
    if (sensorID == VIIRSN) {
        lsst = lsst - CtoK; /* internally, want all sst's in Deg C */
    }
    return (lsst);
}

/* ----------------------------------------------------------------------------------- */
/* nlsst4() - nonlinear sst4, 4um sea surface temperature (3.9 & 4.0 um)               */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
float nlsst4(float Bt39, float Bt40, int32_t is, float **bounds, float **coef, int32_t ip, size_t nlatbands) {
    extern l1qstr l1que;
    int ic;
    int mside = l1que.r[is].mside;
    float lat = l1que.r[is].lat[ip];
    float senz = l1que.r[is].senz[ip];
    float mu = l1que.r[is].csenz[ip];
    float dBt = Bt39 - Bt40;

    float lsst4;
    float sst4lo;
    float sst4hi;
    float dBtlo;
    float dBthi;
    float satzdir;

    /* choose coeffs by latband */

    /* bounds go from -90 to 90 so we want the first set */
    /* that has max lat greater than pixel lat           */
    ic = nlatbands - 1;
    while ((ic > 0) && lat <= (bounds[ic - 1][1] + latwin)) {
        ic--;
    }

    /* this WILL now work if working on a subset of the data (spixl or epixl were specified) */
    satzdir = (l1que.r[is].pixnum[ip] < fullscanpix / 2) ? -1.0 : 1.0;

    /* Only coeffs 4-6 for terra, they're zeroed for others */
    if (lat < bounds[ic][1] - latwin || ic == (nlatbands - 1)) {
        lsst4 = coef[ic][0] + coef[ic][1] * Bt39 + coef[ic][2] * dBt
                + coef[ic][3] * ((1.0 / mu) - 1.0) + coef[ic][4] * mside
                + coef[ic][5] * (senz * satzdir)
                + coef[ic][6] * pow((senz * satzdir), 2);

    } else {
        dBtlo = bounds[ic][1] - latwin;
        dBthi = bounds[ic][1] + latwin;
        sst4lo = coef[ic][0] + coef[ic][1] * Bt39 + coef[ic][2] * dBt
                + coef[ic][3] * ((1.0 / mu) - 1.0) + coef[ic][4] * mside
                + coef[ic][5] * (l1que.r[is].senz[ip] * satzdir)
                + coef[ic][6] * pow((l1que.r[is].senz[ip] * satzdir), 2);
        sst4hi = coef[ic + 1][0] + coef[ic + 1][1] * Bt39 + coef[ic + 1][2] * dBt
                + coef[ic + 1][3] * ((1.0 / mu) - 1.0) + coef[ic + 1][4] * mside
                + coef[ic + 1][5] * (l1que.r[is].senz[ip] * satzdir)
                + coef[ic + 1][6] * pow((l1que.r[is].senz[ip] * satzdir), 2);

        lsst4 = sst4lo + ((lat - dBtlo) / (dBthi - dBtlo)) * (sst4hi - sst4lo);
    }

    return (lsst4);
}

/* ----------------------------------------------------------------------------------- */
/* comp_sst4() - compute sea surface temperature                           .           */
/*                                                                                     */
/* B. Franz, SAIC, August 2003.                                                        */

/* ----------------------------------------------------------------------------------- */
void comp_sst4(l2str *l2rec, int32_t is) {
    /* use l2rec for the general stuff, but use l1rec for the scan being processed */
    /*   l1rec may be ahead or behind l2rec to make the box needed for statistics */
    extern l1qstr l1que;
    static int firstCall = 1;

    static float **coef;
    static float **bounds;
    static size_t nlatbands = 0;
    static size_t ncoefficients = 0;

    statstr statrec;

    int32_t i, j, ip, ipb;
    float Bt39;
    float Bt40;
    float elecor;

    if (firstCall) {
        if (input->proc_sst != 1) {
            printf(
                    "-E- %s line %d: SST processing must be enabled (proc_sst=1) to make sst4.\n",
                    __FILE__, __LINE__);
            exit(1);
        }

        if (input->sst4coeffile != NULL) {
            size_t nmonth = 1;
            int32_t nlats = 2;
            int32_t latbandDimID, coeffDimID, boundsID, coefficientID;
            int32_t ncid;
            if (nc_open(input->sst4coeffile, NC_NOWRITE, &ncid) == NC_NOERR) {
                printf("Loading SST4 lat band coefficients from %s:\n", input->sst4coeffile);

                if (nc_inq_dimid(ncid, "latband", &latbandDimID) != NC_NOERR){
                    printf("Whoops! something is wrong reading the SST algorithm coefficient file: %s\n",input->sst4coeffile);
                    exit(EXIT_FAILURE);
                }
                nc_inq_dimlen(ncid, latbandDimID, &nlatbands);

                if (nc_inq_dimid(ncid, "coefficient", &coeffDimID) != NC_NOERR){
                    printf("Whoops! something is wrong reading the SST algorithm coefficient file: %s\n",input->sst4coeffile);
                    exit(EXIT_FAILURE);
                }
                nc_inq_dimlen(ncid, coeffDimID, &ncoefficients);
                if(ncoefficients < 7) {
                    printf("ERROR - need ncoefficients >= 7 for sst4, found %ld\n", ncoefficients);
                    exit(EXIT_FAILURE);
                }
                
                bounds = allocate2d_float(nlatbands, nlats);
                if (nc_inq_varid (ncid, "latbands", &boundsID) == NC_NOERR){
                    nc_get_var_float(ncid, boundsID, bounds[0]);
                }

                int16_t year, month, day;
                double sec;
                unix2ymds(l2rec->l1rec->scantime, &year, &month, &day, &sec);
                month -= 1;

                coef = allocate2d_float(nlatbands, ncoefficients);
                if (nc_inq_varid (ncid, "coefficients", &coefficientID) == NC_NOERR){
                    size_t start[] = {month, 0, 0}; /* start at first value */
                    size_t count[] = {nmonth, nlatbands, ncoefficients};
                    nc_get_vara_float(ncid, coefficientID, start, count, coef[0]);
                }

                nc_close(ncid);
                // print coefficients to be used
                for (i = 0; i < nlatbands; i++){
                    printf("%d %6.2f %6.2f ", month+1, bounds[i][0], bounds[i][1]);
                    for (j = 0; j < ncoefficients; j++){
                        printf("%9.6f ", coef[i][j]);
                    }
                    printf("\n");
                }
            } else {

                nlatbands = 7;
                ncoefficients = 7;
                coef = allocate2d_float(nlatbands, ncoefficients);
                bounds = allocate2d_float(nlatbands, nlats);

                read_sst4_coeff(l2rec->l1rec->l1file->sensorID, input->sst4coeffile,
                    l1que.r[is].scantime, bounds, coef);
            }
        } else {
            printf("SST4 algorithm coefficient file not specified.");
            exit(1);
        }
        firstCall = 0;
    }

    for (ip = 0; ip < l2rec->l1rec->npix; ip++) {

        sst4q[is][ip] = sstbad;
        flags_sst4[ip] = 0;

        /* skip if pixel already masked */
        if (sstmasked(l1que.r[is].flags, ip))
            continue;

        ipb = ip * NBANDSIR;
        Bt39 = l1que.r[is].Bt[ipb + ib39];
        Bt40 = l1que.r[is].Bt[ipb + ib40];

        /* aqua detector 0 channel 23 has a problem so average detectors 9 and 1 instead */
        if ((l2rec->l1rec->l1file->sensorID == MODISA)
                && l1que.r[is].detnum == 0) {
            if (btavg(ip, is, ib40, NBANDSIR, &statrec) > 0) {
                Bt40 = statrec.avg;
            }
        }

        /* compute SST4 */
        /* skip pixel if BT could not be computed */
        if (Bt39 < BT_LO + 0.1 || Bt39 > BT_HI - 0.1 || Bt40 < BT_LO + 0.1
                || Bt40 > BT_HI - 0.1) {
            continue;
        }
        sst4q[is][ip] = nlsst4(Bt39, Bt40, is, bounds, coef, ip, nlatbands);

        /* Add post-hoc correction for electronics side effects to TERRA sst4 */
        if (l2rec->l1rec->l1file->sensorID == MODIST) {

            if (l2rec->l1rec->scantime <= 972777600) {
                /* 972777600 (2000303) is 29 Oct 2000 */
                elecor = 0.4452;
            } else if (l2rec->l1rec->scantime <= 993945600) {
                /* 993945600 (2001182) is 1 Jul 2001 */
                elecor = 0.2593;
            } else {
                elecor = 0.0;
            }
            sst4q[is][ip] += elecor;
        }
    }

    return;
}

/* ----------------------------------------------------------------------------------- */
/* comp_sst() - compute sea surface temperature                            .           */
/*                                                                                     */
/* B. Franz, SAIC, August 2003.                                                        */

/* ----------------------------------------------------------------------------------- */
void comp_sst(l2str *l2rec, int32_t is) {
    /* use l2rec for the general stuff, but use l1rec for the scan being processed */
    /*   l1rec may be ahead or behind l2rec to make the box needed for statistics */
    extern l1qstr l1que;
    static int firstCall = 1;

    static size_t ncoefficients = 0;
    static size_t nlatbands = 0;
    static float **coef;
    static float **bounds;

    static float sstrefoffday;
    static float sstrefoffnight;
    float sstref;
    int32_t i, j, ip, ipb;
    float Bt11;
    float Bt12;
    float Bt39;
    float Bt85;

    if (firstCall) {
        if (input->proc_sst != 1) {
            printf(
                    "-E- %s line %d: SST processing must be enabled (proc_sst=1) to make sst.\n",
                    __FILE__, __LINE__);
            exit(1);
        }
        if (input->sstcoeffile != NULL) {
            size_t nmonth = 1;
            int32_t nlats = 2;
            int32_t latbandDimID, coeffDimID, boundsID, coefficientID;
            int32_t ncid;
            if (nc_open(input->sstcoeffile, NC_NOWRITE, &ncid) == NC_NOERR) {
                printf("Loading SST lat band coefficients from %s:\n", input->sstcoeffile);

                if (nc_inq_dimid(ncid, "latband", &latbandDimID) != NC_NOERR){
                    printf("Whoops! something is wrong reading the SST algorithm coefficient file: %s\n",input->sstcoeffile);
                    exit(EXIT_FAILURE);
                }
                nc_inq_dimlen(ncid, latbandDimID, &nlatbands);
                if (nc_inq_dimid(ncid, "coefficient", &coeffDimID) != NC_NOERR){
                    printf("Whoops! something is wrong reading the SST algorithm coefficient file: %s\n",input->sstcoeffile);
                    exit(EXIT_FAILURE);
                }
                nc_inq_dimlen(ncid, coeffDimID, &ncoefficients);

                bounds = allocate2d_float(nlatbands, nlats);
                if (nc_inq_varid (ncid, "latbands", &boundsID) == NC_NOERR){
                    nc_get_var_float(ncid, boundsID, bounds[0]);
                }

                int16_t year, month, day;
                double sec;
                unix2ymds(l2rec->l1rec->scantime, &year, &month, &day, &sec);
                month -= 1;

                coef = allocate2d_float(nlatbands, ncoefficients);
                if (nc_inq_varid (ncid, "coefficients", &coefficientID) == NC_NOERR){
                    size_t start[] = {month, 0, 0}; /* start at first value */
                    size_t count[] = {nmonth, nlatbands, ncoefficients};
                    nc_get_vara_float(ncid, coefficientID, start, count, coef[0]);
                }

                nc_close(ncid);
                // print coefficients to be used
                for (i = 0; i < nlatbands; i++){
                    printf("%d %6.2f %6.2f ", month+1, bounds[i][0], bounds[i][1]);
                    for (j = 0; j < ncoefficients; j++){
                        printf("%9.6f ", coef[i][j]);
                    }
                    printf("\n");
                }
            } else {

                sstrefoffday = 0.0; /* default to 0.0 */
                sstrefoffnight = 0.0; /* default to 0.0 */
                if (isV5) {
                    // this one only reads 2 lat bands
                    ncoefficients = 7;
                    nlatbands = 2;
                    coef = allocate2d_float(nlatbands, ncoefficients);
                    bounds = allocate2d_float(nlatbands, nlats);
                    read_v5_sst_coeff(l2rec, bounds, coef, &sstrefoffday, &sstrefoffnight);
                } else {
                    ncoefficients = 7;
                    nlatbands = 7;
                    coef = allocate2d_float(nlatbands, ncoefficients);
                    bounds = allocate2d_float(nlatbands, nlats);
                    read_sst_coeff(l2rec, bounds, coef, &sstrefoffday, &sstrefoffnight);
                }
            }

        } else {
            printf("SST algorithm coefficient file not specified.");
            exit(1);
        }

        firstCall = 0;
    }

    for (ip = 0; ip < l2rec->l1rec->npix; ip++) {

        sstq[is][ip] = sstbad;
        flags_sst[ip] = 0;

        /* skip if pixel already masked */
        if (sstmasked(l1que.r[is].flags, ip))
            continue;

        ipb = ip * NBANDSIR;
        Bt11 = l1que.r[is].Bt[ipb + ib11];
        Bt12 = l1que.r[is].Bt[ipb + ib12];
        Bt39 = l1que.r[is].Bt[ipb + ib39];
        Bt85 = l1que.r[is].Bt[ipb + ib85];

        /* skip pixel if BT could not be computed */
        if (Bt11 < BT_LO + 0.1 || Bt11 > BT_HI - 0.1 || Bt12 < BT_LO + 0.1
                || Bt12 > BT_HI - 0.1) {
            continue;
        }

        /* use 4um SST for night reference */
        if (haveSST4 && l1que.r[is].solz[ip] >= solznight && sst4q[is][ip] > sstbad + 1.0)
            sstref = sst4q[is][ip];
        else
            sstref = l1que.r[is].sstref[ip];

        /* don't bother processing if no sstref */
        if (sstref < sstbad + 1.0) {
            continue;
        }

        /* compute SST */
        sstq[is][ip] = nlsst(Bt11, Bt12, is, sstref, bounds, coef, sstrefoffday,
                sstrefoffnight, ip, l2rec->l1rec->l1file->subsensorID, 
                nlatbands, ncoefficients);
        if (l1que.r[is].l1file->sensorID == AVHRR && isV5 == 1) {
            sstq[is][ip] -= 0.17;
        }
        // if ancillary aerosol provided and lat between 20S and 35N and lon
        // between 90W and 90E, apply the dust correction
        if (l2rec->l1rec->anc_aerosol &&  l1que.r[is].solz[ip] >= solznight &&
               (l1que.r[is].l1file->sensorID == MODISA || 
                l1que.r[is].l1file->sensorID == MODIST)) {
//            if ((l1que.r[is].lat[ip] >= -20.) &&
//                    (l1que.r[is].lat[ip] <= 35.) &&
//                    (l1que.r[is].lon[ip] >= -90.) &&
//                    (l1que.r[is].lon[ip] <= 90.)) {

                float dustExtinction = l1que.r[is].anc_aerosol->dust_ext[ip];
                float csenz = l1que.r[is].csenz[ip];
                dsdi_correction[ip] = dust_correction(dustExtinction, csenz, Bt39, Bt85, Bt11, Bt12,l1que.r[is].l1file->sensorID);
                sstq[is][ip] += dsdi_correction[ip];
//            }
        }
    }

    return;
}

/* ------------------------------------------------------------------------ */
/* comp_sses_sstv6v() - computes sensor-specific error stats for viirs SST  */

/* ------------------------------------------------------------------------ */
void comp_sses_sstv6v(ssestabstrv6v *sses, float diff, float lsst, float solz,
        float senz, int doy, float lat, int8 iqual, int16 iflags, float glintv,
        int iviirsnv7, float *bias, float *stdv, float *bias_mean,
        int16 *bias_counts) {
    int i, iday, isst, idiff, isenz, iquar, ilat;

    /* get table indices */

    /* if qual is one and glint and no other reason to be qual one, then use qual zero sses values */
    /*   because the night sses hypercubes don't include glint */
    /* July 2017, still use qual 0 for glint only qual 1 pixels, even though there are viirs day sses */
    /* modis qual one if glint or hisenz or btnonunif */
    /* viirs same except btnonunif is qual 2 if v7 */
    if (iqual == 1 && glintv > glintmax
            && ((iviirsnv7 <= 0) && ((iflags & SSTF_BTNONUNIF) == 0))
            && (iflags & SSTF_HISENZ) == 0) {
        iqual = 0;
    }

    iquar = doy / 91;

    if (solz >= solznight)
        iday = 0; /* night */
    else
        iday = 1; /* day */

    for (i = 1; i < sses->nlat; i++)
        if (lat < sses->lat[i])
            break;
    ilat = i - 1;
    for (i = 1; i < sses->nsenz; i++)
        if (senz < sses->senz[i])
            break;
    isenz = i - 1;
    for (i = 1; i < sses->nsst; i++)
        if (lsst < sses->sst[i])
            break;
    isst = i - 1;
    for (i = 1; i < sses->ndiff; i++)
        if (diff <= sses->diff[i])
            break;
    idiff = i - 1;

    /* set bias and standard deviation */
    if (iqual < sses->nqual) {
        *bias = sses->bias[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        *stdv = sses->stdv[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        *bias_mean =
                sses->bias_mean[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        *bias_counts =
                sses->counts[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        /* (15-Aug-2011) v6 bias's are skin */
    } else {
        *bias = BAD_FLT;
        *stdv = BAD_FLT;
        *bias_mean = BAD_FLT;
        *bias_counts = 0;
    }
}

/* -------------------------------------------------------------------------- */
/* comp_sses_sstv6v3() - computes sensor-specific error stats for viirs SST3  */

/* -------------------------------------------------------------------------- */
void comp_sses_sstv6v3(ssestabstrv6v3 *sses, float diff, float lsst, float solz,
        float senz, int doy, float lat, int8 iqual, int16 iflags, float glintv,
        int iviirsnv7, float *bias, float *stdv, float *bias_mean,
        int16 *bias_counts) {
    int i, iday, isst, idiff, isenz, iquar, ilat;

    /* get table indices */

    /* if qual is one and glint and no other reason to be qual one, then use qual zero sses values */
    /*   because the night sses hypercubes don't include glint */
    /* July 2017, still use qual 0 for glint only qual 1 pixels, even though there are viirs day sses */
    /* modis qual one if glint or hisenz or btnonunif */
    /* viirs same except btnonunif is qual 2 if v7 */
    if (iqual == 1 && glintv > glintmax
            && (((iviirsnv7 <= 0) && ((iflags & SSTF_BTNONUNIF) == 0))
            || (iviirsnv7 >= 1)) && (iflags & SSTF_HISENZ) == 0) {
        iqual = 0;
    }

    iquar = doy / 91;

    if (solz >= solznight)
        iday = 0; /* night */
    else
        iday = 1; /* day */


    for (i = 1; i < sses->nlat; i++)
        if (lat < sses->lat[i])
            break;
    ilat = i - 1;
    for (i = 1; i < sses->nsenz; i++)
        if (senz < sses->senz[i])
            break;
    isenz = i - 1;
    for (i = 1; i < sses->nsst; i++)
        if (lsst < sses->sst[i])
            break;
    isst = i - 1;
    for (i = 1; i < sses->ndiff; i++)
        if (diff <= sses->diff[i])
            break;
    idiff = i - 1;

    /* set bias and standard deviation */
    if (iqual < sses->nqual) {
        *bias = sses->bias[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        *stdv = sses->stdv[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        *bias_mean =
                sses->bias_mean[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        *bias_counts =
                sses->counts[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        /* (15-Aug-2011) v6 bias's are skin */
    } else {
        *bias = BAD_FLT;
        *stdv = BAD_FLT;
        *bias_mean = BAD_FLT;
        *bias_counts = 0;
    }
}
/* ------------------------------------------------------------------------ */
/* comp_sses_sstv6m() - computes sensor-specific error stats for modis SST  */

/* ------------------------------------------------------------------------ */
void comp_sses_sstv6m(ssestabstrv6m *sses, float diff, float lsst, float solz,
        float senz, int doy, float lat, int8 iqual, int16 iflags, float glintv,
        int iviirsnv7, float *bias, float *stdv, float *bias_mean,
        int16 *bias_counts) {
    int i, iday, isst, idiff, isenz, iquar, ilat;

    /* get table indices */

    /* if qual is one and glint and no other reason to be qual one, then use qual zero sses values */
    /*   because the night sses hypercubes don't include glint */
    /* July 2017, still use qual 0 for glint only qual 1 pixels, even though there are viirs day sses */
    /* modis qual one if glint or hisenz or btnonunif */
    /* viirs same except btnonunif is qual 2 if v7 */
    if (iqual == 1 && glintv > glintmax
            && (((iviirsnv7 <= 0) && ((iflags & SSTF_BTNONUNIF) == 0))
            || (iviirsnv7 >= 1)) && (iflags & SSTF_HISENZ) == 0) {
        iqual = 0;
    }

    iquar = doy / 91;

    if (solz >= solznight)
        iday = 0; /* night */
    else
        iday = 1; /* day */

    for (i = 1; i < sses->nlat; i++)
        if (lat < sses->lat[i])
            break;
    ilat = i - 1;
    for (i = 1; i < sses->nsenz; i++)
        if (senz < sses->senz[i])
            break;
    isenz = i - 1;
    for (i = 1; i < sses->nsst; i++)
        if (lsst < sses->sst[i])
            break;
    isst = i - 1;
    for (i = 1; i < sses->ndiff; i++)
        if (diff < sses->diff[i])
            break;
    idiff = i - 1;

    /* set bias and standard deviation */
    if (iqual < sses->nqual) {
        *bias = sses->bias[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        *stdv = sses->stdv[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        *bias_mean =
                sses->bias_mean[(int) iqual][ilat][idiff][isenz][iquar][isst];
        *bias_counts =
                sses->counts[(int) iqual][ilat][idiff][isenz][iquar][isst];
        /* (15-Aug-2011) v6 bias's are skin */
    } else {
        *bias = BAD_FLT;
        *stdv = BAD_FLT;
        *bias_mean = BAD_FLT;
        *bias_counts = 0;
    }
}
/* -------------------------------------------------------------------------- */
/* comp_sses_sstv6v3() - computes sensor-specific error stats for viirs SST3  */

/* -------------------------------------------------------------------------- */
void comp_sses_sstv6m4(ssestabstrv6m4 *sses, float diff, float lsst, float solz,
        float senz, int doy, float lat, int8 iqual, int16 iflags, float glintv,
        int iviirsnv7, float *bias, float *stdv, float *bias_mean,
        int16 *bias_counts) {
    int i, iday, isst, idiff, isenz, iquar, ilat;

    /* get table indices */

    /* if qual is one and glint and no other reason to be qual one, then use qual zero sses values */
    /*   because the night sses hypercubes don't include glint */
    /* July 2017, still use qual 0 for glint only qual 1 pixels, even though there are viirs day sses */
    /* modis qual one if glint or hisenz or btnonunif */
    /* viirs same except btnonunif is qual 2 if v7 */
    if (iqual == 1 && glintv > glintmax
            && (((iviirsnv7 <= 0) && ((iflags & SSTF_BTNONUNIF) == 0))
            || (iviirsnv7 >= 1)) && (iflags & SSTF_HISENZ) == 0) {
        iqual = 0;
    }

    iquar = doy / 91;

    if (solz >= solznight)
        iday = 0; /* night */
    else
        iday = 1; /* day */


    for (i = 1; i < sses->nlat; i++)
        if (lat < sses->lat[i])
            break;
    ilat = i - 1;
    for (i = 1; i < sses->nsenz; i++)
        if (senz < sses->senz[i])
            break;
    isenz = i - 1;
    for (i = 1; i < sses->nsst; i++)
        if (lsst < sses->sst[i])
            break;
    isst = i - 1;
    for (i = 1; i < sses->ndiff; i++)
        if (diff <= sses->diff[i])
            break;
    idiff = i - 1;

    /* set bias and standard deviation */
    if (iqual < sses->nqual) {
        *bias = sses->bias[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        *stdv = sses->stdv[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        *bias_mean =
                sses->bias_mean[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        *bias_counts =
                sses->counts[(int) iqual][ilat][idiff][isenz][iquar][iday][isst];
        /* (15-Aug-2011) v6 bias's are skin */
    } else {
        *bias = BAD_FLT;
        *stdv = BAD_FLT;
        *bias_mean = BAD_FLT;
        *bias_counts = 0;
    }
}

/* ----------------------------------------------------------------------------------- */
/* comp_sses_sstv6a() - computes sensor-specific error stats for avhrr SST             */

/* ----------------------------------------------------------------------------------- */
void comp_sses_sstv6a(ssestabstrv6a *sses, float diff, float lsst, float solz,
        float senz, int doy, float lat, int8 iqual, int16 iflags, float *bias,
        float *stdv, float *bias_mean, int16 *bias_counts) {
    int i, isst, idiff, isenz, iquar, ilat;

    /* get table indices */

    /* if qual is one and glint and no other reason to be qual one, then use qual zero sses values */
    /*   because the night sses hypercubes don't include glint */
    /* avhrr qual one if glint or hisenz */
    if (iqual == 1 && ((iflags & SSTF_GLINT) > 0)
            && ((iflags & SSTF_HISENZ) == 0)) {
        iqual = 0;
    }

    iquar = doy / 91;

    for (i = 1; i < sses->nlat; i++)
        if (lat < sses->lat[i])
            break;
    ilat = i - 1;
    for (i = 1; i < sses->nsenz; i++)
        if (senz < sses->senz[i])
            break;
    isenz = i - 1;
    for (i = 1; i < sses->nsst; i++)
        if (lsst < sses->sst[i])
            break;
    isst = i - 1;
    for (i = 1; i < sses->ndiff; i++)
        if (diff < sses->diff[i])
            break;
    idiff = i - 1;

    /* set bias and standard deviation */
    if (iqual < sses->nqual) {
        *bias = sses->bias[(int) iqual][ilat][idiff][isenz][iquar][isst];
        *stdv = sses->stdv[(int) iqual][ilat][idiff][isenz][iquar][isst];
        *bias_mean =
                sses->bias_mean[(int) iqual][ilat][idiff][isenz][iquar][isst];
        *bias_counts =
                sses->counts[(int) iqual][ilat][idiff][isenz][iquar][isst];
        /* (15-Aug-2011) v6 bias's are skin */
        //        if ((evalmask & SSTMODS) != 0) *bias += 0.17;  // add skin-temp offset (7/22/2009)
    } else {
        *bias = BAD_FLT;
        *stdv = BAD_FLT;
        *bias_mean = BAD_FLT;
        *bias_counts = 0;
    }
}

/* -------------------------------------------------------------------------------------------- */
/* set_sses_sst3v6v3() - set sensor-specific error stats for viirs jpss triple window algorithm */
/*                                                                                              */

/* -------------------------------------------------------------------------------------------- */
void set_sses_sst3v6v3(l2str *l2rec) {

    int32_t npix = l2rec->l1rec->npix;
    int32_t ip, ipb;
    float Bt37;
    float Bt12;
    float dBt;
    double pasutime = l2rec->l1rec->scantime;
    int16_t year, day;
    double sec;
    unix2yds(pasutime, &year, &day, &sec);
    /* set each pixels bias and stdev (assumes sst and qual computed) */
    for (ip = 0; ip < npix; ip++) {
        ipb = ip * NBANDSIR;
        Bt37 = l2rec->l1rec->Bt[ipb + ib37];
        Bt12 = l2rec->l1rec->Bt[ipb + ib12];
        dBt = Bt37 - Bt12;


        comp_sses_sstv6v3(&sses_sst3v6v3, dBt, sst3q[csstbox][ip], l2rec->l1rec->solz[ip],
                l2rec->l1rec->senz[ip], day, l2rec->l1rec->lat[ip], qual_sst3[ip],
                flags_sst3[ip], l2rec->l1rec->glint_coef[ip], input->viirsnv7,
                &bias_sst3[ip], &stdv_sst3[ip], &bias_mean_sst3[ip],
                &bias_counts_sst3[ip]);
    }
}

/* ----------------------------------------------------------------------------------------- */
/* set_sses_sst4v6m() - set sensor-specific error stats for short wave sea surface temperature  */
/*                                                                                           */
/* B. Franz, SAIC, August 2005.                                                              */

/* ----------------------------------------------------------------------------------------- */
void set_sses_sst4v6m(l2str *l2rec) {

    int32_t npix = l2rec->l1rec->npix;
    int32_t ip;
    double pasutime = l2rec->l1rec->scantime;
    int16_t year, day;
    double sec;
    unix2yds(pasutime, &year, &day, &sec);
    /* set each pixels bias and stdev (assumes sst and qual computed) */
    for (ip = 0; ip < npix; ip++) {

        comp_sses_sstv6m4(&sses_sst4v6m, d3940ref[ip], sst4q[csstbox][ip],
                l2rec->l1rec->solz[ip], l2rec->l1rec->senz[ip], day, l2rec->l1rec->lat[ip],
                qual_sst4[ip], flags_sst4[ip], l2rec->l1rec->glint_coef[ip],
                input->viirsnv7, &bias_sst4[ip], &stdv_sst4[ip],
                &bias_mean_sst4[ip], &bias_counts_sst4[ip]);
    }
}

/* --------------------------------------------------------------------------------------------    */
/* set_sses_sstv6() - set sensor-specific error stats for long wave  sea surface temperature       */
/*                                                                                                 */
/* B. Franz, SAIC, August 2005.                                                                    */

/* --------------------------------------------------------------------------------------------    */
void set_sses_sstv6(l2str *l2rec) {

    int32_t npix = l2rec->l1rec->npix;
    int32_t ip, ipb;
    float Bt11;
    float Bt12;
    float dBt;
    double pasutime = l2rec->l1rec->scantime;
    int16_t year, day;
    double sec;
    unix2yds(pasutime, &year, &day, &sec);
    /* set each pixels bias and stdev (assumes sst and qual computed) */
    for (ip = 0; ip < npix; ip++) {
        ipb = ip * NBANDSIR;
        Bt11 = l2rec->l1rec->Bt[ipb + ib11];
        Bt12 = l2rec->l1rec->Bt[ipb + ib12];
        dBt = Bt11 - Bt12;
        /* all avhrr sses files are v6 format */
        /* but a different format with more qualities than modis and viirs */
        if (l2rec->l1rec->l1file->sensorID == AVHRR) {
            comp_sses_sstv6a(&sses_sstv6a, dBt, sstq[csstbox][ip], l2rec->l1rec->solz[ip],
                    l2rec->l1rec->senz[ip], day, l2rec->l1rec->lat[ip], qual_sst[ip],
                    flags_sst[ip], &bias_sst[ip], &stdv_sst[ip],
                    &bias_mean_sst[ip], &bias_counts_sst[ip]);
        } else { //MODIS and VIIRSN
            if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
                comp_sses_sstv6v(&sses_sstv6v, dBt, sstq[csstbox][ip], l2rec->l1rec->solz[ip],
                        l2rec->l1rec->senz[ip], day, l2rec->l1rec->lat[ip], qual_sst[ip],
                        flags_sst[ip], l2rec->l1rec->glint_coef[ip],
                        input->viirsnv7, &bias_sst[ip], &stdv_sst[ip],
                        &bias_mean_sst[ip], &bias_counts_sst[ip]);
            } else { //MODIS
                comp_sses_sstv6m(&sses_sstv6m, dBt, sstq[csstbox][ip], l2rec->l1rec->solz[ip],
                        l2rec->l1rec->senz[ip], day, l2rec->l1rec->lat[ip], qual_sst[ip],
                        flags_sst[ip], l2rec->l1rec->glint_coef[ip],
                        input->viirsnv7, &bias_sst[ip], &stdv_sst[ip],
                        &bias_mean_sst[ip], &bias_counts_sst[ip]);
            }
        }
    }
}

/* -------------------------------------------------------------------------------*/
/* set_flags_sst3() - set quality flags for viirs jpss triple window algorithm    */
/*                                                                                */

/* -------------------------------------------------------------------------------*/
void set_flags_sst3(l2str *l2rec) {

    int32_t npix = l2rec->l1rec->npix;
    int32_t ip, ipbir;
    float Bt37;
    float Bt40;
    float Bt11;
    float Bt12;
    float dSST3_ref; /* sst3 - ref */
    float dSST_SST3; /* sst - sst3 */
    float dBt_37_11, dBt_11_12; // brightness temp diffs

    /* check each pixel in scan */
    for (ip = 0; ip < npix; ip++) {

        /* SST not processed */
        if (sstmasked(l2rec->l1rec->flags, ip)) {
            flags_sst3[ip] |= SSTF_ISMASKED;
            continue;
        }

        ipbir = ip * NBANDSIR;
        Bt37 = l2rec->l1rec->Bt[ipbir + ib37]; /* modis chan 21, avhrr 3b,   viirs m12 */
        Bt40 = l2rec->l1rec->Bt[ipbir + ib40]; /* modis chan 23,             viirs m13 */
        Bt11 = l2rec->l1rec->Bt[ipbir + ib11]; /* modis chan 31, avhrr cen4, viirs m15 */
        Bt12 = l2rec->l1rec->Bt[ipbir + ib12]; /* modis chan 32, avhrr cen5, viirs m16 */

        /* BT could not be computed (radiance out-of-range) */
        /* sst3 is night only so don't need the clutter of checking Bt37 at night only or glint */
        if (Bt11 < BT_LO + 0.1 || Bt11 > BT_HI - 0.1 || Bt12 < BT_LO + 0.1
                || Bt12 > BT_HI - 0.1 || Bt37 < BT_LO + 0.1
                || Bt37 > BT_HI - 0.1) {
            flags_sst3[ip] |= SSTF_BTBAD;
            continue;
        }

        /* check BT range */
        if (Bt11 < Btmin || Bt11 > Btmax || Bt12 < Btmin || Bt12 > Btmax
                || Bt37 < Btmin || Bt37 > Btmax || Bt40 < Btmin
                || Bt40 > Btmax40) {
            flags_sst3[ip] |= SSTF_BTRANGE;
        }

        /* check BT diff */
        dBt_11_12 = Bt11 - Bt12;
        if (dBt_11_12 < dBtmin || dBt_11_12 > dBtmax)
            flags_sst3[ip] |= SSTF_BTDIFF;

        /* check SST range */
        if (sst3q[csstbox][ip] < SSTmin || sst3q[csstbox][ip] > SSTmaxn)
            flags_sst3[ip] |= SSTF_SSTRANGE;

        /* check SST difference with references */
        dSST3_ref = sst3q[csstbox][ip] - l2rec->l1rec->sstref[ip];

        /* set the flag bit if sst is too much colder than reference */
        if (dSST3_ref < -SSTdiff || l2rec->l1rec->sstref[ip] < sstbad + 1.0) {
            flags_sst3[ip] |= SSTF_SSTREFDIFF;
        }
        // the "coldonly" and equatorial aerosol tests are to be run by default, but if SSTMODS is set, don't
        if ((evalmask & SSTMODS) == 0) {
            if (dSST3_ref < -input->sstrefdif &&
                    l2rec->l1rec->lat[ip] >= equatorialSouth && l2rec->l1rec->lat[ip] <= equatorialNorth &&
                    l2rec->l1rec->lon[ip] >= equatorialWest && l2rec->l1rec->lon[ip] <= equatorialEast) {
                /* tighter test for avhrr between 10S and 30N and -105 to 105 longitude */
                /* equatorial aerosol test */
                flags_sst3[ip] |= SSTF_SSTREFDIFF;
            }
        }
        /* set the flag bit if sst3  is too much warmer than reference at night */
        if (l2rec->l1rec->solz[ip] >= solznight) {
            if (fabs(dSST3_ref) > SSTdiff)
                flags_sst3[ip] |= SSTF_SSTREFDIFF;
        }
        /* is sst3 way too much colder than reference? */
        if (dSST3_ref < -SSTvdiff || l2rec->l1rec->sstref[ip] < sstbad + 1.0)
            flags_sst3[ip] |= SSTF_SSTREFVDIFF;

        /* set the flag bit if sst is way too much warmer than reference at night */
        if (l2rec->l1rec->solz[ip] >= solznight) {
            if (fabs(dSST3_ref) > SSTvdiff)
                flags_sst3[ip] |= SSTF_SSTREFVDIFF;
        }

        /* check SST3 difference with 11um SST */
        dSST_SST3 = sstq[csstbox][ip] - sst3q[csstbox][ip];
        if (sstq[csstbox][ip] > sstbad + 1.0) {
            if (fabs(dSST_SST3) > SST3diff1)
                flags_sst3[ip] |= SSTF_SST3DIFF;
            if (fabs(dSST_SST3) > SST3diff2)
                flags_sst3[ip] |= SSTF_SST3VDIFF;
        }

        /* check sensor zenith limits */
        if (l2rec->l1rec->senz[ip] > hisenz)
            flags_sst3[ip] |= SSTF_HISENZ;
        if (l2rec->l1rec->senz[ip] > vhisenz)
            flags_sst3[ip] |= SSTF_VHISENZ;
        // flag 2 edge pixels as SSTF_VHISENZ so quality gets set to 3
        if (l2rec->l1rec->pixnum[ip] < 2 || l2rec->l1rec->pixnum[ip] > (fullscanpix - 3))
            flags_sst3[ip] |= SSTF_VHISENZ;

        /* check homogeneity of BT */
        if (Bt37_maxmin[ip] > Bt37unif1)
            flags_sst3[ip] |= SSTF_BTNONUNIF;
        if (Bt37_maxmin[ip] > Bt37unif2)
            flags_sst3[ip] |= SSTF_BTVNONUNIF;

        if (Bt11_maxmin[ip] > Bt11unif1)
            flags_sst3[ip] |= SSTF_BTNONUNIF;
        if (Bt11_maxmin[ip] > Bt11unif2)
            flags_sst3[ip] |= SSTF_BTVNONUNIF;

        if (Bt12_maxmin[ip] > Bt12unif1)
            flags_sst3[ip] |= SSTF_BTNONUNIF;
        if (Bt12_maxmin[ip] > Bt12unif2)
            flags_sst3[ip] |= SSTF_BTVNONUNIF;
        /* end of homogeneity checks */

        /* values from Kay's VIIRS trees */
        dBt_37_11 = Bt37 - Bt11;
        //m14 = Bt85; /* SVM14 is 8.55 um */

        /* --------------------------------------------------------------------------*/
        /* decision_tree() - Tree models are based on binary recursive partitioning  */
        /*                   to indicate whether data are potentially contaminated.  */
        /*                   based on charts sent to NODC from Kay Kilpatrick        */
        /*                   worked on by  Vicky Lin, SAIC, October 2007.            */
        /*                   V6 trees from Guillermo Podesta.                        */
        /* ------------------------------------------------------------------------- */

        /* v6.4 night tree */
        /* spectral */
        treesum[ip] = 0.466;
        /* spectral */
        if (!((Bt37 > Bt11) && (Bt11 > Bt12))) {
            treesum[ip] += -1.995;
        } else {
            treesum[ip] += 0.55;
            if (sstq[csstbox][ip] < (292.504 - CtoK)) {
                treesum[ip] += -0.368;
                if (dBt_37_11 < 1.898) {
                    treesum[ip] += 0.091;
                } else {
                    treesum[ip] += -0.787;
                }
                if (sstq[csstbox][ip] < (276.138 - CtoK)) {
                    treesum[ip] += -0.505;
                } else {
                    treesum[ip] += 0.09;
                }
            } else {
                treesum[ip] += 0.441;
            }
            if (dSST_SST3 < -0.567) {
                treesum[ip] += -0.841;
            } else {
                treesum[ip] += 0.177;
                if (dSST_SST3 < 0.484) {
                    treesum[ip] += 0.067;
                    if (dSST_SST3 < -0.216) {
                        treesum[ip] += -0.313;
                    } else {
                        treesum[ip] += 0.085;
                    }
                } else {
                    treesum[ip] += -0.516;
                }
                if (dBt_37_11 < 0.289) {
                    treesum[ip] += -0.705;
                } else {
                    treesum[ip] += 0.066;
                }
            }
        }
        if (sst3q[csstbox][ip] < (270.023 - CtoK)) {
            treesum[ip] += -4.116;
        } else {
            treesum[ip] += 0.047;
        }
        if (Bt37_stdev[ip] < 0.266) {
            treesum[ip] += 0.191;
        } else {
            treesum[ip] += -0.255;
        }
        if (treesum[ip] <= 0.0) {
            flags_sst3[ip] |= SSTF_CLOUD;
        }
        /* end case for sst3 */
    } /* End of pixel loop */
    return;
}

/* ------------------------------------------------------------------------------------*/
/* set_qual_sst3() - set quality levels for viirs jpss triple window algorithm        */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ------------------------------------------------------------------------------------*/
void set_qual_sst3(l2str *l2rec) {
    int32_t ip;

    for (ip = 0; ip < l2rec->l1rec->npix; ip++) {

        if (l2rec->l1rec->solz[ip] < solznight) {

            /* daytime SST3 quality level */

            /* daytime jpss triple window SST quality level */

            if ((flags_sst3[ip] & SSTF_ISMASKED) > 0
                    || (flags_sst3[ip] & SSTF_BTBAD) > 0) {

                qual_sst3[ip] = 4;

            } else {

                qual_sst3[ip] = 3;

            }

        } else {

            /* night sst3 quality */

            if ((flags_sst3[ip] & SSTF_ISMASKED) > 0
                    || (flags_sst3[ip] & SSTF_BTBAD) > 0) {

                qual_sst3[ip] = 4;

            } else if ((flags_sst3[ip] & SSTF_BTRANGE) > 0
                    || (flags_sst3[ip] & SSTF_SSTRANGE) > 0
                    || (flags_sst3[ip] & SSTF_CLOUD) > 0
                    || (flags_sst3[ip] & SSTF_BTVNONUNIF) > 0
                    || (flags_sst3[ip] & SSTF_VHISENZ) > 0
                    || (flags_sst3[ip] & SSTF_SSTREFVDIFF) > 0) {
                /* BTVNONUNIF and VHISENZ are very bad */

                qual_sst3[ip] = 3;

            } else if (((input->viirsnv7 >= 1)
                    && ((flags_sst3[ip] & SSTF_BTNONUNIF) > 0))
                    || ((input->viirsnv7 >= 1)
                    && ((flags_sst3[ip] & SSTF_SST3DIFF) > 0))
                    || (flags_sst3[ip] & SSTF_SSTREFDIFF) > 0
                    || (flags_sst3[ip] & SSTF_SST3VDIFF) > 0) {
                /* BTNONUNIF is q2 to match sst day and night */

                qual_sst3[ip] = 2;

            } else if (((input->viirsnv7 <= 0)
                    && ((flags_sst3[ip] & SSTF_BTNONUNIF) > 0))
                    || ((input->viirsnv7 <= 0)
                    && ((flags_sst3[ip] & SSTF_SST3DIFF) > 0))
                    || (flags_sst3[ip] & SSTF_HISENZ) > 0) {

                qual_sst3[ip] = 1;

            } else {

                qual_sst3[ip] = 0;

            }
        }

    }
    return;
}

/* ----------------------------------------------------------------------------------------------- */
/* read_sst3_coeff() - reads viirs jpss triple window  sst coefficients for the specified date    */
/*                                                                                                 */
/* B. Franz, SAIC, 11 August 2005                                                                  */

/* ----------------------------------------------------------------------------------------------- */
void read_sst3_coeff(l2str *l2rec, float **bounds, float **coef,
        float sstrefoff, size_t *numbands) {
    char mission[5];
    char mission2[5];

    FILE *fp;
    char line[200];
    char name[80];
    char value[80];
    char *p;
    char *p1;
    char *p2;
    char odatel[14] = ""; /* yyyydddhhmmss */
    char sdatel[14] = ""; /* yyyydddhhmmss */
    char edatel[14] = ""; /* yyyydddhhmmss */
    char sdates[8]; /* yyyyddd */
    char edates[8]; /* yyyyddd */
    char stime[7]; /* hhmmss */
    char etime[7]; /* hhmmss */
    char dorn[2];
    char *ztime;
    int found = 0;
    //    int tmonth;
    int month;
    int indx;
    int32 gotsstrefoff = 0;
    int32 leap;
    double pasutime = l2rec->l1rec->scantime;
    int16_t year, day;
    double sec;
    unix2yds(pasutime, &year, &day, &sec);

    strcpy(mission, "VIIR");

    /* Open the file */
    if ((fp = fopen(input->sst3coeffile, "r")) == NULL) {
        fprintf(stderr, "-E- %s line %d: unable to open %s for reading\n",
                __FILE__, __LINE__, input->sst3coeffile);
        exit(1);
    }

    indx = 0;
    if(6 > *numbands) {
        printf("ERROR - number_of_latbands is too big, reading %d into %ld\n", 6, *numbands);
        exit(EXIT_FAILURE);
    }
    *numbands = 6; /* default to 6 latitude bands */

    /* VIIRS sst3 OSI-SAF coeffs are this format: */
    /*   sat, start yyyyddd,, end yyyyddd, day/night, c0, c1, c2, c3 c4 c5 */
    /*   VIIR 2011001 0000000 N -1.22636 1.00787 0.0314639 0.934653 0.255025 -7.79800 ! skin, night */
    /* VIIRS sst3 v6.4.1 latband coeffs are this format: */
    /*   sat, month, min lat, max lat, c0, c1, c2, c3 c4 c5 */
    /*   VIIR 1 -90 -40 5.2152935 0.9856523 0.1053436 1.2018229 -4.59e-05 0.0001334 */
    /* VIIRS sst3 v7 latband coeffs are this format: */
    /*   sat, start yyyyddd, start hhmmss, end yyyyddd, end hhmmss, min lat, max lat, c0, c1, c2, c3, c4, power */
    /*   VIIR 2012001 000000 2012031 235959 -90 -40 -6.7764418603 1.0273867073 0.0035144672 0.8870043776 0.211230466 2 */

    /* Form date string */

    if (input->viirsnv7 == -1) {
        /* Find month for viirs v6.4.1 */
        leap = (isleap(year) == TRUE ? 1 : 0);
        if (tmonth < 0) {
            for (tmonth = 11; tmonth >= 0; tmonth--) {
                /* day is one based, StartOfMonth is zero based */
                if (day > StartOfMonth[leap][tmonth]) {
                    break;
                }
            }
        }
    } else {
        /* find date */
        /* msec is the scan line start time */
        /* should calculate the actual pixel time (extrapolate from msec) for field 57? */
        /* modis, viirs, and avhhr: year, day are per scan line, not just time of first scan */
    //    pasutime = yds2unix(year, day, ((double) (msec)) / 1000.0);
        ztime = ydhmsf(pasutime, 'G');
        /* ztime is yyyydddhhmmssfff */
        strncpy(odatel, ztime, 13);
        odatel[13] = '\0';
    }

    /* Loop through to find bounding times */

    while (fgets(line, 200, fp)) {
        if (line[0] == '#') {
            /* look for lines with: # variable = value */
            if (!(p = strchr(line, '=')))
                continue;
            p1 = line + 1; /* look for white space after # and before variable name */
            while (isspace(*p1))
                p1++;
            p2 = p - 1; /* look for white space before = and after variable name */
            while (isspace(*p2))
                p2--;
            /* get variable name from input line */
            strncpy(name, p1, p2 - p1 + 1);
            name[p2 - p1 + 1] = '\0';

            /*
             * Parse parameter value string
             */
            /* start at character after = and ignore white space before value */
            p1 = p + 1;
            while (isspace(*p1))
                p1++;
            p2 = p1;
            /* look for white space to determine end of value */
            while (!isspace(*p2))
                p2++;
            /* get value from input line */
            strncpy(value, p1, p2 - p1);
            value[p2 - p1] = '\0';
            /*
             * Copy value to appropriate variable
             */
            if (strcmp(name, "sstref_offset") == 0
                    || strcmp(name, "sstref_night_offset") == 0) {
                sstrefoff = (float) atof(value);
                gotsstrefoff = 1;
            } else if (strcmp(name, "number_of_latbands") == 0) {
                size_t nbands = (size_t) atoi(value);
                if(nbands > *numbands) {
                    printf("ERROR - number_of_latbands is too big, reading %ld into %ld\n", nbands, *numbands);
                    exit(EXIT_FAILURE);
                }
                *numbands = nbands;
            }

        } else {
            if (strncmp(line, mission, 4) == 0) {
                if (input->viirsnv7 < 0) {
                    /* read monthly sst latband coeffs for VIIRS v6.4.1 */
                    if (strncmp(line, mission, 4) == 0) {
                        sscanf(line, "%4s %d %f %f %f %f %f %f %f %f",
                                mission2, &month, &bounds[indx][0], &bounds[indx][1],
                                &coef[indx][0], &coef[indx][1], &coef[indx][2],
                                &coef[indx][3], &coef[indx][4], &coef[indx][5]);
                        coef[indx][6] = 0;
                    }
                    if (month == tmonth + 1) {
                        indx++;
                        if (indx == *numbands) {
                            found = 1;
                            break;
                        }
                    }
                } else {
                    /* find coeffs for correct date */
                    if (input->viirsnosisaf == 1) {
                        sscanf(line, "%4s %7s %7s %1s %f %f %f %f %f %f %f",
                                mission2, sdates, edates, dorn,
                                &coef[0][0], &coef[0][1], &coef[0][2], &coef[0][3],
                                &coef[0][4], &coef[0][5], &coef[0][6]);
                        sprintf(sdatel, "%s%s", sdates, "000000");
                        sprintf(edatel, "%s%s", edates, "000000");
                    } else {
                        if (input->viirsnv7 >= 1) {
                            sscanf(line, "%4s %7s %6s %7s %6s %f %f %f %f %f %f %f %f",
                                    mission2, sdates, stime, edates, etime,
                                    &bounds[indx][0], &bounds[indx][1], &coef[indx][0],
                                    &coef[indx][1], &coef[indx][2], &coef[indx][3],
                                    &coef[indx][4], &coef[indx][5]);
                            coef[indx][6] = 0.0;
                        } else {
                            sscanf(line, "%4s %7s %6s %7s %6s %f %f %f %f %f %f",
                                    mission2, sdates, stime, edates, etime,
                                    &bounds[indx][0], &bounds[indx][1], &coef[indx][0],
                                    &coef[indx][1], &coef[indx][2], &coef[indx][3]);
                            coef[indx][4] = 0.0;
                            coef[indx][5] = 0.0;
                            coef[indx][6] = 0.0;
                        }
                    }
                    sprintf(sdatel, "%s%s", sdates, stime);
                    sprintf(edatel, "%s%s", edates, etime);
                    if (strcmp(odatel, sdatel) >= 0
                            && (strcmp(odatel, edatel) <= 0
                            || strcmp(edates, "0000000") == 0
                            || strcmp(edates, "") == 0)) {
                        indx++;
                        if (indx == *numbands) {
                            found = 1;
                            break;
                        }
                    }
                }
            }
        }
    }

    fclose(fp);

    if (found == 1) {

        if (gotsstrefoff == 0) {
            fprintf(stderr,
                    "-E- %s line %d: No sst reference offset found in %s\n",
                    __FILE__, __LINE__, input->sst3coeffile);
            exit(1);
        }

        printf("Loading SST3 lat band coefficients from %s:\n",
                input->sst3coeffile);

        if (input->viirsnosisaf == 1) {
            printf("%s %s %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f\n",
                    sdates, edates,
                    coef[0][0], coef[0][1], coef[0][2],
                    coef[0][3], coef[0][4], coef[0][5]);
        } else {
            for (indx = 0; indx<*numbands; indx++) {
                if (input->viirsnv7 < 0) {
                    printf("%d %6.1f %6.1f %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f\n",
                            month,
                            bounds[indx][0], bounds[indx][1],
                            coef[indx][0], coef[indx][1], coef[indx][2], coef[indx][3],
                            coef[indx][4], coef[indx][5]);
                } else {
                    printf("%s %s %s %s %6.1f %6.1f %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f\n",
                            sdates, stime, edates, etime,
                            bounds[indx][0], bounds[indx][1],
                            coef[indx][0], coef[indx][1], coef[indx][2], coef[indx][3],
                            coef[indx][4], coef[indx][5]);
                }
            }
        }
        printf(" sst reference offset = %f\n", sstrefoff);

    } else {
        fprintf(stderr,
                "-E- %s line %d: unable to locate valid SST3 coefficients for %s in %s\n",
                __FILE__, __LINE__, odatel, input->sst3coeffile);

        exit(EXIT_FAILURE);
    }

    return;
}

/* ----------------------------------------------------------------------------------- */
/* nlsst3() - VIIRS SST Triple Window Algorithm (going into sst4 field)          */
/*                                                                                     */
/* S. Walsh, RSMAS, Nov. 2011                                                          */

/* ----------------------------------------------------------------------------------- */
float nlsst3(float Bt37, float Bt11, float Bt12, int32_t is, float sstref,
        float **bounds, float **coef, float sstrefoff, size_t numbands,
        int32_t ip) {

    extern l1qstr l1que;
    int ic;
    float senz = l1que.r[is].senz[ip];
    float lat = l1que.r[is].lat[ip];
    float mu = l1que.r[is].csenz[ip];

    float dBt_37_12 = Bt37 - Bt12;
    float sstlo, ssthi;
    float dBtlo, dBthi;
    float lsst3;
    float latpow;
    float satzdir;

    /* this routine is only for viirs so don't need to check sensorID */

    Bt11 += CtoK;

    /* convert sstref to K */
    sstref = sstref + sstrefoff;

    /* sstref is supposed to warm the retrieved temperature, not cool it */
    /* this is the scaling factor for temp deficit term */
    /* not a problem for deg F or K, but deg C can go negative and we don't want that */
    if (sstref < 0.0) {
        sstref = 0.0;
    }

    /* bounds go from -90 to 90 so we want the first set */
    /* that has max lat greater than pixel lat           */
    ic = numbands-1;
    while ((ic > 0) && (lat <= bounds[ic - 1][1] + latwin)) {
        ic--;
    }

    if (input->viirsnv7 == 2) {
        /* v7.2 high satz latband coeffs */
        /* choose coeffs by latband */

        latpow = (4.018114e-04 * pow(lat, 2))+ (1.968591e-18 * lat)
                + 3.197907e-01;
        if (lat < bounds[ic][1] - latwin || ic == (numbands-1)) {
            lsst3 = coef[ic][0] + coef[ic][1] * Bt11 + coef[ic][2] * dBt_37_12 * sstref
                    + coef[ic][3] * ((1.0 / mu) - 1.0)
                    + coef[ic][4] * pow(((1.0 / mu) - 1.0), latpow)
                    + coef[ic][5] * lat * ((1.0 / mu) - 1.0);
        } else {
            dBtlo = bounds[ic][1] - latwin;
            dBthi = bounds[ic][1] + latwin;
            sstlo = coef[ic][0] + coef[ic][1] * Bt11 + coef[ic][2] * dBt_37_12 * sstref
                    + coef[ic][3] * ((1.0 / mu) - 1.0)
                    + coef[ic][4] * pow(((1.0 / mu) - 1.0), latpow)
                    + coef[ic][5] * lat * ((1.0 / mu) - 1.0);
            ssthi = coef[ic + 1][0] + coef[ic + 1][1] * Bt11
                    + coef[ic + 1][2] * dBt_37_12 * sstref
                    + coef[ic + 1][3] * ((1.0 / mu) - 1.0)
                    + coef[ic + 1][4] * pow(((1.0 / mu) - 1.0), latpow)
                    + coef[ic + 1][5] * lat * ((1.0 / mu) - 1.0);
            lsst3 = sstlo + ((lat - dBtlo) / (dBthi - dBtlo)) * (ssthi - sstlo);

        }
    } else if (input->viirsnv7 == 1) {
        /* v7 high satz latband coeffs */
        /* choose coeffs by latband */

        if (lat < bounds[ic][1] - latwin || ic == (numbands-1)) {
            lsst3 = coef[ic][0] + coef[ic][1] * Bt11 + coef[ic][2] * dBt_37_12 * sstref
                    + coef[ic][3] * ((1.0 / mu) - 1.0)
                    + coef[ic][4] * pow(((1.0 / mu) - 1.0), coef[ic][5]);
        } else {
            dBtlo = bounds[ic][1] - latwin;
            dBthi = bounds[ic][1] + latwin;
            sstlo = coef[ic][0] + coef[ic][1] * Bt11 + coef[ic][2] * dBt_37_12 * sstref
                    + coef[ic][3] * ((1.0 / mu) - 1.0)
                    + coef[ic][4] * pow(((1.0 / mu) - 1.0), coef[ic][5]);
            ssthi = coef[ic + 1][0] + coef[ic + 1][1] * Bt11
                    + coef[ic + 1][2] * dBt_37_12 * sstref
                    + coef[ic + 1][3] * ((1.0 / mu) - 1.0)
                    + coef[ic + 1][4] * pow(((1.0 / mu) - 1.0), coef[ic + 1][5]);
            lsst3 = sstlo + ((lat - dBtlo) / (dBthi - dBtlo)) * (ssthi - sstlo);

        }
    } else if (input->viirsnosisaf == 1) {
        /* OSI-SAF equation: Ts = a0 + (a1 + a2 Stheta) T3.7 + (a3 + a4 Stheta) dT11-12 + a5 Stheta */
        lsst3 = coef[0][0]
                + (coef[0][1] + coef[0][2]*((1.0 / mu) - 1.0)) * Bt37
                + (coef[0][3]* + coef[0][4]*((1.0 / mu) - 1.0)) * dBt_37_12
                + coef[0][5]*((1.0 / mu) - 1.0);
    } else {

        /* v6 latband coeffs */
        /* choose coeffs by latband */

        /* this WILL now work if working on a subset of the data (spixl or epixl were specified) */
        satzdir = (l1que.r[is].pixnum[ip] < fullscanpix / 2) ? -1.0 : 1.0;

        if (lat < bounds[ic][1] - latwin || ic == (numbands-1)) {
            lsst3 = coef[ic][0] + coef[ic][1] * Bt11 + coef[ic][2] * dBt_37_12 * sstref
                    + coef[ic][3]*((1.0 / mu) - 1.0)
                    + coef[ic][4]*(senz * satzdir)
                    + coef[ic][5] * pow((senz * satzdir), 2);
        } else {
            dBtlo = bounds[ic][1] - latwin;
            dBthi = bounds[ic][1] + latwin;
            sstlo = coef[ic][0] + coef[ic][1] * Bt11 + coef[ic][2] * dBt_37_12 * sstref
                    + coef[ic][3]*((1.0 / mu) - 1.0)
                    + coef[ic][4]*(senz * satzdir)
                    + coef[ic][5] * pow((senz * satzdir), 2);
            ssthi = coef[ic + 1][0] + coef[ic + 1][1] * Bt11 + coef[ic + 1][2] * dBt_37_12 * sstref
                    + coef[ic + 1][3]*((1.0 / mu) - 1.0) +
                    + coef[ic + 1][4]*(senz * satzdir) +
                    + coef[ic + 1][5] * pow((senz * satzdir), 2);
            lsst3 = sstlo + ((lat - dBtlo) / (dBthi - dBtlo)) * (ssthi - sstlo);
        }
    }

    lsst3 -= CtoK; /* internally, want all sst's in Deg C */

    return (lsst3);
}

/* ----------------------------------------------------------------------------------- */
/* comp_sst3() - compute jpss triple window sea surface temperature                    */
/*                                                                                     */
/* B. Franz, SAIC, August 2003.                                                        */

/* ----------------------------------------------------------------------------------- */
void comp_sst3(l2str *l2rec, int32_t is) {
    /* use l2rec for the general stuff, but use l1rec for the scan being processed */
    /*   l1rec may be ahead or behind l2rec to make the box needed for statistics */
    extern l1qstr l1que;
    static int firstCall = 1;

    static float **coef;
    static float **bounds;
    static size_t nlatbands = 0;
    static size_t ncoefficients = 0;
    static float sstrefoff;

    int32_t i, j, ip, ipb;
    float Bt37;
    float Bt11;
    float Bt12;
    float sstref;

    if (firstCall) {
        if (input->proc_sst != 1) {
            printf(
                    "-E- %s line %d: SST processing must be enabled (proc_sst=1) to make sst3.\n",
                    __FILE__, __LINE__);
            exit(1);
        }
        if (input->sst3coeffile != NULL) {
            sstrefoff = 0.0; /* default to 0.0 */
            size_t nmonth = 1;
            int32_t nlats = 2;
            int32_t latbandDimID, coeffDimID, boundsID, coefficientID;
            int32_t ncid;
            if (nc_open(input->sst3coeffile, NC_NOWRITE, &ncid) == NC_NOERR) {
                printf("Loading SST3 lat band coefficients from %s:\n", input->sst3coeffile);

                if (nc_inq_dimid(ncid, "latband", &latbandDimID) != NC_NOERR){
                    printf("Whoops! something is wrong reading the SST algorithm coefficient file: %s\n",input->sst3coeffile);
                    exit(EXIT_FAILURE);
                }
                nc_inq_dimlen(ncid, latbandDimID, &nlatbands);
                if (nc_inq_dimid(ncid, "coefficient", &coeffDimID) != NC_NOERR){
                    printf("Whoops! something is wrong reading the SST algorithm coefficient file: %s\n",input->sst3coeffile);
                    exit(EXIT_FAILURE);
                }
                nc_inq_dimlen(ncid, coeffDimID, &ncoefficients);

                bounds = allocate2d_float(nlatbands, nlats);
                if (nc_inq_varid (ncid, "latbands", &boundsID) == NC_NOERR){
                    nc_get_var_float(ncid, boundsID, bounds[0]);
                }

                int16_t year, month, day;
                double sec;
                unix2ymds(l2rec->l1rec->scantime, &year, &month, &day, &sec);
                month -= 1;

                coef = allocate2d_float(nlatbands, ncoefficients);
                if (nc_inq_varid (ncid, "coefficients", &coefficientID) == NC_NOERR){
                    size_t start[] = {month, 0, 0}; /* start at first value */
                    size_t count[] = {nmonth, nlatbands, ncoefficients};
                    nc_get_vara_float(ncid, coefficientID, start, count, coef[0]);
                }

                nc_close(ncid);
                // print coefficients to be used
                for (i = 0; i < nlatbands; i++){
                    printf("%d %6.2f %6.2f ", month+1, bounds[i][0], bounds[i][1]);
                    for (j = 0; j < ncoefficients; j++){
                        printf("%9.6f ", coef[i][j]);
                    }
                    printf("\n");
                }
            } else {

                nlatbands = 6;
                ncoefficients = 7;
                coef = allocate2d_float(nlatbands, ncoefficients);
                bounds = allocate2d_float(nlatbands, nlats);

                read_sst3_coeff(l2rec, bounds, coef, sstrefoff, &nlatbands);
            }
        } else {
            printf("SST3 coefficient file not specified.");
            exit(1);
        }
        firstCall = 0;
    }

    for (ip = 0; ip < l2rec->l1rec->npix; ip++) {

        sst3q[is][ip] = sstbad;
        flags_sst3[ip] = 0;

        /* skip if pixel already masked */
        if (sstmasked(l1que.r[is].flags, ip))
            continue;

        ipb = ip * NBANDSIR;
        sstref = l1que.r[is].sstref[ip];
        Bt37 = l1que.r[is].Bt[ipb + ib37];
        Bt11 = l1que.r[is].Bt[ipb + ib11];
        Bt12 = l1que.r[is].Bt[ipb + ib12];

        /* don't bother processing if no sstref */
        if (sstref < sstbad + 1.0) {
            continue;
        }

        /* compute SST3 */
        if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
            /* skip pixel if BT could not be computed */
            if (Bt37 < BT_LO + 0.1 || Bt37 > BT_HI - 0.1 || Bt11 < BT_LO + 0.1
                    || Bt11 > BT_HI - 0.1 || Bt12 < BT_LO + 0.1
                    || Bt12 > BT_HI - 0.1) {
                continue;
            }
            if(ncoefficients < 6) {
                printf("ERROR - nlsst3 needs ncoefficients >= 6, we found %ld\n", ncoefficients);
                exit(EXIT_FAILURE);
            }
            sst3q[is][ip] = nlsst3(Bt37, Bt11, Bt12, is, sstref, bounds, coef,
                    sstrefoff, nlatbands, ip);
        }

    }

    return;
}

/* ----------------------------------------------------------------------------------- */
/* run_sst() - run the SST algorithm over full scan and store in static arrays         */
/*                                                                                     */
/* Note: the 11um SST is using 4um SST as input (when we have it), so order matters.   */
/* Similarly, flags can't be computed until both SST products are computed.            */
/*                                                                                     */
/* B. Franz, SAIC, May 2005.                                                           */

/* ----------------------------------------------------------------------------------- */
void run_sst(l2str *l2rec) {
    static int firstCall = 1;
    extern l1qstr l1que;
    int32_t is = l1que.nq / 2;
    int32_t is1, is2, iq;

    int32_t ij;

    if (firstCall) {
        init_sst(l2rec);
        firstCall = 0;
    }

    /* 'is' is always 3, the center of the l1que.nq box which contains 7 scans */
    /* so for viirs with btbox=5, is1 is always 1, and is2 is always 5 which are indecis into the 0..6 l1que array */
    is1 = MIN(MAX(0, is - btbox / 2), l1que.nq - 1);
    is2 = MAX(MIN(l1que.nq - 1, is + btbox / 2), 0);

    // Set up max-min arrays
    run_rhoCboxmaxmin(l2rec->l1rec->npix, rhoCirrus_maxmin, rhoCirrus_min, rhoCirrus_max);

    if (haveRed) {
        /* ibred = 9 for modisa */
        run_rhotboxmaxmin(l2rec->l1rec->npix, ibred, l2rec->l1rec->l1file->nbands, rhotRED_maxmin, rhotRED_max);
        run_rhotboxmin(l2rec->l1rec->npix, ibred, l2rec->l1rec->l1file->nbands, rhotRED_min);
    }
    if (ib07 >= 0) {
        /* ib07 = 10 for modisa */
        run_rhotboxmin(l2rec->l1rec->npix, ib07, l2rec->l1rec->l1file->nbands, rhotNIR7_min);
    }
    if (ib16 >= 0) {
        /* ib16 = 14 for modisa */
        run_rhotboxmin(l2rec->l1rec->npix, ib16, l2rec->l1rec->l1file->nbands, rhot16_min);
    }
    if (ib11 >= 0) {
        /* ib11 = 6 for modisa */
        run_btboxmaxmin(l2rec->l1rec->npix, ib11, Bt11_maxmin);
        run_btboxmax(l2rec->l1rec->npix, ib11, Bt11_max);
        run_btboxmin(l2rec->l1rec->npix, ib11, Bt11_min);
        run_btboxstdev(l2rec->l1rec->npix, ib11, Bt11_stdev);
    }
    if (ib12 >= 0) {
        /* ib12 = 7 for modisa */
        run_btboxmaxmin(l2rec->l1rec->npix, ib12, Bt12_maxmin);
        run_btboxmin(l2rec->l1rec->npix, ib12, Bt12_min);
    }
    if (ib37 >= 0) {
        /* ib37 = 0 for modisa */
        run_btboxmaxmin(l2rec->l1rec->npix, ib37, Bt37_maxmin);
        run_btboxstdev(l2rec->l1rec->npix, ib37, Bt37_stdev);
    }
    if (ib39 >= 0) {
        /* ib39 = 1 for modisa */
        run_btboxmaxmin(l2rec->l1rec->npix, ib39, Bt39_maxmin);
    }
    if (ib40 >= 0) {
        /* ib40 = 2 for modisa */
        run_btboxmaxmin(l2rec->l1rec->npix, ib40, Bt40_maxmin);
        run_btboxstdev(l2rec->l1rec->npix, ib40, Bt40_stdev);
    }
    if (ib73 >= 0) {
        run_btboxmax(l2rec->l1rec->npix, ib73, Bt73_max);
    }
    if (ib85 >= 0) {
        /* ib85 = 5 for modisa */
        run_btboxmin(l2rec->l1rec->npix, ib85, Bt85_min);
    }
    /* compute SST */

    /* make sure scan being processed is the center of sst boxes for cloud tree stats */
    /* only in l2rec, not l1que: fileInfo */
    if (sstboxcscan == -1 || sstboxcscan != l2rec->l1rec->iscan - 1) {
        /* current center scan is not the preceeding scan, so either first call or */
        /* processing out of sequence, so need to calculate sst for all btbox lines */
        for (ij = is1; ij <= is2; ij++) {
            if (haveSST4) {
                comp_sst4(l2rec, ij);
            }
            if (haveSST) {
                if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
                    comp_sst3(l2rec, ij);
                }
                comp_sst(l2rec, ij);
            }
        }
    } else {
        /* re-arrange the record data pointers and compute the next needed scan of sst's */
        if (haveSST4) {
            for (iq = is1 + 1; iq <= is2; iq++)
                memcpy(&sst4q[iq - 1][0], &sst4q[iq][0], sizeof (s_array));
            /* new compute the next needed scan of sst's */
            comp_sst4(l2rec, is2);
        }
        if (haveSST) {
            if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
                for (iq = is1 + 1; iq <= is2; iq++)
                    memcpy(&sst3q[iq - 1][0], &sst3q[iq][0], sizeof (s_array));
                /* new compute the next needed scan of sst's */
                comp_sst3(l2rec, is2);
            }
            for (iq = is1 + 1; iq <= is2; iq++) {
                memcpy(&sstq[iq - 1][0], &sstq[iq][0], sizeof (s_array));
            }
            /* now compute the next needed scan of sst's */
            comp_sst(l2rec, is2);
        }
    }
    sstboxcscan = l2rec->l1rec->iscan;
    
    run_sstboxstdev(l2rec->l1rec->npix, sst_stdev);

    /* compute flags */
    if (haveSST4)
        set_flags_sst4(l2rec);
    if (l2rec->l1rec->l1file->sensorID == VIIRSN)
        set_flags_sst3(l2rec);
    if (haveSST)
        set_flags_sst(l2rec);

    /* set quality levels */
    if (haveSST4)
        set_qual_sst4(l2rec);
    if (l2rec->l1rec->l1file->sensorID == VIIRSN)
        set_qual_sst3(l2rec);
    if (haveSST)
        set_qual_sst(l2rec);

    /* set SSES */
    if (haveSSES) {
        if (haveSST4) {
            set_sses_sst4v6m(l2rec);
        }
        if (haveSST) {
            if (l2rec->l1rec->l1file->sensorID == VIIRSN) {
                set_sses_sst3v6v3(l2rec);

            }
            set_sses_sstv6(l2rec);

        }
    }
    recnumSST = l2rec->l1rec->iscan;
}

/* =================================================================================== */
/*              interface functions for l2_hdf_generic()                               */
/* =================================================================================== */

float *get_sst_dust_correction(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (dsdi_correction);
}
/* ----------------------------------------------------------------------------------- */
/* get_bias_sst() - SSES bias for long wave sea surface temperature                    */
/*                                                                                     */
/* B. Franz, SAIC, January 2006.                                                       */

/* ----------------------------------------------------------------------------------- */
float *get_bias_sst(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (bias_sst);
}

/* ----------------------------------------------------------------------------------- */
/* get_bias_sst4() - SSES bias for short wave sea surface temperature                  */
/*                                                                                     */
/* B. Franz, SAIC, January 2006.                                                       */

/* ----------------------------------------------------------------------------------- */
float *get_bias_sst4(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    if (!haveSST4) {
        printf("short-wave SST is not available for this sensor.\n");
        exit(1);
    }
    return (bias_sst4);

}

/* ----------------------------------------------------------------------------------- */
/* get_bias_sst_triple() - SSES bias for 3 band sea surface temperature                  */
/*                                                                                     */
/* B. Franz, SAIC, January 2006.                                                       */

/* ----------------------------------------------------------------------------------- */
float *get_bias_sst_triple(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (bias_sst3);

}

/* ----------------------------------------------------------------------------------- */
/* get_stdv_sst() - SSES stdv for long wave sea surface temperature                    */
/*                                                                                     */
/* B. Franz, SAIC, January 2006.                                                       */

/* ----------------------------------------------------------------------------------- */
float *get_stdv_sst(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (stdv_sst);
}

/* ----------------------------------------------------------------------------------- */
/* get_counts_sst() - SSES counts for long wave sea surface temperature                */
/*                                                                                     */
/* B. Franz, SAIC, January 2006.                                                       */

/* ----------------------------------------------------------------------------------- */
int16 *get_counts_sst(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);
    return (bias_counts_sst);
}

/* ----------------------------------------------------------------------------------- */
/* get_counts_sst4() - SSES counts for short wave sea surface temperature              */
/*                                                                                     */
/* B. Franz, SAIC, January 2006.                                                       */

/* ----------------------------------------------------------------------------------- */
int16 *get_counts_sst4(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (bias_counts_sst4);
}

/* ----------------------------------------------------------------------------------- */
/* get_counts_sst_triple() - SSES counts for 3 band sea surface temperature              */
/*                                                                                     */
/* B. Franz, SAIC, January 2006.                                                       */

/* ----------------------------------------------------------------------------------- */
int16 *get_counts_sst_triple(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (bias_counts_sst3);
}

/* ----------------------------------------------------------------------------------- */
/* get_stdv_sst4() - SSES stdv for short wave sea surface temperature                  */
/*                                                                                     */
/* B. Franz, SAIC, January 2006.                                                       */

/* ----------------------------------------------------------------------------------- */
float *get_stdv_sst4(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    if (!haveSST4) {
        printf("short-wave SST is not available for this sensor.\n");
        exit(1);
    }
    return (stdv_sst4);
}

/* ----------------------------------------------------------------------------------- */
/* get_stdv_sst_triple() - SSES stdv for 3 band sea surface temperature                  */
/*                                                                                     */
/* B. Franz, SAIC, January 2006.                                                       */

/* ----------------------------------------------------------------------------------- */
float *get_stdv_sst_triple(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (stdv_sst3);
}

/* ----------------------------------------------------------------------------------- */
/* get_qual_sst() - quality levels for long wave sea surface temperature                */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
int8 *get_qual_sst(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (qual_sst);
}

/* ----------------------------------------------------------------------------------- */
/* get_qual_sst4() - quality levels for short wave sea surface temperature             */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
int8 *get_qual_sst4(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    if (!haveSST4) {
        printf("short-wave SST is not available for this sensor.\n");
        exit(1);
    }

    return (qual_sst4);
}

/* ----------------------------------------------------------------------------------- */
/* get_qual_sst_triple() - quality levels for 3 band sea surface temperature             */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
int8 *get_qual_sst_triple(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (qual_sst3);
}

/* ----------------------------------------------------------------------------------- */
/* get_flags_sst() - quality flags for long wave sea surface temperature               */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
int16 *get_flags_sst(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (flags_sst);
}

/* ----------------------------------------------------------------------------------- */
/* get_flags_sst4() - quality flags for short wave sea surface temperature             */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
int16 *get_flags_sst4(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    if (!haveSST4) {
        printf("short-wave SST is not available for this sensor.\n");
        exit(1);
    }

    return (flags_sst4);
}

/* ----------------------------------------------------------------------------------- */
/* get_flags_sst_triple() - quality flags for 3 band sea surface temperature             */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
int16 *get_flags_sst_triple(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (flags_sst3);
}

/* ----------------------------------------------------------------------------------- */
/* get_sst() - return longwave sea surface temperature for current scan                */
/*                                                                                     */
/* B. Franz, SAIC, August 2005.                                                        */

/* ----------------------------------------------------------------------------------- */
float *get_sst(l2str *l2rec) {

    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    if (!haveSST) {
        printf("long-wave SST is not available for this sensor.\n");
        exit(1);
    }

    return (sstq[csstbox]);
}

/* ----------------------------------------------------------------------------------- */
/* get_sst4() - return shortwave sea surface temperature for current scan              */
/*                                                                                     */
/* B. Franz, SAIC, May 2005.                                                           */

/* ----------------------------------------------------------------------------------- */
float *get_sst4(l2str *l2rec) {

    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    if (!haveSST4) {
        printf("short-wave SST is not available for this sensor.\n");
        exit(1);
    }

    return (sst4q[csstbox]);
}

/* ----------------------------------------------------------------------------------- */
/* get_sst_triple() - return shortwave sea surface temperature for current scan              */
/*                                                                                     */
/* B. Franz, SAIC, May 2005.                                                           */

/* ----------------------------------------------------------------------------------- */
float *get_sst_triple(l2str *l2rec) {

    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (sst3q[csstbox]);
}

/* ----------------------------------------------------------------------------------- */
/* get_bias_mean_sst() - SSES bias_mean for long wave sea surface temperature          */
/*                                                                                     */
/* B. Franz, SAIC, January 2006.                                                       */

/* ----------------------------------------------------------------------------------- */
float *get_bias_mean_sst(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (bias_mean_sst);
}

/* ----------------------------------------------------------------------------------- */
/* get_bias_mean_sst4() - SSES bias mean for short wave sea surface temperature        */
/*                                                                                     */
/* B. Franz, SAIC, January 2006.                                                       */

/* ----------------------------------------------------------------------------------- */
float *get_bias_mean_sst4(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    if (!haveSST4) {
        printf("short-wave SST is not available for this sensor.\n");
        exit(1);
    }

    return (bias_mean_sst4);
}

/* ----------------------------------------------------------------------------------- */
/* get_bias_mean_sst_triple() - SSES bias mean for 3 band sea surface temperature        */
/*                                                                                     */
/* B. Franz, SAIC, January 2006.                                                       */

/* ----------------------------------------------------------------------------------- */
float *get_bias_mean_sst_triple(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (bias_mean_sst3);
}

/* ----------------------------------------------------------------------------------- */
/* get_treesum() - decision tree sum value                                             */

/* ----------------------------------------------------------------------------------- */
float *get_sst_treesum(l2str *l2rec) {
    if (!sst_ran(l2rec->l1rec->iscan))
        run_sst(l2rec);

    return (treesum);
}
