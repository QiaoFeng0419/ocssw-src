#include <sensorInfo.h>

#include <stddef.h>
#include <strings.h>
#include <stdlib.h>

#include <genutils.h>

// sensor name indexed by sensorId
static const char* sensorName[] = {
    "SeaWiFS",
    "MOS",
    "OCTS",
    "AVHRR",
    "OSMI",
    "CZCS",
    "MODIST",
    "MODISA",
    "OCM1",
    "OCM2",
    "MERIS",
    "VIIRSN",
    "OCRVC",
    "HICO",
    "GOCI",
    "OLI",
    "Aquarius",
    "OCIA",
    "AVIRIS",
    "PRISM",
    "OLCIS3A",
    "SGLI",
    "MSIS2A",
    "L5TM",
    "L7ETMP",
    "VIIRSJ1",
    "MSIS2B",
    "HAWKEYE",
    "MISR",
    "OLCIS3B",
    "OCI",
    "OCIS"
};

// instrument name indexed by sensorId
static const char *instrumentName[] = {
    "SeaWiFS",
    "MOS",
    "OCTS",
    "AVHRR",
    "OSMI",
    "CZCS",
    "MODIS",
    "MODIS",
    "OCM",
    "OCM-2",
    "MERIS",
    "VIIRS",
    "OCRVC",
    "HICO",
    "GOCI",
    "OLI",
    "Aquarius",
    "OCIA",
    "AVIRIS",
    "PRISM",
    "OLCI",
    "SGLI",
    "MSI",
    "L5TM",
    "L7ETMP",
    "VIIRS",
    "MSI",
    "HAWKEYE",
    "MISR",
    "OLCI",
    "OCI",
    "OCIS"
};


// platform name indexed by sensorId
static const char *platformName[] = {
    "Orbview-2",
    "IRS-P3",
    "ADEOS",
    "AVHRR",
    "KOMPSAT",
    "Nimbus-7",
    "Terra",
    "Aqua",
    "IRS-P4",
    "Oceansat-2",
    "Envisat",
    "Suomi-NPP",
    "OCRVC",
    "ISS",
    "COMS",
    "Landsat-8",
    "SAC-D",
    "PACE",
    "AVIRIS",
    "PRISM",
    "Sentinel-3A",
    "GCOM_C",
    "Sentinel-2A",
    "L5TM",
    "L7ETMP",
    "JPSS-1",
    "Sentinel-2B",
    "Seahawk1",
    "Terra",
    "Sentinel-3B",
    "PACE",
    "PACE"
};

// sensor directory indexed by sensorId
static const char *sensorDir[] = {
    "seawifs",
    "mos",
    "octs",
    "avhrr",
    "osmi",
    "czcs",
    "modis",
    "modis",
    "ocm1",
    "ocm2",
    "meris",
    "viirs",
    "ocrvc",
    "hico",
    "goci",
    "oli",
    "aquarius",
    "ocia",
    "aviris",
    "prism",
    "olci",
    "sgli",
    "msi",
    "l5tm",
    "l7etmp",
    "viirs",
    "msi",
    "hawkeye",
    "misr",
    "olci",
    "oci",
    "ocis"
};

// subsensor directory indexed by subsensorId
static const char *subsensorDir[] = {
    "gac",
    "lac",
    "terra",
    "aqua",
    "npp",
    "j1",
    "s2a",
    "s2b",
    "s3a",
    "s3b"
};


/**
 * Get the name of the sensor for this sensorId.
 * 
 * @param sensorId sensor identifier to look up.
 * @return name of sensor or NULL if not found.
 */
const char* sensorId2SensorName(int sensorId) {
    if(sensorId < 0)
        return NULL;
    if(sensorId >= SENSOR_NUM)
        return NULL;
    return sensorName[sensorId];
}

/**
 * Get the name of the instrument for this sensorId.
 * 
 * @param sensorId sensor identifier to look up.
 * @return name of the instrument or NULL if not found.
 */
const char* sensorId2InstrumentName(int sensorId) {
    if(sensorId < 0)
        return NULL;
    if(sensorId >= SENSOR_NUM)
        return NULL;
    return instrumentName[sensorId];
}

/**
 * Get the name of the platform for this sensorId.
 * 
 * @param sensorId sensor identifier to look up.
 * @return name of the platform or NULL if not found.
 */
const char* sensorId2PlatformName(int sensorId) {
    if(sensorId < 0)
        return NULL;
    if(sensorId >= SENSOR_NUM)
        return NULL;
    return platformName[sensorId];
}

/**
 * Get the name of the sensor directory for this sensorId.
 * 
 * @param sensorId sensor identifier to look up.
 * @return sensor directory or NULL if not found.
 */
const char* sensorId2SensorDir(int sensorId) {
    if(sensorId < 0)
        return NULL;
    if(sensorId >= SENSOR_NUM)
        return NULL;
    return sensorDir[sensorId];
}

/**
 * Get the name of the subsensor directory for this subsensorId.
 * 
 * @param subsensorId subsensor identifier to look up.
 * @return subsensor directory or NULL if not found.
 */
const char* subsensorId2SubsensorDir(int subsensorId) {
    if(subsensorId < 0)
        return NULL;
    if(subsensorId >= SENSOR_NUM)
        return NULL;
    return subsensorDir[subsensorId];
}

/**
 * lookup the ID for a sensor name
 *
 * @param sensorName name of the sensor to lookup
 * @return sensor ID for the given sensor, -1 if not found.
 */
int sensorName2SensorId(const char* name) {
    int i;

    if(name == NULL)
        return -1;
    
    // convert the number if an int was sent in
    if (isValidInt(name)) {
        i = atoi(name);
        if (i >= 0 && i < SENSOR_NUM) {
            return i;
        }
    }

    if (strcasecmp(name, "hmodisa") == 0)
        return MODISA;
    if (strcasecmp(name, "hmodist") == 0)
        return MODIST;

    for (i = 0; i < SENSOR_NUM; i++) {
        if (strcasecmp(sensorName[i], name) == 0)
            return i;
    }

    return -1;
}

/**
 * lookup the sensorID for a given instrument and platform
 *
 * @param instrument instrument to use for sensorID look up
 * @param platform platform to use for sensorID lookup
 * @return the matching sensorID, or -1 if not found
 */
int instrumentPlatform2SensorId(const char* instrument, const char* platform) {
    int i;

    if(instrument == NULL  || platform == NULL)
        return -1;
    
    for (i = 0; i < SENSOR_NUM; i++) {
        if (strcasecmp(instrumentName[i], instrument) == 0)
            if (strcasecmp(platformName[i], platform) == 0)
                return i;
    }

    return -1;
}

/**
 * return the subsensorId for the given sensorId
 * @param sensorId sensorId to lookup
 * @return the subsensorId, or -1 if not found
 */
int sensorId2SubsensorId(int sensorId) {
    switch(sensorId) {
    case MODIST:
        return MODIS_TERRA;
    case MODISA:
        return MODIS_AQUA;
    case VIIRSN:
        return VIIRS_NPP;
    case VIIRSJ1:
        return VIIRS_J1;
    case MSIS2A:
        return MSI_S2A;
    case MSIS2B:
        return MSI_S2B;
    case OLCIS3A:
        return OLCI_S3A;
    case OLCIS3B:
        return OLCI_S3B;
    default:
        return -1;
    }
}

/**
 * fine the sensor name given the instrument and platform
 * @param instrument name of instrument
 * @param platform name of platform
 * @return sensor name
 */
const char* instrumentPlatform2SensorName(const char* instrument, const char* platform) {
    return sensorId2SensorName(instrumentPlatform2SensorId(instrument, platform));
}
