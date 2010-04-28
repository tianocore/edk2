#/*++
#   
#  Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
#  This program and the accompanying materials                          
#  are licensed and made available under the terms and conditions of the BSD License         
#  which accompanies this distribution.  The full text of the license may be found at        
#  http://opensource.org/licenses/bsd-license.php                                            
#                                                                                            
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
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

!INCLUDE $(BUILD_DIR)\PlatformTools.env

#
# Define some macros we use here. Should get rid of them someday and 
# get rid of the extra level of indirection.
#
TARGET_NAME        = CustomizedCompress
TARGET_SOURCE_DIR  = $(EDK_TOOLS_SOURCE)\$(TARGET_NAME)
COMMON_SOURCE      = $(EDK_TOOLS_COMMON)

#
# Common information
#

TARGET_LIB = $(EDK_TOOLS_OUTPUT)\CustomizedCompress.lib

OBJECTS = "$(EDK_TOOLS_OUTPUT)\CustomizedCompress.obj" 

#
# Build targets
#

all: $(TARGET_LIB)

#
# Object targets
#
 
"$(EDK_TOOLS_OUTPUT)\CustomizedCompress.obj": "$(TARGET_SOURCE_DIR)\CustomizedCompress.c" $(EDK_SOURCE)\Foundation\Include\EfiCommon.h
  $(CC) $(C_FLAGS) "$(TARGET_SOURCE_DIR)\CustomizedCompress.c" /Fo"$(EDK_TOOLS_OUTPUT)\CustomizedCompress.obj"

#
# Build LIB
#

#
# Add Binary Build description for this lib.
#

!IF (("$(EFI_BINARY_TOOLS)" == "YES") && EXIST($(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).lib))
$(TARGET_LIB): $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).lib
  copy $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).lib $(TARGET_LIB) /Y
  if exist $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME)Obj.pdb \
  copy $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME)Obj.pdb $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME)Obj.pdb /Y
!ELSE
$(TARGET_LIB): $(OBJECTS)
  $(LIB_EXE) $(LIB_FLAGS) $(OBJECTS) /OUT:$(TARGET_LIB)
!IF ("$(EFI_BINARY_BUILD)" == "YES")
  if not exist $(EFI_PLATFORM_BIN)\Tools mkdir $(EFI_PLATFORM_BIN)\Tools
  if exist $(TARGET_LIB) copy $(TARGET_LIB) $(EFI_PLATFORM_BIN)\tools\$(TARGET_NAME).lib /Y
  if exist $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME)Obj.pdb \
  copy $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME)Obj.pdb $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME)Obj.pdb /Y
!ENDIF
!ENDIF

clean:
  @if exist $(EDK_TOOLS_OUTPUT)\CustomizedCompress.* del /q $(EDK_TOOLS_OUTPUT)\CustomizedCompress.* > NUL
  @if exist $(TARGET_LIB) del $(TARGET_LIB)
