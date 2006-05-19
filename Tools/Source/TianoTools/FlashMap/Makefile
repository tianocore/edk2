#/*++
#  
#  Copyright (c) 2004 Intel Corporation. All rights reserved
#  This software and associated documentation (if any) is furnished
#  under a license and may only be used or copied in accordance
#  with the terms of the license. Except as permitted by such
#  license, no part of this software or documentation may be
#  reproduced, stored in a retrieval system, or transmitted in any
#  form or by any means without the express written consent of
#  Intel Corporation.
#  
#  
#  Module Name:
#  
#    Makefile
#  
#  Abstract:
#  
#    makefile for building the FlashMap utility
#    
#--*/

#
# Make sure environmental variable EFI_SOURCE is set
#
!IFNDEF EFI_SOURCE
!ERROR EFI_SOURCE environmental variable not set 
!ENDIF


!INCLUDE PlatformTools.env

INCLUDE_PATHS = -I $(TIANO_TOOLS_SOURCE)\Common

#
# Target specific information
#
TARGET_NAME         = FlashMap
TARGET_SRC_DIR      = $(TIANO_TOOLS_SOURCE)\$(TARGET_NAME)
TARGET_EXE          = $(TIANO_TOOLS_OUTPUT)\FlashMap.exe
LIBS                = $(LIBS) "$(TIANO_TOOLS_OUTPUT)\Common.lib"

#
# Build targets
#
all: $(TARGET_EXE)

OBJECTS   = $(TIANO_TOOLS_OUTPUT)\FlashMap.obj  \
            $(TIANO_TOOLS_OUTPUT)\FlashDefFile.obj \
            $(TIANO_TOOLS_OUTPUT)\Symbols.obj \
            $(TIANO_TOOLS_OUTPUT)\Microcode.obj 
            
#            $(TIANO_TOOLS_OUTPUT)\TrackMallocFree.obj 
 
#C_FLAGS  = $(C_FLAGS) /D TRACK_MALLOC_FREE
C_FLAGS   = $(C_FLAGS) /D _malloc=malloc /D _free=free
           
#
# Compile each source file
#
$(TIANO_TOOLS_OUTPUT)\FlashMap.obj : $(TARGET_SRC_DIR)\FlashMap.c $(TARGET_SRC_DIR)\Symbols.h $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INCLUDE_PATHS) $(TARGET_SRC_DIR)\FlashMap.c /Fo$@

$(TIANO_TOOLS_OUTPUT)\Symbols.obj : $(TARGET_SRC_DIR)\Symbols.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INCLUDE_PATHS) $(TARGET_SRC_DIR)\Symbols.c /Fo$@

$(TIANO_TOOLS_OUTPUT)\Microcode.obj : $(TARGET_SRC_DIR)\Microcode.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_SRC_DIR)\Microcode.c /Fo$@

$(TIANO_TOOLS_OUTPUT)\FlashDefFile.obj : $(TARGET_SRC_DIR)\FlashDefFile.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_SRC_DIR)\FlashDefFile.c /Fo$@

$(TIANO_TOOLS_OUTPUT)\TrackMallocFree.obj : $(TARGET_SRC_DIR)\TrackMallocFree.c $(INC_DEPS)
  $(CC) $(C_FLAGS) $(INC) $(TARGET_SRC_DIR)\TrackMallocFree.c /Fo$@

#
# Link the object files together to create the final executable
#
$(TARGET_EXE) : $(OBJECTS) $(LIBS)
  $(LINK) $(MSVS_LINK_LIBPATHS) $(L_FLAGS) $(LIBS) /out:$(TARGET_EXE) $(OBJECTS)

