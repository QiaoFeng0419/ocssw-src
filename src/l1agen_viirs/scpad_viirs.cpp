#include <math.h>
#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <string.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include "nc4utils.h"
#include "l1agen_viirs.h"

#define SWAP_2(x) ( (((x) & 0xff) << 8) | ((unsigned short)(x) >> 8) )

#define VERSION "1.10"

//    Modification history:
//  Programmer     Organization   Date     Ver   Description of change
//  ----------     ------------   ----     ---   ---------------------
//  Joel Gales     FutureTech     09/25/15 0.10  Original development
//  Joel Gales     FutureTech     09/30/15 0.11  Return 110 if time gaps
//                                               in output file
//  Joel Gales     FutureTech     10/01/15 0.12  Return 120 if no overlap
//                                               with prev/following files
//  Joel Gales     FutureTech     10/01/15 0.13  Set default nPad to 10"
//  Joel Gales     FutureTech     10/01/15 0.14  Add support in case files
//                                               abut with no overlap
//  Joel Gales     FutureTech     10/02/15 0.15  Assume prev/foll files abut
//  Joel Gales     FutureTech     10/05/15 0.16  Remove path from filename
//                                               before determining filetype
//  Joel Gales     FutureTech     10/06/15 0.17  Change toff to 6 for S/C recs
//  Joel Gales     FutureTech     10/06/15 0.18  Change get_pkt_sec() to
//                                               support both input and output
//                                               of julian date 
//  Joel Gales     FutureTech     10/09/15 0.19  Change time comparison to
//                                               (secEnd+nPad + 0.5) for the
//                                               following file to avoid
//                                               roundoff error
//  Joel Gales     FutureTech     10/15/15 0.19  Change time comparison to
//                                               (secEnd+nPad - 0.5) for the
//                                               preceeding file
//  Joel Gales     FutureTech     12/07/16 1.00  Add support for JPSS1
//  Joel Gales     FutureTech     02/27/18 1.10  Exit with 1 if all times in
//                                               following file less than
//                                               endtime (plus pad) of file
//                                               to be padded

using namespace std;

extern "C" int32_t jday( int16_t i, int16_t j, int16_t k);
extern "C" int jdate( int32_t julian, int32_t *year, int32_t *doy);
extern "C" int ccsds_to_yds( uint8_t *cctime, int32_t *iyear, int32_t *iday,
                             double *sec);

double get_pkt_sec( uint8_t *pkt_time, int32_t in_jd, int32_t *out_jd) {

  uint16_t ui16a;
  double sec;
  int16_t iy16, idy16;
  int32_t iy, idy, jd, iyr, iday, jd0;

  if ( in_jd == -1) {
    memcpy( &ui16a, (uint16_t *) pkt_time, 2);
    jd0 = SWAP_2( ui16a) + 2436205;
  } else {
    jd0 = in_jd;
  }

  if ( out_jd != NULL) *out_jd = jd0;

  ccsds_to_yds( pkt_time, &iy, &idy, &sec);
  iy16 = iy;
  idy16 = idy;
  jd = jday( iy16, 1, idy16);
 
  return sec + (jd - jd0)*86400;
}


int main (int argc, char* argv[])
{

  cout << "scpad_viirs " << VERSION << " (" 
       <<  __DATE__ << " " << __TIME__ << ")" << endl;

  if ( argc == 1) {
    cout << endl <<
      "scpad_viirs [-n num-records]" << endl <<
      "          { [-p previous-file]" << endl <<
      "            [-f following-file] }" << endl <<
      "         file-to-pad" << endl <<
      "         [output-file]" << endl;
    return 0;
  }

  int c;
  int index;
  int nPad = 10;
  char *pfile = NULL;
  char *ffile = NULL;
  char *file_to_pad = NULL;
  while ((c = getopt (argc, argv, "n:p:f:")) != -1)
    switch (c)
      {
      case 'n':
        nPad = atoi( optarg);
        break;

      case 'p':
        pfile = optarg;
        break;

      case 'f':
        ffile = optarg;
        break;

      default:
        abort ();
      }

  //  cout << nPad << endl;

  if ( pfile == 0x0 && ffile == 0x0) {
    cout << "Neither previous nor following file specified" << endl;
    return 120;
  }

  fstream xfileStream, pfileStream, ffileStream, ofileStream;

  xfileStream.open( argv[optind], fstream::in | fstream::binary);
  if ( xfileStream.fail()) {
    cout << argv[optind] << " not found" << endl;
    exit(1);
  }


  xfileStream.seekg (0, ios::end);
  int xFileSize = xfileStream.tellg();
  xfileStream.seekg (0, ios::beg);

  char fileType = 0;
  if (strncmp(basename(argv[optind]), "P1570011", 8) == 0) fileType = 'S';
  if (strncmp(basename(argv[optind]), "P1570008", 8) == 0) fileType = 'A';
  if (strncmp(basename(argv[optind]), "P1570000", 8) == 0) fileType = 'B';

  if (strncmp(basename(argv[optind]), "P1590011", 8) == 0) fileType = 'S';
  if (strncmp(basename(argv[optind]), "P1590008", 8) == 0) fileType = 'A';
  if (strncmp(basename(argv[optind]), "P1590000", 8) == 0) fileType = 'B';

  if ( fileType == 0) {
    cout << "File type cannot be determined from filename" << endl;
    exit(1);
  }

  int32_t recSize, toff;
  toff = 6;

  if (strncmp(basename(argv[optind]), "P157", 4) == 0) {
    if (fileType == 'S') {
      recSize = 71;
    } else if (fileType == 'A') {
      recSize = 355;
    } else if (fileType == 'B') {
      recSize = 207;
    }
  }

  if (strncmp(basename(argv[optind]), "P159", 4) == 0) {
    if (fileType == 'S') {
      recSize = 71;
    } else if (fileType == 'A') {
      recSize = 393;
    } else if (fileType == 'B') {
      recSize = 183;
    }
  }
  
  int32_t nRec = xFileSize / recSize;
 
  // Allocate continuous 2-dim array
  uint8_t **pkts = new uint8_t*[nRec];
  pkts[0] = new uint8_t[recSize*nRec];
  for (int i=1; i<nRec; i++) pkts[i] = pkts[i-1] + recSize; 

  // Read entire file into buffer
  xfileStream.read( (char *) &pkts[0][0], xFileSize);
  xfileStream.close();

  uint16_t ui16a;
  double sec;
  int16_t iy16, idy16;
  int32_t iy, idy, jd, iyr, iday, jdtest;

  // Get start time of file to pad

  double secStart = get_pkt_sec( &pkts[0][toff], -1, &jdtest);
  cout << "Start of file to pad: " << secStart << endl;

  double secEnd = get_pkt_sec( &pkts[nRec-1][toff], jdtest, NULL);
  cout << "Stop of file to pad:  " << secEnd << endl;

  //
  // Read previous file
  //
  int iret = 0;
  int iPad = 0;
  uint8_t **pPkts;
  if (pfile != 0x0) {
    pfileStream.open( pfile, fstream::in | fstream::binary);
    if ( pfileStream.fail()) {
      cout << pfile << " not found" << endl;
      exit(1);
    }

    pPkts = new uint8_t*[nPad];
    pPkts[0] = new uint8_t[recSize*nPad];
    for (int i=1; i<nPad; i++) pPkts[i] = pPkts[i-1] + recSize; 

    //    pfileStream.seekg (0, ios::beg);
    int i = 0;
    // Find record just greater than start time of file to pad
    while (1) {
      pfileStream.read( (char *) &pPkts[0][0], recSize);
      if (pfileStream.eof()) {
        cout << "No padding from previous file within " << nPad << " seconds"
             << endl;
        iPad = iPad | 1;
        iret = 110;
        break;
      }

      double sec0 = get_pkt_sec( &pPkts[0][toff], jdtest, NULL);

      //      cout << "prev: " << i << " " << sec0 << endl;
      if (sec0 > (secStart-nPad - 0.5) && (sec0 < secStart)) {
        pfileStream.seekg (-recSize*1, ios::cur);
        pfileStream.read( (char *) &pPkts[0][0], recSize*nPad);
        break;
      }
      i++;
    }
    pfileStream.close();
  } // if (pfile != 0x0)


  //
  // Read following file
  //
  uint8_t **fPkts;
  if (ffile != 0x0) {
    ffileStream.open( ffile, fstream::in | fstream::binary);
    if ( ffileStream.fail()) {
      cout << ffile << " not found" << endl;
      exit(1);
    }

    fPkts = new uint8_t*[nPad];
    fPkts[0] = new uint8_t[recSize*nPad];
    for (int i=1; i<nPad; i++) fPkts[i] = fPkts[i-1] + recSize; 

    ffileStream.seekg (0, ios::beg);
    int i = 0;
    // Find record just greater than start time of file to pad
    while (1) {
      ffileStream.read( (char *) &fPkts[0][0], recSize);

      double sec0 = get_pkt_sec( &fPkts[0][toff], jdtest, NULL);
      //    cout << "foll: " << i << " " << sec0 << endl;
      if (sec0 > (secEnd+nPad + 0.5)) {
        ffileStream.seekg (-recSize*(nPad+1), ios::cur);
        ffileStream.read( (char *) &fPkts[0][0], recSize*nPad);

        //        for (int i=0; i<nPad; i++) 
        //cout << i << " " << sec0 << endl;

        break;
      }
      i++;
      if (i == nRec) {
        cout << "All times in following file less than endtime (plus pad) of file to be padded." << endl;
        exit(1);
      }
    }

    // Read nPad records of following file so that first record
    // is right after end time
    if (i == 0) {
      iPad = iPad | 2;
      iret = 110;
      cout << "No padding from following file within " << nPad << " seconds"
             << endl;
    }

    ffileStream.close();
  } // if (ffile != 0x0)

  if (ffile == 0x0 && (iPad & 1) == 1) {
    delete[] pkts[0];
    delete[] pkts;

    delete[] pPkts[0];
    delete[] pPkts;

    return 120;
  }


  if (pfile == 0x0 && (iPad & 2) == 2) {
    delete[] pkts[0];
    delete[] pkts;

    delete[] fPkts[0];
    delete[] fPkts;

    return 120;
  }


  //
  // Open and write to output file
  //
  ofileStream.open( argv[optind+1], fstream::out | fstream::binary);

  if (pfile != 0x0 && (iPad & 1) == 0) {
    cout << "Writing " << nPad << " records from previous file" << endl;
    ofileStream.write( (char *) &pPkts[0][0], recSize*nPad);
  }

  cout << "Writing " << nRec << " records from original file" << endl;
  ofileStream.write( (char *) &pkts[0][0], recSize*nRec);

  if (ffile != 0x0 && (iPad & 2) == 0) {
    cout << "Writing " << nPad << " records from following file" << endl;
    ofileStream.write( (char *) &fPkts[0][0], recSize*nPad);
  }

  ofileStream.close();

  delete[] pkts[0];
  delete[] pkts;

  if (pfile != 0x0) {
    delete[] pPkts[0];
    delete[] pPkts;
  }

  if (ffile != 0x0) {
    delete[] fPkts[0];
    delete[] fPkts;
  }


  // Check
  uint8_t **oPkts;
  int32_t oRec = nRec;
  if (pfile != 0x0 && (iPad & 1) == 0) oRec += nPad; 
  if (ffile != 0x0 && (iPad & 2) == 0) oRec += nPad; 
  oPkts = new uint8_t*[oRec];
  oPkts[0] = new uint8_t[recSize*oRec];
  for (int i=1; i<oRec; i++) oPkts[i] = oPkts[i-1] + recSize; 

  ofileStream.open( argv[optind+1], fstream::in | fstream::binary);
  ofileStream.read( (char *) &oPkts[0][0], recSize*oRec);
  ofileStream.close();

  //Days since 1/1/1958
  uint16_t ui16;
  uint32_t ui32;
  memcpy( &ui16, (uint16_t *) &oPkts[0][toff], 2);
  int32_t jd0 = SWAP_2( ui16) + 2436205;

  jdate( jd0, &iyr, &iday);

  // Loop through packets
  ccsds_to_yds( &oPkts[0][toff], &iy, &idy, &sec);
  iy16 = iy;
  idy16 = idy;
  jd = jday( iy16, 1, idy16);
  double prevsec = sec + (jd - jd0)*86400;

  for (int i=0; i<oRec; i++) {
    ccsds_to_yds( &oPkts[i][toff], &iy, &idy, &sec);
    iy16 = iy;
    idy16 = idy;
    jd = jday( iy16, 1, idy16);
    //cout << i << " " << sec + (jd - jd0)*86400 << endl;
    if (fabs(sec + (jd - jd0)*86400 - prevsec) > 1.5) {
      cout << "Time gap in output file at record: " << i <<
        "  " << prevsec << "  " << sec + (jd - jd0)*86400 << endl;
      iret = 110;
    }
    prevsec = sec + (jd - jd0)*86400;
  }

  delete[] oPkts[0];
  delete[] oPkts;

  return iret;
}
