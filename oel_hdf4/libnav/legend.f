      SUBROUTINE LEGEND(N,M,X,P)
C  VERSION OF 2/9/87
C  PURPOSE   
C    COMPUTES LEGENDRE AND ASSOCIATED LEGENDRE FUNCTIONS UP TO 
C    DEGREE N AND ORDER M, N.GE.M  
C  INPUT 
C    N      = DEGREE OF SPHERICAL HARMONICS
C    M      = ORDER OF SPHERICAL HARMONICS 
C    X      = SIN (LATITUDE)   
C  OUTPUT
C    P      = 2-D LEGENDRE FUNCTIONS   
C  CALLED BY SUBROUTINES 
C    DER   
C  CALL SUBROUTINES  
C    NONE  
C  REFERENCES
C    JPL EM 312/87-153, 20 APRIL 1987
C  ANALYSIS
C    J. H. KWOK - JPL  
C  PROGRAMMER
C    J. H. KWOK - JPL  
C  PROGRAM MODIFICATION  
C    NONE  
C  COMMENTS  
C    THIS PROGRAM HAS BEEN DIMENSION FOR MAXIMUM OF 40X40 HARMONICS.   
C    FOR HIGHER ORDER OR DEGREE, REDIMENSION P( , ). THE FUNCTIONS 
C    ARE STORED AS PIJ=P(I+1,J+1).  INPUT N CAN BE .GE. M. 
C   
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)   
      DIMENSION P(41,41)
      DATA ONE/1.D0/
      P(1,1)=ONE
      P(2,1)=X
C *** COMPUTE LEGENDRE FUNCTIONS UP TO DEGREE N 
      DO 100 I=2,N  
  100 P(I+1,1)=((2*I-1)*X*P(I,1)-(I-1)*P(I-1,1))/I  
      IF (M.EQ.0) RETURN
C *** COMPUTE ASSOCIATED LEGENDRE FUNCTIONS UP TO ORDER M   
      Y=DSQRT(ONE-X*X) 
      P(2,2)=Y  
      IF (M.EQ.1) GO TO 400 
C *** THIS IS THE SECTORIAL PART OF THE ASSOCIATED FUNCTIONS
      DO 200 I=2,M  
  200 P(I+1,I+1)=(2*I-1)*Y*P(I,I)   
  400 CONTINUE  
C *** THIS THE TESSERAL PART OF THE ASSOCIATED FUNCTIONS
      DO 500 I=2,N  
      I1=I-1
      DO 300 J=1,I1 
      IF (J.GT.M) GO TO 500 
      P(I+1,J+1)=(2*I-1)*Y*P(I,J)   
      IF (I-2.GE.J) P(I+1,J+1)=P(I+1,J+1)+P(I-1,J+1)
  300 CONTINUE  
  500 CONTINUE  
      RETURN
      END