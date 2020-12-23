/*
    Modification history:
    Programmer       Organization      Date      Description of change
    --------------   ------------    --------    ---------------------
    Joel Gales       Futuretech      01/31/00    Original Development
    Bryan Franz      GSC             03/01/00    Add reading of LAC 
                                                 pix start & subsamp to 
                                                 readL2meta
    Joel Gales       Futuretech      03/03/00    Change nflag in l2_str
                                                 from byte to int32
                                                 Fix allocation
                                                 problem with prodlist

    Joel Gales       Futuretech      03/14/00    Add "getL3units" routine

    Joel Gales       Futuretech      05/25/00    Fix isalpha (Linux) bug
    Joel Gales       Futuretech      06/14/00    Fix case where there are
                                                 no L2 products (flags only)
    Joel Gales       Futuretech      06/15/00    Fix units problem for flag
                                                 products
    Joel Gales       Futuretech      06/20/00    Add read support for FLOAT32
                                                 data products
    Ewa Kwiatkowska  SAIC            07/28/04    Made prodtype file-dependent
                                                 what enables reading L2 files
                                                 from different sensors
    Joel Gales       Futuretech      08/26/07    Increase buffer in openL2 to 
                                                 read "Processing_Control" 
                                                 attribute.
                                                 Do not read "eng_qual", 
                                                 "s_flags", "nflag" fields if 
                                                 they do not exist
    Joel Gales       Futuretech      08/31/09    Check scaled value for bad 
                                                 value.
                                                 Set to NaN if bad.
                                                 Implemented for INT16 only.

    Joel Gales       Futuretech      03/14/11    Add n_cntl_pnts readjustment
                                                 for bad latitude control pnts

    Joel Gales       Futuretech      07/30/12    Only allocate data products
                                                 cache at beginning or if
                                                 number of data products
                                                 increased.
    Joel Gales       Futuretech      06/14/13    Add support for NETCDF4
    Joel Gales       Futuretech      07/23/13    Fix incomplete idDS structure
                                                 for pixnum, eng_qual, s_flags,
                                                 nflag.

    Joel Gales       Futuretech      02/11/14    Clear bufnum buffer in 
                                                 getL3units before reading
                                                 units attribute
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <netcdf.h>
#include <genutils.h>
#include "readL2scan.h"
#include "navigation.h"
#include <sensorInfo.h>
#include <timeutils.h>
#include <nc4utils.h>

#define TITLE           "title"
#define TITLE_OLD       "Title"
#define INFILES         "source"
#define INFILES_OLD     "Input Files"
#define SENNME          "instrument"
#define SENNME_OLD      "Sensor Name"
#define DCENTER         "project"
#define DCENTER_OLD     "Data Center"
#define NFREC           "Filled Scan Lines"
#define PCTFLAG         "flag_percentages"
#define PCTFLAG_OLD     "Flag Percentages"
#define NTIME           "equatorCrossingDateTime"
#define NTIME_OLD       "Node Crossing Time"
#define SNODE           "startDirection"
#define SNODE_OLD       "Start Node"
#define ENODE           "endDirection"
#define ENODE_OLD       "End Node"
#define MISSION         "platform"
#define MISSION_OLD     "Mission"
#define MSNCHAR         "Mission Characteristics"
#define SENSOR          "Sensor"
#define SNSCHAR         "Sensor Characteristics"
#define ORBNUM          "orbit_number"
#define ORBNUM_OLD      "Orbit Number"
#define NLAT            "northernmost_latitude"
#define NLAT_OLD        "Northernmost Latitude"
#define SLAT            "southernmost_latitude"
#define SLAT_OLD        "Southernmost Latitude"
#define WLON            "westernmost_longitude"
#define WLON_OLD        "Westernmost Longitude"
#define ELON            "easternmost_longitude"
#define ELON_OLD        "Easternmost Longitude"
#define STCLAT          "start_center_latitude"
#define STCLAT_OLD      "Start Center Latitude"
#define STCLON          "start_center_longitude"
#define STCLON_OLD      "Start Center Longitude"
#define ENDCLAT         "end_center_latitude"
#define ENDCLAT_OLD     "End Center Latitude"
#define ENDCLON         "end_center_longitude"
#define ENDCLON_OLD     "End Center Longitude"
#define NODEL           "equatorCrossingLongitude"
#define NODEL_OLD       "Orbit Node Longitude"
#define LAC_PX_ST       "LAC Pixel Start Number"
#define LAC_PX_SUBSAMP  "LAC Pixel Subsampling"

typedef struct cache_struct {
    int32 bscan;
    int32 escan;
    int32 dataSize;
    VOIDP data;
} cache_str;

// #define NON_CACHED 1


static int32 n_files_open = 0;

typedef idDS ds_id_prod_t[1000];
static ds_id_prod_t *ds_id_prod;

typedef int32 prodtype_t[1000];
static prodtype_t *prodtype;

typedef float32 slope_t[1000];
static slope_t *slope;
static slope_t *intercept;

typedef idDS ds_id_ll_t[3];
static ds_id_ll_t *ds_id_ll;
static ds_id_ll_t *ds_id_date;

typedef idDS ds_id_geonav_t[6];
static ds_id_geonav_t *ds_id_geonav;

typedef int32 grp_id_t[6];
static grp_id_t *grp_id;

static idDS *ds_id_l2_flags;
static idDS *ds_id_eng_qual;
static idDS *ds_id_s_flags;
static idDS *ds_id_nflag;
static idDS *ds_id_pixnum;
static idDS *ds_id_file;

static int32 *nsta;
static int32 *ninc;

static unsigned char **databuf;
static char **prodlist;

static int32 n_cntl_pnts;
static int32 prev_n_cntl_pnts = -1;

static float32 geonav[6][9];

static int32 one = 1;
static int32 zero32 = 0;
static int32 l2_flags_type;

static cache_str cache_l2_flags;
static cache_str cache_eng_qual;
static cache_str cache_s_flags;
static cache_str cache_nflag;
static cache_str cache_pixnum;
static cache_str cache_longitude;
static cache_str cache_latitude;
static int32 cache_nprod;
static cache_str **cache_l2_data;

/**
 * get_dtype get proper data type for reading
 * This converts to appropriate NCDF type given an HDF type
 * May want a better mouse trap, but for now solves the problem
 */
int32 get_dtype(int32 dtype, ds_format_t fileformat) {

    if (fileformat == DS_NCDF) {
        if (dtype == DFNT_INT8)
            dtype = NC_BYTE;
        else if (dtype == DFNT_UINT8)
            dtype = NC_UBYTE;
        else if (dtype == DFNT_INT16)
            dtype = NC_SHORT;
        else if (dtype == DFNT_UINT16)
            dtype = NC_USHORT;
        else if (dtype == DFNT_INT32)
            dtype = NC_INT;
        else if (dtype == DFNT_UINT32)
            dtype = NC_UINT;
        else if (dtype == DFNT_FLOAT32)
            dtype = NC_FLOAT;
        else if (dtype == DFNT_FLOAT64)
            dtype = NC_DOUBLE;
    }
    return dtype;
}
//----------------------------------------------------------------
// free cache

void free_rowgroup_cache() {
    int i;

    cache_l2_flags.bscan = -1;
    cache_eng_qual.bscan = -1;
    cache_s_flags.bscan = -1;
    cache_nflag.bscan = -1;
    cache_pixnum.bscan = -1;
    cache_longitude.bscan = -1;
    cache_latitude.bscan = -1;

    cache_l2_flags.escan = -1;
    cache_eng_qual.escan = -1;
    cache_s_flags.escan = -1;
    cache_nflag.escan = -1;
    cache_pixnum.escan = -1;
    cache_longitude.escan = -1;
    cache_latitude.escan = -1;

    for (i = 0; i < cache_nprod; i++) {
        cache_l2_data[i]->bscan = -1;
        cache_l2_data[i]->escan = -1;
    }
}


/* init the cache structures  */

/* -------------------------- */
void init_rowgroup_cache() {
    cache_str tmp_cache;

    tmp_cache.bscan = -1;
    tmp_cache.escan = -1;
    tmp_cache.dataSize = 0;
    tmp_cache.data = NULL;

    cache_l2_flags = tmp_cache;
    cache_eng_qual = tmp_cache;
    cache_s_flags = tmp_cache;
    cache_nflag = tmp_cache;
    cache_pixnum = tmp_cache;
    cache_longitude = tmp_cache;
    cache_latitude = tmp_cache;
}

//----------------------------------------------------------------

int32 openL2(char *fname, char *plist, l2_prod *l2_str) {
    int32 i;
    int32 sds_id;
    int32 len;
    int32 dims[8];
    int32 n_l2flags;
    int32 tilt_start[2] = {0, 0};
    int32 tilt_edges[2] = {20, 2};
    int16 t_ranges[2 * 20];
    int status;
    int32 listlen = 0;
    int32 fileindex;
    idDS ds_id;
    int dim_id;
    size_t tmpSizet;

    char buffer[2048 * 8];
    char *tmpStr;
    char *numstr[] = {"01", "02", "03", "04", "05", "06", "07", "08", "09", "10",
        "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
        "21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
        "31", "32", "33"};

    static int32 first = 1;

    ds_format_t fileformat;

    if (Hishdf(fname) == 1) {
        fileformat = DS_HDF;
    } else {
        fileformat = DS_NCDF;
    }

    /* Copy filename and product list into L2 structure */
    /* ------------------------------------------------ */
    if (l2_str->nrec == 0) {
        strcpy(l2_str->filename, fname);
        n_files_open++;
        fileindex = n_files_open - 1;
        l2_str->fileindex = fileindex;
    } else {
        fileindex = l2_str->fileindex;
    }

    if (first) {
        ds_id_prod = (ds_id_prod_t*) allocateMemory(MAXNFILES * sizeof (ds_id_prod_t), "ds_id_prod");
        prodtype = (prodtype_t*) allocateMemory(MAXNFILES * sizeof (prodtype_t), "prodtype");
        slope = (slope_t*) allocateMemory(MAXNFILES * sizeof (slope_t), "slope");
        intercept = (slope_t*) allocateMemory(MAXNFILES * sizeof (slope_t), "intercept");
        ds_id_ll = (ds_id_ll_t*) allocateMemory(MAXNFILES * sizeof (ds_id_ll_t), "ds_id_ll");
        ds_id_date = (ds_id_ll_t*) allocateMemory(MAXNFILES * sizeof (ds_id_ll_t), "ds_id_date");
        ds_id_geonav = (ds_id_geonav_t*) allocateMemory(MAXNFILES * sizeof (ds_id_geonav_t), "ds_id_geonav");
        grp_id = (grp_id_t*) allocateMemory(MAXNFILES * sizeof (grp_id_t), "grp_id");

        ds_id_l2_flags = (idDS*) allocateMemory(MAXNFILES * sizeof (idDS), "ds_id_l2_flags");
        ds_id_eng_qual = (idDS*) allocateMemory(MAXNFILES * sizeof (idDS), "ds_id_eng_qual");
        ds_id_s_flags = (idDS*) allocateMemory(MAXNFILES * sizeof (idDS), "ds_id_s_flags");
        ds_id_nflag = (idDS*) allocateMemory(MAXNFILES * sizeof (idDS), "ds_id_nflag");
        ds_id_pixnum = (idDS*) allocateMemory(MAXNFILES * sizeof (idDS), "ds_id_pixnum");
        ds_id_file = (idDS*) allocateMemory(MAXNFILES * sizeof (idDS), "ds_id_file");

        nsta = (int32*) allocateMemory(MAXNFILES * sizeof (int32), "nsta");
        ninc = (int32*) allocateMemory(MAXNFILES * sizeof (int32), "ninc");

        databuf = (unsigned char **) allocateMemory(MAXNFILES * sizeof (unsigned char *), "databuf");
        prodlist = (char **) allocateMemory(MAXNFILES * sizeof (char *), "prodlist");

        for (i = 0; i < MAXNFILES; i++) prodlist[i] = NULL;
        for (i = 0; i < MAXNFILES; i++) databuf[i] = NULL;
    }

    /* Generate prodlist */
    /* ----------------- */
    if (plist != 0x0) {
        prodlist[fileindex] = (char *) calloc(strlen(plist) + 1, sizeof (char));
        strcpy(prodlist[fileindex], plist);
    } else {
        getProdlist(fname, &prodlist[fileindex], &l2_flags_type);
    }


    /* Parse Product list */
    /* ------------------ */
    l2_str->nprod = 1;
    l2_str->prodname[0] = &prodlist[fileindex][0];
    len = strlen(prodlist[fileindex]);
    for (i = 0; i < len; i++) {
        if (prodlist[fileindex][i] == ':') {
            l2_str->prodname[l2_str->nprod] = prodlist[fileindex] + i + 1;
            l2_str->nprod++;
            prodlist[fileindex][i] = 0;
        }
    }
    /* No L2 products in L2 file (flags only) */
    if (strlen(prodlist[fileindex]) == 0) l2_str->nprod = 0;
    /* Start DS interface */
    /* ------------------ */
    ds_id = startDS(l2_str->filename, fileformat, DS_READ, 0);

    ds_id_file[fileindex] = ds_id;

    if (fileformat == DS_NCDF) {
        nc_inq_ncid(ds_id.fid, "sensor_band_parameters", &grp_id[fileindex][0]);
        nc_inq_ncid(ds_id.fid, "sensor_tilt", &grp_id[fileindex][1]);
        nc_inq_ncid(ds_id.fid, "scan_line_attributes", &grp_id[fileindex][2]);
        nc_inq_ncid(ds_id.fid, "geophysical_data", &grp_id[fileindex][3]);
        nc_inq_ncid(ds_id.fid, "navigation_data", &grp_id[fileindex][4]);
        nc_inq_ncid(ds_id.fid, "processing_control", &grp_id[fileindex][5]);
    }

    /* Get # of scans and # of pixels */
    /* ------------------------------ */
    if (fileformat == DS_NCDF) {
        status = nc_inq_dimid(ds_id.fid, "number_of_lines", &dim_id);
        if (status) {
            status = nc_inq_dimid(ds_id.fid, "Number_of_Scan_Lines", &dim_id);
            if(status) {
                printf("-E- %s:%d - openL2 - could not find number_of_lines.\n",
                        __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        nc_inq_dimlen(ds_id.fid, dim_id, &tmpSizet);
        l2_str->nrec = tmpSizet;
        status = nc_inq_dimid(ds_id.fid, "pixels_per_line", &dim_id);
        if (status) {
            status = nc_inq_dimid(ds_id.fid, "Pixels_per_Scan_Line", &dim_id);
            if(status) {
                printf("-E- %s:%d - openL2 - could not find pixels_per_line.\n",
                        __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        nc_inq_dimlen(ds_id.fid, dim_id, &tmpSizet);
        l2_str->nsamp = tmpSizet;
    } else {
        readAttr(ds_id, "Number of Scan Lines", (VOIDP) & l2_str->nrec);
        readAttr(ds_id, "Pixels per Scan Line", (VOIDP) & l2_str->nsamp);
    }

    /* Get start & end times, orbit number and data type */
    /* ------------------------------------------------- */
    if (fileformat == DS_NCDF) {
        int32_t yr, dy, sc;
        status = readAttr(ds_id, "orbit_number", (VOIDP) & l2_str->orbit);
        // not checking the return status here as there are real live cases of
        // orbit number NOT existing in the L2 metadata (e.g. GOCI)
        
        tmpStr = readAttrStr(ds_id, "time_coverage_start");
        if(!tmpStr) {
            printf("-E- %s:%d - openL2 - could not find time_coverage_start.\n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        isodate2ydmsec(tmpStr, &yr, &dy, &sc);
        free(tmpStr);
        
        l2_str->syear = (int16_t) yr;
        l2_str->sday = (int16_t) dy;
        l2_str->smsec = sc;
        //    l2_str->smsec = 1000 * ((int32) scs);

        tmpStr = readAttrStr(ds_id, "time_coverage_end");
        if(!tmpStr) {
            printf("-E- %s:%d - openL2 - could not find time_coverage_end.\n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        isodate2ydmsec(tmpStr, &yr, &dy, &sc);
        free(tmpStr);
        
        l2_str->eyear = (int16_t) yr;
        l2_str->eday = (int16_t) dy;
        //    l2_str->emsec = 1000 * ((int32) scs);
        l2_str->emsec = sc;
    } else {
        readAttr(ds_id, "Start Year", (VOIDP) & l2_str->syear);
        readAttr(ds_id, "Start Day", (VOIDP) & l2_str->sday);
        readAttr(ds_id, "Start Millisec", (VOIDP) & l2_str->smsec);
        readAttr(ds_id, "End Year", (VOIDP) & l2_str->eyear);
        readAttr(ds_id, "End Day", (VOIDP) & l2_str->eday);
        readAttr(ds_id, "End Millisec", (VOIDP) & l2_str->emsec);
        readAttr(ds_id, "Orbit Number", (VOIDP) & l2_str->orbit);
        readAttr(ds_id, "Data Type", (VOIDP) & l2_str->dtype);
    }

    /* Allocate geoloc (lon/lat) arrays */
    /* -------------------------------- */
    l2_str->geoloc = (float32 *) calloc(2 * l2_str->nsamp, sizeof (float32));
    l2_str->latitude = l2_str->geoloc;
    l2_str->longitude = l2_str->geoloc + l2_str->nsamp;


    /* Get longitude, latitude, & date sds ids */
    /* --------------------------------------- */
    if (fileformat == DS_NCDF) ds_id.fid = grp_id[fileindex][4];
    ds_id_ll[fileindex][0] = (idDS){ds_id.fid,
        selectDS(ds_id, "longitude"),
        ds_id.fftype};
    ds_id_ll[fileindex][1] = (idDS){ds_id.fid,
        selectDS(ds_id, "latitude"),
        ds_id.fftype};
    ds_id_ll[fileindex][2] = (idDS){ds_id.fid, -1, ds_id.fftype};

    getDimsDS(ds_id, "longitude", dims);

    if (fileformat == DS_NCDF) ds_id.fid = grp_id[fileindex][2];
    ds_id_date[fileindex][0] = (idDS){ds_id.fid,
        selectDS(ds_id, "year"),
        ds_id.fftype};
    ds_id_date[fileindex][1] = (idDS){ds_id.fid,
        selectDS(ds_id, "day"),
        ds_id.fftype};
    ds_id_date[fileindex][2] = (idDS){ds_id.fid,
        selectDS(ds_id, "msec"),
        ds_id.fftype};

    l2_str->geointerp = 0;

    /* Test if full-size lon/lat SDS */
    /* ----------------------------- */
    if (dims[0] != l2_str->nrec || dims[1] != l2_str->nsamp ||
            ds_id_ll[fileindex][0].sid == -1) {

        l2_str->geointerp = 1;

        /* Test for geonav arrays (SeaWIFS) */
        /* -------------------------------- */
        if (fileformat == DS_NCDF) ds_id.fid = grp_id[fileindex][4];
        ds_id_geonav[fileindex][0] = (idDS){ds_id.fid,
            selectDS(ds_id, "orb_vec"),
            ds_id.fftype};
        ds_id_geonav[fileindex][1] = (idDS){ds_id.fid,
            selectDS(ds_id, "sen_mat"),
            ds_id.fftype};
        ds_id_geonav[fileindex][2] = (idDS){ds_id.fid,
            selectDS(ds_id, "scan_ell"),
            ds_id.fftype};
        ds_id_geonav[fileindex][3] = (idDS){ds_id.fid,
            selectDS(ds_id, "sun_ref"),
            ds_id.fftype};
        //    ds_sid_geonav[fileindex][4] = selectDS(ds_id, "l_vert");
        ds_id_geonav[fileindex][4].sid = -1;
        ds_id_geonav[fileindex][5] = (idDS){ds_id.fid,
            selectDS(ds_id, "att_ang"),
            ds_id.fftype};

        ds_id = ds_id_file[fileindex];
        nsta[fileindex] = -1;
        ninc[fileindex] = -1;
        readAttr(ds_id_file[fileindex], "LAC Pixel Start Number",
                (VOIDP) & nsta[fileindex]);
        readAttr(ds_id_file[fileindex], "LAC Pixel Subsampling",
                (VOIDP) & ninc[fileindex]);

        if (ds_id_geonav[fileindex][0].sid != -1 &&
                ds_id_geonav[fileindex][1].sid != -1 &&
                ds_id_geonav[fileindex][2].sid != -1 &&
                ds_id_geonav[fileindex][3].sid != -1 &&
                nsta[fileindex] != -1 && ninc[fileindex] != -1) {

            l2_str->geointerp = 2;

            for (i = 0; i < 6; i++) l2_str->geonav[i] = geonav[i];
        } else {

            /* Get # of control points */
            /* ----------------------- */
            if (fileformat == DS_NCDF) {
                status = nc_inq_dimid(ds_id_file[fileindex].fid, "pixel_control_points", &dim_id);
                if (status) {
                    printf("-E- %s:%d - openL2 - Could not find the dimension pixel_control_points.\n",
                            __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                status = nc_inq_dimlen(ds_id_file[fileindex].fid, dim_id, &tmpSizet);
                if (status) {
                    printf("-E- %s:%d - openL2 - Could not read the dimension pixel_control_points\n",
                            __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                n_cntl_pnts = tmpSizet;
            } else
                readAttr(ds_id_file[fileindex], "Number of Pixel Control Points",
                    (VOIDP) & n_cntl_pnts);

            /* Check that all L2 files have same number of control points */
            /* ---------------------------------------------------------- */
            if (prev_n_cntl_pnts != -1 && prev_n_cntl_pnts != n_cntl_pnts) {
                printf("L2 file #:%4d has %d control points.\n", fileindex,
                        prev_n_cntl_pnts);
                printf("L2 file #:%4d has %d control points.\n", fileindex + 1,
                        n_cntl_pnts);
                printf("These must be identical.\n");
                exit(-1);
            }
            prev_n_cntl_pnts = n_cntl_pnts;


            /* Allocate arrays needed for lon/lat interpolation */
            /* ------------------------------------------------ */
            l2_str->lon_cntl = (float32 *) calloc(n_cntl_pnts, sizeof (float32));
            l2_str->lat_cntl = (float32 *) calloc(n_cntl_pnts, sizeof (float32));
            l2_str->cntl_pnts = (float32 *) calloc(n_cntl_pnts, sizeof (float32));
            l2_str->cntl_pnts_cache = (float32 *) calloc(n_cntl_pnts, sizeof (float32));
            l2_str->spline_arr = (float32 *) calloc(n_cntl_pnts, sizeof (float32));

            /* Get control point sds id */
            /* ------------------------ */
            ds_id_ll[fileindex][2].sid = selectDS(ds_id, "cntl_pt_cols");

            /* Read control points array if first time through */
            /* ----------------------------------------------- */
            int32 *tmpPtr = (int32*) l2_str->cntl_pnts;
            readDS(ds_id_ll[fileindex][2], "cntl_pt_cols",
                    &zero32, NULL, &n_cntl_pnts, tmpPtr);
            endaccessDS(ds_id);
            ds_id_ll[fileindex][2].sid = -1;

            /* Convert cntl pnts from I32 to F32 */
            /* --------------------------------- */
            for (i = 0; i < n_cntl_pnts; i++) {
                l2_str->cntl_pnts_cache[i] = tmpPtr[i];
            }

        }
    }


    /* Store data products info in L2 structure */
    /* ---------------------------------------- */
    if (fileformat == DS_NCDF)
        ds_id.fid = grp_id[fileindex][3];
    for (i = 0; i < l2_str->nprod; i++) {
        sds_id = selectDS(ds_id, l2_str->prodname[i]);

        if (sds_id != -1) {
            getTypeDS(ds_id, l2_str->prodname[i], &(prodtype[fileindex][i]));

            /* Read scaling slope and intercept */
            /* -------------------------------- */
            idDS ds_id0 = {-ds_id.fid, sds_id, ds_id.fftype};

            if (fileformat == DS_NCDF) {
                if (prodtype[fileindex][i] != NC_FLOAT && prodtype[fileindex][i] != NC_DOUBLE) {
                    status = readAttr(ds_id0, "scale_factor", (VOIDP) &(slope[fileindex][i]));
                    if (status) {
                        slope[fileindex][i] = 1.0;
                    }
                    readAttr(ds_id0, "add_offset", (VOIDP) &(intercept[fileindex][i]));
                    if (status) {
                        intercept[fileindex][i] = 0.0;
                    }
                } else {
                    slope[fileindex][i] = 1.0;
                    intercept[fileindex][i] = 0.0;
                }
                readAttr(ds_id0, "_FillValue", (VOIDP) (&l2_str->bv_scaled[i]));
            } else {
                readAttr(ds_id0, "slope", (VOIDP) &(slope[fileindex][i]));
                readAttr(ds_id0, "intercept", (VOIDP) &(intercept[fileindex][i]));
                if (slope[fileindex][i] == 0.0)
                    slope[fileindex][i] = 1.0;

                if (findAttr(ds_id0, "bad_value_unscaled") != -1)
                    readAttr(ds_id0, "bad_value_unscaled", (VOIDP) (&l2_str->bv_unscaled[i]));
                else
                    l2_str->bv_unscaled[i] = -1e30;

                if (findAttr(ds_id0, "bad_value_scaled") != -1)
                    readAttr(ds_id0, "bad_value_scaled", (VOIDP) (&l2_str->bv_scaled[i]));
                else
                    l2_str->bv_scaled[i] = -32768;
            }

            ds_id_prod[fileindex][i] = (idDS){ds_id.fid, sds_id, ds_id.fftype};
        } else {
            printf("Data Product: \"%s\" not found.\n", l2_str->prodname[i]);
            exit(1);
        }
    }

    /* Allocate cache for products */
    /* --------------------------- */
    // Allocate cache for data products if NULL
    if (cache_l2_data == NULL) {
        cache_nprod = l2_str->nprod;
        cache_l2_data = (cache_str**) malloc(sizeof (cache_str*) * cache_nprod);
        for (i = 0; i < cache_nprod; i++) {
            cache_l2_data[i] = (cache_str*) malloc(sizeof (cache_str));
            cache_l2_data[i]->bscan = -1;
            cache_l2_data[i]->escan = -1;
            cache_l2_data[i]->dataSize = 0;
            cache_l2_data[i]->data = NULL;
        }
    } else if (cache_nprod < l2_str->nprod) {
        // Allocate cache for data products if number of products increases

        for (i = 0; i < cache_nprod; i++) free(cache_l2_data[i]);
        free(cache_l2_data);

        cache_nprod = l2_str->nprod;
        cache_l2_data = (cache_str**) malloc(sizeof (cache_str*) * cache_nprod);
        for (i = 0; i < cache_nprod; i++) {
            cache_l2_data[i] = (cache_str*) malloc(sizeof (cache_str));
            cache_l2_data[i]->bscan = -1;
            cache_l2_data[i]->escan = -1;
            cache_l2_data[i]->dataSize = 0;
            cache_l2_data[i]->data = NULL;
        }
    }

    /* Allocate databuf data array */
    /* --------------------------- */
    databuf[fileindex] = (unsigned char *) calloc(l2_str->nsamp, 8);


    /* Allocate L2 data array */
    /* ---------------------- */
    l2_str->l2_data = (float32 *) calloc(l2_str->nprod * l2_str->nsamp,
            sizeof (float32));

    /* Allocate pixnum (if applicable) */
    /* ------------------------------- */
    if ((ds_id_pixnum[fileindex].sid = selectDS(ds_id, "pixnum")) != -1) {
        l2_str->pixnum = (int32 *) calloc(l2_str->nsamp, sizeof (int32));
    } else {
        l2_str->pixnum = NULL;
    }
    ds_id_pixnum[fileindex].fid = ds_id.fid;
    ds_id_pixnum[fileindex].fftype = ds_id.fftype;

    /* Read mside, detnum (if applicable) */
    /* ---------------------------------- */
    if (fileformat == DS_NCDF) ds_id.fid = grp_id[fileindex][2];
    if ((ds_id.sid = selectDS(ds_id, "mside")) != -1) {
        l2_str->mside = (byte *) calloc(l2_str->nrec, sizeof (byte));
        if (ds_id.fftype == DS_NCDF) {
            readDS(ds_id, "mside", &zero32, NULL, &l2_str->nrec,
                    (VOIDP) l2_str->mside);
        } else {
            int32 rank;
            int32 dimsizes[3];
            int32 ntype;
            int32 num_attrs;

            SDgetinfo(ds_id.sid, NULL, &rank, dimsizes, &ntype, &num_attrs);
            if (ntype == DFNT_INT8) {
                readDS(ds_id, "mside", &zero32, NULL, &l2_str->nrec,
                        (VOIDP) l2_str->mside);
            } else {
                int i;
                int32 junk[l2_str->nrec];

                readDS(ds_id, "mside", &zero32, NULL, &l2_str->nrec, (VOIDP) junk);
                for (i = 0; i < l2_str->nrec; i++)
                    l2_str->mside[i] = (byte) junk[i];
            }
        }
        endaccessDS(ds_id);
    } else {
        l2_str->mside = NULL;
    }

    if ((ds_id.sid = selectDS(ds_id, "detnum")) != -1) {
        l2_str->detnum = (byte *) calloc(l2_str->nrec, sizeof (byte));
        if (ds_id.fftype == DS_NCDF) {
            readDS(ds_id, "detnum", &zero32, NULL, &l2_str->nrec,
                    (VOIDP) l2_str->detnum);
        } else {
            int32 rank;
            int32 dimsizes[3];
            int32 ntype;
            int32 num_attrs;

            SDgetinfo(ds_id.sid, NULL, &rank, dimsizes, &ntype, &num_attrs);
            if (ntype == DFNT_INT8) {
                readDS(ds_id, "detnum", &zero32, NULL, &l2_str->nrec,
                        (VOIDP) l2_str->detnum);
            } else {
                int i;
                int32 junk[l2_str->nrec];

                readDS(ds_id, "detnum", &zero32, NULL, &l2_str->nrec, (VOIDP) junk);
                for (i = 0; i < l2_str->nrec; i++)
                    l2_str->detnum[i] = (byte) junk[i];
            }
        }
        endaccessDS(ds_id);
    } else {
        l2_str->detnum = NULL;
    }


    /* Read tilt data (if applicable) */
    /* ------------------------------ */
    if (fileformat == DS_NCDF) ds_id.fid = grp_id[fileindex][1];
    if (selectDS(ds_id, "ntilts") != -1) {
        ds_id.sid = selectDS(ds_id, "ntilts");
        readDS(ds_id, "ntilts", tilt_start, NULL, &one,
                (VOIDP) & l2_str->ntilts);
        ds_id.sid = selectDS(ds_id, "tilt_flags");
        readDS(ds_id, "tilt_flags", tilt_start, NULL, tilt_edges,
                (VOIDP) l2_str->tilt_flags);
        ds_id.sid = selectDS(ds_id, "tilt_ranges");
        readDS(ds_id, "tilt_ranges", tilt_start, NULL, tilt_edges,
                (VOIDP) t_ranges);

        for (i = 0; i < l2_str->ntilts; i++) {
            l2_str->tilt_ranges[0][i] = t_ranges[i * 2];
            l2_str->tilt_ranges[1][i] = t_ranges[i * 2 + 1];
        }
    }



    /* Open L2 flags SDS */
    /* ----------------- */
    if (fileformat == DS_NCDF) {
        ds_id.fid = grp_id[fileindex][3];
        ds_id_l2_flags[fileindex] = (idDS){
            ds_id.fid,
            selectDS(ds_id, "l2_flags"),
            ds_id.fftype
        };
        idDS temp_ds_id = ds_id_l2_flags[fileindex];
        temp_ds_id.fid *= -1;

        if (ds_id_l2_flags[fileindex].sid != -1) {
            l2_str->l2_flags = (int32 *) calloc(l2_str->nsamp, sizeof (int32));
            l2_str->flagnames = readAttrStr(temp_ds_id, "flag_meanings");
            if(!l2_str->flagnames) {
                printf("-E- %s:%d - openL2 - could not find flag_meanings.\n",
                        __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (i = 0; i < strlen(l2_str->flagnames); i++) {
                if (l2_str->flagnames[i] == ' ')
                    l2_str->flagnames[i] = ',';
            }
        } else {
            l2_str->l2_flags = 0x0;
        }
    } else {

        // old HDF4 flag names
        ds_id_l2_flags[fileindex] = (idDS){
            ds_id.fid,
            selectDS(ds_id, "l2_flags"),
            ds_id.fftype
        };

        if (ds_id_l2_flags[fileindex].sid != -1) {
            l2_str->l2_flags = (int32 *) calloc(l2_str->nsamp, sizeof (int32));

            /* Read L2 flagnames */
            /* ----------------- */
            n_l2flags = 0;
            listlen = 0;
            idDS temp_ds_id = ds_id_l2_flags[fileindex];
            temp_ds_id.fid *= -1;
            while (1) {
                sprintf(buffer, "f%s_name", numstr[n_l2flags]);
                if (findAttr(temp_ds_id, buffer) != -1) {
                    tmpStr = readAttrStr(temp_ds_id, buffer);
                    listlen += strlen(tmpStr) + 1;
                    free(tmpStr);
                    n_l2flags++;
                } else
                    break;
            }
            l2_str->flagnames = (char *) calloc(listlen, sizeof (char));
            for (i = 0; i < n_l2flags; i++) {
                sprintf(buffer, "f%s_name", numstr[i]);
                tmpStr = readAttrStr(temp_ds_id, buffer);
                strcat(l2_str->flagnames, tmpStr);
                free(tmpStr);
                if (i < n_l2flags - 1)
                    strcat(l2_str->flagnames, ",");
            }
        } else {
            l2_str->l2_flags = 0x0;
        }
    }

    /* Open eng_qual SDS */
    /* ----------------- */
    if (checkDS(ds_id_file[fileindex], "eng_qual") != -1) {
        ds_id_eng_qual[fileindex].sid = selectDS(ds_id, "eng_qual");
        ds_id_eng_qual[fileindex].fid = ds_id.fid;
        ds_id_eng_qual[fileindex].fftype = ds_id.fftype;
    } else {
        ds_id_eng_qual[fileindex].sid = -1;
    }

    /* Open s_flags SDS */
    /* ---------------- */
    if (checkDS(ds_id_file[fileindex], "s_flags") != -1) {
        ds_id_s_flags[fileindex].sid = selectDS(ds_id, "s_flags");
        ds_id_s_flags[fileindex].fid = ds_id.fid;
        ds_id_s_flags[fileindex].fftype = ds_id.fftype;
    } else {
        ds_id_s_flags[fileindex].sid = -1;
    }

    /* Open nflag SDS */
    /* -------------- */
    if (checkDS(ds_id_file[fileindex], "nflag") != -1) {
        ds_id_nflag[fileindex].sid = selectDS(ds_id, "nflag");
        ds_id_nflag[fileindex].fid = ds_id.fid;
        ds_id_nflag[fileindex].fftype = ds_id.fftype;
    } else {
        ds_id_nflag[fileindex].sid = -1;
    }

    /* Read cached date fields */
    /* ----------------------- */
    if (fileformat == DS_NCDF) ds_id.fid = grp_id[fileindex][2];
    if (ds_id_date[fileindex][0].sid != -1) {
        l2_str->year_cache = (int32*) malloc(sizeof (int32) * l2_str->nrec);
        readDS(ds_id_date[fileindex][0], "year", &zero32, NULL,
                &(l2_str->nrec), l2_str->year_cache);
        endaccessDS(ds_id_date[fileindex][0]);
    }

    if (ds_id_date[fileindex][1].sid != -1) {
        l2_str->day_cache = (int32*) malloc(sizeof (int32) * l2_str->nrec);
        readDS(ds_id_date[fileindex][1], "day", &zero32, NULL,
                &(l2_str->nrec), l2_str->day_cache);
        endaccessDS(ds_id_date[fileindex][1]);
    }

    if (ds_id_date[fileindex][2].sid != -1) {
        l2_str->msec_cache = (int32*) malloc(sizeof (int32) * l2_str->nrec);
        readDS(ds_id_date[fileindex][2], "msec", &zero32, NULL,
                &(l2_str->nrec), l2_str->msec_cache);
        endaccessDS(ds_id_date[fileindex][2]);
    }


    first = 0;

    return 0;
}

int32 reopenL2(int32 fileindex, l2_prod *l2_str) {
    int32 i;
    //int32 sd_id;
    idDS ds_id;

    ds_format_t fileformat;
    if (Hishdf(l2_str->filename) == 1) {
        fileformat = DS_HDF;
    } else {
        fileformat = DS_NCDF;
    }

    ds_id = startDS(l2_str->filename, fileformat, DS_READ, 0);
    ds_id_file[fileindex] = ds_id;

    if (fileformat == DS_NCDF) {
        nc_inq_ncid(ds_id.fid, "sensor_band_parameters", &grp_id[fileindex][0]);
        nc_inq_ncid(ds_id.fid, "sensor_tilt", &grp_id[fileindex][1]);
        nc_inq_ncid(ds_id.fid, "scan_line_attributes", &grp_id[fileindex][2]);
        nc_inq_ncid(ds_id.fid, "geophysical_data", &grp_id[fileindex][3]);
        nc_inq_ncid(ds_id.fid, "navigation_data", &grp_id[fileindex][4]);
        nc_inq_ncid(ds_id.fid, "processing_control", &grp_id[fileindex][5]);
    }

    if (fileformat == DS_NCDF) ds_id.fid = grp_id[fileindex][4];
    ds_id_ll[fileindex][0] = (idDS){ds_id.fid,
        selectDS(ds_id, "longitude"),
        ds_id.fftype};
    ds_id_ll[fileindex][1] = (idDS){ds_id.fid,
        selectDS(ds_id, "latitude"),
        ds_id.fftype};

    if (l2_str->geointerp == 2) {
        ds_id_geonav[fileindex][0] = (idDS){ds_id.fid,
            selectDS(ds_id, "orb_vec"),
            ds_id.fftype};
        ds_id_geonav[fileindex][1] = (idDS){ds_id.fid,
            selectDS(ds_id, "sen_mat"),
            ds_id.fftype};
        ds_id_geonav[fileindex][2] = (idDS){ds_id.fid,
            selectDS(ds_id, "scan_ell"),
            ds_id.fftype};
        ds_id_geonav[fileindex][3] = (idDS){ds_id.fid,
            selectDS(ds_id, "sun_ref"),
            ds_id.fftype};
        ds_id_geonav[fileindex][5] = (idDS){ds_id.fid,
            selectDS(ds_id, "att_ang"),
            ds_id.fftype};
    }

    if (fileformat == DS_NCDF) ds_id.fid = grp_id[fileindex][3];
    for (i = 0; i < l2_str->nprod; i++) {
        ds_id_prod[fileindex][i] = (idDS){ds_id.fid,
            selectDS(ds_id, l2_str->prodname[i]),
            ds_id.fftype};
    }

    ds_id_l2_flags[fileindex] = (idDS){ds_id.fid,
        selectDS(ds_id, "l2_flags"),
        ds_id.fftype};

    ds_id_eng_qual[fileindex] = (idDS){ds_id.fid,
        selectDS(ds_id, "eng_qual"),
        ds_id.fftype};

    ds_id_s_flags[fileindex] = (idDS){ds_id.fid,
        selectDS(ds_id, "s_flags"),
        ds_id.fftype};

    ds_id_nflag[fileindex] = (idDS){ds_id.fid,
        selectDS(ds_id, "nflag"),
        ds_id.fftype};

    ds_id_pixnum[fileindex] = (idDS){ds_id.fid,
        selectDS(ds_id, "pixnum"),
        ds_id.fftype};

    return 0;
}

//----------------------------------------------------------------
// function to cache the read data
// Note:  make sure there is a terminating 0 at the end of scan_in_rowgroup

int32 readdata_cached(idDS ds_id, char *name,
        int32 *start, int32 *stride, int32 *edges,
        VOIDP data, unsigned char *scan_in_rowgroup,
        cache_str *cache, int32 dtype) {
    int32 start2[3];
    int32 edges2[3];
    int32 bscan;
    int32 escan;
    int32 nscans;
    int32 size;
    size_t recSize = hdf_sizeof(dtype) * edges[1];
    char* ptr;
    int32 status;

#ifdef NON_CACHED
    status = readDS(ds_id, name, start, stride, edges, data);
#else

    if (ds_id.fftype == DS_NCDF) {
        status = nc_inq_type(ds_id.fid, dtype, NULL, &recSize);
        check_err(status, __LINE__, __FILE__);
        recSize *= edges[1];
    }

    status = 0;
    // see if scan line is in the cache
    if ((start[0] < cache->bscan) || (start[0] > cache->escan)) {

        // find out how many lines to cache.  Note: scan_in_rowgroup was
        // allocated with an extra 0 at the end to mark the end of lines
        bscan = escan = start[0];
        while (scan_in_rowgroup[escan] != 0)
            escan++;
        escan--;
        nscans = escan - bscan + 1;
        size = nscans * recSize;
        if (size > cache->dataSize) {
            cache->dataSize = size;
            if (cache->data)
                free(cache->data);
            cache->data = malloc(size);
        }
        cache->bscan = bscan;
        cache->escan = escan;
        start2[0] = bscan;
        start2[1] = start[1];
        edges2[0] = nscans;
        edges2[1] = edges[1];
        status = readDS(ds_id, name, start2, stride, edges2, cache->data);
    } // cache miss

    ptr = (char*) (cache->data) + (start[0] - cache->bscan) * recSize;
    memcpy(data, ptr, recSize);
#endif

    return status;
}

int32 readL2(l2_prod *l2_str, int32 ifile, int32 recnum, int32 iprod,
        unsigned char *scan_in_rowgroup) {
    int32 i;
    int32 start[3] = {0, 0, 0};
    int32 edges[3] = {1, 1, 1};
    int32 ipix;
    int32 ptype;
    int32 status;

    float32 slp;
    float32 itp;


    int32 flag_edges[2] = {1, 4};
    int32 nflag_edges[2] = {1, 8};

    float32 nan = __builtin_nanf("");

    start[0] = recnum;
    start[1] = 0;
    edges[0] = 1;
    edges[1] = l2_str->nsamp;


    /* Read L2 flags */
    /* ------------- */
    if (ds_id_l2_flags[ifile].sid != -1) {
        if (scan_in_rowgroup == NULL) {
            status = readDS(ds_id_l2_flags[ifile], "l2_flags", start, NULL, edges,
                    (VOIDP) l2_str->l2_flags);
        } else {
            status = readdata_cached(ds_id_l2_flags[ifile], "l2_flags",
                    start, NULL, edges,
                    (VOIDP) l2_str->l2_flags, scan_in_rowgroup,
                    &cache_l2_flags, l2_flags_type);
        }

        /* If INT16 array then convert to INT32 */
        /* ------------------------------------ */
        if (l2_flags_type == DFNT_INT16) {
            int16 *ptr16 = (int16*) l2_str->l2_flags;
            for (i = l2_str->nsamp - 1; i >= 0; i--)
                l2_str->l2_flags[i] = ptr16[i];
        }
    }


    /* Read eng_qual */
    /* ------------- */
    if (ds_id_eng_qual[ifile].sid != -1) {
        if (scan_in_rowgroup == NULL) {
            status = readDS(ds_id_eng_qual[ifile], "eng_qual",
                    start, NULL, flag_edges,
                    (VOIDP) l2_str->eng_qual);
        } else {
            status = readdata_cached(ds_id_eng_qual[ifile], "eng_qual",
                    start, NULL, flag_edges,
                    (VOIDP) l2_str->eng_qual, scan_in_rowgroup,
                    &cache_eng_qual, get_dtype(DFNT_INT8, ds_id_eng_qual[ifile].fftype));
        }
    }

    /* Read s_flags */
    /* ------------ */
    if (ds_id_s_flags[ifile].sid != -1) {
        if (scan_in_rowgroup == NULL) {
            status = readDS(ds_id_s_flags[ifile], "s_flags",
                    start, NULL, flag_edges,
                    (VOIDP) l2_str->s_flags);
        } else {
            status = readdata_cached(ds_id_s_flags[ifile], "s_flags",
                    start, NULL, flag_edges,
                    (VOIDP) l2_str->s_flags, scan_in_rowgroup,
                    &cache_s_flags, get_dtype(DFNT_INT8, ds_id_s_flags[ifile].fftype));
        }
    }

    /* Read nflag */
    /* ---------- */
    if (ds_id_nflag[ifile].sid != -1) {
        if (scan_in_rowgroup == NULL) {
            status = readDS(ds_id_nflag[ifile], "nflag",
                    start, NULL, nflag_edges,
                    (VOIDP) l2_str->nflag);
        } else {
            status = readdata_cached(ds_id_nflag[ifile], "nflag",
                    start, NULL, nflag_edges,
                    (VOIDP) l2_str->nflag, scan_in_rowgroup,
                    &cache_nflag, get_dtype(DFNT_INT32, ds_id_nflag[ifile].fftype));
        }
    }

    /* Read pixnum (if applicable) */
    /* --------------------------- */
    if (ds_id_pixnum[ifile].sid != -1) {
        if (scan_in_rowgroup == NULL) {
            status = readDS(ds_id_pixnum[ifile], "pixnum",
                    start, NULL, edges,
                    (VOIDP) l2_str->pixnum);
        } else {
            status = readdata_cached(ds_id_pixnum[ifile], "pixnum",
                    start, NULL, edges,
                    (VOIDP) l2_str->pixnum, scan_in_rowgroup,
                    &cache_pixnum, get_dtype(DFNT_INT32, ds_id_pixnum[ifile].fftype));
        }
    }

    /* Read date fields */
    /* ---------------- */
    l2_str->year = l2_str->year_cache[recnum];
    l2_str->day = l2_str->day_cache[recnum];
    l2_str->msec = l2_str->msec_cache[recnum];


    /* Main product loop */
    /* ----------------- */
    for (i = 0; i < l2_str->nprod; i++) {

        if ((iprod != -1) && (i != iprod)) continue;


        slp = slope[ifile][i];
        itp = intercept[ifile][i];

        /* Read into data buffer */
        /* --------------------- */
        if (scan_in_rowgroup == NULL) {
            status = readDS(ds_id_prod[ifile][i], l2_str->prodname[i],
                    start, NULL, edges,
                    (VOIDP) databuf[ifile]);
        } else {
            status = readdata_cached(ds_id_prod[ifile][i], l2_str->prodname[i],
                    start, NULL, edges,
                    (VOIDP) databuf[ifile], scan_in_rowgroup,
                    cache_l2_data[i], prodtype[ifile][i]);
        }

        if (status != 0) {
            printf("Read Error: %d (%s) %d\n",
                    ifile, l2_str->filename, i);
            exit(-1);
        }


        if (ds_id_file[ifile].fftype == DS_HDF) {
            ptype = prodtype[ifile][i];
        } else {
            switch (prodtype[ifile][i]) {
            case NC_UBYTE:
                ptype = DFNT_UINT8;
                break;
            case NC_BYTE:
            case NC_CHAR:
                ptype = DFNT_INT8;
                break;
            case NC_SHORT:
                ptype = DFNT_INT16;
                break;
            case NC_FLOAT:
                ptype = DFNT_FLOAT32;
                break;
            };
        }


        /* Convert to proper data type (unscale) */
        /* ------------------------------------- */
        switch (ptype) {

        case DFNT_UINT8: {
            uint8* tmp = (uint8*) databuf[ifile];
            float32* result = &l2_str->l2_data[i * l2_str->nsamp];
            for (ipix = 0; ipix < l2_str->nsamp; ipix++) {
                if (*tmp == l2_str->bv_scaled[i]) {
                    *result = nan;
                } else {
                    *result = *tmp * slp + itp;
                }
                tmp++;
                result++;
            }
        }
        break;

        case DFNT_INT8: {
            int8* tmp = (int8*) databuf[ifile];
            float32* result = &l2_str->l2_data[i * l2_str->nsamp];
            for (ipix = 0; ipix < l2_str->nsamp; ipix++) {
                if (*tmp == l2_str->bv_scaled[i]) {
                    *result = nan;
                } else {
                    *result = *tmp * slp + itp;
                }
                tmp++;
                result++;
            }
        }
        break;

        case DFNT_INT16: {
            int16* tmp = (int16*) databuf[ifile];
            float32* result = &l2_str->l2_data[i * l2_str->nsamp];
            for (ipix = 0; ipix < l2_str->nsamp; ipix++) {
                if (*tmp == l2_str->bv_scaled[i]) {
                    *result = nan;
                } else {
                    if (*tmp <= -32767)
                        *result = -32767;
                    else
                        *result = *tmp * slp + itp;
                }
                tmp++;
                result++;
            }
        }
        break;

        case DFNT_FLOAT32:
            memcpy(&l2_str->l2_data[i * l2_str->nsamp], databuf[ifile], l2_str->nsamp * sizeof(float32));
        
        }; /* end switch */

    } /* product loop */


    /* Read lon/lat fields (Note: start & edges are changed) */
    /* ----------------------------------------------------- */
    status = readlonlat(l2_str, ifile, start, edges, scan_in_rowgroup);
    if (status == 1) {
        return 5;
    }

    /* Check whether lon/lat values are within range */
    /* --------------------------------------------- */
    for (i = 0; i < l2_str->nsamp; i++) {
        if ((l2_str->longitude[i] > 180 || l2_str->longitude[i] < -180) &&
                ((l2_str->l2_flags[i] & 33554432) == 0)) {
            printf("Scheme: %d\n", l2_str->geointerp);
            printf("Pixel Longitude %d out of range (%f) for scan %d in %s.\n",
                    i, l2_str->longitude[i], recnum, l2_str->filename);
            exit(-1);
        }



        if ((l2_str->latitude[i] > 180 || l2_str->latitude[i] < -180) &&
                ((l2_str->l2_flags[i] & 33554432) == 0)) {
            printf("Scheme: %d\n", l2_str->geointerp);
            printf("Pixel Latitude %d out of range (%f) for scan %d in %s.\n",
                    i, l2_str->latitude[i], recnum, l2_str->filename);
            exit(-1);
        }
    }

    return 0;
}

int32 readlonlat(l2_prod *l2_str, int32 ifile, int32 *start, int32 *edges,
        unsigned char *scan_in_rowgroup) {
    int32 i;
    int32 geo_edge[6] = {3, 3, 6, 3, 3, 3};

    int32 n_cntl_pnts_removed;

    float32 delta;
    float32 *cntl_pnt_buf1;
    float32 *cntl_pnt_buf2;
    float32 lon_lat_lim = 0.1;

    char *geonav_name[6] = {"orb_vec", "sen_mat", "scan_ell",
        "sun_ref", "l_vert", "att_ang"};


    switch (l2_str->geointerp) {

    case 0:

        /* Read full-size lon/lat fields */
        /* ----------------------------- */
        if (scan_in_rowgroup == NULL) {
            readDS(ds_id_ll[ifile][0], "longitude", start, NULL, edges,
                    (VOIDP) l2_str->longitude);
            readDS(ds_id_ll[ifile][1], "latitude", start, NULL, edges,
                    (VOIDP) l2_str->latitude);
        } else {
            readdata_cached(ds_id_ll[ifile][0], "longitude",
                    start, NULL, edges,
                    (VOIDP) l2_str->longitude, scan_in_rowgroup,
                    &cache_longitude, DFNT_FLOAT32);
            readdata_cached(ds_id_ll[ifile][1], "latitude",
                    start, NULL, edges,
                    (VOIDP) l2_str->latitude, scan_in_rowgroup,
                    &cache_latitude, DFNT_FLOAT32);
        }
        break;


    case 1:

        /* Read subsampled lon/lat fields */
        /* ------------------------------ */
        edges[1] = n_cntl_pnts;
        if (scan_in_rowgroup == NULL) {
            readDS(ds_id_ll[ifile][0], "longitude", start, NULL, edges,
                    (VOIDP) l2_str->lon_cntl);
            readDS(ds_id_ll[ifile][1], "latitude", start, NULL, edges,
                    (VOIDP) l2_str->lat_cntl);
        } else {
            readdata_cached(ds_id_ll[ifile][0], "longitude",
                    start, NULL, edges,
                    (VOIDP) l2_str->lon_cntl, scan_in_rowgroup,
                    &cache_longitude, DFNT_FLOAT32);
            readdata_cached(ds_id_ll[ifile][1], "latitude",
                    start, NULL, edges,
                    (VOIDP) l2_str->lat_cntl, scan_in_rowgroup,
                    &cache_latitude, DFNT_FLOAT32);
        }

        /* Setup buffers used to remove bad control points */
        /* ----------------------------------------------- */
        cntl_pnt_buf1 = (float32 *) calloc(n_cntl_pnts, sizeof (float32));
        cntl_pnt_buf2 = (float32 *) calloc(n_cntl_pnts, sizeof (float32));


        /* Interpolate Latitude */
        /* -------------------- */

        /* Remove bad lat from lat_cntl & cntl_pnts */
        n_cntl_pnts_removed = 0;
        for (i = 0; i < n_cntl_pnts; i++) {
            if (l2_str->lat_cntl[i] < -91 || l2_str->lat_cntl[i] > +91) {
                n_cntl_pnts_removed++;
            } else {
                cntl_pnt_buf1[i - n_cntl_pnts_removed] = l2_str->lat_cntl[i];
                cntl_pnt_buf2[i - n_cntl_pnts_removed] = l2_str->cntl_pnts_cache[i];
            }
        }

        if (((float32) n_cntl_pnts_removed) / n_cntl_pnts > lon_lat_lim) {
            fprintf(stderr, "%s (Latitude failure)\n", l2_str->filename);
            fprintf(stderr, "More that 10%% failure.\n");

            return 1;
        }

        n_cntl_pnts -= n_cntl_pnts_removed;

        for (i = 0; i < n_cntl_pnts; i++) {
            l2_str->lat_cntl[i] = cntl_pnt_buf1[i];
            l2_str->cntl_pnts[i] = cntl_pnt_buf2[i];
        }

        spline(l2_str->cntl_pnts, l2_str->lat_cntl, n_cntl_pnts,
                1e30, 1e30, l2_str->spline_arr);
        for (i = 0; i < l2_str->nsamp; i++) {
            splint(l2_str->cntl_pnts, l2_str->lat_cntl,
                    l2_str->spline_arr, n_cntl_pnts,
                    i + 1.0, &l2_str->latitude[i]);
        }

        // Added JMG 03/14/11
        n_cntl_pnts += n_cntl_pnts_removed;

        /* Interpolate Longitude */
        /* --------------------- */

        /* Remove bad lon from lon_cntl & cntl_pnts */
        n_cntl_pnts_removed = 0;
        for (i = 0; i < n_cntl_pnts; i++) {
            if (l2_str->lon_cntl[i] < -181 || l2_str->lon_cntl[i] > +181) {
                n_cntl_pnts_removed++;
            } else {
                cntl_pnt_buf1[i - n_cntl_pnts_removed] = l2_str->lon_cntl[i];
                cntl_pnt_buf2[i - n_cntl_pnts_removed] = l2_str->cntl_pnts_cache[i];
            }
        }

        if (((float32) n_cntl_pnts_removed) / n_cntl_pnts > lon_lat_lim) {
            fprintf(stderr, "%s (Longitude failure)\n", l2_str->filename);
            fprintf(stderr, "More that 10%% failure.\n");
            return 1;
        }

        n_cntl_pnts -= n_cntl_pnts_removed;

        for (i = 0; i < n_cntl_pnts; i++) {
            l2_str->lon_cntl[i] = cntl_pnt_buf1[i];
            l2_str->cntl_pnts[i] = cntl_pnt_buf2[i];
        }


        /* Remove any dateline discontinuity in the longitudes */
        /* --------------------------------------------------- */
        for (i = 1; i < n_cntl_pnts; i++) {
            delta = l2_str->lon_cntl[i] - l2_str->lon_cntl[i - 1];
            if (delta < -180) l2_str->lon_cntl[i] += 360;
            else if (delta > 180) l2_str->lon_cntl[i] -= 360;
        }

        spline(l2_str->cntl_pnts, l2_str->lon_cntl, n_cntl_pnts,
                1e30, 1e30, l2_str->spline_arr);

        for (i = 0; i < l2_str->nsamp; i++) {
            splint(l2_str->cntl_pnts, l2_str->lon_cntl, l2_str->spline_arr,
                    n_cntl_pnts,
                    i + 1.0, &l2_str->longitude[i]);

            /* Put the longitudes back in the [-180,180] range */
            /* ----------------------------------------------- */
            while (l2_str->longitude[i] > 180)
                l2_str->longitude[i] -= 360;
            while (l2_str->longitude[i] < -180)
                l2_str->longitude[i] += 360;
        }

        n_cntl_pnts += n_cntl_pnts_removed;

        free(cntl_pnt_buf1);
        free(cntl_pnt_buf2);

        break;

    case 2:
        edges[2] = 3;
        for (i = 0; i < 4; i++) {
            edges[1] = geo_edge[i];
            if (ds_id_geonav[ifile][i].sid != -1) {
                readDS(ds_id_file[ifile], geonav_name[i], start, NULL, edges,
                        (VOIDP) geonav[i]);
            }
        }


        geonav_(geonav[0], geonav[1], geonav[2], geonav[3], (int32*) & nsta[ifile],
                (int32*) & ninc[ifile], (int32*) & l2_str->nsamp, l2_str->latitude,
                l2_str->longitude, (float *) databuf[ifile],
                (float *) databuf[ifile], (float *) databuf[ifile],
                (float *) databuf[ifile]);

        break;
    }

    return 0;
}

int32 closeL2(l2_prod *l2_str, int32 ifile) {
    int32 i;
    int32 status;

    for (i = 0; i < l2_str->nprod; i++) {
        if (ds_id_prod[ifile][i].sid != -1) {
            status = endaccessDS(ds_id_prod[ifile][i]);
            if (status != 0) {
                printf("Error ending access to product sds: %d for file: %d\n",
                        i, ifile);
                exit(-1);
            }
        }
    }

    for (i = 0; i < 3; i++) {
        if (ds_id_ll[ifile][i].sid != -1) {
            ds_id_file[ifile].sid = ds_id_ll[ifile][i].sid;
            status = endaccessDS(ds_id_file[ifile]);
            if (status != 0) {
                printf("Error ending access to ll sds: %d for file: %d\n", i, ifile);
                exit(-1);
            }
        }
    }

    if (l2_str->geointerp == 2) {
        for (i = 0; i < 6; i++) {
            ds_id_file[ifile].sid = ds_id_geonav[ifile][i].sid;
            if (ds_id_geonav[ifile][i].sid != -1)
                status = endaccessDS(ds_id_file[ifile]);
            if (status != 0) {
                printf("Error ending access to geonav sds: %d for file: %d\n",
                        i, ifile);
                exit(-1);
            }
        }
    }

    if (ds_id_l2_flags[ifile].sid != -1) {
        status = endaccessDS(ds_id_l2_flags[ifile]);
        if (status != 0) {
            printf("Error ending access to l2_flags sds for file: %d\n", ifile);
            exit(-1);
        }
    }


    if (ds_id_eng_qual[ifile].sid != -1) {
        status = endaccessDS(ds_id_eng_qual[ifile]);
        if (status != 0) {
            printf("Error ending access to eng_qual sds for file: %d\n", ifile);
            exit(-1);
        }
    }


    if (ds_id_s_flags[ifile].sid != -1) {
        status = endaccessDS(ds_id_s_flags[ifile]);
        if (status != 0) {
            printf("Error ending access to s_flags sds for file: %d\n", ifile);
            exit(-1);
        }
    }

    if (ds_id_nflag[ifile].sid != -1) {
        status = endaccessDS(ds_id_nflag[ifile]);
        if (status != 0) {
            printf("Error ending access to n_flag sds for file: %d\n", ifile);
            exit(-1);
        }
    }

    if (ds_id_pixnum[ifile].sid != -1) {
        status = endaccessDS(ds_id_pixnum[ifile]);
        if (status != 0) {
            printf("Error ending access to pixnum sds for file: %d\n", ifile);
            exit(-1);
        }
    }

    status = endDS(ds_id_file[ifile]);
    if (status != 0) {
        printf("Error ending access to file: %d\n", ifile);
        exit(-1);
    }

    return 0;
}

int32 freeL2(l2_prod *l2_str) {
    int32 i;

    if (l2_str == NULL) {

        for (i = 0; i < MAXNFILES; i++) {
            if (databuf[i] != NULL) free(databuf[i]);
            if (prodlist[i] != NULL) free(prodlist[i]);
        }

    } else {

        if (l2_str->geointerp == 1) {
            free(l2_str->lon_cntl);
            free(l2_str->lat_cntl);
            free(l2_str->spline_arr);
            free(l2_str->cntl_pnts);
        }

        free(l2_str->l2_data);
        free(l2_str->geoloc);

        if (l2_str->l2_flags) free(l2_str->l2_flags);
        if (l2_str->flagnames) free(l2_str->flagnames);
        if (l2_str->pixnum) free(l2_str->pixnum);
        if (l2_str->mside) free(l2_str->mside);
        if (l2_str->detnum) free(l2_str->detnum);
        if (l2_str->year_cache) free(l2_str->year_cache);
        if (l2_str->day_cache) free(l2_str->day_cache);
        if (l2_str->msec_cache) free(l2_str->msec_cache);

    }

    return 0;
}

int32 findprod(l2_prod *l2_str, char* prodname) {
    int32 i;

    for (i = 0; i < l2_str->nprod; i++) {
        if (strcmp(l2_str->prodname[i], prodname) == 0) return i;
    }

    return -1;
}

int32 readL2meta(meta_l2Type *meta_l2, int32 ifile) {
    int32 dtype;
    int32 count;
    idDS ds_id;
    int status;
    char* titleStr;
    char* infilesStr;
    char* sennmeStr;
    char* dcenterStr;
    char* ntimeStr;
    char* snodeStr;
    char* enodeStr;
    char* missionStr;
    char* orbnumStr;
    char* nlatStr;
    char* slatStr;
    char* elonStr;
    char* wlonStr;
    char* stclatStr;
    char* stclonStr;
    char* endclatStr;
    char* endclonStr;
    char* nodelStr;

    ds_id = ds_id_file[ifile];

    meta_l2->title = NULL;
    meta_l2->infiles = NULL;
    meta_l2->sensor_name = NULL;
    meta_l2->data_center = NULL;
    meta_l2->ctime = NULL;
    meta_l2->ntime = NULL;
    meta_l2->snode = NULL;
    meta_l2->enode = NULL;
    meta_l2->mission = NULL;
    meta_l2->mission_char = NULL;
    meta_l2->sensor = NULL;
    meta_l2->sensor_char = NULL;

    // fill up metadata names
    if (ds_id.fftype == DS_NCDF) {
        titleStr = TITLE;
        infilesStr = INFILES;
        sennmeStr = SENNME;
        dcenterStr = DCENTER;
        ntimeStr = NTIME;
        snodeStr = SNODE;
        enodeStr = ENODE;
        missionStr = MISSION;
        orbnumStr = ORBNUM;
        nlatStr = NLAT;
        slatStr = SLAT;
        elonStr = ELON;
        wlonStr = WLON;
        stclatStr = STCLAT;
        stclonStr = STCLON;
        endclatStr = ENDCLAT;
        endclonStr = ENDCLON;
        nodelStr = NODEL;
    } else {
        titleStr = TITLE_OLD;
        infilesStr = INFILES_OLD;
        sennmeStr = SENNME_OLD;
        dcenterStr = DCENTER_OLD;
        ntimeStr = NTIME_OLD;
        snodeStr = SNODE_OLD;
        enodeStr = ENODE_OLD;
        missionStr = MISSION_OLD;
        orbnumStr = ORBNUM_OLD;
        nlatStr = NLAT_OLD;
        slatStr = SLAT_OLD;
        elonStr = ELON_OLD;
        wlonStr = WLON_OLD;
        stclatStr = STCLAT_OLD;
        stclonStr = STCLON_OLD;
        endclatStr = ENDCLAT_OLD;
        endclonStr = ENDCLON_OLD;
        nodelStr = NODEL_OLD;
    }

    status = infoAttr(ds_id, titleStr, &dtype, &count);
    if (status != 0) {
        printf("Error - Could not find global attribute \"%s\" in file: %d\n", titleStr, ifile);
        exit(-1);
    }
    meta_l2->title = readAttrStr(ds_id, titleStr);

    if (ds_id.fftype == DS_NCDF)
        ds_id.fid = grp_id[ifile][5];
    if (findAttr(ds_id, infilesStr) != -1) {
        meta_l2->infiles = readAttrStr(ds_id, infilesStr);
    }
    if (ds_id.fftype == DS_NCDF)
        ds_id = ds_id_file[ifile];

    if (findAttr(ds_id, dcenterStr) != -1) {
        meta_l2->data_center = readAttrStr(ds_id, dcenterStr);
    }

    if (findAttr(ds_id, NFREC) != -1)
        readAttr(ds_id, NFREC, (VOIDP) & meta_l2->nfrec);

    if (findAttr(ds_id, PCTFLAG_OLD) != -1)
        readAttr(ds_id, PCTFLAG_OLD, (VOIDP) meta_l2->flags_pc);

    if (findAttr(ds_id, ntimeStr) != -1) {
        meta_l2->ntime = readAttrStr(ds_id, ntimeStr);
    }

    if (findAttr(ds_id, snodeStr) != -1) {
        meta_l2->snode = readAttrStr(ds_id, snodeStr);
    }

    if (findAttr(ds_id, enodeStr) != -1) {
        meta_l2->enode = readAttrStr(ds_id, enodeStr);
    }

    if (ds_id.fftype == DS_HDF) {
        if (findAttr(ds_id, sennmeStr) != -1) {
            meta_l2->sensor_name = readAttrStr(ds_id, sennmeStr);
        }

        if (findAttr(ds_id, missionStr) != -1) {
            meta_l2->mission = readAttrStr(ds_id, missionStr);
        }

        int sensorID = sensorName2SensorId(meta_l2->sensor_name);
        if(sensorID < 0)
            meta_l2->sensor = NULL;
        else
            meta_l2->sensor = strdup(sensorId2InstrumentName(sensorID));
    } else {
        if (findAttr(ds_id, sennmeStr) != -1) {
            meta_l2->sensor = readAttrStr(ds_id, sennmeStr);
        }

        if (findAttr(ds_id, missionStr) != -1) {
            meta_l2->mission = readAttrStr(ds_id, missionStr);
        }

        meta_l2->sensor_name = strdup(instrumentPlatform2SensorName(meta_l2->sensor, meta_l2->mission));
    }

    if (findAttr(ds_id, MSNCHAR) != -1) {
        meta_l2->mission_char = readAttrStr(ds_id, MSNCHAR);
    }

    if (findAttr(ds_id, SNSCHAR) != -1) {
        meta_l2->sensor_char = readAttrStr(ds_id, SNSCHAR);
    }

    readAttr(ds_id, orbnumStr, (VOIDP) & meta_l2->orbnum);
    readAttr(ds_id, nlatStr, (VOIDP) & meta_l2->northlat);
    readAttr(ds_id, slatStr, (VOIDP) & meta_l2->southlat);
    readAttr(ds_id, wlonStr, (VOIDP) & meta_l2->westlon);
    readAttr(ds_id, elonStr, (VOIDP) & meta_l2->eastlon);
    readAttr(ds_id, stclatStr, (VOIDP) & meta_l2->startclat);
    readAttr(ds_id, stclonStr, (VOIDP) & meta_l2->startclon);
    readAttr(ds_id, endclatStr, (VOIDP) & meta_l2->endclat);
    readAttr(ds_id, endclonStr, (VOIDP) & meta_l2->endclon);
    readAttr(ds_id, nodelStr, (VOIDP) & meta_l2->nodel);

    if (findAttr(ds_id, LAC_PX_ST) != -1)
        readAttr(ds_id, LAC_PX_ST, (VOIDP) & meta_l2->pix_start);
    if (findAttr(ds_id, LAC_PX_SUBSAMP) != -1)
        readAttr(ds_id, LAC_PX_SUBSAMP, (VOIDP) & meta_l2->pix_sub);

    return 0;
}

int32 freeL2meta(meta_l2Type *meta_l2) {
#define FREE(ptr) if((ptr) != 0x0) free(ptr);

    FREE(meta_l2->title);
    FREE(meta_l2->infiles);
    FREE(meta_l2->sensor_name);
    FREE(meta_l2->data_center);
    FREE(meta_l2->ctime);
    FREE(meta_l2->ntime);
    FREE(meta_l2->snode);
    FREE(meta_l2->enode);
    FREE(meta_l2->mission);
    FREE(meta_l2->mission_char);
    FREE(meta_l2->sensor);
    FREE(meta_l2->sensor_char);

    return 0;
}

int32 getL3units(l2_prod *l2_str, int32 ifile, char *l3b_prodname, char *units) {
    intn i;

    char bufnum[128];
    char bufden[128];
    char* char_ptr;
    char* tmpStr;

    char_ptr = strchr(l3b_prodname, '/');
    if (char_ptr != NULL) *char_ptr = 0;

    memset(bufnum, 0, 128);
    memset(bufden, 0, 128);
    for (i = 0; i < l2_str[ifile].nprod; i++) {
        if (strcmp(l3b_prodname, l2_str[ifile].prodname[i]) == 0) {
            idDS ds_id0 = {-ds_id_prod[ifile][i].fid,
                ds_id_prod[ifile][i].sid,
                ds_id_prod[ifile][i].fftype};
            tmpStr = readAttrStr(ds_id0, "units");
            if(tmpStr) {
                strncpy(bufnum, tmpStr, 127);
                free(tmpStr);
            } else
                strcpy(bufnum, "undefined");
            break;
        }
    }

    if (char_ptr != NULL) {
        for (i = 0; i < l2_str[ifile].nprod; i++) {
            if (strcmp(char_ptr + 1, l2_str[ifile].prodname[i]) == 0) {
                idDS ds_id0 = {-ds_id_prod[ifile][i].fid,
                    ds_id_prod[ifile][i].sid,
                    ds_id_prod[ifile][i].fftype};
                tmpStr = readAttrStr(ds_id0, "units");
                if(tmpStr) {
                    strncpy(bufden, tmpStr, 127);
                    free(tmpStr);
                } else
                    strcpy(bufden, "undefined");
                break;
            }
        }

        if (strcmp(bufnum, bufden) == 0) {
            if(strcmp(bufnum, "undefined") == 0)
                strcpy(units, "undefined");
            else
                strcpy(units, "dimensionless");
        } else if (strcmp(bufnum, "dimensionless") == 0) {
            strcpy(units, "1 / ");
            strcat(units, bufden);
        } else if (strcmp(bufden, "dimensionless") == 0) {
            strcpy(units, bufnum);
        } else {
            strcpy(units, bufnum);
            strcat(units, " / ");
            strcat(units, bufden);
        }

        *char_ptr = '_';
    } else {
        strcpy(units, bufnum);
    }

    return 0;
}

