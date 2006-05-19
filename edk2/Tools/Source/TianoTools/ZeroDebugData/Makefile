#/*++
#   
#  Copyright (c) 2001 Intel Corporation
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

INC=$(INC)

#
# Target specific information
#

TARGET_NAME=ZeroDebugData
TARGET_SOURCE_DIR = $(TIANO_TOOLS_SOURCE)\$(TARGET_NAME)

TARGET_EXE = $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).exe

TARGET_EXE_SOURCE = "$(TARGET_SOURCE_DIR)\ZeroDebugData.c"
TARGET_EXE_INCLUDE =

#
# Build targets
#

all: $(TARGET_EXE)

#
# Build EXE
#

$(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).obj: $(TARGET_EXE_SOURCE)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_EXE_SOURCE) /Fo$(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).obj

$(TARGET_EXE): $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).obj $(TARGET_EXE_LIBS) $(TARGET_DLL)
  $(LINK) $(MSVS_LINK_LIBPATHS) $(L_FLAGS) $(LIBS) /out:$(TARGET_EXE) $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).obj $(TARGET_LIB)  $(TARGET_EXE_LIBS)

clean:
  @if exist $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).* del $(TIANO_TOOLS_OUTPUT)\$(TARGET_NAME).*
