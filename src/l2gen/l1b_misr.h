#ifndef L1B_MISR_H
#define L1B_MISR_H

#define N_CAMERAS 9

typedef struct misr_struct {
  uint8_t isSingleFile;
  
  int32_t startBlock;
  int32_t endBlock;

  int32_t fileID[N_CAMERAS];
  int32_t blockTimeID[N_CAMERAS];

  int32_t ocean_block_numbers[180];
  int8_t isOceanBlock[180];
  int8_t offset[180];
  
  double radScaleFactors[4];
  
  double SolAzimuth[180*8][32];
  double SolZenith[180*8][32];

  double SenAzimuth[N_CAMERAS][180*8][32];
  double SenZenith[N_CAMERAS][180*8][32];
} misr_t;

int openl1b_misr(filehandle *l1file);
int getRadScaleFactors( char *file, double rad_scl_factors[4]);
int readl1b_misr(filehandle *l1file, l1str *l1rec);
int reduce_res(ushort rad_data[4][2048]);
int interp_values( double *grid_values, double interpolated_values[16][512]);
int interp_values_dbl( int32_t diff_offset, double *grid_values,
                       double interpolated_values[16][512]);
int closel1b_misr(filehandle *l1file);

#endif
