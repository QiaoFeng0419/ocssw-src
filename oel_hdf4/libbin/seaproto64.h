#ifndef SEAPROTO_H
#define SEAPROTO_H

#ifndef METAL3B_H /* avoid re-inclusion */
#include "meta_l3b64.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct l3b_prod_struct {
    char *l3b_prodlist;
    char *l3b_units;
    char *prodname[100];
    int32_t code[100];

} l3b_prod;

/*  Prototypes  */
int32_t put_l3b_open(char *l3b_path, char *replaces, int32_t fsize,
        char *prod_type, char *ptime, int32_t orbit, int32_t start_orb,
        int32_t end_orb, char *proc_con, char *soft_name, char *soft_ver,
        char *input_parms, l3b_prod *parm_opt, meta_l3bType *meta_l3b);
intn put_l3b_record(int32_t file_id, int32_t nrec, int64_t *binno,
        int16_t *nobs, int16_t *time_rec, int16_t *nscenes, float *weights,
        int8_t *sel_cat, int32_t *flags_set, float *l3b_data,
        l3b_prod *parm_opt);
intn put_l3b_close(int32_t prod_ID, int16_t bin_syear, int16_t bin_sday,
        int16_t bin_eyear, int16_t bin_eday, int16_t syear, int16_t sday,
        int32_t smsec, int16_t eyear, int16_t eday, int32_t emsec,
        char *infiles, char *flag_names, char *flag_use, uint8_t *eng_q_use,
        l3b_prod *parm_opt);

intn get_l3b_open(char *l3b_path, int32_t prod_ID, int16_t *bin_syear,
        int16_t *bin_sday, int16_t *bin_eyear, int16_t *bin_eday,
        int16_t *syear, int16_t *sday, int32_t *smsec, int16_t *eyear,
        int16_t *eday, int32_t *emsec, char *infiles, int32_t *start_orb,
        int32_t *end_orb, char *flag_names, char *flag_use, uint8_t *eng_q_use,
        int32_t *fsize, char *prod_type, int64_t *nbins, int32_t *nrows,
        int32_t *max_row, float *bin_hgt, float *seam_lon, int32_t *ibinr,
        int32_t *nbinr, int32_t *irecr, int32_t *nrecr, meta_l3bType *meta_l3b,
        l3b_prod *parm_opt);

intn get_l3b_record(int32_t prod_ID, int32_t sbin, int32_t nrec,
        l3b_prod *parm_opt, int64_t *binno, int16_t *nobs, int16_t *time_rec,
        int16_t *nscenes, float *weights, int8_t *sel_cat, int32_t *flags_set,
        float *l3b_data);

intn get_l3b_close(int32_t prod_ID);

int32_t get_beg_ext(int32_t n_bins_write, int32_t *binnum_data,
        int32_t *basebin, int32_t nrows, int32_t *beg, int32_t *ext);

int64_t get_beg_ext64(int32_t n_bins_write, int64_t *binnum_data,
        int64_t *basebin, int32_t nrows, int64_t *beg, int32_t *ext);

int32_t wr_vdata(char *outname, int32_t fileid_w, int32_t vgid, char *name,
        char *class1, int32_t n_flds, int32_t n_recs_to_write, char *fldname[],
        int32_t type[], int32_t noext, uint8_t *data, int32_t verbose);

/*****************************/
/* prototypes from libbin    */
/*****************************/

void bin_init(int32_t nrow, int64_t **nbin, int64_t **bbin, double **lbin,
        int64_t *tbin);
void bin2ll(int64_t bin, double *lat, double *lon);
void ll2bin(double lat, double lon, int64_t *bin);
void ll2rc(double lat, double lon, int32_t *row, int32_t *col);
void rc2ll(int32_t row, int32_t col, double *lat, double *lon);
void rc2bin(int32_t row, int32_t col, int64_t *bin);
void bin2rc(int64_t bin, int32_t *row, int32_t *col);
void old_bin2ll(int64_t bin, double *lat, double *lon);

#ifdef __cplusplus
}
#endif

#endif /* SEAPROTO_H */
