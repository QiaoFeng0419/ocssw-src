#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>

#include <sstream>
#include <iomanip> 
#include "l1agen_viirs.h"
#include "nc4utils.h"
#include "netcdf.h"
#include "passthebuck.h"
#include <timeutils.h>


l1aFile::l1aFile() {
  ncid = -1;
  SC_records_val = 0;
}


l1aFile::~l1aFile() {
  
}

 /*----------------------------------------------------------------- */
 /* Create an Generic NETCDF4 level1 file                            */
 /* ---------------------------------------------------------------- */
int l1aFile::createl1( char* l1_filename, int32_t sscan, int32_t escan,
                       int32_t spixl, int32_t epixl, int32_t iopt_extract) {

   int status;
   status = nc_create( l1_filename, NC_NETCDF4, &ncid);
   check_err(status,__LINE__,__FILE__);

   ifstream viirs_l1a_data_structure;
   string line;
   string dataStructureFile;

   if ( this->platform == "JPSS-1")
     dataStructureFile.assign("$OCDATAROOT/viirs/j1/VIIRS_Level-1A_Data_Structure_J1.cdl");
   else
     dataStructureFile.assign("$OCDATAROOT/viirs/npp/VIIRS_Level-1A_Data_Structure.cdl");
   expandEnvVar( &dataStructureFile);

   viirs_l1a_data_structure.open( dataStructureFile.c_str(), ifstream::in);
   if ( viirs_l1a_data_structure.fail() == true) {
     cout << "\"" << dataStructureFile.c_str() << "\" not found" << endl;
     exit(1);
   }

   // Find "dimensions" section of CDL file
   while(1) {
     getline( viirs_l1a_data_structure, line);
     size_t pos = line.find("dimensions:");
     if ( pos == 0) break;
   }

   // Define dimensions from "dimensions" section of CDL file
   ndims = 0;
   int32_t numScans = escan - sscan + 1;
   while(1) {
     getline( viirs_l1a_data_structure, line);
     size_t pos = line.find(" = ");
     if ( pos == string::npos) break;

     uint32_t dimSize;
     istringstream iss(line.substr(pos+2, string::npos));
     iss >> dimSize;

     iss.clear(); 
     iss.str( line);
     iss >> skipws >> line;

     //     cout << "Dimension Name: " << line.c_str() << " Dimension Size: "
     //	  << dimSize << endl;

     if (line.compare("number_of_scans") == 0 && numScans > 0) {
       dimSize = numScans;
     }

     // Add 0.5 to correct rounding error JMG 12/07/16
     if (line.compare("SC_records") == 0 && numScans > 0) {
       dimSize = (int32_t) (numScans*1.78 + 0.5) + 20;
       //       cout << "SC_records_val: " << dimSize << endl;
       SC_records_val = dimSize;
     }

     //            0 - 639                     pixel_offset
     //          640 - 1007                  640 + 2*(pixel_offset - 640)
     //         1008 - 2191                 1376 + 3*(pixel_offset - 1008)
     //         2192 - 2559                 4928 + 2*(pixel_offset - 2192)
     //         2560 - 3199                 5664 + (pixel_offset - 2560)

     if ( spixl != 0 || epixl != 3199) {
       if (line.compare("Mband_pixels") == 0) {
         dimSize = epixl - spixl + 1;
       } else if (line.compare("Iband_pixels") == 0) {
         dimSize = 2*(epixl - spixl + 1);
       } else if (line.compare("Mband_samples") == 0) {
         if ( spixl <= 639) {
           dimSize = spixl;
         } else if ( spixl > 639 && spixl <= 1007) {
           dimSize = 640 + 2*(spixl - 640);
         } else if ( spixl > 1007 && spixl <= 2191) {
           dimSize = 1376 + 3*(spixl - 1008);
         } else if ( spixl > 2191 && spixl <= 2559) {
           dimSize = 4928 + 2*(spixl - 2192);
         } else if ( spixl > 2559) {
           dimSize = 5664 + (spixl - 2560);
         }

         int32_t epixl0;
         if ( epixl <= 639) {
           epixl0 = epixl;
         } else if ( epixl > 639 && epixl <= 1007) {
           epixl0 = 641 + 2*(epixl - 640);
         } else if ( epixl > 1007 && epixl <= 2191) {
           epixl0 = 1378 + 3*(epixl - 1008);
         } else if ( epixl > 2191 && epixl <= 2559) {
           epixl0 = 4929 + 2*(epixl - 2192);
         } else if ( epixl > 2559) {
           epixl0 = 5664 + (epixl - 2560);
         }
         dimSize = epixl0 - dimSize + 1;
       }
     }

     status = nc_def_dim( ncid, line.c_str(), dimSize, &dimid[ndims++]);
     check_err(status,__LINE__,__FILE__);
   } // while loop

   // Read global attributes (string attributes only) 
   while(1) {
     getline( viirs_l1a_data_structure, line);
     size_t pos = line.find("// global attributes");
     if ( pos == 0) break;
   }

   while(1) {
     getline( viirs_l1a_data_structure, line);
     size_t pos = line.find(" = ");
     if ( pos == string::npos) break;

     string attValue;

     // Remove leading and trailing quotes
     attValue.assign(line.substr(pos+4));
     size_t posQuote = attValue.find("\"");
     attValue.assign(attValue.substr(0, posQuote));

     istringstream iss(line.substr(pos+2));
     iss.clear(); 
     iss.str( line);
     iss >> skipws >> line;

     // Skip commented out attributes
     if (line.compare("//") == 0) continue;

     string attName;
     attName.assign(line.substr(1).c_str());

     // Skip non-string attributes
     if (attName.compare("orbit_number") == 0) continue;
     if (attName.compare("history") == 0) continue;
     if (attName.compare("format_version") == 0) continue;
     if (attName.compare("instrument_number") == 0) continue;
     if (attName.compare("pixel_offset") == 0) continue;
     if (attName.compare("number_of_filled_scans") == 0) continue;

     // cout << attName.c_str() << " " << attValue.c_str() << endl;

     status = nc_put_att_text(ncid, NC_GLOBAL, attName.c_str(), 
                              strlen(attValue.c_str()), attValue.c_str());
     check_err(status,__LINE__,__FILE__);

   }

   ngrps = 0;
   // Loop through groups
   while(1) {
     getline( viirs_l1a_data_structure, line);

     // Check if end of CDL file
     // If so then close CDL file and return
     if (line.substr(0,1).compare("}") == 0) {
       viirs_l1a_data_structure.close();
       return 0;
     }

     // Check for beginning of new group
     size_t pos = line.find("group:");

     // If found then create new group and variables
     if ( pos == 0) {

       // Parse group name
       istringstream iss(line.substr(6, string::npos));
       iss >> skipws >> line;

       // Create NCDF4 group
       status = nc_def_grp( ncid, line.c_str(), &this->gid[ngrps]);
       check_err(status,__LINE__,__FILE__);

       ngrps++;

       int numDims=0;
       int varDims[NC_MAX_DIMS];
       size_t dimSize[NC_MAX_DIMS];
       char dimName[NC_MAX_NAME+1];
       string sname;
       string lname;
       string standard_name;
       string units;
       string flag_values;
       string flag_meanings;
       double valid_min=0.0;
       double valid_max=0.0;
       double fill_value=0.0;

       int ntype=0;

       // Loop through datasets in group
       getline( viirs_l1a_data_structure, line); // skip "variables:"
       while(1) {
         getline( viirs_l1a_data_structure, line);

         if (line.length() == 0) continue;
         if (line.substr(0,1).compare("\r") == 0) continue;
         if (line.substr(0,1).compare("\n") == 0) continue;

         size_t found = line.find("DNB");
         if (found != string::npos && iopt_extract == 1) {
           continue;
         }

         size_t pos = line.find(":");

         // No ":" found, new dataset or empty line or end-of-group
         if ( pos == string::npos) {

           if ( numDims > 0) {
             // Create previous dataset
             createNCDF( gid[ngrps-1], 
                         sname.c_str(), lname.c_str(),
                         standard_name.c_str(), units.c_str(),
                         (void *) &fill_value, 
                         flag_values.c_str(), flag_meanings.c_str(),
                         valid_min, valid_max, ntype, numDims, varDims);

             flag_values.assign("");
             flag_meanings.assign("");
             units.assign("");
           }

           valid_min=0.0;
           valid_max=0.0;
           fill_value=0.0;

           if (line.substr(0,10).compare("} // group") == 0) break;

           // Parse variable type
           string varType;
           istringstream iss(line);
           iss >> skipws >> varType;

           // Get corresponding NC variable type
           if ( varType.compare("char") == 0) ntype = NC_CHAR;
           else if ( varType.compare("byte") == 0) ntype = NC_BYTE;
           else if ( varType.compare("short") == 0) ntype = NC_SHORT;
           else if ( varType.compare("int") == 0) ntype = NC_INT;
           else if ( varType.compare("long") == 0) ntype = NC_INT;
           else if ( varType.compare("float") == 0) ntype = NC_FLOAT;
           else if ( varType.compare("real") == 0) ntype = NC_FLOAT;
           else if ( varType.compare("double") == 0) ntype = NC_DOUBLE;
           else if ( varType.compare("ubyte") == 0) ntype = NC_UBYTE;
           else if ( varType.compare("ushort") == 0) ntype = NC_USHORT;
           else if ( varType.compare("uint") == 0) ntype = NC_UINT;
           else if ( varType.compare("int64") == 0) ntype = NC_INT64;
           else if ( varType.compare("uint64") == 0) ntype = NC_UINT64;

           // Parse short name (sname)
           pos = line.find("(");
           size_t posSname = line.substr(0, pos).rfind(" ");
           sname.assign(line.substr(posSname+1, pos-posSname-1));
           //           cout << "sname: " << sname.c_str() << endl;

           // Parse variable dimension info
           this->parseDims( line.substr(pos+1, string::npos), 
                            &numDims, varDims);
           for (int i=0; i<numDims; i++) { 
             nc_inq_dim( ncid, varDims[i], dimName, &dimSize[i]);
             //cout << line.c_str() << " " << i << " " << dimName
             //	  << " " << dimSize[i] << endl;
           }

         } else {
           // Parse variable attributes
           size_t posEql = line.find("=");
           size_t pos1qte = line.find("\"");
           size_t pos2qte = line.substr(pos1qte+1, string::npos).find("\"");
	   // cout << line.substr(pos+1, posEql-pos-2).c_str() << endl;

           string attrName = line.substr(pos+1, posEql-pos-2);

           // Get long_name
           if ( attrName.compare("long_name") == 0) {
             lname.assign(line.substr(pos1qte+1, pos2qte));
             //             cout << "lname: " << lname.c_str() << endl;
           }

           // Get units
           else if ( attrName.compare("units") == 0) {
             units.assign(line.substr(pos1qte+1, pos2qte));
             //             cout << "units: " << units.c_str() << endl;
           }

           // Get _FillValue
           else if ( attrName.compare("_FillValue") == 0) {
             iss.clear(); 
             iss.str( line.substr(posEql+1, string::npos));
             iss >> fill_value;
             //             cout << "_FillValue: " << fill_value << endl;
           }

           // Get flag_values
           else if ( attrName.compare("flag_values") == 0) {
             flag_values.assign(line.substr(pos1qte+1, pos2qte));
           }

           // Get flag_meanings
           else if ( attrName.compare("flag_meanings") == 0) {
             flag_meanings.assign(line.substr(pos1qte+1, pos2qte));
           }

           // Get valid_min
           else if ( attrName.compare("valid_min") == 0) {
             iss.clear(); 
             iss.str( line.substr(posEql+1, string::npos));
             iss >> valid_min;
             //             cout << "valid_min: " << valid_min << endl;
           }

           // Get valid_max
           else if ( attrName.compare("valid_max") == 0) {
             iss.clear(); 
             iss.str( line.substr(posEql+1, string::npos));
             iss >> valid_max;
             //             cout << "valid_max: " << valid_max << endl;
           }

         } // if ( pos == string::npos)
       } // datasets in group loop
     } // New Group loop
   } // Main Group loop
   
   
   return 0;
}


int l1aFile::createl1( char* l1_filename, int32_t numScans) {
  this -> createl1( l1_filename, (int32_t) 0, numScans-1, 
                    (int32_t) 0, (int32_t) 3199, (int32_t) 0);

  return 0;
}

int l1aFile::parseDims( string dimString, int *numDims, int *varDims) {

  size_t dimSize, curPos=0;
  char dimName[NC_MAX_NAME+1];

  *numDims = 0;

  while(1) {
    size_t pos = dimString.find(",", curPos);
    if ( pos == string::npos) 
      pos = dimString.find(")");

    string varDimName;
    istringstream iss(dimString.substr(curPos, pos-curPos));
    iss >> skipws >> varDimName;

    for (int i=0; i<ndims; i++) {
      int status = nc_inq_dim( ncid, dimid[i], dimName, &dimSize);
      if (status != NC_NOERR) {
         printf("-E - failed to retrieve dim: %s\n",dimName);
      }
      if ( varDimName.compare(dimName) == 0) {
        varDims[(*numDims)++] = dimid[i];
        break;
      }
    }
    if ( dimString.substr(pos, 1).compare(")") == 0) break;

    curPos = pos + 1;
  }

  return 0;
}


int l1aFile::write_science_data( string platform, int32_t isc,
                                 uint16_t (*mbands)[16][6304],
                                 uint16_t (*ibands)[32][6400],
                                 uint16_t (*dnb)[16][4064],
                                 uint8_t (*mqfl)[16],
                                 uint8_t (*iqfl)[32],
                                 uint8_t (*dqfl)[16]) {

  int groupid;
  int status = nc_inq_grp_ncid( ncid, "earth_view_data", &groupid);
  check_err(status,__LINE__,__FILE__);

  size_t start[3]={(size_t)isc, 0, 0};
  size_t count[3]={1, 0, 0};

  // Write M-band data
  for ( int i=0; i<=15; i++) {
    count[2] = 3200;
    if (i < 5 or i == 6 or i == 12) count[2] = 6304;

    // Check if any missing detectors
    uint8_t m[16];
    for ( int j=0; j<16; j++) {
      if ( mqfl[i][j] >= 64) {
        m[j] = 1;
      } else {
        m[j] = 0;
      }
    }

    stringstream varstr;
    varstr << "EV_M" << setfill('0') << setw(2) << i+1;

    int varid;
    status = nc_inq_varid( groupid, varstr.str().c_str(), &varid);
    check_err(status,__LINE__,__FILE__);

    count[1] = 1;
    for ( int j=0; j<16; j++) {
      if ( m[j] == 0) {
        start[1] = j;
        status = nc_put_vara_short( groupid, varid, start, count, 
                                    (const short int*) &mbands[i][j][0]);
        check_err(status,__LINE__,__FILE__);
      }
    }
  }


  // Write I-band data
  count[2] = 6400;
  for ( int i=0; i<=4; i++) {

    // Check if any missing detectors
    uint8_t m[32];
    uint8_t missing = 0;
    for ( int j=0; j<32; j++) {
      if ( iqfl[i][j] >= 64) {
        m[j] = 1;
        missing = 1;
      } else {
        m[j] = 0;
      }
    }

    stringstream varstr;
    varstr << "EV_I" << setfill('0') << setw(2) << i+1;
    //    cout << varstr.str() << endl;

    int varid, status;
    status = nc_inq_varid( groupid, varstr.str().c_str(), &varid);
    check_err(status,__LINE__,__FILE__);

    if ( missing == 0) {
      count[1] = 32;
      start[1] = 0;
      status = nc_put_vara_short( groupid, varid, start, count, 
                                  (const short int*) &ibands[i][0][0]);
      check_err(status,__LINE__,__FILE__);

    } else {
      count[1] = 1;
      for ( int j=0; j<16; j++) {
        if ( m[j] == 0) {
          start[1] = j;
          status = nc_put_vara_short( groupid, varid, start, count, 
                                      (const short int*) &ibands[i][j][0]);
          check_err(status,__LINE__,__FILE__);
        }
      }
    }
  }


  // Write DNB
  int nDNB=0;
  if ( platform.compare("SNPP") == 0)
    nDNB = 3;
  else  if ( platform.compare("JPSS-1") == 0)
    nDNB = 5;
  
  for ( int i=0; i<nDNB; i++) {
    count[2] = 4064;

    // Check if any missing detectors
    uint8_t m[16];
    uint8_t missing = 0;
    for ( int j=0; j<16; j++) {
      if ( dqfl[i][j] >= 64) {
        m[j] = 1;
        missing = 1;
      } else {
        m[j] = 0;
      }
    }

    stringstream varstr;
    string dnb_tag[5]={"HGS", "MGS", "LGS", "HGA", "HGB"};
    varstr << "EV_DNB_" << dnb_tag[i];
    //cout << varstr.str() << endl;

    int varid, status;
    status = nc_inq_varid( groupid, varstr.str().c_str(), &varid);
    check_err(status,__LINE__,__FILE__);

    if ( missing == 0) {
      count[1] = 16;
      start[1] = 0;
      status = nc_put_vara_short( groupid, varid, start, count, 
                                  (const short int*) &dnb[i][0][0]);
      check_err(status,__LINE__,__FILE__);

    } else {
      count[1] = 1;
      for ( int j=0; j<16; j++) {
        if ( m[j] == 0) {
          start[1] = j;
          status = nc_put_vara_short( groupid, varid, start, count, 
                                      (const short int*) &dnb[i][j][0]);
          check_err(status,__LINE__,__FILE__);
        }
      }
    }
  } // nDNB loop

  return 0;
}


int l1aFile::write_scan_metadata(int32_t isc, 
                                 uint8_t (*p1)[180],
                                 uint8_t (*hrmets)[146*26],
                                 uint8_t (*calmets)[134],
                                 uint8_t *mode, int iret,
                                 const char* l1a_name,
                                 char* VIIRS_packet_file) {

  cout << "Writing scan-level metadata" << endl;

  static char initial_L0_time[32] = "9999";
  static char final_L0_time[32] = "0000";
  static int32_t total_isc = 0;
  static int total_iret = 0;

  total_isc += isc;
  total_iret |= iret;

  char buf[1000];
  strcpy( buf, (char *) l1a_name);
  ofstream fout;
  char *last_dot = strrchr( buf, '.');
  if (!last_dot) {
    strcat(buf, ".txt");
  } else {
    strcpy( last_dot, ".txt");
    *last_dot = 0;
  }
  fout.open( buf);
  strcat( buf, ".nc");
  fout << "basename=" << basename(buf) << endl;

  int groupid;
  int status = nc_inq_grp_ncid( ncid, "scan_line_attributes", &groupid);
  check_err(status,__LINE__,__FILE__);

  // Extract and convert times
  double *scs = new double[isc];
  double *sce = new double[isc];
  int16_t *scsd = new int16_t[isc];
  int32_t *scsm = new int32_t[isc];
  int16_t *scsu = new int16_t[isc];
  int16_t *sced = new int16_t[isc];
  int32_t *scem = new int32_t[isc];
  int16_t *sceu = new int16_t[isc];

  uint16_t ui16;
  uint32_t ui32;

  for ( int i=0; i<isc; i++) {
    // Convert scan start and end times to TAI93

    int32_t toff, syear, sday, eyear, eday, iyear, iday;
    double stime, ptime, etime;
  
    // Scan start time
    toff = 6;
    ccsds_to_yds( &p1[i][toff], &syear , &sday, &stime);

    // Packet time
    toff = 20;
    ccsds_to_yds( &p1[i][toff], &iyear, &iday, &ptime);

    // Scan end time
    toff = 38;
    ccsds_to_yds( &p1[i][toff], &eyear, &eday, &etime);

    scs[i] = yds2tai93( syear, sday, stime);
    sce[i] = yds2tai93( eyear, eday, etime);

    char datebuf[32];
    if ( i == 0) {
      double dsec = (double) stime;
      double myUnixTime= yds2unix(syear, sday, dsec);
      strcpy( datebuf, unix2isodate(myUnixTime, 'G'));
      //      cout << datebuf << endl;
      fout << "start_time=" << datebuf << endl;
      if ( strcmp( datebuf, initial_L0_time) < 0)
        strcpy( initial_L0_time, datebuf);
    }

    if ( i == (isc-1)) {
      // Change from etime 08/19/15 JMG
      // Change eyear,eday to syear,sday 08/24/15 JMG
      double dsec = (double) stime;
      double myUnixTime= yds2unix(syear, sday, dsec);
      strcpy( datebuf, unix2isodate(myUnixTime, 'G'));
      //      cout << datebuf << endl;
      fout << "stop_time=" << datebuf << endl;
      if ( strcmp( datebuf, final_L0_time) > 0)
        strcpy( final_L0_time, datebuf);
    }

    memcpy( &ui16, (uint16_t *) &p1[i][6], 2);
    scsd[i] = SWAP_2( ui16);

    memcpy( &ui32, (uint32_t *) &p1[i][8], 4);
    scsm[i] = SWAP_4( ui32);

    memcpy( &ui16, (uint16_t *) &p1[i][12], 2);
    scsu[i] = SWAP_2( ui16);

    memcpy( &ui16, (uint16_t *) &p1[i][38], 2);
    sced[i] = SWAP_2( ui16);

    memcpy( &ui32, (uint32_t *) &p1[i][40], 4);
    scem[i] = SWAP_4( ui32);

    memcpy( &ui16, (uint16_t *) &p1[i][44], 2);
    sceu[i] = SWAP_2( ui16);
  }

  int varid;
  size_t start[3]={0, 0, 0};
  size_t count[3];
  count[0] = (size_t) isc;
  count[1] = this->EV_APIDs;
  count[2] = 146;

  // Scan start time (TAI93) 
  status = nc_inq_varid( groupid, "scan_start_time", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, (double *) scs);
  check_err(status,__LINE__,__FILE__);

  // Scan end time (TAI93) 
  status = nc_inq_varid( groupid, "scan_end_time", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, (double *) sce);
  check_err(status,__LINE__,__FILE__);


  // Scan start time (CCSDS)
  // Day
  status = nc_inq_varid( groupid, "scan_start_CCSDS_day", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, (int16_t *) scsd);
  check_err(status,__LINE__,__FILE__);

  // Milliseconds
  status = nc_inq_varid( groupid, "scan_start_CCSDS_msec", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, (int32_t *) scsm);
  check_err(status,__LINE__,__FILE__);

  status = nc_inq_varid( groupid, "scan_start_CCSDS_usec", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, (int16_t *) scsu);
  check_err(status,__LINE__,__FILE__);


  // Scan end time (CCSDS)
  // Day
  status = nc_inq_varid( groupid, "scan_end_CCSDS_day", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, (int16_t *) sced);
  check_err(status,__LINE__,__FILE__);

  // Milliseconds
  status = nc_inq_varid( groupid, "scan_end_CCSDS_msec", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, (int32_t *) scem);
  check_err(status,__LINE__,__FILE__);

  status = nc_inq_varid( groupid, "scan_end_CCSDS_usec", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, (int16_t *) sceu);
  check_err(status,__LINE__,__FILE__);


  // Extract and write HAM side, instrument scan number and sensor mode
  uint8_t *ham = new uint8_t[isc];
  for ( int i=0; i<isc; i++) ham[i] = (p1[i][32] & 128) / 128;

  for ( int i=0; i<isc; i++) mode[i] = p1[i][46];

  uint32_t *scnum = new uint32_t[isc];
  for ( int i=0; i<isc; i++) {
    memcpy( &ui32, (uint32_t *) &p1[i][34], 4);
    scnum[i] = SWAP_4( ui32);
  }

  status = nc_inq_varid( groupid, "HAM_side", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, (uint8_t *) ham);
  check_err(status,__LINE__,__FILE__);

  status = nc_inq_varid( groupid, "VIIRS_scan_number", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, (int32_t *) scnum);
  check_err(status,__LINE__,__FILE__);

  status = nc_inq_varid( groupid, "sensor_mode", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, (uint8_t *) mode);
  check_err(status,__LINE__,__FILE__);

  // Write HR metadata and cal metadata
  status = nc_inq_varid( groupid, "HR_metadata", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, 
                        (uint8_t *) &hrmets[0][0]);
  check_err(status,__LINE__,__FILE__);

  count[1] = 134;
  status = nc_inq_varid( groupid, "cal_metadata", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, 
                        (uint8_t *) &calmets[0][0]);
  check_err(status,__LINE__,__FILE__);


  // Write DNB_sequence
  status = nc_inq_grp_ncid( ncid, "onboard_calibration_data", &groupid);
  check_err(status,__LINE__,__FILE__);

  uint8_t *dnb_seq = (uint8_t *) calloc( isc, sizeof(uint8_t));
  for ( int i=0; i<isc; i++) {
    dnb_seq[i] = (calmets[i][0] % 4)*16 + calmets[i][1]/16;
  }

  status = nc_inq_varid( groupid, "DNB_sequence", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_vara( groupid, varid, start, count, 
                        (uint8_t *) dnb_seq);
  check_err(status,__LINE__,__FILE__);

  free( dnb_seq);

  delete[] scs;
  delete[] sce;
  delete[] scsd;
  delete[] scsm;
  delete[] scsu;
  delete[] sced;
  delete[] scem;
  delete[] sceu;

  delete[] ham;
  delete[] scnum;

  fout << "number_of_records=" << isc << endl;
  fout << "quality_flag=" << iret << endl;
  fout.close();

  strcpy( buf, (char *) VIIRS_packet_file);
  last_dot = strrchr( buf, '.');
  if (!last_dot) {
    strcat(buf, ".txt");
  } else {
    strcpy( last_dot, ".txt");
    *last_dot = 0;
  }
  fout.open( buf);
  strcat( buf, ".PDS");
  fout << "basename=" << basename(buf) << endl;
  fout << "start_time=" << initial_L0_time << endl;
  fout << "stop_time=" << final_L0_time << endl;
  fout << "number_of_records=" << total_isc << endl;
  fout << "quality_flag=" << total_iret << endl;
  fout.close();

  return 0;
}


int l1aFile::write_eng_data( int32_t isc, uint8_t (*engdata)[9318]) {

  cout << "Writing engineering data" << endl;

  int groupid;
  int status = nc_inq_grp_ncid( ncid, "engineering_data", &groupid);
  check_err(status,__LINE__,__FILE__);

  string engdataName[] = {"engineering_status",
                          "DPP_config",
                          "ASP_config",
                          "DNB_config",
                          "ASP_offsets",
                          "ASP_analog",
                          "analog_temperature",
                          "SDSM_data",
                          "eng_reserved"};

  int engdataOffset[] = {52, 60, 188, 316, 444, 3516, 3644, 3772, 9188};

  int varid;

  int dimids[2];
  size_t start[2]={0, 0};
  size_t count[2]={1, 0};

  // Check for "empty" enginnering buffers
  bool *empty = new bool[isc];
  for ( int j=0; j<isc; j++) {
    uint8_t non_zero = 0;
    for ( int i=0; i<9318; i++) {
      if ( engdata[j][i] != 0) {
        non_zero = 1;
        break;
      }
    } // i-loop
    if ( non_zero == 1)
      empty[j] = false;
    else
      empty[j] = true;
  } // j-loop
  
  for ( int i=0; i<9; i++) {

    status = nc_inq_varid( groupid, engdataName[i].c_str(), &varid);
    check_err(status,__LINE__,__FILE__);

    status = nc_inq_var( groupid, varid, NULL, NULL, NULL, dimids, NULL);
    check_err(status,__LINE__,__FILE__);

    status = nc_inq_dimlen( groupid, dimids[1], &count[1]);
    check_err(status,__LINE__,__FILE__);

    for ( int j=0; j<isc; j++) {
      start[0] = j;
      status = nc_put_vara( groupid, varid, start, count, 
                            (uint8_t *) &engdata[j][engdataOffset[i]]);
      check_err(status,__LINE__,__FILE__);
    }
  }

  uint16_t encoder_data[1290];
  count[1] = 1290;

  string encoderName[] = {"tel_encoder", "HAM_encoder"};
  int encoderOffset[] = {4028, 4028+2580};

  for ( int i=0; i<2; i++) {
    status = nc_inq_varid( groupid, encoderName[i].c_str(), &varid);
    check_err(status,__LINE__,__FILE__);

    for ( int j=0; j<isc; j++) {
      start[0] = j;

      for ( size_t k=0; k<count[1]; k++) {
        encoder_data[k] = 
          engdata[j][encoderOffset[i]+2*k]*256 + 
          engdata[j][encoderOffset[i]+2*k+1];

        // if empty buffer then overwrite with fill value (65535)
        if ( empty[j]) encoder_data[k] = USHRT_MAX;
      }

      status = nc_put_vara( groupid, varid, start, count, 
                            (uint16_t *) encoder_data);
      check_err(status,__LINE__,__FILE__);
    }
  }

  int16_t *i16_buffer = (int16_t *) calloc( isc, sizeof(int16_t));

  string start_deltaName[] = {"tel_start_enc", 
                              "HAM_start_enc", 
                              "scan_encoder_delta"};

  int start_deltaOffset[] = {9188, 9188+2, 60+11};

  size_t zero=0;
  size_t isc_size_t = isc;

  for ( int i=0; i<3; i++) {
    status = nc_inq_varid( groupid, start_deltaName[i].c_str(), &varid);
    check_err(status,__LINE__,__FILE__);

    for ( int j=0; j<isc; j++) {
      i16_buffer[j] =
        engdata[j][start_deltaOffset[i]]*256 + 
        engdata[j][start_deltaOffset[i]+1];

      // Extend 14-bit 2's complement sign bit if set
      // for "scan_encoder_delta" 
      if (i == 2)
        if (i16_buffer[j] >= 8192) i16_buffer[j] -= 16384;

      // if empty buffer then overwrite with fill value (-999)
      if ( empty[j]) {
        if ( strcmp( start_deltaName[i].c_str(), "scan_encoder_delta") == 0)
          i16_buffer[j] = -32768;
        else
          i16_buffer[j] = -999;
      }

      status = nc_put_vara( groupid, varid, &zero, &isc_size_t, 
                          (int16_t *) i16_buffer);
      check_err(status,__LINE__,__FILE__);
    }
  }
  
  free ( i16_buffer);


  uint8_t *ui8_buffer = (uint8_t *) calloc( isc, sizeof(uint8_t));

  string seName[] = {"se_a_teleHAM_scansyn",
                     "se_b_teleHAM_scansyn",
                     "se_a_anlg_pwr_on",
                     "se_b_anlg_pwr_on",
                     "servo_in_use",
                     "se_a_tele_pos_known",
                     "se_b_tele_pos_known",
                     "se_a_mtrs_stopped",
                     "se_b_mtrs_stopped"};

  int seOffset[] = {52+1, 52+1, 60+9, 60+9, 60+8, 60+9, 60+9, 60+9, 60+9};

  uint8_t andFac[] = {2, 1, 128, 8, 16, 16, 1, 32, 2};

  for ( int i=0; i<9; i++) {
    status = nc_inq_varid( groupid, seName[i].c_str(), &varid);
    check_err(status,__LINE__,__FILE__);

    for ( int j=0; j<isc; j++) {
      ui8_buffer[j] = (engdata[j][seOffset[i]] & andFac[i]) / andFac[i];

      // if empty buffer then overwrite with fill value (255)
      if ( empty[j]) ui8_buffer[j] = 255;
    }

    status = nc_put_vara( groupid, varid, &zero, &isc_size_t, 
                          (uint8_t *) ui8_buffer);
    check_err(status,__LINE__,__FILE__);
  }

  free ( ui8_buffer);

  delete[] empty;
  
  return 0;
}

 int l1aFile::write_cal_data( int32_t isc,
                              int16_t (*sd_m)[48*16],
                              int16_t (*sv_m)[48*16],
                              int16_t (*bb_m)[48*16],
                              int16_t (*sd_i)[96*32],
                              int16_t (*sv_i)[96*32],
                              int16_t (*bb_i)[96*32],
                              int16_t (*sd_d)[64*16],
                              int16_t (*sv_d)[64*16],
                              int16_t (*bb_d)[64*16]) {

   cout << "Writing calibration data" << endl;

  int groupid;
  int status = nc_inq_grp_ncid( ncid, "onboard_calibration_data", &groupid);
  check_err(status,__LINE__,__FILE__);

  int varid;

   // M-band data

   // Solar diffuser
  status = nc_inq_varid( groupid, "SD_M", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_var( groupid, varid, (int16_t *) sd_m);
  check_err(status,__LINE__,__FILE__);

  // Space view
  status = nc_inq_varid( groupid, "SV_M", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_var( groupid, varid, (int16_t *) sv_m);
  check_err(status,__LINE__,__FILE__);

  // Blackbody
  status = nc_inq_varid( groupid, "BB_M", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_var( groupid, varid, (int16_t *) bb_m);
  check_err(status,__LINE__,__FILE__);


   // I-band data

   // Solar diffuser
  status = nc_inq_varid( groupid, "SD_I", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_var( groupid, varid, (int16_t *) sd_i);
  check_err(status,__LINE__,__FILE__);

  // Space view
  status = nc_inq_varid( groupid, "SV_I", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_var( groupid, varid, (int16_t *) sv_i);
  check_err(status,__LINE__,__FILE__);

  // Blackbody
  status = nc_inq_varid( groupid, "BB_I", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_var( groupid, varid, (int16_t *) bb_i);
  check_err(status,__LINE__,__FILE__);


   // DNB data

   // Solar diffuser
  status = nc_inq_varid( groupid, "SD_DNB", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_var( groupid, varid, (int16_t *) sd_d);
  check_err(status,__LINE__,__FILE__);

  // Space view
  status = nc_inq_varid( groupid, "SV_DNB", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_var( groupid, varid, (int16_t *) sv_d);
  check_err(status,__LINE__,__FILE__);

  // Blackbody
  status = nc_inq_varid( groupid, "BB_DNB", &varid);
  check_err(status,__LINE__,__FILE__);
  status = nc_put_var( groupid, varid, (int16_t *) bb_d);
  check_err(status,__LINE__,__FILE__);

  return 0;
}


int l1aFile::write_diary( int32_t iyear, int32_t iday, 
                          int32_t ltime, int32_t mtime, 
                          int32_t iyrsc, int32_t idysc, int32_t nscd,
                          double *otime, float (*orb)[6], 
                          double *atime, float (*quat)[4],
                          char *sdir, char *edir) {

  int groupid;
  int status = nc_inq_grp_ncid( ncid, "navigation_data", &groupid);
  check_err(status,__LINE__,__FILE__);

  int varid;
  size_t start[2]={0, 0};
  size_t count[2]={1, 0};
  size_t cnt;

  double tai;
  int16_t iyr16 = (int16_t) iyear;
  int16_t idy16 = (int16_t) iday;
  int16_t iyrsc16 = (int16_t) iyrsc;
  int16_t idysc16 = (int16_t) idysc;
  int32_t jdg = jday( iyr16, 1, idy16);
  int32_t jds = jday( iyrsc16, 1, idysc16);

  uint8_t *k = new uint8_t[nscd];
  int32_t nk=0;
  int32_t first=-1, last=-1;
  for ( int i=0; i<nscd; i++) {
    double otm = otime[i] + (jds - jdg)*86400;

    if ( otm > (ltime-10) && otm < (mtime+10)) {
      k[i] = 1;
      nk++;
      if ( first == -1) first = i;
      last = i;
    } else {
      k[i] = 0;
    }
  }
  cout << nk << " orbit vectors in time range" << endl;

  // Write orbit times
  if (k[0] != -1) {
    cout << "Writing orbit data" << endl;
    status = nc_inq_varid( groupid, "orb_time", &varid);
    check_err(status,__LINE__,__FILE__);

    cnt = 0;
    for ( int i=0; i<nscd; i++) {
      if ( k[i] == 1) {
	start[0] = cnt;
      
	tai = yds2tai93( iyrsc16, idysc16, otime[i]);
	
	// Write TAI93 time
	status = nc_put_vara_double( groupid, varid, start, count, &tai);
	if (status != NC_NOERR) {
          check_err(status,__LINE__,__FILE__);
	  //report_err(status,__LINE__,__FILE__);
	  //return 1;
	}
	//      check_err(status,__LINE__,__FILE__);

	cnt++;
      }
    }

    // Write orbit position and velocity vectors
    status = nc_inq_varid( groupid, "orb_pos", &varid);
    check_err(status,__LINE__,__FILE__);

    cnt = 0;
    for ( int i=0; i<nscd; i++) {
      if ( k[i] == 1) {
	start[0] = cnt;
	count[1] = 3;

	status = nc_put_vara_float( groupid, varid, start, count, &orb[i][0]);
	check_err(status,__LINE__,__FILE__);

	cnt++;
      }
    }

    status = nc_inq_varid( groupid, "orb_vel", &varid);
    check_err(status,__LINE__,__FILE__);

    cnt = 0;
    for ( int i=0; i<nscd; i++) {
      if ( k[i] == 1) {
	start[0] = cnt;
	count[1] = 3;

	status = nc_put_vara_float( groupid, varid, start, count, &orb[i][3]);
	check_err(status,__LINE__,__FILE__);

	cnt++;
      }
    }
  }

  // Get orbit direction for granule-level netadata
  if ( first != -1) {
    if ( orb[first][5] > 0.0) {
      strcpy( sdir, "Ascending");
    } else {
      strcpy( sdir, "Descending");
    }
  }

  if ( last != -1) {
    if ( orb[last][5] > 0.0) {
      strcpy( edir, "Ascending");
    } else {
      strcpy( edir, "Descending");
    }
  }

  // Identify attitude times within granule range +/-10 seconds
  nk=0;
  for ( int i=0; i<nscd; i++) {
    double atm = atime[i] + (jds - jdg)*86400;
    if ( atm > (ltime-10) && atm < (mtime+10)) {
      k[i] = 1;
      nk++;
    } else {
      k[i] = 0;
    }
  }
  cout << nk << " attitude quaternions in time range" << endl;

  // Write attitude times
  if (k[0] != -1) {
    cout << "Writing attitude data" << endl;
    status = nc_inq_varid( groupid, "att_time", &varid);
    check_err(status,__LINE__,__FILE__);

    cnt = 0;
    for ( int i=0; i<nscd; i++) {
      if ( k[i] == 1) {
	start[0] = cnt;

	tai = yds2tai93( iyrsc16, idysc16, atime[i]);

	// Write TAI93 time
	status = nc_put_vara_double( groupid, varid, start, count, &tai);
	if (status != NC_NOERR) {
          check_err(status,__LINE__,__FILE__);
          //	  report_err(status,__LINE__,__FILE__);
	  //return 1;
	}
	//      check_err(status,__LINE__,__FILE__);

	cnt++;
      }
    }

    status = nc_inq_varid( groupid, "att_quat", &varid);
    check_err(status,__LINE__,__FILE__);

    cnt = 0;
    for ( int i=0; i<nscd; i++) {
      if ( k[i] == 1) {
	start[0] = cnt;
	count[1] = 4;

	status = nc_put_vara_float( groupid, varid, start, count, &quat[i][0]);
	check_err(status,__LINE__,__FILE__);

	cnt++;
      }
    }
  }
  
  delete[] k;

  return 0;
}


int l1aFile::write_adcs_bus( int32_t iyear, int32_t iday, 
                             int32_t ltime, int32_t mtime, 
                             int32_t iyrad, int32_t idyad, 
                             int32_t nadc, int32_t nbus,
                             double *adctime, uint8_t *admandone,
                             int16_t *adfftid,
                             double *bustime, uint8_t *adstate,
                             uint8_t *adsolution,
			     uint8_t *adcpkts[],
			     uint8_t *buspkts[]) {
 
  int groupid;
  int status = nc_inq_grp_ncid( ncid, "navigation_data", &groupid);
  check_err(status,__LINE__,__FILE__);

  int varid;
  size_t start[2]={0, 0};
  size_t count[2]={1, 0};
  size_t cnt;

  double tai;
  int16_t iyr16 = (int16_t) iyear;
  int16_t idy16 = (int16_t) iday;
  int16_t iyrad16 = (int16_t) iyrad;
  int16_t idyad16 = (int16_t) idyad;
  int32_t jdg = jday( iyr16, 1, idy16);
  int32_t jds = jday( iyrad16, 1, idyad16);

  uint8_t *k;
  k = new uint8_t[nadc];
  uint32_t nk=0;
  int32_t first=-1;
  double prevtime=0;
  for ( int i=0; i<nadc; i++) {
    double atm = adctime[i] + (jds - jdg)*86400;

    if ( atm > (ltime-10) && atm < (mtime+10)) {
      if ( (atm - prevtime) < 0.01) {
        cout << "Removing short adc period record: "
             << i << " " << fixed << setprecision(4)
             << atm << " " << prevtime << endl;
        prevtime = atm;
        k[i] = 0;
        continue;
      }
      k[i] = 1;
      nk++;
      if ( first == -1) first = i;

      if ( nk > SC_records_val) {
        nk = SC_records_val;
        break;
      }

    } else {
      k[i] = 0;
    }
    prevtime = atm;
  }
  cout << nk << " adc records in time range" << endl;

  
  // Write adc times
  cout << "Writing adc data" << endl;
  status = nc_inq_varid( groupid, "adcs_time", &varid);
  check_err(status,__LINE__,__FILE__);

  cnt = 0;
  for ( int i=0; i<nadc; i++) {
    if ( k[i] == 1 && cnt < SC_records_val) {
      start[0] = cnt;
      
      tai = yds2tai93( iyrad16, idyad16, adctime[i]);

      // Write TAI93 time
      status = nc_put_vara_double( groupid, varid, start, count, &tai);
      if (status != NC_NOERR) {
        check_err(status,__LINE__,__FILE__);
        //        report_err(status,__LINE__,__FILE__);
        //return 1;
      }
      //      check_err(status,__LINE__,__FILE__);

      cnt++;
    }
  }

  // Write admandone & adfftid
  status = nc_inq_varid( groupid, "admandone", &varid);
  check_err(status,__LINE__,__FILE__);

  cnt = 0;
  for ( int i=0; i<nadc; i++) {
    if ( k[i] == 1 && cnt < SC_records_val) {
      start[0] = cnt;

      status = nc_put_vara_uchar( groupid, varid, start, count, &admandone[i]);
      check_err(status,__LINE__,__FILE__);

      cnt++;
    }
  }

  status = nc_inq_varid( groupid, "adfftid", &varid);
  check_err(status,__LINE__,__FILE__);

  cnt = 0;
  for ( int i=0; i<nadc; i++) {
    if ( k[i] == 1 && cnt < SC_records_val) {
      start[0] = cnt;

      status = nc_put_vara_short( groupid, varid, start, count, &adfftid[i]);
      check_err(status,__LINE__,__FILE__);

      cnt++;
    }
  }

  // Write ADCS_tlm
  status = nc_inq_varid( groupid, "ADCS_tlm", &varid);
  check_err(status,__LINE__,__FILE__);

  cnt = 0;
  for ( int i=0; i<nadc; i++) {
    if ( k[i] == 1 && cnt < SC_records_val) {
      start[0] = cnt;
      count[1] = this->apktsize-14;
      status = nc_put_vara( groupid, varid, start, count, &adcpkts[i][14]);
      check_err(status,__LINE__,__FILE__);

      cnt++;
    }
  }

  delete[] k;


  // Identify bus times within granule range +/-10 seconds
  k = new uint8_t[nbus];
  nk=0;
  first=-1;
  prevtime = 0;
  for ( int i=0; i<nbus; i++) {
    double btm = bustime[i] + (jds - jdg)*86400;

    if ( btm > (ltime-10) && btm < (mtime+10)) {
      if ( (btm - prevtime) < 0.01) {
        cout << "Removing short bus period record: "
             << i << " " << fixed << setprecision(4)
             << btm << " " << prevtime << endl;
        prevtime = btm;
        k[i] = 0;
        continue;
      }
      k[i] = 1;
      nk++;
      if ( first == -1) first = i;

      if ( nk > SC_records_val) {
        nk = SC_records_val;
        break;
      }

    } else {
      k[i] = 0;
    }
    prevtime = btm;
  }
  cout << nk << " bus records in time range" << endl;

  // Write bus times
  cout << "Writing bus data" << endl;
  status = nc_inq_varid( groupid, "bus_time", &varid);
  check_err(status,__LINE__,__FILE__);

  cnt = 0;
  for ( int i=0; i<nbus; i++) {
    if ( k[i] == 1 && cnt < SC_records_val) {
      start[0] = cnt;
      
      tai = yds2tai93( iyrad16, idyad16, bustime[i]);

      // Write TAI93 time
      status = nc_put_vara_double( groupid, varid, start, count, &tai);
      if (status != NC_NOERR) {
        check_err(status,__LINE__,__FILE__);
        //        report_err(status,__LINE__,__FILE__);
        //return 1;
      }
      //      check_err(status,__LINE__,__FILE__);

      cnt++;
    }
  }

  // Write adstate
  status = nc_inq_varid( groupid, "adstate", &varid);
  check_err(status,__LINE__,__FILE__);

  cnt = 0;
  for ( int i=0; i<nbus; i++) {
    if ( k[i] == 1 && cnt < SC_records_val) {
      start[0] = cnt;

      status = nc_put_vara_uchar( groupid, varid, start, count, &adstate[i]);
      check_err(status,__LINE__,__FILE__);

      cnt++;
    }
  }

  // Write adsolution
  status = nc_inq_varid( groupid, "adsolution", &varid);
  check_err(status,__LINE__,__FILE__);

  cnt = 0;
  for ( int i=0; i<nbus; i++) {
    if ( k[i] == 1 && cnt < SC_records_val) {
      start[0] = cnt;

      status = nc_put_vara_uchar( groupid, varid, start, count, &adsolution[i]);
      check_err(status,__LINE__,__FILE__);

      cnt++;
    }
  }

  // Write bus_critical_tlm
  status = nc_inq_varid( groupid, "bus_critical_tlm", &varid);
  check_err(status,__LINE__,__FILE__);

  cnt = 0;
  for ( int i=0; i<nbus; i++) {
    if ( k[i] == 1 && cnt < SC_records_val) {
      start[0] = cnt;
      count[1] = this->bpktsize-14;
      status = nc_put_vara( groupid, varid, start, count, &buspkts[i][14]);
      check_err(status,__LINE__,__FILE__);

      cnt++;
    }
  }

  delete[] k;

  return 0;
}


int l1aFile::write_granule_metadata( int32_t iyear, int32_t iday, 
                                     int32_t ltime, int32_t mtime,
                                     int32_t orbit,
                                     const char* l1a_name,
                                     char *sdir, char *edir,
                                     uint8_t *p1,
                                     int32_t isc, uint8_t *mode,
                                     int argc, char *argv[]) {

  int status;
  char buf[2048];

  int16_t year, doy;

  stringstream timestr, datestr;

  status = nc_put_att_int(ncid,  NC_GLOBAL, "orbit_number", NC_INT, 
                            1, &orbit);
  check_err(status,__LINE__,__FILE__);

  // Write history attribute
  strcpy( buf, basename(argv[0]));
  for (int i=1; i<argc; i++) {
    strcat( buf, " ");
    strcat( buf, argv[i]);
  }
  status = nc_put_att_text(ncid, NC_GLOBAL, "history", strlen(buf), buf);
  check_err(status,__LINE__,__FILE__);

  // Write format version and instrument number attributes
  uint8_t fmtver = p1[28];
  uint8_t insnum = p1[29];

  status = nc_put_att_uchar(ncid,  NC_GLOBAL, "format_version", NC_BYTE, 
                            1, &fmtver);
  check_err(status,__LINE__,__FILE__);

  status = nc_put_att_uchar(ncid,  NC_GLOBAL, "instrument_number", NC_BYTE, 
                            1, &insnum);
  check_err(status,__LINE__,__FILE__);

  // time_coverage_start
  year = iyear;
  doy = iday;

  double dsec = (double) ltime;
  double myUnixTime= yds2unix(year, doy, dsec);
  strcpy( buf, unix2isodate(myUnixTime, 'G'));

  status = nc_put_att_text(ncid, NC_GLOBAL, "time_coverage_start", 
                           strlen(buf), buf);
  check_err(status,__LINE__,__FILE__);


  // time_coverage_end
  dsec = (double) mtime;

  if ( dsec >= 86400) {
    dsec -= 86400.0;
    doy = doy + 1;

    if (isleap(iyear)) {
      if (doy == 367) {
        year++;
        doy = 1;
      }
    } else {
      if (doy == 366) {
        year++;
        doy = 1;
      }
    }
  }

  myUnixTime= yds2unix(year, doy, dsec);
  strcpy( buf, unix2isodate(myUnixTime, 'G'));

  status = nc_put_att_text(ncid, NC_GLOBAL, "time_coverage_end", 
                           strlen(buf), buf);
  check_err(status,__LINE__,__FILE__);

  strcpy( buf, unix2isodate(now(), 'G'));
  status = nc_put_att_text(ncid, NC_GLOBAL, "date_created", strlen(buf), buf);
  check_err(status,__LINE__,__FILE__);

  strcpy( buf, sdir);
  status = nc_put_att_text(ncid, NC_GLOBAL, "startDirection", strlen(buf), buf);
  check_err(status,__LINE__,__FILE__);

  strcpy( buf, edir);
  status = nc_put_att_text(ncid, NC_GLOBAL, "endDirection", strlen(buf), buf);
  check_err(status,__LINE__,__FILE__);


  // Determine sensor mode for granule and write
  int32_t nday=0, nngt=0;
  for (int i=0; i<isc; i++) {
    if ( mode[i] == 4) nday++;
    if ( mode[i] == 5) nngt++;
  }
  if ( nday == isc) strcpy( buf, "Day");
  else if ( nngt == isc) strcpy( buf, "Night");
  else if ( (nday + nngt) == isc) strcpy( buf, "Mixed");
  else strcpy( buf, "Other");
  status = nc_put_att_text(ncid, NC_GLOBAL, "day_night_flag", strlen(buf), buf);
  check_err(status,__LINE__,__FILE__);


  // Write number of scans
  status = nc_put_att_int(ncid,  NC_GLOBAL, "number_of_filled_scans",
                          NC_INT, 1, &isc);
  check_err(status,__LINE__,__FILE__);

  return 0;
}

int l1aFile::openl1(char* l1_filename) {

    int status;
    size_t t_len; /* attribute lengths */
    char dimname[256];

    status = nc_open(l1_filename, NC_NOWRITE, &this->ncid);
    check_err(status, __LINE__, __FILE__);
    status = nc_inq_grps(ncid, &ngrps, gid);
    check_err(status, __LINE__, __FILE__);


    /* find out how much space is needed for attribute values */
    status = nc_inq_attlen(ncid, NC_GLOBAL, "platform", &t_len);
    check_err(status, __LINE__, __FILE__);

    char *platform;
    platform = (char *) malloc(t_len + 1);

    status = nc_get_att_text(ncid, NC_GLOBAL, "platform", platform);
    check_err(status, __LINE__, __FILE__);
    this->platform.assign(platform);
    free(platform);

    status = nc_inq_ndims(ncid, &ndims);
    check_err(status, __LINE__, __FILE__);
    for (int i = 0; i < ndims; i++) {
        this->dimid[i] = i;
        nc_inq_dimname(ncid, i, dimname);
        if (strcmp(dimname, "SC_records") == 0) {
            size_t dimlen;
            nc_inq_dimlen(ncid, i, &dimlen);
            SC_records_val = dimlen;
        }
    }

    return 0;
}


int l1aFile::copyl1( char *ifilename, char *ofilename, l1aFile* l1_ofile,
                     int32_t sscan, int32_t escan, 
                     int32_t spixl, int32_t epixl) {

   int status;

   ifstream viirs_l1a_data_structure;
   string line;
   string dataStructureFile;
   
   if ( this->platform == "JPSS-1")
     dataStructureFile.assign("$OCDATAROOT/viirs/j1/VIIRS_Level-1A_Data_Structure_J1.cdl");
   else
     dataStructureFile.assign("$OCDATAROOT/viirs/npp/VIIRS_Level-1A_Data_Structure.cdl");
   expandEnvVar( &dataStructureFile);

   viirs_l1a_data_structure.open( dataStructureFile.c_str(), ifstream::in);
   if ( viirs_l1a_data_structure.fail() == true) {
     cout << "\"" << dataStructureFile.c_str() << "\" not found" << endl;
     exit(1);
   }

   size_t start[4], count[4], ofile_start[4]={0,0,0,0};
   int varid;
   double scan_start_time=FP_NAN, scan_end_time=FP_NAN;

   // Find "dimensions" section of CDL file
   while(1) {
     getline( viirs_l1a_data_structure, line);
     size_t pos = line.find("dimensions:");
     if ( pos == 0) break;
   }

   while(1) {
     getline( viirs_l1a_data_structure, line);
     size_t pos = line.find(" = ");
     if ( pos == string::npos) break;

   } // while loop

   ngrps = 0;
   // Loop through groups
   while(1) {
     getline( viirs_l1a_data_structure, line);

     // Check if end of CDL file
     // If so then close CDL file and break
     if (line.substr(0,1).compare("}") == 0) {
       viirs_l1a_data_structure.close();
       break;
     }

     // Check for beginning of new group
     size_t pos = line.find("group:");

     // If found then create new group and variables
     if ( pos == 0) {

       int numDims=0;
       int varDims[NC_MAX_DIMS];
       size_t dimSize[NC_MAX_DIMS];
       char dimName[NC_MAX_NAME+1];
       string sname;

       char *buffer = NULL;

       // Loop through datasets in group
       getline( viirs_l1a_data_structure, line); // skip "variables:"
       while(1) {
         getline( viirs_l1a_data_structure, line);

         if (line.length() == 0) continue;
         if (line.substr(0,1).compare("\r") == 0) continue;
         if (line.substr(0,1).compare("\n") == 0) continue;

         size_t found = line.find("DNB");
         if (found != string::npos) {
           continue;
         }

         size_t pos = line.find(":");

         // No ":" found, new dataset or empty line or end-of-group
         if ( pos == string::npos) {

           // Note: scan_start_time & scan_end_time are for the extract
 
           if ( numDims > 0) {
             status = nc_get_vara( gid[ngrps], varid, 
                                   start, count, (void *) buffer);
             check_err(status,__LINE__,__FILE__);

             if ( sname.compare( "scan_start_time") == 0) {
               memcpy( &scan_start_time, buffer, 8);
               cout << scan_start_time << endl;
               scan_end_time = scan_start_time + l1_ofile->SC_records_val-20;
               cout << scan_end_time << endl;
             }

             int ofile_varid;
             status = nc_inq_varid( l1_ofile->gid[ngrps], 
                                    sname.c_str(), &ofile_varid);
             check_err(status,__LINE__,__FILE__);

             status = nc_put_vara( l1_ofile->gid[ngrps], ofile_varid, 
                                   ofile_start, count, (void *) buffer);
             check_err(status,__LINE__,__FILE__);

             free (buffer);
             buffer = NULL;
           }

           if (line.substr(0,10).compare("} // group") == 0) break;

           // Parse short name (sname)
           pos = line.find("(");
           size_t posSname = line.substr(0, pos).rfind(" ");
           sname.assign(line.substr(posSname+1, pos-posSname-1));
           cout << "Extracting: " << sname.c_str() << endl;

           // Parse variable dimension info
           this->parseDims( line.substr(pos+1, string::npos), 
                            &numDims, varDims);

           size_t bufcnt = 1;
           for (int i=0; i<numDims; i++) { 
             nc_inq_dim( ncid, varDims[i], dimName, &dimSize[i]);
             if ( strcmp(dimName, "number_of_scans") == 0) {
               start[i] = sscan;
               count[i] = escan - sscan + 1;
             } else if ( strcmp(dimName, "SC_records") == 0) {
               if ( strcmp(sname.c_str(), "att_time") == 0) {
                 double *time = new double[dimSize[i]];
                 status = nc_inq_varid( gid[ngrps], sname.c_str(), &varid);
                 check_err(status,__LINE__,__FILE__);

                 status = nc_get_var_double( gid[ngrps], varid, time);
                 check_err(status,__LINE__,__FILE__);

                 // Note: scan_start_time & scan_end_time are for the extract

                 bool found = false;
                 count[i] = 0;
                 for (uint32_t j=0; j<dimSize[i]; j++) {
                   if (time[j] > scan_start_time-10 && 
                       time[j] < scan_end_time+10) {
                     count[i]++;
                     if (!found) start[i] = j;
                     found = true;
                   }
                 }
                 if(!found) {
                   printf("-E- %s %d: could not find starting record for att_time.\n", 
                           __FILE__, __LINE__);
                   exit(1);
                 }

                 if (count[i] == l1_ofile->SC_records_val+1)
                   count[i] = l1_ofile->SC_records_val;

                 if (count[i] > l1_ofile->SC_records_val) {
                   cout << "Insufficient allocation for extracted SC_records dimension" << endl;
                   cout << 
                     "Allocated: " << l1_ofile->SC_records_val << 
                     "  Required: " << count[i] << endl;
                   exit(1);
                 }
                 delete(time);
               }
             } else if ( strcmp(dimName, "Mband_pixels") == 0) {
               // Mband_pixels = 3200
               start[i] = spixl;
               count[i] = epixl - spixl + 1;
             } else if ( strcmp(dimName, "Iband_pixels") == 0) {
               // Iband_pixels = 6400
               start[i] = 2*spixl;
               count[i] = 2*(epixl - spixl + 1);
             } else if ( strcmp(dimName, "Mband_samples") == 0) {
               /*
                 Mband_samples = 6304 ;

                 pixel_offset range              sample_offset

                     0 - 639                     pixel_offset
                   640 - 1007                  640 + 2*(pixel_offset - 640)
                  1008 - 2191                 1376 + 3*(pixel_offset - 1008)
                  2192 - 2559                 4928 + 2*(pixel_offset - 2192)
                  2560 - 3199                 5664 + (pixel_offset - 2560)


               */
               if ( spixl < 639) {
                 start[i] = spixl;
               } else if ( spixl > 639 && spixl <= 1007) {
                 start[i] = 640 + 2*(spixl - 640);
               } else if ( spixl > 1007 && spixl <= 2191) {
                 start[i] = 1376 + 3*(spixl - 1008);
               } else if ( spixl > 2191 && spixl <= 2559) {
                 start[i] = 4928 + 2*(spixl - 2192);
               } else if ( spixl > 2559) {
                 start[i] = 5664 + (spixl - 2560);
              }

               if ( epixl <= 639) {
                 count[i] = epixl;
               } else if ( epixl > 639 && epixl <= 1007) {
                 count[i] = 641 + 2*(epixl - 640);
               } else if ( epixl > 1007 && epixl <= 2191) {
                 count[i] = 1378 + 3*(epixl - 1008);
               } else if ( epixl > 2191 && epixl <= 2559) {
                 count[i] = 4929 + 2*(epixl - 2192);
               } else if ( epixl > 2559) {
                 count[i] = 5664 + (epixl - 2560);
              }
               count[i] = count[i] - start[i] + 1;
             } else {
               start[i] = 0;
               count[i] = dimSize[i];
             }
             bufcnt *= count[i];
             //             cout << line.c_str() << " " << i << " " << dimName
             //   << " " << dimSize[i] << endl;
           }

           status = nc_inq_varid( gid[ngrps], sname.c_str(), &varid);
           check_err(status,__LINE__,__FILE__);

           nc_type dtype;
           size_t typesz;
           status = nc_inq_vartype( gid[ngrps], varid, &dtype);
           check_err(status,__LINE__,__FILE__);
           
           status = nc_inq_type( gid[ngrps], dtype, NULL, &typesz);
           check_err(status,__LINE__,__FILE__);

           buffer = (char *) calloc( bufcnt, typesz);
         } // if ( pos == string::npos)
       } // datasets in group loop

       ngrps++;

     } // New Group loop
   } // Main Group loop
   
   int ngatts;
   nc_type type;
   size_t attlen, typesz;

   char name[NC_MAX_NAME], databuf[1024];

   status = nc_inq_natts(ncid, &ngatts);
   check_err(status,__LINE__,__FILE__);
   
   stringstream history;
   history << "l1aextract_viirs " << basename(ifilename) << " "
           << (spixl+1) << " " << (epixl+1) << " "
           << (sscan+1) << " " << (escan+1) << " "
           << basename(ofilename);

   for (int i=0; i<ngatts; i++) {
     status = nc_inq_attname( ncid, NC_GLOBAL, i, name);
     status = nc_inq_att( ncid, NC_GLOBAL, name, &type, &attlen);
     status = nc_inq_type( ncid, type, NULL, &typesz);
     status = nc_get_att( ncid, NC_GLOBAL, name, (void *) databuf);

     if ( strcmp(name, "product_name") == 0) {
       strcpy(databuf, basename(ofilename));
       attlen = strlen(databuf);
     }

     if ( strcmp(name, "history") == 0) {
       strcpy(databuf, history.str().c_str());
       attlen = strlen(databuf);
     }

     if ( strcmp(name, "time_coverage_start") == 0) {
       int nleap = leapseconds_since_1993( scan_start_time);
       uint32_t int_scan_start_time =
         (uint32_t) (scan_start_time - nleap + 0.5);
       double sec = (double) (int_scan_start_time % 86400);

       int16_t iyr, imn, idy;
       sscanf ( databuf,"%hu-%hu-%hu%*s", &iyr, &imn, &idy);

       double myUnixTime= ymds2unix( iyr, imn, idy, sec);
       strcpy( databuf, unix2isodate(myUnixTime, 'G'));

     }

     if ( strcmp(name, "time_coverage_end") == 0) {
       int nleap = leapseconds_since_1993( scan_end_time);
       uint32_t int_scan_end_time =
         (uint32_t) (scan_end_time - nleap + 0.5);
       double sec = (double) (int_scan_end_time % 86400);

       int16_t iyr, imn, idy;
       sscanf ( databuf,"%hu-%hu-%hu%*s", &iyr, &imn, &idy);

       double myUnixTime= ymds2unix( iyr, imn, idy, sec);
       strcpy( databuf, unix2isodate(myUnixTime, 'G'));

     }

     if ( strcmp(name, "date_created") == 0) {
       strcpy( databuf, unix2isodate(now(), 'G'));
     }

     if ( strcmp(name, "number_of_filled_scans") == 0) {
       int32_t filled_scans = escan - sscan + 1;
       memcpy( databuf, &filled_scans, sizeof(int32_t));
     }

     status = nc_put_att( l1_ofile->ncid, NC_GLOBAL,
                          name, type, attlen, databuf);
  if (status != NC_NOERR)
  {
     printf("-E - nc_put_att failed for ncid: %i\n",ncid);
  }
 
   }
 
   if (spixl != 0 || epixl != 3199) {
     int i;
     i = spixl + 1;
     status = nc_put_att_int( l1_ofile->ncid, NC_GLOBAL, "extract_pixel_start", 
                             NC_INT, 1, &i);
     i = epixl + 1;
     status = nc_put_att_int( l1_ofile->ncid, NC_GLOBAL, "extract_pixel_stop", 
                             NC_INT, 1, &i);
   }


   return 0;
}


int l1aFile::close() {
  
  int status = nc_close(ncid);

  if (status != NC_NOERR)
  {
     printf("-E - nc_close failed for ncid: %i\n",ncid);
  }
  return 0;
}

int createNCDF( int ncid, const char *sname, const char *lname, 
                const char *standard_name, const char *units,
                void *fill_value,
                const char *flag_values, const char *flag_meanings,
                double low, double high, int nt,
                int rank, int *dimids) {

  int32_t varid;
  int status;
  size_t dimlength;
  size_t chunksize[3];
     
  /* Create the NCDF dataset */
  status = nc_def_var(ncid, sname, nt, rank, dimids, &varid);
  if( status != NC_NOERR) {
    printf("-E- %s %d: %s for %s\n", 
	   __FILE__, __LINE__, nc_strerror(status), sname);
    exit(1);
  } 

  // Set fill value
  double fill_value_dbl;
  memcpy( &fill_value_dbl, fill_value, sizeof(double));

  int8_t i8;
  uint8_t ui8;
  int16_t i16;
  int32_t i32;
  float f32;

  //  if ( (low < high) && (low != fill_value_dbl)) {
  if ( low != fill_value_dbl) {
    if ( nt == NC_BYTE) {
      i8 = fill_value_dbl;
      status = nc_def_var_fill( ncid, varid, 0, (void *) &i8);
    } else if ( nt == NC_UBYTE) {
      ui8 = fill_value_dbl;
      status = nc_def_var_fill( ncid, varid, 0, (void *) &ui8);
    } else if ( nt == NC_SHORT) {
      i16 = fill_value_dbl;
      status = nc_def_var_fill( ncid, varid, 0, (void *) &i16);
    } else if ( nt == NC_INT) {
      i32 = fill_value_dbl;
      status = nc_def_var_fill( ncid, varid, 0, (void *) &i32);
    } else if ( nt == NC_FLOAT) {
      f32 = fill_value_dbl;
      status = nc_def_var_fill( ncid, varid, 0, (void *) &f32);
    } else {
      status = nc_def_var_fill( ncid, varid, 0, (void *) &fill_value_dbl);
    }
    check_err(status,__LINE__,__FILE__);
  }

  /* vary chunck size based on dimensions */ 
  int do_deflate = 0;
  if ( rank == 3 && (strncmp(sname, "EV_", 3) == 0)) {
    status = nc_inq_dimlen(ncid, dimids[2], &dimlength);
    chunksize[0] = 1;
    chunksize[1] = 16;
    chunksize[2] = dimlength/10;
    do_deflate = 1;
  }

  /* Set compression */
  if ( do_deflate) {
    /* First set chunking */
    status = nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksize);
    if (status != NC_NOERR) {
      printf("-E- %s %d: %s for %s\n", __FILE__, __LINE__,
             nc_strerror(status), sname);
      exit(1);
    }

    /* Now we can set compression */
    //    status = nc_def_var_deflate(ncid, varid, NC_NOSHUFFLE, 1, 5);
    status = nc_def_var_deflate(ncid, varid, NC_SHUFFLE, 1, 5);
    if (status != NC_NOERR) {
      printf("-E- %s %d: %s for %s\n", __FILE__, __LINE__,
             nc_strerror(status), sname);
      exit(1);
    }
  }


  /* Add a "long_name" attribute */
  status = nc_put_att_text(ncid, varid, "long_name", strlen(lname), lname);
  if( status != NC_NOERR) {
    printf("-E- %s %d: %s for %s\n", 
	   __FILE__, __LINE__, nc_strerror(status), "long_name");
    exit(1);
  } 

  /* Add a "flag_values" attribute if specified*/
  // Parse string and save as signed bytes
  if ( strcmp( flag_values, "") != 0) {

    size_t curPos=0;

    string fv;
    fv.assign( flag_values);
    size_t pos = fv.find("=", curPos);
    fv = fv.substr(pos+1);

    size_t semicln = fv.find(";");
    pos = 0;

    int8_t vec[1024];
    int n = 0;
    while(pos != semicln) {
      pos = fv.find(",", curPos);
      if ( pos == string::npos) 
        pos = semicln;

      string flag_value;
      istringstream iss(fv.substr(curPos, pos-curPos));
      iss >> skipws >> flag_value;
      vec[n++] = atoi( flag_value.c_str());
      curPos = pos + 1;
    }

    status = nc_put_att_schar(ncid, varid, "flag_values", NC_BYTE, n, vec);
    if( status != NC_NOERR) {
      printf("-E- %s %d: %s for %s\n", 
             __FILE__, __LINE__, nc_strerror(status), "flag_values");
      exit(1);
    } 
  }

  /* Add a "flag_meanings" attribute if specified*/
  if ( strcmp( flag_meanings, "") != 0) {
    status = nc_put_att_text(ncid, varid, "flag_meanings", 
                             strlen(flag_meanings), flag_meanings);
    if( status != NC_NOERR) {
      printf("-E- %s %d: %s for %s\n", 
             __FILE__, __LINE__, nc_strerror(status), "flag_meanings");
      exit(1);
    } 
  }

  /* Add "valid_min/max" attributes if specified */
  if (low < high) {
    switch(nt) {              /* Use the appropriate number type */
    case NC_BYTE:
      {
	uint8_t vr[2];
	vr[0] = (uint8_t)low;
	vr[1] = (uint8_t)high;
	status = nc_put_att_uchar(ncid, varid,"valid_min",NC_BYTE,1,&vr[0]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_min");
	  exit(1);
	} 
	status = nc_put_att_uchar(ncid, varid,"valid_max",NC_BYTE,1,&vr[1]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_max");
	  exit(1);
	} 
      }
      break;
    case NC_UBYTE:
      {
	uint8_t vr[2];
	vr[0] = (uint8_t)low;
	vr[1] = (uint8_t)high;
	status = nc_put_att_uchar(ncid, varid,"valid_min",NC_UBYTE,1,&vr[0]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_min");
	  exit(1);
	} 
	status = nc_put_att_uchar(ncid, varid,"valid_max",NC_UBYTE,1,&vr[1]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_max");
	  exit(1);
	} 
      }
      break;
    case NC_SHORT:
      {
	int16_t vr[2];
	vr[0] = (int16_t)low;
	vr[1] = (int16_t)high;
	status = nc_put_att_short(ncid, varid,"valid_min",NC_SHORT,1,&vr[0]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_min");
	  exit(1);
	} 
	status = nc_put_att_short(ncid, varid,"valid_max",NC_SHORT,1,&vr[1]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_max");
	  exit(1);
	} 
      }
      break;
    case NC_USHORT:
      {
	uint16_t vr[2];
	vr[0] = (uint16_t)low;
	vr[1] = (uint16_t)high;
	status = nc_put_att_ushort(ncid, varid,"valid_min",NC_USHORT,1,&vr[0]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_min");
	  exit(1);
	} 
	status = nc_put_att_ushort(ncid, varid,"valid_max",NC_USHORT,1,&vr[1]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_max");
	  exit(1);
	} 
      }
      break;
    case NC_INT:
      {
	int32_t vr[2];
	vr[0] = (int32_t)low;
	vr[1] = (int32_t)high;
	status = nc_put_att_int(ncid, varid,"valid_min",NC_INT,1,&vr[0]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_min");
	  exit(1);
	} 
	status = nc_put_att_int(ncid, varid,"valid_max",NC_INT,1,&vr[1]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_max");
	  exit(1);
	} 
      }
      break;
    case NC_FLOAT:
      {
	float vr[2];
	vr[0] = (float)low;
	vr[1] = (float)high;
	status = nc_put_att_float(ncid, varid,"valid_min",NC_FLOAT,1,&vr[0]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_min");
	  exit(1);
	} 
	status = nc_put_att_float(ncid, varid,"valid_max",NC_FLOAT,1,&vr[1]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_max");
	  exit(1);
	} 
      }
      break;
    case NC_DOUBLE:
      {
	double vr[2];
	vr[0] = low;
	vr[1] = high;
	status = nc_put_att_double(ncid, varid,"valid_min",NC_DOUBLE,1,&vr[0]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_min");
	  exit(1);
	} 
	status = nc_put_att_double(ncid, varid,"valid_max",NC_DOUBLE,1,&vr[1]);
	if( status != NC_NOERR) {
	  printf("-E- %s %d: %s for %s\n", 
		 __FILE__, __LINE__, nc_strerror(status), "valid_max");
	  exit(1);
	} 
      }
      break;
    default:
      fprintf(stderr,"-E- %s line %d: ",__FILE__,__LINE__);
      fprintf(stderr,"Got unsupported number type (%d) ",nt);
      fprintf(stderr,"while trying to create NCDF variable, \"%s\", ",sname);
      return(PROGRAMMER_BOOBOO);
    }
  }           
    
  /* Add a "units" attribute if one is specified */
  if(units != NULL && *units != 0) {
    status = nc_put_att_text(ncid, varid, "units", strlen(units), units);
    if( status != NC_NOERR) {
      printf("-E- %s %d: %s for %s\n", 
	     __FILE__, __LINE__, nc_strerror(status), "units");
      exit(1);
    } 
  }

  /* Add a "standard_name" attribute if one is specified */
  if(standard_name != NULL && *standard_name != 0) {
    status = nc_put_att_text(ncid, varid, "standard_name", 
			     strlen(standard_name), standard_name);
    if( status != NC_NOERR) {
      printf("-E- %s %d: %s for %s\n", 
	     __FILE__, __LINE__, nc_strerror(status), "standard_name");
      exit(1);
    } 
  }
  
  return 0;
}


int jgleap( int32_t jday, int32_t *leap) {

  // NO LONGER USED

  // Determines leap seconds since GPS epoch (6 January 1980)
  // for a specified Astronomical Julian Day

  // Updated for 2015 JUL 1 leapsec 02/11/15 JMG
  // Julian days of leap second epochs
  int32_t jdleap[17] = {2444787, 2445152, 2445517, 2446248, 
                        2447162, 2447893, 2448258, 2448805, 
                        2449170, 2449535, 2450084, 2450631, 
                        2451180, 2453737, 2454833, 2456110,
                        2457205};

  int32_t cnt=0;
  for ( int i=0; i<17; i++) {
    if ( jday >= jdleap[i]) cnt++;
  }

  *leap = cnt;

  return 0;
}
