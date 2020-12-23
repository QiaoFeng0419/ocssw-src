/*
 * createNCDF_viirs.c
 *
 *  Created on: Sep 2, 2016
 *      Author: rhealy
 */
#include <string>
#include "netcdf.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "nc4utils.h"
#include <sstream>
#include <iomanip>
#include "l1agen_viirs.h"

int createNCDF_viirs( int ncid, const char *sname, const char *lname,
                const char *standard_name, const char *units,
                void *fill_value,
                const char *flag_values, const char *flag_meanings,
                double low, double high, int nt,
                int rank, int *dimids) {

  int32_t varid;
  int i;
  int status;
  size_t dimlength;
  size_t newchunk;
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
  if ( rank == 3) {
    status = nc_inq_dimlen(ncid, dimids[2], &dimlength);
    if ( dimlength >= 3200) {
      chunksize[0] = 1;
      chunksize[1] = 16;
      chunksize[2] = 320;
      do_deflate = 1;
    }
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
      return(EXIT_FAILURE);
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


