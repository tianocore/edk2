#/*++
#
#  Copyright (c) 2006 - 2007, Intel Corporation                                                         
#  All rights reserved. This program and the accompanying materials                          
#  are licensed and made available under the terms and conditions of the BSD License         
#  which accompanies this distribution.  The full text of the license may be found at        
#  http://opensource.org/licenses/bsd-license.php                                            
#                                                                                            
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#  
#  Module Name:  
#    Makefile
#  
#  Abstract:
#   
#--*/

#
# Globals
#
BIN_DIR     = $(BUILD_DIR)\$(PROCESSOR)
TOOLCHAIN   = TOOLCHAIN_$(PROCESSOR)

TOOLBIN_DIR = $(BUILD_DIR)\Tools

#
# Include CommonTools.env enviroment
#

!INCLUDE $(BUILD_DIR)\PlatformTools.env

#
# Include paths
#
INC = -I $(SOURCE_DIR)\. -I $(SOURCE_DIR)\.\$(PROCESSOR) $(INC)
INC = -I $(EDK_SOURCE)\Foundation\ \
      -I $(EDK_SOURCE)\Foundation\Include \
      -I $(EDK_SOURCE)\Foundation\Include\$(PROCESSOR) \
      -I $(EDK_SOURCE)\Foundation\Efi \
      -I $(EDK_SOURCE)\Foundation\Efi\Include \
      -I $(EDK_SOURCE)\Foundation\Framework \
      -I $(EDK_SOURCE)\Foundation\Framework\Include \
      -I $(EDK_SOURCE)\Foundation\Library\Dxe\Include \
      -I $(EDK_SOURCE)\Foundation\Library\Pei\Include \
      -I $(EDK_SOURCE)\Foundation\Include\Pei \
      $(INC)

LDRDEP = $(BUILD_DIR)\..\Loader\EfiLdr\Efildr.c \
         $(BUILD_DIR)\..\Loader\EfiLdr\EfiLdrHandoff.h \
         $(BUILD_DIR)\..\Loader\EfiLdr\EfiLoader.c \
         $(BUILD_DIR)\..\Loader\EfiLdr\Debug.c \
         $(BUILD_DIR)\..\Loader\EfiLdr\Debug.h \
         $(BUILD_DIR)\..\Loader\EfiLdr\PeLoader.c \
         $(BUILD_DIR)\..\Loader\EfiLdr\PeLoader.h \
         $(BUILD_DIR)\..\Loader\EfiLdr\Support.c \
         $(BUILD_DIR)\..\Loader\EfiLdr\Support.h

all : \
  $(BIN_DIR)\BootSect.com \
  $(BIN_DIR)\Bs16.com     \
  $(BIN_DIR)\Bs32.com     \
  $(BIN_DIR)\Gpt.com      \
  $(BIN_DIR)\Mbr.com      \
  $(BIN_DIR)\Start.com    \
  $(BIN_DIR)\Start16.com  \
  $(BIN_DIR)\Start32.com  \
  $(BIN_DIR)\Efi32.com2   \
  $(BIN_DIR)\Efildr.efi   \


loader :      \
  Fv\Efildr   \
  Fv\Efildr16 \
  Fv\Efildr20 \


#
# Generate loader object
#
$(BIN_DIR)\BootSect.obj: $(BUILD_DIR)\..\Loader\BootSector\BootSect.asm
  $(ASM16) /c /omf /Fo$(BIN_DIR)\BootSect.obj $(BUILD_DIR)\..\Loader\BootSector\BootSect.asm 

$(BIN_DIR)\BootSect.com: $(BIN_DIR)\BootSect.obj
  cd $(BIN_DIR)
  $(ASMLINK16) /tiny BootSect.obj,BootSect.com,BootSect.map,,,

$(BIN_DIR)\Bs16.obj: $(BUILD_DIR)\..\Loader\BootSector\Bs16.asm
  $(ASM16) /c /omf /Fo$(BIN_DIR)\Bs16.obj $(BUILD_DIR)\..\Loader\BootSector\Bs16.asm 

$(BIN_DIR)\Bs16.com: $(BIN_DIR)\Bs16.obj
  cd $(BIN_DIR)
  $(ASMLINK16) /tiny Bs16.obj,Bs16.com,Bs16.map,,,

$(BIN_DIR)\Bs32.obj: $(BUILD_DIR)\..\Loader\BootSector\Bs32.asm
  $(ASM16) /c /omf /Fo$(BIN_DIR)\Bs32.obj $(BUILD_DIR)\..\Loader\BootSector\Bs32.asm 

$(BIN_DIR)\Bs32.com: $(BIN_DIR)\Bs32.obj
  cd $(BIN_DIR)
  $(ASMLINK16) /tiny Bs32.obj,Bs32.com,Bs32.map,,,

$(BIN_DIR)\Gpt.obj: $(BUILD_DIR)\..\Loader\BootSector\Gpt.asm
  $(ASM16) /c /omf /Fo$(BIN_DIR)\Gpt.obj $(BUILD_DIR)\..\Loader\BootSector\Gpt.asm 

$(BIN_DIR)\Gpt.com: $(BIN_DIR)\Gpt.obj
  cd $(BIN_DIR)
  $(ASMLINK16) /tiny Gpt.obj,Gpt.com,Gpt.map,,,

$(BIN_DIR)\Mbr.obj: $(BUILD_DIR)\..\Loader\BootSector\Mbr.asm
  $(ASM16) /c /omf /Fo$(BIN_DIR)\Mbr.obj $(BUILD_DIR)\..\Loader\BootSector\Mbr.asm 

$(BIN_DIR)\Mbr.com: $(BIN_DIR)\Mbr.obj
  cd $(BIN_DIR)
  $(ASMLINK16) /tiny Mbr.obj,Mbr.com,Mbr.map,,,

$(BIN_DIR)\Start.obj: $(BUILD_DIR)\..\Loader\BootSector\Start.asm
  $(ASM16) /c /omf /Fo$(BIN_DIR)\Start.obj $(BUILD_DIR)\..\Loader\BootSector\Start.asm 

$(BIN_DIR)\Start.com: $(BIN_DIR)\Start.obj
  cd $(BIN_DIR)
  $(ASMLINK16) /tiny Start.obj,Start.com,Start.map,,,

$(BIN_DIR)\Start16.obj: $(BUILD_DIR)\..\Loader\BootSector\Start16.asm
  $(ASM16) /c /omf /Fo$(BIN_DIR)\Start16.obj $(BUILD_DIR)\..\Loader\BootSector\Start16.asm 

$(BIN_DIR)\Start16.com: $(BIN_DIR)\Start16.obj
  cd $(BIN_DIR)
  $(ASMLINK16) /tiny Start16.obj,Start16.com,Start16.map,,,

$(BIN_DIR)\Start32.obj: $(BUILD_DIR)\..\Loader\BootSector\Start32.asm
  $(ASM16) /c /omf /Fo$(BIN_DIR)\Start32.obj $(BUILD_DIR)\..\Loader\BootSector\Start32.asm 

$(BIN_DIR)\Start32.com: $(BIN_DIR)\Start32.obj
  cd $(BIN_DIR)
  $(ASMLINK16) /tiny Start32.obj,Start32.com,Start32.map,,,

$(BIN_DIR)\Efi32.obj: $(BUILD_DIR)\..\Loader\BootSector\Efi32.asm
  $(ASM16) /c /omf /Fo$(BIN_DIR)\Efi32.obj $(BUILD_DIR)\..\Loader\BootSector\Efi32.asm 

$(BIN_DIR)\Efi32.com: $(BIN_DIR)\Efi32.obj
  cd $(BIN_DIR)
  $(ASMLINK16) /tiny Efi32.obj,Efi32.com,Efi32.map,,,

$(BIN_DIR)\Efi32.com2: $(BIN_DIR)\Efi32.com
  $(TOOLBIN_DIR)\Splitfile $(BIN_DIR)\Efi32.com 135168

$(BIN_DIR)\Efildr.obj: $(LDRDEP)
  $(CC) $(C_FLAGS) $(BUILD_DIR)\..\Loader\EfiLdr\Efildr.c

$(BIN_DIR)\Efildr.dll: $(BIN_DIR)\Efildr.obj
  $(LINK) /nologo /MACHINE:X86 /SUBSYSTEM:CONSOLE /NODEFAULTLIB /INCREMENTAL:NO \
          /MAP /FIXED /BASE:0x00010000 /OPT:REF /ALIGN:32 /MERGE:.data=.text \
          /MERGE:.rdata=.text /DRIVER /ENTRY:EfiLoader $(BIN_DIR)\Efildr.obj \
          $(BIN_DIR)\CompilerStub.lib $(BIN_DIR)\EfiCommonLib.lib $(BIN_DIR)\PeiLib.lib \
          /OUT:$(BIN_DIR)\Efildr.dll /IGNORE:4078,4096

$(BIN_DIR)\Efildr.efi: $(BIN_DIR)\Efildr.dll
  $(TOOLBIN_DIR)\FwImage app $(BIN_DIR)\Efildr.dll $(BIN_DIR)\Efildr.efi

#
# Generate loader binary
#
Fv\EfiMain.z : Fv\EfiMain.fv
  $(TOOLBIN_DIR)\Eficompress -tTiano Fv\EfiMain.fv Fv\EfiMain.z
 
Fv\DxeMain.z : $(BIN_DIR)\DxeMain.efi
  $(TOOLBIN_DIR)\Eficompress -tTiano $(BIN_DIR)\DxeMain.efi Fv\DxeMain.z
 
Fv\DxeIpl.z : $(BIN_DIR)\DxeIpl.efi
  $(TOOLBIN_DIR)\Eficompress -tTiano $(BIN_DIR)\DxeIpl.efi Fv\DxeIpl.z

Fv\Efildr32: $(BIN_DIR)\Efildr.efi Fv\DxeIpl.z Fv\DxeMain.z Fv\EfiMain.z
  $(TOOLBIN_DIR)\Efildrimage Fv\Efildr32 $(BIN_DIR)\Efildr.efi Fv\DxeIpl.z Fv\DxeMain.z Fv\EfiMain.z

Fv\Efildr: $(BIN_DIR)\Start.com $(BIN_DIR)\Efi32.com2 Fv\Efildr32
  copy /b $(BIN_DIR)\Start.com+$(BIN_DIR)\Efi32.com2+Fv\Efildr32 Fv\Efildr

Fv\Efildr16: $(BIN_DIR)\Start16.com $(BIN_DIR)\Efi32.com2 Fv\Efildr32
  copy /b $(BIN_DIR)\Start16.com+$(BIN_DIR)\Efi32.com2+Fv\Efildr32 Fv\Efildr16

Fv\Efildr20: $(BIN_DIR)\Start32.com $(BIN_DIR)\Efi32.com2 Fv\Efildr32
  copy /b $(BIN_DIR)\Start32.com+$(BIN_DIR)\Efi32.com2+Fv\Efildr32 Fv\Efildr20

