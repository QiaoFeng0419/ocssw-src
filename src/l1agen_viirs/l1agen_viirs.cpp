#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <sstream>
#include <iomanip>
#include <getopt.h>

#include "l1agen_viirs.h"
#include "netcdf.h"
#include <timeutils.h>

//#define SCAN_TIME 1.779275

#define VERSION "2.06"

//    Modification history:
//  Programmer     Organization   Date     Ver   Description of change
//  ----------     ------------   ----     ---   ---------------------
//  Joel Gales     FutureTech     06/10/14       Original development
//                                               based on IDL routines
//                                               developed by F. Patt
//
//  Joel Gales     FutureTech     09/22/14       Define pbuffer size
//                                               in header file.
//                                               Set to 32768.
//  Joel Gales     FutureTech     10/22/14 0.012 Replace usds() with
//                                               rice() for decryption.
//  Joel Gales     FutureTech     11/05/14 0.013 Add ADCS/Bus fields
//                                               Handle day roll-over
//  Joel Gales     FutureTech     11/19/14 0.014 Include unistd.h for
//                                               getpid()
//  Joel Gales     FutureTech     11/20/14 0.015 Trap tellg() = -1
//                                               in read_packet()
//                                               Set endfile=1
//  Joel Gales     FutureTech     12/16/14 0.016 Encoder data split into
//                                               separate fields for the
//                                               telescope and HAM data
//                                               and stored as uint16_t
//  Joel Gales     FutureTech     12/26/14 0.017 Fix bad offsets
//                                               Change 16363 to 16383.
//  Joel Gales     FutureTech     01/14/15 0.018 Add DNB_sequence and 
//                                               additional engineering
//                                               datafields, number_of_scans
//                                               global attribute 
//  Joel Gales     FutureTech     02/05/15 0.019 Add support for L1B granule
//                                               listing file
//  Joel Gales     FutureTech     02/21/15 0.020 Remove check for EOF after
//                                               read_viirs_scan_packets()
//  Joel Gales     FutureTech     02/23/15 0.021 Add support for granules
//                                               greater than 5 minutes.
//                                               Add compression for EV fields.
//  Joel Gales     FutureTech     03/02/15 0.022 Add support for adsolution,
//                                               Fix bugs in unpack_viirs_cal()
//                                               Add support for DR data
//  Joel Gales     FutureTech     03/09/15 0.023 Store flag_values attribute
//                                               as ints
//  Joel Gales     FutureTech     03/26/15 0.024 Make number of scans in
//                                               "complete" file
//                                               depend on granule period
//  Joel Gales     FutureTech     04/03/15 0.025 Allocate mbands, ibands, dnb,
//                                               pbuffer arrays dynamically
//  Joel Gales     FutureTech     04/23/15 0.026 Fix problem with orphan
//                                               packets in 
//                                               read_viirs_scan_packets
//  Joel Gales     FutureTech     04/30/15 0.027 Change ham_side and 
//                                               sensor_mode from short
//                                               to ubyte.  Change flt/dbl
//                                               flag value from -999 to
//                                               -999.9
//  Joel Gales     FutureTech     05/05/15 0.028 Change flag value for SD,
//                                               SV, and BB to -999.
//                                               Add support for orbit number
//                                               determination
//  Joel Gales     FutureTech     05/12/15 0.029 Return from unpack_sci()
//                                               if (lp == -4)
//  Joel Gales     FutureTech     05/12/15 0.030 Remove duplicate pbuffer array
//                                               Check for M4 and M5 cal data 
//                                               replacement
//  Joel Gales     FutureTech     06/12/15 0.031 Move adsolution calculation
//                                               from astore loop to bstore
//                                               loop in extract_adcs_bus
//  Joel Gales     FutureTech     06/15/15 0.032 Change allocation of 
//                                               adsolution from nacd to nbus
//  Joel Gales     FutureTech     06/16/15 0.033 Generate output text files with
//                                               Start/Stop Time, 
//                                               Number of Records
//  Joel Gales     FutureTech     06/24/15 0.034 Fix day used in yds2tai for
//                                               stime/etime in write scan
//                                               metadata
//  Joel Gales     FutureTech     06/27/15 0.035 Exit if OCVARROOT not defined
//  Joel Gales     FutureTech     07/05/15 0.036 Don't exit for short last
//                                               packet.
//  Joel Gales     FutureTech     07/23/15 0.040 Modify text file to use
//                                               parameter=value, Output
//                                               actual start/stop time,
//                                               granule name, quality flag
//  Joel Gales     FutureTech     07/27/15 0.041 Add additional range checks
//                                               in unpack_viirs_scan() and
//                                               unpack_cal_sci()
//  Joel Gales     FutureTech     07/30/15 0.042 Move date/time functions to
//                                               libtimeutils
//  Joel Gales     FutureTech     08/04/15 0.043 Fix basename parameter in text
//                                               file ('txt' -> 'nc')
//  Joel Gales     FutureTech     08/19/15 0.044 Set stop_time entry in text
//                                               file to start time of last
//                                               scan rather than end time
//  Joel Gales     FutureTech     08/19/15 0.045 Change NPP to SNPP
//  Joel Gales     FutureTech     08/19/15 0.046 In V0.044 forgot to change
//                                               eyear, eday to syear, sday
//                                               Also fix "time_coverage_start"
//  Joel Gales     FutureTech     08/31/15 0.047 Trap fill values for ibid
//                                               in unpack_viirs_cal() and
//                                               out-of-order/duplicate stime
//  Joel Gales     FutureTech     09/02/15 0.048 Fix typo for out-of-order fix
//  Joel Gales     FutureTech     09/04/15 0.049 Change valid_range to
//                                               valid_min for SHORT & USHORT
//                                               fields
//  Joel Gales     FutureTech     09/10/15 0.050 Check for end-of-file before
//                                               computing stime
//  Joel Gales     FutureTech     09/21/15 0.051 Check for band number less
//                                               than 1 and bad return status
//                                               from unpack_cal_sci() in 
//                                               unpack_viirs_cal() and
//                                               idat[25] less than one in
//                                               unpack_cal_sci()
//  Joel Gales     FutureTech     09/21/15 0.052 Report and return rather than
//                                               exit for orb, att, adcs, bus
//                                               TAI time write errors
//  Joel Gales     FutureTech     09/22/15 0.053 Move sync word check in 
//                                               unpack_cal_sci() before memcpy
//  Joel Gales     FutureTech     09/25/15 0.054 Add sync word checks to
//                                               unpack_sci() 
//  Joel Gales     FutureTech     11/02/15 0.055 Read viirs orbit time data
//                                               before main loop with scope
//                                               throughout main program
//  Joel Gales     FutureTech     12/24/15 0.056 if npkts == 1 continue
//                                               Set exit status to 101
//  Joel Gales     FutureTech     01/01/16 0.057 Fix day bug for year change
//  Joel Gales     FutureTech     01/28/16 0.990 Write most global attributes
//                                               from CDL file
//  Joel Gales     FutureTech     02/25/16 0.991 Check status (not iret) after
//                                               return from unpack_cal_sci()
//  Joel Gales     FutureTech     03/29/16 0.992 Remove main loop break if
//                                               no records in L1A granule
//  Joel Gales     FutureTech     04/29/16 0.993 Delete 0-scan L1A granules
//                                               and do not list in outlist
//  Joel Gales     FutureTech     05/18/16 0.994 Implement updated M4/M5 
//                                               cal view replacement logic
//  Joel Gales     FutureTech     09/06/16 0.995 Add additional checks for
//                                               endfile status after
//                                               calls to read_packet()
//  Joel Gales     FutureTech     10/20/16 0.996 Add support for JPSS1
//  Joel Gales     FutureTech     10/21/16 0.997 If npkts == 1 also check for
//                                               EOF.  Check platform when
//                                               writing DNB to determine
//                                               number of fields
//  Joel Gales     FutureTech     11/09/16 0.998 Fix handling of checksum
//                                               errors, Flag bad mband and
//                                               iband with -999, return
//                                               status from unpack_sci().
//  Joel Gales     FutureTech     12/07/16 1.900 Add support for platform
//                                               specific EV_APIDs, Add
//                                               rounding correction to
//                                               SC_records computation.
//  Joel Gales     FutureTech     02/02/17 1.910 Fix quality flag bug if early
//                                               return in unpack_sci()
//  Joel Gales     FutureTech     02/17/17 1.920 Clear enddata if apid != 826
//  Joel Gales     FutureTech     02/23/17 1.930 Set engineering quantities
//                                               to flag values in engineering
//                                               buffer is empty (apid != 826)
//  Joel Gales     FutureTech     03/10/17 1.940 Initialize first,last to -1.
//                                               Check before getting orbit
//                                               direction in write_diary()
//  Joel Gales     FutureTech     04/21/17 1.950 Trap PDS granules with
//                                               times incompatible with
//                                               expected scan periods.
//  Joel Gales     FutureTech     05/02/17 1.96  Fix bug when mper=0
//                                               (stime was being overwritten)
//  Joel Gales     FutureTech     05/25/17 1.97  Initialize sdir/edir to
//                                               "Undetermined"
//  Joel Gales     FutureTech     08/03/17 1.98  Check reconstructed band flag
//                                               values in unpack_viirs_scan
//  Joel Gales     FutureTech     11/28/17 1.99  Skip short period adc/bus
//                                               records in write_adcs_bus()
//  Joel Gales     FutureTech     12/13/17 2.00  Add support for J1 orbitfile
//  Joel Gales     FutureTech     12/20/17 2.01  Check for M-band data in DNB
//                                               packet (from F.Patt)
//  Joel Gales     FutureTech     12/22/17 2.02  Fix packet loop limit in
//                                               scan_complete
//  Joel Gales     FutureTech     01/10/18 2.03  Check for SC_records_val before
//                                               writing adcs_bus fields
//  Joel Gales     FutureTech     01/11/18 2.04  Initialize mbands, ibands,
//                                               dnb arrays to -999
//  Joel Gales     FutureTech     01/25/18 2.05  Check if npkts exceeds 513
//                                               in read_viirs_scan_packets()
//  Sean Bailey    NASA           06/27/18 2.06  Incorporated minor changes made by the LSIPS team
//                                               variable initializations; dead code deletion;
//                                               delete->delete[] where appropriate, 
//                                               i.e. no functional changes :)
#define MAX_PKTS_PER_SCAN 513
      
int main (int argc, char* argv[])
{

  cout << "l1agen_viirs " << VERSION << " (" 
       <<  __DATE__ << " " << __TIME__ << ")" << endl;

  if ( argc == 1) {
    cout << endl << 
      "l1agen_viirs VIIRS_packet_file S/C_diary_packet_file " <<
      "ADCS_file bus_telemetry_file granule_len [-o output_list_file] " <<
      "[--outlist output_list_file]" 
	 << endl;
    return 0;
  }

  int c;
  string outlist = "";
  while (1) {
    static struct option long_options[] = {
      {"outlist",  required_argument, 0, 'o'},
      {0, 0, 0, 0}
    };

    /* getopt_long stores the option index here. */
    int option_index = 0;
    
    c = getopt_long( argc, argv, "o:", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c)
      {
      case 'o':
        //        printf ("option -o with value `%s'\n", optarg);
        outlist.assign( optarg);
        break;

      default:
        abort ();
      }
  }

  ofstream fout;
  if ( outlist.compare("") != 0)
    fout.open( outlist.c_str());

  fstream vfileStream, sfileStream, afileStream, bfileStream;

  // VIIRS packet file
  vfileStream.open( argv[optind+0], fstream::in | fstream::binary);
  if ( vfileStream.fail()) {
    cout << argv[optind+0] << " not found" << endl;
    exit(1);
  }

  // S/C diary file
  sfileStream.open( argv[optind+1], fstream::in | fstream::binary);
  if ( sfileStream.fail()) {
    cout << argv[optind+1] << " not found" << endl;
    exit(1);
  }

  // Get S/C diary filesize
  sfileStream.seekg (0, ios::end);
  int sFileSize = sfileStream.tellg();
  sfileStream.seekg (0, ios::beg);
  int32_t const nscd = sFileSize/71;
 
  // Allocate continuous 2-dim scdpkts array
  uint8_t **scdpkts = new uint8_t*[nscd];
  scdpkts[0] = new uint8_t[71*nscd];
  for (int i=1; i<nscd; i++) scdpkts[i] = scdpkts[i-1] + 71; 

  // Read entire S/C diary file into buffer
  sfileStream.read( (char *) &scdpkts[0][0], sFileSize);
  sfileStream.close();

  // Determine platform
  string platform;
  int apktsize;
  int bpktsize;
  int EV_APIDs;
  float scan_time;
  int adoffsets[4];
  switch (scdpkts[0][14]) {
      case 157:
        platform.assign("SNPP");
	apktsize = 355;
	bpktsize = 207;
        EV_APIDs = 24;
	scan_time = 1.7793;
	adoffsets[0] = 169;
	adoffsets[1] = 127;
	adoffsets[2] = 255;
	adoffsets[3] = 339; 
        break;

      case 159:
        platform.assign("JPSS-1");
	apktsize = 393;
	bpktsize = 183;
        EV_APIDs = 26;
	scan_time = 1.7864;
	adoffsets[0] = 81;
	adoffsets[1] = 131;
	adoffsets[2] = 96;
	adoffsets[3] = 163; 
        break;	

      default:
	cout << "Unknown platform type" << endl;
        exit(1);
  }
  
  // Allocate orbit and attitude time arrays
  double *otime = new double[nscd];
  double *atime = new double[nscd];

  // Allocate continuous 2-dim orbit vector array
  float **orb = new float *[nscd];
  orb[0] = new float[6*nscd];
  for (int i=1; i<nscd; i++) orb[i] = orb[i-1] + 6; 

  // Allocate continuous 2-dim quaternion array
  float **quat = new float *[nscd];
  quat[0] = new float[4*nscd];
  for (int i=1; i<nscd; i++) quat[i] = quat[i-1] + 4; 

  int32_t iyrsc, idysc;

  convert_diary( nscd, (uint8_t (*)[71]) &scdpkts[0][0], &iyrsc, &idysc, 
                 otime, (float (*)[6]) &orb[0][0], 
                 atime, (float (*)[4]) &quat[0][0]);

  // ADCS data
  afileStream.open( argv[optind+2], fstream::in | fstream::binary);
  if ( afileStream.fail()) {
    cout << argv[optind+2] << " not found" << endl;
    exit(1);
  }

  // Get ADCS filesize
  afileStream.seekg (0, ios::end);
  int aFileSize = afileStream.tellg();
  afileStream.seekg (0, ios::beg);
  int32_t nadc = aFileSize/apktsize;
 
  // Allocate continuous 2-dim adcpkts array
  uint8_t **adcpkts = new uint8_t*[nadc];

  adcpkts[0] = new uint8_t[apktsize*nadc];
  for (int i=1; i<nadc; i++) adcpkts[i] = adcpkts[i-1] + apktsize; 

  // Read entire ADCD file into buffer
  afileStream.read( (char *) &adcpkts[0][0], aFileSize);
  afileStream.close();


  // Bus-critical telemetry
  bfileStream.open( argv[optind+3], fstream::in | fstream::binary);
  if ( bfileStream.fail()) {
    cout << argv[optind+3] << " not found" << endl;
    exit(1);
  }

  // Get Bus-critical telemetry filesize
  bfileStream.seekg (0, ios::end);
  int bFileSize = bfileStream.tellg();
  bfileStream.seekg (0, ios::beg);
  int32_t nbus = bFileSize/bpktsize;
 
  // Allocate continuous 2-dim buspkts array
  uint8_t **buspkts = new uint8_t*[nbus];
  buspkts[0] = new uint8_t[bpktsize*nbus];
  for (int i=1; i<nbus; i++) buspkts[i] = buspkts[i-1] + bpktsize; 

  // Read entire Bus-critical telemetry file into buffer
  bfileStream.read( (char *) &buspkts[0][0], bFileSize);
  bfileStream.close();


  int32_t iyrad, idyad;

  // Allocate orbit and attitude time arrays
  double *adctime = new double[nadc];
  double *bustime = new double[nbus];
  uint8_t *adstate = new uint8_t[nbus];
  uint8_t *adsolution = new uint8_t[nbus];
  uint8_t *admandone = new uint8_t[nadc];
  int16_t *adfftid = new int16_t[nadc];

  //  extract_adcs_bus( nadc, (uint8_t (*)[355]) &adcpkts[0][0],
  //                nbus, (uint8_t (*)[207]) &buspkts[0][0],
  //                &iyrad, &idyad, adctime, bustime,
  //                  adstate, admandone, adfftid, adsolution);

  extract_adcs_bus( adoffsets, nadc, &adcpkts[0], nbus, &buspkts[0],
		    &iyrad, &idyad, adctime, bustime,
		    adstate, admandone, adfftid, adsolution);

  int32_t apid = 0;
  int32_t len = 0;
  int32_t endfile;
  uint8_t epacket[9318];

  // Read to the first engineering packet of a scan (APID = 826)
  while (1) {

    read_packet( &vfileStream, NULL, &len, &apid, &endfile);
    if ( endfile) {
      cout << "Start of scan not found in file" << endl;
      exit(1);
    }

    if ( apid == 826 && len == 9318) {
      vfileStream.seekg( -6, ios_base::cur);
      read_packet( &vfileStream, epacket, &len, NULL, &endfile);
      break;
    } else {
      vfileStream.seekg( len-6, ios_base::cur);
    }
  }

  int vfilePos = vfileStream.tellg();

  // Determine start and end time of first granule 
  int32_t toff, iyear, iday, iyr, idy;

  double stime, ptime, etime;

  // Scan start time
  toff = 6;
  ccsds_to_yds( &epacket[toff], &iyear, &iday, &stime);

  // Packet time
  toff = 20;
  ccsds_to_yds( &epacket[toff], &iyr, &idy, &ptime);

  // Scan end time
  toff = 38;
  ccsds_to_yds( &epacket[toff], &iyr, &idy, &etime);

  // Get granule period in minutes
  int32_t mper;
  string str = argv[optind+4];
  istringstream( str) >> mper;

  // Allocate continuous 2-dim pbuffer array
  uint8_t **pbuffer = new uint8_t *[MAX_PKTS_PER_SCAN];
  pbuffer[0] = new uint8_t[MAX_PKTS_PER_SCAN*PBUFFER_SIZE];
  for (int i=1; i<MAX_PKTS_PER_SCAN; i++) {
    pbuffer[i] = pbuffer[i-1] + PBUFFER_SIZE;
  }

  
  // Determine number of scans if granule period not specified
  if ( mper == 0) {
    cout << "Determining number of scans in " << argv[optind+0] << ": ";
    cout << flush;

    uint8_t epacket0[9318];
    memcpy ( epacket0, epacket, 9318);

    // Read VIIRS scans
    int32_t isc = 0;
    while ( !endfile) {
      //      if ((isc % 10) == 0) cout << "Processing scan " << isc << endl;

      // Read packets for that scan
      int32_t npkts;
      read_viirs_scan_packets( &vfileStream, epacket0, 
                               (uint8_t (*)[PBUFFER_SIZE]) &pbuffer[0][0],
                               &npkts, &endfile);

      // Get scan time from next engineering packet
      double tmptime;
      toff = 6;
      ccsds_to_yds( &epacket0[toff], &iyr, &idy, &tmptime);
      //      cout << "isc: " << isc << " time: " << tmptime << endl;

      if ( npkts == 1) continue;

      isc++;
    }
//    mper = isc * (scan_time/60) + 3;
    mper = isc * (scan_time/60) + 1;
    cout << isc << endl;
    vfileStream.clear();
    vfileStream.seekg( vfilePos, ios::beg);
    endfile = 0;
  }
  //  cout << vfileStream.tellg() << endl;

  // Find closest previous mper-min time for starttime
  int32_t ltime = (((int32_t) (stime)) / 60 / mper) * (mper*60);
//  if (mper > 10) ltime = (((int32_t) (stime)) / 60 / 5) * (5*60);
  if (mper > 10) ltime = (int32_t) (stime);
  int32_t mtime = ltime + mper*60;
  int32_t ih = (int32_t) (ltime / 3600);
  int32_t mn = (int32_t) ((ltime - ih*3600) / 60);
  int32_t isec = (int32_t) (ltime - ih*3600 - mn*60);

  stringstream timestr, datestr;
  timestr << setfill('0') << setw(2) << ih
          << setfill('0') << setw(2) << mn
          << setfill('0') << setw(2) << isec;

  datestr << setfill('0') << setw(4) << iyear
          << setfill('0') << setw(3) << iday;

  
  //  cout << timestr.str() << endl;
  //cout << datestr.str() << endl;


  // Generate L1A file name
//  string l1a_name = string("V") + datestr.str() + timestr.str() +
//    ".L1A_" + platform + ".nc";
  string l1a_name = string("V") + datestr.str() + timestr.str() + ".L1A_";
  if ( platform.compare("SNPP") == 0) {
    l1a_name += "SNPP.nc";
  } else if ( platform.compare("JPSS-1") == 0) {
    l1a_name += "JPSS1.nc";
  }

  cout << l1a_name.c_str() << endl;

  static l1aFile outfile;
  outfile.platform.assign(platform);
  outfile.apktsize = apktsize;
  outfile.bpktsize = bpktsize;
  outfile.EV_APIDs = EV_APIDs;
  
  int32_t maxsc = mper*60/scan_time + 1;
  cout << "maxsc: " << maxsc << endl;

  //  Initialize data arrays

  // M-band cal data
  int16_t sdm[17][16][48], svm[17][16][48], bbm[17][16][48]; 

  // I-band cal data
  int16_t sdi[5][32][96], svi[5][32][96], bbi[5][32][96]; 

  // DNB cal data
  int16_t sdd[16][64], svd[16][64], bbd[16][64]; 


  // Allocate continuous 2-dim engdata array
  uint8_t **engdata = new uint8_t *[maxsc];
  engdata[0] = new uint8_t[9318*maxsc];
  for (int i=1; i<maxsc; i++) engdata[i] = engdata[i-1] + 9318;

  // Allocate continuous 2-dim p1 array
  uint8_t **p1 = new uint8_t *[maxsc];
  p1[0] = new uint8_t[180*maxsc];
  for (int i=1; i<maxsc; i++) p1[i] = p1[i-1] + 180;

  // Allocate continuous 2-dim HR metadata array
  uint8_t **hrmets = new uint8_t *[maxsc];
  hrmets[0] = new uint8_t[146*EV_APIDs*maxsc];
  for (int i=1; i<maxsc; i++) hrmets[i] = hrmets[i-1] + 146*EV_APIDs;

  // Allocate single-scan HR metadata array
  uint8_t **hrmet;
  hrmet = new uint8_t *[EV_APIDs];
  hrmet[0] = new uint8_t[146*EV_APIDs];
  for ( int i=1; i<EV_APIDs; i++) hrmet[i] = hrmet[i-1] + 146;

  // Allocate continuous 2-dim CAL metadata array
  uint8_t **calmets = new uint8_t *[maxsc];
  calmets[0] = new uint8_t[134*maxsc];
  for (int i=1; i<maxsc; i++) calmets[i] = calmets[i-1] + 134;

  // Allocate continuous 2-dim SD M cal arrays
  int16_t **sd_m = new int16_t *[17*maxsc];
  sd_m[0] = new int16_t[17*maxsc*48*16];
  for (int i=1; i<17*maxsc; i++) {
    sd_m[i] = sd_m[i-1] + 48*16;
    for (int j=0; j<48*16; j++) sd_m[i][j] = -999;
  }

  // Allocate continuous 2-dim SV M cal arrays
  int16_t **sv_m = new int16_t *[17*maxsc];
  sv_m[0] = new int16_t[17*maxsc*48*16];
  for (int i=1; i<17*maxsc; i++) {
    sv_m[i] = sv_m[i-1] + 48*16;
    for (int j=0; j<48*16; j++) sv_m[i][j] = -999;
  }

  // Allocate continuous 2-dim BB M cal arrays
  int16_t **bb_m = new int16_t *[17*maxsc];
  bb_m[0] = new int16_t[17*maxsc*48*16];
  for (int i=1; i<17*maxsc; i++) {
    bb_m[i] = bb_m[i-1] + 48*16;
    for (int j=0; j<48*16; j++) bb_m[i][j] = -999;
  }

  // Allocate continuous 2-dim SD I cal arrays
  int16_t **sd_i = new int16_t *[5*maxsc];
  sd_i[0] = new int16_t[5*maxsc*96*32];
  for (int i=1; i<5*maxsc; i++) {
    sd_i[i] = sd_i[i-1] + 96*32;
    for (int j=0; j<96*32; j++) sd_i[i][j] = -999;
  }

  // Allocate continuous 2-dim SV I cal arrays
  int16_t **sv_i = new int16_t *[5*maxsc];
  sv_i[0] = new int16_t[5*maxsc*96*32];
  for (int i=1; i<5*maxsc; i++) {
    sv_i[i] = sv_i[i-1] + 96*32;
    for (int j=0; j<96*32; j++) sv_i[i][j] = -999;
  }

  // Allocate continuous 2-dim BB I cal arrays
  int16_t **bb_i = new int16_t *[5*maxsc];
  bb_i[0] = new int16_t[5*maxsc*96*32];
  for (int i=1; i<5*maxsc; i++) {
    bb_i[i] = bb_i[i-1] + 96*32;
    for (int j=0; j<96*32; j++) bb_i[i][j] = -999;
  }

  // Allocate continuous 2-dim SD DNB cal arrays
  int16_t **sd_d = new int16_t *[maxsc];
  sd_d[0] = new int16_t[maxsc*64*16];
  for (int i=1; i<maxsc; i++) {
    sd_d[i] = sd_d[i-1] + 64*16;
    for (int j=0; j<64*16; j++) sd_d[i][j] = -999;
  }

  // Allocate continuous 2-dim SV DNB cal arrays
  int16_t **sv_d = new int16_t *[maxsc];
  sv_d[0] = new int16_t[maxsc*64*16];
  for (int i=1; i<maxsc; i++) {
    sv_d[i] = sv_d[i-1] + 64*16;
    for (int j=0; j<64*16; j++) sv_d[i][j] = -999;
  }

  // Allocate continuous 2-dim BB DNB cal arrays
  int16_t **bb_d = new int16_t *[maxsc];
  bb_d[0] = new int16_t[maxsc*64*16];
  for (int i=1; i<maxsc; i++) {
    bb_d[i] = bb_d[i-1] + 64*16;
    for (int j=0; j<64*16; j++) bb_d[i][j] = -999;
  }

  typedef uint16_t mbands_array[16][6304];
  typedef uint16_t ibands_array[32][6400];
  typedef uint16_t dnb_array[16][4064]; 

  mbands_array *mbands;
  mbands = new mbands_array[16];
  for (int i=0; i<16; i++)
    for (int j=0; j<16; j++)
      for (int k=0; k<6304; k++) 
        mbands[i][j][k] = -999;

  ibands_array *ibands;
  ibands = new ibands_array[5];
  for (int i=0; i<5; i++)
    for (int j=0; j<32; j++)
      for (int k=0; k<6400; k++) 
        ibands[i][j][k] = -999;

  dnb_array *dnb;
  dnb = new dnb_array[5];
  for (int i=0; i<5; i++)
    for (int j=0; j<16; j++)
      for (int k=0; k<4064; k++) 
        dnb[i][j][k] = -999;


  // Load orbit times
  uint32_t orbitStartYearDay[128];
  double orbitStartSecOfDay[128];
  double orbitPeriods[128];
  uint32_t orbitNumbers[128];
  ifstream orbitStartTimesFile;

  char *ocvarroot_str = getenv("OCVARROOT");
  if (ocvarroot_str == 0x0) {
    printf("Environment variable OCVARROOT not defined.\n");
    exit(1);
  }

  string orbitStartTimesFilename = ocvarroot_str;
  if ( platform.compare("SNPP") == 0) {
    orbitStartTimesFilename += "/viirsn/viirs_orbit_times.txt";
  } else if ( platform.compare("JPSS-1") == 0) {
    orbitStartTimesFilename += "/viirsj1/viirs_j1_orbit_times.txt";
  }

  orbitStartTimesFile.open( orbitStartTimesFilename.c_str());
  if ( !orbitStartTimesFile.is_open()) {
    cout << orbitStartTimesFilename.c_str() << " not found." << endl;
    exit(1);
  }
  for (size_t i=0; i<128; i++) {
    orbitStartTimesFile >> orbitNumbers[i];
    orbitStartTimesFile >> orbitStartYearDay[i];
    orbitStartTimesFile >> orbitStartSecOfDay[i];
    orbitStartTimesFile >> orbitPeriods[i];
    if (  orbitNumbers[i] == 0) break;
  }
  orbitStartTimesFile.close();


  int status;
  int iret;

  bool exit_101 = false;

  ////////////////////// Main Loop ///////////////////
  while (!endfile) {
    outfile.createl1( (char *) l1a_name.c_str(), maxsc);

    iret = 0;

    // Read and process VIIRS scans
    int32_t isc = 0;
    while ( stime < mtime) {

      if ( isc == maxsc) {
        cout << "Bad granule -- Times incompatible with scan period." << endl;
        exit(101);
      }

      if ((isc % 10) == 0) cout << "Processing scan " << isc << endl;
      //      cout << "Processing scan " << isc << " " << stime << endl;

      // Save engineering packet in array
      apid = (epacket[0] % 8)*256 + epacket[1];
      if (apid == 826) 
        memcpy( engdata[isc], epacket, 9318); 
      else
        memset( engdata[isc], 0, 9318);

      // Read packets for that scan
      int32_t npkts;
      read_viirs_scan_packets( &vfileStream, epacket, 
                               (uint8_t (*)[PBUFFER_SIZE]) &pbuffer[0][0],
                               &npkts, &endfile);

      // if npkts == 1 then bad scan
      if ( npkts == 1 && endfile == 0) {
        exit_101 = true;
        continue;
      }

      // Check for complete scan
      int icmp = scan_complete( (uint8_t (*)[PBUFFER_SIZE]) &pbuffer[0][0],
                                npkts);

      iret = iret | icmp;

      int32_t ip = 1;
      while( pbuffer[ip][0] < 8) ip++;
      memcpy( &p1[isc][0], &pbuffer[ip][0], 180);

      //uint8_t hrmet[26][146];
      uint8_t mqfl[16][16];
      uint8_t iqfl[5][32];
      uint8_t dqfl[5][16];

      for ( int i=0; i<EV_APIDs; i++) memset( &hrmet[i][0], 0, 146);
      //      memset( &hrmet, 0, 26*146);
      memset( &mqfl, 127, 16*16);
      memset( &iqfl, 127, 5*32);
      memset( &dqfl, 127, 3*16);

      // Unpack science data from packets
      status = unpack_viirs_scan( npkts,
                                  (uint8_t (*)[PBUFFER_SIZE]) &pbuffer[0][0],
                                  (uint16_t (*) [16][6304]) &mbands[0][0][0],
                                  (uint16_t (*) [32][6400]) &ibands[0][0][0],
                                  (uint16_t (*) [16][4064]) &dnb[0][0][0],
                                  (uint8_t (*) [16]) &mqfl[0][0],
                                  (uint8_t (*) [32]) &iqfl[0][0],
                                  (uint8_t (*) [16]) &dqfl[0][0],
                                  (uint8_t (*)[146]) &hrmet[0][0]);
      iret = iret | status;

      // Write science data to file
      outfile.write_science_data( platform, isc,
                                  (uint16_t (*) [16][6304]) &mbands[0][0][0],
                                  (uint16_t (*) [32][6400]) &ibands[0][0][0],
                                  (uint16_t (*) [16][4064]) &dnb[0][0][0],
                                  (uint8_t (*) [16]) &mqfl[0][0],
                                  (uint8_t (*) [32]) &iqfl[0][0],
                                  (uint8_t (*) [16]) &dqfl[0][0]);
        
      // Unpack science data from packets
      uint8_t calmet[134];
      status = unpack_viirs_cal( npkts,
                                 (uint8_t (*)[PBUFFER_SIZE]) &pbuffer[0][0],
                                 (int16_t (*) [16][48]) &sdm[0][0][0],
                                 (int16_t (*) [16][48]) &svm[0][0][0],
                                 (int16_t (*) [16][48]) &bbm[0][0][0],
                                 (int16_t (*) [32][96]) &sdi[0][0][0],
                                 (int16_t (*) [32][96]) &svi[0][0][0],
                                 (int16_t (*) [32][96]) &bbi[0][0][0],
                                 (int16_t (*) [64]) &sdd[0][0],
                                 (int16_t (*) [64]) &svd[0][0],
                                 (int16_t (*) [64]) &bbd[0][0],
                                 calmet);
      iret = iret | status;

      for (int i=0; i<17; i++) {
        //       cout << "i: " << i << "  isc: " << isc << endl;
        memcpy( &sd_m[i*maxsc+isc][0], &sdm[i][0][0], 48*16*sizeof(int16_t));
        memcpy( &sv_m[i*maxsc+isc][0], &svm[i][0][0], 48*16*sizeof(int16_t));
        memcpy( &bb_m[i*maxsc+isc][0], &bbm[i][0][0], 48*16*sizeof(int16_t));
      }

      for (int i=0; i<5; i++) {
        memcpy( &sd_i[i*maxsc+isc][0], &sdi[i][0][0], 96*32*sizeof(int16_t));
        memcpy( &sv_i[i*maxsc+isc][0], &svi[i][0][0], 96*32*sizeof(int16_t));
        memcpy( &bb_i[i*maxsc+isc][0], &bbi[i][0][0], 96*32*sizeof(int16_t));
      }

      memcpy( &sd_d[isc][0], &sdd[0][0], 64*16*sizeof(int16_t));
      memcpy( &sv_d[isc][0], &svd[0][0], 64*16*sizeof(int16_t));
      memcpy( &bb_d[isc][0], &bbd[0][0], 64*16*sizeof(int16_t));

      // Save HR and cal metadata in array
      memcpy( &hrmets[isc][0], &hrmet[0][0], 146*EV_APIDs*sizeof(uint8_t));
      //printf("3b sd_m[0]: %p\n", sd_m[0]);
      memcpy( &calmets[isc][0], &calmet[0], 134*sizeof(uint8_t));


      // Bail if end of L0 input file
      if ( endfile) isc++;
      if ( endfile) break;

      double prev_stime = stime;

      // Get scan time from next engineering packet
      toff = 6;
      ccsds_to_yds( &epacket[toff], &iyr, &idy, &stime);
      //      cout << "isc: " << isc << " stime: " << stime << endl;

      if (stime == prev_stime) {
        cout << "stime: " << stime << " equal to previous stime: " <<
          prev_stime << endl;
        stime = prev_stime;
        continue;
      }

      if (iday == idy && stime < prev_stime) {
        cout << "stime: " << stime << " less than previous stime: " <<
          prev_stime << " for same day" << endl;
        stime = prev_stime;
        continue;
      }

      if (idy < iday) idy = iday + 1;
      stime = stime + (idy-iday)*86400;

      isc++;

    } // while ( (stime < mtime) && (!endfile))

    cout << "Number of scans to write: " << isc << endl;

    // If not scans in file then break
    //    if ( isc == 0)
    // break;

    // Set remaining cal field entries to fill (-999)
    int16_t minus999 = -999;
    for (int k=0; k<17; k++) {
      for (int j=isc; j<maxsc; j++) {
        for (int i=0; i<48*16; i++) {
          memcpy( &sd_m[k*maxsc+j][i], &minus999, sizeof(int16_t));
          memcpy( &sv_m[k*maxsc+j][i], &minus999, sizeof(int16_t));
          memcpy( &bb_m[k*maxsc+j][i], &minus999, sizeof(int16_t));
        }
      }
    }

    for (int k=0; k<5; k++) {
      for (int j=isc; j<maxsc; j++) {
        for (int i=0; i<96*32; i++) {
          memcpy( &sd_i[k*maxsc+j][i], &minus999, sizeof(int16_t));
          memcpy( &sv_i[k*maxsc+j][i], &minus999, sizeof(int16_t));
          memcpy( &bb_i[k*maxsc+j][i], &minus999, sizeof(int16_t));
        }
      }
    }

    for (int j=isc; j<maxsc; j++) {
      for (int i=0; i<64*16; i++) {
        memcpy( &sd_d[j][i], &minus999, sizeof(int16_t));
        memcpy( &sv_d[j][i], &minus999, sizeof(int16_t));
        memcpy( &bb_d[j][i], &minus999, sizeof(int16_t));
      }
    }


    // Get scan metadata and write to file
    uint8_t *mode = new uint8_t[isc];

    int mn_scn = (int) ((mper*60) / scan_time);
    int mx_scn = mn_scn + 1;
    if (isc != mn_scn && isc != mx_scn) iret = iret | 1;

    outfile.write_scan_metadata( isc, 
                                 (uint8_t (*)[180]) &p1[0][0],
                                 (uint8_t (*)[146*26]) &hrmets[0][0],
                                 (uint8_t (*)[134]) &calmets[0][0],
                                 mode, iret, (const char *) l1a_name.c_str(),
                                 argv[optind+0]);

    // Unpack engineering data and write to file
    outfile.write_eng_data( isc, (uint8_t (*)[9318]) &engdata[0][0]); 

    // Write calibration data to file
    outfile.write_cal_data( isc,
                            (int16_t (*) [48*16]) &sd_m[0][0],
                            (int16_t (*) [48*16]) &sv_m[0][0],
                            (int16_t (*) [48*16]) &bb_m[0][0],
                            (int16_t (*) [96*32]) &sd_i[0][0],
                            (int16_t (*) [96*32]) &sv_i[0][0],
                            (int16_t (*) [96*32]) &bb_i[0][0],
                            (int16_t (*) [64*16]) &sd_d[0][0],
                            (int16_t (*) [64*16]) &sv_d[0][0],
                            (int16_t (*) [64*16]) &bb_d[0][0]);


    // Locate S/C diary data and write to file
    char sdir[16], edir[16];
    strcpy(sdir, "Undetermined");
    strcpy(edir, "Undetermined");
    outfile.write_diary( iyear, iday, ltime, mtime, iyrsc, idysc, nscd,
                         otime, (float (*)[6]) &orb[0][0], 
                         atime, (float (*)[4]) &quat[0][0],
                         sdir, edir);

    outfile.write_adcs_bus( iyear, iday, ltime, mtime, iyrad, idyad,
                            nadc, nbus, adctime, admandone, adfftid,
                            bustime, adstate, adsolution,
                            &adcpkts[0], &buspkts[0]); 

    // Compute orbit number

    int32_t orbit = -1;
    uint32_t orbitStartYearDay0 = iyear*1000+iday;

    double time_tai, dbl_ltime;
    dbl_ltime = (double) ltime;
    time_tai = yds2tai93( iyear, iday, dbl_ltime);

    for (size_t i=0; i<128; i++) {

      if ( orbitStartYearDay[i] > orbitStartYearDay0 ||
           (orbitStartYearDay[i] == orbitStartYearDay0 &&
            orbitStartSecOfDay[i] > ltime)) {
        i--;

        double tai;
        uint32_t iyr = (uint32_t) orbitStartYearDay[i]/1000;
        uint32_t idy =  orbitStartYearDay[i] - iyr*1000;
        tai = yds2tai93( iyr, idy, orbitStartSecOfDay[i]);

        orbit = uint32_t ((time_tai - tai) / orbitPeriods[i]) +
          orbitNumbers[i];

        break;
      }
    }
    
    // Generate granule metadata and write to file
    outfile.write_granule_metadata( iyear, iday,
                                    ltime, mtime, orbit,
                                    (const char *) l1a_name.c_str(), 
                                    sdir, edir, 
                                    (uint8_t *) p1[0],
                                    isc, mode, argc, argv);

    delete mode;

    outfile.close();

    // Remove 0-scan file
    if ( isc == 0) {
      int status = remove( l1a_name.c_str());
      if( status == 0) {
        cout << "Removing 0-scan file: " << l1a_name.c_str() << endl;
      } else {
        cout << "Error removing " << l1a_name.c_str() << endl;
        exit(1);
      }
    }

    // If not 0-scan file then list in outlist
    if ( isc > 0) {
      fout << l1a_name.c_str();

      if (isc == mn_scn || isc == mx_scn)
        fout << " 1" << endl;
      else
        fout << " 0" << endl;
    }

    // If not end of file, compute start and times for next granule 
    // and generate file name
    if ( !endfile) {
      ltime = mtime;
      if ( ltime >= 86400) {
        ltime = ltime - 86400;
        stime = stime - 86400;
        iday = iday + 1;

        if (isleap(iyear)) {
          if (iday == 367) {
            iyear++;
            iday = 1;
          }
        } else {
          if (iday == 366) {
            iyear++;
            iday = 1;
          }
        }
      }

      mtime = ltime + mper*60;
      ih = (int32_t) (ltime / 3600);
      mn = (int32_t) ((ltime - ih*3600) / 60);
      isec = (int32_t) (ltime - ih*3600 - mn*60);

      timestr.str("");
      timestr << setfill('0') << setw(2) << ih
              << setfill('0') << setw(2) << mn
              << setfill('0') << setw(2) << isec;

      datestr.str("");
      datestr << setfill('0') << setw(4) << iyear
              << setfill('0') << setw(3) << iday;

      //    cout << timestr.str() << endl;
      //cout << datestr.str() << endl;


      if ( platform.compare("SNPP") == 0) {
        l1a_name.assign(string("V") + datestr.str() + timestr.str() +
                      ".L1A_SNPP.nc");
      } else if ( platform.compare("JPSS-1") == 0) {
        l1a_name.assign(string("V") + datestr.str() + timestr.str() +
                      ".L1A_JPSS1.nc");
      }
      cout << endl;
      cout << l1a_name.c_str() << endl;
    }

  } // while (!endfile)
  /////////////////// End Main Loop ///////////////////

  if ( outlist.compare("") != 0) fout.close();

  vfileStream.close();

  //Deallocate
  delete[] scdpkts[0];
  delete[] scdpkts;

  delete[] otime;
  delete[] atime;

  delete[] orb[0];
  delete[] orb;

  delete[] quat[0];
  delete[] quat;

  delete[] engdata[0];
  delete[] engdata;

  delete[] p1[0];
  delete[] p1;

  delete[] hrmets[0];
  delete[] hrmets;

  delete[] hrmet[0];
  delete[] hrmet;

  delete[] calmets[0];
  delete[] calmets;

  delete[] sd_m[0];
  delete[] sd_m;

  delete[] sv_m[0];
  delete[] sv_m;

  delete[] bb_m[0];
  delete[] bb_m;

  delete[] sd_i[0];
  delete[] sd_i;

  delete[] sv_i[0];
  delete[] sv_i;

  delete[] bb_i[0];
  delete[] bb_i;

  delete[] sd_d[0];
  delete[] sd_d;

  delete[] sv_d[0];
  delete[] sv_d;

  delete[] bb_d[0];
  delete[] bb_d;

  delete[] adcpkts[0];
  delete[] adcpkts;

  delete[] buspkts[0];
  delete[] buspkts;

  delete[] adctime;
  delete[] bustime;

  delete[] adstate;
  delete[] admandone;
  delete[] adfftid;

  delete[] mbands;
  delete[] ibands;
  delete[] dnb;

  delete[] pbuffer[0];
  delete[] pbuffer;

  if (exit_101) return 101; else return 0;
}

int convert_diary( int32_t npkts, uint8_t (*dstore)[71], 
                   int32_t *iyr, int32_t *iday, 
                   double *otime, float (*orb)[6], 
                   double *atime, float (*quat)[4]) {

  uint16_t ui16;
  uint32_t ui32;

  int io=0, ia=0;
  double ot, at;
  
  //Days since 1/1/1958
  memcpy( &ui16, (uint16_t *) &dstore[0][6], 2);
  int32_t jd0 = SWAP_2( ui16) + 2436205;

  jdate( jd0, iyr, iday);

  // Loop through packets
  for (int i=0; i<npkts; i++) {

    int32_t ioff;

    double sec;
    int16_t iy16, idy16;
    int32_t iy, idy, jd;

    // Get orbit time
    ccsds_to_yds( &dstore[i][15], &iy, &idy, &sec);
    iy16 = iy;
    idy16 = idy;
    jd = jday( iy16, 1, idy16);
    ot = sec + (jd - jd0)*86400;
    if ((ot > -10.e0) && (ot < 1728.0e2)) otime[io++]= ot;

    // Convert orbit vectors to floats
    ioff = 23;
    for (int j=0; j<=5; j++) {
      memcpy( &ui32, (uint32_t *) &dstore[i][ioff], 4);
      uint32_t k = SWAP_4( ui32);
      memcpy( &orb[i][j], &k, sizeof(float)); 
      ioff = ioff + 4;
    }
     
    // Get attitude time
    ccsds_to_yds( &dstore[i][47], &iy, &idy, &sec);
    iy16 = iy;
    idy16 = idy;
    jd = jday( iy16, 1, idy16);
    at = sec + (jd - jd0)*86400;
    if ((at > -10.e0) && (at < 1728.0e2)) atime[ia++]= at;
    
    // Convert attitude quaternion to floats
    ioff = 55;
    for (int j=0; j<=3; j++) {
      memcpy( &ui32, (uint32_t *) &dstore[i][ioff], 4);
      uint32_t k = SWAP_4( ui32);
      memcpy( &quat[i][j], &k, sizeof(float)); 
      ioff = ioff + 4;
    }
  }
     
  return 0;
} 


int extract_adcs_bus( int adoffsets[4],
		      int32_t nadc, uint8_t *astore[], 
                      int32_t nbus, uint8_t *bstore[], 
                      int32_t *iyr, int32_t *iday, 
                      double *adctime, double *bustime,
                      uint8_t *adstate, uint8_t *admandone, 
                      int16_t *adfftid, uint8_t *adsolution) {

  uint16_t ui16;

  //Days since 1/1/1958
  memcpy( &ui16, (uint16_t *) &astore[0][6], 2);
  int32_t jd0 = SWAP_2( ui16) + 2436205;

  jdate( jd0, iyr, iday);

  double sec;
  int16_t iy16, idy16;
  int32_t iy, idy, jd;

  // Loop through packets
  for (int i=0; i<nadc; i++) {
    ccsds_to_yds( &astore[i][6], &iy, &idy, &sec);
    iy16 = iy;
    idy16 = idy;
    jd = jday( iy16, 1, idy16);
    adctime[i] = sec + (jd - jd0)*86400;

    //    admandone[i] = (astore[i][108] % 128) / 64;
    admandone[i] = (astore[i][adoffsets[2]] % 2);
    adfftid[i] = astore[i][adoffsets[3]] * 256 + astore[i][adoffsets[3]+1];
  }

  // Loop through packets
  for (int i=0; i<nbus; i++) {
    ccsds_to_yds( &bstore[i][6], &iy, &idy, &sec);
    iy16 = iy;
    idy16 = idy;
    jd = jday( iy16, 1, idy16);
    bustime[i] = sec + (jd - jd0)*86400;

    adstate[i] = (bstore[i][adoffsets[0]] % 32) / 4;
    adsolution[i] = (bstore[i][adoffsets[1]] % 32);
  }

  return 0;
}


int read_viirs_scan_packets( fstream *vfileStream, uint8_t *epacket, 
                             uint8_t (*pbuffer)[PBUFFER_SIZE], 
                             int32_t *npkts, int32_t *endfile) {

  // Get VIIRS scan number and start time from engineering packet
  int32_t iyear, iday;
  double stime;
  ccsds_to_yds( &epacket[6], &iyear, &iday, &stime);
  uint32_t ui32;

  uint32_t scnum;
  if ( epacket[1] == 58) {
    memcpy( &ui32, &epacket[48], 4);
    scnum = SWAP_4( ui32);
  } else {
    memcpy( &ui32, &epacket[34], 4);
    scnum = SWAP_4( ui32);
  }

  // Store the engineering packet in the buffer
  memcpy( &pbuffer[0], epacket, 9317);
  *npkts = 1;

  // Read all of the packets with this start time and scan number 
  // and store in the packet buffer

  // Read the next packet
  int32_t apid, len = 0;

  read_packet( vfileStream, NULL, &len, &apid, endfile);
  if ( *endfile) return 0;
  if ( len > PBUFFER_SIZE) {
    cout << "Packet size " << len << " greater than buffer size: ";
    cout << PBUFFER_SIZE << endl;
    exit(1);
  }
  uint8_t *packet = new uint8_t[len];

  vfileStream->seekg( -6, ios_base::cur);
  read_packet( vfileStream, packet, &len, NULL, endfile);

  // Check for missing first packet of a group
  uint8_t first = (packet[0] & 8) / 8;
  while ( !first) {
    //    memcpy( &pbuffer[*npkts][0], packet, len);
    //    *npkts = *npkts + 1;

    read_packet( vfileStream, NULL, &len, &apid, endfile);
    if ( *endfile) return 0;
    if ( len > PBUFFER_SIZE) {
      cout << "Packet size " << len << " greater than buffer size: ";
      cout << PBUFFER_SIZE << endl;
      exit(1);
    }

    delete[] packet;
    packet = new uint8_t[len];

    vfileStream->seekg( -6, ios_base::cur);
    read_packet( vfileStream, packet, &len, NULL, endfile);

    first = (packet[0] & 8) / 8;
  }

  int32_t iyr, idy;
  double stm;
  memcpy( &ui32, &packet[34], 4);
  uint32_t scn = SWAP_4( ui32);
  ccsds_to_yds( &packet[6], &iyr, &idy, &stm);

  while ( (scn == scnum) && (stm == stime)) {

    // Get APID and number of packets in group and load packet into buffer
    apid = (packet[0] % 8)*256 + packet[1];
    uint8_t npkg = packet[14];
    if(*npkts >= MAX_PKTS_PER_SCAN)
    {
        cout << "L1A Algorithm: Number of packets exceeds in scan"<< MAX_PKTS_PER_SCAN << endl;
	exit(1);
    }
    
    memcpy( &pbuffer[*npkts][0], packet, len);
    (*npkts)++;

    // Read remaining packets in group
    int i = 0;
    while (i <= npkg) {
      int32_t apd;

      read_packet( vfileStream, NULL, &len, &apd, endfile);
      if ( *endfile) return 0;

      if ( len > PBUFFER_SIZE) {
        cout << "Packet size " << len << " greater than buffer size: ";
        cout << PBUFFER_SIZE << endl;
        exit(1);
      }
      delete[] packet;
      packet = new uint8_t[len];

      vfileStream->seekg( -6, ios_base::cur);
      read_packet( vfileStream, packet, &len, NULL, endfile);

      // Check APID; if not a match, missing packets
      if ( apd == apid) {
	if(MAX_PKTS_PER_SCAN <= *npkts)
	{
            cout << "L1A Algorithm: Number of packets exceeds in scan"<< MAX_PKTS_PER_SCAN << endl;
	    exit(1);
	}
        memcpy( &pbuffer[*npkts][0], packet, len);
	//  cout << "scn: " << scn << " i: " << i << " len: " << len <<
	//    " npkts: " << *npkts << endl;
	(*npkts)++;
        i++;
      } else {
        i = npkg + 1;
      }
    }

    first = (packet[0] & 8) / 8;
    while ( !first) {
      //      memcpy( &pbuffer[*npkts][0], packet, len);
      //      *npkts = *npkts + 1;

      read_packet( vfileStream, NULL, &len, &apid, endfile);
      if ( *endfile) return 0;
      if ( len > PBUFFER_SIZE) {
        cout << "Packet size " << len << " greater than buffer size: ";
        cout << PBUFFER_SIZE << endl;
        exit(1);
      }
      delete[] packet;
      packet = new uint8_t[len];

      vfileStream->seekg( -6, ios_base::cur);
      read_packet( vfileStream, packet, &len, NULL, endfile);
      if ( *endfile) return 0;
      first = (packet[0] & 8) / 8;
    }
    memcpy( &ui32, &packet[34], 4);
    scn = SWAP_4( ui32);
    ccsds_to_yds( &packet[6], &iyr, &idy, &stm);
  }

  memcpy( epacket, packet, len);
  delete packet;

  //  if ( len != 9318) memset( &epacket[len], 0, 9318-len);

  return 0;
}


int unpack_viirs_scan( int32_t npkts, 
                       uint8_t (*pbuffer)[PBUFFER_SIZE],
                       uint16_t (*mbands)[16][6304],
                       uint16_t (*ibands)[32][6400],
                       uint16_t (*dnb)[16][4064],
                       uint8_t (*mqfl)[16],
                       uint8_t (*iqfl)[32],
                       uint8_t (*dqfl)[16],
                       uint8_t (*hrmet)[146]) {

  int iret = 0;

  // Set up band index array
  int32_t ibnds[] = {4, 5, 3, 2, 1, 6, 7, 9, 10, 8, 11, 13, 12, 34, 
                     16, 15, 14, 35, 31, 32, 33, 21, 22, 23, 24, 25};

  int32_t ipkt = 0;
  while (ipkt < npkts) {

    int32_t apid = (pbuffer[ipkt][0] % 8)*256 + pbuffer[ipkt][1];

    // Check for non-engineering packet
    if (apid != 825 && apid != 826) {

      // Look for header packet
      uint8_t jhd = (pbuffer[ipkt][0] & 8) / 8;
      if (jhd) {

        // Determine band number
        int32_t iap = apid - 800;
	if (iap > 23) iap = iap - 3;
        
        int32_t ibnd = ibnds[iap];
        //;         print,apid,ibnd
         
        // Determine scan length
        int32_t nsamp = 3200;
        if (ibnd <= 5 || ibnd == 7 || ibnd == 13) nsamp = 6304;
        if (ibnd >= 21 && ibnd <= 25) nsamp = 4064;
        if (ibnd > 30) nsamp = 6400;

        // Extract HR metadata
        memcpy( &hrmet[iap][0], &pbuffer[ipkt][32], 146);
          
        // Get number of remaining packets in group
        int32_t ngpk = pbuffer[ipkt][14];
        //        int32_t lpkt = ipkt + ngpk
        ipkt++;
        int32_t idet = -1;

        // Check for M-band data in DNB packet (12/20/17 from F.Patt)
        if (ibnd >= 22 && ibnd <= 25) {
          if (pbuffer[ipkt][96] == 0 && pbuffer[ipkt][97] == 8) {
            idet = ngpk;
            cout << "M-band data in DNB packets" << endl;
          }
        }
        
        // Look through remaining packets and unpack science data
        while (idet < ngpk && ipkt < npkts) {

          // Verify same APID and extract science data
          int32_t apin =  (pbuffer[ipkt][0] % 8)*256 + pbuffer[ipkt][1];
          if (apin == apid) {
            int16_t *scan = new int16_t[nsamp];
            for (int i=0; i<nsamp; i++) scan[i] = -999;

            uint8_t qfl = 0;
            idet = pbuffer[ipkt][25];
            //            cout << "ipkt: " << ipkt << endl;

            // Check for valid detector index
            if ((idet >= 0) && (idet < ngpk)) {
              int status = unpack_sci( &pbuffer[ipkt][0], nsamp, scan, &qfl);
              iret = iret | status;

              // Load data into output array
              if (ibnd <= 16) {
                memcpy( &mbands[ibnd-1][idet][0], scan, nsamp*sizeof(int16_t));
                mqfl[ibnd-1][idet] = qfl;
                //              cout << "idet qfl: " << idet << " "
                //   << static_cast<unsigned>(qfl) << endl;
              }
              if (ibnd >= 21 && ibnd <= 25) {
                memcpy( &dnb[ibnd-21][idet][0], scan, nsamp*sizeof(int16_t));
                dqfl[ibnd-21][idet] = qfl;
              }
              if (ibnd >= 31) {
                memcpy( &ibands[ibnd-31][idet][0], scan, nsamp*sizeof(int16_t));
                iqfl[ibnd-31][idet] = qfl;
              }
            }
            ipkt++;
            delete[] scan;
          } else {
            idet = ngpk;
          }
        }
      } else {
        ipkt++;
      }
    } else {
      ipkt++;
    }
  }

  // Reconstruct differenced bands
  // Set up index arrays
  int32_t *jj = new int32_t[3200];
  for ( int i=0; i<3200; i++) jj[i] = 2*i;
 
  int32_t ms1[] = {0,640,1008,1600,2192,2560};    // single-gain M-bands
  int32_t ms2[] = {639,1007,1599,2191,2559,3199}; // single-gain M-bands
  int32_t md1[] = {0,640,1376,3152,4928,5664};    // dual-gain M-bands 
  int32_t md2[] = {639,1375,3151,4927,5663,6303}; // dual-gain M-bands

  int32_t i1[6], i2[6];
  for ( int i=0; i<6; i++) {
    i1[i] = ms1[i] * 2;     // I-bands
    i2[i] = ms2[i] * 2 + 1; // I-bands
  }

  // Check for missing segments
  int32_t ik = 1;
  for ( int k=0; k<=5; k++) {
    for ( int i=0; i<=15; i++) {
      // Dual-gain M-bands
      // M3 and M5 using M4
      if ((mqfl[3][i] & ik) == 0 &&
          (mqfl[2][i] & ik) == 0) {
        for ( int kk=md1[k]; kk<=md2[k]; kk++) {
          mbands[2][i][kk] += (mbands[3][i][kk] - 16383);
        }
      } else {
        mqfl[2][i] = mqfl[2][i] | ik;
        for ( int kk=md1[k]; kk<=md2[k]; kk++) {
          mbands[2][i][kk] = -999;
        }
      }

      if ((mqfl[3][i] & ik) == 0 &&
          (mqfl[4][i] & ik) == 0) {
        for ( int kk=md1[k]; kk<=md2[k]; kk++) {
          mbands[4][i][kk] += (mbands[3][i][kk] - 16383);
        }
      } else {
        mqfl[4][i] = mqfl[4][i] | ik;
        for ( int kk=md1[k]; kk<=md2[k]; kk++) {
          mbands[4][i][kk] = -999;
        }
      }

      // M2 using M3
      if ((mqfl[2][i] & ik) == 0 &&
          (mqfl[1][i] & ik) == 0) {
        for ( int kk=md1[k]; kk<=md2[k]; kk++) {
          mbands[1][i][kk] += (mbands[2][i][kk] - 16383);
        }
      } else {
        mqfl[1][i] = mqfl[1][i] | ik;
        for ( int kk=md1[k]; kk<=md2[k]; kk++) {
          mbands[1][i][kk] = -999;
        }
      }

      // M1 using M2
      if ((mqfl[1][i] & ik) == 0 &&
          (mqfl[0][i] & ik) == 0) {
        for ( int kk=md1[k]; kk<=md2[k]; kk++) {
          mbands[0][i][kk] += (mbands[1][i][kk] - 16383);
        }
      } else {
        mqfl[0][i] = mqfl[0][i] | ik;
        for ( int kk=md1[k]; kk<=md2[k]; kk++) {
          mbands[0][i][kk] = -999;
        }
      }


      // Single-gain M-bands
      // M8 and M11 using M10
      if ((mqfl[9][i] & ik) == 0 &&
          (mqfl[7][i] & ik) == 0) {
        for ( int kk=ms1[k]; kk<=ms2[k]; kk++) {
          int16_t adj = mbands[9][i][kk] - 16383;
          mbands[ 7][i][kk] += adj;
        } 
      } else {
        mqfl[ 7][i] = mqfl[ 7][i] | ik;
        for ( int kk=ms1[k]; kk<=ms2[k]; kk++) {
          mbands[ 7][i][kk] = -999;
        } 
      }


      if ((mqfl[9][i] & ik) == 0 &&
          (mqfl[10][i] & ik) == 0) {
        for ( int kk=ms1[k]; kk<=ms2[k]; kk++) {
          int16_t adj = mbands[9][i][kk] - 16383;
          mbands[10][i][kk] += adj;
        } 
      } else {
        mqfl[10][i] = mqfl[10][i] | ik;
        for ( int kk=ms1[k]; kk<=ms2[k]; kk++) {
          mbands[10][i][kk] = -999;
        } 
      }

      // M14 using M15
      if ((mqfl[14][i] & ik) == 0 &&
          (mqfl[13][i] & ik) == 0) {
        for ( int kk=ms1[k]; kk<=ms2[k]; kk++) {
          uint16_t ii;
          memcpy( &ii, &mbands[14][i][kk], 2);
          mbands[13][i][kk] += (mbands[14][i][kk] - 16383);
        }
      } else {
        mqfl[13][i] = mqfl[13][i] | ik;
        for ( int kk=ms1[k]; kk<=ms2[k]; kk++) {
          mbands[13][i][kk] = -999;
        }
      }

      // I-bands differenced to M-bands
      // I4 using M12
      for (int p=0; p<2; p++) {
        if ((mqfl[11][i] & ik) == 0 &&
            (iqfl[3][2*i+p] & ik) == 0) {
          for ( int j=0; j<=1; j++) {
            for ( int kk=ms1[k]; kk<=ms2[k]; kk++) {
              int16_t adj = mbands[11][i][kk] - 16383;
              ibands[3][2*i+p]  [jj[kk]+j] += adj;
            }
          }
        } else {
          iqfl[3][2*i+p] = iqfl[3][2*i+p]   | ik;
          for ( int j=0; j<=1; j++) {
            for ( int kk=ms1[k]; kk<=ms2[k]; kk++) {
              ibands[3][2*i+p]  [jj[kk]+j] = -999;
            }
          }
        }
      } // p-loop
      

      // I5 using M15
      for (int p=0; p<2; p++) {
        if ((mqfl[14][i] & ik) == 0 &&
            (iqfl[4][2*i+p] & ik) == 0) {
          for ( int j=0; j<=1; j++) {
            for ( int kk=ms1[k]; kk<=ms2[k]; kk++) {
              uint16_t ii;
              memcpy( &ii, &mbands[14][i][kk], 2);
              int16_t adj = mbands[14][i][kk] - 16383;
              ibands[4][2*i+p]  [jj[kk]+j] += adj;
            }
          }
        } else {
          iqfl[4][2*i+p] = iqfl[4][2*i+p]   | ik;
          for ( int j=0; j<=1; j++) {
            for ( int kk=ms1[k]; kk<=ms2[k]; kk++) {
              ibands[4][2*i+p]  [jj[kk]+j] = -999;
            }
          }
        }
      } // p-loop

    } // M-detector loop


    // I-bands 
    for ( int i=0; i<=31; i++) {
      // I2 using I1
      if ((iqfl[0][i] & ik) == 0 &&
          (iqfl[1][i] & ik) == 0) {
        for ( int kk=i1[k]; kk<=i2[k]; kk++) {
          ibands[1][i][kk] += (ibands[0][i][kk] - 16383);
        }
      } else {
        iqfl[1][i] = iqfl[1][i] | ik;
        for ( int kk=i1[k]; kk<=i2[k]; kk++) {
          ibands[1][i][kk] = -999;
        }
      }
      
      // I3 using I2
      if ((iqfl[1][i] & ik) == 0 &&
          (iqfl[2][i] & ik) == 0) {
        for ( int kk=i1[k]; kk<=i2[k]; kk++) {
          ibands[2][i][kk] += (ibands[1][i][kk] - 16383);
        }
      } else {
        iqfl[2][i] = iqfl[2][i] | ik;
        for ( int kk=i1[k]; kk<=i2[k]; kk++) {
          ibands[2][i][kk] = -999;
        }
      }
    }

    ik *= 2;
  }
  delete[] jj;

  return iret;
}


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
                      uint8_t *calmet) {
   
  int iret = 0;

  // Procedure to unpack VIIRS calibrator view data for a scan

  // sd = solar diffuser
  // sv = space view
  // bb = blackbody

  // m = M-band
  // i = I-band
  // d = DNB

  // Set to fill value (-999)
  for (int i=0; i<17; i++) {
    for (int j=0; j<16; j++) {
      for (int k=0; k<48; k++) {
        sdm[i][j][k] = -999;
        svm[i][j][k] = -999;
        bbm[i][j][k] = -999;
      }
    }
  }

  for (int i=0; i<5; i++) {
    for (int j=0; j<32; j++) {
      for (int k=0; k<96; k++) {
        sdi[i][j][k] = -999;
        svi[i][j][k] = -999;
        bbi[i][j][k] = -999;
      }
    }
  }

  for (int j=0; j<16; j++) {
    for (int k=0; k<64; k++) {
      sdd[j][k] = -999;
      svd[j][k] = -999;
      bbd[j][k] = -999;
    }
  }

  // Array of band values for VIIRS band control word
  int32_t ibnds[] = 
    {0,21,1,2,4,3,31,32,7,5,6,13,12,34,33,10,11,8,9,35,16,17,15,14};

  // Find calibration packets
  int32_t ipkt = -1;
  int32_t apid = 0;

  while (ipkt < (npkts-1) && apid != 825) {
    ipkt++;
    apid = (pbuffer[ipkt][0] % 8)*256 + pbuffer[ipkt][1];
  }

  // Check for first packet
  memset( calmet, 255, 134); // Fill with 255
  if ((pbuffer[ipkt][0] & 8) == 8) {
    memcpy( calmet, &pbuffer[ipkt][32], 134);
    ipkt++;
    apid = (pbuffer[ipkt][0] % 8)*256 + pbuffer[ipkt][1];
  }

  int32_t lpd = 0;
  int32_t ipd = 0;

  // Process remaining cal packets
  while (ipkt < npkts && apid == 825) {
   
    // Get band number
    int32_t ibid = pbuffer[ipkt][24];
    if (ibid < 1 || ibid > 23) {
      cout << "ibid out of bounds: " << ibid << endl;
      break;
    }

    int32_t ibnd = ibnds[ibid];

    // Check for M4 and M5 cal data replacement
    // Need this logic because indicators are not reliable
    // Save length of DNB cal packet
    if (ibnd == 21) {
      lpd = pbuffer[ipkt][4]*256 + pbuffer[ipkt][5] + 7;
      ipd = ipkt;
    }

    // If length of M1, M2, M4 or M5 packet matches DNB, cal replacement
    if (ibnd == 1 || ibnd == 2 || ibnd == 4 || ibnd == 5) {
      int32_t lpkt = pbuffer[ipkt][4]*256 + pbuffer[ipkt][5] + 7;

      bool test = true;
      if (lpkt == lpd) {
        for ( int i=98; i<lpd; i++) {
          if (pbuffer[ipd][i] != pbuffer[ipkt][i]) {
            test = false;
            break;
          }
        }
        if (test) ibnd = ibnd + 21;
      }
    }

    int32_t nsamps;
    int32_t ndet;
    if (ibnd != 22 && ibnd != 23 && ibnd != 25 && ibnd != 26) {
      nsamps = 48;
      if (ibnd == 21) nsamps = pbuffer[ipkt][25];
      if (ibnd >  30) nsamps = 96;

      ndet = 16;
      if (nsamps == 96) ndet = 32;

      int16_t *sv = new int16_t[nsamps*ndet];
      int16_t *bb = new int16_t[nsamps*ndet];
      int16_t *sd = new int16_t[nsamps*ndet];
      //cout << "ipkt: " << ipkt << endl;
      //cout << "ibnd: " << ibnd << endl;

      int status = unpack_cal_sci( &pbuffer[ipkt][0],
                                   nsamps, ndet, sv, bb, sd);
      iret = iret | status;

      if (status == 0) {
        if (ibnd <= 17) {
          memcpy( &sdm[ibnd-1][0][0], sd, nsamps*ndet*sizeof(int16_t));
          memcpy( &svm[ibnd-1][0][0], sv, nsamps*ndet*sizeof(int16_t));
          memcpy( &bbm[ibnd-1][0][0], bb, nsamps*ndet*sizeof(int16_t));
        }

        if (ibnd == 21) {
          if ( nsamps < 64) {
            for (int i=0; i<ndet; i++) {
              memcpy( &sdd[i][0], &sd[i*nsamps], nsamps*sizeof(int16_t));
              memcpy( &svd[i][0], &sv[i*nsamps], nsamps*sizeof(int16_t));
              memcpy( &bbd[i][0], &bb[i*nsamps], nsamps*sizeof(int16_t));
            }
          } else {
            memcpy( &sdd[0][0], sd, nsamps*ndet*sizeof(int16_t));
            memcpy( &svd[0][0], sv, nsamps*ndet*sizeof(int16_t));
            memcpy( &bbd[0][0], bb, nsamps*ndet*sizeof(int16_t));
          }
        }

        if (ibnd >= 31) {
          memcpy( &sdi[ibnd-31][0][0], sd, nsamps*ndet*sizeof(int16_t));
          memcpy( &svi[ibnd-31][0][0], sv, nsamps*ndet*sizeof(int16_t));
          memcpy( &bbi[ibnd-31][0][0], bb, nsamps*ndet*sizeof(int16_t));
        }
      }

      delete[] sv;
      delete[] bb;
      delete[] sd;
    }
    memcpy( &calmet[4*ibid+18], &pbuffer[ipkt][94], 4);

    ipkt++;
    apid = (pbuffer[ipkt][0] % 8)*256 + pbuffer[ipkt][1];
  }

  return iret;
}


int unpack_sci( uint8_t *idat, int32_t nsamp, int16_t *scan, uint8_t *qfl) {

  int iret = 0;

  int32_t lsegs[] = {640,736,1776,1776,736,640};
  int32_t lsegs_3200[] = {640,368,592,592,368,640};
  int32_t lsegs_4064[] = {784,488,760,760,488,784};
  int32_t lsegs_6400[] = {1280,736,1184,1184,736,1280};
  if (nsamp == 3200) memcpy( lsegs, lsegs_3200, 6*sizeof(int32_t));
  if (nsamp == 4064) memcpy( lsegs, lsegs_4064, 6*sizeof(int32_t));
  if (nsamp == 6400) memcpy( lsegs, lsegs_6400, 6*sizeof(int32_t));

  uint8_t tmp[65536];
  uint8_t qflag = 63;
  // uint8_t fill[4] = {222,173,222,173};
  int32_t np = idat[4]*256 + idat[5];

  // Find first synch work offset
  int32_t ip = 96;

  // Find first synch work offset
  uint8_t synp[4];
  memcpy( synp, &idat[26], 4);

  // Loop through aggregations
  int32_t is = 0;
  for ( int id=0; id<6; id++) {

    int32_t lp = idat[ip]*256 + idat[ip+1] - 4;
    if ((ip+lp) > np || lp < 4) {
      *qfl = qflag;
      return 8;
    }
    
    //    // Added 05/12/15 to avoid problem with memcpy
    //if (lp == -4) return 0;

  
    uint8_t chksm[4], sync[4];
    for ( int i=0; i<4; i++) sync[i] = idat[ip+lp+6+i];
    // Move compressed data to temporary array
    memcpy( tmp, &idat[ip+2], lp);

    // Check for invalid sync word, indicating rest of packet
    // is fill or zero
    int totchksum = 0;
    for ( int i=0; i<4; i++) totchksum += (sync[i] == synp[i]);
    if (totchksum != 4) {
      *qfl = qflag;
      return 8;
    }
    
    for ( int i=0; i<4; i++) chksm[i] = idat[ip+lp+2+i];
    uint8_t check = check_sum( lp/4, tmp, chksm);
    if ( !check) {
      iret = iret | 4;
      //      continue;
      //      return 4;
    }

    // Strip off trailing zeroes
    int32_t nbytes = lp;
    if ( idat[ip-2] >= 8) nbytes -= 2;
    if ( (tmp[nbytes-1] == 0) && (tmp[nbytes-2] == 0)) nbytes -= 2;
    //    if ((odat(nbytes(id)-1,id) mod 64) eq 0) then nbytes(id) =
    //    nbytes(id) - 1
    
    if ( nbytes > 5 && check) {

      static unsigned int packet_count=0;
      packet_count++;

      //      pid_t pid = getpid();
      //stringstream pidstr;
      //pidstr << pid;
      // printf("lsegs[id]: %d  nbytes: %d\n", lsegs[id], nbytes);

      // uncompress with rice.c
      // 128=RAW, 32=NN_MODE, 16=MSB, 2=CHIP
      int options_mask=128+32+2;
      int bits_per_pixel=15;
      int pixels_per_block=8;
      int pixels_per_scanline=lsegs[id];
      uint16_t *decryptArrRice = new uint16_t[lsegs[id]+32*16];
      //      printf("RICE\n");

      long bytes_written = 
        szip_uncompress_memory( options_mask, bits_per_pixel, pixels_per_block,
                                pixels_per_scanline, (const char*) tmp, nbytes, 
                                decryptArrRice, 2*(lsegs[id]+32*16));
      
      if ( bytes_written < 0) {
        cout << "Error in rice uncompress" << endl;
        exit(1);
      }

#if 0
      // Compare Usds and rice output
      int argc = 7;
      char *argv[] = { (char *) "Usds", 
                       (char *) "-n", (char *) "15", 
                       (char *) "-j", (char *) "8", 
                       (char *) "-br", (char *) "128"};
      //      printf("USDS\n");
      uint16_t *decryptArr = new uint16_t[lsegs[id]+32*16];
      usds(argc, argv, nbytes, tmp, decryptArr);

      for ( int i=0; i<lsegs[id]; i++) {
        uint16_t ui16;
        memcpy( &ui16, &decryptArr[i], sizeof(uint16_t));
        decryptArr[i] = SWAP_2( ui16);
      }

      for ( int i=0; i<lsegs[id]; i++) {
        if (decryptArr[i] != decryptArrRice[i]) {
          printf("%d %d %d %d\n", 
                 i, lsegs[id], decryptArr[i], decryptArrRice[i]);
          exit(1);
        }
      }
      delete decryptArr;
      // End compare Usds and rice
#endif

      memcpy( &scan[is], decryptArrRice, lsegs[id]*sizeof(int16_t));
      uint8_t idpow = 1;
      for ( int i=0; i<id; i++) idpow *= 2;
      qflag -= idpow;
      delete[] decryptArrRice;
    }
    is += lsegs[id];
    ip += lp + 12;
  } // id loop

  *qfl = qflag;

  //  cout << "qfl is ip: " << static_cast<unsigned>(qflag) << " " << is 
  //   << " " << ip << endl;
  return iret;
}


int unpack_cal_sci( uint8_t *idat, int32_t nsamp, int32_t ndet, 
                    int16_t *sp, int16_t *bl, int16_t *sd) {

  // This routine unpacks the 15-bit samples from a VIIRS middle or last
  // calibration packet and stores them right-justified in 2-byte arrays.
  // This requires external calls to the Rice decompression routine
  // szip_uncompress_memory() which replaces Usds.

  //   Arguments

  //   Name    Type          I/O    Description
  //   ----    ----          ---    -----------
  //   idat    byte(*)        I     Input packet array
  //   nsamp   int            I     Number of data samples in packet
  //   ndet    int            I     Number of detectors
  //   sp      int(nsamps*16) O     Output array of space view samples
  //   bl      int(nsamps*16) O     Output array of blackbody samples
  //   sd      int(nsamps*16) O     Output array of solar diffuser samples
  //   iret    int            O     Return code 0 = good
  //                                            4 = checksum failure
  //                                            8 = fill or corrupted pkt data

  uint8_t tmp[200];
  uint16_t *decryptArrRice = new uint16_t[nsamp+32*16];

  // Check for M-band cal data substitution by DNB
  int32_t nsamps = idat[25];
  if (nsamps <= 0) return 8;

  //  uint8_t fill[4] = {222,173,222,173};
  int32_t np = idat[4]*256 + idat[5];

  // Find first synch work offset
  int32_t ip = 100;

  // Find first synch work offset
  uint8_t synp[4];
  memcpy( synp, &idat[26], 4);

  // Loop through detectors and views
  for ( int id=0; id<ndet; id++) {

    // Space view
    int32_t lp = idat[ip]*256+idat[ip+1] - 4;
    //    cout << "SV id: " << id << " lp: " << lp << endl;
    if (ip+lp > np || lp < 4) return 8;

    // Check for invalid sync word
    uint8_t sync[4];
    for ( int i=0; i<4; i++) sync[i] = idat[ip+lp+6+i];
    int cnt = 0;
    for ( int i=0; i<4; i++) if (sync[i] == synp[i]) cnt++;
    if (cnt != 4) return 8;

    memcpy( tmp, &idat[ip+2], lp*sizeof(int8_t));
    int32_t nbytes = lp;

    if (idat[ip-2] >= 8) nbytes = nbytes - 2;

    uint8_t chksm[4];
    for ( int i=0; i<4; i++) chksm[i] = idat[ip+lp+2+i];
    uint8_t check = check_sum( lp/4, tmp, chksm);
    if ( !check) return 4;

#if 0
    int argc = 7;
    char *argv_sp[] = { (char *) "Usds", 
                        (char *) "-n", (char *) "15", 
                        (char *) "-j", (char *) "8", 
                        (char *) "-br", (char *) "128"};

    uint16_t *decryptArr = new uint16_t[nsamp+32*16];

    usds(argc, argv_sp, nbytes, tmp, decryptArr);

    for ( int i=0; i<nsamp; i++) {
      uint16_t ui16;
      memcpy( &ui16, &decryptArr[i], sizeof(uint16_t));
      decryptArr[i] = SWAP_2( ui16);
    }
    delete decryptArr;
#endif

    // uncompress with rice.c
    // 128=RAW, 32=NN_MODE, 16=MSB, 2=CHIP
    int options_mask=128+32+2;
    int bits_per_pixel=15;
    int pixels_per_block=8;
    int pixels_per_scanline=nsamp;
    long bytes_written;

    if (nbytes > 4 && check) {
      bytes_written = 
        szip_uncompress_memory( options_mask, bits_per_pixel, pixels_per_block,
                                pixels_per_scanline, (const char*) tmp, nbytes, 
                                decryptArrRice, 200);
    
      if ( bytes_written < 0) {
        cout << "Error in rice uncompress" << endl;
        exit(1);
      }

      memcpy( &sp[id*nsamp], decryptArrRice, nsamp*sizeof(int16_t));
    }

    ip = ip + lp + 12;

    // Blackbody view
    lp = idat[ip]*256+idat[ip+1] - 4;
    //    cout << "BB id: " << id << " lp: " << lp << endl;
    if ((ip+lp > np) || (lp < 4)) return 8;

    // Check for invalid sync word
    for ( int i=0; i<4; i++) sync[i] = idat[ip+lp+6+i];
    cnt = 0;
    for ( int i=0; i<4; i++) if (sync[i] == synp[i]) cnt++;
    if (cnt != 4) return 8;

    memcpy( tmp, &idat[ip+2], lp*sizeof(int8_t));
    nbytes = lp;

    if (idat[ip-2] >= 8) nbytes = nbytes - 2;
    for ( int i=0; i<4; i++) chksm[i] = idat[ip+lp+2+i];

    // Check for invalid sync word, indicating rest of packet
    // is fill or zero
    check = check_sum( lp/4, tmp, chksm);
    for ( int i=0; i<4; i++) chksm[i] = idat[ip+lp+2+i];
    if ( !check) return 4;

    // uncompress with rice.c
   if (nbytes > 4 && check) {
     bytes_written = 
       szip_uncompress_memory( options_mask, bits_per_pixel, pixels_per_block,
                               pixels_per_scanline, (const char*) tmp, nbytes, 
                               decryptArrRice, 200);
    
     if ( bytes_written < 0) {
       cout << "Error in rice uncompress" << endl;
       exit(1);
     }

     memcpy( &bl[id*nsamp], decryptArrRice, nsamp*sizeof(int16_t));
   }

    ip = ip + lp + 12;

    // Solar diffuser view
    lp = idat[ip]*256+idat[ip+1] - 4;
    //cout << "SD id: " << id << " lp: " << lp << endl;
    if (ip+lp > np || lp < 4) return 8;

    // Check for invalid sync word
    for ( int i=0; i<4; i++) sync[i] = idat[ip+lp+6+i];
    cnt = 0;
    for ( int i=0; i<4; i++) if (sync[i] == synp[i]) cnt++;
    if (cnt != 4) return 8;

    memcpy( tmp, &idat[ip+2], lp*sizeof(int8_t));
    nbytes = lp;

    if (idat[ip-2] >= 8) nbytes = nbytes - 2;
    for ( int i=0; i<4; i++) chksm[i] = idat[ip+lp+2+i];

    check = check_sum( lp/4, tmp, chksm);
    for ( int i=0; i<4; i++) chksm[i] = idat[ip+lp+2+i];
    if ( !check) return 4;

    if (nbytes > 4 && check) {
      bytes_written = 
        szip_uncompress_memory( options_mask, bits_per_pixel, pixels_per_block,
                                pixels_per_scanline, (const char*) tmp, nbytes, 
                                decryptArrRice, 200);
    
      if ( bytes_written < 0) {
        cout << "Error in rice uncompress" << endl;
        exit(1);
      }

      memcpy( &sd[id*nsamp], decryptArrRice, nsamp*sizeof(int16_t));
    }

    ip = ip + lp + 12;
  }
  delete[] decryptArrRice;

  return 0;
}

int read_packet( fstream *vfileStream, uint8_t *packet, 
                 int32_t *len, int32_t *apid, int32_t *endfile) {

  *endfile = 0;
  if ( vfileStream->eof()) {
    *endfile = 1;
    *len = 0;
    cout << "End of packet file" << endl;
    return 0;
  }

  // Read packet header
  uint8_t phead[6];
  if ( packet == NULL) {
    vfileStream->read( (char *) &phead, 6);

    // Get length of packet body and APID
    *len = phead[4]*256 + phead[5] + 1 + 6;
    *apid = (phead[0] % 8)*256 + phead[1];

    if ( vfileStream->tellg() == -1) *endfile = 1;
    return 0;
  }

  vfileStream->read( (char *) packet, *len);

  return 0;
}


uint8_t check_sum( int32_t nc, uint8_t *dat, uint8_t *chk) {

  // Function to check data against checksum
  // Checksum is 4 bytes computed by XOR

  uint8_t chks[4], tmp[4];
  memcpy( chks, &dat[0], 4);

  for ( int i=1; i<nc; i++) {
    memcpy( &tmp, &dat[4*i], 4);
    for ( int j=0; j<4; j++) chks[j] = chks[j] ^ tmp[j];
  }

  uint8_t check[4];
  for ( int i=0; i<4; i++) check[i] = (chk[i] == chks[i]);

  uint8_t check_sum = (check[0] & check[1] & check[2] & check[3]);

  return check_sum;
}


int scan_complete( uint8_t (*pbuffer)[PBUFFER_SIZE], int32_t npkts) {

  int icmp = 0;
  int32_t mpkts;

  // Find first science header packet in file
  int32_t i = 0;
  while ( (pbuffer[i][0] != 11) || (pbuffer[i][1] == 58)) i++;

  // Get VIIRS mode
  uint8_t mode = pbuffer[i][46];

  // If Night mode
  if ( mode == 5) {
    mpkts = 244;
    // Need to check for M11 and DNB MGS, LGS, HGA, HGB packets
    for ( int i=0; i<npkts; i++) {
      if (pbuffer[i][0] == 11) {
	if (pbuffer[i][1] == 42) mpkts = mpkts + 17; // M11
        if (pbuffer[i][1] == 54) mpkts = mpkts + 17; // DNB MGS
        if (pbuffer[i][1] == 55) mpkts = mpkts + 17; // DNB LGS
	if (pbuffer[i][1] == 59) mpkts = mpkts + 17; // DNB HGA
	if (pbuffer[i][1] == 60) mpkts = mpkts + 17; // DNB HGB
      }
    }

  } else {
    // Else assume day mode
    mpkts = 479;
  }

  if (npkts != mpkts) icmp = 2;

  return icmp;
}
