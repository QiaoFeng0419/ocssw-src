#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <sstream>
#include <iomanip>
#include "l1agen_viirs.h"
#include "netcdf.h"
#include "nc4utils.h"

#define VERSION "0.998"
#define BOUNDS_ERROR 110

//    Modification history:
//  Programmer     Organization   Date     Ver   Description of change
//  ----------     ------------   ----     ---   ---------------------
//  Joel Gales     FutureTech     01/27/15 0.01  Original development
//  Joel Gales     FutureTech     09/02/15 0.02  Remove DNB bands,
//                                               Extract navigation fields
//  Joel Gales     FutureTech     09/15/15 0.03  Add support for pixel extract
//  Joel Gales     FutureTech     12/11/15 0.04  Fix pixel->sample conversion
//                                               in createl1() and copyl1()
//  Joel Gales     FutureTech     01/28/16 0.990 Write spixl, epixl in history
//  Joel Gales     FutureTech     02/02/16 0.991 Adjust spixl/spixl to line
//                                               up with GEO interpolation
//                                               boundaries
//  Sean Bailey    NASA/GSFC      04/21/16 0.992 accept M-band lines as input
//                                               and convert to scans
//  Joel Gales     FutureTech     04/29/16 0.993 Shorten time period used to
//                                               determine att/orb start/count
//                                               values in copy_l1()
//  Joel Gales     FutureTech     06/16/16 0.994 Tweak if number of extracted
//                                               scans one greater than
//                                               # of allocated scans
//  Joel Gales     FutureTech     04/03/17 0.995 Compress extracted EV fields
//  Joel Gales     FutureTech     04/24/17 0.996 Increase GEO alignment size
//                                               from 8 to 16
//  Joel Gales     FutureTech     09/27/17 0.997 Adjust so at least 8 scans
//                                               are extracted
//  Joel Gales     FutureTech     10/12/17 0.998 Check for number of filled
//                                               scans when less than 8 scans

int main(int argc, char* argv[])
{

    cout << "l1aextract_viirs " << VERSION << " ("
         <<  __DATE__ << " " << __TIME__ << ")" << endl;

    if (argc == 1) {
        cout << endl <<
            "l1aextract_viirs infile spixl epixl sline eline outfile\n" << endl;
        cout << "  where:" << endl;
        cout << "    infile  - input VIIRS L1A file\n" << endl;
        cout << "    spixl   - start pixel (1-based)" << endl;
        cout << "    epixl   - end pixel (1-based)" << endl;
        cout << "    sline   - start line (M-band resolution, 1-based)" << endl;
        cout << "    eline   - end line (M-band resolution, 1-based)" << endl;
        cout << "    outfile - output VIIRS L1A extract file" << endl;

        cout << endl;
        cout << "To extract entire scan line set both spixl and epixl to 0"
             << endl;

        return EXIT_FAILURE;
    }

    static l1aFile infile, outfile;
    string str;
    int32_t spixl, epixl, npixl, sline, eline, sscan, escan, nscan;
    int npixels = 3200;

    str.assign(argv[2]);
    istringstream(str) >> spixl;
    if (spixl != 0) spixl--;

    str.assign(argv[3]);
    istringstream(str) >> epixl;
    if (epixl == 0) epixl = npixels;
    epixl--;

    str.assign(argv[4]);
    istringstream(str) >> sline;
    sline--;
    sscan = floor(sline / 16);

    str.assign(argv[5]);
    istringstream(str) >> eline;
    eline--;
    escan = ceil(eline / 16);

    if ((spixl > epixl) || (sscan > escan)) {
        cout << "\nInvalid range requested:" << endl;
        npixl = epixl - spixl + 1;
        nscan = escan - sscan + 1;
        printf("spixl: %4d  epixl: %4d  npixl: %4d\n", spixl+1, epixl+1, npixl);
        printf("sscan: %4d  escan: %4d  nscan: %4d\n", sscan+1, escan+1, nscan);
        exit(BOUNDS_ERROR);
    }

    // Get dimension info
    int nfilled;
    infile.openl1(argv[1]);
    int ncid = infile.getNcid();
    nc_get_att(ncid, NC_GLOBAL, "number_of_filled_scans", &nfilled);
    infile.close();

    // Adjust start/end scans if necessary
    nscan = escan - sscan + 1;
    if (nscan < 8) {
        cout << "Adjusting start/end scans so that extract is at least 8 scans"
             << endl;

        int diff = 8 - nscan;
        int half_diff;
        if ((diff % 2) == 0)
            half_diff = diff / 2;
        else
            half_diff = (diff / 2) + 1;

        if ((sscan - half_diff) < 0) {
            escan = escan + 2 * half_diff - sscan;
            sscan = 0;
        } else if ((escan + half_diff) > (nfilled - 1)) {
            sscan = sscan - 2 * half_diff + ((nfilled - 1) - escan);
            escan = nfilled - 1;
        } else {
            sscan = sscan - half_diff;
            escan = escan + half_diff;
        }

        nscan = escan - sscan + 1;
    }

    // Adjust to align with GEO processing
    spixl = (spixl / 16) * 16;
    epixl = (epixl / 16) * 16 + 15;
    npixl = epixl - spixl + 1;

    // Check requested number of pixels & scan lines
    if (spixl >= npixels) {
        cout << "spixl " << spixl << " >= pixel count ("
             << npixels << ")" << endl;
        exit(BOUNDS_ERROR);
    }
    if (epixl >= npixels) {
        cout << "epixl " << epixl << " >= pixel count ("
             << npixels << ") ; adjusting." << endl;
        epixl = npixels - 1;
    }
    if (sscan >= nfilled) {
        cout << "sscan " << sscan << " >= scan count ("
             << nfilled << ")" << endl;
        exit(BOUNDS_ERROR);
    }
    if (escan >= nfilled) {
        cout << "escan " << escan << " >= scan count ("
             << nfilled << ") ; adjusting." << endl;
        escan = nfilled - 1;
    }

    // Print (1-based) sscan, escan, etc
    cout << "Adjusted extract range:" << endl;
    printf("spixl: %4d  epixl: %4d  npixl: %4d\n", spixl+1, epixl+1, npixl);
    printf("sscan: %4d  escan: %4d  nscan: %4d\n", sscan+1, escan+1, nscan);
    cout << endl;

    ////////////////////// Main Loop ///////////////////
    infile.openl1(argv[1]);
    outfile.platform.assign(infile.platform);
    outfile.createl1(argv[6], sscan, escan, spixl, epixl, (int32_t) 1);

    infile.copyl1(argv[1], argv[6], &outfile, sscan, escan, spixl, epixl);

    outfile.close();

    /////////////////// End Main Loop ///////////////////

    return EXIT_SUCCESS;
}
