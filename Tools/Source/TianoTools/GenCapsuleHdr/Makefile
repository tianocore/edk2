#/*++
#  
#  Copyright (c)  2002 Intel Corporation. All rights reserved
#  This software and associated documentation (if any) is furnished
#  under a license and may only be used or copied in accordance
#  with the terms of the license. Except as permitted by such
#  license, no part of this software or documentation may be
#  reproduced, stored in a retrieval system, or transmitted in any
#  form or by any means without the express written consent of
#  Intel Corporation.
#  
#  
#  Module Name:
#  
#    Makefile
#  
#  Abstract:
#  
#    makefile for the GenCapsuleHdr utility.
#    
#--*/  

#
# Make sure environmental variable EFI_SOURCE is set
#
!IFNDEF EFI_SOURCE
!ERROR EFI_SOURCE environmental variable not set 
!ENDIF

#
# Do this if you want to compile from this directory
#
!IFNDEF TOOLCHAIN
TOOLCHAIN = TOOLCHAIN_MSVC
!ENDIF

!INCLUDE PlatformTools.env

#
# Target specific information
#


TARGET_NAME         = GenCapsuleHdr
TARGET_EXE          = $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).exe
SRC                 = $(TIANO_TOOLS_SOURCE)\$(TARGET_NAME)
ETO                 = $(TIANO_TOOLS_OUTPUT)

#
# Build targets
#

all: $(TARGET_EXE)

LIBS    = $(LIBS) "$(TIANO_TOOLS_OUTPUT)\Common.lib" ole32.lib

OBJECTS = $(ETO)\$(TARGET_NAME).obj \
          $(ETO)\CreateGuid.obj

#
# Compile each source file
#
$(ETO)\$(TARGET_NAME).obj : $(SRC)\$(TARGET_NAME).c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(SRC)\$(TARGET_NAME).c /Fo$@

$(ETO)\CreateGuid.obj : $(SRC)\CreateGuid.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(SRC)\CreateGuid.c /Fo$@

#
# Link the object files together
#
$(TARGET_EXE) : $(OBJECTS)
  $(LINK) $(MSVS_LINK_LIBPATHS) $(L_FLAGS) $(LIBS) /out:$(TARGET_EXE) $(OBJECTS)

clean:
