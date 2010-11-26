#/*++
#  
#  Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
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
#    makefile for building the ProcessDsc utility.
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

TARGET_NAME         = ProcessDsc
TARGET_SRC_DIR      = $(EDK_TOOLS_SOURCE)\$(TARGET_NAME)
TARGET_EXE          = $(EDK_TOOLS_OUTPUT)\ProcessDsc.exe

#
# Build targets
#

all: $(TARGET_EXE)

INC_DEPS  = $(TARGET_SRC_DIR)\DSCFile.h     $(INC_DEPS)
INC_DEPS  = $(TARGET_SRC_DIR)\MultiThread.h $(INC_DEPS)
INC_DEPS  = $(TARGET_SRC_DIR)\FWVolume.h    $(INC_DEPS)
INC_DEPS  = $(TARGET_SRC_DIR)\Exceptions.h  $(INC_DEPS)
INC_DEPS  = $(TARGET_SRC_DIR)\Common.h      $(INC_DEPS)

LIBS      = $(LIBS) "$(EDK_TOOLS_OUTPUT)\Common.lib"

OBJECTS   = $(EDK_TOOLS_OUTPUT)\DSCFile.obj     \
            $(EDK_TOOLS_OUTPUT)\MultiThread.obj \
            $(EDK_TOOLS_OUTPUT)\FWVolume.obj    \
            $(EDK_TOOLS_OUTPUT)\ProcessDsc.obj  \
            $(EDK_TOOLS_OUTPUT)\Exceptions.obj

#
# Compile each source file
#
C_FLAGS   = $(C_FLAGS) /MT /wd4201

$(EDK_TOOLS_OUTPUT)\DSCFile.obj : $(TARGET_SRC_DIR)\DSCFile.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(TARGET_SRC_DIR)\DSCFile.c /Fo$@

$(EDK_TOOLS_OUTPUT)\MultiThread.obj : $(TARGET_SRC_DIR)\MultiThread.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(TARGET_SRC_DIR)\MultiThread.c /Fo$@

$(EDK_TOOLS_OUTPUT)\FWVolume.obj : $(TARGET_SRC_DIR)\FWVolume.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(TARGET_SRC_DIR)\FWVolume.c /Fo$@

$(EDK_TOOLS_OUTPUT)\ProcessDsc.obj : $(TARGET_SRC_DIR)\ProcessDsc.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(TARGET_SRC_DIR)\ProcessDsc.c /Fo$@

$(EDK_TOOLS_OUTPUT)\Exceptions.obj : $(TARGET_SRC_DIR)\Exceptions.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(TARGET_SRC_DIR)\Exceptions.c /Fo$@

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
  $(LINK) $(MSVS_LINK_LIBPATHS) $(L_FLAGS) $(LIBS) /out:$(TARGET_EXE) $(OBJECTS) shlwapi.lib \
  /NODEFAULTLIB:libc.lib  /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcd.lib \
  /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib
!IF ("$(EFI_BINARY_BUILD)" == "YES")
  if not exist $(EFI_PLATFORM_BIN)\Tools mkdir $(EFI_PLATFORM_BIN)\Tools
  if exist $(TARGET_EXE) copy $(TARGET_EXE) $(EFI_PLATFORM_BIN)\tools\$(TARGET_NAME).exe /Y
  if exist $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb \
  copy $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb /Y
!ENDIF
!ENDIF

clean:
  @if exist $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).* del $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\DscFile.* del $(EDK_TOOLS_OUTPUT)\DscFile.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\MultiThread.* del $(EDK_TOOLS_OUTPUT)\MultiThread.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\Exceptions* del $(EDK_TOOLS_OUTPUT)\Exceptions.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\FwVolume.* del $(EDK_TOOLS_OUTPUT)\FwVolume.* > NUL
