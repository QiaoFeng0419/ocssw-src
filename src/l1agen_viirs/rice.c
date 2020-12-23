/**************************************************
    ***** WARNING  ******  WARNING   *****   WARNING   *******
         This Rice code version has the compilation of compression code
turned on.  Compilation of "main" is off because "HDF" is on. This is not
the decompress-only version in the VIIRS SDR software.  However, this
code can be used according to the "NPOESS Specific Permission" statements
below.
*********************************************** */
/*  BEGIN THIRD_PARTY_COPYRIGHT  */
/*==============================================================
The SZIP Science Data Lossless Compression Program is Copyright (C) 2001
Science & Technology Corporation @ UNM.  All rights released.
Copyright (C) 2003 Lowell H. Miles and Jack A. Venbrux.  Licensed to ICs, LLC,
for distribution by the University of Illinois' National Center for
Supercomputing Applications as a part of the HDF data storage and retrieval
file format and software library products package.  All rights reserved.
Do not modify or use for other purposes.

SZIP implements an extended Rice adaptive lossless compression algorithm for
sample data.  The primary algorithm was developed by R. F. Rice at Jet
Propulsion Laboratory.

SZIP embodies certain inventions patented by the National Aeronautics &
Space Administration.  United States Patent Nos. 5,448,642, 5,687,255,
and 5,822,457 have been licensed to ICs, LLC, for distribution with the
HDF data storage and retrieval file format and software library products.
All rights reserved.

Revocable, royalty-free, nonexclusive sublicense to use SZIP decompression
software routines and underlying patents is hereby granted by ICs, LLC, to
all users of and in conjunction with HDF data storage and retrieval file
format and software library products.

Revocable, royalty-free, nonexclusive sublicense to use SZIP compression
software routines and underlying patents for non-commercial, scientific use
only is hereby granted by ICs, LLC, to users of and in conjunction with HDF
data storage and retrieval file format and software library products.

For commercial use license to SZIP compression software routines and
underlying patents please contact ICs, LLC, at:
        ICs, LLC
        2600-A E. Seltice Way #234,
        Post Falls, ID 83854

        Phone: (208) 755-8990
        Fax: (208) 773-5747.

*************Begin NPOESS Specific Permission *******************
NPOESS Permissions: Email correspondence, dated February 6, 2007, from
Dr. Joseph J. Feeley, Chief Executive Officer, of ICs, LLC
2040 Warren Wagon Road, PO Box 2236, McCall ID 83638, USA, grants:
1. Raytheon Company is granted Royalty free permission to use this code
file (as modified) for the compression and decompression of all NPOESS data
packets.
2. Same permission is granted to the NPOESS Integrated Program Office,
all NPOESS contractors, and all U.S. Government agencies using NPOESS data.
  **
3. NPOESS data users may use SZIP decompression software routines and
underlying patents only in conjunction with decompressing NPOESS data in
accordance with and as limited by the above Third Party License.

4. All other rights reserved.  Do not modify or use for other purposes.
Further information available at http://www.hdfgroup.org/doc.resource/SZIP/
or by contacting ICs, LLC, at support@ICs4chips.com
*************End NPOESS Specific Permission *******************

==============================================================================*/
/*  END THIRD_PARTY_COPYRIGHT  */

/* BEGIN GOVERNMENT RIGHTS */
/**************************************************************************
* This code was partially developed under NASA, and other
* U.S. Government contracts.
*
* Patent/copyrights are retained as described in THIRD_PARTY_COPYRIGHT above.
*
* Please also see "LIMITATIONS" below.
*
****************************************************************************/
/* END GOVERNMENT RIGHTS */

/* BEGIN ITAR */
/* END ITAR CONTROL */

/* BEGIN CATEGORY */
/**************************************************************************
* CATEGORY: 1 (see SEN for definitions)
*
* rice.cpp
*
* Refer to Software Standards and Practices Manual (SSPM) for
* coding standards.
*
* NOTE: This code is exempt from NPOESS SSPM because it is Third Party,
* copyrighted code.  (Please see THIRD_PARTY_COPYRIGHT above)
*
**************************************************************************/
/* END CATEGORY */

/**************************************************************************
*
* NAME: rice.cpp
*
*
* USAGE:
* Call functions:  szip_uncompress_memory
* Called by:       viirs_decmp
*
*
* Parameter         Type            I/O    Description
* -----------       -----           ---    ----------------
* options_mask       int             I     processing options
* new_bits_per_pixel int             I     bits per output pixel
* new_pixels_per_block int           I     pixels per compressed block
* new_pixels_per_scanline int        I     pixels in scanline
* in                const char*      I     pointer to compressed buffer
* in_bytes          long             I     bytes of compressed data
* out               void*           I/O    pointer to buffer for output bytes
* out_pixels        long             I     maximum output pixels
*                   long          return   bytes in output buffer
*
*
* REVISION/EVENT HISTORY:
* DATE        PR#      AUTHOR            Build    DESCRIPTION
* ---------   ---      ------            -----    -----------
* 29AUG2006            Miles              1.5     received source from L. Miles
*                                                at ICs Corp. Source is same as
*                                                U. of Illinois HDF Library.
* ---------   ---      ------            -----    -----------
* 25SEP2006            Arbeiter           1.5     inserted modifications
*                                               developed by Jennings, SBRS
* ---------   ---      ------            -----    -----------
* 27SEP2006            Arbeiter           1.5     added NPOESS headings
* ---------   ---      ------            -----    -----------
* 16FEB2007            Arbeiter           1.5     added NPOESS specific
*                                               permissions to 3rd Party
*                                               copyright Notices.
* ---------   ---      ------            -----    -----------
* 16APR2007            Arbeiter           1.5     Updated header comments
* ---------   ---      ------            -----    -----------
* 21DEC2007            Arbeiter           1.5     Updated header comments
*                                               and LIMITATIONS notes
* ---------   ---      ------            -----    -----------
* 31JAN2008            Arbeiter          1.5x1    Limitations updated
* ---------   ---      ------            -----    -----------
* 21NOV2008            Arbeiter          1.5x1    Comment updates
* ---------   ---      ------            -----    -----------
*
* REFERENCES: None.
*
*
* LIMITATIONS:
* 1. This code is not generalized Rice Compress/Uncompress.  It will only
* work with the VIIRS sensor data.
*
* 2. Use of this software for data from satellites other than NPP or NPOESS
* is prohibited.
*
* 3. This code has only been tested with the numbers of pixels in the VIIRS
* aggregation or compression zones, or the number of calibration pixels:
* 16, 48, 64, 96, 368, 488, 592, 640, 736, 760, 784, 1184, 1280, and 1776.
* Results with other numbers of pixels may be unreliable.
*
* 4. Raytheon has only tested this code with settings for 15 bits per pixel,
* 8 pixels per block, and option mask for NN, MSB, RAW, and CHIP.  Results with
* other settings may be unreliable. (These are the settings used by
* the VIIRS sensor for all compressions.)
*
* NOTES (MISCELLANEOUS) SECTION:
* Raytheon Company, Bellevue, Nebraska
*  2006 Raytheon Company.  All Rights Reserved.
*  2007 Raytheon Company.  All Rights Reserved.
*  2008 Raytheon Company.  All Rights Reserved.
*
* Comment marking "MJJ" is code written by Michael J. Jennings,
* of Raytheon SBRS, Goleta, California, 04/28/2004.
*
* Comment marking "RGA" is code written by Randolph G. Arbeiter,
* of Raytheon IIS, Omaha, Nebraska, September 2006.
*
*******************************************************************************/

/*==============================================================================
To compile szip on the following operating systems do:

UNIX:      cc -o szip rice.c
MSDOS:     cc -o szip -DMSDOS rice.c
WINDOWS95: cc -o szip -DWINDOWS95 rice.c
WINDOSNT:  cc -o szip -DWINDOWS95 rice.c

If your compiler is not named "cc", then replace "cc" above with the name of
your compiler.

The szip program will run much faster when compiler optimization options are
used.

==============================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#if defined(MSDOS) || defined(WINDOWS95) 
#include <fcntl.h>
#endif
#if defined(WIN32)
#include <fcntl.h>
#include <io.h>
#endif
#include "rice.h"

/*  BEGIN Function prototypes for ANSI C compilation */

static void unmap_nn(unsigned* sigma, int pixels);
static void deinterleave(char* in, long bytes, int bits, char* out);

/*  END Function prototypes */

/*      VIIRS - used for NPOESS satellite sensor named VIIRS  */
#define VIIRS 1  /*** 1 if being used for VIIRS data */

#define HDF 1   /*** 1 if part of HDF package, or if compression needed ***/
//#define HDF 0

/* Uncomment the following line to compile out szip encoding */
/*  make the following line a comment to compile szip encoding */
/*           #define REMOVE_SZIP_ENCODER    */
#define REMOVE_SZIP_ENCODER

/* next two declarations are function pointer declarations */
/* modified to ANSI C function prototype declarations */

#if HDF
#include "ricehdf.h"

#define uncompress_memory szip_uncompress_memory
#define output_buffer_full szip_output_buffer_full
#define check_params szip_check_params
#endif /* HDF */

//char copyright[] = "@(#) (C) Copyright 1993,1994,1996 University of New Mexico.  All Rights Reserved.";

static boolean allow_k13 = TRUE;

static boolean msb_first = TRUE;
static boolean raw_mode = FALSE;

static boolean compress_exactly_as_chip;

boolean output_buffer_full;

/*** variables for reading data ***/
static long input_byte_count;
static unsigned char *input_byte_data;

static int bits_per_pixel;
static int blocks_per_scanline;
static int bytes_per_pixel;
static int compression_mode;
static int padded_pixels_per_scanline;
static int pixels_per_block;
static int pixels_per_scanline;

#if VIIRS == 1
int pixels_per_reference;
int blocks_per_reference;
int pixels_to_process;
#endif    /* VIIRS */

static int default_id;
static int xmax;

static long output_pixel_count;

static char *bptr;
static char *bmid;

static char output_buffer[OUTPUT_BUFFER_SIZE];

static unsigned char ext2_array1[MAX_EXT2_SUM+1];
static unsigned char ext2_array2[MAX_EXT2_SUM+1];

static int leading_zeros[256];

static int error_count;
static int warning_count;

#if 0  /*** set to 1 to check packed bits ***/
#define check_value(x, n) \
   { \
   if (x > n) \
      internal_error(__FILE__, __LINE__); \
   }
#else
#define check_value(x, n)
#endif

#define pack1(value, pbits) \
   { \
   packed_bits -= pbits; \
   check_value(packed_bits, 32); \
   packed_value |= value << packed_bits; \
   if (packed_bits <= 16) \
      { \
      unsigned long v16; \
      v16 = packed_value >> 16; \
      *bptr++ = v16 >> 8; \
      *bptr++ = v16 & 0xff; \
      packed_value <<= 16; \
      packed_bits += 16; \
      } \
   }

#define pack2(xvalue, xbits) \
   { \
   unsigned long value; \
   long pbits; \
   \
   pbits = xbits; \
   value = xvalue; \
   if (pbits > 16) \
      { \
      unsigned long v16; \
      v16 = value >> 16; \
      pbits -= 16; \
      pack1(v16, pbits); \
      value &= 0xffff; \
      pbits = 16; \
      } \
   \
   pack1(value, pbits); \
   }

#define packfs(xbits) \
   { \
   long pbits; \
   \
   pbits = xbits; \
   while (pbits > 16) \
      { \
      pack1(0, 16); \
      pbits -= 16; \
      } \
   pack1(1, pbits); \
   }

static void
warning(const char *fmt, ...)
{
/* Disable warning output if compiled as part of the HDF library */
   warning_count++;
}

static void
error(const char *fmt, ...)
{
  printf("%s\n", fmt);
  error_count++;
}

static int
getch()
{
   int ch;
      {
      ch = *input_byte_data++;
      if (input_byte_count == 0)
         {
         error("Premature end of memory while reading header.\n");
         return 0;
         }

      input_byte_count--;
      return ch;
      }
}

static void
read_header()
{
   int mode;
   int mult;
   long scanline_count;
   unsigned long value;

   value = getch();
   value = (value << 8) | getch();
   if (value & 0x8000)
      {
      /*** 1bppnnnjjjssss-- ***/
      msb_first = (value >> 14) & 1;
      mode = (value >> 12) & 3;
      bits_per_pixel = short_header.bits_per_pixel[(value >> 9) & 7]; 
      pixels_per_block = short_header.pixels_per_block[(value >> 6) & 7]; 
      mult = short_header.pixels_per_block_mult[(value >> 2) & 15];
      pixels_per_scanline = pixels_per_block * mult;
      }
   else if (value & 0x4000)
      {
      /*** 01bpppnnnnjjjjjssssssssssssss--- ***/
      value = (value << 8) | getch();
      value = (value << 8) | getch();
      msb_first = (value >> 29) & 1;
      mode = (value >> 26) & 7;
      bits_per_pixel = ((value >> 22) & 0xf) + 1; 
      pixels_per_block = ((value >> 17) & 0x1f) * 2 + 2;
      pixels_per_scanline = ((value >> 3) & 0x3fff) + 1;
      }
   else if (value & 0x2000)
      {
      /*** 001bpppnnnnnnjjjjjssssssssssssss ***/
      value = (value << 8) | getch();
      value = (value << 8) | getch();
      msb_first = (value >> 28) & 1;
      mode = (value >> 25) & 7;
      bits_per_pixel = ((value >> 19) & 0x3f) + 1; 
      pixels_per_block = ((value >> 14) & 0x1f) * 2 + 2;
      pixels_per_scanline = (value & 0x3fff) + 1;
      }
   else
      {
      error("Header format error - sz file has been corrupted.\n");
      return;
      }

   value = getch();
   if (value == 0)
      output_pixel_count = 0x7fffffff;
   else if (value & 0x80)
      {
      /*** 1vvvvvvv ***/
      scanline_count = short_header.scanlines_per_file[value & 0x7f];
      output_pixel_count = scanline_count * pixels_per_scanline;
      }
   else if (value & 0x40)
      {
      /*** 01vvvvvvvvvvvvvvvvvvvvvvvvvvvvv ***/
      value = (value << 8) | getch();
      value = (value << 8) | getch();
      value = (value << 8) | getch();

      output_pixel_count = value & 0x3fffffff;
      }
   else
      {
      error("Unknown file size format in input file.\n");
      return;
      }

   if (mode == 0 || mode == 1)
      compression_mode = mode ? NN_MODE : EC_MODE;
   else
      {
      error("This decoder program does not support the encoded mode.\n");
      return;
      }
}


static void
decode_initialize()
{
   int i;
   int index;
   int j;
   int k;
   int *p;

   output_pixel_count = 0x7fffffff;
   if (!raw_mode)
      {
      read_header();
      if (error_count)
         return;
      }

   blocks_per_scanline = (pixels_per_scanline + pixels_per_block - 1)/pixels_per_block;
   padded_pixels_per_scanline = blocks_per_scanline * pixels_per_block;

   if (bits_per_pixel > 16)
      {
      bytes_per_pixel = 4;
      default_id = ID_DEFAULT3;
      }
   else if (bits_per_pixel > 8)
      {
      bytes_per_pixel = 2;
      default_id = ID_DEFAULT2;
      }
   else 
      {
      bytes_per_pixel = 1;
      default_id = ID_DEFAULT1;
      }

   xmax = (1 << bits_per_pixel) - 1;

   bptr = output_buffer;
   bmid = output_buffer + sizeof(output_buffer)/2;

   p = leading_zeros;
   *p++ = 8;
   for (i = 1, k = 7; i < 256; i += i, k--)
      for (j = 0; j < i; j++)
         *p++ = k;

   for (i = 0; i <= MAX_EXT2; i++)
      for (j = 0; j <= MAX_EXT2-i; j++)
         {
         index = (i+j)*(i+j+1)/2 + j;
         ext2_array1[index] = i;
         ext2_array2[index] = j;
         }

   output_buffer_full = FALSE;
}


/* changed ANSI declaration */
static void
deinterleave(char* in,
             long  bytes,
             int   bits,
             char* out)
{
   char *b;
   char *s;
   int i;
   int j;
   int words;
   int word_size;

   word_size = bits/8;
   words = bytes/word_size;
   b = in;
   for (j = 0; j < word_size; j++)
      {
      s = out + j;
      for (i = 0; i < words; i++)
         {
         *s = *b++; 
         s += word_size;
         }
      }
}

/* changed declaration to ANSI C */
static void
unmap_nn(unsigned* sigma, int pixels)
{
   char *xptr;
   int sig1;
   int sig2;
   int x;
   unsigned *end;
   unsigned *s;

   end = sigma + pixels;
   s = sigma;
   xptr = bptr;
   if (pixels & 1)
      {
      if (bytes_per_pixel == 1)
         {
         x = *s++;
         *xptr++ = x;

         while (s < end)
            {
            sig1 = *s++;
            if (sig1 >= (x << 1))
               x = sig1;
            else if (sig1 > (xmax - x) << 1)
               x = xmax - sig1;
            else if (sig1 & 1)
               x = x - ((sig1 + 1) >> 1);
            else 
               x = x + (sig1 >> 1);
   
            *xptr++ = x;
            }

         bptr += pixels;
         }
      else if (bytes_per_pixel == 2)
         {
         if (msb_first)
            {
            x = *s++;
            *xptr++ = (unsigned) x >> 8;
            *xptr++ = x;

            while (s < end)
               {
               sig1 = *s++;
               if (sig1 >= (x << 1))
                  x = sig1;
               else if (sig1 > (xmax - x) << 1)
                  x = xmax - sig1;
               else if (sig1 & 1)
                  x = x - ((sig1 + 1) >> 1);
               else 
                  x = x + (sig1 >> 1);
   
               *xptr++ = (unsigned) x >> 8;
               *xptr++ = x;
               }

            bptr += pixels << 1;
            }
         else
            {
            x = *s++;
            *xptr++ = x;
            *xptr++ = (unsigned) x >> 8;

            while (s < end)
               {
               sig1 = *s++;
               if (sig1 >= (x << 1))
                  x = sig1;
               else if (sig1 > (xmax - x) << 1)
                  x = xmax - sig1;
               else if (sig1 & 1)
                  x = x - ((sig1 + 1) >> 1);
               else 
                  x = x + (sig1 >> 1);
   
               *xptr++ = x;
               *xptr++ = (unsigned) x >> 8;
               }

            bptr += pixels << 1;
            }
         }
      else
         {
         if (msb_first)
            {
            x = *s++;
            *xptr++ = (unsigned) x >> 24;
            *xptr++ = (unsigned) x >> 16;
            *xptr++ = (unsigned) x >>  8;
            *xptr++ = x;

            while (s < end)
               {
               sig1 = *s++;
               if (sig1 >= (x << 1))
                  x = sig1;
               else if (sig1 > (xmax - x) << 1)
                  x = xmax - sig1;
               else if (sig1 & 1)
                  x = x - ((sig1 + 1) >> 1);
               else 
                  x = x + (sig1 >> 1);

               *xptr++ = (unsigned) x >> 24;
               *xptr++ = (unsigned) x >> 16;
               *xptr++ = (unsigned) x >>  8;
               *xptr++ = x;
               }

            bptr += pixels << 2;
            }
         else
            {
            x = *s++;
            *xptr++ = x;
            *xptr++ = (unsigned) x >>  8;
            *xptr++ = (unsigned) x >> 16;
            *xptr++ = (unsigned) x >> 24;

            while (s < end)
               {
               sig1 = *s++;
               if (sig1 >= (x << 1))
                  x = sig1;
               else if (sig1 > (xmax - x) << 1)
                  x = xmax - sig1;
               else if (sig1 & 1)
                  x = x - ((sig1 + 1) >> 1);
               else 
                  x = x + (sig1 >> 1);

               *xptr++ = x;
               *xptr++ = (unsigned) x >>  8;
               *xptr++ = (unsigned) x >> 16;
               *xptr++ = (unsigned) x >> 24;
               }

            bptr += pixels << 2;
            }
         }
      }
   else
      {
      if (bytes_per_pixel == 1)
         {
         x = *s++;
         *xptr++ = x;

         sig1 = *s++;
         if (sig1 >= (x << 1))
            x = sig1;
         else if (sig1 > (xmax - x) << 1)
            x = xmax - sig1;
         else if (sig1 & 1)
            x = x - ((sig1 + 1) >> 1);
         else 
            x = x + (sig1 >> 1);

         *xptr++ = x;
   
         while (s < end)
            {
            sig1 = *s++;
            sig2 = *s++;
            if (sig1 >= (x << 1))
               x = sig1;
            else if (sig1 > (xmax - x) << 1)
               x = xmax - sig1;
            else if (sig1 & 1)
               x = x - ((sig1 + 1) >> 1);
            else 
               x = x + (sig1 >> 1);

            *xptr++ = x;

            if (sig2 >= (x << 1))
               x = sig2;
            else if (sig2 > (xmax - x) << 1)
               x = xmax - sig2;
            else if (sig2 & 1)
               x = x - ((sig2 + 1) >> 1);
            else 
               x = x + (sig2 >> 1);

            *xptr++ = x;
            }

         bptr += pixels;
         }
      else if (bytes_per_pixel == 2)
         {
         if (msb_first)
            {
            x = *s++;
            *xptr++ = (unsigned) x >> 8;
            *xptr++ = x;

            sig1 = *s++;
            if (sig1 >= (x << 1))
               x = sig1;
            else if (sig1 > (xmax - x) << 1)
               x = xmax - sig1;
            else if (sig1 & 1)
               x = x - ((sig1 + 1) >> 1);
            else 
               x = x + (sig1 >> 1);

            *xptr++ = (unsigned) x >> 8;
            *xptr++ = x;

            while (s < end)
               {
               sig1 = *s++;
               sig2 = *s++;
               if (sig1 >= (x << 1))
                  x = sig1;
               else if (sig1 > (xmax - x) << 1)
                  x = xmax - sig1;
               else if (sig1 & 1)
                  x = x - ((sig1 + 1) >> 1);
               else 
                  x = x + (sig1 >> 1);

               *xptr++ = (unsigned) x >> 8;
               *xptr++ = x;

               if (sig2 >= (x << 1))
                  x = sig2;
               else if (sig2 > (xmax - x) << 1)
                  x = xmax - sig2;
               else if (sig2 & 1)
                  x = x - ((sig2 + 1) >> 1);
               else 
                  x = x + (sig2 >> 1);

               *xptr++ = (unsigned) x >> 8;
               *xptr++ = x;
               }

            bptr += pixels << 1;
            }
         else
            {
            x = *s++;
            *xptr++ = x;
            *xptr++ = (unsigned) x >> 8;

            sig1 = *s++;
            if (sig1 >= (x << 1))
               x = sig1;
            else if (sig1 > (xmax - x) << 1)
               x = xmax - sig1;
            else if (sig1 & 1)
               x = x - ((sig1 + 1) >> 1);
            else 
               x = x + (sig1 >> 1);

            *xptr++ = x;
            *xptr++ = (unsigned) x >> 8;

            while (s < end)
               {
               sig1 = *s++;
               sig2 = *s++;
               if (sig1 >= (x << 1))
                  x = sig1;
               else if (sig1 > (xmax - x) << 1)
                  x = xmax - sig1;
               else if (sig1 & 1)
                  x = x - ((sig1 + 1) >> 1);
               else 
                  x = x + (sig1 >> 1);

               *xptr++ = x;
               *xptr++ = (unsigned) x >> 8;

               if (sig2 >= (x << 1))
                  x = sig2;
               else if (sig2 > (xmax - x) << 1)
                  x = xmax - sig2;
               else if (sig2 & 1)
                  x = x - ((sig2 + 1) >> 1);
               else 
                  x = x + (sig2 >> 1);

               *xptr++ = x;
               *xptr++ = (unsigned) x >> 8;
               }

            bptr += pixels << 1;
            }
         }
      else
         {
         if (msb_first)
            {
            x = *s++;
            *xptr++ = (unsigned) x >> 24;
            *xptr++ = (unsigned) x >> 16;
            *xptr++ = (unsigned) x >>  8;
            *xptr++ = x;

            sig1 = *s++;
            if (sig1 >= (x << 1))
               x = sig1;
            else if (sig1 > (xmax - x) << 1)
               x = xmax - sig1;
            else if (sig1 & 1)
               x = x - ((sig1 + 1) >> 1);
            else 
               x = x + (sig1 >> 1);

            *xptr++ = (unsigned) x >> 24;
            *xptr++ = (unsigned) x >> 16;
            *xptr++ = (unsigned) x >>  8;
            *xptr++ = x;

            while (s < end)
               {
               sig1 = *s++;
               sig2 = *s++;
               if (sig1 >= (x << 1))
                  x = sig1;
               else if (sig1 > (xmax - x) << 1)
                  x = xmax - sig1;
               else if (sig1 & 1)
                  x = x - ((sig1 + 1) >> 1);
               else 
                  x = x + (sig1 >> 1);

               *xptr++ = (unsigned) x >> 24;
               *xptr++ = (unsigned) x >> 16;
               *xptr++ = (unsigned) x >>  8;
               *xptr++ = x;

               if (sig2 >= (x << 1))
                  x = sig2;
               else if (sig2 > (xmax - x) << 1)
                  x = xmax - sig2;
               else if (sig2 & 1)
                  x = x - ((sig2 + 1) >> 1);
               else 
                  x = x + (sig2 >> 1);

               *xptr++ = (unsigned) x >> 24;
               *xptr++ = (unsigned) x >> 16;
               *xptr++ = (unsigned) x >>  8;
               *xptr++ = x;
               }

            bptr += pixels << 2;
            }
         else
            {
            x = *s++;
            *xptr++ = x;
            *xptr++ = (unsigned) x >>  8;
            *xptr++ = (unsigned) x >> 16;
            *xptr++ = (unsigned) x >> 24;
   
            sig1 = *s++;
            if (sig1 >= (x << 1))
               x = sig1;
            else if (sig1 > (xmax - x) << 1)
               x = xmax - sig1;
            else if (sig1 & 1)
               x = x - ((sig1 + 1) >> 1);
            else 
               x = x + (sig1 >> 1);
   
            *xptr++ = x;
            *xptr++ = (unsigned) x >>  8;
            *xptr++ = (unsigned) x >> 16;
            *xptr++ = (unsigned) x >> 24;
   
            while (s < end)
               {
               sig1 = *s++;
               sig2 = *s++;
               if (sig1 >= (x << 1))
                  x = sig1;
               else if (sig1 > (xmax - x) << 1)
                  x = xmax - sig1;
               else if (sig1 & 1)
                  x = x - ((sig1 + 1) >> 1);
               else 
                  x = x + (sig1 >> 1);
   
               *xptr++ = x;
		               *xptr++ = (unsigned) x >>  8;
               *xptr++ = (unsigned) x >> 16;
               *xptr++ = (unsigned) x >> 24;
   
               if (sig2 >= (x << 1))
                  x = sig2;
               else if (sig2 > (xmax - x) << 1)
                  x = xmax - sig2;
               else if (sig2 & 1)
                  x = x - ((sig2 + 1) >> 1);
               else 
                  x = x + (sig2 >> 1);
   
               *xptr++ = x;
               *xptr++ = (unsigned) x >>  8;
               *xptr++ = (unsigned) x >> 16;
               *xptr++ = (unsigned) x >> 24;
               }
   
            bptr += pixels << 2;
            }
         }
      }
}

/* changed declaration to ANSI C - 14SEP2006 */
static void
output_decoded_data(unsigned* sigma)
{
   int i;
   int pixels;

#if VIIRS == 1
    pixels = (output_pixel_count < pixels_to_process) ? output_pixel_count :
                pixels_to_process;
    output_pixel_count -= pixels_to_process;
#else
   pixels = (output_pixel_count < pixels_per_scanline) ? output_pixel_count :
               pixels_per_scanline;
   output_pixel_count -= pixels;
#endif  /* VIIRS */

   if (pixels == 0)
      output_buffer_full = TRUE;

   if (compression_mode == NN_MODE)
      unmap_nn(sigma, pixels);
   else
      {
      if (bytes_per_pixel == 1)
         for (i = 0; i < pixels; i++)
            *bptr++ = sigma[i];
      else if (bytes_per_pixel == 2)
         {
         if (msb_first)
            for (i = 0; i < pixels; i++)
               {
               *bptr++ = sigma[i] >> 8;
               *bptr++ = sigma[i];
               }
         else
            for (i = 0; i < pixels; i++)
               {
               *bptr++ = sigma[i];
               *bptr++ = sigma[i] >> 8;
               }
         }
      else
         {
         if (msb_first)
            for (i = 0; i < pixels; i++)
               {
               *bptr++ = sigma[i] >> 24;
               *bptr++ = sigma[i] >> 16;
               *bptr++ = sigma[i] >> 8;
               *bptr++ = sigma[i];
               }
         else
            for (i = 0; i < pixels; i++)
               {
               *bptr++ = sigma[i];
               *bptr++ = sigma[i] >> 8;
               *bptr++ = sigma[i] >> 16;
               *bptr++ = sigma[i] >> 24;
               }
         }
      }
}

static void
rice_decode()
{
   int ext2_bit;
   int i;
   int id;
   int k_bits;
   int zero_blocks;
   unsigned *end;
   unsigned *s;
   unsigned *s1;
   unsigned sigma[MAX_PIXELS_PER_SCANLINE];
   unsigned char *b;
   unsigned char byte_buffer[4*INPUT_BUFFER_SIZE];
   int count;
   int extra;
   int n;
   unsigned short *pend;
   unsigned short *p;
   unsigned data_word;
   int data_bits;
   unsigned short input_buffer[INPUT_BUFFER_SIZE+2];
   unsigned short *input_ptr;
   unsigned short *input_end;

#if VIIRS == 1
   unsigned int new_scanline;
   int pixels_remaining;
   int blocks_to_process;
#endif  /* VIIRS */

   input_end = input_buffer;
   input_ptr = input_buffer;

   data_bits = 0;
   data_word = 0;

#if VIIRS == 1

   pixels_per_reference = blocks_per_reference * pixels_per_block;
   if(pixels_per_reference > pixels_per_scanline)
   {
      pixels_per_reference = pixels_per_scanline;
      blocks_per_reference = pixels_per_reference /
                                  pixels_per_block;
   }
   new_scanline = 1;

#endif   /* VIIRS  */

   while (1)
      {

#if VIIRS == 1

         if(new_scanline == 1)
         {
            pixels_remaining = pixels_per_scanline;
         }

         if( pixels_remaining > pixels_per_reference)
         {
            pixels_to_process = pixels_per_reference;
            pixels_remaining -= pixels_per_reference;
            new_scanline = 0;
         }
         else
         {
            pixels_to_process = pixels_remaining;
            pixels_remaining = 0;
            new_scanline = 1;
         }
          
         blocks_to_process = pixels_to_process / pixels_per_block;

#endif     /* VIIRS */

      if (input_ptr + pixels_per_scanline*4 >= input_end)
         {
            {
            n = input_byte_count >= INPUT_BUFFER_SIZE ? INPUT_BUFFER_SIZE : input_byte_count;
            input_byte_count -= n;
            memcpy(byte_buffer, input_byte_data, n);
            input_byte_data += n;
            }

         if (n != 0)
            {
            if (n & 1)
               {
               byte_buffer[n] = 0;
               n++;
               }

            count = input_end - input_ptr;
            memcpy(input_buffer, input_ptr, count*sizeof(short));
            p = input_buffer + count;
   
            pend = input_buffer + (n >> 1) + count;
            b = byte_buffer;
            while (p < pend)
               {
               *p++ = (*b << 8) | *(b+1); 
               b += 2;
               }
   
            *p++ = 0;
            *p++ = 0;
            *p++ = 0; // JMG  10/22/14

            input_end = pend;
            input_ptr = input_buffer;
            }
         else if (data_word == 0 && input_ptr >= input_end)
            break;

         if (data_bits == 0)
            {
            data_word = *input_ptr++ << 16;
            data_word |= *input_ptr++;
            data_bits = 32;
            }
         }

      s = sigma;
      end = s + pixels_per_block;
      if (compression_mode == NN_MODE)
         {
         i = 1;
         s++;
         /*** do first block of scanline since it contains the reference ***/
         if (bytes_per_pixel == 1)
            {
            id = data_word >> 29;
            data_word <<= 3;
            data_bits -= 3;
            }
         else if (bytes_per_pixel == 2)
            {
            id = data_word >> 28;
            data_word <<= 4;
            data_bits -= 4;
            }
         else
            {
            id = data_word >> 27;
            data_word <<= 5;
            data_bits -= 5;
            }

         if (id == ID_LOW)
            {
            ext2_bit = data_word & 0x80000000;
            data_word <<= 1;
            data_bits--;
            }

         if (data_bits <= 16)
            {
            data_word |= *input_ptr++ << (16 - data_bits);
            data_bits += 16;
            }

         k_bits = id - K_FACTOR;
         extra = bits_per_pixel - 16;
         if (extra <= 0)
            {
            *sigma = data_word >> (32 - bits_per_pixel);
            data_word <<= bits_per_pixel;
            data_bits -= bits_per_pixel;
            }
         else
            {
            *sigma = data_word >> 16;
            data_word <<= 16;
            data_bits -= 16;

            data_word |= *input_ptr++ << (16 - data_bits);
            data_bits += 16;

            *sigma <<= extra;
            *sigma |= data_word >> (32 - extra);
            data_word <<= extra;
            data_bits -= extra;
            }

         if (data_bits <= 16)
            {
            data_word |= *input_ptr++ << (16 - data_bits);
            data_bits += 16;
            }

         zero_blocks = 0;
         if (id >= ID_K1)
            {
            if (id == default_id)
               {
               int bits;
               int shift;

               extra = bits_per_pixel - 16;
               if (extra <= 0)
                  {
                  shift = 32 - bits_per_pixel;
                  while (s < end)
                     {
                     bits = data_word >> shift;
                     data_word <<= bits_per_pixel;
                     data_bits -= bits_per_pixel;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }
               
                     *s++ = bits;
                     }
                  }
               else
                  {
                    while (s < end)
                     {
                     bits = data_word >> 16;
                     data_word <<= 16;
                     data_bits -= 16;
                     data_word |= *input_ptr++ << (16 - data_bits);
                     data_bits += 16;

                     bits <<= extra;
                     bits |= data_word >> (32 - extra);
                     data_word <<= extra;
                     data_bits -= extra;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }
               
                     *s++ = bits;
                     }
                  }
               }
            else
               {
               int bits;
               int shift;
               int zero_count;
               int big_zero_count;
            
               s1 = s; 
               while (s < end)
                  {
                  big_zero_count = 0;
                  while ((data_word & 0xff000000) == 0)
                     {
                     big_zero_count += 8;
                     data_word <<= 8;
                     data_bits -= 8;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }
                     }
            
                  zero_count = leading_zeros[data_word >> 24];
                  data_word <<= zero_count+1;
                  data_bits -= zero_count+1;
                  if (data_bits <= 16)
                     {
                     data_word |= *input_ptr++ << (16 - data_bits);
                     data_bits += 16;
                     }
            
                  *s++ = zero_count + big_zero_count;
                  }
            
               s = s1;
               shift = 32 - k_bits;
               extra = k_bits - 16;
               if (extra <= 0)
                  {
                  while (s < end)
                     {
                     bits = data_word >> shift;
                     data_word <<= k_bits;
                     data_bits -= k_bits;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }
            
                     *s = (*s << k_bits) | bits;
                     s++;
                     }
                  }
               else
                  {
                  while (s < end)
                     {
                     bits = data_word >> 16;
                     data_word <<= 16;
                     data_bits -= 16;
                     data_word |= *input_ptr++ << (16 - data_bits);
                     data_bits += 16;

                     bits <<= extra;
                     bits |= data_word >> (32 - extra);

                     data_word <<= extra;
                     data_bits -= extra;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }
            
                     *s = (*s << k_bits) | bits;
                     s++;
                     }
                  }
               }
            }
         else if (id == ID_FS)
            {
            int big_zero_count;
            int zero_count;
         
            while (s < end)
               {
               big_zero_count = 0;
               while ((data_word & 0xff000000) == 0)
                  {
                  big_zero_count += 8;
                  data_word <<= 8;
                  data_bits -= 8;
                  if (data_bits <= 16)
                     {
                     data_word |= *input_ptr++ << (16 - data_bits);
                     data_bits += 16;
                     }
                  }
         
               zero_count = leading_zeros[data_word >> 24];
               data_word <<= zero_count+1;
               data_bits -= zero_count+1;
               if (data_bits <= 16)
                  {
                  data_word |= *input_ptr++ << (16 - data_bits);
                  data_bits += 16;
                  }
         
               *s++ = zero_count + big_zero_count;
               }
            }
         else
            {
            if (ext2_bit)
               {
               int big_zero_count;
               int m;
               int zero_count;
               unsigned *t;
               unsigned *tend;
               unsigned temp[MAX_PIXELS_PER_BLOCK];
            
               /*** Note: The value, m, must be range checked, to avoid a segmentation violation ***/
               /*** caused by the data being decoded and encoded with a different number of ***/
               /*** pixels_per_scanline. ***/ 
            
               /*** Read EXT2 FS values ***/
               t = temp;
               tend = temp + pixels_per_block/2;
               while (t < tend)
                  {
                  big_zero_count = 0;
                  while ((data_word & 0xff000000) == 0)
                     {
                     big_zero_count += 8;
                     data_word <<= 8;
                     data_bits -= 8;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }
                     }
            
                  zero_count = leading_zeros[data_word >> 24];
                  data_word <<= zero_count+1;
                  data_bits -= zero_count+1;
                  if (data_bits <= 16)
                     {
                     data_word |= *input_ptr++ << (16 - data_bits);
                     data_bits += 16;
                     }
            
                  *t++ = zero_count + big_zero_count;
                  }
            
               t = temp;
               m = *t++;
               if (m >= MAX_EXT2_SUM)
                  m = 0;
            
               *s++ = ext2_array2[m];
               while (s < end)
                  {
                  m = *t++;
                  if (m >= MAX_EXT2_SUM)
                     m = 0;
            
                  *s++ = ext2_array1[m];
                  *s++ = ext2_array2[m];
                  }
               }
            else
               {
               int bits;
               int big_zero_count;
               int zero_count;
            
               big_zero_count = 0;
               while ((data_word & 0xff000000) == 0)
                  {
                  big_zero_count += 8;
                  data_word <<= 8;
                  data_bits -= 8;
                  if (data_bits <= 16)
                     {
                     data_word |= *input_ptr++ << (16 - data_bits);
                     data_bits += 16;
                     }
                  }
            
               zero_count = leading_zeros[data_word >> 24];
               data_word <<= zero_count+1;
               data_bits -= zero_count+1;
               if (data_bits <= 16)
                  {
                  data_word |= *input_ptr++ << (16 - data_bits);
                  data_bits += 16;
                  }
            
               bits = zero_count + big_zero_count + 1;
            
               if (bits < 5)
                  zero_blocks = bits;
               else if (bits == 5)
                  zero_blocks = blocks_per_scanline > MAX_ZERO_BLOCKS ? MAX_ZERO_BLOCKS : blocks_per_scanline;
               else 
                  zero_blocks = bits - 1;
            
               i += zero_blocks-1;
               if (i > blocks_per_scanline)
                  {
                  error("Decoded more blocks than in scanline.  Check -s value.\n");
                  return;
                  }

               end += (zero_blocks-1) * pixels_per_block;
               memset(s, 0, (end-s)*sizeof(int));
               }
            }

         s = end;
         end += pixels_per_block;
         i++;
         }
      else
         i = 1;
         

      for (; i <= blocks_to_process; i++)
         {
         /*** do rest of blocks on the scanline since they contains no references ***/
         if (bytes_per_pixel == 1)
            {
            id = data_word >> 29;
            data_word <<= 3;
            data_bits -= 3;
            }
         else if (bytes_per_pixel == 2)
            {
            id = data_word >> 28;
            data_word <<= 4;
            data_bits -= 4;
            }
         else
            {
            id = data_word >> 27;
            data_word <<= 5;
            data_bits -= 5;
            }

         if (id == ID_LOW)
            {
            ext2_bit = data_word & 0x80000000;
            data_word <<= 1;
            data_bits--;
            }

         if (data_bits <= 16)
            {
            data_word |= *input_ptr++ << (16 - data_bits);
            data_bits += 16;
            }

         k_bits = id - K_FACTOR;
         zero_blocks = 0;
         if (id >= ID_K1)
            {
            if (id == default_id)
               {
               int bits;
               int shift;

               extra = bits_per_pixel - 16;
               if (extra <= 0)
                  {
                  shift = 32 - bits_per_pixel;
                  while (s < end)
                     {
                     bits = data_word >> shift;
                     data_word <<= bits_per_pixel;
                     data_bits -= bits_per_pixel;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }
               
		                     *s++ = bits;
                     }
                  }
               else
                  {
                  while (s < end)
                     {
                     bits = data_word >> 16;
                     data_word <<= 16;
                     data_bits -= 16;
                     data_word |= *input_ptr++ << (16 - data_bits);
                     data_bits += 16;

                     bits <<= extra;
                     bits |= data_word >> (32 - extra);
                     data_word <<= extra;
                     data_bits -= extra;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }
               
                     *s++ = bits;
                     }
                  }
               }
            else
               {
               int shift;
               int zero_count;
               int big_zero_count;

               s1 = s;

               while (s < end)
                  {
                  if ((data_word >> 30) == 3)
                     {
                     data_word <<= 2;
                     data_bits -= 2;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }

                     *s++ = 0;
                     *s++ = 0;
                     }
                  else if ((data_word >> 29) == 3)
                     {
                     data_word <<= 3;
                     data_bits -= 3;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }

                     *s++ = 1;
                     *s++ = 0;
                     }
                  else if ((data_word >> 29) == 5)
                     {
                     data_word <<= 3;
                     data_bits -= 3;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }

                     *s++ = 0;
                     *s++ = 1;
                     }
                  else if ((data_word >> 28) == 5)
                     {
                     data_word <<= 4;
                     data_bits -= 4;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }

                     *s++ = 1;
                     *s++ = 1;
                     }
                  else
                     {
                     big_zero_count = 0;
                     while ((data_word & 0xff000000) == 0)
                        {
                        big_zero_count += 8;
                        data_word <<= 8;
                        data_bits -= 8;
                        if (data_bits <= 16)
                           {
                           data_word |= *input_ptr++ << (16 - data_bits);
                           data_bits += 16;
                           }
                        }

                     zero_count = leading_zeros[data_word >> 24];
                     data_word <<= zero_count+1;
                     data_bits -= zero_count+1;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }

                     *s++ = zero_count + big_zero_count;

                     big_zero_count = 0;
                     while ((data_word & 0xff000000) == 0)
                        {
                        big_zero_count += 8;
                        data_word <<= 8;
                        data_bits -= 8;
                        if (data_bits <= 16)
                           {
                           data_word |= *input_ptr++ << (16 - data_bits);
                           data_bits += 16;
                           }
                        }

                     zero_count = leading_zeros[data_word >> 24];
                     data_word <<= zero_count+1;
                     data_bits -= zero_count+1;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }

                     *s++ = zero_count + big_zero_count;
                     }
                  }

               s = s1;
               if ((pixels_per_block & 7) == 0 && k_bits <= 16)
                  switch (k_bits)
                     {
                     case 1:
                        while (s < end)
                           {
                           *(s+0) = (*(s+0) << 1) | (data_word >> 31);
                           *(s+1) = (*(s+1) << 1) | (data_word >> 30) & 1;
                           *(s+2) = (*(s+2) << 1) | (data_word >> 29) & 1;
                           *(s+3) = (*(s+3) << 1) | (data_word >> 28) & 1;
                           *(s+4) = (*(s+4) << 1) | (data_word >> 27) & 1;
                           *(s+5) = (*(s+5) << 1) | (data_word >> 26) & 1;
                           *(s+6) = (*(s+6) << 1) | (data_word >> 25) & 1;
                           *(s+7) = (*(s+7) << 1) | (data_word >> 24) & 1;
                           s += 8;
                           data_word <<= 8;
                           data_bits -= 8;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }
                           }
                        break;
   
                     case 2:
                        while (s < end)
                           {
                           *(s+0) = (*(s+0) << 2) | (data_word >> 30) & 3;
                           *(s+1) = (*(s+1) << 2) | (data_word >> 28) & 3;
                           *(s+2) = (*(s+2) << 2) | (data_word >> 26) & 3;
                           *(s+3) = (*(s+3) << 2) | (data_word >> 24) & 3;
                           *(s+4) = (*(s+4) << 2) | (data_word >> 22) & 3;
                           *(s+5) = (*(s+5) << 2) | (data_word >> 20) & 3;
                           *(s+6) = (*(s+6) << 2) | (data_word >> 18) & 3;
                           *(s+7) = (*(s+7) << 2) | (data_word >> 16) & 3;
                           s += 8;
                           data_word <<= 16;
                           data_bits -= 16;
                           data_word |= *input_ptr++ << (16 - data_bits);
                           data_bits += 16;
                           }
                        break;

                     case 3:
                        while (s < end)
                           {
                           *(s+0) = (*(s+0) << 3) | (data_word >> 29) & 7;
                           *(s+1) = (*(s+1) << 3) | (data_word >> 26) & 7;
                           *(s+2) = (*(s+2) << 3) | (data_word >> 23) & 7;
                           *(s+3) = (*(s+3) << 3) | (data_word >> 20) & 7;
                           *(s+4) = (*(s+4) << 3) | (data_word >> 17) & 7;
                           s += 5;
                           data_word <<= 15;
                           data_bits -= 15;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }

                           *(s+0) = (*(s+0) << 3) | (data_word >> 29) & 7;
                           *(s+1) = (*(s+1) << 3) | (data_word >> 26) & 7;
                           *(s+2) = (*(s+2) << 3) | (data_word >> 23) & 7;
                           s += 3;
                           data_word <<= 9;
                           data_bits -= 9;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }
                           }
                        break;

                     case 4:
                        while (s < end)
                           {
                           *(s+0) = (*(s+0) << 4) | (data_word >> 28) & 0xf;
                           *(s+1) = (*(s+1) << 4) | (data_word >> 24) & 0xf;
                           *(s+2) = (*(s+2) << 4) | (data_word >> 20) & 0xf;
                           *(s+3) = (*(s+3) << 4) | (data_word >> 16) & 0xf;
                           s += 4;
                           data_word <<= 16;
                           data_bits -= 16;
                           data_word |= *input_ptr++ << (16 - data_bits);
                           data_bits += 16;

                           *(s+0) = (*(s+0) << 4) | (data_word >> 28) & 0xf;
                           *(s+1) = (*(s+1) << 4) | (data_word >> 24) & 0xf;
                           *(s+2) = (*(s+2) << 4) | (data_word >> 20) & 0xf;
                           *(s+3) = (*(s+3) << 4) | (data_word >> 16) & 0xf;
                           s += 4;
                           data_word <<= 16;
                           data_bits -= 16;
                           data_word |= *input_ptr++ << (16 - data_bits);
                           data_bits += 16;
                           }
                        break;

                     case 5:
                        while (s < end)
                           {
                           *(s+0) = (*(s+0) << 5) | (data_word >> 27) & 0x1f;
                           *(s+1) = (*(s+1) << 5) | (data_word >> 22) & 0x1f;
                           *(s+2) = (*(s+2) << 5) | (data_word >> 17) & 0x1f;
                           s += 3;
                           data_word <<= 15;
                           data_bits -= 15;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }

                           *(s+0) = (*(s+0) << 5) | (data_word >> 27) & 0x1f;
                           *(s+1) = (*(s+1) << 5) | (data_word >> 22) & 0x1f;
                           *(s+2) = (*(s+2) << 5) | (data_word >> 17) & 0x1f;
                           s += 3;
                           data_word <<= 15;
                           data_bits -= 15;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }

                           *(s+0) = (*(s+0) << 5) | (data_word >> 27) & 0x1f;
                           *(s+1) = (*(s+1) << 5) | (data_word >> 22) & 0x1f;
                           s += 2;
                           data_word <<= 10;
                           data_bits -= 10;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }
                           }
                        break;

                     case 6:
                        while (s < end)
                           {
                           *(s+0) = (*(s+0) << 6) | (data_word >> 26) & 0x3f;
                           *(s+1) = (*(s+1) << 6) | (data_word >> 20) & 0x3f;
                           s += 2;
                           data_word <<= 12;
                           data_bits -= 12;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }

                           *(s+0) = (*(s+0) << 6) | (data_word >> 26) & 0x3f;
                           *(s+1) = (*(s+1) << 6) | (data_word >> 20) & 0x3f;
                           s += 2;
                           data_word <<= 12;
                           data_bits -= 12;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }

                           *(s+0) = (*(s+0) << 6) | (data_word >> 26) & 0x3f;
                           *(s+1) = (*(s+1) << 6) | (data_word >> 20) & 0x3f;
                           s += 2;
                           data_word <<= 12;
                           data_bits -= 12;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }

                           *(s+0) = (*(s+0) << 6) | (data_word >> 26) & 0x3f;
                           *(s+1) = (*(s+1) << 6) | (data_word >> 20) & 0x3f;
                           s += 2;
                           data_word <<= 12;
                           data_bits -= 12;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }
                           }
                        break;

                     case 7:
                        while (s < end)
                           {
                           *(s+0) = (*(s+0) << 7) | (data_word >> 25) & 0x7f;
                           *(s+1) = (*(s+1) << 7) | (data_word >> 18) & 0x7f;
                           s += 2;
                           data_word <<= 14;
                           data_bits -= 14;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }

                           *(s+0) = (*(s+0) << 7) | (data_word >> 25) & 0x7f;
                           *(s+1) = (*(s+1) << 7) | (data_word >> 18) & 0x7f;
                           s += 2;
                           data_word <<= 14;
                           data_bits -= 14;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }

                           *(s+0) = (*(s+0) << 7) | (data_word >> 25) & 0x7f;
                           *(s+1) = (*(s+1) << 7) | (data_word >> 18) & 0x7f;
                           s += 2;
                           data_word <<= 14;
                           data_bits -= 14;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }

                           *(s+0) = (*(s+0) << 7) | (data_word >> 25) & 0x7f;
                           *(s+1) = (*(s+1) << 7) | (data_word >> 18) & 0x7f;
                           s += 2;
                           data_word <<= 14;
                           data_bits -= 14;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }
                           }
                        break;

                     case 8:
                        while (s < end)
                           {
                           *(s+0) = (*(s+0) << 8) | (data_word >> 24) & 0xff;
                           *(s+1) = (*(s+1) << 8) | (data_word >> 16) & 0xff;
                           s += 2;
                           data_word <<= 16;
                           data_bits -= 16;
                           data_word |= *input_ptr++ << (16 - data_bits);
                           data_bits += 16;

                           *(s+0) = (*(s+0) << 8) | (data_word >> 24) & 0xff;
                           *(s+1) = (*(s+1) << 8) | (data_word >> 16) & 0xff;
                           s += 2;
                           data_word <<= 16;
                           data_bits -= 16;
                           data_word |= *input_ptr++ << (16 - data_bits);
                           data_bits += 16;

                           *(s+0) = (*(s+0) << 8) | (data_word >> 24) & 0xff;
                           *(s+1) = (*(s+1) << 8) | (data_word >> 16) & 0xff;
                           s += 2;
                           data_word <<= 16;
                           data_bits -= 16;
                           data_word |= *input_ptr++ << (16 - data_bits);
                           data_bits += 16;

                           *(s+0) = (*(s+0) << 8) | (data_word >> 24) & 0xff;
                           *(s+1) = (*(s+1) << 8) | (data_word >> 16) & 0xff;
                           s += 2;
                           data_word <<= 16;
                           data_bits -= 16;
                           data_word |= *input_ptr++ << (16 - data_bits);
                           data_bits += 16;
                           }
                        break;

                     default:
                        shift = 32 - k_bits;
                        while (s < end)
                           {
                           *s = (*s << k_bits) | (data_word >> shift);
                           s++;
                           data_word <<= k_bits;
                           data_bits -= k_bits;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }
   
                           *s = (*s << k_bits) | (data_word >> shift);
                           s++;
                           data_word <<= k_bits;
                           data_bits -= k_bits;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }
                           }
                        break;
                     }
                  else
                     {
                     extra = k_bits - 16;
                     if (extra <= 0)
                        {
                        shift = 32 - k_bits;
                        while (s < end)
                           {
                           *s = (*s << k_bits) | (data_word >> shift);
                           s++;
                           data_word <<= k_bits;
                           data_bits -= k_bits;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }

                           *s = (*s << k_bits) | (data_word >> shift);
                           s++;
                           data_word <<= k_bits;
                           data_bits -= k_bits;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }
                           }
                        }
                     else
                        {
                        while (s < end)
                           {
                           *s = (*s << 16) | (data_word >> 16);
                           data_word <<= 16;
                           data_bits -= 16;
                           data_word |= *input_ptr++ << (16 - data_bits);
                           data_bits += 16;

                           *s = (*s << extra) | (data_word >> (32 - extra));
                           s++;
                           data_word <<= extra;
                           data_bits -= extra;
                           if (data_bits <= 16)
                              {
                              data_word |= *input_ptr++ << (16 - data_bits);
                              data_bits += 16;
                              }
                           }
                        }
                     }
               }
            }   
         else if (id == ID_FS)
            {
            int big_zero_count;
            int zero_count;
         
            while (s < end)
               {
               big_zero_count = 0;
               while ((data_word & 0xff000000) == 0)
                  {
                  big_zero_count += 8;
                  data_word <<= 8;
                  data_bits -= 8;
                  if (data_bits <= 16)
                     {
                     data_word |= *input_ptr++ << (16 - data_bits);
                     data_bits += 16;
                     }
                  }
         
               zero_count = leading_zeros[data_word >> 24];
               data_word <<= zero_count+1;
               data_bits -= zero_count+1;
               if (data_bits <= 16)
                  {
                  data_word |= *input_ptr++ << (16 - data_bits);
                  data_bits += 16;
                  }
         
               *s++ = zero_count + big_zero_count;
               }
            }
         else
            {
            if (ext2_bit)
               {
               int big_zero_count;
               int m;
               int zero_count;
               unsigned *t;
               unsigned *tend;
               unsigned temp[MAX_PIXELS_PER_BLOCK];
            
               /*** Note: The value, m, must be range checked, to avoid a segmentation violation ***/
               /*** caused by the data being decoded and encoded with a different number of ***/
               /*** pixels_per_scanline. ***/ 
            
               /*** Read EXT2 FS values ***/
               t = temp;
               tend = temp + pixels_per_block/2;
               while (t < tend)
                  {
                  big_zero_count = 0;
                  while ((data_word & 0xff000000) == 0)
                     {
                     big_zero_count += 8;
                     data_word <<= 8;
                     data_bits -= 8;
                     if (data_bits <= 16)
                        {
                        data_word |= *input_ptr++ << (16 - data_bits);
                        data_bits += 16;
                        }
                     }
            
                  zero_count = leading_zeros[data_word >> 24];
                  data_word <<= zero_count+1;
                  data_bits -= zero_count+1;
                  if (data_bits <= 16)
                     {
                     data_word |= *input_ptr++ << (16 - data_bits);
                     data_bits += 16;
                     }
            
                  *t++ = zero_count + big_zero_count;
                  }
            
		               t = temp;
               while (s < end)
                  {
                  m = *t++;
                  if (m >= MAX_EXT2_SUM)
                     m = 0;
            
                  *s++ = ext2_array1[m];
                  *s++ = ext2_array2[m];
                  }
               }
            else
               {
               int bits;
               int big_zero_count;
               int zero_count;
            
               big_zero_count = 0;
               while ((data_word & 0xff000000) == 0)
                  {
                  big_zero_count += 8;
                  data_word <<= 8;
                  data_bits -= 8;
                  if (data_bits <= 16)
                     {
                     data_word |= *input_ptr++ << (16 - data_bits);
                     data_bits += 16;
                     }
                  }
            
               zero_count = leading_zeros[data_word >> 24];
               data_word <<= zero_count+1;
               data_bits -= zero_count+1;
               if (data_bits <= 16)
                  {
                  data_word |= *input_ptr++ << (16 - data_bits);
                  data_bits += 16;
                  }
            
               bits = zero_count + big_zero_count + 1;
            
               if (bits < 5)
                  zero_blocks = bits;
               else if (bits == 5)
                  {
                  /*** Zero blocks till the end of the scanline or to the nearest scanline block that is ***/
                  /*** a multiple of MAX_ZERO_BLOCKS. ***/
                  /*** Hardware is limited to 64 zero blocks and 64 blocks per scanline ***/
                  zero_blocks = MAX_ZERO_BLOCKS - ((i-1) & (MAX_ZERO_BLOCKS-1));
                  if (blocks_per_scanline - (i-1) < zero_blocks)
                     zero_blocks = blocks_per_scanline - (i-1);
                  }
               else 
                  zero_blocks = bits - 1;
            
               i += zero_blocks-1;
               if (i > blocks_per_scanline)
                  {
                  error("Decoded more blocks than in scanline.  Check -s value.\n");
                  return;
                  }

               end += (zero_blocks-1) * pixels_per_block;
               memset(s, 0, (end-s)*sizeof(int));
               }
            }

         s = end;
         end += pixels_per_block;
         }

      output_decoded_data(sigma);
      }
}

/****************************************************************
* Must be external to be linked to HDF code.                    *
* This function calls the proper function to encode byte, word, *
* long, float, and double data types.                           *
* Valid bits_per_pixel are: 1-24,32,64.                         *
****************************************************************/
/* changed declaration to ANSI C */
long
uncompress_memory(int new_options_mask,
                  int new_bits_per_pixel,
                  int new_pixels_per_block,
                  int new_pixels_per_scanline,
                  const char* in,
                  long in_bytes,
                  void* out,
                  long out_pixels)
{
   static char *interleave_array;
   long bytes_written;
   long out_bytes;

   /*** reset error_count; if non zero an error occured during decompression ***/
   error_count = 0;
   warning_count = 0;

   compression_mode = (new_options_mask & NN_OPTION_MASK) ? NN_MODE : EC_MODE;
   msb_first = (new_options_mask & MSB_OPTION_MASK) != 0;
   raw_mode  = (new_options_mask & RAW_OPTION_MASK) != 0;

#if VIIRS == 1
   blocks_per_reference = (int)VIIRS_BLOCKS_PER_REFERENCE;
#endif  /* VIIRS */

   bits_per_pixel = new_bits_per_pixel;
   pixels_per_block = new_pixels_per_block;
   pixels_per_scanline = new_pixels_per_scanline;

   input_byte_data = (unsigned char *) in;
   input_byte_count = in_bytes;

   if (new_bits_per_pixel == 32 || new_bits_per_pixel == 64)
      {
      if (interleave_array)
         free(interleave_array);

      out_bytes = out_pixels * (new_bits_per_pixel >> 3);
      interleave_array = (char *) malloc(out_bytes);
      if (interleave_array == 0)
         {
         error("Out of Memory.\n");
         return MEMORY_ERROR;
         }

      bits_per_pixel = 8;
      }

   decode_initialize();

   if (new_bits_per_pixel == 32 || new_bits_per_pixel == 64)
      {
      bptr = interleave_array;
      output_pixel_count = out_pixels * (new_bits_per_pixel >> 3);
      }
   else
      {
/* added (char*) type cast */
      bptr = (char*)out;
      output_pixel_count = out_pixels;
      }

   rice_decode();
   if (error_count)
      return PARAM_ERROR;

   if (new_bits_per_pixel == 32 || new_bits_per_pixel == 64)
      {
      bytes_written = bptr - interleave_array; 

/* added type cast to argument out */
      deinterleave(interleave_array, bytes_written, new_bits_per_pixel,
                   (char*)out);

      }
   else
      bytes_written = bptr - (char *) out; 

   return bytes_written;
}


/* change declaration to ANSI C */
int
check_params(int bits_per_pixel,
             int pixels_per_block,
             int pixels_per_scanline,
             long image_pixels,
             char** msg)
{
   if (bits_per_pixel >= 1 && bits_per_pixel <= 24)
      ;
   else if (bits_per_pixel == 32 || bits_per_pixel == 64)
      ;
   else
      {
      *msg = "bits per pixel must be in range 1..24,32,64";
      return 0;
      }

   if (pixels_per_block > MAX_PIXELS_PER_BLOCK)   
      {
      *msg = "maximum pixels per block exceeded";
      return 0;
      }

   if (pixels_per_block & 1)   
      {
      *msg = "pixels per block must be even";
      return 0;
      }

   if (pixels_per_block > pixels_per_scanline)
      {
      *msg = "pixels per block > pixels per scanline";
      return 0;
      }

   if (pixels_per_scanline > MAX_PIXELS_PER_SCANLINE)
      {
      *msg = "maximum pixels per scanline exceeded";
      return 0;
      }

   if (image_pixels < pixels_per_scanline)
      {
      *msg = "image pixels less than pixels per scanline";
      return 0;
      }

   return 1;
}

#if HDF

/*********************************************************************/
/*** Make sure that the defines in ricehdf.h match those in rice.h ***/
/*********************************************************************/

#if ALLOW_K13_OPTION_MASK != SZ_ALLOW_K13_OPTION_MASK
#error "define ALLOW_K13_OPTION_MASK != SZ_ALLOW_K13_OPTION_MASK"
#endif

#if CHIP_OPTION_MASK != SZ_CHIP_OPTION_MASK
#error "define CHIP_OPTION_MASK != SZ_CHIP_OPTION_MASK"
#endif

#if EC_OPTION_MASK != SZ_EC_OPTION_MASK
#error "define EC_OPTION_MASK != SZ_EC_OPTION_MASK"
#endif

#if LSB_OPTION_MASK != SZ_LSB_OPTION_MASK
#error "define LSB_OPTION_MASK != SZ_LSB_OPTION_MASK"
#endif

#if MSB_OPTION_MASK != SZ_MSB_OPTION_MASK
#error "define MSB_OPTION_MASK != SZ_MSB_OPTION_MASK"
#endif

#if NN_OPTION_MASK != SZ_NN_OPTION_MASK 
#error "define NN_OPTION_MASK  != SZ_NN_OPTION_MASK "
#endif

#if RAW_OPTION_MASK != SZ_RAW_OPTION_MASK 
#error "define NN_RAW_OPTION_MASK != SZ_RAW_OPTION_MASK "
#endif

#if MEMORY_ERROR != SZ_MEM_ERROR 
#error "define MEMORY_ERROR != SZ_MEM_ERROR"
#endif

#if PARAM_ERROR != SZ_PARAM_ERROR 
#error "define PARAM_ERROR != SZ_PARAM_ERROR"
#endif

#if NO_ENCODER_ERROR != SZ_NO_ENCODER_ERROR 
#error "define NO_ENCODER_ERROR != SZ_NO_ENCODER_ERROR"
#endif

#if MAX_BLOCKS_PER_SCANLINE != SZ_MAX_BLOCKS_PER_SCANLINE 
#error "define MAX_BLOCKS_PER_SCANLINE != SZ_MAX_BLOCKS_PER_SCANLINE"
#endif

#if MAX_PIXELS_PER_BLOCK != SZ_MAX_PIXELS_PER_BLOCK 
#error "define MAX_PIXELS_PER_BLOCK != SZ_MAX_PIXELS_PER_BLOCK"
#endif

#if MAX_PIXELS_PER_SCANLINE != SZ_MAX_PIXELS_PER_SCANLINE 
#error "define MAX_PIXELS_PER_SCANLINE != SZ_MAX_PIXELS_PER_SCANLINE"
#endif

#endif /*** HDF ***/

