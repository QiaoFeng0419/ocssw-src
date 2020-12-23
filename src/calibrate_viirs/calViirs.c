#include <string.h>

#define VIIRS_SCANS    203

int main(int argc, char *argv[]) {

    char *l1aFilename = "/disk02/joel/VIIRSRUN/viirs_L1_test/TEST_DATA/L1A/V2015280115400.L1A_SNPP.nc";
    char *l1aCalParFilename;

    VcstViirsCal_initialize(l1aFilename, l1aCalParFilename);

    int i;
    static float *radptrs[16] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    int iline, nbands = 16;
    for (iline = 0; iline < VIIRS_SCANS * 16; iline++) {
        VcstViirsCal_calibrateMOD(iline, nbands, &radptrs);
    }

    // (band 0) radiance_[15][2000] = radptrs[0][15*3200+2000] = 86.9709167
    // (band 1) radiance_[15][2000] = radptrs[1][15*3200+2000] = 54.40485

    return 0;
}

