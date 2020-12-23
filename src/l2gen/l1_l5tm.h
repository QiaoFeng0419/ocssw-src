#ifndef L1_L5TM_H
#define L1_L5TM_H

int openl1_l5tm( filehandle *l1file );
int readl1_l5tm( filehandle *l1file, int recnum, l1str *l1rec, int lonlat);
int closel1_l5tm( filehandle *l1file );
int get_l57tm_nom_angles( char *meta_filename, int32_t npix, int32_t nscan, int32_t iscan,
        float *solz, float *sola, float *senz, float *sena);
int get_l5tm_angles( char *emeta_filename, int32_t npix, int32_t nscan, int32_t iscan,
        float *solz, float *sola, float *senz, float *sena);
int32_t chk_l5tm_geo(char *fname);
#endif
