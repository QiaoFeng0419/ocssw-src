#ifndef SEAPROTOI_H  /* avoid re-inclusion */
#define SEAPROTOI_H

/*  Prototypes  */
int init_bins(int nrows, float seam_lon);
void binxf_bin2ll(int idx, float *lat, float *lon, float *dellat,
        float *dellon);
int32_t get_coords(int64_t *begin, int64_t *lastdatabin, int32_t lastrec,
        float *nlat, float *slat, float *elon, float *wlon);
int32_t setupgrid(int32_t fileid, int32_t fid);
int32_t closegrid(int32_t fileid, int32_t gridid);
int32_t writegeom(int32_t fid, int32_t nbins);
int32_t setupindex(int32_t fid);
intn writeindex(int32_t ndxid, int64_t *start_num, int64_t *begin,
        int32_t *extent, int64_t *maxbin);
intn closeindex(int32_t ndxid);
int32_t setupmaster(int32_t fid, int32_t fsize);
int32_t *setupslaves(int32_t fid, int32_t fsize, char *l3b_path,
        l3b_prod *parm_opt);
intn closedata(int32_t fileid, int32_t mstrid, int32_t *slvid,
        l3b_prod *parm_opt);
intn buffbins(int32_t *fstcall, int32_t fileid, int32_t mstrid, int32_t *slvid,
        int32_t nrec, int64_t *binno, int16_t *nobs, int16_t *ttag,
        int16_t *nscenes, float *weights, int8_t *sel_cat, int32_t *flags_set,
        float *l3b_data, l3b_prod *parm_opt);
intn flushbuff(int32_t fileid, int32_t mstrid, int32_t *slvid,
        l3b_prod *parm_opt);
int32_t writeattr(int32_t fid, char *name, int32_t nt, intn count, VOIDP data);

#endif /* SEAPROTOI_H */
