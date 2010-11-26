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
#    makefile
#  
#  Abstract:
#  
#    Makefile for building the EFI VFR compiler
#    
#--*/  


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

.SUFFIXES :

TARGET_NAME       = VfrCompile
ANTLR_H           = $(PCCTS_DIR)\h
C_FLAGS_PCCTS     = -I. -I$(ANTLR_H) /Zi /Fd$(EDK_TOOLS_OUTPUT)\$(TARGET_NAME)Obj /WX /Od /D _CRT_SECURE_NO_DEPRECATE $(VERSION_FLAGS) $(BUILD_STRING_FLAGS)
C_FLAGS_PCCTS     = $(C_FLAGS_PCCTS)  

LINK_FLAGS_PCCTS  =
LIBS              = "$(EDK_TOOLS_OUTPUT)\Common.lib"

# 
# Define the EFI output and source directories.
#
ETO               = $(EDK_TOOLS_OUTPUT)
SRC               = $(EDK_TOOLS_SOURCE)\$(TARGET_NAME)
TARGET_EXE        = $(ETO)\$(TARGET_NAME).exe

#
# Add deeper lookahead with -ck 3
#
ANTLR_FLAGS       = -CC -e3 -ck 3
DLG_FLAGS         = -C2 -i -CC

#
# Define paths for include files
#
INC = -I $(SRC) 
INC = $(INC) -I $(EDK_SOURCE)\Foundation\Include\Ia32
INC = $(INC) -I $(EDK_SOURCE)\Foundation\Efi\Include
INC = $(INC) -I $(EDK_SOURCE)\Foundation\Framework\Include
INC = $(INC) -I $(EDK_SOURCE)\Foundation\Include\IndustryStandard
INC = $(INC) -I $(EDK_SOURCE)\Foundation\         \
             -I $(EDK_SOURCE)\Foundation\Core\Dxe \
             -I $(EDK_SOURCE)\Foundation\Efi      \
             -I $(EDK_SOURCE)\Foundation\Framework
INC = $(INC) -I $(EDK_TOOLS_SOURCE)\Common
INC = $(INC) -I $(EDK_SOURCE)\Foundation\Include 

DLG_FILE          = Parser.dlg
SCAN_FILE         = DLGLexer
PARSER_FILE       = EfiVfrParser

#
# Create a list of include dependencies 
#
INC_DEPS = $(INC_DEPS) $(SRC)\EfiVfr.h
INC_DEPS = $(INC_DEPS) $(EDK_SOURCE)\Foundation\Framework\Include\EfiInternalFormRepresentation.h

#
# This is the grammer file for our project
#
GRAMMER_FILE  = $(SRC)\$(TARGET_NAME).g

#
# If we utilize a separate token file, define it here.
#
#TOKEN_FILE    = 

    
OBJECTS =       $(ETO)\$(TARGET_NAME).obj \
                $(ETO)\$(PARSER_FILE).obj \
                $(ETO)\AParser.obj \
                $(ETO)\DLexerBase.obj \
                $(ETO)\ATokenBuffer.obj \
                $(ETO)\$(SCAN_FILE).obj  \
                $(ETO)\VfrServices.obj

#
# Per the Language Translation Using PCCTS and C++ Reference Guide, page 109,
# these are the outputs of ANTLR and DLG
#                
ANTLR_SPAWN =   $(ETO)\$(TARGET_NAME).cpp \
                $(ETO)\$(PARSER_FILE).cpp \
	              $(ETO)\$(PARSER_FILE).h \
                $(ETO)\$(DLG_FILE) \
                $(ETO)\tokens.h 

DLG_SPAWN =     $(ETO)\$(SCAN_FILE).cpp \
                $(ETO)\$(SCAN_FILE).h


#
# Default target
#
all : $(TARGET_EXE)

#
# All antlr-generated files depend on the .g grammer file. Use the -o
# option to emit them to the appropriate output directory.
#
$(ANTLR_SPAWN) : $(GRAMMER_FILE) $(INC_DEPS)
	$(ANTLR) $(ANTLR_FLAGS) -o $(ETO) $(GRAMMER_FILE)

$(ETO)\$(TARGET_NAME).obj : $(ETO)\$(TARGET_NAME).cpp $(DLG_SPAWN) $(INC_DEPS)
	$(CC) -c $(C_FLAGS_PCCTS) /Fo$@ $(INC) $(ETO)\$(TARGET_NAME).cpp

$(ETO)\$(SCAN_FILE).obj : $(ETO)\$(SCAN_FILE).cpp $(DLG_SPAWN) $(INC_DEPS)
	$(CC) -c $(C_FLAGS_PCCTS) /Fo$@ $(INC) $(ETO)\$(SCAN_FILE).cpp

$(ETO)\$(PARSER_FILE).obj : $(ETO)\$(PARSER_FILE).cpp $(ETO)\$(PARSER_FILE).h $(DLG_SPAWN) $(INC_DEPS)
  $(CC) -c $(C_FLAGS_PCCTS) /Fo$@ $(INC) $(ETO)\$(PARSER_FILE).cpp

$(DLG_SPAWN) : $(ETO)\$(DLG_FILE) $(INC_DEPS)
	$(DLG) $(DLG_FLAGS) -o $(ETO) $(ETO)\$(DLG_FILE)

$(ETO)\AParser.obj : $(ANTLR_H)/AParser.cpp
	$(CC) -c $(C_FLAGS_PCCTS) /Fo$@ $(ANTLR_H)/AParser.cpp

$(ETO)\ATokenBuffer.obj : $(ANTLR_H)/ATokenBuffer.cpp
	$(CC) -c $(C_FLAGS_PCCTS) /Fo$@ $(ANTLR_H)/ATokenBuffer.cpp

$(ETO)\DLexerBase.obj : $(ANTLR_H)/DLexerBase.cpp
	$(CC) -c $(C_FLAGS_PCCTS) /Fo$@ $(ANTLR_H)/DLexerBase.cpp

$(ETO)\VfrServices.obj : $(SRC)\VfrServices.cpp $(SRC)\VfrServices.h $(INC_DEPS)
	$(CC) -c $(C_FLAGS_PCCTS) $(INC) /Fo$@ $(SRC)\VfrServices.cpp

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
	$(LINK) $(MSVS_LINK_LIBPATHS) $(LIBS) /DEBUG /OUT:$(TARGET_EXE) $(LINK_FLAGS_PCCTS) $(OBJECTS) /PDB:$*.pdb
!IF ("$(EFI_BINARY_BUILD)" == "YES")
  if not exist $(EFI_PLATFORM_BIN)\Tools mkdir $(EFI_PLATFORM_BIN)\Tools
  if exist $(TARGET_EXE) copy $(TARGET_EXE) $(EFI_PLATFORM_BIN)\tools\$(TARGET_NAME).exe /Y
  if exist $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb \
  copy $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb /Y
!ENDIF
!ENDIF

clean:
  
