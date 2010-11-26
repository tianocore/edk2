#/*++
#
#  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#  Module Name:
#
#    Makefile
#
#  Abstract:
#
#    Makefile for building the UEFI HiiPack utility
#
#--*/

#
# Make sure environmental variable EDK_SOURCE is set
#
!IFNDEF EDK_SOURCE
!ERROR EDK_SOURCE environmental variable not set
!ENDIF

!INCLUDE $(BUILD_DIR)\PlatformTools.env

#
# Target specific information
#
TARGET_NAME         = HiiPack
TARGET_SOURCE_DIR   = $(EDK_TOOLS_SOURCE)\Uefi$(TARGET_NAME)
TARGET_EXE          = $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).exe

OBJECTS         = $(EDK_TOOLS_OUTPUT)\HiiPack.obj
LIBS            = $(LIBS) $(EDK_TOOLS_OUTPUT)\Common.lib

#
# Build targets
#

all: $(TARGET_EXE)

#
# Compile each tool source file
#
$(EDK_TOOLS_OUTPUT)\HiiPack.obj : $(TARGET_SOURCE_DIR)\HiiPack.c
  $(CC) $(C_FLAGS) $(TARGET_SOURCE_DIR)\HiiPack.c  /Fo$@

#
# Add Binary Build description for this tool.
#

!IF (("$(EFI_BINARY_TOOLS)" == "YES") && EXIST($(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).exe))
$(TARGET_EXE): $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).exe
  copy $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).exe $(TARGET_EXE) /Y
  if exist $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb \
  copy $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb /Y
!ELSE
$(TARGET_EXE) : $(OBJECTS) $(LIBS)
  $(LINK) $(MSVS_LINK_LIBPATHS) $(L_FLAGS) $(LIBS) /out:$(TARGET_EXE) $(OBJECTS)
!IF ("$(EFI_BINARY_BUILD)" == "YES")
  if not exist $(EFI_PLATFORM_BIN)\Tools mkdir $(EFI_PLATFORM_BIN)\Tools
  if exist $(TARGET_EXE) copy $(TARGET_EXE) $(EFI_PLATFORM_BIN)\tools\$(TARGET_NAME).exe /Y
  if exist $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb \
  copy $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb /Y
!ENDIF
!ENDIF
