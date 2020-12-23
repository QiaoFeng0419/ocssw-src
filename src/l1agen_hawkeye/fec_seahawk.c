/* This function wraps around the fixed 8-bit decoder, performing the
 * basis transformations necessary to meet the CCSDS standard
 *
 * Copyright 2002, Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */
#include <stdlib.h>
#include <memory.h>
#include <limits.h>

#include "ccsds.h"
#include "fec.h"
#ifdef DEBUG
#include <stdio.h>
#endif

#include <string.h>

/* Stuff specific to the CCSDS (255,223) RS codec
 * (255,223) code over GF(256). Note: the conventional basis is still
 * used; the dual-basis mappings are performed in [en|de]code_rs_ccsds.c
 *
 * Copyright 2003 Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */
typedef unsigned char data_t;

static inline int mod255(int x){
  while (x >= 255) {
    x -= 255;
    x = (x >> 8) + (x & 255);
  }
  return x;
}
#define MODNN(x) mod255(x)

data_t CCSDS_alpha_to[];
data_t CCSDS_index_of[];
data_t CCSDS_poly[];

#define MM 8
#define NN 255
#define ALPHA_TO CCSDS_alpha_to
#define INDEX_OF CCSDS_index_of
#define GENPOLY CCSDS_poly
#define NROOTS 32
#define FCR 112
#define PRIM 11
#define IPRIM 116
#define PAD pad


int decode_rs_ccsds(data_t *data,int *eras_pos,int no_eras,int pad){
  int i,r;
  data_t cdata[NN];

  /* Convert data from dual basis to conventional */
  for(i=0;i<NN-pad;i++)
    cdata[i] = Tal1tab[data[i]];

  r = decode_rs_8(cdata,eras_pos,no_eras,pad);

  if(r > 0){
    /* Convert from conventional to dual basis */
    for(i=0;i<NN-pad;i++)
      data[i] = Taltab[cdata[i]];
  }
  return r;
}
/* General purpose Reed-Solomon decoder for 8-bit symbols or less
 * Copyright 2003 Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */


int decode_rs_8(data_t *data, int *eras_pos, int no_eras, int pad){
  int retval;
 
  if(pad < 0 || pad > 222){
    return -1;
  }

#include "decode_rs.h"
  
  return retval;
}
data_t CCSDS_alpha_to[] = {
0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x87,0x89,0x95,0xad,0xdd,0x3d,0x7a,0xf4,
0x6f,0xde,0x3b,0x76,0xec,0x5f,0xbe,0xfb,0x71,0xe2,0x43,0x86,0x8b,0x91,0xa5,0xcd,
0x1d,0x3a,0x74,0xe8,0x57,0xae,0xdb,0x31,0x62,0xc4,0x0f,0x1e,0x3c,0x78,0xf0,0x67,
0xce,0x1b,0x36,0x6c,0xd8,0x37,0x6e,0xdc,0x3f,0x7e,0xfc,0x7f,0xfe,0x7b,0xf6,0x6b,
0xd6,0x2b,0x56,0xac,0xdf,0x39,0x72,0xe4,0x4f,0x9e,0xbb,0xf1,0x65,0xca,0x13,0x26,
0x4c,0x98,0xb7,0xe9,0x55,0xaa,0xd3,0x21,0x42,0x84,0x8f,0x99,0xb5,0xed,0x5d,0xba,
0xf3,0x61,0xc2,0x03,0x06,0x0c,0x18,0x30,0x60,0xc0,0x07,0x0e,0x1c,0x38,0x70,0xe0,
0x47,0x8e,0x9b,0xb1,0xe5,0x4d,0x9a,0xb3,0xe1,0x45,0x8a,0x93,0xa1,0xc5,0x0d,0x1a,
0x34,0x68,0xd0,0x27,0x4e,0x9c,0xbf,0xf9,0x75,0xea,0x53,0xa6,0xcb,0x11,0x22,0x44,
0x88,0x97,0xa9,0xd5,0x2d,0x5a,0xb4,0xef,0x59,0xb2,0xe3,0x41,0x82,0x83,0x81,0x85,
0x8d,0x9d,0xbd,0xfd,0x7d,0xfa,0x73,0xe6,0x4b,0x96,0xab,0xd1,0x25,0x4a,0x94,0xaf,
0xd9,0x35,0x6a,0xd4,0x2f,0x5e,0xbc,0xff,0x79,0xf2,0x63,0xc6,0x0b,0x16,0x2c,0x58,
0xb0,0xe7,0x49,0x92,0xa3,0xc1,0x05,0x0a,0x14,0x28,0x50,0xa0,0xc7,0x09,0x12,0x24,
0x48,0x90,0xa7,0xc9,0x15,0x2a,0x54,0xa8,0xd7,0x29,0x52,0xa4,0xcf,0x19,0x32,0x64,
0xc8,0x17,0x2e,0x5c,0xb8,0xf7,0x69,0xd2,0x23,0x46,0x8c,0x9f,0xb9,0xf5,0x6d,0xda,
0x33,0x66,0xcc,0x1f,0x3e,0x7c,0xf8,0x77,0xee,0x5b,0xb6,0xeb,0x51,0xa2,0xc3,0x00,
};

data_t CCSDS_index_of[] = {
255,  0,  1, 99,  2,198,100,106,  3,205,199,188,101,126,107, 42,
  4,141,206, 78,200,212,189,225,102,221,127, 49,108, 32, 43,243,
  5, 87,142,232,207,172, 79,131,201,217,213, 65,190,148,226,180,
103, 39,222,240,128,177, 50, 53,109, 69, 33, 18, 44, 13,244, 56,
  6,155, 88, 26,143,121,233,112,208,194,173,168, 80,117,132, 72,
202,252,218,138,214, 84, 66, 36,191,152,149,249,227, 94,181, 21,
104, 97, 40,186,223, 76,241, 47,129,230,178, 63, 51,238, 54, 16,
110, 24, 70,166, 34,136, 19,247, 45,184, 14, 61,245,164, 57, 59,
  7,158,156,157, 89,159, 27,  8,144,  9,122, 28,234,160,113, 90,
209, 29,195,123,174, 10,169,145, 81, 91,118,114,133,161, 73,235,
203,124,253,196,219, 30,139,210,215,146, 85,170, 67, 11, 37,175,
192,115,153,119,150, 92,250, 82,228,236, 95, 74,182,162, 22,134,
105,197, 98,254, 41,125,187,204,224,211, 77,140,242, 31, 48,220,
130,171,231, 86,179,147, 64,216, 52,176,239, 38, 55, 12, 17, 68,
111,120, 25,154, 71,116,167,193, 35, 83,137,251, 20, 93,248,151,
 46, 75,185, 96, 15,237, 62,229,246,135,165, 23, 58,163, 60,183,
};

data_t CCSDS_poly[] = {
  0,249, 59, 66,  4, 43,126,251, 97, 30,  3,213, 50, 66,170,  5,
 24,  5,170, 66, 50,213,  3, 30, 97,251,126, 43,  4, 66, 59,249,
  0,
};
unsigned char Taltab[] = {

0x00,0x7b,0xaf,0xd4,0x99,0xe2,0x36,0x4d,0xfa,0x81,0x55,0x2e,0x63,0x18,0xcc,0xb7,
0x86,0xfd,0x29,0x52,0x1f,0x64,0xb0,0xcb,0x7c,0x07,0xd3,0xa8,0xe5,0x9e,0x4a,0x31,
0xec,0x97,0x43,0x38,0x75,0x0e,0xda,0xa1,0x16,0x6d,0xb9,0xc2,0x8f,0xf4,0x20,0x5b,
0x6a,0x11,0xc5,0xbe,0xf3,0x88,0x5c,0x27,0x90,0xeb,0x3f,0x44,0x09,0x72,0xa6,0xdd,
0xef,0x94,0x40,0x3b,0x76,0x0d,0xd9,0xa2,0x15,0x6e,0xba,0xc1,0x8c,0xf7,0x23,0x58,
0x69,0x12,0xc6,0xbd,0xf0,0x8b,0x5f,0x24,0x93,0xe8,0x3c,0x47,0x0a,0x71,0xa5,0xde,
0x03,0x78,0xac,0xd7,0x9a,0xe1,0x35,0x4e,0xf9,0x82,0x56,0x2d,0x60,0x1b,0xcf,0xb4,
0x85,0xfe,0x2a,0x51,0x1c,0x67,0xb3,0xc8,0x7f,0x04,0xd0,0xab,0xe6,0x9d,0x49,0x32,
0x8d,0xf6,0x22,0x59,0x14,0x6f,0xbb,0xc0,0x77,0x0c,0xd8,0xa3,0xee,0x95,0x41,0x3a,
0x0b,0x70,0xa4,0xdf,0x92,0xe9,0x3d,0x46,0xf1,0x8a,0x5e,0x25,0x68,0x13,0xc7,0xbc,
0x61,0x1a,0xce,0xb5,0xf8,0x83,0x57,0x2c,0x9b,0xe0,0x34,0x4f,0x02,0x79,0xad,0xd6,
0xe7,0x9c,0x48,0x33,0x7e,0x05,0xd1,0xaa,0x1d,0x66,0xb2,0xc9,0x84,0xff,0x2b,0x50,
0x62,0x19,0xcd,0xb6,0xfb,0x80,0x54,0x2f,0x98,0xe3,0x37,0x4c,0x01,0x7a,0xae,0xd5,
0xe4,0x9f,0x4b,0x30,0x7d,0x06,0xd2,0xa9,0x1e,0x65,0xb1,0xca,0x87,0xfc,0x28,0x53,
0x8e,0xf5,0x21,0x5a,0x17,0x6c,0xb8,0xc3,0x74,0x0f,0xdb,0xa0,0xed,0x96,0x42,0x39,
0x08,0x73,0xa7,0xdc,0x91,0xea,0x3e,0x45,0xf2,0x89,0x5d,0x26,0x6b,0x10,0xc4,0xbf,
};

unsigned char Tal1tab[] = {
0x00,0xcc,0xac,0x60,0x79,0xb5,0xd5,0x19,0xf0,0x3c,0x5c,0x90,0x89,0x45,0x25,0xe9,
0xfd,0x31,0x51,0x9d,0x84,0x48,0x28,0xe4,0x0d,0xc1,0xa1,0x6d,0x74,0xb8,0xd8,0x14,
0x2e,0xe2,0x82,0x4e,0x57,0x9b,0xfb,0x37,0xde,0x12,0x72,0xbe,0xa7,0x6b,0x0b,0xc7,
0xd3,0x1f,0x7f,0xb3,0xaa,0x66,0x06,0xca,0x23,0xef,0x8f,0x43,0x5a,0x96,0xf6,0x3a,
0x42,0x8e,0xee,0x22,0x3b,0xf7,0x97,0x5b,0xb2,0x7e,0x1e,0xd2,0xcb,0x07,0x67,0xab,
0xbf,0x73,0x13,0xdf,0xc6,0x0a,0x6a,0xa6,0x4f,0x83,0xe3,0x2f,0x36,0xfa,0x9a,0x56,
0x6c,0xa0,0xc0,0x0c,0x15,0xd9,0xb9,0x75,0x9c,0x50,0x30,0xfc,0xe5,0x29,0x49,0x85,
0x91,0x5d,0x3d,0xf1,0xe8,0x24,0x44,0x88,0x61,0xad,0xcd,0x01,0x18,0xd4,0xb4,0x78,
0xc5,0x09,0x69,0xa5,0xbc,0x70,0x10,0xdc,0x35,0xf9,0x99,0x55,0x4c,0x80,0xe0,0x2c,
0x38,0xf4,0x94,0x58,0x41,0x8d,0xed,0x21,0xc8,0x04,0x64,0xa8,0xb1,0x7d,0x1d,0xd1,
0xeb,0x27,0x47,0x8b,0x92,0x5e,0x3e,0xf2,0x1b,0xd7,0xb7,0x7b,0x62,0xae,0xce,0x02,
0x16,0xda,0xba,0x76,0x6f,0xa3,0xc3,0x0f,0xe6,0x2a,0x4a,0x86,0x9f,0x53,0x33,0xff,
0x87,0x4b,0x2b,0xe7,0xfe,0x32,0x52,0x9e,0x77,0xbb,0xdb,0x17,0x0e,0xc2,0xa2,0x6e,
0x7a,0xb6,0xd6,0x1a,0x03,0xcf,0xaf,0x63,0x8a,0x46,0x26,0xea,0xf3,0x3f,0x5f,0x93,
0xa9,0x65,0x05,0xc9,0xd0,0x1c,0x7c,0xb0,0x59,0x95,0xf5,0x39,0x20,0xec,0x8c,0x40,
0x54,0x98,0xf8,0x34,0x2d,0xe1,0x81,0x4d,0xa4,0x68,0x08,0xc4,0xdd,0x11,0x71,0xbd,
};
/* K=7 r=1/2 Viterbi decoder with optional Intel or PowerPC SIMD
 * Copyright Feb 2004, Phil Karn, KA9Q
 */

/* Create a new instance of a Viterbi decoder */
void *create_viterbi27(int len){
  find_cpu_mode();

  switch(Cpu_mode){
  case PORT:
  default:
    return create_viterbi27_port(len);
#ifdef __VEC__
  case ALTIVEC:
    return create_viterbi27_av(len);
#endif
#ifdef __i386__
  case MMX:
    return create_viterbi27_mmx(len);
  case SSE:
    return create_viterbi27_sse(len);
  case SSE2:
    return create_viterbi27_sse2(len);
#endif
  }
}

void set_viterbi27_polynomial(int polys[2]){
  switch(Cpu_mode){
  case PORT:
  default:
    set_viterbi27_polynomial_port(polys);
    break;
#ifdef __VEC__
  case ALTIVEC:
    set_viterbi27_polynomial_av(polys);
    break;
#endif
#ifdef __i386__
  case MMX:
    set_viterbi27_polynomial_mmx(polys);
    break;
  case SSE:
    set_viterbi27_polynomial_sse(polys);
    break;
  case SSE2:
    set_viterbi27_polynomial_sse2(polys);
    break;
#endif
  }
}

/* Initialize Viterbi decoder for start of new frame */
int init_viterbi27(void *p,int starting_state){
    switch(Cpu_mode){
    case PORT:
    default:
      return init_viterbi27_port(p,starting_state);
#ifdef __VEC__
    case ALTIVEC:
      return init_viterbi27_av(p,starting_state);
#endif
#ifdef __i386__
    case MMX:
      return init_viterbi27_mmx(p,starting_state);
    case SSE:
      return init_viterbi27_sse(p,starting_state);
    case SSE2:
      return init_viterbi27_sse2(p,starting_state);
#endif
    }
}

/* Viterbi chainback */
int chainback_viterbi27(
      void *p,
      unsigned char *data, /* Decoded output data */
      unsigned int nbits, /* Number of data bits */
      unsigned int endstate){ /* Terminal encoder state */

    switch(Cpu_mode){
    case PORT:
    default:
      return chainback_viterbi27_port(p,data,nbits,endstate);
#ifdef __VEC__
    case ALTIVEC:
      return chainback_viterbi27_av(p,data,nbits,endstate);
#endif
#ifdef __i386__
    case MMX:
      return chainback_viterbi27_mmx(p,data,nbits,endstate);
    case SSE:
      return chainback_viterbi27_sse(p,data,nbits,endstate);
    case SSE2:
      return chainback_viterbi27_sse2(p,data,nbits,endstate);
#endif
    }
}

/* Delete instance of a Viterbi decoder */
void delete_viterbi27(void *p){
    switch(Cpu_mode){
    case PORT:
    default:
      delete_viterbi27_port(p);
      break;
#ifdef __VEC__
    case ALTIVEC:
      delete_viterbi27_av(p);
      break;
#endif
#ifdef __i386__
    case MMX:
      delete_viterbi27_mmx(p);
      break;
    case SSE:
      delete_viterbi27_sse(p);
      break;
    case SSE2:
      delete_viterbi27_sse2(p);
      break;
#endif
    }
}

/* Update decoder with a block of demodulated symbols
 * Note that nbits is the number of decoded data bits, not the number
 * of symbols!
 */
int update_viterbi27_blk(void *p,unsigned char syms[],int nbits){
  if(p == NULL)
    return -1;

  switch(Cpu_mode){
  case PORT:
  default:
    update_viterbi27_blk_port(p,syms,nbits);
    break;
#ifdef __VEC__
  case ALTIVEC:
    update_viterbi27_blk_av(p,syms,nbits);
    break;
#endif
#ifdef __i386__
  case MMX:
    update_viterbi27_blk_mmx(p,syms,nbits);
    break;
  case SSE:
    update_viterbi27_blk_sse(p,syms,nbits);
    break;
  case SSE2:
    update_viterbi27_blk_sse2(p,syms,nbits);
    break;
#endif
  }
  return 0;
}
/* K=7 r=1/2 Viterbi decoder in portable C
 * Copyright Feb 2004, Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */

typedef union { unsigned int w[64]; } metric_t;
typedef union { unsigned long w[2];} decision_t;
static union branchtab27 { unsigned char c[32]; } Branchtab27[2] __attribute__ ((aligned(16)));
static int Init = 0;

/* State info for instance of Viterbi decoder
 * Don't change this without also changing references in [mmx|sse|sse2]bfly29.s!
 */
struct v27 {
  metric_t metrics1; /* path metric buffer 1 */
  metric_t metrics2; /* path metric buffer 2 */
  decision_t *dp;          /* Pointer to current decision */
  metric_t *old_metrics,*new_metrics; /* Pointers to path metrics, swapped on every bit */
  decision_t *decisions;   /* Beginning of decisions for block */
};

/* Initialize Viterbi decoder for start of new frame */
int init_viterbi27_port(void *p,int starting_state){
  struct v27 *vp = p;
  int i;

  if(p == NULL)
    return -1;
  for(i=0;i<64;i++)
    vp->metrics1.w[i] = 63;

  vp->old_metrics = &vp->metrics1;
  vp->new_metrics = &vp->metrics2;
  vp->dp = vp->decisions;
  vp->old_metrics->w[starting_state & 63] = 0; /* Bias known start state */
  return 0;
}

void set_viterbi27_polynomial_port(int polys[2]){
  int state;

  for(state=0;state < 32;state++){
    Branchtab27[0].c[state] = (polys[0] < 0) ^ parity((2*state) & abs(polys[0])) ? 255 : 0;
    Branchtab27[1].c[state] = (polys[1] < 0) ^ parity((2*state) & abs(polys[1])) ? 255 : 0;
  }
  Init++;
}

/* Create a new instance of a Viterbi decoder */
void *create_viterbi27_port(int len){
  struct v27 *vp;

  if(!Init){
    int polys[2] = { V27POLYA, V27POLYB };
    set_viterbi27_polynomial_port(polys);
  }
  if((vp = malloc(sizeof(struct v27))) == NULL)
     return NULL;
  if((vp->decisions = malloc((len+6)*sizeof(decision_t))) == NULL){
    free(vp);
    return NULL;
  }
  init_viterbi27_port(vp,0);

  return vp;
}

/* Viterbi chainback */
int chainback_viterbi27_port(
      void *p,
      unsigned char *data, /* Decoded output data */
      unsigned int nbits, /* Number of data bits */
      unsigned int endstate){ /* Terminal encoder state */
  struct v27 *vp = p;
  decision_t *d;

  if(p == NULL)
    return -1;
  d = vp->decisions;
  /* Make room beyond the end of the encoder register so we can
   * accumulate a full byte of decoded data
   */
  endstate %= 64;
  endstate <<= 2;

  /* The store into data[] only needs to be done every 8 bits.
   * But this avoids a conditional branch, and the writes will
   * combine in the cache anyway
   */
  d += 6; /* Look past tail */
  while(nbits-- != 0){
    int k;

    k = (d[nbits].w[(endstate>>2)/32] >> ((endstate>>2)%32)) & 1;
    data[nbits>>3] = endstate = (endstate >> 1) | (k << 7);
  }
  return 0;
}

/* Delete instance of a Viterbi decoder */
void delete_viterbi27_port(void *p){
  struct v27 *vp = p;

  if(vp != NULL){
    free(vp->decisions);
    free(vp);
  }
}

/* C-language butterfly */
#define BFLY(i) {\
unsigned int metric,m0,m1,decision;\
    metric = (Branchtab27[0].c[i] ^ sym0) + (Branchtab27[1].c[i] ^ sym1);\
    m0 = vp->old_metrics->w[i] + metric;\
    m1 = vp->old_metrics->w[i+32] + (510 - metric);\
    decision = (signed int)(m0-m1) > 0;\
    vp->new_metrics->w[2*i] = decision ? m1 : m0;\
    d->w[i/16] |= decision << ((2*i)&31);\
    m0 -= (metric+metric-510);\
    m1 += (metric+metric-510);\
    decision = (signed int)(m0-m1) > 0;\
    vp->new_metrics->w[2*i+1] = decision ? m1 : m0;\
    d->w[i/16] |= decision << ((2*i+1)&31);\
}

/* Update decoder with a block of demodulated symbols
 * Note that nbits is the number of decoded data bits, not the number
 * of symbols!
 */
int update_viterbi27_blk_port(void *p,unsigned char *syms,int nbits){
  struct v27 *vp = p;
  void *tmp;
  decision_t *d;

  if(p == NULL)
    return -1;
  d = (decision_t *)vp->dp;
  while(nbits--){
    unsigned char sym0,sym1;

    d->w[0] = d->w[1] = 0;
    sym0 = *syms++;
    sym1 = *syms++;
    
    BFLY(0);
    BFLY(1);
    BFLY(2);
    BFLY(3);
    BFLY(4);
    BFLY(5);
    BFLY(6);
    BFLY(7);
    BFLY(8);
    BFLY(9);
    BFLY(10);
    BFLY(11);
    BFLY(12);
    BFLY(13);
    BFLY(14);
    BFLY(15);
    BFLY(16);
    BFLY(17);
    BFLY(18);
    BFLY(19);
    BFLY(20);
    BFLY(21);
    BFLY(22);
    BFLY(23);
    BFLY(24);
    BFLY(25);
    BFLY(26);
    BFLY(27);
    BFLY(28);
    BFLY(29);
    BFLY(30);
    BFLY(31);
    d++;
    /* Swap pointers to old and new metrics */
    tmp = vp->old_metrics;
    vp->old_metrics = vp->new_metrics;
    vp->new_metrics = tmp;
  }    
  vp->dp = d;
  return 0;
}
/* Utility routines for FEC support
 * Copyright 2004, Phil Karn, KA9Q
 */

unsigned char Partab[256];
int P_init;

/* Create 256-entry odd-parity lookup table
 * Needed only on non-ia32 machines
 */
void partab_init(void){
  int i,cnt,ti;

  /* Initialize parity lookup table */
  for(i=0;i<256;i++){
    cnt = 0;
    ti = i;
    while(ti){
      if(ti & 1)
	cnt++;
      ti >>= 1;
    }
    Partab[i] = cnt & 1;
  }
  P_init=1;
}

/* Lookup table giving count of 1 bits for integers 0-255 */
int Bitcnt[] = {
 0, 1, 1, 2, 1, 2, 2, 3,
 1, 2, 2, 3, 2, 3, 3, 4,
 1, 2, 2, 3, 2, 3, 3, 4,
 2, 3, 3, 4, 3, 4, 4, 5,
 1, 2, 2, 3, 2, 3, 3, 4,
 2, 3, 3, 4, 3, 4, 4, 5,
 2, 3, 3, 4, 3, 4, 4, 5,
 3, 4, 4, 5, 4, 5, 5, 6,
 1, 2, 2, 3, 2, 3, 3, 4,
 2, 3, 3, 4, 3, 4, 4, 5,
 2, 3, 3, 4, 3, 4, 4, 5,
 3, 4, 4, 5, 4, 5, 5, 6,
 2, 3, 3, 4, 3, 4, 4, 5,
 3, 4, 4, 5, 4, 5, 5, 6,
 3, 4, 4, 5, 4, 5, 5, 6,
 4, 5, 5, 6, 5, 6, 6, 7,
 1, 2, 2, 3, 2, 3, 3, 4,
 2, 3, 3, 4, 3, 4, 4, 5,
 2, 3, 3, 4, 3, 4, 4, 5,
 3, 4, 4, 5, 4, 5, 5, 6,
 2, 3, 3, 4, 3, 4, 4, 5,
 3, 4, 4, 5, 4, 5, 5, 6,
 3, 4, 4, 5, 4, 5, 5, 6,
 4, 5, 5, 6, 5, 6, 6, 7,
 2, 3, 3, 4, 3, 4, 4, 5,
 3, 4, 4, 5, 4, 5, 5, 6,
 3, 4, 4, 5, 4, 5, 5, 6,
 4, 5, 5, 6, 5, 6, 6, 7,
 3, 4, 4, 5, 4, 5, 5, 6,
 4, 5, 5, 6, 5, 6, 6, 7,
 4, 5, 5, 6, 5, 6, 6, 7,
 5, 6, 6, 7, 6, 7, 7, 8,
};

/* Determine CPU support for SIMD
 * Copyright 2004 Phil Karn, KA9Q
 */

/* Various SIMD instruction set names */
char *Cpu_modes[] = {"Unknown","Portable C","x86 Multi Media Extensions (MMX)",
		   "x86 Streaming SIMD Extensions (SSE)",
		   "x86 Streaming SIMD Extensions 2 (SSE2)",
		   "PowerPC G4/G5 Altivec/Velocity Engine"};

enum cpu_mode Cpu_mode;

void find_cpu_mode(void){

  if(Cpu_mode != UNKNOWN)
    return;

  /* Figure out what kind of CPU we have */
  //f = cpu_features();
  //if(f & (1<<26)){ /* SSE2 is present */
  //  Cpu_mode = SSE2;
  //} else if(f & (1<<25)){ /* SSE is present */
  //  Cpu_mode = SSE;
  //} else if(f & (1<<23)){ /* MMX is present */
  //  Cpu_mode = MMX;
  //} else { /* No SIMD at all */
  //  Cpu_mode = PORT;
  //}
  Cpu_mode = PORT;

}
