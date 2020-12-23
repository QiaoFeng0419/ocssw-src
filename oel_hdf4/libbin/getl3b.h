#ifndef GETL3B_H /* avoid re-inclusion */
#define GETL3B_H

#include "seaproto.h"

#ifdef __cplusplus
extern "C" {
#endif

intn get_fsize(char *prod_type);

intn get_bins(int32_t fid);

intn rd_common_data(int32_t vskey, int32_t prod_ID, int32_t start,
        int32_t nelts);

intn getindex(int32_t fst_call, int32_t prod_ID, int32_t vskey, int32_t sbin,
        int32_t nrec, int32_t *begin, int32_t *extent, int32_t *begin_index,
        int32_t *sum_extent, int32_t *bins_infile, int32_t *indexlist,
        int32_t *binno);

intn out_data(int32_t fst_call, int32_t fid, int32_t sdfid, int32_t prod_ID,
        l3b_prod *parm_opt, int32_t sbin, int32_t nrec, int32_t binlist_key,
        int32_t *begin, int32_t *extent, int32_t *p_vskeys, int32_t *last_bin,
        int32_t *binno, int16_t *nobs, int16_t *time_rec, int16_t *nscenes,
        float *weights, int8_t *sel_cat, int32_t *flags_set, float *l3b_data);

intn alloc_parm_bufs(int32_t prod_ID, int32_t nbins);

intn free_parm_bufs(int32_t prod_ID);

intn rd_data(int32_t prod_ID, int32_t binlist_key, int32_t *p_vskey,
        int32_t start, int32_t nelts, l3b_prod *parm_opt);

intn get_parm_vskeys(int32_t fid, int32_t prod_ID, int32_t *p_vskey);

intn attach_slave(int32_t fid, char *sname, int32_t parm, int32_t *p_vskeys);

intn get_index(int32_t sbin, int32_t prod_ID, int32_t bufsz);

#ifdef __cplusplus
}
#endif

#endif /* GETL3B_H  */
