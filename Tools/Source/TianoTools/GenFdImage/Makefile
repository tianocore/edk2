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
# Define some macros we use here. Should get rid of them someday and 
# get rid of the extra level of indirection.
#
COMMON_SOURCE      = $(EDK_TOOLS_COMMON)

#
# Common information
#

INC=$(INC) \
    -I "$(TIANO_TOOLS_SOURCE)\GenFvImage"

#
# Target specific information
#

TARGET_NAME=GenFdImage
TARGET_SOURCE_DIR = $(TIANO_TOOLS_SOURCE)\$(TARGET_NAME)

TARGET_LIB = $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).lib
TARGET_EXE = $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).exe

TARGET_EXE_SOURCE = "$(TARGET_SOURCE_DIR)\GenFdImageExe.c"
TARGET_EXE_INCLUDE = "$(TARGET_SOURCE_DIR)\GenFdImageExe.h" \
                     "$(TARGET_SOURCE_DIR)\GenFdImage.h" \
                     "$(COMMON_SOURCE)\ParseInf.h" \
                     "$(EDK_SOURCE)\Foundation\Include\TianoCommon.h"
TARGET_EXE_LIBS = "$(TIANO_TOOLS_OUTPUT)\Common.lib" \
                  "$(TIANO_TOOLS_OUTPUT)\GenFvImage.lib" \
                  "$(TIANO_TOOLS_OUTPUT)\PeimFixup.lib"

TARGET_LIB_SOURCE = "$(TARGET_SOURCE_DIR)\GenFdImageLib.c"
TARGET_LIB_INCLUDE = "$(TARGET_SOURCE_DIR)\GenFdImage.h" \
                     "$(TIANO_TOOLS_SOURCE)\GenFvImage\GenFvImage.h" \
                     "$(COMMON_SOURCE)\ParseInf.h" \
                     "$(EDK_SOURCE)\Foundation\Include\TianoCommon.h"
TARGET_LIB_LIBS = "$(TIANO_TOOLS_OUTPUT)\Common.lib" \
                  "$(TIANO_TOOLS_OUTPUT)\GenFvImage.lib" \
                  "$(TIANO_TOOLS_OUTPUT)\PeimFixup.lib"


#
# Build targets
#

all: $(TARGET_LIB) $(TARGET_EXE)

#
# Build EXE
#

$(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).obj: $(TARGET_EXE_SOURCE) $(TARGET_EXE_INCLUDE)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_EXE_SOURCE) /Fo$(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).obj

$(TARGET_EXE): $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).obj $(TARGET_EXE_LIBS) $(TARGET_LIB)
  $(LINK) $(MSVS_LINK_LIBPATHS) $(L_FLAGS) $(LIBS) /out:$(TARGET_EXE) $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).obj $(TARGET_LIB)  $(TARGET_EXE_LIBS)

#
# Build LIB
#

$(TARGET_LIB): $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME)Lib.obj $(TARGET_LIB_LIBS)
  $(LIB) $(LIB_FLAGS) $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME)Lib.obj /OUT:$(TARGET_LIB)

$(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME)Lib.obj:  $(TARGET_LIB_SOURCE) $(TARGET_LIB_INCLUDE)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_LIB_SOURCE) /Fo$(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME)Lib.obj


clean:
  @if exist $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME)Lib.* del /q $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME)Lib.* > NUL
  @if exist $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).* del /q $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).* > NUL
