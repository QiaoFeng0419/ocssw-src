/*============================================================================
                 CONFIDENTIAL    Do not distribute.
  This source code contains confidential and proprietary algorithms.
  ------------------------------------------------------------------------

  (C) Copyright 1993,1994 University of New Mexico. All Rights Reserved.
          Name: Usds.c

FOR INTERNAL USE ONLY  INCLUDES MS OPTION

   Description: Decoding software compatible with USES chip and uses software.
        Software is to be used for evaluation purposes only,
        not for commercial purposes.
        Author: Jack Venbrux of the Microelectronics Research Center
      Date: September 9, 1993
       Revised: May 4, 1994
    To Compile: cc -O -o Usds Usds.c 
      For help: compile and type: Usds 
=============================================================================*/
#include <stdlib.h>  /* for PC compatibility*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*****************************************************************************
                       GLOBAL DEFINES
*****************************************************************************/
#define MIN(x,y) ((x)<(y)? (x): (y))

#define MAXLINE         80
#define INBLK_size    1024
#define TYP              0 
#define EXT2             1
#define ZERO             2
#define ID_DECODE        3
#define MAX_BLKREF     128
#define MAX_REFPAC     128
#define MAX_ZBLKS       64
#define J_MAX           16
#define PIXREF_DEFAULT 256
#define N_DEFAULT        8
#define J_DEFAULT       16

/*****************************************************************************
                       GLOBAL VARIABLES
*****************************************************************************/
int SIGN_EXT_OUTPUT=1;  /* sign extend output bits to nearest next byte*/

unsigned IN_array[INBLK_size+1];
unsigned XP_IN[INBLK_size+1];
char     Buffer_in[INBLK_size*2+2];
char     Buffer_in2[10000];
char     *Binptr; /* buffer ptr */
unsigned SIG[2048+1];  /* maximum line size.. max size of zero blks */
unsigned short OUT[4096+1];
char     Buffer_out[10240];
char     *Bptr; /* buffer ptr */
char     *Bend; /* buffer end ptr */

int  N;
int  NN;
int  J;
int  XP;       /* predictor: last pixel from previous block */
int  MASK_SIGN;
int  MASK_IN;
int  MASKN_neg;
unsigned short MASKNOT[17];
int  NEG_data;
int  IDbits;
int  REF;           /* reference flag */
int  REF2REF;       /* difference references */
int  REFREF_flag;   /* difference references */
int  TRUEREF;       /* true when REF*(!REF2REF) */
int  SCANLINE;
int  BYTE_pixel;
int  IN_INDEX=0;
int  NEXT_BIT=15;
int  BYPASS_flag;
int  NN_mode;
int  EP_mode;
int  TWOD_mode;
int  TWOD_line_mode; /* first line with NN, rest with 2d */
int  MS_mode;
int  BYPASS_mode;
int  ENT_CODING;
int  BlkPac;
int  PixRef;
int  BlkRef;
int  RefPac;
int  PACKET_flag;
int  EXT2_flag;
int  EXT2_bit; /* ID flag bit */
int  ZOP_flag;
int  ZBLKS;
int  ZFLAG;
int  LAST_word;
int  XMAX;
int  XMAX_MS;
int  XMAXneg;
FILE *In_ptr,*Ep_ptr,*Out_ptr,*Twod_ptr,*Ms_ptr;
char In_file[MAXLINE];
unsigned REFERENCE;
int  K_bits,Kfactor;
long BLKCNT;
int FS_id,Low_id,D_id;
int  MS[]={0,1,3,6,10,15,21,28};
int  M[]={ 0, 1,1, 2,2,2, 3,3,3,3, 4,4,4,4,4, 5,5,5,5,5,5, 6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7};
char copyright[] = "@(#) (C) Copyright 1993,1994 University of New Mexico. All Rights Reserved.";

static int More_data=1;
static int end_fill=0;
static char sccs_id[] = "@(#) Usds.c - Version: 1.3, Revised: 06/10/96 10:48:30";
static char noncommercial1[] = "This software is for evaluating the extended Rice algorithm found";
static char noncommercial2[] = "in the USES chip.  It is not to be used for commercial purposes.";
static char credits1[] = "This work was supported in part by NASA Space Engineering Research Center grant";
static char credits2[] = "NAGW-3293 and Lossless Data Compression grant NAG 5-2166.  Work on USES was in";
static char credits3[] = "collaboration with NASA, Goddard Space Flight Center, Greenbelt, Maryland, USA."; 


/*****************************************************************************
                      FUNCTION PROTOTYPES
*****************************************************************************/
int get_ref();
int un_z();
int check_end( int id, int end_fill);
int get_input( int *max_data, int32_t inBytes, 
               uint8_t *encryptData);
int get_EP_IN( FILE *file_ptr, int pixels);

void un_K();
void un_ext2();
void un_FS( int flag);
void un_DEF ();
void data_output( uint16_t *decryptData, int *decryptPtr);
//void data_output();
//void flush_buffer();
void flush_buffer( uint16_t *decryptData, int32_t *decryptPtr);
void check_ref_per_sample( int blkref_flag, int pixref_flag);
void open_files( int argc, char *argv[]);
void set_flags( int argc, char *argv[], int *pixref_flag, int *blkref_flag);
void initialize( int pixref_flag, int blkref_flag);
void do_uncoding( int32_t inBytes, 
                  uint8_t *encryptData, 
                  uint16_t *decryptData);

/*****************************************************************************
                      CODING SECTION
*****************************************************************************/

/* main(argc,argv)*/

//int usds(argc,argv)
int usds(argc,argv, inBytes, encryptData, decryptData)
int argc;
char *argv[];
int32_t inBytes;
uint8_t *encryptData;
uint16_t *decryptData;

{
    int pixref_flag,blkref_flag;
    set_flags(argc,argv,&pixref_flag,&blkref_flag);
    initialize(pixref_flag,blkref_flag);
    do_uncoding( inBytes, encryptData, decryptData);
    //    fclose(In_ptr);
    //    fclose(Out_ptr);
    return(0);
}


/*--------------------------------------------------------------------------- 
do_uncoding() 
---------------------------------------------------------------------------*/
void do_uncoding( int32_t inBytes, 
                  uint8_t *encryptData, 
                  uint16_t *decryptData)
{
    int id;
    int new_packet,max_data;
    int i,zblks;

    static int32_t decryptPtr = 0;

    BLKCNT=0;
    ZFLAG=0;
    new_packet=0;
    REFERENCE=0;
    TRUEREF=REF;
    while ((id=get_input(&max_data, inBytes, encryptData)) >= 0)
    {
      //      if ( inBytes == 2056) printf("id %d\n", id);
        BLKCNT++;
        for (i=0;i<J;i++)
            SIG[i]= 0; /* when N large and k large this is needed */
        if (TRUEREF) 
        {
            REFERENCE=get_ref(); /* REFERENCE is X+XMAXneg */
            if ((id > FS_id)&&(id != D_id))
                un_K();
            else if (id == FS_id)
                un_FS(TYP);
            else if (id == Low_id)
                if (EXT2_bit)
                    un_ext2();
                else
                {
                    zblks=un_z();
                    BLKCNT=BLKCNT+zblks-1;
                }
            else
                un_DEF();
            data_output( decryptData, &decryptPtr);
        }
        else
        {
            if ((id > FS_id)&&(id != D_id))
                un_K();
            else if (id == FS_id)
                un_FS(TYP);
            else if (id == Low_id)
                if (EXT2_bit)
                    un_ext2();
                else
                {
                    zblks=un_z();
                    BLKCNT=BLKCNT+zblks-1;
                }
            else un_DEF();
            data_output( decryptData, &decryptPtr);
        }
        if ((PACKET_flag)&&(BLKCNT % BlkPac==0))
        { /* end of packet */
            /* if NEXT_BIT==15. then packet ended on word boundary, no fill*/
            if (NEXT_BIT != 15)
                IN_INDEX++;  /* skip to next word */
            NEXT_BIT=15;
            new_packet=1;
        }
        REF=0;
        REF2REF=0;
        TRUEREF=0;
        if (BLKCNT%BlkRef==0)
        {
            TWOD_line_mode=0;
            REF2REF=(((!EP_mode)&&(REFREF_flag))&&(!new_packet)&&(!BYPASS_mode));
            REF= ((!EP_mode)&&(!BYPASS_mode));
        TRUEREF=((REF)&&(!REF2REF));
            if (IN_INDEX > max_data)
        {
          flush_buffer( decryptData, &decryptPtr);
                   return;
            }
        }
        new_packet=0;
    }
    /*    fprintf(stderr," id %d\n", id); */
    flush_buffer( decryptData, &decryptPtr);
    return;
}
/*--------------------------------------------------------------------------- 
get_ref() 
---------------------------------------------------------------------------*/
int get_ref()
{
    int in0,in1,msb,lsb,shift;
    int ref,in_bit_ptr;

    /*--- globals ---------*/
    in0=IN_array[IN_INDEX];
    in1=IN_array[IN_INDEX+1];
    in_bit_ptr=NEXT_BIT;

    /* condition 1: reference contained within first word*/
    in0=in0 & MASKNOT[in_bit_ptr+1]; /* zero out leading bits */
    if (in_bit_ptr >= (NN-1))
    {
        shift=in_bit_ptr-(NN-1);
        ref=(in0>>shift);
        if (shift==0)
        {
            NEXT_BIT=15;
            IN_INDEX++;
        }
        else
            NEXT_BIT=shift-1;
    }
    else 
        /* condition 2: reference spans two bytes*/
        {
            shift=NN-(in_bit_ptr+1);
            msb=(in0<<shift);
            lsb= in1 >> (16-shift);
            ref=msb|lsb;
            NEXT_BIT= 15-shift;
            IN_INDEX++;
        }
    return(ref);
}
/*---------------------------------------------------------------------------
get_id3 
---------------------------------------------------------------------------*/
int get_id3()
{
    int id,shift,msb,lsb;

    /*----- get ID -----*/
    if (NEXT_BIT >= 2)
    {
        if (NEXT_BIT==2)
        {
            NEXT_BIT=15;
            id=IN_array[IN_INDEX++] & 0x07;
        }
        else
        {
            shift=NEXT_BIT-2;
            id=(IN_array[IN_INDEX]>>shift)&0x07;
            NEXT_BIT=shift-1;
        }
    }
    else 
    {
        msb=(IN_array[IN_INDEX]<<(2-NEXT_BIT))&0x07;
        NEXT_BIT=15-(2-NEXT_BIT);
        lsb=(IN_array[++IN_INDEX]>>(NEXT_BIT+1));
        id=msb|lsb;
    }
    K_bits= id-Kfactor;
    if (id==Low_id)
    {
        /*---- find if EXT2 or ZERO flag -----*/
        EXT2_bit=(IN_array[IN_INDEX]>>NEXT_BIT)&0x01;
        if (NEXT_BIT==0)
        {
            NEXT_BIT=15;
            IN_INDEX++;
        }
        else
            NEXT_BIT--;
    }
    return(id);
}

/*---------------------------------------------------------------------------
get_id4  
---------------------------------------------------------------------------*/
int get_id4()
{
    int id,shift,msb,lsb;

    /*----- get ID -----*/
    if (NEXT_BIT >= 3)
    {
        if (NEXT_BIT==3)
        {
            NEXT_BIT=15;
            id=IN_array[IN_INDEX++] & 0x0F;
        }
        else
        {
            shift=NEXT_BIT-3;
            id=(IN_array[IN_INDEX]>>shift)&0x0F;
            NEXT_BIT=shift-1;
        }
    }
    else 
    {
        msb=(IN_array[IN_INDEX]<<(3-NEXT_BIT))&0x0F;
        NEXT_BIT=15-(3-NEXT_BIT);
        lsb=(IN_array[++IN_INDEX]>>(NEXT_BIT+1));
        id=msb|lsb;
    }
    K_bits= id-Kfactor;
    if (id==Low_id)
    {
        /*---- find if EXT2 or ZERO flag -----*/
        EXT2_bit=(IN_array[IN_INDEX]>>NEXT_BIT)&0x01;
        if (NEXT_BIT==0)
        {
            NEXT_BIT=15;
            IN_INDEX++;
        }
        else
            NEXT_BIT--;
    }
    return(id);
}
/*---------------------------------------------------------------------------
    Function: un_z  
---------------------------------------------------------------------------*/
int un_z()
{
    int i,j,bits,zcnt,zblks,line_blk;

    /* find size of  FS coded word */
    un_FS(ZERO);
    bits=SIG[0]+1;

    if ((LAST_word)&&(bits > MAX_ZBLKS))
      return 0;
      /* (void)exit(0); */

    /* number of blocks of zeros = zerocnt+1 */
    if (bits <5)
        zblks=bits;
    else if (bits==5)
    {
        /* EOL condition NOTE: USES will not code this if <5 blks of zeros*/
    /* how many blocks to EOL?... USES forces an EOL after 64 blocks  */
    /* present blk for given line is equal to (BLKCNT%BlkRef)+1       */
    line_blk=(BLKCNT%BlkRef); /* start of blk cnt for given line*/
    if (BlkRef > 64)
         {
         if (line_blk > MAX_ZBLKS)
                 zblks=BlkRef-line_blk+1;
         else
                 zblks=MAX_ZBLKS-line_blk+1;
         }
    else
             zblks=BlkRef-line_blk+1;
        /* but hardware limited to MAX_ZBLKS blocks */
        /**** new stuff to be compatible with USES chip ****/
    }
    else 
        zblks=bits-1;
    if (TRUEREF)
    {
        SIG[0]=REFERENCE;
        for (i=1;i<J;i++)
            SIG[i]=0;
    }
    else
    {
        for (i=0;i<J;i++)
            SIG[i]=0;
    }
    zcnt=J;
    for (j=2;j<=zblks;j++)
    {
        for (i=0;i<J;i++)
            SIG[zcnt++]=0;
    }
    ZFLAG=1;
    ZBLKS=zblks;
    if (ZBLKS > BlkRef)
    {
        fprintf(stderr,"ERROR: Usds lost. Illegal number of zero_blks=%d. Must decode with same\n",ZBLKS);
        fprintf(stderr,"       options you encoded with. Aborting on blk %ld, pixel %ld.\n",BLKCNT,BLKCNT*J);
        (void)exit(1);
    }
    return(zblks);
}
/*---------------------------------------------------------------------------
    Function: un_ext2  
---------------------------------------------------------------------------*/
void un_ext2()
{
    int i,j,itemp,m,b;
    int temp[J_MAX];  

    /* fill in the J/2 SIG values */
    j=0;
    itemp=J/2+TRUEREF;
    un_FS(EXT2);
    for (i=TRUEREF;i<itemp;i++)
    {
        m=SIG[i];/* after FS fcn SIG array is filled with FS */
        b=M[m];
        temp[j+1]=m-MS[b];
        temp[j]=b-temp[j+1];
        j=j+2;
    }
    for (i=0;i<J;i++)
        SIG[i]=temp[i];  /* fill global array for output fcn to use */
}
/*---------------------------------------------------------------------------
    Function: un_FS  
---------------------------------------------------------------------------*/
void un_FS(flag)
int flag;
{
    int sigwrds;
    int zerocnt,one_cnt,bits;
    int word;

    zerocnt=0;

    if (flag==TYP)
    {
        one_cnt=J-TRUEREF;
        sigwrds=TRUEREF;
    }
    else if (flag==EXT2)
    {
        one_cnt=J/2;
        sigwrds=TRUEREF;
    }
    else /* count FS zeros */ 
        {
        one_cnt=1;
        sigwrds=0;
        }

    word=IN_array[IN_INDEX++]<<(15-NEXT_BIT);  /* get to first FS bit */
    bits=NEXT_BIT+1;           /* remaining bits of first word */

    /*----------- FS decoding -------------------------*/
    while (one_cnt>0)
    {
        if ((word&0x8000)==0)
            zerocnt++;
        else /* found a "1" */
            {
                SIG[sigwrds++]=zerocnt;
                zerocnt=0;
                one_cnt--;
            }
        word=word<<1;
        bits--;
        if (bits==0)  /* finished 1 word */
            {
                word=IN_array[IN_INDEX++];
                bits=16;
            }
    }
    IN_INDEX--;
    NEXT_BIT=bits-1;
}
/*---------------------------------------------------------------------------
    Function: un_K 
---------------------------------------------------------------------------*/
void un_K ()
{
    int i,bitptr,dat,dat1,dat0,shift,mask;
    int zerocnt,one_cnt,bits;
    int word;
    int sigwrds;

    mask=MASKNOT[K_bits];
    /*----------- FS decoding -------------------------*/
    zerocnt=0;
    sigwrds=TRUEREF;
    one_cnt=J-TRUEREF;
    word=IN_array[IN_INDEX++]<<(15-NEXT_BIT);  /* get to first FS bit */
    bits=NEXT_BIT+1;           /* remaining bits of first word */
    while (one_cnt>0)
    {
        if ((word&0x8000)==0)
            zerocnt++;
        else /* found a "1" */
            {
                SIG[sigwrds++]=zerocnt;
                zerocnt=0;
                one_cnt--;
            }
        word=word<<1;
        bits--;
        if (bits==0)  /* finished 1 word */
            {
                word=IN_array[IN_INDEX++];
                bits=16;
            }
    }
    NEXT_BIT=bits-1;
    IN_INDEX--;
    bitptr=bits;
    /*-------------------------------------------------*/
    for (i=TRUEREF;i<J;i++)
    {
        if (bitptr > K_bits)
        {
            bitptr=bitptr-K_bits;
            dat=((IN_array[IN_INDEX])>>bitptr)&mask;
        }
        else if (bitptr < K_bits)
        {
            shift=K_bits-bitptr;
            dat0=(IN_array[IN_INDEX++]<<shift)&mask;
            bitptr=16-shift;
            dat1=((IN_array[IN_INDEX])>>(bitptr))&mask;
            dat=(dat0|dat1);
        }
        else
        {
            dat=IN_array[IN_INDEX++]&mask;
            bitptr=16;
        }
        SIG[i]= (SIG[i]<<K_bits) | dat;
    }
    NEXT_BIT=bitptr-1;
}
/*---------------------------------------------------------------------------
    Function: un_DEF  
---------------------------------------------------------------------------*/
void un_DEF ()
{
    int t_cnt,bits,sigwrds,D_cnt;
    int unsigned short word; /* must be a short */

    sigwrds=TRUEREF;
    D_cnt=J-TRUEREF;
    word=IN_array[IN_INDEX++];
    word=word<<(15-NEXT_BIT);  /* get to first Default bit */
    bits=NEXT_BIT+1;           /* remaining bits of first word */
    /*-------------------------------------------------*/
    t_cnt=1;
    while (D_cnt > 0)
    {
        SIG[sigwrds]=(SIG[sigwrds]<<1)|(((unsigned)word)>>15);
        if (t_cnt==NN )
        {
            sigwrds++;
            t_cnt=0;
            D_cnt--;
        }
        t_cnt++;
        word=word<<1;
        bits--;
        if (bits==0)
        {
            word=IN_array[IN_INDEX++];
            bits=16;
        }
    }
    IN_INDEX--;
    NEXT_BIT=bits-1;
}
/*--------------------------------------------------------------------------- 
    Function: unmap_NN
---------------------------------------------------------------------------*/
void unmap_NN(start,pixels)
int start,pixels;
{
    int i,x,sig;

    for (i=start;i<pixels;i++)
    {
        sig=SIG[i];

        if (sig >= (XP<<1))
            x=sig;
        else if (sig > ((XMAX-XP)<<1))
            x=XMAX-sig;
        else if (sig&1)
            x=XP-((sig+1)>>1);
        else 
            x=XP+(sig>>1);
        OUT[i]=x-XMAXneg;
        XP=x;
    }
}
/*--------------------------------------------------------------------------- 
    Function: unmap_EP
---------------------------------------------------------------------------*/
void unmap_EP(start,pixels)
int start,pixels;
{
    int i,x,xp,sig;

    (void) get_EP_IN(Ep_ptr,pixels); /* fill block of XP_IN predictors*/
    for (i=start;i<pixels;i++)
    {
        xp=XP_IN[i];
        sig=SIG[i];
        if (sig >= (xp<<1))
            x=sig;
        else if (sig > ((XMAX-xp)<<1))
            x=XMAX-sig;
        else if (sig&1)
            x=xp-((sig+1)>>1);
        else 
            x=xp+(sig>>1);
        OUT[i]=x-XMAXneg;
    }
}
/*--------------------------------------------------------------------------- 
    Function: unmap_2D
---------------------------------------------------------------------------*/
void unmap_TWOD(start,pixels)
int start,pixels;
{
    int i,x,xp,sig;

    (void) get_EP_IN(Twod_ptr,pixels); /* fill block of XP_IN predictors*/

    for (i=start;i<pixels;i++)
    {
        xp=(XP+XP_IN[i])/2;
        sig=SIG[i];
        if (sig >= (xp<<1))
            x=sig;
        else if (sig > ((XMAX-xp)<<1))
            x=XMAX-sig;
        else if (sig&1)
            x=xp-((sig+1)>>1);
        else 
            x=xp+(sig>>1);
        OUT[i]=x-XMAXneg;
        XP=x;
    }
}
/*---------------------------------------------------------------------------
    Function: unmap_MS
    NOTE on REFERENCES:
       --------------------------------------------------
       USES does the following with Raw data X and XP:
          REFERENCE=(X+XMAXneg)-(XP+XMAXneg)+XMAX
          where XMAXneg=(2**(N-1))if bipolar data and XMAXneg=0 else.
                XMAX=(2**N)-1
          eg. for N=12, XMAXneg=2048 and XMAX=4095
       --------------------------------------------------
       USDS does the inverse
          (X+XMAXneg)=REFERENCE+(XP+XMAXneg)-XMAX
          X=REFERENCE+(XP+XMAXneg)-XMAX-XMAXneg
       --------------------------------------------------

---------------------------------------------------------------------------*/
void unmap_MS(start,pixels)
int start,pixels;
{
    int i,x,d0,d1,sig;
    static int prev_d1=0;

  /* fill block of MS predictors,bipolar is shifted already*/
  (void) get_EP_IN(Ms_ptr,pixels);

   if (REF)
       {
       prev_d1=REFERENCE;
       if (! REF2REF)  /* XP_IN[0] already has been level shifted */
          OUT[0]=REFERENCE+XP_IN[0]-XMAX-XMAXneg; /* refs are level shifted */
       }


    for (i=start;i<pixels;i++)
        {
        sig=SIG[i];

        if (sig >= (prev_d1<<1))
            d1=sig;
        else if (sig > ((XMAX_MS-prev_d1)<<1))
            d1=XMAX_MS-sig;
        else if (sig&1)
            d1=prev_d1-((sig+1)>>1);
        else
            d1=prev_d1+(sig>>1);

        d0=d1-XMAX;
        x=d0+XP_IN[i];
        prev_d1=d1;
        OUT[i]=x-XMAXneg;
        }
}
/*---------------------------------------------------------------------------
   Function: findid 
---------------------------------------------------------------------------*/
int findid(max_data)
int max_data;
{
  
  int id;
  
  if (IN_INDEX > max_data)
    return(-1);
  
  if (!BYPASS_mode)
    {
      if (NN>8)
	id=get_id4();
      else
	id=get_id3();
      
      if (IN_INDEX > max_data)
	return(-1);
      
      if ((id < 0)||(id > D_id)) 
	{
	  fprintf(stderr,"ERROR: Usds lost-- found bogus ID of %d. You must decode with same\n",id);
	  fprintf(stderr,"	   options you encoded with. Aborting on blk %ld, pixel %ld.\n",BLKCNT,
		  BLKCNT*J);
	  (void)exit(1);
	}
    }
  else
    id=D_id; /* in Bypass mode */
  
  return(id);
}
/*****************************************************************************
                     INPUT OUTPUT SECTION 
*****************************************************************************/
/*---------------------------------------------------------------------------
   Function: get_input  
Description: grab chunks of data and fill up IN_array[]
---------------------------------------------------------------------------*/
int get_input( int *max_data, int32_t inBytes, 
               uint8_t *encryptData)
//int *max_data;
{
    int id,i,index,msb,incnt,cnt=0;
    static int   Max_data=0;
    static int called_cnt=0;
    static int encryptPtr=0;
    //    if ( inBytes == 2056) printf("called_cnt: %d\n", called_cnt);
    called_cnt++;

    /*-------- get data section --------------------------*/
    if (IN_INDEX >= INBLK_size-18)
      { /* close to end of IN_array readjust then refill with more data*/
        if (Max_data == INBLK_size-1)
          { /* if Max_data < INBLK_size then no more data to load in */
            cnt=0;
            for (index=IN_INDEX;index<INBLK_size;index++)
              IN_array[cnt++]=IN_array[index];
            IN_INDEX=0;
            More_data=1;
          }
      }
    if (More_data)
      {
        Binptr= &Buffer_in[0];
        More_data=0;

        //        printf("encryptPtr: %d\n", encryptPtr);
        if ( ((INBLK_size-cnt)<<1) > inBytes) {
          memcpy( (void*)Buffer_in, encryptData, inBytes);
          incnt = inBytes;
          encryptPtr = 0;
        } else {
          if ( encryptPtr == 0) {
            memcpy( (void*)Buffer_in, encryptData, ((INBLK_size-cnt)<<1));
            incnt = (INBLK_size-cnt)<<1;
            encryptPtr = incnt;
          } else {
            int32_t nread = inBytes-encryptPtr;
            if ( (inBytes-encryptPtr) > (INBLK_size-cnt)<<1) nread = (INBLK_size-cnt)<<1;
            memcpy ( (void*)Buffer_in, &encryptData[encryptPtr], nread);
            incnt = nread;
            encryptPtr += incnt;
            if ( encryptPtr == inBytes) encryptPtr = 0;
          }
        }

        if (incnt >= 1)
          {
            //            if ( inBytes == 2056) printf("incnt: %d\n", incnt);
            //            printf("buffer_in[0]: %d\n", Buffer_in[0]);
            index=(incnt>>1); /* divide by 2 */
            for (i=0;i<index;i++)
              {
                msb = (*Binptr)<<8;; 
                Binptr++;
                IN_array[cnt++]=((msb|((*Binptr)&0xFF))&0xFFFF);
                Binptr++;
              }
            if ((incnt&0x01)==1)
              {  /* final byte is fill */
                IN_array[cnt++]=((((*Binptr)<<8)|0xFF)&0xFFFF);
                Binptr++;
                end_fill=1;  /* entire byte of fill */
              }
          }
        //        if ( inBytes == 2056) printf("cnt: %d\n", cnt);
        Max_data=cnt-1;
        IN_array[cnt] = 0; // JMG  10/17/14
      }
    *max_data=Max_data;
    id=findid(Max_data);

    if (IN_INDEX >= Max_data)
    {
        LAST_word=1;
        id=check_end(id,end_fill);
    
    }

    return(id);
}
/*---------------------------------------------------------------------------
check_end()
Description: determine when file ends (final fill bits have been turned to '1's
             unless operating in Chip Mode)
---------------------------------------------------------------------------*/
int check_end(id,end_fill)
int id,end_fill;
{
    int zbits,ext2bits,fsbits,blks_togo;

    /* Smallest blocksize != zero, is ext2..J=8,IDcompressed totcnt=7 bits */
    /* so using smallest blocksize.. see if possible to fit a scanline     */
    /* in the last data word. Encoder has already filled with              */
    /* ones anything that isn't data in the last byte (so no zero IDs)     */
    /* The smallest winning blocksize for FS is 10 bits                    */

    if (id > FS_id)
        id=(-1);
    else
    {
        /*ID found in last word.. so no more than 15 bits left in lastword*/
        /* if zero or ext2.. toggle bit exists-- no more than 14 bits left*/

        blks_togo=BlkRef-(BLKCNT%BlkRef);

        zbits=1;  /* once id and toggle grabbed.. 1 bit needed to end zero blk*/
        ext2bits=((J/2)+1);
        fsbits=J+1;

        /**if (blks_togo > 1)**/
    /** force this never to happen to use with */
        if (blks_togo > 9999)
        {
            zbits++;  /* once id is grabbed, only 2 bits needed to end zblks*/
            ext2bits=ext2bits+5;  /* coding ID + zeroblk*/
            fsbits=fsbits+5;      /* coding ID + zeroblock */
        }
        if ((id == Low_id)&&(EXT2_bit==0))
        {  /* zero blk */
            if  (((NEXT_BIT+1)-(end_fill*8)) < zbits)
                id=(-1);
        }
        else if ((id == FS_id) &&
            (((NEXT_BIT+1)-(end_fill*8)) < fsbits))
            id=(-1);
        else if  ((id == Low_id)&&(((NEXT_BIT+1)-(end_fill*8)) < ext2bits))
            id=(-1);
        else if  (id > FS_id)
            id=(-1);
    }
    return(id);
}

/*--------------------------------------------------------------------------- 
    Function: get_EP_IN() 
    Description: fill XP_IN[] array with pixels from an external pixel file
    Returns:  Number of samples read in (1,2,..)
---------------------------------------------------------------------------*/
int get_EP_IN(file_ptr,pixels)
FILE *file_ptr;
int pixels;
{
    int i,lsb,msb,x,pos,x_cnt;

    x_cnt=0;
    if (BYTE_pixel == 2)  /* 2 bytes per pixel */
        {
            while(x_cnt <pixels)
            {
                msb = getc(file_ptr);
                if ((lsb = getc(file_ptr)) == EOF)
                    goto BOTTOM;
                x = (msb<<8) | lsb;
                XP_IN[x_cnt++]=(x & MASK_IN);
            }
        }/*twobytes*/
    else /* one byte per pixel */
        {
            while(x_cnt <pixels)
            {
                if ((x = getc(file_ptr)) == EOF)
                    goto BOTTOM;
                XP_IN[x_cnt++]= (x & MASK_IN);
            }
       }
    if (NEG_data)
    {
        for (i=0;i<x_cnt;i++)
        {
            if ((MASK_SIGN&XP_IN[i])!= 0)  /* check sign bit for neg*/
                {
                    pos=(int)((MASK_IN&(~XP_IN[i]))+1);
                    XP_IN[i]=((-pos)+XMAXneg)&MASK_IN;
                }
            else
                {
                XP_IN[i]=(XP_IN[i]+XMAXneg)&MASK_IN;
                }
        }
    }
BOTTOM:
return((int)x_cnt);
}
/*--------------------------------------------------------------------------- 
    Function: data_output 
---------------------------------------------------------------------------*/
void data_output( uint16_t *decryptData, int *decryptPtr)
{
    int i,pixels;
    int start;
    unsigned short *p,*pend;

    if (ZFLAG)
    {
        pixels=ZBLKS*J;
        ZBLKS=0;
    }
    else
        pixels=J;

    start=0;
    if (REF)
    {
        XP=REFERENCE;
        if (! REF2REF)
            {
            OUT[0]=REFERENCE-XMAXneg;  /* refs are level shifted pixels */
            start=1;
            }
    }
    if ((ENT_CODING)||(BYPASS_mode))
          for (i=0;i<pixels;i++)
            OUT[i] = SIG[i]-XMAXneg;
    else if (NN_mode)
        unmap_NN(start,pixels);
    else if (TWOD_line_mode)
    {
        (void) get_EP_IN(Twod_ptr,pixels);   /* fill block of predictors*/
        unmap_NN(start,pixels);
    }
    else if(EP_mode) /* external Predictor mode */
        unmap_EP(start,pixels);
    else if(TWOD_mode) /* external Predictor mode */
        unmap_TWOD(start,pixels);
    else if(MS_mode)
        unmap_MS(start,pixels);


    if (REF) /* for next ref2ref compression */
    {
        if (!MS_mode)
           REFERENCE=(OUT[0]+XMAXneg)&MASK_IN;/*this restores to orig. ref */
        else
           REFERENCE=(OUT[0]+XMAXneg)-XP_IN[0]+XMAX; /*XPin is level shifted*/
    }
    /*-------- output data -----------*/
    if (BYTE_pixel==1)
    {
        pend= &OUT[pixels-1];
        for (p= &OUT[0];p<=pend;p++)
        {
            if ((MASK_SIGN & *p)== 0)
                *Bptr++ = *p;
            else
                *Bptr++ = *p | MASKN_neg; /* sign extend */
            if (Bptr == Bend) /* dump buffer to output */
            {
              memcpy( &decryptData[*decryptPtr], Buffer_out, sizeof(Buffer_out));

              //              fwrite((void *)Buffer_out,(size_t)sizeof(Buffer_out),(size_t)1,Out_ptr);
              Bptr=Buffer_out;
              *decryptPtr += sizeof(Buffer_out);
            }
        }
    }
    else
    {
        pend= &OUT[pixels-1];
        for (p= &OUT[0];p<=pend;p++)
        {
            if ((MASK_SIGN & *p)== 0)
                *Bptr++ = ((unsigned)(*p) & MASK_IN)>>8;
            else
                *Bptr++ = ((unsigned)(*p) | MASKN_neg)>>8; /* sign extend */
            *Bptr++ = *p;

            if (Bptr == Bend) /* dump buffer to output */
            {
              memcpy( &decryptData[*decryptPtr], Buffer_out, sizeof(Buffer_out));
              //              fwrite((void*)Buffer_out,(size_t)sizeof(Buffer_out),(size_t)1,Out_ptr);
                 Bptr=Buffer_out;
                 *decryptPtr += sizeof(Buffer_out);
            }

        }
    }
    ZFLAG=0;
}
/*--------------------------------------------------------------------------- 
    Function:  flush_buffer 
  Description: called upon EOF flushes output buffer
---------------------------------------------------------------------------*/
void flush_buffer( uint16_t *decryptData, int32_t *decryptPtr)
{
    if (Bptr != Buffer_out)
    {
      memcpy( &decryptData[*decryptPtr], Buffer_out, Bptr-Buffer_out);
      //      fwrite((void*)Buffer_out,(size_t)(Bptr-Buffer_out),(size_t)1,Out_ptr);
      Bptr=Buffer_out;
      *decryptPtr = 0;
    }
}
/*****************************************************************************
                    USER INTERFACE and HELP
*****************************************************************************/
/*--------------------------------------------------------------------
    help()   
*--------------------------------------------------------------------*/
void help()
{
    fprintf(stderr,"================================================================================\n");
    fprintf(stderr,"               Usds:   Universal Source Decoder for Space\n");
    fprintf(stderr,"   %s\n", copyright+5);
    fprintf(stderr,"  UNM, Microelectronics Research Center, Albuquerque, New Mexico 87131 USA\n");
    fprintf(stderr,"%s\n",noncommercial1);
    fprintf(stderr,"%s\n",noncommercial2);
    fprintf(stderr,"===============================================================================\n");
    fprintf(stderr,"%s\n",credits1);
    fprintf(stderr,"%s\n",credits2);
    fprintf(stderr,"%s\n",credits3);
    fprintf(stderr,"================================================================================\n");
    fprintf(stderr,"\nUsds -n <N> -s <scanline_length> [-j <J>][-rr][-help][options] file\n\n");
    fprintf(stderr,"    -n : <N> Followed by quantization in bits per sample. Default is 8.\n");
    fprintf(stderr,"    -s : Followed by Scanline length. (Defines pixels per reference.)\n");
    fprintf(stderr,"         Default Scanline length is 256.\n");
    fprintf(stderr,"    -j : <J> Blocksize. Optional. Default is 16 pixels per block. J=[8,10,16].\n");
    fprintf(stderr,"   -rr : Optional. Reference to Reference differencing. Typically improves\n");
    fprintf(stderr,"         compression by exploiting correlation between ajacent references.\n");
    fprintf(stderr,"         Only supported in nearest neighbor 2D, and 2L modes\n");
    fprintf(stderr," -help : usage is: 'uses -help' to get more options and examples\n");
    fprintf(stderr,"  file : Encoded file name without the '.E' extension\n");
    fprintf(stderr,"         Must decode with same options used for encoding\n");
    fprintf(stderr,"Output : Output is binary file 'file.D'..should be same as original.\n");

    fprintf(stderr,"\nDATA FORMAT options: [-neg][-pos]\n");
    fprintf(stderr,"  -neg : Specify data is two's complement.\n");
    fprintf(stderr,"  -pos : Positive data. This is the default and need not be specified.\n");
    fprintf(stderr,"\nSIMPLE OPERATING MODES: [-nn ][-ec][-by]\n");
    fprintf(stderr,"   -nn : Typical mode. Default. Nearest Neighbor prediction.\n");
    fprintf(stderr,"   -ec : Entropy Coding for positive data. No DPCM, mapping, or references.\n");
    fprintf(stderr,"   -by : Bypass. Packetize input to 16 bits. No coding, no IDs or References.\n");

    fprintf(stderr,"\nOPERATING MODES REQUIRING PREDICTOR FILE: [-2d][-2L][-ep]\n");
    fprintf(stderr,"         These modes require another file the same size as the original\n");
    fprintf(stderr,"         image.  The second binary file contains predictor values that are\n");
    fprintf(stderr,"         used in coding the original image.  The Decoder also needs the\n");
    fprintf(stderr,"         predictor file when decoding in these modes.\n");
    fprintf(stderr,"   -2d : 2D prediction.  Avg of previous X and external prediction.\n");
    fprintf(stderr,"         Predictors must be in binary file 'file.2D'.\n");
    fprintf(stderr,"   -2L : 2D prediction only uses nearest neighbor prediction for first line.\n");
    fprintf(stderr,"         First line of file.2D is ignored to allow nearest neighbor prediction.\n");
    fprintf(stderr,"   -ep : External Prediction. Predictors must be binary file file.EP.\n");
    fprintf(stderr,"   -ms : Multi-spectral option. Predictors must be binary file.MS.\n");
    fprintf(stderr,"\nCHIP SPECIFIC:\n");
    fprintf(stderr,"   -br : <Blocks per reference>. Default is 16 blocks. Typically,\n");
    fprintf(stderr,"         insert a reference every scanline.\n");
    fprintf(stderr,"   -rp : <Refs per packet>.(OPTIONAL). If this arg. is not used, uses assumes\n");
    fprintf(stderr,"         continuous mode til EOF (and ends on a byte). Packets are coded\n");
    fprintf(stderr,"         to a 16 bit boundary, but stats are for 16 and 8 bit conditions.\n");
    fprintf(stderr,"\nDECODER SPECIFIC OPTION:\n");
    fprintf(stderr,"-nosign: Do NOT sign extend output when data is negative.\n");
    fprintf(stderr,"         Sign extending to nearest byte is the default for negative data.\n");
    fprintf(stderr,"================================================================================\n");
    fprintf(stderr,"  NOTE: you must decode using the same options used when encoding.\n");
    fprintf(stderr,"   %s\n", copyright+5);

    fprintf(stderr,"          %s\n",sccs_id+5);
    fprintf(stderr,"================================================================================\n");
}
/*--------------------------------------------------------------------
    set_flags()   
*--------------------------------------------------------------------*/
void set_flags(argc,argv,pixref_flag,blkref_flag)
int argc;
char *argv[];
int *blkref_flag,*pixref_flag;
{
    int tempc;

    if (argc < 2)
    {
        help();
        (void)exit(1);
    }

    *blkref_flag=0;
    *pixref_flag=0;
    BYPASS_flag=0;
    N=N_DEFAULT;
    J=J_DEFAULT;
    if (N <= 8)
        IDbits=3;
    else
        IDbits=4;
    PixRef=PIXREF_DEFAULT;
    BlkRef=(PixRef+J-1)/J;
    BYTE_pixel=1;
    NN_mode=1;
    EP_mode=0;
    SCANLINE=BlkRef*J;
    Kfactor=1;  /* id-Kfactor = ksplit_bits */

    EXT2_flag=1;
    EXT2_bit=0;
    ZOP_flag=1;
    Low_id=0;
    FS_id=1;
    D_id=15;

    argc--;
    tempc=0;
    while (++tempc < argc)
    {
        if (strcmp(argv[tempc],"-help")==0)
        {
            help();
            (void)exit(1);
        }
        else if (strcmp(argv[tempc],"-ec")==0)
        {
            ENT_CODING=1;
            EP_mode=1;
            NN_mode=0; /* no reference */
        }
        else if (strcmp(argv[tempc],"-by")==0)
        {
            BYPASS_mode=1;
            NN_mode=0;
        }
        else if (strcmp(argv[tempc],"-neg")==0)
            NEG_data=1;
        else if (strcmp(argv[tempc],"-pos")==0)
            NEG_data=0;
        else if (strcmp(argv[tempc],"-nn")==0)
        {
            NN_mode=1;
            TWOD_mode=0;
            EP_mode=0;
        }
        else if (strcmp(argv[tempc],"-2d")==0)
        {
            TWOD_mode=1;
            NN_mode=0;
        }
        else if (strcmp(argv[tempc],"-2L")==0)
        {
            TWOD_line_mode=1;
            TWOD_mode=1;
            NN_mode=0;
        }
        else if (strcmp(argv[tempc],"-ep")==0)
        {
            EP_mode=1;
            NN_mode=0;
            BYPASS_flag=0;
        }
        else if (strcmp(argv[tempc],"-ms")==0)
        {
            MS_mode=1;
            NN_mode=0;
        }
        else if (strcmp(argv[tempc],"-n")==0)
        {
            tempc++;
            sscanf(argv[tempc],"%d",&N);
        }
        else if (strcmp(argv[tempc],"-j")==0)
        {
            tempc++;
            sscanf(argv[tempc],"%d",&J);
        }
        else if (strcmp(argv[tempc],"-stats")==0)
             ;  /* do nothing: just allow same flags as is in encoder */
        else if (strcmp(argv[tempc],"-chip")==0)
             ;  /* do nothing: just allow same flags as is in encoder */
        else if (strcmp(argv[tempc],"-pf")==0)
             ;  /* do nothing: just allow same flags as is in encoder */
        else if ((strcmp(argv[tempc],"-refref")==0)||
            (strcmp(argv[tempc],"-rr")==0))
            REFREF_flag=1;
        else if (strcmp(argv[tempc],"-bits")==0)
             ;  /* do nothing: just allow same flags as is in encoder */
        else if ((strcmp(argv[tempc],"-refpac")==0)||
            (strcmp(argv[tempc],"-rp")==0))
        {
            tempc++;
            sscanf(argv[tempc],"%d",&RefPac);
            PACKET_flag=1;
            if (RefPac==0)
        RefPac=128;
        }
        else if ((strcmp(argv[tempc],"-blkref")==0)||
            (strcmp(argv[tempc],"-br")==0))
        {
            tempc++;
            sscanf(argv[tempc],"%d",&BlkRef);
        if (BlkRef==0)
        BlkRef=128;
            *blkref_flag=1;
        }
        else if (strcmp(argv[tempc],"-s")==0)
        {
            tempc++;
            sscanf(argv[tempc],"%d",&PixRef);
            *pixref_flag=1;
        }
        else if ((strcmp(argv[tempc],"-pixref")==0)||
            (strcmp(argv[tempc],"-pr")==0)) /* to be backward compatible */
        {
            tempc++;
            sscanf(argv[tempc],"%d",&PixRef);
            *pixref_flag=1;
        }
        else if ((strcmp(argv[tempc],"-nosign")==0))
            SIGN_EXT_OUTPUT=0;/*NO sign extend output to nearest next byte*/
        else 
        {
          fprintf(stderr,"ERROR: %s is not a valid argument. Aborting\n",argv[tempc]);
          (void)exit(1);
       }
    }/*while*/

    //    open_files(argc,argv);

}
/*****************************************************************************
                   OPEN FILES 
*****************************************************************************/
/*--------------------------------------------------------------------
open_files()
*--------------------------------------------------------------------*/
void open_files(argc,argv)
int argc;
char *argv[];
{
    char out_file[80],ep_file[80],twod_file[80],ms_file[80];

    /* files */
    sprintf(In_file,"%s.E",argv[argc]); /* binary compressed pixel data */
    sprintf(out_file,"%s.D",argv[argc]);/* Decompressed binary file     */
    sprintf(ep_file,"%s.EP",argv[argc]); /* external predictor input file*/
    sprintf(twod_file,"%s.2D",argv[argc]);/* external predictor input file*/
    sprintf(ms_file,"%s.MS",argv[argc]);  /* multispectral pred. file */

    //    if ((In_ptr=fopen(In_file,"rb"))==NULL)
    //{
    //   fprintf(stderr,"ERROR: not able to open file %s.\n",In_file);
    //   (void)exit(1);
    //}

    if ((Out_ptr=fopen(out_file,"wb"))==NULL)
    {
        fprintf(stderr,"ERROR: not able to open file %s.\n",out_file);
        (void)exit(1);
    }
    if (!ENT_CODING)
    {
        if ((EP_mode)&&((Ep_ptr=fopen(ep_file,"rb")) == NULL))
        {
            fprintf(stderr,"ERROR: External predictor file %s not found.\n",ep_file);
            (void)exit(1);
        }
        if ((TWOD_mode)&&((Twod_ptr=fopen(twod_file,"rb")) == NULL))
        {
            fprintf(stderr,"ERROR: External predictor file %s not found.\n",twod_file);
            (void)exit(1);
        }
        if ((MS_mode)&&((Ms_ptr=fopen(ms_file,"rb")) == NULL))
        {
            fprintf(stderr,"ERROR: External predictor file %s not found.\n",
            ms_file);
            (void)exit(1);
        }
    }
}
/*****************************************************************************
                    INITIALIZING 
*****************************************************************************/
/*-------------------------------------------------------------------
initialize()
*--------------------------------------------------------------------*/
void initialize(pixref_flag,blkref_flag)
int pixref_flag,blkref_flag;
{
    int i;

    if ((EP_mode)||(BYPASS_mode))
        REFREF_flag=0;  /* idiot proofing.. inhibit ref2ref differences */

     if (BYPASS_mode)
        {
        IDbits=0;
        ENT_CODING=0;
        NEG_data=0;  
        }
    /*--------------*/
    if (pixref_flag)
       BlkRef=(int)(PixRef+J-1)/J;
    else
       {
       PixRef=BlkRef*J;
       SCANLINE=BlkRef*J;
       }
 
    check_ref_per_sample(blkref_flag,pixref_flag);

    if (N>8 )
    {
        BYTE_pixel=2;
        IDbits=4;
    }
    else 
        D_id=7; /* only change when default is chosen over k=6 */


    /* define MASKING array */
    MASKNOT[0]=0;
    for (i=1;i<=16;i++)
    MASKNOT[i]=(1<<i)-1;

    MASKN_neg=0; /* will be ORed with output */
    if (NEG_data)
    {
        XMAXneg=1<<(N-1);
        MASK_SIGN=(1<<(N-1));
        if (SIGN_EXT_OUTPUT)
              MASKN_neg=(~MASKNOT[N]);
    }
    else
    {
        XMAXneg=0;
        MASK_SIGN=0;
    }
    if (MS_mode)
    {
       NN=N+1;
       if (NN>8)
          {
          IDbits=4;
          D_id=15;
          }
    }
    else
       NN=N;
    XMAX_MS=(1<<NN)-1; 
    XMAX=(1<<N)-1; /* max pos integer before level shifting back to negative*/

    MASK_IN=MASKNOT[N];

    Bptr= &Buffer_out[0];
    Bend= &Buffer_out[0]+sizeof(Buffer_out);
    REF= ((!EP_mode)&&(!BYPASS_mode));
    Binptr= &Buffer_in[0];
    More_data = 1;
    end_fill = 0;
    IN_INDEX = 0;
    NEXT_BIT = 15;
}
/*---------------------------------------------------------------------------
check_ref_per_sample()
---------------------------------------------------------------------------*/
void check_ref_per_sample(blkref_flag,pixref_flag)
int blkref_flag,pixref_flag;
{
    if ((blkref_flag==1)&&(pixref_flag==1))
        {
        fprintf(stderr,"ERROR:%s either blkref or pixref may be specified but not both. Aborting.\n",In_file);
        (void)exit(1);
        }

    SCANLINE=BlkRef*J;
    if (PACKET_flag)
        BlkPac=RefPac*BlkRef;
    /*--------- error checking on samples_per_ref--------------*/
    if (((PixRef % J) !=0)||(PixRef < J))
        fprintf(stderr,"WARNING: %d not multiple of J=%d. Assuming encoder filled out scanline.\n",PixRef,J);
}
