#include "l12_proto.h"
#include "l12_parms.h"

#define ConfidentCloudy 0
#define ProbablyCloudy  1
#define ProbablyClear   2
#define ConfidentClear  3

int modis_cloud_mask(l1str *l1rec, int32_t ip) {
    static int firstCall = 1;
    static int32 sd_id;
    static int32 sds_id;
    static char* cldmask;
    static int32 start[3];
    static int32 end [3];
    static int lastScan = -1;

    char flag;
    int32 status;

    if (firstCall) {

        char name [H4_MAX_NC_NAME] = "";
        char sdsname[H4_MAX_NC_NAME] = "";
        char file [FILENAME_MAX] = "";
        int32 rank;
        int32 nt;
        int32 dims[H4_MAX_VAR_DIMS];
        int32 nattrs;

        if (strcmp(input->cldfile, "") == 0) {
            printf("-E- %s line %d: no cloud flag file provide.\n",
                    __FILE__, __LINE__);
            exit(1);
        }

        strcpy(file, input->cldfile);
        printf("Loading cloud flag file %s\n", file);

        /* Open the file */
        sd_id = SDstart(file, DFACC_RDONLY);
        if (sd_id == FAIL) {
            fprintf(stderr, "-E- %s line %d: error opening %s for reading.\n",
                    __FILE__, __LINE__, file);
            exit(1);
        }

        strcpy(sdsname, "Cloud_Mask");
        sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
        status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);

        if (dims[2] < l1rec->npix || dims[1] < l1rec->l1file->nscan) {
            fprintf(stderr, "-E- %s line %d: incompatible dimensions (%d,%d) vs (%d,%d)\n",
                    __FILE__, __LINE__, dims[2], dims[1], l1rec->npix, l1rec->l1file->nscan);
            exit(1);
        }

        end[0] = 1;
        end[1] = 1;
        end[2] = dims[2];

        if ((cldmask = (char *) malloc(dims[2] * sizeof (char))) == NULL) {
            fprintf(stderr, "-E- %s line %d: error allocating %d bytes for cloud mask\n",
                    __FILE__, __LINE__, dims[2]);
            exit(1);
        }

        firstCall = 0;
    }

    if (lastScan != l1rec->iscan) {
        start[0] = 0;
        start[1] = l1rec->iscan;
        start[2] = 0;
        status = SDreaddata(sds_id, start, NULL, end, (VOIDP) cldmask);
        if (status != 0) {
            printf("-E- %s Line %d:  Error reading SDS cloud mask at line %d.\n",
                    __FILE__, __LINE__, l1rec->iscan);
            exit(1);
        }
        lastScan = l1rec->iscan;
    }


    flag = (cldmask[l1rec->pixnum[ip]] & 6) / 2;

    if (flag < ProbablyClear)
        return (1);
    else
        return (0);

}

int modis_cirrus_mask(l1str *l1rec, int32_t ip) {
    static int firstCall = 1;
    static int32 sd_id;
    static int32 sds_id;
    static char* cldmask;
    static int32 start[3];
    static int32 end [3];
    static int lastScan = -1;

    char flag;
    int32 status;

    if (firstCall) {

        char name [H4_MAX_NC_NAME] = "";
        char sdsname[H4_MAX_NC_NAME] = "";
        char file [FILENAME_MAX] = "";
        int32 rank;
        int32 nt;
        int32 dims[H4_MAX_VAR_DIMS];
        int32 nattrs;

        if (strcmp(input->cldfile, "") == 0) {
            printf("-E- %s line %d: no cloud flag file provide.\n",
                    __FILE__, __LINE__);
            exit(1);
        }

        strcpy(file, input->cldfile);
        printf("Loading cloud flag file %s\n", file);

        /* Open the file */
        sd_id = SDstart(file, DFACC_RDONLY);
        if (sd_id == FAIL) {
            fprintf(stderr, "-E- %s line %d: error opening %s for reading.\n",
                    __FILE__, __LINE__, file);
            exit(1);
        }

        strcpy(sdsname, "Cloud_Mask");
        sds_id = SDselect(sd_id, SDnametoindex(sd_id, sdsname));
        status = SDgetinfo(sds_id, name, &rank, dims, &nt, &nattrs);

        if (dims[2] < l1rec->npix || dims[1] < l1rec->l1file->nscan) {
            fprintf(stderr, "-E- %s line %d: incompatible dimensions (%d,%d) vs (%d,%d)\n",
                    __FILE__, __LINE__, dims[2], dims[1], l1rec->npix, l1rec->l1file->nscan);
            exit(1);
        }

        end[0] = 1;
        end[1] = 1;
        end[2] = dims[2];

        if ((cldmask = (char *) malloc(dims[2] * sizeof (char))) == NULL) {
            fprintf(stderr, "-E- %s line %d: error allocating %d bytes for cloud mask\n",
                    __FILE__, __LINE__, dims[2]);
            exit(1);
        }

        firstCall = 0;
    }

    if (lastScan != l1rec->iscan) {
        start[0] = 1;
        start[1] = l1rec->iscan;
        start[2] = 0;
        status = SDreaddata(sds_id, start, NULL, end, (VOIDP) cldmask);
        if (status != 0) {
            printf("-E- %s Line %d:  Error reading SDS cloud mask at line %d.\n",
                    __FILE__, __LINE__, l1rec->iscan);
            exit(1);
        }
        lastScan = l1rec->iscan;
    }


    flag = (cldmask[l1rec->pixnum[ip]] & 2);

    if (flag == 0)
        return (1);
    else
        return (0);

}

char get_cloudmask_meris(l1str *l1rec, int32_t ip) {
    // Cloud Masking for MERIS

    static int ib443, ib490, ib620, ib665, ib681, ib709, ib754, ib865, ib885, firstCall = 1;
    int ipb;
    float *rhos = l1rec->rhos, *cloud_albedo = l1rec->cloud_albedo;
    float ftemp, cldtmp;
    char flagcld;

    if (firstCall == 1) {
        ib443 = bindex_get(443);
        ib490 = bindex_get(490);
        ib620 = bindex_get(620);
        ib665 = bindex_get(665);
        ib681 = bindex_get(681);
        ib709 = bindex_get(709);
        ib754 = bindex_get(754);
        ib865 = bindex_get(865);
        ib885 = bindex_get(885);

        if (ib443 < 0 || ib490 < 0 || ib620 < 0 || ib665 < 0 || ib681 < 0 || ib709 < 0 || ib754 < 0 || ib865 < 0 || ib885 < 0) {
            printf("get_habs_cldmask: incompatible sensor wavelengths for this algorithm\n");
            exit(EXIT_FAILURE);
        }
        firstCall = 0;
    }
    flagcld = 0;
    ipb = l1rec->l1file->nbands*ip;

    if (rhos[ipb + ib443] >= 0.0 &&
        rhos[ipb + ib620] >= 0.0 &&
        rhos[ipb + ib665] >= 0.0 &&
        rhos[ipb + ib681] >= 0.0 &&
        rhos[ipb + ib709] >= 0.0 &&
        rhos[ipb + ib754] >= 0.0) {
        // turbidity signal in water
        ftemp = (rhos[ipb + ib620] + rhos[ipb + ib665] + rhos[ipb + ib681]) - 3 * rhos[ipb + ib443] - \
                (rhos[ipb + ib754] - rhos[ipb + ib443]) / (754 - 443)*(620 + 665 + 681 - 3 * 443);
//     cldtmp = cloud_albedo[ip] - 3 * ftemp;
        //switch to rhos_865 where cldalb fails
        if (cloud_albedo[ip] >= 0.0) {
            cldtmp = cloud_albedo[ip];
        } else {
            cldtmp = rhos[ipb + ib865];
        }

        //remove turbidity signal from cloud albedo
        if (ftemp > 0) cldtmp = cldtmp - 3 * ftemp;

        if (cldtmp > 0.08) {
            flagcld = 1;
        }

        // to deal with scum look at relative of NIR and blue for lower albedos
        if ((rhos[ipb + ib754] + rhos[ipb + ib709]) > (rhos[ipb + ib443] + rhos[ipb + ib490]) && cldtmp < 0.1) flagcld = 0;
        if (((rhos[ipb + ib754] + rhos[ipb + ib709]) - (rhos[ipb + ib665] + rhos[ipb + ib681])) > 0.01 && cldtmp < 0.15) flagcld = 0;
        if ((((rhos[ipb + ib754] + rhos[ipb + ib709]) - (rhos[ipb + ib665] + rhos[ipb + ib681])) / cldtmp) > 0.1) flagcld = 0;
        if ((rhos[ipb + ib665] > 0.1) && (cldtmp > 0.15)) {
            flagcld = 1;
        }
    }
    return (flagcld);
}

char get_cloudmask_modis(l1str *l1rec, int32_t ip) {
    // Cloud Masking for MODIS

    int ib469, ib555, ib645, ib667, ib859, ib1240, ib2130, firstCall = 1;
    int ipb;
    float *rhos = l1rec->rhos, *cloud_albedo = l1rec->cloud_albedo;
    float ftemp, ftemp2, ftemp3;
    float cloudthr = 0.027;
    char flagcld;

    if (firstCall == 1) {
        ib469 = bindex_get(469);
        ib555 = bindex_get(555);
        ib645 = bindex_get(645);
        ib667 = bindex_get(667);
        ib859 = bindex_get(859);
        ib1240 = bindex_get(1240);
        ib2130 = bindex_get(2130);
        if (ib469 < 0 || ib555 < 0 || ib645 < 0 || ib667 < 0 || ib859 < 0 || ib1240 < 0 || ib2130 < 0) {
            printf("get_habs_cldmask: incompatible sensor wavelengths for this algorithm\n");
            exit(EXIT_FAILURE);
        }
        firstCall = 0;
    }

    ipb = l1rec->l1file->nbands*ip;
    flagcld = 0;
    ftemp = 0; //rhos[ipb+ib667];
    //      first correct for turbid water

    if (rhos[ipb + ib667] < 0.0) ftemp = 0.0;
    ftemp2 = cloud_albedo[ip] - ftemp;

    if (ftemp2 > 0.027) flagcld = 1;

    //        non-water check  1240 is bright relative to 859 and the combination is bright
    //        this may hit glint by accident, need to be checked.

    if (rhos[ipb + ib1240] / rhos[ipb + ib859] > 0.5 && (rhos[ipb + ib1240] + rhos[ipb + ib2130]) > 0.10) flagcld = 1;


    //        now try to correct for glint
    //        region check was thrown out {IF (region = "OM") cloudthr = 0.04} rjh 11/2/2015

    ftemp = rhos[ipb + ib645] - rhos[ipb + ib555] + (rhos[ipb + ib555] - rhos[ipb + ib859])*(645.0 - 555.0) / (859.0 - 555.0);
    ftemp2 = cloud_albedo[ip] + ftemp;
    if (ftemp2 < cloudthr) flagcld = 0;
    if (rhos[ipb + ib859] / rhos[ipb + ib1240] > 4.0) flagcld = 0;

    //     scum areas

    if ((rhos[ipb + ib859] - rhos[ipb + ib469]) > 0.01 && cloud_albedo[ip] < 0.30) flagcld = 0;
    if ((rhos[ipb + ib859] - rhos[ipb + ib645]) > 0.01 && cloud_albedo[ip] < 0.15) flagcld = 0;
    if (rhos[ipb + ib1240] < 0.2)
        ftemp2 = ftemp2 - (rhos[ipb + ib859] - rhos[ipb + ib1240]) * fabs(rhos[ipb + ib859] - rhos[ipb + ib1240]) / cloudthr;

    ftemp3 = ftemp2;
    if (ftemp2 < cloudthr * 2) {
        if ((rhos[ipb + ib555] - rhos[ipb + ib1240]) > (rhos[ipb + ib469] - rhos[ipb + ib1240])) {
            ftemp3 = ftemp2 - (rhos[ipb + ib555] - rhos[ipb + ib1240]);
        } else {
            ftemp3 = ftemp2 - (rhos[ipb + ib469] - rhos[ipb + ib1240]);
        }
    }

    if (ftemp3 < cloudthr) flagcld = 0;

    return (flagcld);
}

char get_cldmask(l1str *l1rec, int32_t ip) {
    //function for cloud mask by pixel
    switch (l1rec->l1file->sensorID) {
    case MERIS:
    case OLCIS3A:
    case OLCIS3B:
        return (get_cloudmask_meris(l1rec, ip));
        break;
    case MODISA:
    case MODIST:
        return (get_cloudmask_modis(l1rec, ip));
        break;
    default:
        printf("HABS cldmsk not supported for this sensor (%s).\n",
                sensorId2SensorName(l1rec->l1file->sensorID));
        exit(EXIT_FAILURE);
    }
    return (0);
}
