/* =================================================================== */
/* module raman.c - Raman scattering correction for Rrs                */
/*                                                                     */
/* This module contains the functions to correct sensor-observed       */
/* above-water remote sensing reflectances, Rrs. for Raman Scattering  */
/* effects.                                                            */
/*                                                                     */
/* References:                                                         */
/* Lee et al (1994) Model for the interpretation of hyperspectral      */
/* remote-sensing reflectance, Applied Optics, 33(24), 5721-5732       */
/* doi:10.1364/AO.33.005721                                            */
/*                                                                     */
/* Lee et al (2013) Penetration of UV-visible solar radiation in the   */
/* global oceans: Insights from ocean color remote sensing, Journal of */
/* Geophysical Research Oceans, 118, 4241-4255, doi:10.1002/jgrc.20308 */
/*                                                                     */
/* Mobley. C.D. (2010) Hydrolight Technical Note 10: Interpretation    */
/* of Raman Scattering Computations, Sequoia Scientific.               */
/*                                                                     */
/* Sathyendranath and Platt (1998) Ocean-color model incorporating     */
/* transspectral processes, Applied Optics, 37(12), 2216-2227,         */
/* doi:10.1364/AO.37.002216                                            */
/*                                                                     */
/* Walrafen (1967) Raman spectral studies of the effects of temperature*/
/* on water structure, Journal of Chemical Physics, 47, 118-121,       */
/* doi:10.1063/1.711834                                                */
/*                                                                     */
/* Westberry et al. (2013) Influence if Raman scattering on ocean color*/
/* inversion models, Applied Optics, 52(22), 5552-5561,                */
/* doi:10.1364/AO.52.005552                                            */
/*                                                                     */
/*                                                                     */
/* Implementation:                                                     */
/* L. McKinna NASA/OBPG/SAIC, April 2015                               */
/*                                                                     */
/* Notes:                                                              */
/*                                                                     */
/* =================================================================== */

#define NORAMAN 0
#define LEE2013 1
#define WESTBERRY2013 2
#define LEE1994 3

#include <stdio.h>
#include <math.h>
#include <gsl/gsl_fit.h>
#include "l12_proto.h"
#include "sensorDefs.h"

static int32_t nbandVis; //Number of visible bands
static float *lambda; //Sensor wavelengths

//Wavelength indices
static int idx412;
static int idx443;
static int idx488;
static int idx550;
static int idx670;
static int bandNum550; //Number of bands shorter than 500 nm;

static float angstEst; //Estimate Angstrom exponent (Taua power law slope)

//Bio-optical model parameters
static float *aw; //water absorption coefficient at sensor bands
static float *bbw; //water backscattering coefficient at sensor bands
static float *awR; //water absorption coeficient at Raman excitation bands
static float *bbwR; //water backscattering coefficient at Raman excitation bandss
static float *bbtot; //total backscattering modelled at sensor bands
static float *atot; //total absorption modelled at sensor bands
static float *atotR; //total absorption coefficient at Raman excitation bands
static float *bbtotR; //total backscattering coefficient at Raman excitation bands
static float *aphStar; //Normalised phytoplankton absorption spectral shape at sensor bands
static float *aphStarR; //Normalised phytoplankton absorption spectral shape at Raman bands
static float *kdSen; //Diffuse attenuation coefficient at sensor bands
static float *kdRam; //Diffuse attenuation coefficient at raman bands
static float *kappaSen;
static float *kappaRam;
static float *kLuSen; //Diffuse attenuation coefficient of up-welling radiance
// at sensor bands
static float aph443; //QAA-derived phytoplankton absorption at 443 nm      
static float adg443; //QAA-derived Absorption of CDOM and colored detrital matter at 443 nm
static float bbp443; //QAA-derived backscattering coefficient at 443 nm
static float Ybbp; //Power exponent of bbp
static float Sdg; //Exponential slope of adg

static float *aph1Sen; //Bricaud aph0 coefficient at sensor bands
static float *aph2Sen; //Bricaud aph1 coefficient at sensor bands
static float *aph1Ram; //Bricaud aph0 coefficient at Raman bands
static float *aph2Ram; //Bricaud aph1 coefficient at Raman bands


//Radtran atmospheric model parameters
static float *eDRam; //Down-welling irradiance at Raman excitation bands
static float *eDSen; //Down-welling irradiance at sensor bands
static float *tauaRam; //Aerosol optical thickness at Raman bands

static float *ramLam; //Raman excitation wavelengths
static float *ramBandW; //Raman band widths
static float *bRex; //Raman scattering coefficient

static float *f0BarRam; //TOA solar irradiance at Raman bands
static float *rayRam; //Rayleigh transmittance at Raman bands
static float *ozRam; //Ozone absorption at Raman bands
static float *wvRam; //Water vapor absorption at Raman bands
static float *oxyRam; //Oxygen absorption at Raman bands

static float *f0BarSen; //TOA solar irradiance at sensor bands
static float *raySen; //Rayleigh transmittance at sensor bands
static float *ozSen; //Ozone absorption at sensor bands
static float *wvSen; //Water vapor absorption at sensor bands
static float *oxySen; //Oxygen absorption at sensor bands

static float *Rrs_ram; //Rrs due to Raman scattering

//Lee et al. 2013 Raman correction 1 coefficients
//Pointers for interpolation/extrapolation
static float *alphaCor;
static float *betaCor0;
static float *betaCor1;

//Lee et al 2013 Empirical coefficients
static float lamCor1[6] = {412.0, 443.0, 488.0, 531.0, 551.0, 667.0};
static float alpha[6] = {0.003, 0.004, 0.011, 0.015, 0.017, 0.018};
static float beta0[6] = {0.014, 0.015, 0.010, 0.010, 0.010, 0.010};
static float beta1[6] = {-0.022, -0.023, -0.051, -0.070, -0.080, -0.081};

/*----------------------------------------------------------------------------*/
/*                      SENSOR-SPECIFIC COEFFICIENTS                          */
/*----------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/
/* OCTS static coefficients for Raman correction                              */
/* ---------------------------------------------------------------------------*/
//Raman band centers, band widths and sensor band widths
static float rlOcts[6] = {362., 385., 421., 440., 475., 544.};
static float rwOcts[6] = {13., 14., 16., 13., 16., 18.};

//OCTS Raman scattering coefficients at excitation bands
static float bROcts[6] = {0.00132377, 0.00094603, 0.000596231, 0.000493522, 0.000314177, 0.000151688};

// Radtran coefficients at Raman bands
static float f0brOcts[6] = {105.057, 102.888, 174.964, 170.741, 206.527, 185.922};
static float rayROcts[6] = {0.547442, 0.42027, 0.292905, 0.251769, 0.177694, 0.101232};
static float ozROcts[6] = {0.0, 0.0, 0.000548079, 0.00191232, 0.0119877, 0.0829727};
static float wvROcts[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 9.70075E-11};
static float oxyROcts[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

// Radtran coefficients at OCTS bands
static float f0bsOcts[6] = {170.758, 188.796, 192.236, 187.314, 179.506, 152.375};
static float raySOcts[6] = {0.317948, 0.235324, 0.155394, 0.131271, 0.0868446, 0.0442263};
static float ozSOcts[6] = {0.000229791, 0.00310197, 0.0204102, 0.0420779, 0.116476, 0.0479347};
static float wvSOcts[6] = {0.0, 0.0, 0.0, 0.0, 0.00679166, 0.00129678};
static float oxySOcts[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

//Bricaud aph coefficients
static float aph1sOcts[6] = {0.029608, 0.0369429, 0.0252457, 0.0159346, 0.00526282, 0.015449};
static float aph2sOcts[6] = {0.680274, 0.614661, 0.606804, 0.72779, 0.931381, 0.81726};
static float aph1rOcts[6] = {0.0240515, 0.0240515, 0.0327317, 0.0367956, 0.0295587, 0.00787535};
static float aph2rOcts[6] = {0.687735, 0.687735, 0.663118, 0.632766, 0.591507, 0.913724};

/*----------------------------------------------------------------------------*/
/*                          SENSOR-SPECIFIC COEFFICIENTS                      */
/* ---------------------------------------------------------------------------*/
/* SEAWIFS static coefficients for Raman correction                           */
/* ---------------------------------------------------------------------------*/
//Raman band centers, band widths and sensor band widths
static float rlSeawifs[6] = {362., 385., 420., 435., 467., 546.};
static float rwSeawifs[6] = {7., 7., 8., 9., 12., 12.};

//SeaWiFS Raman scattering coefficients at excitation bands
static float bRSeawifs[6] = {0.00132377, 0.00094603, 0.000596231, 0.000497341, 0.000340195, 0.000148775};

// Radtran coefficients at Raman bands
static float f0brSeawifs[6] = {105.057, 102.888, 174.964, 167.773, 203.908, 186.144};
static float rayRSeawifs[6] = {0.547442, 0.42027, 0.292905, 0.253643, 0.189143, 0.0997312};
static float ozRSeawifs[6] = {0.0, 0.0, 0.000548079, 0.00165534, 0.0088063, 0.0851299};
static float wvRSeawifs[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 2.44958E-09};
static float oxyRSeawifs[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

// Radtran coefficients at SeaWiFS bands
static float f0bsSeawifs[6] = {170.758, 188.796, 192.236, 193.132, 183.63, 151.281};
static float raySSeawifs[6] = {0.317948, 0.235324, 0.155394, 0.132038, 0.0934382, 0.0434297};
static float ozSSeawifs[6] = {0.000229791, 0.00310197, 0.0204102, 0.0410486, 0.0952688, 0.0447048};
static float wvSSeawifs[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.000301748};
static float oxySSeawifs[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

//Bricaud aph coefficients
static float aph1sSeawifs[6] = {0.029608, 0.0369429, 0.0252457, 0.0162046, 0.00627478, 0.0170438};
static float aph2sSeawifs[6] = {0.680274, 0.614661, 0.606804, 0.720135, 0.941274, 0.813841};
static float aph1rSeawifs[6] = {0.0240515, 0.0240515, 0.0327317, 0.0369255, 0.0313457, 0.00757419};
static float aph2rSeawifs[6] = {0.687735, 0.687735, 0.663118, 0.634894, 0.595437, 0.920029};

/* ---------------------------------------------------------------------------*/
/* MODIS AQUA static coefficients for  Raman Correction                       */
/* ---------------------------------------------------------------------------*/
//Raman band centers, band widths and sensor band widths
static float rlModisa[10] = {362., 383., 405., 419., 450., 462., 467., 530., 544., 552.};
static float rwModisa[10] = {7., 7., 8., 8., 9., 9., 9., 11., 12., 12.};

// MODIS Raman scattering coefficients at excitation bands
static float bRModisa[10] = {0.00132377, 0.00094603, 0.000728057, 0.000607423, 0.000414652, 0.00036299, 0.000340195, 0.000175436, 0.000151688, 0.000141347};

// Radtran coefficients at Raman excitation bands
static float f0brModisa[10] = {105.057, 102.888, 173.814, 175.285, 203.309, 207.043, 203.908, 185.548, 185.922, 185.47};
static float rayRModisa[10] = {0.547442, 0.42027, 0.342557, 0.297133, 0.220407, 0.198927, 0.189143, 0.113219, 0.101232, 0.0959207};
static float ozRModisa[10] = {0.0, 0.0, 4.10097E-05, 0.000461462, 0.00382323, 0.00776277, 0.0088063, 0.0669728, 0.0829727, 0.0913593};
static float wvRModisa[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 9.70075E-11, 1.78547E-06};
static float oxyRModisa[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

// Radtran coefficients at modis bands
static float f0bsModisa[10] = {170.758, 188.796, 202.359, 192.335, 185.707, 186.916, 183.63, 158.66, 152.375, 148.144};
static float raySModisa[10] = {0.317948, 0.235324, 0.186167, 0.158092, 0.11184, 0.0991017, 0.0934382, 0.0506747, 0.0442263, 0.0413876};
static float ozSModisa[10] = {0.000229791, 0.00310197, 0.00889489, 0.0199428, 0.0709312, 0.0862022, 0.0952688, 0.0720248, 0.0479347, 0.0373123};
static float wvSModisa[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0909799, 0.00129678, 0.000237234};
static float oxySModisa[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

//Bricaud aph coefficients
static float aph1sModisa[10] = {0.029608, 0.0369429, 0.0309933, 0.0258717, 0.0100736, 0.00745871, 0.00627478, 0.00740517, 0.015449, 0.0168516};
static float aph2sModisa[10] = {0.680274, 0.614661, 0.595357, 0.600516, 0.856872, 0.922909, 0.941274, 0.831006, 0.81726, 0.823776};
static float aph1rModisa[10] = {0.0240515, 0.0240515, 0.0261147, 0.0322894, 0.0349146, 0.0324558, 0.0313457, 0.0104169, 0.00787535, 0.00679344};
static float aph2rModisa[10] = {0.687735, 0.687735, 0.686703, 0.666775, 0.598609, 0.595061, 0.595437, 0.847853, 0.913724, 0.93314};

/* ---------------------------------------------------------------------------*/
/* MODIS TERRA static coefficients for  Raman Correction                       */
/* ---------------------------------------------------------------------------*/
//Raman band centers, band widths and sensor band widths
static float rlModist[10] = {362., 385., 405., 419., 450., 462., 467., 530., 544., 552.};
static float rwModist[10] = {7., 7., 8., 8., 9., 9., 9., 11., 12., 12.};

static float bRModist[10] = {0.00132377, 0.00094603, 0.000728057, 0.000607423, 0.000414652, 0.00036299, 0.000340195, 0.000175436, 0.000151688, 0.000141347};

// Radtran coefficients at Raman excitation bands
static float f0brModist[10] = {105.057, 102.888, 173.814, 175.285, 203.309, 207.043, 203.908, 185.548, 185.922, 185.47};
static float rayRModist[10] = {0.547442, 0.42027, 0.342557, 0.297133, 0.220407, 0.198927, 0.189143, 0.113219, 0.101232, 0.0959207};
static float ozRModist[10] = {0.0, 0.0, 4.10097E-05, 0.000461462, 0.00382323, 0.00776277, 0.0088063, 0.0669728, 0.0829727, 0.0913593};
static float wvRModist[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 9.70075E-11, 1.78547E-06};
static float oxyRModist[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

// Radtran coefficients at modis bands
static float f0bsModist[10] = {170.758, 188.796, 202.359, 192.335, 185.707, 186.916, 183.63, 158.66, 152.375, 148.144};
static float raySModist[10] = {0.317948, 0.235324, 0.186167, 0.158092, 0.11184, 0.0991017, 0.0934382, 0.0506747, 0.0442263, 0.0413876};
static float ozSModist[10] = {0.000229791, 0.00310197, 0.00889489, 0.0199428, 0.0709312, 0.0862022, 0.0952688, 0.0720248, 0.0479347, 0.0373123};
static float wvSModist[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0909799, 0.00129678, 0.000237234};
static float oxySModist[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

//Bricaud aph coefficients
static float aph1sModist[10] = {0.029608, 0.0369429, 0.0309933, 0.0258717, 0.0100736, 0.00745871, 0.00627478, 0.00740517, 0.015449, 0.0168516};
static float aph2sModist[10] = {0.680274, 0.614661, 0.595357, 0.600516, 0.856872, 0.922909, 0.941274, 0.831006, 0.81726, 0.823776};
static float aph1rModist[10] = {0.0240515, 0.0240515, 0.0261147, 0.0322894, 0.0349146, 0.0324558, 0.0313457, 0.0104169, 0.00787535, 0.00679344};
static float aph2rModist[10] = {0.687735, 0.687735, 0.686703, 0.666775, 0.598609, 0.595061, 0.595437, 0.847853, 0.913724, 0.93314};
/* ---------------------------------------------------------------------------*/
/* VIIRS static coefficients for  Raman Correction                       */
/* ---------------------------------------------------------------------------*/
static float rlViirs[5] = {360., 385., 417., 464., 547.};
static float rwViirs[5] = {7., 7., 8., 9., 12.};

static float bRViirs[5] = {0.00135406, 0.00094603, 0.000618878, 0.00035136, 0.00014782};

static float f0brViirs[5] = {101.911, 102.888, 175.015, 205.343, 186.197};
static float rayRViirs[5] = {0.558048, 0.42027, 0.301439, 0.19399, 0.0992401};
static float ozRViirs[5] = {0.0, 0.0, 0.000392296, 0.00837063, 0.0858628};
static float wvRViirs[5] = {0.0, 0.0, 0.0, 0.0, 6.4108E-09};
static float oxyRViirs[5] = {0.0, 0.0, 0.0, 0.0, 0.0};

static float f0bsViirs[5] = {168.769, 188.796, 194.51, 187.061, 150.809};
static float raySViirs[5] = {0.324534, 0.235324, 0.160906, 0.0961984, 0.0431702};
static float ozSViirs[5] = {0.00016985, 0.00310197, 0.0198909, 0.0901554, 0.0436888};
static float wvSViirs[5] = {0.0, 0.0, 0.0, 0.0, 0.000187014};
static float oxySViirs[5] = {0.0, 0.0, 0.0, 0.0, 0.0};

//Bricaud aph coefficients
static float aph1sViirs[5] = {0.0286259, 0.0369429, 0.026435, 0.00685008, 0.017345};
static float aph2sViirs[5] = {0.683025, 0.614661, 0.595353, 0.934767, 0.813731};
static float aph1rViirs[5] = {0.0240515, 0.0240515, 0.0318008, 0.0319295, 0.00747445};
static float aph2rViirs[5] = {0.687735, 0.687735, 0.670169, 0.595605, 0.922004};

/* ---------------------------------------------------------------------------*/
/* MERIS static coefficients for  Raman Correction                            */
/* ---------------------------------------------------------------------------*/
static float rlMeris[9] = {362., 385., 420., 435., 471., 513., 543., 554., 572.};
static float rwMeris[9] = {7., 7., 8., 8., 9., 11., 12., 14., 13.};

static float bRMeris[9] = {0.0013089, 0.00094603, 0.000596231, 0.000497341, 0.00032686, 0.000208477, 0.000153671, 0.000138684, 0.000116668};

static float f0brMeris[9] = {107.272, 102.888, 174.964, 167.773, 204.263, 186.843, 185.811, 184.55, 179.558};
static float rayRMeris[9] = {0.542386, 0.42027, 0.292905, 0.253643, 0.183261, 0.129582, 0.102256, 0.0945441, 0.0827138};
static float ozRMeris[9] = {0.0, 0.0, 0.000548079, 0.00165534, 0.00980838, 0.0422707, 0.0815767, 0.094124, 0.123869};
static float wvRMeris[9] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 8.56413E-12, 1.35733E-05, 0.0106084};
static float oxyRMeris[9] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

static float f0bsMeris[9] = {171.672, 189.489, 192.221, 192.966, 179.844, 164.973, 153.142, 147.071, 140.327};
static float raySMeris[9] = {0.314707, 0.235396, 0.155399, 0.132055, 0.0900582, 0.0594901, 0.0447689, 0.0406557, 0.0345366};
static float ozSMeris[9] = {0.000259218, 0.00313902, 0.0204557, 0.0411736, 0.105558, 0.108447, 0.0500913, 0.0352415, 0.0187744};
static float wvSMeris[9] = {0.0, 0.0, 0.0, 0.0, 3.56388E-05, 0.0, 0.00213056, 0.000984887, 0.130155};
static float oxySMeris[9] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

//Bricaud aph coefficients
static float aph1sMeris[9] = {0.0300823, 0.0369807, 0.0252367, 0.0162079, 0.00567835, 0.00607524, 0.0141711, 0.0149778, 0.00248126};
static float aph2sMeris[9] = {0.678686, 0.614817, 0.606839, 0.720099, 0.938248, 0.868982, 0.820075, 0.839236, 1.02861};
static float aph1rMeris[9] = {0.0240515, 0.0240515, 0.0327317, 0.0369255, 0.0305037, 0.0152557, 0.00807796, 0.00651751, 0.00506931};
static float aph2rMeris[9] = {0.687735, 0.687735, 0.663118, 0.634894, 0.593951, 0.738445, 0.909239, 0.936041, 0.916574};
/*----------------------------------------------------------------------------*/
/*                      END OF SENSOR-SPECIFIC COEFFICIENTS                   */
/*----------------------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/* raman_pixel_alloc() Allocate pointer memory for Raman computations         */

/* ---------------------------------------------------------------------------*/
void raman_pixel_alloc(l2str *l2rec) {

    //Allocate static pointer memory
    lambda = (float*) allocateMemory(nbandVis * sizeof (float), "lambda");

    aw = (float*) allocateMemory(nbandVis * sizeof (float), "aw");
    bbw = (float*) allocateMemory(nbandVis * sizeof (float), "bbw");
    awR = (float*) allocateMemory(nbandVis * sizeof (float), "awR");
    bbwR = (float*) allocateMemory(nbandVis * sizeof (float), "bbwR");
    aphStar = (float*) allocateMemory(nbandVis * sizeof (float), "aphStar");
    aphStarR = (float*) allocateMemory(nbandVis * sizeof (float), "aphStarR");
    bbtot = (float*) allocateMemory(nbandVis * sizeof (float), "bbtot");
    atot = (float*) allocateMemory(nbandVis * sizeof (float), "atot");
    bbtotR = (float*) allocateMemory(nbandVis * sizeof (float), "bbtotR");
    atotR = (float*) allocateMemory(nbandVis * sizeof (float), "atotR");

    tauaRam = (float*) allocateMemory(nbandVis * sizeof (float), "tauaRam");

    kdSen = (float*) allocateMemory(nbandVis * sizeof (float), "kdSen");
    kdRam = (float*) allocateMemory(nbandVis * sizeof (float), "kdRam");
    kappaSen = (float*) allocateMemory(nbandVis * sizeof (float), "kappaSen");
    kappaRam = (float*) allocateMemory(nbandVis * sizeof (float), "kappaRam");
    kLuSen = (float*) allocateMemory(nbandVis * sizeof (float), "kLuSen");

    betaCor0 = (float*) allocateMemory(nbandVis * sizeof (float), "betaCor0");
    betaCor1 = (float*) allocateMemory(nbandVis * sizeof (float), "betaCor1");
    alphaCor = (float*) allocateMemory(nbandVis * sizeof (float), "alphaCor");

    eDRam = (float*) allocateMemory(nbandVis * sizeof (float), "eDRam");
    eDSen = (float*) allocateMemory(nbandVis * sizeof (float), "eDSen");

    Rrs_ram = (float*) allocateMemory(l2rec->l1rec->npix *
            l2rec->l1rec->l1file->nbands * sizeof (float), "Rrs_ram");


}

/* ---------------------------------------------------------------------------*/
/* get_raman_coeffs() Allocate pointer memory for Raman computations         */

/* ---------------------------------------------------------------------------*/
void get_raman_coeffs(l2str *l2rec) {

    if (l2rec->l1rec->l1file->sensorID == OCTS) {

        ramLam = rlOcts;
        ramBandW = rwOcts;
        bRex = bROcts;

        f0BarRam = f0brOcts;
        rayRam = rayROcts;
        ozRam = ozROcts;
        wvRam = wvROcts;
        oxyRam = oxyROcts;
        f0BarSen = f0bsOcts;
        raySen = raySOcts;
        ozSen = ozSOcts;
        wvSen = wvSOcts;
        oxySen = oxySOcts;

        aph1Sen = aph1sOcts;
        aph2Sen = aph2sOcts;
        aph1Ram = aph1rOcts;
        aph2Ram = aph2rOcts;

    } else if (l2rec->l1rec->l1file->sensorID == SEAWIFS) {

        ramLam = rlSeawifs;
        ramBandW = rwSeawifs;
        bRex = bRSeawifs;

        f0BarRam = f0brSeawifs;
        rayRam = rayRSeawifs;
        ozRam = ozRSeawifs;
        wvRam = wvRSeawifs;
        oxyRam = oxyRSeawifs;
        f0BarSen = f0bsSeawifs;
        raySen = raySSeawifs;
        ozSen = ozSSeawifs;
        wvSen = wvSSeawifs;
        oxySen = oxySSeawifs;

        aph1Sen = aph1sSeawifs;
        aph2Sen = aph2sSeawifs;
        aph1Ram = aph1rSeawifs;
        aph2Ram = aph2rSeawifs;

    } else if (l2rec->l1rec->l1file->sensorID == MODISA) {

        ramLam = rlModisa;
        ramBandW = rwModisa;
        bRex = bRModisa;

        f0BarRam = f0brModisa;
        rayRam = rayRModisa;
        ozRam = ozRModisa;
        wvRam = wvRModisa;
        oxyRam = oxyRModisa;
        f0BarSen = f0bsModisa;
        raySen = raySModisa;
        ozSen = ozSModisa;
        wvSen = wvSModisa;
        oxySen = oxySModisa;

        aph1Sen = aph1sModisa;
        aph2Sen = aph2sModisa;
        aph1Ram = aph1rModisa;
        aph2Ram = aph2rModisa;

    } else if (l2rec->l1rec->l1file->sensorID == MODIST) {

        ramLam = rlModist;
        ramBandW = rwModist;
        bRex = bRModist;

        f0BarRam = f0brModist;
        rayRam = rayRModist;
        ozRam = ozRModist;
        wvRam = wvRModist;
        oxyRam = oxyRModist;
        f0BarSen = f0bsModist;
        raySen = raySModist;
        ozSen = ozSModist;
        wvSen = wvSModist;
        oxySen = oxySModist;

        aph1Sen = aph1sModist;
        aph2Sen = aph2sModist;
        aph1Ram = aph1rModist;
        aph2Ram = aph2rModist;


    } else if (l2rec->l1rec->l1file->sensorID == VIIRSN) {

        ramLam = rlViirs;
        ramBandW = rwViirs;
        bRex = bRViirs;

        f0BarRam = f0brViirs;
        rayRam = rayRViirs;
        ozRam = ozRViirs;
        wvRam = wvRViirs;
        oxyRam = oxyRViirs;
        f0BarSen = f0bsViirs;
        raySen = raySViirs;
        ozSen = ozSViirs;
        wvSen = wvSViirs;
        oxySen = oxySViirs;

        aph1Sen = aph1sViirs;
        aph2Sen = aph2sViirs;
        aph1Ram = aph1rViirs;
        aph2Ram = aph2rViirs;

    } else if (l2rec->l1rec->l1file->sensorID == MERIS) {

        ramLam = rlMeris;
        ramBandW = rwMeris;
        bRex = bRMeris;

        f0BarRam = f0brMeris;
        rayRam = rayRMeris;
        ozRam = ozRMeris;
        wvRam = wvRMeris;
        oxyRam = oxyRMeris;
        f0BarSen = f0bsMeris;
        raySen = raySMeris;
        ozSen = ozSMeris;
        wvSen = wvSMeris;
        oxySen = oxySMeris;

        aph1Sen = aph1sMeris;
        aph2Sen = aph2sMeris;
        aph1Ram = aph1rMeris;
        aph2Ram = aph2rMeris;

    }

}
/* -----------------------------------------------------------------------*/
/* set_raman_aph_uv() - Extrapolates aph* into the UV (below 400 nm).     */
/*                      This methods assumes that aph can be linearly     */
/*                      extrapolated into the UV                          */

/* -----------------------------------------------------------------------*/
void set_raman_aph_uv(l2str *l2rec, int ip) {

    int iw;
    float aphM, aphC;
    float aphStar443;

    //Calculate aphStar at 443 nm using Bricaud et al 1998 
    aphStar443 = aph1Sen[idx443] * pow(l2rec->chl[ip], (aph2Sen[idx443]));

    //Using sensor-specific coefficients, calculate aphstar at all bands and
    //normalize to 1.0 at 443 nm
    for (iw = 0; iw < nbandVis; iw++) {
        aphStar[iw] = (aph1Sen[iw] * pow(l2rec->chl[ip], (aph2Sen[iw])))
                / aphStar443;
        aphStarR[iw] = (aph1Ram[iw] * pow(l2rec->chl[ip], (aph2Ram[iw])))
                / aphStar443;

    }

    //Linear model of aph betweeen 412 and 443 nm
    aphM = (aphStar[idx443] - aphStar[idx412]) / (lambda[idx443] - lambda[idx412]);
    aphC = aphStar[idx443] - aphM * lambda[idx443];

    //If outside the Bricaud in UV, extrapolate as linear function.        
    for (iw = 0; iw < nbandVis; iw++) {
        if (ramLam[iw] <= 400) {
            aphStarR[iw] = (aphM * ramLam[iw] + aphC);
        }
    }

}

/* -----------------------------------------------------------------------*/
/* fit_raman_taua() - Interpolates/extrapolates aerosol optical thickness */
/*                      (taua) at Raman excitation bands. The assumption  */
/*                      that taua follows a power law has been used.      */
/*                       Models angstrom for only the 400-500nm domain     */

/* -----------------------------------------------------------------------*/
void fit_raman_taua(l2str *l2rec, int ip) {

    int iw;

    double logTaua[bandNum550];
    double waveRatio[bandNum550];

    double c0, c1, cov00, cov01, cov11, sumsq;
    int nbands = l2rec->l1rec->l1file->nbands;

    //Log transform data so function has a linear form
    for (iw = 0; iw < bandNum550; iw++) {
        logTaua[iw] = log(l2rec->taua[ip * nbands + iw] / l2rec->taua[ip * nbands + idx488]);
        waveRatio[iw] = log(lambda[iw] / lambda[idx488]);
    }

    //Use a linear fit on long-transformed data to compute power law exponent (range 412-490 nm)
    gsl_fit_linear(waveRatio, 1, logTaua, 1, nbandVis, &c0, &c1, &cov00, &cov01, &cov11, &sumsq);

    //Estimated slope of Taua
    angstEst = c1;

    //Extrapolate/interpolate using power law model
    for (iw = 0; iw < nbandVis; iw++) {
        tauaRam[iw] = l2rec->taua[ip * nbands + idx488] * pow((ramLam[iw] / lambda[idx488]), c1);
    }

}

/* -----------------------------------------------------------------------*/
/* raman_qaa() - computes a first order estimate of the constituent IOPs  */

/* -----------------------------------------------------------------------*/
void raman_qaa(l2str *l2rec, int ip) {

    int iw, idxref;

    float aref, bbpref, numer, denom, rho;
    float rat, zeta, xi, term1, term2;

    float rrs_a[nbandVis];
    float rrs_s[nbandVis];
    float u[nbandVis];
    float at[nbandVis];
    float bbt[nbandVis];

    float g0 = 0.08945;
    float g1 = 0.1247;
    float acoefs[3] = {-1.146, -1.366, -0.469};

    //Calculate the sub-surface remote sensing reflectance
    for (iw = 0; iw < nbandVis; iw++) {
        rrs_a[iw] = l2rec->Rrs[ip * l2rec->l1rec->l1file->nbands + iw];
        rrs_s[iw] = rrs_a[iw] / (0.52 + 1.7 * rrs_a[iw]);
    }

    /*pre-test of Rrs550*/
    if (rrs_a[idx550] <= 0.0)
        rrs_a[idx550] = 0.001;

    /* pre-test 670 */
    if ((rrs_a[idx670] > 20.0 * pow(rrs_a[idx550], 1.5)) || (rrs_a[idx670] < 0.9 * pow(rrs_a[idx550], 1.7))) {
        rrs_a[idx670] = 1.27 * pow(rrs_a[idx550], 1.47) + 0.00018 * pow(rrs_a[idx488] / rrs_a[idx550], -3.19);
    }

    /*Quadratic formula to get the quantity u*/
    for (iw = 0; iw < nbandVis; iw++) {
        u[iw] = (sqrt(g0 * g0 + 4.0 * g1 * rrs_s[iw]) - g0) / (2.0 * g1);
    }

    /*Determine whether to use 550 or 670 nm as reference then compute total*/
    /*absorption at the reference wavelength, aref*/
    if (rrs_s[idx670] >= 0.0015) {
        aref = aw[idx670] + 0.07 * pow(rrs_a[idx670] / rrs_s[idx443], 1.1);
        idxref = idx670;

    } else {
        numer = rrs_a[idx443] + rrs_a[idx488];
        denom = rrs_a[idx550] + 5 * rrs_a[idx670]*(rrs_a[idx670] / rrs_a[idx488]);
        rho = log10(numer / denom);
        rho = acoefs[0] + acoefs[1] * rho + acoefs[2] * rho*rho;
        aref = aw[idx550] + pow(10.0, rho);
        idxref = idx550;
    }

    /*Calculate the backscattering coefficient at the reference wavelength*/
    bbpref = ((u[idxref] * aref) / (1.0 - u[idxref])) - bbw[idxref];

    /*Steps to compute power exponent of bbp coefficient*/
    rat = rrs_s[idx443] / rrs_s[idx550];
    Ybbp = 2.0 * (1.0 - 1.2 * exp(-0.9 * rat));

    /*Compute backscattering coefficient at 443nm, this is needed later for */
    /*calculating bbp at the Raman excitation bands.s                        */
    bbp443 = bbpref * pow((lambda[idxref] / lambda[idx443]), Ybbp);

    /*Compute the total backscattering coefficient for sensor bands*/
    for (iw = 0; iw < nbandVis; iw++) {
        bbt[iw] = bbpref * pow((lambda[idxref] / lambda[iw]), Ybbp) + bbw[iw];
    }

    /*Calculate the total absorption coefficient at sensor bands*/
    for (iw = 0; iw < nbandVis; iw++) {
        at[iw] = ((1.0 - u[iw]) * bbt[iw]) / u[iw];
    }

    /*Calculate the exponential slope coefficient of absorption by colored */
    /*dissolved and detrital matter */
    Sdg = 0.015 + 0.002 / (0.6 + rrs_s[idx443] / rrs_s[idxref]);

    /*Compute the absorption coefficient of colored dissolved and detrital */
    /*matter at 443 nm, adg443*/
    zeta = 0.74 + 0.2 / (0.8 + rrs_s[idx443] / rrs_s[idxref]);
    xi = exp(Sdg * (lambda[idx443] - lambda[idx412]));
    term1 = (at[idx412] - zeta * at[idx443]) - (aw[idx412] - zeta * aw[idx443]);
    term2 = xi - zeta;
    adg443 = term1 / term2;

    /*Calculate the absorption of phytoplankton at 443 nm*/
    aph443 = at[idx443] - adg443 - aw[idx443];

}

/* -----------------------------------------------------------------------*/
/* raman_iops() - computes IOPs at raman excitation bands using a    */
/*                      bio-optical model                                 */

/* -----------------------------------------------------------------------*/
void raman_and_sensor_iops() {

    int iw;
    float bbpR, adgR, aphR;
    float bbp, adg, aph;

    /*loop over sensor and raman bands*/
    for (iw = 0; iw < nbandVis; iw++) {

        /*Compute IOPs at Raman bands*/
        bbpR = bbp443 * pow((lambda[idx443] / ramLam[iw]), Ybbp);
        adgR = adg443 * exp(-Sdg * (ramLam[iw] - lambda[idx443]));
        aphR = aph443 * aphStarR[iw];

        /*Compute IOPs at sensor bands*/
        bbp = bbp443 * pow((lambda[idx443] / lambda[iw]), Ybbp);
        adg = adg443 * exp(-Sdg * (lambda[iw] - lambda[idx443]));
        aph = aph443 * aphStar[iw];

        /*total absorption and backscattering coefficients at Raman bands*/
        atotR[iw] = awR[iw] + adgR + aphR;
        bbtotR[iw] = bbwR[iw] + bbpR;

        /*total absorption and backscattering coefficients at sensor bands*/
        atot[iw] = aw[iw] + adg + aph;
        bbtot[iw] = bbw[iw] + bbp;
    }

}

/* -----------------------------------------------------------------------*/
/* k_functions() - computes k functions from IOP bio-optical models       */

/* -----------------------------------------------------------------------*/
void raman_k_func(l2str *l2rec, int ip) {

    int iw;
    float senzRad, solzRad, cosSenz, ucoeffS, muD, muU, duC;

    //Convert solar and sensor zenith angles from degrees to radianss
    solzRad = l2rec->l1rec->solz[ip]*(M_PI / 180.0);
    senzRad = l2rec->l1rec->senz[ip]*(M_PI / 180.0);

    //Means consines
    cosSenz = cos(senzRad);
    muD = cos(solzRad);
    muU = 0.5;

    for (iw = 0; iw < nbandVis; iw++) {

        ucoeffS = bbtot[iw] / (atot[iw] + bbtot[iw]);

        //Diffuse attenuation coefficient
        kdSen[iw] = (atot[iw] + bbtot[iw]) / muD;
        kdRam[iw] = (atotR[iw] + bbtotR[iw]) / muD;

        kappaSen[iw] = (atot[iw] + bbtot[iw]) / muU;
        kappaRam[iw] = (atotR[iw] + bbtotR[iw]) / muU;

        //Pathlength elongation factor Lee et al.  1998,1999.
        duC = 1.03 * pow((1.0 + 2.4 * ucoeffS), 0.5);
        kLuSen[iw] = (atot[iw] + bbtot[iw]) * duC / cosSenz;

    }

}

/* -----------------------------------------------------------------------*/
/* radtran_raman() - calculate total downwelling irradiances              */

/* -----------------------------------------------------------------------*/
void raman_radtran_ed(l2str *l2rec, int ip) {

    int iw;

    //Constants
    float p0 = 29.92 * 33.8639; //standard pressure (convert mmHg -> HPa);
    float nWater = 1.341; //Refractive index of seawater
    float airDens = 1.2E3; //Density of air g/m^3
    float radCon = M_PI / 180.0; //degrees -> radians conversion

    //Define model variables
    float solzRad, cosSolz;
    float asymFac, B3, B2, B1, Fa;
    float mPres, mAtm, mOz;
    float cDrag, rhoFm, rhoSSP, rhoDSP, rhoD, rhoS, thetaDiff, thetaSum, sinTerm, tanTerm;
    float f0Em, f0Ex, tauEx, tauEm;
    float tRayEm, tRayEx;
    float tOzEm, tOzEx, tAerEm, tAerEx, tAerAEm, tAerAEx, tAerSEm, tAerSEx;
    float tGasEm, tGasEx, tWatEm, tWatEx, eDDEx, eDDEm, iRayEx, iRayEm, iAerEx;
    float iAerEm, eDSEx, eDSEm;
    float solz, airPres, clmWater, windSpd, oZone, omega;

    //Load per-pixel variables from L2 record
    float f0Cor = l2rec->l1rec->fsol; //Earth-sun distance correction
    solz = l2rec->l1rec->solz[ip]; //solar zenith
    airPres = l2rec->l1rec->pr[ip]; //surface pressure in hPa
    clmWater = l2rec->l1rec->wv[ip]; //water column vapour
    windSpd = l2rec->l1rec->ws[ip]; //wind speed m/s
    oZone = l2rec->l1rec->oz[ip]; //ozone height in cm
    omega = l2rec->eps[ip]; //Aerosol single scattering albedo

    //Estimate the aerosol optical thickness as Raman bands (assume that taua
    //follows a power law - note: this may not always hold true in the UV)
    fit_raman_taua(l2rec, ip);

    //Convert solar zenith in degrees to radians
    solzRad = solz*radCon;

    //Calculate cosine of the solar zenith angle
    cosSolz = cos(solzRad);

    //Calculate Raman band asymmetry parameter based on the Angstrom coefficient
    if (angstEst > 1.2) {
        asymFac = 0.65;
    } else if (angstEst < 0.0) {
        asymFac = 0.82;
    } else {
        asymFac = -0.14167 * angstEst + 0.82;
    }

    //Calculate the sensor forward scattering probability coefficients
    B3 = log(1.0 - asymFac);
    B1 = B3 * (1.459 + B3 * (0.1595 + B3 * 0.4129));
    B2 = B3 * (0.0783 + B3 * (-0.3824 - B3 * 0.5874));

    //Forward scattering probability sensor bands
    Fa = 1.0 - 0.5 * exp((B1 + B2 * cosSolz) * cosSolz);

    //---------------------    
    //Calculate the atmospheric pathlengths
    //Use the Kasten abd Young (1989) coefficients
    // mAtm = 1.0 / ( cosSolz + 0.50572*(96.07995 - solz)**(-1.6364) );
    mAtm = 1.0 / (cosSolz + 0.50572 * pow((86.07995 - solz), -1.6364));

    //Greg and Carder (1990) values **NOT USED**
    //mAtm = 1.0 / ( np.cos(solzRad) + 0.15* pow((93.885 - solz),-1.253) );

    //Pathlength corrected for non-standard atmoshperic pressure
    mPres = mAtm * (airPres / p0);

    //Calculate Ozone pathlength
    mOz = 1.00345 / (pow(cosSolz * cosSolz + 0.0069, 0.5));

    //---------------------  
    //Calculate surface reflectance (diffuse and direct)
    //Calculate the foam reflectance and diffuse specular reflectance
    if (windSpd > 4.0) {

        if (windSpd <= 7.0) {
            cDrag = 6.2E-4 + (1.56E-3 / windSpd);
            rhoFm = airDens * cDrag * 2.2E-5 * windSpd * windSpd - 4.04E-4;
        } else {
            cDrag = 0.49E-3 + 0.065E-3 * windSpd;
            rhoFm = (airDens * cDrag * 4.5E-5 - 4.0E-5) * windSpd * windSpd;
        }

        rhoSSP = 0.057;

    } else {
        rhoFm = 0.0;
        rhoSSP = 0.066;
    }

    //Calculate Fresnel reflectance of direct irradiance
    if (solz < 40.0 || windSpd < 2.0) {
        if (solz == 0.0) {
            rhoDSP = 0.0211;
        } else {
            thetaDiff = solzRad - asin(cosSolz / nWater);
            thetaSum = solzRad + asin(cosSolz / nWater);
            sinTerm = (pow(sin(thetaDiff), 2.0)) / (pow(sin(thetaSum), 2.0));
            tanTerm = (pow(tan(thetaDiff), 2.0)) / (pow(tan(thetaSum), 2.0));
            rhoDSP = 0.5 * (sinTerm + tanTerm);
        }
    } else {
        //Empirical fit for wind speeds greater than 2 m/s
        rhoDSP = 0.0253 * exp((-7.14E-4 * windSpd + 0.0618) *(solz - 40.));
    }

    //Total surface reflectances
    rhoD = rhoDSP + rhoFm; //Direct surface reflectance
    rhoS = rhoSSP + rhoFm; //Diffuse surface reflectance

    //---------------------//
    //Spectral calculations - loop over sensor
    for (iw = 0; iw < nbandVis; iw++) {

        //Correct for sun-earth distance
        f0Em = f0BarSen[iw] * f0Cor; //TOA solar irradiance at sensor bands
        f0Ex = f0BarRam[iw] * f0Cor; //TOA solar irradiance at Raman bands

        //Get taua at sensor and Raman bands
        tauEm = l2rec->taua[ip * l2rec->l1rec->l1file->nbands + iw];
        tauEx = tauaRam[iw];

        //printf("%f, %f, %f, %f\n", lambda[iw], tauEm, ramLam[iw], tauEx );

        //Compute Rayleigh scattering transmittance
        //Note convert nm -> um by factor of 1E-3
        tRayEm = 1.0 - raySen[iw];
        tRayEx = 1.0 - rayRam[iw];

        //Compute ozone transmittance
        tOzEm = exp(-(ozSen[iw]) * oZone * mOz);
        tOzEx = exp(-(ozRam[iw]) * oZone * mOz);

        //Calculate spectral aerosol transmitance coefficients...
        //Transmittance due to aeros
        tAerEm = exp(-tauEm * mAtm);
        tAerEx = exp(-tauEx * mAtm);

        //Transmittance after aerosol abasorption
        tAerAEm = exp(-(1.0 - omega) * tauEm * mAtm);
        tAerAEx = exp(-(1.0 - omega) * tauEx * mAtm);

        //Transmittance after aerosol scattering   
        tAerSEm = exp(-(omega) * tauEm * mAtm);
        tAerSEx = exp(-(omega) * tauEx * mAtm);

        //Gas Transmittance due to oxygen/gases
        tGasEm = exp((-1.41 * oxySen[iw] * mPres) / (pow((1.0 + 118.3 * oxySen[iw] * mPres), 0.45)));
        tGasEx = exp((-1.41 * oxyRam[iw] * mPres) / (pow((1.0 + 118.3 * oxyRam[iw] * mPres), 0.45)));

        //Water vapour transmittance
        tWatEm = exp((-0.2385 * wvSen[iw] * clmWater * mAtm) /
                (pow((1.0 + 20.07 * wvSen[iw] * clmWater * mAtm), 0.45)));
        tWatEx = exp((-0.2385 * wvRam[iw] * clmWater * mAtm) /
                (pow((1.0 + 20.07 * wvRam[iw] * clmWater * mAtm), 0.45)));

        //Calculate diffuse irradiance component due to Rayleigh Scattering
        iRayEx = f0Ex * cosSolz * tOzEx * tGasEx * tWatEx * tAerAEx * 0.5 * (1.0 - pow(tRayEx, 0.95));
        iRayEm = f0Em * cosSolz * tOzEm * tGasEm * tWatEm * tAerAEm * 0.5 * (1.0 - pow(tRayEm, 0.95));

        //Calculate the diffuse irradiance component due to aerosol scattering
        iAerEx = f0Ex * cosSolz * tOzEx * tGasEx * tWatEx * tAerAEx * pow(tRayEx, 1.5) * Fa * (1.0 - tAerSEx);
        iAerEm = f0Em * cosSolz * tOzEm * tGasEm * tWatEm * tAerAEm * pow(tRayEm, 1.5) * Fa * (1.0 - tAerSEm);

        //Calculate direct irradiance eDD
        //For Raman bands, calculate sub-surface values 0-
        eDDEx = f0Ex * cosSolz * tRayEx * tAerEx * tOzEx * tGasEx * tWatEx * (1.0 - rhoD);
        //For sensor bands, calculate above-water values 0+ by leaving off the (1-rhoD) factor
        eDDEm = f0Em * cosSolz * tRayEm * tAerEm * tOzEm * tGasEm*tWatEm;

        //Total diffuse irradiance eDS
        //For Raman bands, calculate sub-surface values 0-
        eDSEx = (iRayEx + iAerEx)*(1.0 - rhoS);
        //For sensor bands, calculate above-water values 0+ by leaving off the (1-rhoS) factor
        eDSEm = (iRayEm + iAerEm);

        //Total downwelling irradiance;        
        eDRam[iw] = eDDEx + eDSEx;
        eDSen[iw] = eDDEm + eDSEm;

    }

}

/* -------------------------------------------------------------------------*/
/* raman_cor_1() - computes the Rrs corrected for Raman scattering reported */
/*                  by Lee et al, 2013                                      */

/* -------------------------------------------------------------------------*/
void raman_cor_lee1(l2str *l2rec, int ip) {

    int iw;
    float Rrs443, Rrs550;
    float rFactor;
    float rrs_a;
    int nbands = l2rec->l1rec->l1file->nbands;
    //If pixel is already masked, return and do not attempt the correction.
    if (l2rec->l1rec->mask[ip]) {
        return;
    }

    //Get Rrs443 and Rrs550 from the L2 record
    Rrs443 = l2rec->Rrs[ip * nbands + idx443];
    Rrs550 = l2rec->Rrs[ip * nbands + idx550];

    //Loop over vis bands
    for (iw = 0; iw < 6; iw++) {

        //Raman correction factor - Eq. 11 in Lee et al 2013
        rFactor = alphaCor[iw] * Rrs443 / Rrs550 + betaCor0[iw] * pow(Rrs550, betaCor1[iw]);

        rrs_a = l2rec->Rrs[ip * nbands + iw];

        Rrs_ram[ip * nbands + iw] = rrs_a - rrs_a / (1.0 + rFactor);

    }


}

/* -----------------------------------------------------------------------*/
/* raman_cor_westberry() - computes the Rrs corrected for Raman scattering*/
/*                          following the method of Westberry et al 2013  */

/* -----------------------------------------------------------------------*/
void raman_cor_westberry(l2str *l2rec, int ip) {

    int iw;
    float term0, term1, term2, term3;
    float numer1, denom1, numer2, denom2, numer3, denom3;
    float nWater = 1.341; //Refractive index of seawater
    float muU = 0.5;
    float Rrs_rc;

    //float *Rrs_raman = &l2rec->Rrs_raman[ip*l2rec->nbands];


    //If pixel is already masked, return and do not attempt the correction.
    if (l2rec->l1rec->mask[ip]) {
        return;
    }

    //Extrapolate aphStar into the UV Raman excitation bands
    set_raman_aph_uv(l2rec, ip);

    //Run QAA to estimate IOP magnitudes and spectral slopes
    raman_qaa(l2rec, ip);

    //Get total absorption and scattering coefficients at both sensor
    //and Raman excitation bands.
    raman_and_sensor_iops();

    //Get the k-functions
    raman_k_func(l2rec, ip);

    //Get the down-welling irradiances
    raman_radtran_ed(l2rec, ip);

    //From Mobley 2010 - assume isotropic emission
    term0 = 1.0 / (4.0 * M_PI * nWater * nWater);

    //Loop over visible bands    
    for (iw = 0; iw < nbandVis; iw++) {

        //First term in in Westberry et al. 2013, Eq 7.
        numer1 = bRex[iw] * eDRam[iw];
        denom1 = (kdRam[iw] + kLuSen[iw]) * eDSen[iw];
        //denom1 = (kdRam[iw]+kappaSen[iw])*eDSen[iw];
        term1 = numer1 / denom1;

        //Second term in in Westberry et al. 2013, Eq 7.
        numer2 = bbtotR[iw];
        denom2 = muU * (kdRam[iw] + kappaRam[iw]);
        term2 = numer2 / denom2;

        //Third term in in Westberry et al. 2013, Eq 7.
        numer3 = bbtot[iw];
        //denom3 = 2.0*muU*kappaSen[iw];
        denom3 = kappaSen[iw];
        term3 = numer3 / denom3;

        //Raman corrected Rrs
        Rrs_rc = term0 * term1 * (1.0 + term2 + term3);

        /*Avoid negative Raman scattering returns*/
        /*A subtraction of -ve Rrs_raman will increase total Rrs*/
        /*A subtraction of Rrs_raman > Rrs will give negative Rrs values*/
        if (Rrs_rc < 0.0 || Rrs_rc > l2rec->Rrs[ip * l2rec->l1rec->l1file->nbands + iw]) {
            Rrs_rc = 0.0;
        }

        //Fill Rrs_raman array
        Rrs_ram[ip * l2rec->l1rec->l1file->nbands + iw] = Rrs_rc;

    }

}

/* -----------------------------------------------------------------------*/
/* raman_cor_lee2() - computes the Rrs corrected for Raman scattering     */
/*                          following the method of Lee et al. 1994       */

/* -----------------------------------------------------------------------*/
void raman_cor_lee2(l2str *l2rec, int ip) {

    int iw;
    float Rrs_rc;

    //If pixel is already masked, return and do not attempt the correction.
    if (l2rec->l1rec->mask[ip]) {
        return;
    }

    //Extrapolate aphStar into the UV Raman excitation bands
    set_raman_aph_uv(l2rec, ip);

    //Run QAA to estimate IOP magnitudes and spectral slopes
    raman_qaa(l2rec, ip);

    //Get total absorption and scattering coefficients at both sensor
    //and Raman excitation bands.
    raman_and_sensor_iops();

    //Get the down-welling irradiances
    raman_radtran_ed(l2rec, ip);

    //Loop over vis bands
    for (iw = 0; iw < nbandVis; iw++) {

        //Raman-corrected Rrs 
        Rrs_rc = 0.072 * (bRex[iw] * eDRam[iw]) / ((2.0 * atot[iw] + atotR[iw]) * eDSen[iw]);

        //Avoid negative Raman scattering returns
        if (Rrs_rc < 0.0) {
            Rrs_rc = 0.0;
        }

        //Fill Rrs_raman array
        Rrs_ram[ip * l2rec->l1rec->l1file->nbands + iw] = Rrs_rc;

    }


}

/* -----------------------------------------------------------------------*/
/* run_raman_cor() - run the Raman scattering correction algorithm        */

/* -----------------------------------------------------------------------*/
void run_raman_cor(l2str *l2rec, int ip) {

    int iw;
    static int firstCall = 1;
    static int sensorSupported;
    int32_t ocSensorID;

    //Number of visible bands
    nbandVis = rdsensorinfo(l2rec->l1rec->l1file->sensorID, input->evalmask, "NbandsVIS", NULL);

    //First call 
    if (firstCall) {

        ocSensorID = l2rec->l1rec->l1file->sensorID;

        //Supported sensors list
        if (ocSensorID == MODISA || ocSensorID == MODIST || ocSensorID == SEAWIFS  \
                || ocSensorID == VIIRSN || ocSensorID == MERIS || ocSensorID == OCTS) {

            //Set flag to 1 - supported sensor
            sensorSupported = 1;

        } else {

            //Force l2gen input to raman_opt=0 
            input->raman_opt = NORAMAN;

            //Set flag to 1 - supported sensor
            sensorSupported = 0;
        }

        //No Raman correction selected
        if (input->raman_opt == 0) {

            printf("\n");
            if (!sensorSupported) {
                printf("Raman correction unsupported for this sensor. \n");
            }

            printf("No Raman scattering correction calculated for Rrs. \n");
            printf("\n");

            //Raman correction selected
        } else if (input->raman_opt > 0 && input->raman_opt <= 3) {

            printf("\n");
            printf("Compute Raman scattering correction #%d. \n", input->raman_opt);
            printf("\n");

            //Invalid Raman correction selected
        } else {
            printf("-E- %s line %d : '%d' is not a valid Raman correction option.\n",
                    __FILE__, __LINE__, input->raman_opt);
            exit(1);
        }


        /*Allocate memory according to number of sensor bands*/
        raman_pixel_alloc(l2rec);

        //If sensor is supported for Raman correction, coefficients are allocated
        if (sensorSupported) {

            /*Get the sensor-specific coefficients*/
            get_raman_coeffs(l2rec);

            /*Loop over visible bands*/
            for (iw = 0; iw < nbandVis; iw++) {

                //Get the visible band centers
                lambda[iw] = l2rec->l1rec->l1file->fwave[iw];

                //Interpolate the Lee et al. (2013) correction coefficients to sensor resolution
                alphaCor[iw] = linterp(lamCor1, alpha, 6, lambda[iw]);
                betaCor0[iw] = linterp(lamCor1, beta0, 6, lambda[iw]);
                betaCor1[iw] = linterp(lamCor1, beta1, 6, lambda[iw]);

                //Populate the pure water absorption data//
                awR[iw] = aw_spectra(ramLam[iw], ramBandW[iw]);
                bbwR[iw] = bbw_spectra(ramLam[iw], ramBandW[iw]);
                aw[iw] = l2rec->l1rec->l1file->aw[iw];
                bbw[iw] = l2rec->l1rec->l1file->bbw[iw];
            }

            //Find the sensor wavelength indices, do this once.
            idx412 = windex(412.0, lambda, nbandVis);
            idx443 = windex(443.0, lambda, nbandVis);
            idx488 = windex(488.0, lambda, nbandVis);
            idx550 = windex(547.0, lambda, nbandVis);
            idx670 = windex(667.0, lambda, nbandVis);

            //How many bands between 412 and 490?
            for (iw = 0; iw < nbandVis; iw++) {
                if (lambda[iw] > 550.0) {
                    bandNum550 = iw - 1;
                    break;
                }
            }

        }
        firstCall = 0;
    }

    /*record that we have run the model on this scan line*/
    //ramanScanNum = l2rec->l1rec->iscan;               

    /*No raman correction is applied, raman_opt=0*/
    if (input->raman_opt == NORAMAN) {
        /*Fill Rrs_raman with zeros*/
        for (iw = 0; iw < l2rec->l1rec->l1file->nbands; iw++) {
            Rrs_ram[ip * l2rec->l1rec->l1file->nbands + iw] = 0.0;
        }

        /*Apply the Lee et al 2013 correction, raman_opt=1*/
    } else if (input->raman_opt == LEE2013) {
        raman_cor_lee1(l2rec, ip);

        /*Apply the Westberry et al 2013 correction, raman_opt=2*/
    } else if (input->raman_opt == WESTBERRY2013) {
        raman_cor_westberry(l2rec, ip);

        /*Apply the Lee et al 1994 Raman correction, raman_opt=3*/
    } else if (input->raman_opt == LEE1994) {
        raman_cor_lee2(l2rec, ip);
    }


    l2rec->Rrs_raman = Rrs_ram;

}

/* ---------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/
