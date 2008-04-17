
#
# Platform Macro Definition
#
PLATFORM_NAME = DuetPkg
PLATFORM_GUID = 199E24E0-0989-42aa-87F2-611A8C397E72
PLATFORM_VERSION = 0.3
PLATFORM_RELATIVE_DIR = DuetPkg
PLATFORM_DIR = $(WORKSPACE)\DuetPkg
PLATFORM_OUTPUT_DIR = Build\DuetPkg

#
# Module Macro Definition
#
MODULE_NAME = BootSector
MODULE_GUID = 2410F0DF-D915-4137-BD04-AAB6BA4C50E0
MODULE_VERSION = 1.0
MODULE_TYPE = USER_DEFINED
MODULE_FILE_BASE_NAME = BootSector
BASE_NAME = $(MODULE_NAME)
MODULE_RELATIVE_DIR = DuetPkg\BootSector
MODULE_DIR = $(WORKSPACE)\DuetPkg\BootSector

#
# Build Configuration Macro Definition
#
ARCH = IA32
TOOLCHAIN_TAG = MYTOOLS
TARGET = DEBUG
BASETOOLS_DIR=m:\tree\working\BaseTools\Bin\Win32

#
# Build Directory Macro Definition
#
# PLATFORM_BUILD_DIR = m:\tree\working\Build\DuetPkg\DEBUG_MYTOOLS
BUILD_DIR = $(WORKSPACE)\Build\DuetPkg\DEBUG_MYTOOLS
BIN_DIR = $(BUILD_DIR)\IA32
LIB_DIR = $(BIN_DIR)
MODULE_BUILD_DIR = $(BUILD_DIR)\IA32\DuetPkg\BootSector\BootSector
OUTPUT_DIR = $(MODULE_BUILD_DIR)\OUTPUT
DEBUG_DIR = $(MODULE_BUILD_DIR)\DEBUG
DEST_DIR_OUTPUT = $(OUTPUT_DIR)
DEST_DIR_DEBUG = $(DEBUG_DIR)

#
# Default Tools Flags Macro Definition (from tools_def.txt by default)
#
DEFAULT_PP_FLAGS = /nologo /E /TC /FIAutoGen.h
DEFAULT_SLINK_FLAGS = /nologo /LTCG
DEFAULT_CC_FLAGS = /nologo /c /WX /GS- /W4 /Gs8192 /Gy /D UNICODE /O1ib2 /GL /FIAutoGen.h /EHs-c- /GR- /GF /Zi /Gm
DEFAULT_APP_FLAGS = /nologo /E /TC
DEFAULT_VFRPP_FLAGS = /nologo /E /TC /DVFRCOMPILE /FIAutoGen.h
DEFAULT_DLINK_FLAGS = /NOLOGO /NODEFAULTLIB /IGNORE:4086 /OPT:REF /OPT:ICF=10 /MAP /ALIGN:32 /MACHINE:I386 /LTCG /DLL /ENTRY:$(IMAGE_ENTRY_POINT) /SUBSYSTEM:CONSOLE /SAFESEH:NO /BASE:0 /DRIVER /DEBUG /PDB:$(DEBUG_DIR)/$(BASE_NAME).pdb
DEFAULT_ASM_FLAGS = /nologo /c /WX /W3 /coff /Cx /Zd /Zi
DEFAULT_TIANO_FLAGS = 
DEFAULT_MAKE_FLAGS = /nologo
DEFAULT_ASMLINK_FLAGS = /nologo /tiny
DEFAULT_ASL_FLAGS = 


#
# Platform Tools Flags Macro Definition (from platform description file)
#
PLATFORM_PP_FLAGS = 
PLATFORM_SLINK_FLAGS = 
PLATFORM_CC_FLAGS = 
PLATFORM_APP_FLAGS = 
PLATFORM_VFRPP_FLAGS = 
PLATFORM_DLINK_FLAGS = 
PLATFORM_ASM_FLAGS = 
PLATFORM_TIANO_FLAGS = 
PLATFORM_MAKE_FLAGS = 
PLATFORM_ASMLINK_FLAGS = 
PLATFORM_ASL_FLAGS = 


#
# Module Tools Flags Macro Definition (from platform/module description file)
#
MODULE_PP_FLAGS = 
MODULE_SLINK_FLAGS = 
MODULE_CC_FLAGS = 
MODULE_APP_FLAGS = 
MODULE_VFRPP_FLAGS = 
MODULE_DLINK_FLAGS = 
MODULE_ASM_FLAGS = 
MODULE_TIANO_FLAGS = 
MODULE_MAKE_FLAGS = 
MODULE_ASMLINK_FLAGS = 
MODULE_ASL_FLAGS = 


#
# Tools Flag Macro
#
PP_FLAGS = $(DEFAULT_PP_FLAGS) $(PLATFORM_PP_FLAGS) $(MODULE_PP_FLAGS)
SLINK_FLAGS = $(DEFAULT_SLINK_FLAGS) $(PLATFORM_SLINK_FLAGS) $(MODULE_SLINK_FLAGS)
CC_FLAGS = $(DEFAULT_CC_FLAGS) $(PLATFORM_CC_FLAGS) $(MODULE_CC_FLAGS)
APP_FLAGS = $(DEFAULT_APP_FLAGS) $(PLATFORM_APP_FLAGS) $(MODULE_APP_FLAGS)
VFRPP_FLAGS = $(DEFAULT_VFRPP_FLAGS) $(PLATFORM_VFRPP_FLAGS) $(MODULE_VFRPP_FLAGS)
DLINK_FLAGS = $(DEFAULT_DLINK_FLAGS) $(PLATFORM_DLINK_FLAGS) $(MODULE_DLINK_FLAGS)
ASM_FLAGS = $(DEFAULT_ASM_FLAGS) $(PLATFORM_ASM_FLAGS) $(MODULE_ASM_FLAGS)
TIANO_FLAGS = $(DEFAULT_TIANO_FLAGS) $(PLATFORM_TIANO_FLAGS) $(MODULE_TIANO_FLAGS)
MAKE_FLAGS = $(DEFAULT_MAKE_FLAGS) $(PLATFORM_MAKE_FLAGS) $(MODULE_MAKE_FLAGS)
ASMLINK_FLAGS = $(DEFAULT_ASMLINK_FLAGS) $(PLATFORM_ASMLINK_FLAGS) $(MODULE_ASMLINK_FLAGS)
ASL_FLAGS = $(DEFAULT_ASL_FLAGS) $(PLATFORM_ASL_FLAGS) $(MODULE_ASL_FLAGS)


#
# Tools Path Macro
#
PP = C:\Program Files\Microsoft Visual Studio 8\Vc\bin\cl.exe
SLINK = C:\Program Files\Microsoft Visual Studio 8\Vc\bin\lib.exe
CC = C:\Program Files\Microsoft Visual Studio 8\Vc\bin\cl.exe
APP = C:\Program Files\Microsoft Visual Studio 8\Vc\bin\cl.exe
VFRPP = C:\Program Files\Microsoft Visual Studio 8\Vc\bin\cl.exe
DLINK = C:\Program Files\Microsoft Visual Studio 8\Vc\bin\link.exe
ASM = C:\WINDDK\3790.1830\bin\x86\ml.exe
TIANO = TianoCompress.exe
MAKE = C:\Program Files\Microsoft Visual Studio 8\Vc\bin\nmake.exe
#ASMLINK = C:\WINDDK\3790.1830\bin\bin16\link.exe
ASMLINK = C:\WINDDK\3790.1830\bin\bin16\link16.exe
ASL = C:\ASL\iasl.exe


MAKE_FILE = $(MODULE_BUILD_DIR)\Makefile

#
# Shell Command Macro
#
RD = rmdir /s /q
RM = del /f /q
MD = mkdir
CP = copy /y
MV = move /y


#
# Build Macro
#
ASSEMBLY_CODE_FILE_LIST = $(MODULE_DIR)\bootsect.asm \
                          $(MODULE_DIR)\bs16.asm \
                          $(MODULE_DIR)\bs32.asm \
                          $(MODULE_DIR)\efi32.asm \
                          $(MODULE_DIR)\Gpt.asm \
                          $(MODULE_DIR)\Mbr.asm \
                          $(MODULE_DIR)\start.asm \
                          $(MODULE_DIR)\start16.asm \
                          $(MODULE_DIR)\start32.asm 

TARGET_FILES = $(OUTPUT_DIR)\bootsect.com \
               $(OUTPUT_DIR)\bs16.com \
               $(OUTPUT_DIR)\bs32.com \
               $(OUTPUT_DIR)\Gpt.com \
               $(OUTPUT_DIR)\Mbr.com \
               $(OUTPUT_DIR)\Start.com \
               $(OUTPUT_DIR)\Start16.com \
               $(OUTPUT_DIR)\Start32.com \
               $(OUTPUT_DIR)\efi32.com2

INC = 
      

#OBJECTS = 

LIBS = 

COMMON_DEPS = 

all: init $(TARGET_FILES) loader
          
init:
  if not exist $(OUTPUT_DIR) mkdir $(OUTPUT_DIR)
  if not exist $(DEBUG_DIR) mkdir $(DEBUG_DIR)

#=============                
$(OUTPUT_DIR)\bootsect.obj:$(MODULE_DIR)\bootsect.asm
  $(ASM) /c /omf /Fo"$(OUTPUT_DIR)\bootsect.obj" /FR"$(OUTPUT_DIR)\bootsect.txt" "$(MODULE_DIR)\bootsect.asm"
             
$(OUTPUT_DIR)\bootsect.com:$(OUTPUT_DIR)\bootsect.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\bootsect.obj,$(OUTPUT_DIR)\bootsect.com,$(OUTPUT_DIR)\bootsect.map,,,

#=============                

$(OUTPUT_DIR)\bs16.obj:$(MODULE_DIR)\bs16.asm
  $(ASM) /c /omf /Fo"$(OUTPUT_DIR)\bs16.obj" "$(MODULE_DIR)\bs16.asm"
             
$(OUTPUT_DIR)\bs16.com:$(OUTPUT_DIR)\bs16.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\bs16.obj,$(OUTPUT_DIR)\bs16.com,$(OUTPUT_DIR)\bs16.map,,,

#=============                

$(OUTPUT_DIR)\bs32.obj:$(MODULE_DIR)\bs32.asm
  $(ASM) /c /omf /Fo"$(OUTPUT_DIR)\bs32.obj" "$(MODULE_DIR)\bs32.asm"
             
$(OUTPUT_DIR)\bs32.com:$(OUTPUT_DIR)\bs32.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\bs32.obj,$(OUTPUT_DIR)\bs32.com,$(OUTPUT_DIR)\bs32.map,,,

#=============                

$(OUTPUT_DIR)\Gpt.obj:$(MODULE_DIR)\Gpt.asm
  $(ASM) /c /omf /Fo"$(OUTPUT_DIR)\Gpt.obj" "$(MODULE_DIR)\Gpt.asm"
             
$(OUTPUT_DIR)\Gpt.com:$(OUTPUT_DIR)\Gpt.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\Gpt.obj,$(OUTPUT_DIR)\Gpt.com,$(OUTPUT_DIR)\Gpt.map,,,

#=============                

$(OUTPUT_DIR)\Mbr.obj:$(MODULE_DIR)\Mbr.asm
  $(ASM) /c /omf /Fo"$(OUTPUT_DIR)\Mbr.obj" "$(MODULE_DIR)\Mbr.asm"
             
$(OUTPUT_DIR)\Mbr.com:$(OUTPUT_DIR)\Mbr.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\Mbr.obj,$(OUTPUT_DIR)\Mbr.com,$(OUTPUT_DIR)\Mbr.map,,,

#============

$(OUTPUT_DIR)\Start.obj:$(MODULE_DIR)\Start.asm
  $(ASM) /c /omf /Fo"$(OUTPUT_DIR)\Start.obj" "$(MODULE_DIR)\Start.asm"
             
$(OUTPUT_DIR)\Start.com:$(OUTPUT_DIR)\Start.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\Start.obj,$(OUTPUT_DIR)\Start.com,$(OUTPUT_DIR)\Start.map,,,
  
#=============     

$(OUTPUT_DIR)\Start16.obj:$(MODULE_DIR)\Start16.asm
  $(ASM) /c /omf /Fo"$(OUTPUT_DIR)\Start16.obj" "$(MODULE_DIR)\Start16.asm"
             
$(OUTPUT_DIR)\Start16.com:$(OUTPUT_DIR)\Start16.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\Start16.obj,$(OUTPUT_DIR)\Start16.com,$(OUTPUT_DIR)\Start16.map,,,

#=============

$(OUTPUT_DIR)\Start32.obj:$(MODULE_DIR)\Start32.asm
  $(ASM) /c /omf /Fo"$(OUTPUT_DIR)\Start32.obj" "$(MODULE_DIR)\Start32.asm"
             
$(OUTPUT_DIR)\Start32.com:$(OUTPUT_DIR)\Start32.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\Start32.obj,$(OUTPUT_DIR)\Start32.com,$(OUTPUT_DIR)\Start32.map,,,

#=============

$(OUTPUT_DIR)\efi32.obj:$(MODULE_DIR)\efi32.asm
  $(ASM) /c /omf /Fo"$(OUTPUT_DIR)\efi32.obj" "$(MODULE_DIR)\efi32.asm"
             
$(OUTPUT_DIR)\efi32.com:$(OUTPUT_DIR)\efi32.obj
  "$(ASMLINK)" /tiny $(OUTPUT_DIR)\efi32.obj,$(OUTPUT_DIR)\efi32.com,$(OUTPUT_DIR)\efi32.map,,,

#=============

$(OUTPUT_DIR)\efi32.com2:$(OUTPUT_DIR)\efi32.com
  $(BASETOOLS_DIR)\Split.exe -f $(OUTPUT_DIR)\efi32.com -t $(OUTPUT_DIR)\efi32.com2 -s 135168
#
# clean all generated files
#

loader:$(BUILD_DIR)\FV\Efildr

$(BUILD_DIR)\FV\DUETEFIMAINFV.z:$(BUILD_DIR)\FV\DUETEFIMAINFV.Fv
  $(BASETOOLS_DIR)\TianoCompress -e -o $(BUILD_DIR)\FV\DUETEFIMAINFV.z $(BUILD_DIR)\FV\DUETEFIMAINFV.Fv
  
$(BUILD_DIR)\FV\DxeMain.z:$(BUILD_DIR)\IA32\DxeMain.efi  
  $(BASETOOLS_DIR)\TianoCompress -e -o $(BUILD_DIR)\FV\DxeMain.z $(BUILD_DIR)\IA32\DxeMain.efi
  
$(BUILD_DIR)\FV\DxeIpl.z:$(BUILD_DIR)\IA32\DxeIpl.efi  
  $(BASETOOLS_DIR)\TianoCompress -e -o $(BUILD_DIR)\FV\DxeIpl.z $(BUILD_DIR)\IA32\DxeIpl.efi
    
$(BUILD_DIR)\FV\Efildr32:$(BUILD_DIR)\IA32\EfiLoader.efi $(BUILD_DIR)\FV\DxeIpl.z $(BUILD_DIR)\FV\DUETEFIMAINFV.z
  $(BASETOOLS_DIR)\EfiLdrImage.exe -o $(BUILD_DIR)\FV\Efildr32 $(BUILD_DIR)\IA32\EfiLoader.efi $(BUILD_DIR)\FV\DxeIpl.z $(BUILD_DIR)\FV\DUETEFIMAINFV.z
  
$(BUILD_DIR)\FV\Efildr:$(OUTPUT_DIR)\Start.com $(OUTPUT_DIR)\Efi32.com2 $(BUILD_DIR)\FV\Efildr32
  copy /b $(OUTPUT_DIR)\BootSect.com+$(OUTPUT_DIR)\Efi32.com2+$(BUILD_DIR)\FV\Efildr32 $(BUILD_DIR)\FV\Efildr
    
clean:
	if exist $(DEBUG_DIR) rmdir /s /q $(DEBUG_DIR)
	if exist $(OUTPUT_DIR) rmdir /s /q $(OUTPUT_DIR)


