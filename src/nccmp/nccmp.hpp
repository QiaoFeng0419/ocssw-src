/*
   Copyright (C) 2004-2006, 2009 Remik Ziemlinski <first d0t surname att n0aa d0t g0v>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#ifndef NCCMP_H
#define NCCMP_H 1
#include "opt.h"
#include <netcdf.h>
#include <math.h>
#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <string>

#ifdef MACINTOSH
typedef unsigned int uint;
#endif

#include "nccmp_user_type.h"

typedef struct MISSING_STRUCT {
    nc_type type;

    union {
        char c;
        int8_t b;
        uint8_t ub;
        short s;
        uint16_t us;
        int i;
        uint ui;
        long l;
        uint64_t ul;
        float f;
        double d;
    };
} missing_struct;

typedef struct VARSTRUCT {
    char name[NC_MAX_NAME];
    int varid;
    off_t len; /* # elements per record (or overall if static). */
    size_t dimlens[NC_MAX_VAR_DIMS];
    nc_type type;
    int ndims;
    int dimids[NC_MAX_VAR_DIMS];
    char hasrec; /* has record dimension? */
    int natts;
    int user_type_idx;
    char hasmissing;
    int notsupported;
    missing_struct missing;
} varstruct;

typedef struct DIMSTRUCT {
    int dimid;
    size_t len;
    char name[NC_MAX_NAME];
} dimstruct;

typedef struct GROUP_NODE {
    int groupID;
    char groupName[NC_MAX_NAME];
    int ngroups;
    struct GROUP_NODE *childGroups;
} GROUP_NODE;

int openfiles(nccmpopts* opts, int *ncid1, int *ncid2);
int nccmp(nccmpopts* opts);
int nccmpmetadata(nccmpopts* opts, int ncid1, int ncid2);
int nccmpdata(nccmpopts* opts, int ncid1, int ncid2);
nccmp_user_type_t* getvarinfo(int ncid, varstruct* vars, int* nvars, int debug);
void type2string(nc_type type, char* str);
int excludevars(int ncid1, int ncid2, char** finallist,
        int nfinal, char** excludelist, int nexclude);

int allvarnames(char** list, int nvars, int ncid1, int ncid2);
int cmpattval(int nc1, int nc2, int varid1, int varid2, char* name, int len, nc_type type);
int findvar(char * name, varstruct *vars);
void handle_error(int status);
int odometer(size_t* odo, size_t* limits, int first, int last);
int compareGroup(nccmpopts* opts, int ncid1, int ncid2);

extern "C" nccmp_user_type_t* nccmp_load_group_usertype_array(int group_id, int *nuser_types);

#endif /* !NCCMP_H */
