/* ============================================================================ */
/* module l1_xcal_hdf.c - functions to read the XCAL cross-calibration file     */
/*                        from MCcalmerge for processing through MSL12          */
/*                                                                              */
/* Written By: B. Franz, NASA/SIMBIOS, December 2003.                           */
/*                                                                              */
/* ============================================================================ */

#include "l1_xcal_hdf.h"
#include "l12_proto.h"

#define OBSERVED  1
#define VICARIOUS 2
#define NPIX      1
#define SBUF      5000

static int32 sd_id;

/* ----------------------------------------------------------------------------------- */
/* openl1_xcal_hdf() - opens an XCAL file for reading.                                 */
/*                                                                                     */
/* B. Franz, SAIC, February 2003.                                                      */

/* ----------------------------------------------------------------------------------- */
int openl1_xcal_hdf(filehandle *file) {
    int32 nscan;
    int32 dims[3];

    /* Open the HDF input file */
    sd_id = SDstart(file->name, DFACC_RDONLY);
    if (sd_id == FAIL) {
        fprintf(stderr, "-E- %s line %d: SDstart(%s, %d) failed.\n",
                __FILE__, __LINE__, file->name, DFACC_RDONLY);
        return (HDF_FUNCTION_ERROR);
    }

    /* Get pixel and scan dimensions */
    if (getDims(sd_id, "fileID", dims) != 0) {
        printf("-E- %s line %d: Could not read HDF file dimensions, %s .\n",
                __FILE__, __LINE__, file->name);
        SDend(sd_id);
        return (FATAL_ERROR);
    }
    if (dims[0] < 0) {
        printf("-E- %s line %d: Invalid dimension information for %s.\n",
                __FILE__, __LINE__, file->name);
        SDend(sd_id);
        return (FATAL_ERROR);
    }
    nscan = dims[0];

    file->npix = 1;
    file->nscan = nscan;
    file->sd_id = sd_id;

    return (LIFE_IS_GOOD);
}


/* ----------------------------------------------------------------------------------- */
/* readl1_xcal_hdf() - reads 1 line (1 pixel) from a cross-calibration (XCAL) file.    */
/*                                                                                     */
/* B. Franz, SAIC, February 2003.                                                      */

/* ----------------------------------------------------------------------------------- */
int readl1_xcal_hdf(filehandle *file, int32 scan, l1str *l1rec) {
    static int16 *year;
    static int16 *day;
    static int32 *msec;
    static int16 *pixnum;
    static int8 *detnum;
    static int8 *mside;
    static float32 *lon;
    static float32 *lat;
    static float32 *solz;
    static float32 *sola;
    static float32 *senz;
    static float32 *sena;
    static float32 *alpha;
    static float32 *Lt;

    static int32 lastscan = -SBUF, firstCall = 1;

    int32 nbands = (int32) file->nbands;
    int32 nscan = (int32) file->nscan;
    int32 indx = scan % SBUF;

    int32 nread;

    if (firstCall == 1) {
        year = (int16*) allocateMemory(SBUF * sizeof (int16), "readl1_xcal_hdf - year");
        day = (int16*) allocateMemory(SBUF * sizeof (int16), "readl1_xcal_hdf - day");
        msec = (int32*) allocateMemory(SBUF * sizeof (int32), "readl1_xcal_hdf - msec");
        pixnum = (int16*) allocateMemory(SBUF * sizeof (int16), "readl1_xcal_hdf - pixnum");
        detnum = (int8*) allocateMemory(SBUF * sizeof (int8), "readl1_xcal_hdf - detnum");
        mside = (int8*) allocateMemory(SBUF * sizeof (int8), "readl1_xcal_hdf - mside");
        lon = (float32*) allocateMemory(SBUF * sizeof (float32), "readl1_xcal_hdf - lon");
        lat = (float32*) allocateMemory(SBUF * sizeof (float32), "readl1_xcal_hdf - lat");
        solz = (float32*) allocateMemory(SBUF * sizeof (float32), "readl1_xcal_hdf - solz");
        sola = (float32*) allocateMemory(SBUF * sizeof (float32), "readl1_xcal_hdf - sola");
        senz = (float32*) allocateMemory(SBUF * sizeof (float32), "readl1_xcal_hdf - senz");
        sena = (float32*) allocateMemory(SBUF * sizeof (float32), "readl1_xcal_hdf - sena");
        alpha = (float32*) allocateMemory(SBUF * sizeof (float32), "readl1_xcal_hdf - alpha");
        Lt = (float32 *) allocateMemory(file->nbands * SBUF * sizeof (float32), "readl1_xcal_hdf - Lt");
        firstCall = 0;
    }
    /* Do we need to read the next buffered block? */
    if (scan / SBUF != lastscan / SBUF) {

        if (scan + SBUF < nscan)
            nread = SBUF;
        else
            nread = nscan - scan + 1;

        printf("%d %d %d\n", scan, nread, nscan);

        /* Read scan time */
        READ_SDS_ID(sd_id, "year", year, scan, 0, 0, 0, nread, 1, 1, 1);
        READ_SDS_ID(sd_id, "day", day, scan, 0, 0, 0, nread, 1, 1, 1);
        READ_SDS_ID(sd_id, "msec", msec, scan, 0, 0, 0, nread, 1, 1, 1);

        /* Get location within instrument */
        READ_SDS_ID(sd_id, "ipix", pixnum, scan, 0, 0, 0, nread, 1, 1, 1);
        READ_SDS_ID(sd_id, "idet", detnum, scan, 0, 0, 0, nread, 1, 1, 1);
        READ_SDS_ID(sd_id, "mside", mside, scan, 0, 0, 0, nread, 1, 1, 1);

        /* Get position and path geometry */
        READ_SDS_ID(sd_id, "lon", lon, scan, 0, 0, 0, nread, 1, 1, 1);
        READ_SDS_ID(sd_id, "lat", lat, scan, 0, 0, 0, nread, 1, 1, 1);
        READ_SDS_ID(sd_id, "solz", solz, scan, 0, 0, 0, nread, 1, 1, 1);
        READ_SDS_ID(sd_id, "sola", sola, scan, 0, 0, 0, nread, 1, 1, 1);
        READ_SDS_ID(sd_id, "senz", senz, scan, 0, 0, 0, nread, 1, 1, 1);
        READ_SDS_ID(sd_id, "sena", sena, scan, 0, 0, 0, nread, 1, 1, 1);
        READ_SDS_ID(sd_id, "alpha", alpha, scan, 0, 0, 0, nread, 1, 1, 1);

        /* Read radiances */
        READ_SDS_ID(sd_id, "Lt", Lt, scan, 0, 0, 0, nread, nbands, 1, 1);
    }

    l1rec->scantime = yds2unix(year[indx], day[indx], (double) (msec[indx] / 1.e3));

    l1rec->pixnum[0] = (int32_t) pixnum [indx];
    l1rec->detnum = (int32_t) detnum [indx];
    l1rec->mside = (int32_t) mside [indx];

    l1rec->lon [0] = lon [indx];
    l1rec->lat [0] = lat [indx];
    l1rec->solz [0] = solz [indx];
    l1rec->sola [0] = sola [indx];
    l1rec->senz [0] = senz [indx];
    l1rec->sena [0] = sena [indx];
    l1rec->alpha[0] = alpha [indx];

    memcpy(l1rec->Lt, &Lt[indx * nbands], nbands);

    l1rec->npix = 1;
    l1rec->iscan = scan;

    lastscan = scan;

    return (LIFE_IS_GOOD);
}

int closel1_xcal_hdf(filehandle *file) {
    if (SDend(sd_id)) {
        fprintf(stderr, "-E- %s line %d: SDend(%d) failed for file, %s.\n",
                __FILE__, __LINE__, sd_id, file->name);
        return (HDF_FUNCTION_ERROR);
    }

    return (LIFE_IS_GOOD);
}





