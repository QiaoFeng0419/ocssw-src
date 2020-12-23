#include <netcdf.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <nc4utils.h>

#define DEFAULT_CHUNK_SIZE 1000

/**
 * nc_init_compress
 *
 *	R. Healy 9/26/2016
 *
 * 	@param[in]nc_id         - int32_t  netcdf file (or group) id
 * 	@param[in]var_id        - int32_t  netcdf variable id
 * 	@param[in]dimids        - int32_t* netcdf dimension ids for variable with var_id
 * 	@param[in]rank          - int32_t  rank (number) of dimensions
 * 	@param[in]chunksize     - size_t*  chunk size for compression for each dimension with dimid
 * 	@param[in]deflate_level - int      deflation level 1=lowest(fastest), 9=highest(slowest)
 *
 * Note: chunksize needs to be positive non 0 for unlimited dimensions
 */
void nc_init_compress(int32_t nc_id, int32_t var_id, int32_t *dimids, int32_t rank,
        size_t *chunksize, int deflate_level) {
    int i, status;

    // if deflate level below 0 then don't deflate
    if (deflate_level < 1)
        return;

    size_t dimlength, newchunk;
    /*
     * vary chunk size based on dimensions
     * looking to keep the chunks around 32Kb, hopefully no larger than 200Kb
     */
    for (i = 0; i < rank; i++) {
        status = nc_inq_dimlen(nc_id, dimids[i], &dimlength);
        // Try to keep as few chunks as possible per scan line
        if (chunksize[i] == 0) { // only do if chunksize isn't set by user
            if (dimlength < 1000) {
                chunksize[i] = floor(dimlength / 3) + 1;
            } else {
                newchunk = floor(dimlength / 100) + 1;
                if (newchunk > 1500) {
                    newchunk = floor(dimlength / 250) + 1;
                    if (newchunk > 40)
                        newchunk = 40;
                }
                chunksize[i] = newchunk;
            }
        }
    }

    //    /* decide whether it is worth compression - dims must be larger than chunks */
    //    for (i = 0; i < rank; i++) {
    //        status = nc_inq_dimlen(nc_id, dimids[i], &dimlength);
    //        if (dimlength < chunksize[i]) {
    //            do_deflate = 0;
    //            break;
    //        }
    //    }

    /* Set compression */
    /* First set chunking */
    status = nc_def_var_chunking(nc_id, var_id, NC_CHUNKED, chunksize);
    if (status != NC_NOERR) {
        printf("-E- %s %d: %s \n", __FILE__, __LINE__,
                nc_strerror(status));
        exit(EXIT_FAILURE);
    }
    /* Now we can set compression */
    status = nc_def_var_deflate(nc_id, var_id, NC_SHUFFLE, 9,
            deflate_level);
    if (status != NC_NOERR) {
        printf("-E- %s %d: %s \n", __FILE__, __LINE__,
                nc_strerror(status));
        exit(EXIT_FAILURE);
    }
}

/**
 * nc_init_compress
 *
 *	R. Healy 9/26/2016
 *
 * 	@param[in]nc_id         - int32_t  netcdf file (or group) id
 * 	@param[in]var_id        - int32_t  netcdf variable id
 * 	@param[in]dimids        - int32_t* netcdf dimension ids for variable with var_id
 * 	@param[in]rank          - int32_t  rank (number) of dimensions
 * 	@param[in]chunksize     - size_t*  chunk size for compression for each dimension with dimid
 * 	@param[in]deflate_level - int      deflation level 1=lowest(fastest), 9=highest(slowest)
 */
int nc_init_compress2(int32_t nc_id, char *varnam, int32_t var_id, int32_t *dimids, int32_t rank,
        size_t *chunksize, int type_size, int deflate_level) {
    int d, status;

    size_t dimlength;

    /* Pick a chunk length for each dimension, if one has not already
     * been picked above. */
    for (d = 0; d < rank; d++)
        if (!chunksize[d]) {
            size_t suggested_size;
            status = nc_inq_dimlen(nc_id, dimids[d], &dimlength);
            suggested_size = (pow((double) DEFAULT_CHUNK_SIZE / (dimlength * type_size),
                    1 / (double) (rank)) * dimlength - .5);
            if (suggested_size > dimlength)
                suggested_size = dimlength;
            chunksize[d] = suggested_size ? suggested_size : 1;
            //		 LOG((4, "nc_def_var_nc4: name %s dim %d DEFAULT_CHUNK_SIZE %d num_values %f type_size %d "
            //		      "chunksize %ld", varnam, d, DEFAULT_CHUNK_SIZE, dimlength, type_size, chunksize[d]));
        }

    /* But did this add up to a chunk that is too big? */
    status = check_chunksizes(type_size, rank, chunksize);
    if (status) {
        /* Other error? */
        if (status != NC_EBADCHUNK)
            return status;

        /* Chunk is too big! Reduce each dimension by half and try again. */
        for (; status == NC_EBADCHUNK; status = check_chunksizes(type_size, rank, chunksize))
            for (d = 0; d < rank; d++)
                chunksize[d] = chunksize[d] / 2 ? chunksize[d] / 2 : 1;
    }

    /* Do we have any big data overhangs? They can be dangerous to
     * babies, the elderly, or confused campers who have had too much
     * beer. */
#define NC_ALLOWED_OVERHANG .1
    for (d = 0; d < rank; d++)
        for (; dimlength % chunksize[d] > dimlength * NC_ALLOWED_OVERHANG;)
            chunksize[d] -= dimlength * NC_ALLOWED_OVERHANG;

    /* First set chunking */
    status = nc_def_var_chunking(nc_id, var_id, NC_CHUNKED, chunksize);
    if (status != NC_NOERR) {
        printf("-E- %s %d: %s \n", __FILE__, __LINE__,
                nc_strerror(status));
        exit(1);
    }
    //do_deflate = chunksize[0] + rank + deflate_level;
    /* Set compression */
    if (deflate_level > 0) {
        /* Now we can set compression */
        status = nc_def_var_deflate(nc_id, var_id, NC_SHUFFLE, deflate_level,
                deflate_level);
        if (status != NC_NOERR) {
            printf("-E- %s %d: %s \n", __FILE__, __LINE__,
                    nc_strerror(status));
            exit(1);
        }
    }
    return 0;
}

/* Check a set of chunksizes to see if they specify a chunk that is too big. */
int check_chunksizes(size_t type_len, int32_t ndims, const size_t *chunksizes) {
    double dprod;
    int d;

    dprod = (double) type_len;
    for (d = 0; d < ndims; d++) {
        if (chunksizes[d] < 1)
            return NC_EINVAL;
        dprod *= (double) chunksizes[d];
    }

    if (dprod > (double) NC_MAX_UINT)
        return NC_EBADCHUNK;

    return NC_NOERR;
}
