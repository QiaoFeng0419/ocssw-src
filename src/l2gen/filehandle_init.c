#include <string.h>
#include "filehandle.h"

void filehandle_init(filehandle *file) {
    int32_t i;

    strcpy(file->name, "");
    file->format = -1;
    file->sensorID = -1;
    file->subsensorID = -1;
    strcpy(file->spatialResolution, "");
    file->length = 0;
    file->spix = 0;
    file->epix = -1;
    file->dpix = 1;
    file->npix = 0;
    file->ctl_pt_incr = 1;
    file->nscan = 0;
    file->nbands = 0;
    file->nbandsir = 0;
    file->nlvl = 42;  /* fixed 42 GMAO FP-IT levels now */
    file->bindx = NULL;
    file->ndets = 1;
    file->mode = READ;
    strcpy(file->l2prod, "");
    strcpy(file->def_l2prod, "");
    file->sd_id = 0;
    file->tot_prod = 0;
    for (i = 0; i < MAXPROD; i++)
        strcpy(file->l2_prod_names[i], "");
    file->prodptr = NULL;
    file->productInfos = NULL;

    file->calfile = NULL;
    file->geofile = NULL;
    file->orbit_node_lon = -999.0;
    file->orbit_number = 0;
    file->node_crossing_time = 0;
    memset(file->flag_cnt, 0, NFLAGS * sizeof (int32_t));
    file->terrain_corrected = 0;
    file->sv_with_moon = 0;
    for(i=0; i<8; i++)
        file->grp_id[i] = -1;
    
    file->iwave = NULL;
    file->fwave = NULL;
    file->fwhm = NULL;
    file->Fobar = NULL;
    file->Fonom = NULL;
    file->Tau_r = NULL;
    file->k_oz = NULL;
    file->k_no2 = NULL;
    file->aw = NULL;
    file->bbw = NULL;

    file->private_data = NULL;

    file->viirscalparfile = NULL;

    
    return;
}
