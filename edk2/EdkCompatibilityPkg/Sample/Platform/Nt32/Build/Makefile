#/*++
#
#  Copyright (c) 2004 - 2007, Intel Corporation                                                         
#  All rights reserved. This program and the accompanying materials                          
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
#    This makefile is used to build the NT emulation.
#   
#--*/

#
# Everything depends on EDK_SOURCE & EDK_TOOLS_PATH. Make sure it's defined
#
!IFNDEF EDK_SOURCE
!ERROR EDK_SOURCE environmental variable not set 
!ENDIF

!IFNDEF EDK_TOOLS_PATH
!MESSAGE EDK_TOOLS_PATH environmental variable not set, default setting used
!ENDIF

#
# Set the processor type
#
PROCESSOR = IA32

#
# Define a toolchain so we can pick up the basic defines from
# the EFI tool setup file.
#
TOOLCHAIN = TOOLCHAIN_MSVC

!INCLUDE PlatformTools.env

#
# Default target
#
all : customer

#
# Target to build customer config
#
customer : tempstamp_file build_tools makefiles builds

#
# Target to build everything quickly
#
fast : build_tools fast_makefiles builds

#
# Create a file to store the date and time, and used by SetStamp.exe
#
tempstamp_file:
!IF DEFINED(DATE_STAMP) && DEFINED(TIME_STAMP)
  @echo $(DATE_STAMP) $(TIME_STAMP) > GenStamp.txt
!ELSEIF EXIST(GenStamp.txt)
  - @del GenStamp.txt
!ENDIF

#
# Make all the EFI build tools by calling the makefile
#
build_tools :
  -if not exist $(EDK_TOOLS_OUTPUT) mkdir $(EDK_TOOLS_OUTPUT)
  -if exist $(EDK_TOOLS_BIN)\*.* copy $(EDK_TOOLS_BIN)\*.* $(EDK_TOOLS_OUTPUT)
  $(MAKE) -f $(EDK_TOOLS_SOURCE)\makefile PROCESSOR=$(PROCESSOR) BUILD_DIR=$(BUILD_DIR)
  $(GUIDCHK) -b $(EDK_TOOLS_OUTPUT)\GuidDatabase.txt  

#
# Run ProcessDsc on our main description files to generate the
# makefiles and package files. Specify -v option for more verbose output.
# Specify "-xref filename" to generate a cross-reference file of 
# basename-to-fileguid that can be used by VolInfo.
#
makefiles : 
  $(PROCESSDSC)                                    \
    -d EFI_SOURCE=$(EDK_SOURCE)                    \
    -d EDK_SOURCE=$(EDK_SOURCE)                    \
    -d PROCESSOR=$(PROCESSOR)                      \
    -d PROJECT_NAME=$(PROJECT_NAME)                \
    -d COMPRESS_METHOD=$(COMPRESS_METHOD)          \
    -d OEMTOOLPATH=$(EDK_TOOLS_OUTPUT)             \
    -d SHELL_INF=$(SHELL_INF)                      \
    -d UEFI_PREFIX=$(UEFI_PREFIX)                  \
    -d PI_PREFIX=$(PI_PREFIX)                      \
    -d CAPSULE_INF=$(CAPSULE_INF)                  \
    -d ATAPI_PASS_THRU_INF=$(ATAPI_PASS_THRU_INF)  \
    -d DEVPATH_INF=$(DEVPATH_INF)                  \
    -d GRAPHICS_INF=$(GRAPHICS_INF)                \
    -g $(EDK_TOOLS_OUTPUT)\GuidDatabase.txt        \
    -xref FV\Guid.xref                             \
    $(PROJECT_NAME).dsc
    
#
# Build faster by not building components with FV=NULL. Override in the
# DSC file by adding SKIP_FV_NULL=0 on the component lines you want built
# but still have FV=NULL. For example:
#
# Dxe\Drivers\MyDriver\MyDriver.inf  FV=NULL  SKIP_FV_NULL=0
#
fast_makefiles : 
  $(PROCESSDSC)                                    \
    -d EFI_SOURCE=$(EDK_SOURCE)                    \
    -d EDK_SOURCE=$(EDK_SOURCE)                    \
    -d PROCESSOR=$(PROCESSOR)                      \
    -d PROJECT_NAME=$(PROJECT_NAME)                \
    -d COMPRESS_METHOD=$(COMPRESS_METHOD)          \
    -d OEMTOOLPATH=$(EDK_TOOLS_OUTPUT)             \
    -d SHELL_INF=$(SHELL_INF)                      \
    -d UEFI_PREFIX=$(UEFI_PREFIX)                  \
    -d PI_PREFIX=$(PI_PREFIX)                      \
    -d CAPSULE_INF=$(CAPSULE_INF)                  \
    -d ATAPI_PASS_THRU_INF=$(ATAPI_PASS_THRU_INF)  \
    -d DEVPATH_INF=$(DEVPATH_INF)                  \
    -d GRAPHICS_INF=$(GRAPHICS_INF)                \
    -g $(EDK_TOOLS_OUTPUT)\GuidDatabase.txt        \
    -xref FV\Guid.xref                             \
    -d SKIP_FV_NULL=1                              \
    $(PROJECT_NAME).dsc

#
# Create the guid-to-basename cross reference file of protocols
# used by volinfo for decoding dependency expressions.
#
protocols.xref : 
  -@if not exist fv mkdir FV
  cd $(EDK_SOURCE)
  $(GUIDCHK) -g -u $(BUILD_TARGET_DIRECTORY) -e inf -x > $(BUILD_DIR)\Fv\protocols.xref
  cd $(BUILD_DIR)

#
# Call all the makefiles that were generated by ProcessDsc. ProcessDsc
# creates the makefile.out that calls all the component makefiles.
#
builds :
  $(MAKE) -f makefile.out

#
# Useful targets
#

#
# Define a pseudo-target that skips over compiling the build tools and
# also skips creation of the makefiles for the components. The user
# has to be aware that any changes to INF files renders this target
# invalid.
#
quick : builds

#
# Define a pseudo-target to start building with the libraries or with the 
# components.
#
libraries :
  $(MAKE) -f makefile.out libraries

components :
  $(MAKE) -f makefile.out components

#
# Make a pseudo-target to simplify running of the NT emulation. 
#
!IF "$(EFI_WIN_NT_FILE_SYSTEM)" == ""
EFI_WIN_NT_FILE_SYSTEM = $(EDK_SOURCE)\Other\Maintained\Application\$(UEFI_PREFIX)Shell\bin\ia32\Apps
!ELSE
EFI_WIN_NT_FILE_SYSTEM = $(EFI_WIN_NT_FILE_SYSTEM)!$(EDK_SOURCE)\Other\Maintained\Application\$(UEFI_PREFIX)Shell\bin\ia32\Apps
!ENDIF

#
# Make browse info file
#
bsc:
  - dir /s /b *.sbr > $(PROCESSOR)\temp.rm
  $(BSCMAKE) /n /o $(PROCESSOR)\$(PROJECT_NAME).bsc "@$(PROCESSOR)\temp.rm"

#
# Create a target to clean up all build directories
#
clean :
  - @if exist vc??.* del vc??.* > NUL
  - @if exist *.bak del *.bak > NUL
  - @if exist $(PROCESSOR) rd /s /q $(PROCESSOR) > NUL
  - @if exist ipf rd /s /q ipf > NUL
  - @if exist ebc rd /s /q ebc > NUL
  - @if exist fv rd /s /q fv > NUL
  - @if exist Tools rd /s /q Tools > NUL
  - @if exist makefile.out del makefile.out > NUL
  - @if exist module.* del module.* > NUL
  - @echo All targets cleaned

#
# Clean up all build directories except for tools.
#
cleanbuilds :
  - @if exist *.bak del *.bak > NUL
  - @if exist $(PROCESSOR) rd /s /q $(PROCESSOR) > NUL
  - @if exist ipf rd /s /q ipf > NUL
  - @if exist ebc rd /s /q ebc > NUL
  - @if exist fv rd /s /q fv > NUL
  - @if exist makefile.out del makefile.out > NUL
  - @if exist module.* del module.* > NUL
  - @echo All modules cleaned

#
# Clean up only the tools
#
cleantools :
  - @if exist vc??.* del vc??.* > NUL
  - @if exist Tools rd /s /q Tools > NUL
  - @echo All tools cleaned

#
# Rebuild target, clean and build all
#
rebuild : clean all

#
# cleanall target to clean all build directories.
#
cleanall : clean
