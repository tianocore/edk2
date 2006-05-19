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
#    makefile for building the EfiRom utility.
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
# Define the toolchain which is used to set build options and toolchain paths
#
TOOLCHAIN = TOOLCHAIN_MSVC

!INCLUDE PlatformTools.env

#
# Target specific information
#

TARGET_NAME         = EfiRom
TARGET_SRC_DIR      = $(TIANO_TOOLS_SOURCE)\$(TARGET_NAME)
TARGET_EXE          = $(TIANO_TOOLS_OUTPUT)\EfiRom.exe

#
# Build targets
#

all: $(TARGET_EXE)

OBJECTS   = $(TIANO_TOOLS_OUTPUT)\EfiRom.obj \
            $(TIANO_TOOLS_OUTPUT)\EfiCompress.obj
            
#
# Build the EXE by compiling the source files, then linking the resultant
# object files together.
#

$(TIANO_TOOLS_OUTPUT)\EfiRom.obj : $(TARGET_SRC_DIR)\EfiRom.c
  $(CC) $(C_FLAGS) $(TARGET_SRC_DIR)\EfiRom.c /Fo$@

$(TIANO_TOOLS_OUTPUT)\EfiCompress.obj : $(EDK_TOOLS_SOURCE)\Common\EfiCompress.c
  $(CC) $(C_FLAGS) $(EDK_TOOLS_SOURCE)\Common\EfiCompress.c /Fo$@
  
$(TARGET_EXE): $(OBJECTS) $(TARGET_EXE_LIBS)
  $(LINK) $(MSVS_LINK_LIBPATHS) $(L_FLAGS) $(LIBS) /out:$(TARGET_EXE) $(OBJECTS)

clean:
  @if exist $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).* del $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).* > NUL
