#include <stdint.h>
#include <iostream>
#include <fstream>

#define PBUFFER_SIZE 32768

using namespace std;


class l1aFile {
  int ncid;

  int ngrps;
  int gid[10];
  int ev_dids[26];

  int ndims;
  int dimid[1000];
  
  uint32_t SC_records_val;

 public:
  l1aFile();
  ~l1aFile();

  string platform;
  int apktsize;
  int bpktsize;
  int EV_APIDs;
  
  int createl1( char* l1_filename, int32_t sscan, int32_t escan,
                int32_t spixl, int32_t epixl, int32_t iopt_extract);
  int createl1( char* l1_filename, int32_t numScans);

  int parseDims( string dimString, int *numDims, int *varDims);
  
  int getNcid() const { return ncid;}
  int getNDims() const {return ndims;}
  int getDimId (int index) const {return dimid[index];}

  int getGid( int index) { return gid[index];}

  int write_science_data( string platform, int32_t isc,
                          uint16_t (*mbands)[16][6304],
                          uint16_t (*ibands)[32][6400],
                          uint16_t (*dnb)[16][4064],
                          uint8_t (*mqfl)[16],
                          uint8_t (*iqfl)[32],
                          uint8_t (*dqfl)[16]);

  int write_scan_metadata(int32_t isc, 
                          uint8_t (*p1)[180],
                          uint8_t (*hrmets)[146*26],
                          uint8_t (*calmets)[134],
                          uint8_t *mode, int iret,
                          const char* l1a_name,
                          char* VIIRS_packet_file);

  int write_eng_data( int32_t isc, uint8_t (*engdata)[9318]);

  int write_cal_data( int32_t isc,
                      int16_t (*sd_m)[48*16],
                      int16_t (*sv_m)[48*16],
                      int16_t (*bb_m)[48*16],
                      int16_t (*sd_i)[96*32],
                      int16_t (*sv_i)[96*32],
                      int16_t (*bb_i)[96*32],
                      int16_t (*sd_d)[64*16],
                      int16_t (*sv_d)[64*16],
                      int16_t (*bb_d)[64*16]);

  int write_diary( int32_t iyear, int32_t iday, 
                   int32_t ltime, int32_t mtime, 
                   int32_t iyrsc, int32_t idysc, int32_t nscd,
                   double *otime, float (*orb)[6], 
                   double *atime, float (*quat)[4],
                   char *sdir, char *edir);

  int write_adcs_bus( int32_t iyear, int32_t iday, 
                      int32_t ltime, int32_t mtime, 
                      int32_t iyrad, int32_t idyad, 
                      int32_t nadc, int32_t nbus,
                      double *adctime, uint8_t *admandone,
                      int16_t *adfftid,
                      double *bustime, uint8_t *adstate,
                      uint8_t *adsolution,
                      uint8_t *adcpkts[],
                      uint8_t *buspkts[]);

  int write_granule_metadata( int32_t iyear, int32_t iday, 
                              int32_t ltime, int32_t mtime,
                              int32_t orbit,
                              const char* l1a_name,
                              char *sdir, char *edir,
                              uint8_t *p1,
                              int32_t isc, uint8_t *mode,
                              int argc, char *argv[]);

  int openl1( char* l1_filename);

  int copyl1( char *ifilename, char *ofilename, l1aFile* l1_ofile,
              int32_t sscan, int32_t escan, 
              int32_t spixl, int32_t epixl);

  int close();
};

#define SWAP_2(x) ( (((x) & 0xff) << 8) | ((unsigned short)(x) >> 8) )

#define SWAP_4(x) ( ((x) << 24) | \
         (((x) << 8) & 0x00ff0000) | \
         (((x) >> 8) & 0x0000ff00) | \
         ((x) >> 24) )

int convert_diary( int32_t npkts, uint8_t (*dstore)[71], 
                   int32_t *iyr, int32_t *iday, double *otime,
                   float (*orb)[6], double *atime, float (*quat)[4]);

int extract_adcs_bus( int adoffsets[4],
		      int32_t nadc, uint8_t *astore[],
                      int32_t nbus, uint8_t *bstore[], 
                      int32_t *iyr, int32_t *iday, 
                      double *adctime, double *bustime,
                      uint8_t *adstate, uint8_t *admandone, 
                      int16_t *adfftid, uint8_t *adsolution);

int read_packet( fstream *vfileStream, uint8_t *packet, 
                 int32_t *len, int32_t *apid, int32_t *endfile);

int read_viirs_scan_packets( fstream *vfileStream, uint8_t *epacket, 
                             uint8_t (*pbuffer)[PBUFFER_SIZE], 
                             int32_t *npkts, int32_t *endfile);

int unpack_viirs_scan( int32_t npkts, 
                       uint8_t (*pbuffer)[PBUFFER_SIZE],
                       uint16_t (*mbands)[16][6304],
                       uint16_t (*ibands)[32][6400],
                       uint16_t (*dnb)[16][4064],
                       uint8_t (*mqfl)[16],
                       uint8_t (*iqfl)[32],
                       uint8_t (*dqfl)[16],
                       uint8_t (*hrmet)[146]);

int unpack_viirs_cal( int32_t npkts, 
                      uint8_t (*pbuffer)[PBUFFER_SIZE],
                      int16_t (*sdm)[16][48],
                      int16_t (*svm)[16][48],
                      int16_t (*bbm)[16][48],
                      int16_t (*sdi)[32][96],
                      int16_t (*svi)[32][96],
                      int16_t (*bbi)[32][96],
                      int16_t (*sdd)[64],
                      int16_t (*svd)[64],
                      int16_t (*bbd)[64],
                      uint8_t *calmet);
   
int unpack_sci( uint8_t *idat, int32_t nsamp, int16_t *scan, uint8_t *qfl);

int unpack_cal_sci( uint8_t *idat, int32_t nsamps, int32_t ndet, 
                    int16_t *sp, int16_t *bl, int16_t *sd);

uint8_t check_sum( int32_t nc, uint8_t *dat, uint8_t *chk);

extern "C" int isleap(int year);
extern "C" int32_t jday( int16_t i, int16_t j, int16_t k);
extern "C" int jdate( int32_t julian, int32_t *year, int32_t *doy);
extern "C" int yds2tai( int16_t iyr, int16_t idy, double sec, double *tai93);
extern "C" int ccsds_to_yds( uint8_t *cctime, int32_t *iyear, int32_t *iday,
                             double *sec);

//extern "C" int usds( int argc, char *argv[]);
extern "C" int usds( int argc, char *argv[], int32_t inBytes,
                     uint8_t *encryptData, uint16_t *decryptData);

extern "C" long szip_uncompress_memory(int new_options_mask,
                                       int new_bits_per_pixel,
                                       int new_pixels_per_block,
                                       int new_pixels_per_scanline,
                                       const char* in,
                                       long in_bytes,
                                       void* out,
                                       long out_pixels);

int ccsds_to_yds( uint8_t *cctime, 
                  int32_t *iyear, int32_t *iday, double *sec);

int createNCDF( int ncid, const char *sname, const char *lname, 
                const char *standard_name, const char *units,
                void *fill_value,
                const char *flag_values, const char *flag_meanings,
                double low, double high, int nt,
                int rank, int *dimids);

int yds2tai( int16_t iyr, int16_t idy, double sec, double *tai93);

int32_t jday( int16_t i, int16_t j, int16_t k);

int jgleap( int32_t jday, int32_t *leap);

int scan_complete( uint8_t (*pbuffer)[PBUFFER_SIZE], int32_t npkts);

inline
int expandEnvVar( string *sValue) {
  if ( (*sValue).find_first_of( "$" ) == string::npos) return 0;
  string::size_type posEndIdx = (*sValue).find_first_of( "/" );
  if ( posEndIdx == string::npos) return 0;
  const string envVar = sValue->substr (1, posEndIdx - 1);
  char *envVar_str = getenv(envVar.c_str());
  if (envVar_str == 0x0) {
    printf("Environment variable: %s not defined.\n", sValue->c_str());
    exit(1);
  }
  *sValue = envVar_str + (*sValue).substr( posEndIdx);

  return 0;
}
