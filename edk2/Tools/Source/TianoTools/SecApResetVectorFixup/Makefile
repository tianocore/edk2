#/*++
#   
#  Copyright (c) 2005 Intel Corporation.  All rights reserved.
#
#  This software and associated documentation (if any) is furnished under 
#  a license and may only be used or copied in accordance with the terms 
#  of the license.  Except as permitted by such license, no part of this 
#  software or documentation may be reproduced, stored in a retrieval 
#  system, or transmitted in any form or by any means without the express 
#  written consent of Intel Corporation.
#   
#  Module Name:  makefile
#   
#  Abstract:
#   
#    This file is used to build the EFI utility.
#   
#--*/

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

TARGET_NAME         = SecApResetVectorFixup
TARGET_SOURCE_DIR   = $(TIANO_TOOLS_SOURCE)\$(TARGET_NAME)
TARGET_EXE          = $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).exe
TARGET_EXE_SOURCE   = $(TARGET_SOURCE_DIR)\$(TARGET_NAME).c

#
# Build targets
#

all: $(TARGET_EXE)

OBJECTS   = $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).obj
LIBS      = $(TIANO_TOOLS_OUTPUT)\Common.lib  

#
# Build EXE
#

$(OBJECTS) : $(TARGET_EXE_SOURCE) $(INC_DEPS)
  $(CC) $(C_FLAGS) $(TARGET_EXE_SOURCE) /Fo$@

$(TARGET_EXE): $(OBJECTS)
  $(LINK) $(MSVS_LINK_LIBPATHS) $(L_FLAGS) $(OBJECTS) $(LIBS) /out:$(TARGET_EXE)

clean:
  @if exist $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).* del $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).* > NUL
