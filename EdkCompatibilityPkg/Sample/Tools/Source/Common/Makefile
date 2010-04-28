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
TARGET_NAME        = Common
TARGET_SRC_DIR     = $(EDK_TOOLS_SOURCE)\$(TARGET_NAME)
COMMON_SOURCE      = $(EDK_TOOLS_COMMON)

#
# Common information
#

TARGET_LIB = $(EDK_TOOLS_OUTPUT)\Common.lib
TARGET_SOURCE_DIR = $(COMMON_SOURCE)

OBJECTS = "$(EDK_TOOLS_OUTPUT)\ParseInf.obj"      \
          "$(EDK_TOOLS_OUTPUT)\EfiCompress.obj"   \
          "$(EDK_TOOLS_OUTPUT)\TianoCompress.obj"   \
          "$(EDK_TOOLS_OUTPUT)\Decompress.obj"   \
          "$(EDK_TOOLS_OUTPUT)\crc32.obj"   \
          "$(EDK_TOOLS_OUTPUT)\CommonLib.obj"     \
          "$(EDK_TOOLS_OUTPUT)\PeCoffLoader.obj"  \
          "$(EDK_TOOLS_OUTPUT)\PeCoffLoaderEx.obj" \
          "$(EDK_TOOLS_OUTPUT)\FvLib.obj"  \
          "$(EDK_TOOLS_OUTPUT)\EfiUtilityMsgs.obj" \
          "$(EDK_TOOLS_OUTPUT)\SimpleFileParsing.obj" \
          "$(EDK_TOOLS_OUTPUT)\MyAlloc.obj"

#
# Build targets
#

all: $(TARGET_LIB)

#
# Object targets
#
 
"$(EDK_TOOLS_OUTPUT)\ParseInf.obj": "$(TARGET_SOURCE_DIR)\ParseInf.c" "$(TARGET_SOURCE_DIR)\ParseInf.h" $(EDK_SOURCE)\Foundation\Include\TianoCommon.h
  $(CC) $(C_FLAGS) "$(TARGET_SOURCE_DIR)\ParseInf.c" /Fo"$(EDK_TOOLS_OUTPUT)\ParseInf.obj"

"$(EDK_TOOLS_OUTPUT)\MyAlloc.obj": "$(TARGET_SOURCE_DIR)\MyAlloc.c" "$(TARGET_SOURCE_DIR)\MyAlloc.h" $(EDK_SOURCE)\Foundation\Include\TianoCommon.h
  $(CC) $(C_FLAGS) "$(TARGET_SOURCE_DIR)\MyAlloc.c" /Fo"$(EDK_TOOLS_OUTPUT)\MyAlloc.obj"

"$(EDK_TOOLS_OUTPUT)\EfiCompress.obj": "$(TARGET_SOURCE_DIR)\EfiCompress.c" "$(TARGET_SOURCE_DIR)\Compress.h" $(EDK_SOURCE)\Foundation\Include\TianoCommon.h
  $(CC) $(C_FLAGS) "$(TARGET_SOURCE_DIR)\EfiCompress.c" /Fo"$(EDK_TOOLS_OUTPUT)\EfiCompress.obj"

"$(EDK_TOOLS_OUTPUT)\TianoCompress.obj": "$(TARGET_SOURCE_DIR)\TianoCompress.c" "$(TARGET_SOURCE_DIR)\Compress.h" $(EDK_SOURCE)\Foundation\Include\TianoCommon.h
  $(CC) $(C_FLAGS) "$(TARGET_SOURCE_DIR)\TianoCompress.c" /Fo"$(EDK_TOOLS_OUTPUT)\TianoCompress.obj"

"$(EDK_TOOLS_OUTPUT)\Decompress.obj": "$(TARGET_SOURCE_DIR)\Decompress.c" "$(TARGET_SOURCE_DIR)\Decompress.h" $(EDK_SOURCE)\Foundation\Include\TianoCommon.h
  $(CC) $(C_FLAGS) "$(TARGET_SOURCE_DIR)\Decompress.c" /Fo"$(EDK_TOOLS_OUTPUT)\Decompress.obj"

"$(EDK_TOOLS_OUTPUT)\crc32.obj": "$(TARGET_SOURCE_DIR)\crc32.c" "$(TARGET_SOURCE_DIR)\crc32.h" $(EDK_SOURCE)\Foundation\Include\TianoCommon.h
  $(CC) $(C_FLAGS) "$(TARGET_SOURCE_DIR)\crc32.c" /Fo"$(EDK_TOOLS_OUTPUT)\crc32.obj"

"$(EDK_TOOLS_OUTPUT)\CommonLib.obj": "$(TARGET_SOURCE_DIR)\CommonLib.c" "$(TARGET_SOURCE_DIR)\CommonLib.h" $(EDK_SOURCE)\Foundation\Include\TianoCommon.h
  $(CC) $(C_FLAGS) "$(TARGET_SOURCE_DIR)\CommonLib.c" /Fo"$(EDK_TOOLS_OUTPUT)\CommonLib.obj"

"$(EDK_TOOLS_OUTPUT)\PeCoffLoader.obj": "$(EDK_SOURCE)\Foundation\Include\TianoCommon.h" "$(EDK_SOURCE)\Foundation\Library\Pei\PeiLib\$(PROCESSOR)\PeCoffLoaderEx.h" "$(EDK_SOURCE)\Foundation\Library\Pei\PeiLib\PeCoffLoader.c"
#
# This tool is built differently based on the target processor architecture.
# PE32/PE32+ headers are different for IA32 and IPF, so copy the correct file
# to the tools directory and include it in the build.
# Also copy PeCoffLoaderEx.h because it contains the checks for valid image 
# type.
#
  @copy "$(EDK_SOURCE)\Foundation\Include\$(PROCESSOR)\EfiPeOptionalHeader.h" $(EDK_TOOLS_OUTPUT)
  @copy "$(EDK_SOURCE)\Foundation\Library\Pei\PeiLib\$(PROCESSOR)\PeCoffLoaderEx.h" $(EDK_TOOLS_OUTPUT)
  $(CC) -I $(EDK_TOOLS_OUTPUT) $(C_FLAGS) -I "$(EDK_SOURCE)\Foundation\Library\Pei\PeiLib\$(PROCESSOR)" "$(EDK_SOURCE)\Foundation\Library\Pei\PeiLib\PeCoffLoader.c" /Fo"$(EDK_TOOLS_OUTPUT)\PeCoffLoader.obj"

"$(EDK_TOOLS_OUTPUT)\PeCoffLoaderEx.obj": "$(EDK_SOURCE)\Foundation\Library\Pei\PeiLib\$(PROCESSOR)\PeCoffLoaderEx.c"
  $(CC) $(C_FLAGS) "$(EDK_SOURCE)\Foundation\Library\Pei\PeiLib\$(PROCESSOR)\PeCoffLoaderEx.c" /Fo"$(EDK_TOOLS_OUTPUT)\PeCoffLoaderEx.obj"

"$(EDK_TOOLS_OUTPUT)\FvLib.obj": "$(TARGET_SOURCE_DIR)\FvLib.c" "$(TARGET_SOURCE_DIR)\FvLib.h" $(EDK_SOURCE)\Foundation\Framework\Include\EfiFirmwareFileSystem.h "$(EDK_SOURCE)\Foundation\Framework\Include\EfiFirmwareVolumeHeader.h"
  $(CC) $(C_FLAGS) "$(TARGET_SOURCE_DIR)\FvLib.c" /Fo"$(EDK_TOOLS_OUTPUT)\FvLib.obj"

"$(EDK_TOOLS_OUTPUT)\EfiUtilityMsgs.obj": "$(TARGET_SOURCE_DIR)\EfiUtilityMsgs.c" "$(TARGET_SOURCE_DIR)\EfiUtilityMsgs.h"
  $(CC) $(C_FLAGS) "$(TARGET_SOURCE_DIR)\EfiUtilityMsgs.c" /Fo"$(EDK_TOOLS_OUTPUT)\EfiUtilityMsgs.obj"

"$(EDK_TOOLS_OUTPUT)\SimpleFileParsing.obj" : "$(TARGET_SOURCE_DIR)\SimpleFileParsing.c" "$(TARGET_SOURCE_DIR)\SimpleFileParsing.h"
  $(CC) $(C_FLAGS) "$(TARGET_SOURCE_DIR)\SimpleFileParsing.c" /Fo"$(EDK_TOOLS_OUTPUT)\SimpleFileParsing.obj"

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
  @if exist $(EDK_TOOLS_OUTPUT)\ParseInf.* del /q $(EDK_TOOLS_OUTPUT)\ParseInf.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\ParsePeim.* del /q $(EDK_TOOLS_OUTPUT)\ParsePeim.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\EfiCompress.* del /q $(EDK_TOOLS_OUTPUT)\EfiCompress.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\TianoCompress.* del /q $(EDK_TOOLS_OUTPUT)\TianoCompress.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\Decompress.* del /q $(EDK_TOOLS_OUTPUT)\Decompress.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\crc32.* del /q $(EDK_TOOLS_OUTPUT)\crc32.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\CommonLib.* del /q $(EDK_TOOLS_OUTPUT)\CommonLib.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\PeCoffLoader.* del /q $(EDK_TOOLS_OUTPUT)\PeCoffLoader.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\PeCoffLoaderEx.* del /q $(EDK_TOOLS_OUTPUT)\PeCoffLoaderEx.* > NUL
  @if exist $(EDK_TOOLS_OUTPUT)\FvLib.* del /q $(EDK_TOOLS_OUTPUT)\FvLib.* > NUL
  @if exist $(TARGET_LIB) del $(TARGET_LIB)
