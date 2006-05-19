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
# BUGBUG: Override standard flags, cannot be built without warnings.
#

C_FLAGS=/nologo /W4 /GX /Zi /Od /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /c 

#
# Common information
#

INC=$(INC)

#
# Target specific information
#

TARGET_NAME=EfiCompress
TARGET_SOURCE_DIR = $(TIANO_TOOLS_SOURCE)\$(TARGET_NAME)

TARGET_EXE = $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).exe

TARGET_EXE_SOURCE = "$(TARGET_SOURCE_DIR)\EfiCompressMain.c"
TARGET_EXE_INCLUDE = "$(COMMON_SOURCE)\EfiCompress.h"
TARGET_EXE_LIBS = "$(TIANO_TOOLS_OUTPUT)\Common.lib"

#
# Build targets
#

all: $(TARGET_EXE)

#
# Build EXE
#

$(TIANO_TOOLS_OUTPUT)\EfiCompressMain.obj: $(TARGET_EXE_SOURCE) $(TARGET_EXE_INCLUDE)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_EXE_SOURCE) /Fo$(TIANO_TOOLS_OUTPUT)\EfiCompressMain.obj

$(TARGET_EXE): $(TIANO_TOOLS_OUTPUT)\EfiCompressMain.obj $(TARGET_EXE_LIBS) $(TARGET_DLL)
  $(LINK) $(MSVS_LINK_LIBPATHS) $(L_FLAGS) $(LIBS) /out:$(TARGET_EXE) $(TIANO_TOOLS_OUTPUT)\EfiCompressMain.obj $(TARGET_LIB)  $(TARGET_EXE_LIBS)

clean:
  @if exist $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME)Main.* del /q $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME)Main.* > NUL
