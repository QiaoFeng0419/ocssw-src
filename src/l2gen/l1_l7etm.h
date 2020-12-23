#ifndef L1_L7ETM_H
#define L1_L7ETM_H

int openl1_l7etm( filehandle *l1file );
int readl1_l7etm( filehandle *l1file, int recnum, l1str *l1rec, int lonlat);
int closel1_l7etm( filehandle *l1file );
int get_l57tm_nom_angles( char *meta_filename, int32_t npix, int32_t nscan, int32_t iscan,
        float *solz, float *sola, float *senz, float *sena);
#endif
