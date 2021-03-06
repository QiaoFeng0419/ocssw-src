/*******************************************************************************
NAME                            MOLLWEIDE

PURPOSE:	Transforms input longitude and latitude to Easting and
                Northing for the MOllweide projection.  The
                longitude and latitude must be in radians.  The Easting
                and Northing values will be returned in meters.

PROGRAMMER              DATE
----------              ----
D. Steinwand, EROS      May, 1991;  Updated Sept, 1992; Updated Feb, 1993
S. Nelson, EDC		Jun, 2993;	Made corrections in precision and
                                        number of iterations.

ALGORITHM REFERENCES

1.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.

2.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.
 *******************************************************************************/
#include "proj_cproj.h"
#include <stdint.h>

/* Variables common to all subroutines in this code file
  -----------------------------------------------------*/
static double lon_center; /* Center longitude (projection center) */
static double R; /* Radius of the earth (sphere) */
static double false_easting; /* x offset in meters			*/
static double false_northing; /* y offset in meters			*/

/* Initialize the Mollweide projection
  ------------------------------------*/
int molwforint(double r, double center_long, double false_east, double false_north)
/* r		(I) Radius of the earth (sphere) 	*/
/* center_long  (I) Center longitude 			*/
/* false_east	x offset in meters			*/
/* false_north	y offset in meters			*/ {
    /* Place parameters in static storage for common use
      -------------------------------------------------*/
    false_easting = false_east;
    false_northing = false_north;
    R = r;
    lon_center = center_long;
    return (OK);
}

/* Mollweide forward equations--mapping lat,long to x,y
  ----------------------------------------------------*/
int molwfor(double lon, double lat, double *x, double *y)
/* lon	(I) Longitude */
/* lat	(I) Latitude */
/* x	(O) X projection coordinate */
/* y	(O) Y projection coordinate */ {
    double adjust_lon(double lon); /* Function to adjust longitude to -180 - 180 */
    double delta_lon; /* Delta longitude (Given longitude - center */
    double theta;
    double delta_theta;
    double con;
    int32_t i;

    /* Forward equations
      -----------------*/
    delta_lon = adjust_lon(lon - lon_center);
    theta = lat;
    con = PI * sin(lat);

    /* Iterate using the Newton-Raphson method to find theta
      -----------------------------------------------------*/
    for (i = 0;; i++) {
        delta_theta = -(theta + sin(theta) - con) / (1.0 + cos(theta));
        theta += delta_theta;
        if (fabs(delta_theta) < EPSLN) break;
        if (i >= 50) {
            /*
                 p_error("Iteration failed to converge","Mollweide-forward");
             */
            return (241);
        }
    }
    theta /= 2.0;

    /* If the latitude is 90 deg, force the x coordinate to be "0 + false easting"
       this is done here because of precision problems with "cos(theta)"
       --------------------------------------------------------------------------*/
    if (PI / 2 - fabs(lat) < EPSLN)
        delta_lon = 0;
    *x = 0.900316316158 * R * delta_lon * cos(theta) + false_easting;
    *y = 1.4142135623731 * R * sin(theta) + false_northing;
    return (OK);
}
