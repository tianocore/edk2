#/*++
#  
#  Copyright (c) 2001 Intel Corporation.  All rights reserved.
#
#  This software and associated documentation (if any) is furnished under 
#  a license and may only be used or copied in accordance with the terms 
#  of the license.  Except as permitted by such license, no part of this 
#  software or documentation may be reproduced, stored in a retrieval 
#  system, or transmitted in any form or by any means without the express 
#  written consent of Intel Corporation.
#  
#  Module Name:
#  
#    makefile
#    
#  Abstract:
#  
#    makefile for building the PeiRebase utility.
#  
#  Revision History
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

TARGET_NAME         = PeiRebase
TARGET_SRC_DIR      = $(TIANO_TOOLS_SOURCE)\$(TARGET_NAME)
TARGET_EXE          = $(TIANO_TOOLS_OUTPUT)\PeiRebase.exe
TARGET_EXE_LIBS     = $(TIANO_TOOLS_OUTPUT)\Common.lib

#
# Build targets
#

all: $(TARGET_EXE)

OBJECTS   = $(TIANO_TOOLS_OUTPUT)\PeiRebase.obj  

#
# Compile each source file
#
$(TIANO_TOOLS_OUTPUT)\PeiRebase.obj : $(TARGET_SRC_DIR)\PeiRebaseExe.c $(INC_DEPS) $(TARGET_EXE_LIBS)
  $(CC) $(C_FLAGS) $(TARGET_SRC_DIR)\PeiRebaseExe.c /Fo$@

#
# Link the object files together
#
$(TARGET_EXE) : $(OBJECTS) $(TARGET_EXE_LIBS)
  @echo LINKING
  $(LINK) $(MSVS_LINK_LIBPATHS) $(L_FLAGS) $(LIBS) /out:$(TARGET_EXE) $(OBJECTS)  $(TARGET_EXE_LIBS)

clean:
  @if exist $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).* del $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).* > NUL
