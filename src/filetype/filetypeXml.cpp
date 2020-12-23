/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <boost/foreach.hpp>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pugixml.hpp>
#include <vector>

#include <dfutils.h>
#include <genutils.h>
#include <sensorDefs.h>
#include <timeutils.h>

#include <boost/algorithm/string.hpp>

#include "filetype.h"


/**
 *   chk_olci
 *   purpose: check a file to see if it is an OLCI xml file
 *   If it is, populate the data pointer with filenames
 *   associated with Lt's, instrument and geometry data
 *   Returns -1 if not OLCI or the format code
 *
 * @param char *fname      - name of file to check
 * @param filehandle *file - input file information
 *
 * Author: R. Healy (SAIC) 7/29/2015
 * W. Robinson, SAIC, 18 Feb 2016 pad out the olci file name storage by 1 
 *   to accomodate trailing null, allow xml files with alternate product 
 *   name to be accepted as olci files
 * s
 */

extern "C" file_format chk_olci_xml(char *filename) {
    file_format ret = {FT_INVALID, -1, -1};
    pugi::xml_document rootNode;
    pugi::xml_node dataNode;

    const char *productName;
    const char *platformName;

    if (!rootNode.load_file(filename)) {
        return ret;
    }

    productName = rootNode.first_element_by_path("xfdu:XFDU/informationPackageMap/xfdu:contentUnit").attribute("textInfo").value();
    if (strstr(productName, "OLCI") != NULL && strstr(productName, "SENTINEL-3") != NULL && strstr(productName, "Level 1") != NULL) {
        dataNode = rootNode.first_element_by_path("xfdu:XFDU/metadataSection").find_child_by_attribute("metadataObject", "ID", "platform");
        platformName = dataNode.first_element_by_path("metadataWrap/xmlData/sentinel-safe:platform/sentinel-safe:number").child_value();
        if(strcmp("A", platformName) == 0) {
            if (want_verbose != 0){
                std::printf("%s - S3A\n", productName);
            }
            ret.type = FT_OLCI;
            ret.sensor_id = OLCIS3A;
            ret.subsensor_id = OLCI_S3A;
        }
        if(strcmp("B", platformName) == 0) {
            if (want_verbose != 0){
                std::printf("%s - S3B\n", productName);
            }
            ret.type = FT_OLCI;
            ret.sensor_id = OLCIS3B;
            ret.subsensor_id = OLCI_S3B;
        }
    }

    return ret;
}

extern "C" file_format chk_msi_xml(char *filename) {
    file_format ret = {FT_INVALID, -1, -1};
    pugi::xml_document rootNode;
    pugi::xml_node dataNode;

    const char *productName;
    const char *platformName;

    if (!rootNode.load_file(filename)) {
        return ret;
    }

    productName = rootNode.first_element_by_path("xfdu:XFDU/informationPackageMap/xfdu:contentUnit").attribute("textInfo").value();
    if (strstr(productName, "MSI") != NULL && strstr(productName, "SENTINEL-2") != NULL && strstr(productName, "Level-1C") != NULL) {
        dataNode = rootNode.first_element_by_path("xfdu:XFDU/metadataSection").find_child_by_attribute("metadataObject", "ID", "platform");
        platformName = dataNode.first_element_by_path("metadataWrap/xmlData/safe:platform/safe:number").child_value();
        if(!strcmp(platformName, "2A")) {
            if (want_verbose != 0){
                printf("Found Sentinel-2A product: %s\n", productName);
            }
            ret.type = FT_MSIL1C;
            ret.sensor_id = MSIS2A;
            ret.subsensor_id = MSI_S2A;
        } else if(!strcmp(platformName, "2B")) {
            if (want_verbose != 0){
                printf("Found Sentinel-2B product: %s\n", productName);
            }
            ret.type = FT_MSIL1C;
            ret.sensor_id = MSIS2B;
            ret.subsensor_id = MSI_S2B;
        } else {
            printf("Unknown platform: %s  for product: %s\n", platformName, productName);
            exit(EXIT_FAILURE);
        }
    }

    return ret;
}

