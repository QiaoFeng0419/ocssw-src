# ===========================================================================
#	Begin Program Specific Block
# ===========================================================================

MAKEFILE = $(EXE).mk

# Progam to make
EXE	= l0merge_seawifs

# Source files for EXE
SRC	= main_l0merge.c getl0indx.c printindx.c swl0_utils.c

# Object modules for EXE
OBJ	= $(SRC:.c=.o)

# Include file locations
INCLUDE	= -I$(INCDIR)/utils -I$(INCDIR)/swfinc -I$(LIB3_INC)

# Library locations
LIBS 	= -L$(LIBDIR) -L$(LIB3_LIB)

# Libraries required to link
LLIBS 	= -lgenutils

include $(MAKEFILE_APP_TEMPLATE)
