/*
 * l3mapgen.cpp
 *
 *  Created on: Jan 15, 2014
 *      Author: dshea
 */
#include "l3mapgen.h"

#include "OutFile.h"
#include <L3FileSMI.h>

#include <iostream>

#include <genutils.h>
#include <timeutils.h>
#include <string>

#include <proj_api.h>
#include <sensorInfo.h>
#include <productInfo.h>

using namespace std;
using namespace l3;

#define VERSION "1.0"

#define NUM_SEARCH_POINTS 51

enum MeasurementType {
    Avg, Stdev, Variance, Nobs, Nscenes, ObsTime, BinNum
};
enum InterpType {
    Interp_Nearest, Interp_Bin, Interp_Linear, Interp_Area
};
enum EastWest {
    notEastOrWest, IsEast, IsWest
};

// global params for program
double widthInDeg;
double heightInDeg;
double mapEast;
double mapWest;

int imageWidth = 0;
int imageHeight = 0;
bool doingQuality = false;
bool doingRGB = false;
bool doingTransparency = false;
bool trimNSEW = true;

string prodName;
vector<string> prodNameList;
vector<MeasurementType> prodMeasurementList;
string qualName;

clo_optionList_t* optionList = NULL;

// landmask
bool applyMask = false;
static grid_info_t* landmaskGrid = {0};

int isLand(float lat, float lon) {
    if (landmaskGrid != NULL) {
        double value;
        int status = get_bylatlon(landmaskGrid, lat, lon, &value);
        if (!status)
            return ((short) value != 1); // 1 = water
    }
    return (0); // assume water if lon, lat not found
}

void printStartInfo(vector<OutFile*> outFiles) {
    if (want_verbose) {
        meta_l3bType* metaData = outFiles[0]->getMetadata();

        clo_printVersion();
        printf("ifile      : %s\n", clo_getString(optionList, "ifile"));
        printf("ofile      : ");
        for(OutFile* outFile : outFiles) {
            if(outFile == outFiles[0])
                printf("%s", outFile->getFileName().c_str());
            else
                printf(",%s", outFile->getFileName().c_str());
        }
        printf("\n");
        
        printf("oformat    : %s\n", clo_getString(optionList, "oformat"));
        if (clo_isSet(optionList, "ofile2")) {
            printf("ofile2     : %s\n", clo_getString(optionList, "ofile2"));
            printf("oformat2   : %s\n", clo_getString(optionList, "oformat2"));
        }
        printf("projection : %s\n", clo_getRawString(optionList, "projection"));
        printf("resolution : %.3fm\n", outFiles[0]->getResolution());
        if (clo_isSet(optionList, "width"))
            printf("width      : %d\n", clo_getInt(optionList, "width"));
        if (doingRGB)
            printf("product_rgb: %s\n", prodName.c_str());
        else
            printf("product    : %s\n", prodName.c_str());
        if (doingQuality)
            printf("qual_prod  : %s\n", qualName.c_str());
        printf("north      : %8.3f\n", metaData->north);
        printf("south      : %8.3f\n", metaData->south);
        printf("east       : %8.3f\n", metaData->east);
        printf("west       : %8.3f\n", metaData->west);
        float tmpf = clo_getFloat(optionList, "central_meridian");
        if (tmpf > -900.0) {
            printf("central_meridian : %8.3f\n", tmpf);
        }
        if (clo_isSet(optionList, "lat_ts")) {
            printf("lat_ts     : %8.3f\n", clo_getFloat(optionList, "lat_ts"));
        }
        if (clo_isSet(optionList, "lat_0")) {
            printf("lat_0      : %8.3f\n", clo_getFloat(optionList, "lat_0"));
        }
        if (clo_isSet(optionList, "azimuth")) {
            printf("azimuth    : %8.3f\n", clo_getFloat(optionList, "azimuth"));
        }
        printf("image size : %d x %d\n", imageHeight, imageWidth);

        printf("\n");
    }
}

void printEndInfo(OutFile* outFile) {
    if (want_verbose) {
        printf("\n\n");
        printf("actual data min       : %f\n", outFile->getFileMinVal());
        printf("actual data max       : %f\n", outFile->getFileMaxVal());
        printf("num filled pixels     : %d\n", outFile->getNumFilledPixels());
        printf("percent filled pixels : %.2f%%\n",
               outFile->getPercentFilledPixels());
        printf("\n");
    }
}

void printPercentDone(float percentDone) {
    static float percentPrev = 0.0;
    static const float percentDelta = 0.01;
    if (want_verbose && (percentDone - percentPrev > percentDelta)) {
        percentPrev = percentDone;
        printf("\r%2d%% complete", (int) (percentDone * 100));
        fflush(stdout);
    }
}

float getCentralMeridian() {
    int i;
    float centralMeridian = clo_getFloat(optionList, "central_meridian");
    if (centralMeridian > -900.0) {
        i = 0;
        while (centralMeridian < -180.0) {
            centralMeridian += 360.0;
            i++;
            if (i > 5) {
                printf("-E- central meridian is way off\n");
                exit(EXIT_FAILURE);
            }
        }
        i = 0;
        while (centralMeridian > 180.0) {
            centralMeridian -= 360.0;
            i++;
            if (i > 5) {
                printf("-E- central meridian is way off\n");
                exit(EXIT_FAILURE);
            }
        }
    } else {
        centralMeridian = (mapWest + mapEast) / 2.0;
    }
    return centralMeridian;
}

InterpType interpStr2Type(const char* str) {
    string s = str;
    boost::trim(s);
    boost::to_lower(s);

    if (s.compare("bin") == 0)
        return Interp_Bin;
    if (s.compare("linear") == 0)
        return Interp_Linear;
    if (s.compare("area") == 0)
        return Interp_Area;

    return Interp_Nearest;
}

const char* interpType2Str(InterpType interp) {
    switch (interp) {
    case Interp_Nearest:
        return "nearest";
    case Interp_Bin:
        return "bin";
    case Interp_Linear:
        return "linear";
    case Interp_Area:
        return "area";
    default:
        return "unknown";
    }
}

bool checkDateLineCrossed(double lon, double deltaLon) {
    bool crossed = false;

    float minlon = constrainLon(lon) - (deltaLon / 2.0);
    float maxlon = constrainLon(lon) + (deltaLon / 2.0);

    if (minlon > 0 && maxlon < 0)
        crossed = true;

    return crossed;

}

Box_t getBox(float lat, float lon, float deltaLat, float deltaLon, int eastwest = notEastOrWest) {
    Point_t pMin;
    Point_t pMax;
    lon = constrainLon(lon);
    if (eastwest == IsEast) {
        pMin.set<0>(180.0);
        pMin.set<1>(lat - (deltaLat / 2.0));
        pMax.set<0>(lon + (deltaLon / 2.0));
        pMax.set<1>(lat + (deltaLat / 2.0));
    } else if (eastwest == IsWest) {
        pMin.set<0>(lon - (deltaLon / 2.0));
        pMin.set<1>(lat - (deltaLat / 2.0));
        pMax.set<0>(180.0);
        pMax.set<1>(lat + (deltaLat / 2.0));
    } else {
        pMin.set<0>(lon - (deltaLon / 2.0));
        pMin.set<1>(lat - (deltaLat / 2.0));
        pMax.set<0>(lon + (deltaLon / 2.0));
        pMax.set<1>(lat + (deltaLat / 2.0));
    }
    Box_t box(pMin, pMax);
    return box;
}

L3Bin* getBoxBins(L3File* l3File, float lat, float lon, float deltaLat,
                  float deltaLon, float fudge, bool areaWeighted, int eastwest = notEastOrWest) {

    Box_t box = getBox(lat, lon, deltaLat, deltaLon, eastwest);

    L3Bin* l3Bin = l3File->getBinsInside(box, areaWeighted);

    if (!l3Bin && fudge > 1) { // try again with fudge factor
        Box_t box = getBox(lat, lon, deltaLat*fudge, deltaLon*fudge, eastwest);
        l3Bin = l3File->getBinsInside(box, areaWeighted);
    }
    return l3Bin;
}

/**
 * figure out if we want and can do quality processing
 * @param l3File input bin file
 * @param outFile output file
 */
bool setupQualityProcessing(L3File* l3File, vector<OutFile*> outFiles,
                            OutFile* outFile2) {
    doingQuality = true;
    clo_option_t* option = clo_findOption(optionList, "use_quality");
    if (clo_isOptionSet(option)) {
        if (clo_getOptionBool(option)) {
            if (l3File->hasQuality()) {
                doingQuality = true;
            } else {
                printf("-E- Quality processing was requested, "
                       "but the input file does not have quality data.\n");
                exit(EXIT_FAILURE);
            }
            if (clo_isSet(optionList, "quality_product")) {
                qualName = clo_getRawString(optionList, "quality_product");
            } else {
                qualName = "qual_" + prodNameList[0];
            }
        } else {
            doingQuality = false;
        }
    } else {
        if (l3File->hasQuality())
            doingQuality = true;
        else
            doingQuality = false;
    }

    l3File->setQualityProcessing(doingQuality);
    for(OutFile* outFile : outFiles) {
        outFile->setQualityProcessing(doingQuality);
        outFile->setQualityName(qualName);
    }
    if (outFile2) {
        outFile2->setQualityProcessing(doingQuality);
        outFile2->setQualityName(qualName);
    }
    return doingQuality;
}

void setupProduct(string &prodName, MeasurementType measure, OutFile* outFile,
                  OutFile* outFile2) {
    // get the product info
    productInfo_t *p_info;
    p_info = allocateProductInfo();

    int sensorId = sensorName2SensorId(outFile->getMetadata()->sensor_name);
    if (sensorId == -1) {
        printf("-E- Unknown sensor name %s\n",
               outFile->getMetadata()->sensor_name);
        exit(EXIT_FAILURE);
    }

    if (!findProductInfo(prodName.c_str(), sensorId, p_info)) {
        printf("-E- product %s not found in XML product table\n",
               prodName.c_str());
        exit(EXIT_FAILURE);
    }

    // now we have to fix the p_info structure
    // Avg, Stdev, Variance, Nobs, Nscenes, ObsTime, BinNum
    string tmpStr;
    switch (measure) {
    case Avg:
        break;
    case Stdev:
        tmpStr = p_info->description;
        free(p_info->description);
        tmpStr += " (Standard Deviation)";
        p_info->description = strdup(tmpStr.c_str());
        free(p_info->palette);
        p_info->palette = strdup(PRODUCT_DEFAULT_palette);
        free(p_info->dataType);
        p_info->dataType = strdup("float");
        tmpStr = p_info->suffix;
        free(p_info->suffix);
        tmpStr += "_stdev";
        p_info->suffix = strdup(tmpStr.c_str());
        free(p_info->displayScale);
        p_info->displayScale = strdup("linear");
        p_info->addOffset = 0.0;
        p_info->scaleFactor = 1.0;
        break;
    case Variance:
        tmpStr = p_info->description;
        free(p_info->description);
        tmpStr += " (Variance)";
        p_info->description = strdup(tmpStr.c_str());
        free(p_info->palette);
        p_info->palette = strdup(PRODUCT_DEFAULT_palette);
        free(p_info->dataType);
        p_info->dataType = strdup("float");
        tmpStr = p_info->suffix;
        free(p_info->suffix);
        tmpStr += "_var";
        p_info->suffix = strdup(tmpStr.c_str());
        free(p_info->displayScale);
        p_info->displayScale = strdup("linear");
        p_info->addOffset = 0.0;
        p_info->scaleFactor = 1.0;
        break;
    case Nobs:
        tmpStr = p_info->description;
        free(p_info->description);
        tmpStr += " (number of observations)";
        p_info->description = strdup(tmpStr.c_str());
        free(p_info->units);
        p_info->units = strdup("counts");
        free(p_info->palette);
        p_info->palette = strdup(PRODUCT_DEFAULT_palette);
        free(p_info->dataType);
        p_info->dataType = strdup("short");
        tmpStr = p_info->suffix;
        free(p_info->suffix);
        tmpStr += "_nobs";
        p_info->suffix = strdup(tmpStr.c_str());
        p_info->fillValue = PRODUCT_DEFAULT_fillValue;
        p_info->validMin = 0;
        p_info->validMax = 32767;
        free(p_info->displayScale);
        p_info->displayScale = strdup("linear");
        p_info->displayMin = PRODUCT_DEFAULT_displayMin;
        p_info->displayMax = PRODUCT_DEFAULT_displayMax;
        p_info->addOffset = 0.0;
        p_info->scaleFactor = 1.0;
        break;
    case Nscenes:
        tmpStr = p_info->description;
        free(p_info->description);
        tmpStr += " (number of scenes)";
        p_info->description = strdup(tmpStr.c_str());
        free(p_info->units);
        p_info->units = strdup("counts");
        free(p_info->palette);
        p_info->palette = strdup(PRODUCT_DEFAULT_palette);
        free(p_info->dataType);
        p_info->dataType = strdup("short");
        tmpStr = p_info->suffix;
        free(p_info->suffix);
        tmpStr += "_nscenes";
        p_info->suffix = strdup(tmpStr.c_str());
        p_info->fillValue = PRODUCT_DEFAULT_fillValue;
        p_info->validMin = 0;
        p_info->validMax = 32767;
        free(p_info->displayScale);
        p_info->displayScale = strdup("linear");
        p_info->displayMin = PRODUCT_DEFAULT_displayMin;
        p_info->displayMax = PRODUCT_DEFAULT_displayMax;
        p_info->addOffset = 0.0;
        p_info->scaleFactor = 1.0;
        break;
    case ObsTime:
        tmpStr = p_info->description;
        free(p_info->description);
        tmpStr += " (observation time, TAI93)";
        p_info->description = strdup(tmpStr.c_str());
        free(p_info->units);
        p_info->units = strdup("counts");
        free(p_info->palette);
        p_info->palette = strdup(PRODUCT_DEFAULT_palette);
        free(p_info->dataType);
        p_info->dataType = strdup("float");
        tmpStr = p_info->suffix;
        free(p_info->suffix);
        tmpStr += "_obs_time";
        p_info->suffix = strdup(tmpStr.c_str());
        p_info->fillValue = PRODUCT_DEFAULT_fillValue;
        p_info->validMin = PRODUCT_DEFAULT_validMin;
        p_info->validMax = PRODUCT_DEFAULT_validMin;
        free(p_info->displayScale);
        p_info->displayScale = strdup("linear");
        p_info->displayMin = PRODUCT_DEFAULT_displayMin;
        p_info->displayMax = PRODUCT_DEFAULT_displayMax;
        p_info->addOffset = 0.0;
        p_info->scaleFactor = 1.0;
        break;
    case BinNum:
        tmpStr = p_info->description;
        free(p_info->description);
        tmpStr += " (bin ID number)";
        p_info->description = strdup(tmpStr.c_str());
        free(p_info->units);
        p_info->units = strdup("dimensionless");
        free(p_info->palette);
        p_info->palette = strdup(PRODUCT_DEFAULT_palette);
        free(p_info->dataType);
        p_info->dataType = strdup("int");
        tmpStr = p_info->suffix;
        free(p_info->suffix);
        tmpStr += "_bin_num";
        p_info->suffix = strdup(tmpStr.c_str());
        p_info->fillValue = PRODUCT_DEFAULT_fillValue;
        p_info->validMin = PRODUCT_DEFAULT_validMin;
        p_info->validMax = PRODUCT_DEFAULT_validMin;
        free(p_info->displayScale);
        p_info->displayScale = strdup("linear");
        p_info->displayMin = PRODUCT_DEFAULT_displayMin;
        p_info->displayMax = PRODUCT_DEFAULT_displayMax;
        p_info->addOffset = 0.0;
        p_info->scaleFactor = 1.0;
        break;
    } // switch

    // load landmask options before palette
    applyMask = clo_getBool(optionList, "mask_land");
    if (applyMask) {
        char *strVal = clo_getOptionString(clo_findOption(optionList, "land"));
        char landmaskFile[FILENAME_MAX];
        parse_file_name(strVal, landmaskFile);
        static const char* landmaskVars[] ={"watermask", "landmask", "z", NULL};
        landmaskGrid = allocate_gridinfo();
        int status = init_gridinfo(landmaskFile, landmaskVars, landmaskGrid);
        if (status != NC_NOERR) {
            free(landmaskGrid);
            landmaskGrid = NULL;
            cerr << "Error reading file " << landmaskFile << ": "
                 << nc_strerror(status) << "\n"
                 << "Land mask will not be applied.\n";
            applyMask = false;
        }
    }

    // load palette
    if (clo_getBool(optionList, "apply_pal")) {
        clo_option_t* option = clo_findOption(optionList, "palfile");
        if (clo_isOptionSet(option)) {
            outFile->setPalette(clo_getOptionString(option), applyMask);
            if (outFile2)
                outFile2->setPalette(clo_getOptionString(option), applyMask);
        } else {
            outFile->setPalette(p_info->palette, applyMask);
            if (outFile2)
                outFile2->setPalette(p_info->palette, applyMask);
        }
    }

    // set the default scale factors for RGB
    if (doingRGB) {
        p_info->displayMin = 0.01;
        p_info->displayMax = 0.9;
        if (p_info->displayScale)
            free(p_info->displayScale);
        p_info->displayScale = strdup("log");
    }

    // override default scale parameters if set on command line
    if (clo_isSet(optionList, "datamin")) {
        p_info->displayMin = clo_getFloat(optionList, "datamin");
    }
    if (clo_isSet(optionList, "datamax")) {
        p_info->displayMax = clo_getFloat(optionList, "datamax");
    }
    if (clo_isSet(optionList, "scale_type")) {
        if (p_info->displayScale)
            free(p_info->displayScale);
        p_info->displayScale = strdup(clo_getString(optionList, "scale_type"));
    }

    outFile->addProduct(p_info);
    if (outFile2)
        outFile2->addProduct(p_info);

    freeProductInfo(p_info);
}

void writeRawFile(L3File* l3File, vector<OutFile*> outFiles, OutFile* outFile2) {
    L3Bin* l3Bin;
    L3Row* l3Row;
    imageHeight = l3File->getNumRows();
    double resolution = EARTH_CIRCUMFERENCE / (imageHeight * 2);
    imageWidth = imageHeight * 2;
    int32_t start;
    int32_t numBins;
    int64_t baseBin;
    int64_t endBin;
    int64_t binNum;
    int32_t row, col;

    int32_t numFilledPixels = 0;
    meta_l3bType* metaData = outFiles[0]->getMetadata();

    string mapDesc = "Bin";
    string projName = "Integerized Sinusoidal";

    sprintf(metaData->title, "%s Level-3 %s Mapped Image",
            metaData->sensor_name, mapDesc.c_str());
    for(OutFile* outFile : outFiles) {
        strcpy(outFile->getMetadata()->title, metaData->title);
        outFile->setFullLatLon(false);
        outFile->setMapProjection(projName);
        outFile->setResolution(resolution);
        outFile->setSize(imageWidth, imageHeight);
    }
    if (outFile2) {
        strcpy(outFile2->getMetadata()->title, metaData->title);
        outFile2->setMapProjection(projName);
        outFile2->setResolution(resolution);
        outFile2->setSize(imageWidth, imageHeight);
    }

    // setup all the product structures
    for (size_t i = 0; i < prodNameList.size(); i++) {
        OutFile* outFile;
        if(outFiles.size() == 1)
            outFile = outFiles[0];
        else
            outFile = outFiles[i];
        setupProduct(prodNameList[i], prodMeasurementList[i], outFile,
                     outFile2);
    }

    printStartInfo(outFiles);

    
    for(OutFile* outFile : outFiles) {
        if (!outFile->open()) {
            printf("-E- Could not open ofile=\"%s\".\n",
                   outFile->getFileName().c_str());
            exit(EXIT_FAILURE);
        }
    }
    if (outFile2) {
        if (!outFile2->open()) {
            printf("-E- Could not open ofile2=\"%s\".\n",
                   outFile2->getFileName().c_str());
            exit(EXIT_FAILURE);
        }
    }

    float centralMeridian = getCentralMeridian();

    for (row = imageHeight - 1; row >= 0; row--) {
        printPercentDone(1.0 - (float) row / (float) imageHeight);

        l3Row = l3File->getRow(row);
        numBins = l3File->getShape()->getNumCols(row);
        baseBin = l3File->getShape()->getBaseBin(row);
        endBin = baseBin + numBins;
        if (centralMeridian < 0)
            binNum = baseBin + numBins * (centralMeridian + 360.0) / 360.0;
        else
            binNum = baseBin + numBins * centralMeridian / 360.0;
        start = (imageWidth - numBins) / 2;

        // clear out beginning empty pixels
        for (col = 0; col < start; col++) {
            for(OutFile* outFile : outFiles)
                outFile->fillPixel(col);
            if (outFile2)
                outFile2->fillPixel(col);
        }

        // set pixel values
        for (int i = 0; i < numBins; i++) {
            l3Bin = l3Row->getBin(binNum);
            if (l3Bin) {
                if (doingRGB) {
                    outFiles[0]->setPixelRGB(col, l3Bin->getMean(0),
                                         l3Bin->getMean(1), l3Bin->getMean(2));
                    if (outFile2)
                        outFile2->setPixelRGB(col, l3Bin->getMean(0),
                                              l3Bin->getMean(1), l3Bin->getMean(2));
                } else {
                    for (size_t prod = 0; prod < prodNameList.size(); prod++) {
                        float val;
                        switch (prodMeasurementList[prod]) {
                        case Avg:
                            val = l3Bin->getMean(prod);
                            break;
                        case Stdev:
                            val = l3Bin->getStdev(prod);
                            break;
                        case Variance:
                            val = l3Bin->getVariance(prod);
                            break;
                        case Nobs:
                            val = l3Bin->getNobs();
                            break;
                        case Nscenes:
                            val = l3Bin->getNscenes();
                            break;
                        case ObsTime:
                            val = l3Bin->getObsTime();
                            break;
                        case BinNum:
                            val = l3Bin->getBinNum();
                            break;
                        default:
                            val = l3Bin->getMean(prod);
                        }
                        if(outFiles.size() == 1)
                            outFiles[0]->setPixel(col, val, prod);
                        else
                            outFiles[prod]->setPixel(col, val, 0);
                        if (outFile2)
                            outFile2->setPixel(col, val, prod);
                    }
                }
                numFilledPixels++;
            } else {
                for(OutFile* outFile : outFiles)
                    outFile->missingPixel(col);
                if (outFile2)
                    outFile2->missingPixel(col);
            }
            col++;
            binNum++;
            if (binNum >= endBin)
                binNum = baseBin;
        }

        // clear out trailing empty pixels
        for (; col < imageWidth; col++) {
            for(OutFile* outFile : outFiles)
                outFile->fillPixel(col);
            if (outFile2)
                outFile2->fillPixel(col);
        }

        for(OutFile* outFile : outFiles)
            outFile->writeLine();
        if (outFile2)
            outFile2->writeLine();
    }

    for(OutFile* outFile : outFiles) {
        outFile->setNumFilledPixels(numFilledPixels);
        outFile->close();
    }
    if (outFile2) {
        outFile2->setNumFilledPixels(numFilledPixels);
        outFile2->close();
    }
}

void writeSmiFile(L3File* l3File, vector<OutFile*> outFiles, OutFile* outFile2) {

    int32_t numFilledPixels = 0;
    meta_l3bType* metaData = outFiles[0]->getMetadata();
    double resolution = outFiles[0]->getResolution();

    string mapDesc = "Standard";
    string projName = "Equidistant Cylindrical";

    sprintf(metaData->title, "%s Level-3 %s Mapped Image",
            metaData->sensor_name, mapDesc.c_str());

    // set up image parameters
    if (imageWidth <= 0) {
        imageWidth = rint(widthInDeg / 360.0 * EARTH_CIRCUMFERENCE / resolution);
        if(imageWidth == 0)
            imageWidth = 1;
    } else {
        resolution = widthInDeg / 360.0 * EARTH_CIRCUMFERENCE / imageWidth;
        for(OutFile* outFile : outFiles)
            outFile->setResolution(resolution);
        if (outFile2)
            outFile2->setResolution(resolution);
    }
    imageHeight = rint(heightInDeg / 360.0 * EARTH_CIRCUMFERENCE / resolution);
    if(imageHeight == 0)
        imageHeight = 1;
    double deltaLon = widthInDeg / imageWidth;
    double deltaLat = heightInDeg / imageHeight;

    for(OutFile* outFile : outFiles) {
        strcpy(outFile->getMetadata()->title, metaData->title);    
        outFile->setSize(imageWidth, imageHeight);
        outFile->setMapProjection(projName);
    }
    if (outFile2) {
        strcpy(outFile2->getMetadata()->title, metaData->title);
        outFile2->getMetadata()->east = metaData->east;
        outFile2->getMetadata()->west = metaData->west;
        outFile2->setSize(imageWidth, imageHeight);
        outFile2->setMapProjection(projName);
    }

    // set up all the product structures
    for (size_t i = 0; i < prodNameList.size(); i++) {
        OutFile* outFile;
        if(outFiles.size() == 1)
            outFile = outFiles[0];
        else
            outFile = outFiles[i];
        setupProduct(prodNameList[i], prodMeasurementList[i], outFile,
                     outFile2);
    }

    // set up quality processing
   setupQualityProcessing(l3File, outFiles, outFile2);

    printStartInfo(outFiles);

    for(OutFile* outFile : outFiles) {
        if (!outFile->open()) {
            printf("-E- Could not open ofile=\"%s\".\n",
                   outFile->getFileName().c_str());
            exit(EXIT_FAILURE);
        }
    }

    if (outFile2) {
        if (!outFile2->open()) {
            printf("-E- Could not open ofile2=\"%s\".\n",
                   outFile2->getFileName().c_str());
            exit(EXIT_FAILURE);
        }
    }

    InterpType interp = interpStr2Type(clo_getString(optionList, "interp"));
    float fudge = clo_getFloat(optionList, "fudge");

    // loop through output pixels
    double lat = metaData->north - (deltaLat / 2.0);
    L3Bin* l3Bin;

    for (int row = 0; row < imageHeight; row++) {
        printPercentDone((float) row / (float) imageHeight);
        double lon = metaData->west + (deltaLon / 2.0);

        for (int col = 0; col < imageWidth; col++) {
            if (applyMask && isLand(lat, lon)) {
                for(OutFile* outFile : outFiles)
                    outFile->landPixel(col);
                if (outFile2)
                    outFile2->landPixel(col);

            } else {
                switch (interp) {
                case Interp_Nearest:
                    l3Bin = l3File->getClosestBin(lat, lon);
                    break;
                case Interp_Bin:
                case Interp_Area:
                    {
                        bool areaWeighted;

                        if (interp == Interp_Area)
                            areaWeighted = true;
                        else
                            areaWeighted = false;

                        l3Bin = getBoxBins(l3File, lat, lon, deltaLat, deltaLon, fudge, areaWeighted);

                        break;
                    }
                default:
                    printf("-E- interp = %s is not implemented.",
                           interpType2Str(interp));
                    exit(EXIT_FAILURE);
                }

                if (l3Bin) {
                    numFilledPixels++;
                    if (doingRGB) {
                        outFiles[0]->setPixelRGB(col, l3Bin->getMean(0),
                                             l3Bin->getMean(1), l3Bin->getMean(2));
                        if (outFile2)
                            outFile2->setPixelRGB(col, l3Bin->getMean(0),
                                                  l3Bin->getMean(1), l3Bin->getMean(2));
                    } else {
                        for (size_t prod = 0; prod < prodNameList.size();
                             prod++) {
                            float val;
                            switch (prodMeasurementList[prod]) {
                            case Avg:
                                val = l3Bin->getMean(prod);
                                break;
                            case Stdev:
                                val = l3Bin->getStdev(prod);
                                break;
                            case Variance:
                                val = l3Bin->getVariance(prod);
                                break;
                            case Nobs:
                                val = l3Bin->getNobs();
                                break;
                            case Nscenes:
                                val = l3Bin->getNscenes();
                                break;
                            case ObsTime:
                                val = l3Bin->getObsTime();
                                break;
                            case BinNum:
                                val = l3Bin->getBinNum();
                                break;
                            default:
                                val = l3Bin->getMean(prod);
                            }
                            if(outFiles.size() == 1)
                                outFiles[0]->setPixel(col, val, prod);
                            else
                                outFiles[prod]->setPixel(col, val, 0);
                            if (outFile2)
                                outFile2->setPixel(col, val, prod);
                        }
                    }
                    if (doingQuality) {
                        for(OutFile* outFile : outFiles)
                            outFile->setQuality(col, l3Bin->getQuality());
                        if (outFile2)
                            outFile2->setQuality(col, l3Bin->getQuality());
                    }
                } else {
                    for(OutFile* outFile : outFiles)
                        outFile->missingPixel(col);
                    if (outFile2)
                        outFile2->missingPixel(col);
                }
            }
            lon += deltaLon;
        } // for col
        for(OutFile* outFile : outFiles)
            outFile->writeLine();
        if (outFile2)
            outFile2->writeLine();
        lat -= deltaLat;
    } // for row

    for(OutFile* outFile : outFiles) {
        outFile->setNumFilledPixels(numFilledPixels);
        outFile->close();
    }
    if (outFile2) {
        outFile2->setNumFilledPixels(numFilledPixels);
        outFile2->close();
    }
}

void writeProj4File(L3File* l3File, char* projectionStr, vector<OutFile*> outFiles,
                    OutFile* outFile2, bool trimNSEW) {

    int32_t numFilledPixels = 0;
    meta_l3bType* metaData = outFiles[0]->getMetadata();
    double resolution = outFiles[0]->getResolution();

    // how about circumference of earth + 25%
    double limitMin = EARTH_CIRCUMFERENCE * -0.625;
    double limitMax = EARTH_CIRCUMFERENCE * 0.625;

    // parse central meridian (lon_0)
    float centralMeridian = getCentralMeridian();
    char cmStr[50];
    char tsStr[50];
    char lat0Str[50];
    char aziStr[50];

    sprintf(cmStr, " +lon_0=%f", centralMeridian);

    if (clo_isSet(optionList, "lat_ts")) {
        float latitudeTrueScale = clo_getFloat(optionList, "lat_ts");
        sprintf(tsStr, " +lat_ts=%f", latitudeTrueScale);
    }
    if (clo_isSet(optionList, "lat_0")) {
        float latitudeOfOrigin = clo_getFloat(optionList, "lat_0");
        sprintf(lat0Str, " +lat_0=%f", latitudeOfOrigin);
    }
    if (clo_isSet(optionList, "azimuth")) {
        float azimuthAngle = clo_getFloat(optionList, "azimuth");
        sprintf(aziStr, " +alpha=%f", azimuthAngle);
    }

    // define proj.4 parameters according to shortcut
    string mapDesc;
    string projName;
    string projStr;

    if (strcasecmp(projectionStr, "mollweide") == 0) {
        mapDesc = "Mollweide";
        projName = "Mollweide";
        projStr = "+proj=moll";
        projStr += cmStr;

    } else if (strcasecmp(projectionStr, "lambert") == 0) {
        mapDesc = "Lambert";
        projName = "Lambert";
        projStr = "+proj=lcc";
        projStr += cmStr;

    } else if (strcasecmp(projectionStr, "albersconic") == 0) {
        mapDesc = "Albers Equal Area Conic";
        projName = "Albersconic";
        projStr = "+proj=aea";
        projStr += cmStr;

    } else if (strcasecmp(projectionStr, "mercator") == 0) {
        mapDesc = "Mercator";
        projName = "Mercator";
        projStr = "+proj=merc";
        projStr += cmStr;

    } else if (strcasecmp(projectionStr, "obliquemerc") == 0) {
        if (!clo_isSet(optionList, "lat_0") || !clo_isSet(optionList, "azimuth")) {
            printf("-E- lat_0 and azimuth need to be defined for obliquemerc projection");
            exit(EXIT_FAILURE);
        }
        mapDesc = "Oblique Mercator";
        projName = "ObliqueMercator";
        projStr = "+proj=omerc +gamma=0 +k_0=1 +x_0=0 +y_0=0";
        projStr += lat0Str;
        sprintf(cmStr, " +lonc=%f", centralMeridian);
        projStr += cmStr;
        projStr += aziStr;

    } else if (strcasecmp(projectionStr, "ease2") == 0) {
        mapDesc = "Ease Grid 2";
        projName = "Ease2";
        projStr = "+proj=cea +lat_0=0 +lat_ts=30"
                " +ellps=WGS84 +datum=WGS84 +units=m +no_defs";
        projStr += cmStr;

    } else if (strcasecmp(projectionStr, "stere") == 0) {
        if (!clo_isSet(optionList, "lat_0") || !clo_isSet(optionList, "lat_ts")) {
            printf("-E- lat_0 and lat_ts need to be defined for stere projection");
            exit(EXIT_FAILURE);
        }
        mapDesc = "Stereographic";
        projName = "Stereo";
        projStr = "+proj=stere "
                " +ellps=WGS84 +datum=WGS84 +units=m +no_defs";
        projStr += cmStr;
        projStr += tsStr;
        projStr += lat0Str;

    } else if (strcasecmp(projectionStr, "conus") == 0) {
        mapDesc = "USA Contiguous Albers Equal Area Conic USGS version";
        projName = "Conus";
        projStr = "+proj=aea +lat_1=29.5 +lat_2=45.5"
                " +lat_0=23.0 +lon_0=-96 +x_0=0 +y_0=0"
                " +ellps=GRS80 +datum=NAD83 +units=m +no_defs";

    } else if (strcasecmp(projectionStr, "alaska") == 0) {
        mapDesc = "Alaska Albers Equal Area Conic USGS version";
        projName = "Alaska";
        projStr = "+proj=aea +lat_1=55 +lat_2=65"
                " +lat_0=50 +lon_0=-154 +x_0=0 +y_0=0"
                " +ellps=GRS80 +datum=NAD83 +units=m +no_defs";

    } else if (strcasecmp(projectionStr, "gibs") == 0) {
        if (((metaData->north + metaData->south) / 2.) > 60.) {
            mapDesc = "Stereographic";
            projName = "GIBS Stereo";
            projStr = "+init=epsg:3413";
//            projStr = "+proj=stere +lat_0=90"
//                    " +lat_ts=70 +lon_0=-45 +k=1 +x_0=0 +y_0=0"
//                    " +ellps=WGS84 +datum=WGS84 +units=m +no_defs ";
        } else if (((metaData->north + metaData->south) / 2.) < -60.) {
            mapDesc = "Stereographic";
            projName = "GIBS Stereo";
              projStr = "+init=epsg:3031";
//            projStr = "+proj=stere +lat_0=-90"
//                    " +lat_ts=-71 +lon_0=0 +k=1 +x_0=0 +y_0=0"
//                    " +ellps=WGS84 +datum=WGS84 +units=m +no_defs";
        } else {
            mapDesc = "Equidistant Cylindrical";
            projName = "PlateCarre";
            projStr = "+proj=eqc"
                    " +lat_ts=0 +lat_0=0 +x_0=0 +y_0=0"
                    " +ellps=WGS84 +datum=WGS84 +units=m +no_defs";
            projStr += cmStr;
        }

    } else if (strcasecmp(projectionStr, "platecarre") == 0) {
        mapDesc = "Equidistant Cylindrical";
        projName = "PlateCarre";
        projStr = "+proj=eqc"
                " +lat_ts=0 +lat_0=0 +x_0=0 +y_0=0"
                " +ellps=WGS84 +datum=WGS84 +units=m +no_defs";
        projStr += cmStr;

    } else {
        mapDesc = "Proj4";
        projName = projectionStr;
        projStr = projectionStr;
    }

    // save parameters in metadata
    sprintf(metaData->title, "%s Level-3 %s Mapped Image",
            metaData->sensor_name, mapDesc.c_str());
    for(OutFile* outFile : outFiles) {
        strcpy(outFile->getMetadata()->title, metaData->title);
        outFile->setMapProjection(projName);
    }

    if (outFile2) {
        strcpy(outFile2->getMetadata()->title, metaData->title);
        outFile2->getMetadata()->east = metaData->east;
        outFile2->getMetadata()->west = metaData->west;
        outFile2->setMapProjection(projName);
    }

    projPJ pj_new = pj_init_plus(projStr.c_str());
    if (pj_new == NULL) {
        printf("-E- %s:%d - pj_init_plus(\"%s\") failed.\n",
               __FILE__, __LINE__, projStr.c_str());
        printf("     pj_errno=%d, %s\n", pj_errno, pj_strerrno(pj_errno));
        exit(EXIT_FAILURE);
    }

    projPJ pj_latlong = pj_latlong_from_proj(pj_new);
    if (pj_latlong == NULL) {
        printf("-E- %s:%d - pj_latlong_from_proj(\"%s\") failed.\n",
               __FILE__, __LINE__, projStr.c_str());
        printf("     pj_errno=%d, %s\n", pj_errno, pj_strerrno(pj_errno));
        exit(EXIT_FAILURE);
    }

    // calculate the min and max
    double minX = limitMax;
    double minY = limitMax;
    double maxX = limitMin;
    double maxY = limitMin;
    double *tmpX, *tmpY;
    bool *inBox;
    double lat, lon;
    double deltaLat = (metaData->north - metaData->south)
        / (NUM_SEARCH_POINTS - 1);
    double deltaLon = (mapEast - mapWest)
        / (NUM_SEARCH_POINTS - 1);

    tmpX = (double*) allocateMemory(NUM_SEARCH_POINTS * sizeof (double), "tmpX");
    tmpY = (double*) allocateMemory(NUM_SEARCH_POINTS * sizeof (double), "tmpY");

    lat = metaData->south;
    for (int j = 0; j < NUM_SEARCH_POINTS; j++) {
        lon = metaData->west;
        for (int i = 0; i < NUM_SEARCH_POINTS; i++) {
            tmpX[i] = lon * DEG_TO_RAD; // convert to radians
            tmpY[i] = lat * DEG_TO_RAD;
            lon += deltaLon;
        }
        if (pj_transform(pj_latlong, pj_new, NUM_SEARCH_POINTS, 1, tmpX, tmpY,
                         NULL)) {
            printf("Error - min/max proj4 transformation blew up\n");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < NUM_SEARCH_POINTS; i++) {
            if (isnormal(tmpX[i]) && isnormal(tmpY[i])) {
                if (tmpX[i] < limitMax && tmpX[i] > limitMin) {
                    if (tmpX[i] < minX)
                        minX = tmpX[i];
                    if (tmpX[i] > maxX)
                        maxX = tmpX[i];
                }
                if (tmpY[i] < limitMax && tmpY[i] > limitMin) {
                    if (tmpY[i] < minY)
                        minY = tmpY[i];
                    if (tmpY[i] > maxY)
                        maxY = tmpY[i];
                }
            }
        }

        lat += deltaLat;
    }
    free(tmpX);
    free(tmpY);

    // set up image parameters
    if (imageWidth <= 0) {
        imageWidth = rint((maxX - minX) / resolution);
    } else {
        resolution = (maxX - minX) / imageWidth;
        for(OutFile* outFile : outFiles)
            outFile->setResolution(resolution);
        if (outFile2)
            outFile2->setResolution(resolution);
    }
    imageHeight = rint((maxY - minY) / resolution);
    for(OutFile* outFile : outFiles)
        outFile->setSize(imageWidth, imageHeight);
    if (outFile2)
        outFile2->setSize(imageWidth, imageHeight);

    // expand Proj4 string; preserve metadata for output
    char *pj_def = pj_get_def(pj_new, 0);
    for(OutFile* outFile : outFiles)
        outFile->setProj4Info(pj_def, minX, maxY);
    if (outFile2) {
        outFile2->setProj4Info(pj_def, minX, maxY);
    }

    // set up all the product structures
    for (size_t i = 0; i < prodNameList.size(); i++) {
        OutFile* outFile;
        if(outFiles.size() == 1)
            outFile = outFiles[0];
        else
            outFile = outFiles[i];
        setupProduct(prodNameList[i], prodMeasurementList[i], outFile,
                     outFile2);
    }

    // set up quality processing
    setupQualityProcessing(l3File, outFiles, outFile2);

    printStartInfo(outFiles);

    for(OutFile* outFile : outFiles) {
        if (!outFile->open()) {
            printf("-E- Could not open ofile=\"%s\".\n",
                   outFile->getFileName().c_str());
            exit(EXIT_FAILURE);
        }
    }

    if (outFile2) {
        if (!outFile2->open()) {
            printf("-E- Could not open ofile2=\"%s\".\n",
                   outFile2->getFileName().c_str());
            exit(EXIT_FAILURE);
        }
    }

    InterpType interp = interpStr2Type(clo_getString(optionList, "interp"));
    float fudge = clo_getFloat(optionList, "fudge");

    // loop through output pixels
    tmpX = (double*) allocateMemory(imageWidth * sizeof (double), "tmpX");
    tmpY = (double*) allocateMemory(imageWidth * sizeof (double), "tmpY");
    inBox = (bool*) allocateMemory(imageWidth * sizeof (bool), "inBox");

    deltaLon = widthInDeg / imageWidth;
    deltaLat = heightInDeg / imageHeight;
    double startX = minX + resolution / 2;
    double startY = maxY - resolution / 2;
    double y = startY;
    L3Bin* l3Bin;

    for (int row = 0; row < imageHeight; row++) {
        printPercentDone((float) row / (float) imageHeight);
        double x = startX;

        // create lon & lat values for each pixel in this row
        for (int col = 0; col < imageWidth; col++) {
            tmpX[col] = x;
            tmpY[col] = y;
            x += resolution;
        }
        if (pj_transform(pj_new, pj_latlong, imageWidth, 1, tmpX, tmpY, NULL)) {
            printf("Error - min/max proj4 transformation blew up\n");
            exit(EXIT_FAILURE);
        }
        for (int col = 0; col < imageWidth; col++) {
            if(isfinite(tmpX[col]))
                tmpX[col] *= RAD_TO_DEG; // convert to degrees
            else
                tmpX[col] = -999.0; // set to fill value

            if(isfinite(tmpY[col]))
                tmpY[col] *= RAD_TO_DEG;
            else
                tmpY[col] = -999.0; // set to fill value
        }

        // find line start and end points
        int startPoint = 0;
        int endPoint = imageWidth;
        if (trimNSEW) {
            std::fill(inBox, inBox + imageWidth, false);

            float west = constrainLon(metaData->west);

            Point_t pMin(0, metaData->south);
            Point_t pMax(widthInDeg, metaData->north);
            Box_t box(pMin, pMax);

            for (startPoint = imageWidth / 2; startPoint >= 0; startPoint--) {
                lon = constrainLon(tmpX[startPoint] - west);
                if (lon < 0)
                    lon += 180;
                lat = tmpY[startPoint];
                if (isnormal(lon) && isnormal(lat)) {
                    Point_t pixel(lon, lat);

                    if (!boost::geometry::within(pixel, box)) {
                        break;
                    } else {
                        inBox[startPoint] = true;
                    }
                }
            }
            startPoint++;
            for (endPoint = imageWidth / 2; endPoint < imageWidth; endPoint++) {
                lon = constrainLon(tmpX[endPoint] - west);
                if (lon < 0)
                    lon += 180;
                lat = tmpY[endPoint];
                if (isnormal(lon) && isnormal(lat)) {
                    Point_t pixel(lon, lat);

                    if (!boost::geometry::within(pixel, box)) {
                        break;
                    } else {
                        inBox[endPoint] = true;
                    }
                }
            }
            endPoint--;
        }

        // loop through each pixel in this row
        for (int col = 0; col < imageWidth; col++) {
            lon = tmpX[col];
            lat = tmpY[col];
            if (col < startPoint || col > endPoint) {
                for(OutFile* outFile : outFiles)
                    outFile->fillPixel(col);
                if (outFile2)
                    outFile2->fillPixel(col);
            } else if (!isnormal(lon) || !isnormal(lat)) {
                for(OutFile* outFile : outFiles)
                    outFile->fillPixel(col);
                if (outFile2)
                    outFile2->fillPixel(col);
            } else if (trimNSEW && inBox[col] == false) {
                for(OutFile* outFile : outFiles)
                    outFile->fillPixel(col);
                if (outFile2)
                    outFile2->fillPixel(col);
            } else if (applyMask && isLand(lat, lon)) {
                for(OutFile* outFile : outFiles)
                    outFile->landPixel(col);
                if (outFile2)
                    outFile2->landPixel(col);

            } else {
                switch (interp) {
                case Interp_Nearest:
                    l3Bin = l3File->getClosestBin(lat, lon);
                    break;
                case Interp_Bin:
                case Interp_Area:
                    {
                        bool areaWeighted;

                        if (interp == Interp_Area)
                            areaWeighted = true;
                        else
                            areaWeighted = false;

                        if (checkDateLineCrossed(lon, deltaLon)) {
                            // East
                            l3Bin = getBoxBins(l3File, lat, lon, deltaLat, deltaLon, fudge, areaWeighted, IsEast);

                            // West
                            L3Bin* tmpBin = getBoxBins(l3File, lat, lon, deltaLat, deltaLon, fudge, areaWeighted, IsWest);
                            if (tmpBin) {
                                *l3Bin += *tmpBin;
                            }

                        } else {
                            l3Bin = getBoxBins(l3File, lat, lon, deltaLat, deltaLon, fudge, areaWeighted);
                        }
                        break;
                    }
                default:
                    printf("-E- interp = %s is not implemented.",
                           interpType2Str(interp));
                    exit(EXIT_FAILURE);
                }

                if (l3Bin) {
                    numFilledPixels++;
                    if (doingRGB) {
                        outFiles[0]->setPixelRGB(col, l3Bin->getMean(0),
                                             l3Bin->getMean(1), l3Bin->getMean(2));
                        if (outFile2)
                            outFile2->setPixelRGB(col, l3Bin->getMean(0),
                                                  l3Bin->getMean(1), l3Bin->getMean(2));
                    } else {
                        for (size_t prod = 0; prod < prodNameList.size();
                             prod++) {
                            float val;
                            switch (prodMeasurementList[prod]) {
                            case Avg:
                                val = l3Bin->getMean(prod);
                                break;
                            case Stdev:
                                val = l3Bin->getStdev(prod);
                                break;
                            case Variance:
                                val = l3Bin->getVariance(prod);
                                break;
                            case Nobs:
                                val = l3Bin->getNobs();
                                break;
                            case Nscenes:
                                val = l3Bin->getNscenes();
                                break;
                            case ObsTime:
                                val = l3Bin->getObsTime();
                                break;
                            case BinNum:
                                val = l3Bin->getBinNum();
                                break;
                            default:
                                val = l3Bin->getMean(prod);
                            }
                            if(outFiles.size() == 1)
                                outFiles[0]->setPixel(col, val, prod);
                            else
                                outFiles[prod]->setPixel(col, val, 0);
                            if (outFile2)
                                outFile2->setPixel(col, val, prod);
                        }
                    }
                    if (doingQuality) {
                        for(OutFile* outFile : outFiles)
                            outFile->setQuality(col, l3Bin->getQuality());
                        if (outFile2)
                            outFile2->setQuality(col, l3Bin->getQuality());
                    }
                } else {
                    for(OutFile* outFile : outFiles)
                        outFile->missingPixel(col);
                    if (outFile2)
                        outFile2->missingPixel(col);
                }
            }
        } // for col
        for(OutFile* outFile : outFiles) {
            outFile->setLatLon(tmpY,tmpX);
            outFile->writeLine();
        }
        if (outFile2) {
            outFile2->setLatLon(tmpY,tmpX);
            outFile2->writeLine();
        }
        y -= resolution;
    } // for row

    free(tmpX);
    free(tmpY);
    free(inBox);
    pj_free(pj_new);
    pj_free(pj_latlong);
    for(OutFile* outFile : outFiles) {
        outFile->setNumFilledPixels(numFilledPixels);
        outFile->close();
    }
    if (outFile2) {
        outFile2->setNumFilledPixels(numFilledPixels);
        outFile2->close();
    }
}

OutFile* makeOutputFile(const char* oformatStr, bool useColor) {
    OutFile* outFile = NULL;

    const char* oformatStr2 = getFileFormatName(oformatStr);

    if (oformatStr2 == NULL) {
        printf("-E- Unknown output file format \"%s\"\n", oformatStr);
        exit(EXIT_FAILURE);
    }

    string oformat = oformatStr2;
    if (oformat.compare("PPM") == 0) {
        if (doingRGB) {
            outFile = new OutFile_ppm_rgb();
        } else {
            if (useColor) {
                outFile = new OutFile_ppm();
            } else {
                outFile = new OutFile_pgm();
            }
        }
    } else if (oformat.compare("PNG") == 0) {
        if (doingRGB) {
            outFile = new OutFile_png_rgb();
        } else {
            outFile = new OutFile_png(useColor);
        }
    } else if (oformat.compare("TIFF") == 0) {
        if (doingRGB) {
            outFile = new OutFile_tiff_rgb();
        } else {
            if (useColor) {
                outFile = new OutFile_tiff_color();
            } else {
                outFile = new OutFile_tiff_gray();
            }
        }
    } else if (oformat.compare("HDF4") == 0) {
        outFile = new OutFile_hdf4();
    } else if (oformat.compare("netCDF4") == 0) {
        outFile = new OutFile_netcdf4();
    } else {
        printf("-E- Output file type %s not implemented\n", oformat.c_str());
        exit(EXIT_FAILURE);
    }
    if(doingTransparency)
        outFile->setTransparency();

    return outFile;
}

vector<OutFile*> makeOutputFileList(clo_optionList_t* optionList) {
    vector<OutFile*> outFiles;
    OutFile* outFile;
    string oformatStr = getFileFormatName(clo_getString(optionList, "oformat"));
    string originalOfile = clo_getString(optionList, "ofile");
    string tag = clo_getString(optionList, "ofile_product_tag");

    if (oformatStr.compare("netCDF4") == 0  ||
            prodNameList.size() == 1 || doingRGB) {
        outFile = makeOutputFile(clo_getString(optionList, "oformat"),
                clo_getBool(optionList, "apply_pal"));
        outFile->setFileName(originalOfile);
        outFiles.push_back(outFile);
    } else {
        size_t pos = originalOfile.find(tag);
        if(pos == string::npos) {
            printf("Error: ofile_product_tag=%s, not found in ofile=%s\n", 
                    tag.c_str(), originalOfile.c_str());
            printf("       and you asked for multiple products with image oformat\n");
            exit(EXIT_FAILURE);
        }
 
        for(string &prodName : prodNameList) {
            string newName = originalOfile;
            newName.replace(pos, tag.size(), prodName);
            outFile = makeOutputFile(clo_getString(optionList, "oformat"),
                             clo_getBool(optionList, "apply_pal"));
            outFile->setFileName(newName);
            outFiles.push_back(outFile);
        } // loop products
    }
    
    return outFiles;
}
    
    
    



//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    vector<OutFile*> outFiles;
    OutFile* outFile2 = NULL;
    char* ifileName;
    string oformat;
    int i;
    char* tmpStr;

    char softwareVersion[200];
    sprintf(softwareVersion, "%d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR,
            VERSION_PATCH, GITSHA);

    optionList = clo_createList();

    l3mapgen_init_options(optionList, softwareVersion);
    if (argc == 1) {
        clo_printUsage(optionList);
        exit(EXIT_FAILURE);
    }
    l3mapgen_read_options(optionList, argc, argv);

    if (clo_getBool(optionList, "quiet")) {
        want_verbose = 0;
    }

    ifileName = clo_getString(optionList, "ifile");

    // try SMI input file
    int oldVerbose = want_verbose;
    want_verbose = 0;
    L3File* l3File = new L3FileSMI();
    if (!l3File->open(ifileName)) {

        // try real L3 bin format
        delete l3File;
        l3File = new L3File();
        if (!l3File->open(ifileName)) {
            printf("-E- Could not open ifile=\"%s\".\n", ifileName);
            exit(EXIT_FAILURE);
        }
    }
    want_verbose = oldVerbose;

    if (clo_getBool(optionList, "use_transparency")) {
        doingTransparency = true;
    }
    if (clo_getBool(optionList, "use_rgb")) {
        doingRGB = true;
        prodName = clo_getRawString(optionList, "product_rgb");
    } else {
        doingRGB = false;
        if (clo_isSet(optionList, "product")) {
            prodName = clo_getRawString(optionList, "product");
        } else {
            prodName.clear();
            for (int i = 0; i < l3File->getNumProducts(); i++) {
                string tmpName = l3File->getProductName(i);
                if (tmpName != "qual_l3") {
                    if (!prodName.empty())
                        prodName += ",";
                    prodName += tmpName;
                }
            }
        }
    }
    trimNSEW = clo_getBool(optionList, "trimNSEW");

    boost::split(prodNameList, prodName, boost::is_any_of(","));
    string cleanProdName;
    vector<string> parts;
    for (size_t i = 0; i < prodNameList.size(); i++) {
        if (i != 0)
            cleanProdName += ",";
        boost::split(parts, prodNameList[i], boost::is_any_of(":"));
        if (parts.size() == 1) {
            cleanProdName += parts[0];
            prodMeasurementList.push_back(Avg);
        } else if (parts.size() == 2) {
            prodNameList[i] = parts[0]; // get rid of the modifier
            cleanProdName += parts[0];
            if (parts[1].compare("avg") == 0)
                prodMeasurementList.push_back(Avg);
            else if (parts[1].compare("stdev") == 0)
                prodMeasurementList.push_back(Stdev);
            else if (parts[1].compare("var") == 0)
                prodMeasurementList.push_back(Variance);
            else if (parts[1].compare("nobs") == 0)
                prodMeasurementList.push_back(Nobs);
            else if (parts[1].compare("nscenes") == 0)
                prodMeasurementList.push_back(Nscenes);
            else if (parts[1].compare("obs_time") == 0)
                prodMeasurementList.push_back(ObsTime);
            else if (parts[1].compare("bin_num") == 0)
                prodMeasurementList.push_back(BinNum);
            else {
                printf("-E- measurement type \"%s\" "
                       "not understood for product \"%s\".\n",
                       parts[1].c_str(), parts[0].c_str());
                exit(EXIT_FAILURE);
            }
        } else {
            printf("-E- product name not understood \"%s\".\n",
                   prodNameList[i].c_str());
            exit(EXIT_FAILURE);
        }
    }

    if (!l3File->setActiveProductList(cleanProdName.c_str())) {
        printf("-E- Could not find product=\"%s\" in file=\"%s\".\n",
               cleanProdName.c_str(), ifileName);
        exit(EXIT_FAILURE);
    }

    l3File->setNumCacheRows(clo_getInt(optionList, "num_cache"));

    // copy the L3 meta data since we will modify it a bit for the output file.
    meta_l3bType metaData = *l3File->getMetaData();

    outFiles = makeOutputFileList(optionList);

    if (clo_isSet(optionList, "ofile2")) {
        outFile2 = makeOutputFile(clo_getString(optionList, "oformat2"),
                                  clo_getBool(optionList, "apply_pal"));
        outFile2->setFileName(clo_getString(optionList, "ofile2"));
    }

    // resolution = # of meters across 1 pixel in the center of the scene
    double res;
    if (clo_isSet(optionList, "resolution")) {
        outFiles[0]->setResolution(clo_getString(optionList, "resolution"));        
        res = outFiles[0]->getResolution();
    } else {
        res = l3File->getMetaData()->geospatial_resolution; // in deg
        if(res > 0) {
            // convert to meters
            res = res / 360.0 * EARTH_CIRCUMFERENCE;
        } else {
            outFiles[0]->setResolution("9km");
            res = outFiles[0]->getResolution();
        }
    }
     
    for(OutFile* outFile : outFiles) {
        outFile->setResolution(res);
    }
    
    if (outFile2)
        outFile2->setResolution(res);

    // get imageWidth, if specified
    if (clo_isSet(optionList, "width"))
        imageWidth = clo_getInt(optionList, "width");

    // projection
    clo_option_t* projectionOption = clo_findOption(optionList, "projection");
    char* projectionStr = clo_getOptionRawString(projectionOption);

    // check the metadata of the bin file
    if (metaData.north == metaData.south) {
        printf("-E- north and south metadata are equal.\n");
        exit(110);
    }
    if (metaData.east == metaData.west) {
        printf("-E- east and west metadata are equal.\n");
        exit(110);
    }

    // default to whole globe for SMI files
    if ((strcmp(projectionStr, "smi") == 0)
        || (strcmp(projectionStr, "raw") == 0)) {
        metaData.north = 90.0;
        metaData.south = -90.0;
        metaData.east = 180.0;
        metaData.west = -180.0;
    }
    // read in north, south, east, west from command line
    float tmpf = clo_getFloat(optionList, "north");
    if (tmpf > -900.0) {
        metaData.north = tmpf;
    }
    tmpf = clo_getFloat(optionList, "south");
    if (tmpf > -900.0) {
        metaData.south = tmpf;
    }
    tmpf = clo_getFloat(optionList, "east");
    if (tmpf > -900.0) {
        metaData.east = tmpf;
    }
    tmpf = clo_getFloat(optionList, "west");
    if (tmpf > -900.0) {
        metaData.west = tmpf;
    }

    if (metaData.north <= metaData.south) {
        printf("-E- north must be greater than south.\n");
        exit(EXIT_FAILURE);
    }
    heightInDeg = metaData.north - metaData.south;
    if (heightInDeg > 180.0) {
        printf("-E- height in degrees must be less than or equal to 180.\n");
        exit(EXIT_FAILURE);
    }
    mapEast = metaData.east;
    mapWest = metaData.west;
    if (mapEast < mapWest)
        mapEast += 360;
    widthInDeg = mapEast - mapWest;

    if (widthInDeg > 360.0) {
        printf("-E- width in degrees must be less than or equal to 360.\n");
        exit(EXIT_FAILURE);
    }

    // set other fields in the metadata
    strcpy(metaData.soft_name, "l3mapgen");
    strcpy(metaData.soft_ver, softwareVersion);
    if ((tmpStr = strrchr(ifileName, '/')) != NULL)
        tmpStr++;
    else
        tmpStr = ifileName;
    strcpy(metaData.infiles, tmpStr);
    metaData.proc_con[0] = 0;
    for (i = 0; i < argc; i++) {
        strcat(metaData.proc_con, argv[i]);
        strcat(metaData.proc_con, " ");
    }
    strcpy(metaData.pversion, clo_getString(optionList, "pversion"));

    // set input parameters
    metaData.input_parms[0] = 0;
    int numOptions = clo_getNumOptions(optionList);
    for (int i = 0; i < numOptions; i++) {
        clo_option_t* option = clo_getOption(optionList, i);
        if (option) {
            if (strcmp(option->key, "help") == 0)
                continue;
            if (strcmp(option->key, "version") == 0)
                continue;
            if (strstr(option->key, "dump_options"))
                continue;
            char* val = option->valStr;
            if (val == NULL)
                val = option->defaultVal;
            if(val == NULL)
                val = "";
            strcat(metaData.input_parms, option->key);
            strcat(metaData.input_parms, "=");
            strcat(metaData.input_parms, val);
            strcat(metaData.input_parms, "|");
        }
    }

    // set processing time
    get_time(metaData.ptime);

    for(OutFile* outFile : outFiles) {
        outFile->setMetaData(&metaData);
        outFile->setDeflate(clo_getInt(optionList, "deflate"));
        outFile->setFullLatLon(clo_getBool(optionList, "full_latlon"));
    }
    if (outFile2) {
        outFile2->setMetaData(&metaData);
        outFile2->setDeflate(clo_getInt(optionList, "deflate"));
        outFile2->setFullLatLon(clo_getBool(optionList, "full_latlon"));
    }

    if (strcasecmp(projectionStr, "raw") == 0) {
        writeRawFile(l3File, outFiles, outFile2);
    } else if (strcasecmp(projectionStr, "smi") == 0) {
        writeSmiFile(l3File, outFiles, outFile2);
    } else if (strcasecmp(projectionStr, "platecarree") == 0) {
        writeSmiFile(l3File, outFiles, outFile2);
    } else {
        writeProj4File(l3File, projectionStr, outFiles, outFile2, trimNSEW);
    }

    // check if no pixels are filled
    if (outFiles[0]->getNumFilledPixels() == 0) {
        printf("\nThere are no filled pixels\n");
        printf("Deleting output file.\n");

        string cmd;
        for(OutFile* outFile : outFiles) {        
            cmd = "rm -f ";
            cmd += outFile->getFileName();
            system(cmd.c_str());
        }
            
        if (outFile2) {
            cmd = "rm -f ";
            cmd += outFile2->getFileName();
            system(cmd.c_str());
        }
        exit(110);
    }

    // check the % filled threshold
    float threshold = clo_getFloat(optionList, "threshold");
    if (threshold > 0.0) {
        if (outFiles[0]->getPercentFilledPixels() < threshold) {
            printf("\nPercent filled pixels (%.1f) "
                   "is below the threshold (%.1f)\n",
                   outFiles[0]->getPercentFilledPixels(), threshold);
            printf("Deleting output file.\n");

            string cmd;
            for(OutFile* outFile : outFiles) {        
                cmd = "rm -f ";
                cmd += outFile->getFileName();
                system(cmd.c_str());
            }

            if (outFile2) {
                cmd = "rm -f ";
                cmd += outFile2->getFileName();
                system(cmd.c_str());
            }
            exit(110);
        }
    }

    printEndInfo(outFiles[0]);

    for(OutFile* outFile : outFiles) {
        delete outFile;
    }
    if (outFile2)
        delete outFile2;
    delete l3File;

    return EXIT_SUCCESS;
}
