#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include "nc4utils.h"
#include "l1agen_viirs.h"

#define SWAP_2(x) ( (((x) & 0xff) << 8) | ((unsigned short)(x) >> 8) )

#define VERSION "1.11"

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
//  James Kuyper   GST            03/07/18 1.11 Changed to only write packets
//                                               that were successfully read,
//                                               and to fail due to I/O errors
//                                               or premature EOF.

using namespace std;

extern "C" int32_t jday( int16_t i, int16_t j, int16_t k);
extern "C" int jdate( int32_t julian, int32_t *year, int32_t *doy);
extern "C" int ccsds_to_yds( uint8_t *cctime, int32_t *iyear, int32_t *iday,
                             double *sec);

double get_pkt_sec( uint8_t *pkt_time, int32_t in_jd, int32_t *out_jd) {

  uint16_t ui16a;
  double sec;
  int16_t iy16, idy16;
  int32_t iy, idy, jd, jd0;

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
      "            [-m num-minutes]" << endl <<
      "          { [-p previous-file]" << endl <<
      "            [-f following-file] }" << endl <<
      "         file-to-pad" << endl <<
      "         [output-file]" << endl;
    return 0;
  }

  int c;
  //int index;
  int nPad = 10;
  int pPad = 0;
  int nRec = 0;
  int fPad = 0;
  char *pfile = NULL;
  char *ffile = NULL;
  int file_length = 60*6; // Nominal length in seconds of input file.

 // char *file_to_pad = NULL;
  while ((c = getopt (argc, argv, "n:m:p:f:")) != -1)
    switch (c)
      {
      case 'n':
        nPad = atoi( optarg);
        break;

      case 'm':
        file_length = 60*atoi(optarg);
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

  std::ifstream xfileStream( argv[optind], ifstream::binary);

  if (!xfileStream) {
    cout << argv[optind] << " not found" << endl;
    exit(1);
  }


  char fileType = '\0';
  char sat = '\0';
  int offset = 0;
  const std::string name(basename(argv[optind]));
  if (name[0] == 'P' and name[4] == '0')
  {	// Files ingested from EDOS
      offset = 5;
      string scid = name.substr(1, 3);
      if(scid == "157")
          sat = 'S';	// Suomi NPP
      else if(scid == "159")
          sat = 'N';	// NOAA-20
  }
  else if(name[0] == 'V' && (name[3] == 'S' || name[3] == 'T') )
  {	// Files created by splitter (PGE595)
      offset = 4;
      std::string scid = name.substr(1,2);
      if(scid == "NP")
	  sat = 'S';	// Suomi NPP
      else if(scid == "J1")
	  sat = 'N'; 	//
  }
  if(!sat)
  {
    std::cout << "File type cannot be determined from filename:" << name <<
        std::endl;
    exit(1);
  }

  std::string apid = name.substr(offset, 3);
  if (apid == "011")
      fileType = 'S';
  else if (apid == "008")
      fileType = 'A';
  else if (apid == "000")
      fileType = 'B';
  else
  {
      std::cout << "Invalid APID:" << apid << std::endl;
    exit(1);
  }

//  int32_t recSize, toff;
  int32_t recSize = 0;
  int32_t toff = 6;

  if (sat == 'S')
  {
    if (fileType == 'S') {
      recSize = 71;
    } else if (fileType == 'A') {
      recSize = 355;
    } else if (fileType == 'B') {
      recSize = 207;
    }
  }
  else if (sat == 'N')
  {
    if (fileType == 'S') {
      recSize = 71;
    } else if (fileType == 'A') {
      recSize = 393;
    } else if (fileType == 'B') {
      recSize = 183;
    }
  }
  
  uint8_t *packet = new uint8_t[recSize];	// Input packet
  // Read first record from file into buffer
  if(!xfileStream.read( (char *) packet, recSize).gcount())
  {
      if(xfileStream.bad())
      {
	    std::cout << "I/O error reading " << argv[optind] << std::endl;
	    exit(1);
      }
  }
  else if(xfileStream.gcount() < recSize)
  {
      {
	    std::cout << "Premature end of file " << argv[optind] << 
		 " at offset " << xfileStream.tellg() << std::endl;
	    exit(1);
      }
  }
 
//  uint16_t ui16a;
  double sec;
  int16_t iy16, idy16;
  int32_t iy, idy, jd, iyr, iday, jdtest;

  // Get start time of file to pad

  double secStart = get_pkt_sec(packet+toff, -1, &jdtest);
  // Round down to a multiple of file length.
  secStart = file_length*floor(long(secStart)/file_length);
  double secEnd = secStart + file_length;
  cout << "Start of file to pad: " << secStart << endl;
  cout << "Stop of file to pad:  " << secEnd << endl;

  //
  // Read previous file
  //
  int iret = 0;
  int iPad = 0;

  //
  // Open output file
  //
  std::ofstream ofileStream( argv[optind+1], ofstream::binary);
  // Read one packet, extract start time.
  
  uint8_t *padding = new uint8_t[recSize];	// Padding 
  if (pfile) {
    std::ifstream pfileStream( pfile, ifstream::binary);
    for(int i = 0; pfileStream.read( (char *) padding, recSize).gcount(); i++)
    {
      double sec0 = get_pkt_sec( padding+toff, jdtest, NULL);

      if (sec0 >= (secStart-nPad) && (sec0 < secStart)) 
      {
	ofileStream.write( (char *) padding, recSize);
	pPad++;
      }
    }
    if(pfileStream.bad())
    {
	std::cout << "I/O error reading " << pfile << std::endl;
      exit(1);
    }
    if(0 < pfileStream.gcount() && pfileStream.gcount() < recSize)
    {
	std::cout << "Premature end of file " << pfile << 
	     " at offset " << pfileStream.tellg() << std::endl;
	exit(1);
    }
    if (pPad)
    {
	cout << "Writing " << pPad << " records from " << pfile << endl;
        iPad = iPad | 1;
    }
    else
      cout << "No padding from " << pfile << " within " << nPad << " seconds"
           << endl;
      }

  while(ofileStream.write( (char *) packet, recSize))
  {
      nRec++;
      if(xfileStream.read( (char *) packet, recSize).gcount() == 0)
        break;
      }
  delete[] packet;

  if(xfileStream.bad())
  {
	std::cout << "I/O error reading " << argv[optind] << std::endl;
	exit(1);
    }
  if(0 < xfileStream.gcount() && xfileStream.gcount() < recSize)
  {
	std::cout << "Premature end of file " << argv[optind] << 
	     " at offset " << xfileStream.tellg() << std::endl;
	exit(1);
  }
  xfileStream.close();

  cout << "Writing " << nRec << " records from " << argv[optind] << endl;


  //
  // Read following file
  //
  if (ffile) {
    std::ifstream ffileStream( ffile, ifstream::binary);
    while (ffileStream.read( (char *) padding, recSize).gcount())
    {
      double sec0 = get_pkt_sec( padding+toff, jdtest, NULL);
      if (sec0 >= secEnd+nPad)
	break;
      if (sec0 > secEnd)
      {
	ofileStream.write( (char *) padding, recSize);
	fPad++;
      }
    }
    if(ffileStream.bad())
    {
	std::cout << "I/O error reading " << ffile << std::endl;
      exit(1);
    }
    if(0 < ffileStream.gcount() && ffileStream.gcount() < recSize)
    {
	std::cout << "Premature end of file " << ffile << 
	     " at offset " << ffileStream.tellg() << std::endl;
	exit(1);
    }

    if (fPad)
    {
      iPad = iPad | 2;
	cout << "Writing " << fPad << " records from " << ffile << endl;
    }
    else
      cout << "No padding from " << ffile << " within " << nPad << " seconds"
             << endl;
  } // if (ffile)

  delete[] padding;

  if (ffile == 0 && iPad & 1) {

    return 120;
  }


  if (pfile == 0 && iPad & 2) {

    return 120;
  }

  if(ofileStream.bad())
  {
      std::cout << "I/O error reading " << argv[optind+1] << std::endl;
      exit(1);
  }
  ofileStream.close();

  xfileStream.open( argv[optind+1], ofstream::binary);
  xfileStream.seekg(0, ios_base::beg);

  // Check
  uint8_t **oPkts;
  int32_t oRec = pPad + nRec + fPad;
  oPkts = new uint8_t*[oRec];
  oPkts[0] = new uint8_t[recSize*oRec];
  for (int i=1; i<oRec; i++) oPkts[i] = oPkts[i-1] + recSize; 

  xfileStream.read( (char *) &oPkts[0][0], recSize*oRec);
  xfileStream.close();

  //Days since 1/1/1958
  uint16_t ui16;
 //uint32_t ui32;
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
