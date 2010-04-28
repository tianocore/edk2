#/*++
#
#  Copyright (c) 2002 - 2007, Intel Corporation. All rights reserved.<BR>
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
#    makefile
#
#  Abstract:
#
#    makefile for building the GenTEImage utility.
#
#--*/

#
# Make sure environmental variable EDK_SOURCE is set
#
!IFNDEF EDK_SOURCE
!ERROR EDK_SOURCE environmental variable not set 
!ENDIF

#
# Define the toolchain which is used to set build options and toolchain paths
#
TOOLCHAIN = TOOLCHAIN_MSVC

!INCLUDE $(BUILD_DIR)\PlatformTools.env

#
# Target specific information
#

TARGET_NAME         = GenTEImage
TARGET_SRC_DIR      = $(EDK_TOOLS_SOURCE)\$(TARGET_NAME)
TARGET_EXE          = $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).exe

#
# Build targets
#

all: $(TARGET_EXE)

OBJECTS   = $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).obj 

LIBS      = $(EDK_TOOLS_OUTPUT)\Common.lib

INC_DEPS  = $(EDK_SOURCE)\Foundation\Efi\Include\EfiImage.h
          
#
# Build the EXE by compiling the source files, then linking the resultant
# object files together.
#

$(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).obj : $(TARGET_SRC_DIR)\$(TARGET_NAME).c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(TARGET_SRC_DIR)\$(TARGET_NAME).c /Fo$@

#
# Add Binary Build description for this tools.
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

clean:
  @if exist $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).* del $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).* > NUL
