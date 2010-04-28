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
#  Module Name:
#  
#    Makefile
#  
#  Abstract:
#  
#    makefile for building the GUID check utility.
#    
#--*/

#
# Make sure environmental variable EDK_SOURCE is set
#
!IFNDEF EDK_SOURCE
!ERROR EDK_SOURCE environmental variable not set 
!ENDIF

#
# Do this if you want to compile from this directory
#
!IFNDEF TOOLCHAIN
TOOLCHAIN = TOOLCHAIN_MSVC
!ENDIF

!INCLUDE $(BUILD_DIR)\PlatformTools.env

#
# Target specific information
#

TARGET_NAME         = GuidChk
TARGET_SRC_DIR      = $(EDK_TOOLS_SOURCE)\$(TARGET_NAME)
TARGET_EXE          = $(EDK_TOOLS_OUTPUT)\GuidChk.exe

#LIBS                = $(LIBS) "$(EDK_TOOLS_OUTPUT)\Common.lib"

#
# Build targets
#
all: $(TARGET_EXE)

INC_DEPS  = $(TARGET_SRC_DIR)\FileSearch.h    $(INC_DEPS)
#INC_DEPS  = $(TARGET_SRC_DIR)\CommonUtils.h   $(INC_DEPS)
#INC_DEPS  = $(TARGET_SRC_DIR)\UtilsMsgs.h     $(INC_DEPS)

OBJECTS   = $(EDK_TOOLS_OUTPUT)\GuidChk.obj  \
            $(EDK_TOOLS_OUTPUT)\FileSearch.obj \
            $(EDK_TOOLS_OUTPUT)\GuidList.obj \
            $(EDK_TOOLS_OUTPUT)\UtilsMsgs.obj
#
# Compile each source file
#
$(EDK_TOOLS_OUTPUT)\GuidChk.obj : $(TARGET_SRC_DIR)\GuidChk.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_SRC_DIR)\GuidChk.c /Fo$@

$(EDK_TOOLS_OUTPUT)\FileSearch.obj : $(TARGET_SRC_DIR)\FileSearch.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_SRC_DIR)\FileSearch.c /Fo$@

$(EDK_TOOLS_OUTPUT)\GuidList.obj : $(TARGET_SRC_DIR)\GuidList.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_SRC_DIR)\GuidList.c /Fo$@

$(EDK_TOOLS_OUTPUT)\UtilsMsgs.obj : $(TARGET_SRC_DIR)\UtilsMsgs.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_SRC_DIR)\UtilsMsgs.c /Fo$@

#
# Add Binary Build description for this tool.
#

!IF (("$(EFI_BINARY_TOOLS)" == "YES") && EXIST($(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).exe))
$(TARGET_EXE): $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).exe
  copy $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).exe $(TARGET_EXE) /Y
  if exist $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb \
  copy $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb /Y
!ELSE
$(TARGET_EXE) : $(OBJECTS)
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
  @if exist $(EDK_TOOLS_OUTPUT)\FileSearch.* del $(EDK_TOOLS_OUTPUT)\FileSearch.* > NUL
