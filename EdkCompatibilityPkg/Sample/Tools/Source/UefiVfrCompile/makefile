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

!IFNDEF TOOLCHAIN
TOOLCHAIN = TOOLCHAIN_MSVC
!ENDIF

!INCLUDE $(BUILD_DIR)\PlatformTools.env

.SUFFIXES :

TARGET_NAME        = VfrCompile

ETO                = $(EDK_TOOLS_OUTPUT)
SRC                = $(EDK_TOOLS_SOURCE)\Uefi$(TARGET_NAME)
TARGET_EXE         = $(ETO)\$(TARGET_NAME).exe



INC                = -I $(SRC) \
                     -I $(EDK_SOURCE)\Foundation\Include\Ia32 \
                     -I $(EDK_SOURCE)\Foundation\Efi\Include \
                     -I $(EDK_SOURCE)\Foundation\Framework\Include \
                     -I $(EDK_SOURCE)\Foundation\Include\IndustryStandard \
                     -I $(EDK_SOURCE)\Foundation\         \
                     -I $(EDK_SOURCE)\Foundation\Core\Dxe \
                     -I $(EDK_SOURCE)\Foundation\Efi      \
                     -I $(EDK_SOURCE)\Foundation\Framework \
                     -I $(EDK_TOOLS_SOURCE)\Common \
                     -I $(EDK_SOURCE)\Foundation\Include

ANTLR_FLAGS        = -CC -e3 -ck 3 -k 2
DLG_FLAGS          = -C2 -i -CC
LINK_FLAGS_PCCTS   = /DEBUG  /PDB:$*.pdb
C_FLAGS_PCCTS      = -I. -I$(ANTLR_H) /WX /Od /EHsc /Zi /Fd$(ETO)\$(TARGET_NAME)Obj /D _CRT_SECURE_NO_DEPRECATE $(VERSION_FLAGS)  \
                     $(BUILD_STRING_FLAGS) /D PCCTS_USE_NAMESPACE_STD #/D CVFR_VARDATATYPEDB_DEBUG /D CIFROBJ_DEUBG /D VFREXP_DEBUG

VFR_GRAMMER_FILE   = $(SRC)\VfrSyntax.g

VFR_ANTLR_SPAWN    = $(ETO)\VfrSyntax.cpp \
                     $(ETO)\EfiVfrParser.cpp \
                     $(ETO)\EfiVfrParser.h \
                     $(ETO)\VfrParser.dlg \
                     $(ETO)\VfrTokens.h

VFR_DLG_SPAWN      = $(ETO)\VfrLexer.cpp \
                     $(ETO)\VfrLexer.h

ANTLR_H            = $(PCCTS_DIR)\h

HEADER_FILES       = $(SRC)\VfrFormPkg.h \
                     $(SRC)\EfiVfr.h \
                     $(SRC)\VfrCompiler.h \
                     $(SRC)\VfrError.h \
                     $(SRC)\VfrUtilityLib.h

OBJECTS            = $(ETO)\AParser.obj \
                     $(ETO)\DLexerBase.obj \
                     $(ETO)\ATokenBuffer.obj \
                     $(ETO)\VfrSyntax.obj \
                     $(ETO)\EfiVfrParser.obj \
                     $(ETO)\VfrLexer.obj  \
                     $(ETO)\VfrFormPkg.obj \
                     $(ETO)\VfrError.obj \
                     $(ETO)\VfrUtilityLib.obj \
                     $(ETO)\VfrCompiler.obj

all : $(TARGET_EXE)

$(VFR_ANTLR_SPAWN) : $(VFR_GRAMMER_FILE)
    $(ANTLR) $(ANTLR_FLAGS) -fl VfrParser.dlg -ft VfrTokens.h -o $(ETO) $(VFR_GRAMMER_FILE)

$(VFR_DLG_SPAWN) : $(ETO)\VfrParser.dlg
    $(DLG) $(DLG_FLAGS) -cl VfrLexer -o $(ETO) $(ETO)\VfrParser.dlg

$(ETO)\VfrSyntax.obj : $(ETO)\VfrSyntax.cpp $(VFR_DLG_SPAWN)
    $(CC) -c $(C_FLAGS_PCCTS) /Fo$@ $(INC) $(ETO)\VfrSyntax.cpp

$(ETO)\EfiVfrParser.obj : $(ETO)\EfiVfrParser.cpp $(ETO)\EfiVfrParser.h $(VFR_DLG_SPAWN)
    $(CC) -c $(C_FLAGS_PCCTS) /Fo$@ $(INC) $(ETO)\EfiVfrParser.cpp


$(ETO)\AParser.obj : $(ANTLR_H)\AParser.cpp
    $(CC) -c $(C_FLAGS_PCCTS) /Fo$@ $(ANTLR_H)\AParser.cpp

$(ETO)\ATokenBuffer.obj : $(ANTLR_H)\ATokenBuffer.cpp
    $(CC) -c $(C_FLAGS_PCCTS) /Fo$@ $(ANTLR_H)\ATokenBuffer.cpp

$(ETO)\DLexerBase.obj : $(ANTLR_H)\DLexerBase.cpp
    $(CC) -c $(C_FLAGS_PCCTS) /Fo$@ $(ANTLR_H)\DLexerBase.cpp

$(ETO)\VfrLexer.obj : $(ETO)\VfrLexer.cpp $(VFR_DLG_SPAWN)
    $(CC) -c $(C_FLAGS_PCCTS) /Fo$@ $(INC) $(ETO)\VfrLexer.cpp

$(ETO)\VfrFormPkg.obj : $(SRC)\VfrFormPkg.cpp $(HEADER_FILES)
    $(CC) -c $(C_FLAGS_PCCTS) $(INC) /Fo$@ $(SRC)\VfrFormPkg.cpp

$(ETO)\VfrError.obj : $(SRC)\VfrError.cpp $(HEADER_FILES)
    $(CC) -c $(C_FLAGS_PCCTS) $(INC) /Fo$@ $(SRC)\VfrError.cpp

$(ETO)\VfrUtilityLib.obj : $(SRC)\VfrUtilityLib.cpp $(HEADER_FILES)
    $(CC) -c $(C_FLAGS_PCCTS) $(INC) /Fo$@ $(SRC)\VfrUtilityLib.cpp

$(ETO)\VfrCompiler.obj : $(SRC)\VfrCompiler.cpp $(HEADER_FILES)
    $(CC) -c $(C_FLAGS_PCCTS) $(INC) /Fo$@ $(SRC)\VfrCompiler.cpp

#
# Add Binary Build description for this tools.
#

!IF (("$(EFI_BINARY_TOOLS)" == "YES") && EXIST($(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).exe))
$(TARGET_EXE): $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).exe
  copy $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).exe $(TARGET_EXE) /Y
  if exist $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb \
  copy $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb /Y
!ELSE
$(TARGET_EXE) : $(OBJECTS) $(HEADER_FILES)
  $(LINK) $(MSVS_LINK_LIBPATHS) $(LINK_FLAGS_PCCTS) /OUT:$(TARGET_EXE) $(OBJECTS)
!IF ("$(EFI_BINARY_BUILD)" == "YES")
  if not exist $(EFI_PLATFORM_BIN)\Tools mkdir $(EFI_PLATFORM_BIN)\Tools
  if exist $(TARGET_EXE) copy $(TARGET_EXE) $(EFI_PLATFORM_BIN)\tools\$(TARGET_NAME).exe /Y
  if exist $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb \
  copy $(EDK_TOOLS_OUTPUT)\$(TARGET_NAME).pdb $(EFI_PLATFORM_BIN)\Tools\$(TARGET_NAME).pdb /Y
!ENDIF
!ENDIF

clean:
