#include "filehandle.h"
#include "l1_struc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct seabass {
    FILE *fp;
    long int data_start;
    const char *delimiter;
    int32_t *field_indexes;
    int32_t lon_index;
    int32_t lat_index;
    int current_row;
} seabass;

int open_seabass(filehandle *file);
int read_seabass(filehandle *file, l1str *l1rec);
int close_seabass(filehandle *file);

#ifdef __cplusplus
} // extern "C"
#endif

