#/*++
#
# Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials                          
# are licensed and made available under the terms and conditions of the BSD License         
# which accompanies this distribution.  The full text of the license may be found at        
# http://opensource.org/licenses/bsd-license.php                                            
#                                                                                           
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
# 
#  Module Name:
#
#    Common.dsc
#
#  Abstract:
#
#    This is the build description file containing the platform
#    independent build instructions.  Platform specific instructions will
#    be prepended to produce the final build DSC file.
#
#
#  Notes:
#    
#    The info in this file is broken down into sections. The start of a section
#    is designated by a "[" in the first column. So the [=====] separater ends
#    a section.
#    
#--*/

[=============================================================================]
#
# These get emitted at the top of the generated master makefile. 
#
[=============================================================================]
[Makefile.out]
#
# From the [makefile.out] section of the DSC file
#
TOOLCHAIN = 
MAKE      = nmake -nologo

!INCLUDE $(BUILD_DIR)\PlatformTools.env

all : libraries fvs

[=============================================================================]
#
# These get expanded and dumped out to each component makefile after the
# component INF [defines] section gets parsed.
#
[=============================================================================]
[Makefile.Common]
#
# From the [Makefile.Common] section of the description file.
#
PROCESSOR        = $(PROCESSOR)
BASE_NAME        = $(BASE_NAME)
BUILD_NUMBER     = $(BUILD_NUMBER)
VERSION_STRING   = $(VERSION_STRING)
TOOLCHAIN        = TOOLCHAIN_$(PROCESSOR)
FILE_GUID        = $(FILE_GUID)
COMPONENT_TYPE   = $(COMPONENT_TYPE)
FV_DIR           = $(BUILD_DIR)\FV
PLATFORM         = $(PROJECT_NAME) 

#
# Define the global dependency files
#
!IF EXIST ($(DEST_DIR)\$(BASE_NAME)StrDefs.h)
INC_DEPS         = $(INC_DEPS) $(DEST_DIR)\$(BASE_NAME)StrDefs.h
!ENDIF
#ENV_DEPS         = $(ENV_DEPS) $(EDK_SOURCE)\Sample\CommonTools.env
#ENV_DEPS         = $(ENV_DEPS) $(BUILD_DIR)\PlatformTools.env
#ENV_DEPS         = $(ENV_DEPS) $(BUILD_DIR)\Config.env
ALL_DEPS         = $(INC_DEPS) $(ENV_DEPS)

!IF "$(LANGUAGE)" != ""
LANGUAGE_FLAGS    = -lang $(LANGUAGE)
!ENDIF

!INCLUDE $(BUILD_DIR)\PlatformTools.env

!IF "$(COMPONENT_TYPE)" == "PIC_PEIM" || "$(COMPONENT_TYPE)" == "PE32_PEIM" || "$(COMPONENT_TYPE)" == "RELOCATABLE_PEIM" || "$(COMPONENT_TYPE)" == "COMBINED_PEIM_DRIVER"
DEPEX_TYPE = EFI_SECTION_PEI_DEPEX
!ELSE
DEPEX_TYPE = EFI_SECTION_DXE_DEPEX
!ENDIF

!IF "$(COMPONENT_TYPE)" != "LIBRARY" && EXIST($(BUILD_DIR)\$(PROCESSOR)\CompilerStub.lib)
LIBS = $(LIBS) $(BUILD_DIR)\$(PROCESSOR)\CompilerStub.lib
!ENDIF

#
# Command flags for MAKEDEPS tool
#
DEP_FLAGS = -target $** -o $(DEP_FILE) $(INC) -ignorenotfound -q
DEP_FLAGS2 = -target $@ -o $(DEP_FILE) -cl

[=============================================================================]
#
# These are the commands to compile source files. One of these blocks gets 
# emitted to the component's makefile for each source file. The section
# name is encoded as [Compile.$(PROCESSOR).source_filename_extension], where
# the source filename comes from the sources section of the component INF file.
#
# If the dependency list file already exists, then include it for this 
# source file. If it doesn't exist, then this is a clean build and the
# dependency file will get created below and the source file will get 
# compiled. 
#
# Current behavior is that the first clean build will not create dep files. 
# But the following second build has to create dep files before build source files.
# CREATEDEPS flag is used to judge whether current build is the second build or not.
#
#
[=============================================================================]
[Compile.Ia32.asm,Compile.x64.asm]

#
# Add build dependency check
#
DEP_FILE    = $(DEST_DIR)\$(FILE)Asm.dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

!IF EXIST($(DEST_DIR)\$(FILE).obj)
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(FILE)Asm.dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(DEST_DIR)\$(FILE).obj
  $(MAKEDEPS) -f $(SOURCE_FILE_NAME) $(DEP_FLAGS) -asm

#
# Compile the file
#
$(DEST_DIR)\$(FILE).obj : $(SOURCE_FILE_NAME) $(INF_FILENAME) $(ALL_DEPS)
  $(ASM) $(ASM_FLAGS) $(SOURCE_FILE_NAME)

[=============================================================================]
[Compile.Ipf.s]

#
# Add build dependency check
#
DEP_FILE    = $(DEST_DIR)\$(FILE)S.dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"

!IF EXIST($(DEST_DIR)\$(FILE).obj)
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(FILE)S.dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(DEST_DIR)\$(FILE).obj
  $(MAKEDEPS) -f $(SOURCE_FILE_NAME) $(DEP_FLAGS)

!ENDIF

#
# Compile the file
#
$(DEST_DIR)\$(FILE).obj : $(SOURCE_FILE_NAME) $(INF_FILENAME) $(ALL_DEPS)
!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"
  $(CC) $(C_FLAGS_PRO) $(SOURCE_FILE_NAME) > $(DEST_DIR)\$(FILE).pro
!ELSE
  -$(CC) $(C_FLAGS_PRO) $(SOURCE_FILE_NAME) /showIncludes > $(DEST_DIR)\$(FILE).pro 2> $(DEST_DIR)\$(FILE)S.cl
  @$(MAKEDEPS) -f $(DEST_DIR)\$(FILE)S.cl $(DEP_FLAGS2)
!ENDIF
  $(ASM) $(ASM_FLAGS) $(DEST_DIR)\$(FILE).pro

[=============================================================================]
[Compile.Ia32.c,Compile.Ipf.c,Compile.x64.c]

#
# Add build dependency check
#
DEP_FILE    = $(DEST_DIR)\$(FILE).dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"

!IF EXIST($(DEST_DIR)\$(FILE).obj)
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(FILE).dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(DEST_DIR)\$(FILE).obj
  $(MAKEDEPS) -f $(SOURCE_FILE_NAME) $(DEP_FLAGS)

!ENDIF
  
#
# Compile the file
#
$(DEST_DIR)\$(FILE).obj : $(SOURCE_FILE_NAME) $(INF_FILENAME) $(ALL_DEPS)
!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"
  $(CC) $(C_FLAGS) $(SOURCE_FILE_NAME)
!ELSE
  -$(CC) $(C_FLAGS) $(SOURCE_FILE_NAME) /showIncludes > $(DEST_DIR)\$(FILE).cl
  @$(MAKEDEPS) -f $(DEST_DIR)\$(FILE).cl $(DEP_FLAGS2)
!ENDIF

[=============================================================================]
[Compile.Ebc.c]

#
# Add build dependency check
#
DEP_FILE    = $(DEST_DIR)\$(FILE).dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

!IF EXIST($(DEST_DIR)\$(FILE).obj)
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(FILE).dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(DEST_DIR)\$(FILE).obj
  $(MAKEDEPS) -f $(SOURCE_FILE_NAME) $(DEP_FLAGS)

#
# Compile the file
#
$(DEST_DIR)\$(FILE).obj : $(SOURCE_FILE_NAME) $(INF_FILENAME) $(ALL_DEPS)
  $(EBC_CC) $(EBC_C_FLAGS) $(SOURCE_FILE_NAME)

[=============================================================================]
#
# Commands for compiling a ".apr" Apriori source file.
#
[=============================================================================]
[Compile.Ia32.Apr,Compile.Ipf.Apr,Compile.Ebc.Apr,Compile.x64.Apr]
#
# Create the raw binary file. If you get an error on the build saying it doesn't
# know how to create the .apr file, then you're missing (or mispelled) the
# "APRIORI=" on the component lines in components section in the DSC file.
#
$(DEST_DIR)\$(BASE_NAME).bin : $(SOURCE_FILE_NAME) $(INF_FILENAME)
  $(GENAPRIORI) -v -f $(SOURCE_FILE_NAME) -o $(DEST_DIR)\$(BASE_NAME).bin

$(DEST_DIR)\$(BASE_NAME).sec : $(DEST_DIR)\$(BASE_NAME).bin
  $(GENSECTION) -I $(DEST_DIR)\$(BASE_NAME).bin -O $(DEST_DIR)\$(BASE_NAME).sec -S EFI_SECTION_RAW

[=============================================================================]
[Build.Ia32.Apriori,Build.Ipf.Apriori,Build.Ebc.Apriori,Build.x64.Apriori]

all : $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).FFS

#
# Run GenFfsFile on the package file and .raw file to create the firmware file
#
$(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).FFS : $(DEST_DIR)\$(BASE_NAME).sec $(PACKAGE_FILENAME)
  $(GENFFSFILE) -B $(DEST_DIR) -P1 $(PACKAGE_FILENAME) -V

#
# Remove the generated temp and final files for this modules.
#
clean :
!IF ("$(FILE_GUID)" != "")
  @if exist $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).* del $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).*
!ENDIF
  @if exist $(BIN_DIR)\$(BASE_NAME).* del $(BIN_DIR)\$(BASE_NAME).*
  @del /q $(DEST_OUTPUT_DIRS) 

[=============================================================================]
[Build.Ia32.Makefile,Build.Ipf.Makefile,Build.Ebc.Makefile,Build.x64.Makefile]

#
# Set some required macros
#
MAKEFILE_MACROS = SOURCE_DIR=$(SOURCE_DIR)                \
                  BUILD_DIR=$(BUILD_DIR)                  \
                  FILE_GUID=$(FILE_GUID)                  \
                  DEST_DIR=$(DEST_DIR)                    \
                  PROCESSOR=$(PROCESSOR)                  \
                  TOOLCHAIN=TOOLCHAIN_$(PROCESSOR)        \
                  BASE_NAME=$(BASE_NAME)                  \
                  PACKAGE_FILENAME=$(PACKAGE_FILENAME)

#
# Just call the makefile from the source directory, passing in some
# useful info.
#
all :
  $(MAKE) -f $(SOURCE_DIR)\makefile.new all $(MAKEFILE_MACROS)

#
# Remove the generated temp and final files for this modules.
#
clean :
  @- $(MAKE) -f $(SOURCE_DIR)\makefile.new clean $(MAKEFILE_MACROS) > NUL 2>&1
!IF ("$(FILE_GUID)" != "")
  @if exist $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).* del $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).*
!ENDIF
  @if exist $(BIN_DIR)\$(BASE_NAME).* del $(BIN_DIR)\$(BASE_NAME).*
  @del /q $(DEST_OUTPUT_DIRS) 

[=============================================================================]
#
# Instructions for building a component that uses a custom makefile. Encoding 
# is [build.$(PROCESSOR).$(BUILD_TYPE)].
#
# To build these components, simply call the makefile from the source 
# directory.
#
[=============================================================================]
[Build.Ia32.Custom_Makefile,Build.Ipf.Custom_Makefile,Build.Ebc.Custom_Makefile,Build.x64.Custom_Makefile]

#
# Set some required macros
#
MAKEFILE_MACROS = SOURCE_DIR=$(SOURCE_DIR)                \
                  BUILD_DIR=$(BUILD_DIR)                  \
                  DEST_DIR=$(DEST_DIR)                    \
                  FILE_GUID=$(FILE_GUID)                  \
                  PROCESSOR=$(PROCESSOR)                  \
                  TOOLCHAIN=TOOLCHAIN_$(PROCESSOR)        \
                  BASE_NAME=$(BASE_NAME)                  \
                  PLATFORM=$(PLATFORM)                    \
                  SOURCE_FV=$(SOURCE_FV)                  \
                  PACKAGE_FILENAME=$(PACKAGE_FILENAME)

#
# Just call the makefile from the source directory, passing in some
# useful info.
#
all : 
  $(MAKE) -f $(SOURCE_DIR)\makefile all $(MAKEFILE_MACROS)

#
# Remove the generated temp and final files for this modules.
#
clean :
  @- $(MAKE) -f $(SOURCE_DIR)\makefile clean $(MAKEFILE_MACROS) > NUL 2>&1
!IF ("$(FILE_GUID)" != "")
  @if exist $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).* del $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).*
!ENDIF
  @if exist $(BIN_DIR)\$(BASE_NAME).* del $(BIN_DIR)\$(BASE_NAME).*
  @del /q $(DEST_OUTPUT_DIRS) 

[=============================================================================]
#
# These commands are used to build libraries
#
[=============================================================================]
[Build.Ia32.LIBRARY,Build.Ipf.LIBRARY,Build.x64.LIBRARY]
#
# LIB all the object files into to our target lib file. Put
# a dependency on the component's INF file in case it changes.
#
LIB_NAME = $(LIB_DIR)\$(BASE_NAME).lib

#
# $(DEP_TARGETS) are not needed for binary build.
#
!IF ("$(BINARY)" == "TRUE") || (("$(BINARY)" == "") && ("$(EFI_BINARY_LIBRARY)" == "YES"))
DEP_TARGETS=
CREATEDEPS=
!ENDIF

#
# Module can be built from source code or binary files. 
#
!IF ((("$(BINARY)" == "TRUE") || (("$(BINARY)" == "") && ("$(EFI_BINARY_LIBRARY)" == "YES"))) \
    && EXIST($(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).lib))
$(LIB_NAME) : $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).lib
  copy $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).lib $(LIB_NAME) /Y
  if exist $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME)Obj.pdb \
  copy $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME)Obj.pdb $(DEST_DIR)\$(BASE_NAME)Obj.pdb /Y
!ELSE
$(LIB_NAME) : $(OBJECTS) $(LIBS) $(INF_FILENAME) $(ENV_DEPS)
  $(LIB) $(LIB_FLAGS) $(OBJECTS) $(LIBS) /OUT:$@
!IF ("$(EFI_BINARY_BUILD)" == "YES")
  if not exist $(EFI_PLATFORM_BIN)\$(PROCESSOR) mkdir $(EFI_PLATFORM_BIN)\$(PROCESSOR)
  if exist $(LIB_NAME) copy $(LIB_NAME) $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).lib /Y
  if exist $(DEST_DIR)\$(BASE_NAME)Obj.pdb copy $(DEST_DIR)\$(BASE_NAME)Obj.pdb $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME)Obj.pdb /Y
!ENDIF
!ENDIF

!IF "$(CREATEDEPS)"=="YES"
all : $(DEP_TARGETS)
  $(MAKE) -f $(MAKEFILE_NAME) all
!ELSE
all : $(LIB_NAME) $(DEP_TARGETS)
!ENDIF

#
# Remove the generated temp and final files for this modules.
#
clean :
!IF ("$(FILE_GUID)" != "")
  @if exist $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).* del $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).*
!ENDIF
  @if exist $(BIN_DIR)\$(BASE_NAME).* del $(BIN_DIR)\$(BASE_NAME).*
  @del /q $(DEST_OUTPUT_DIRS) 

[=============================================================================]
[Build.Ebc.LIBRARY]
#
# LIB all the object files into to our target lib file. Put
# a dependency on the component's INF file in case it changes.
#
LIB_NAME = $(LIB_DIR)\$(BASE_NAME).lib

$(LIB_NAME) : $(OBJECTS) $(LIBS) $(INF_FILENAME) $(ENV_DEPS)
   $(EBC_LIB) $(EBC_LIB_FLAGS) $(OBJECTS) $(LIBS) /OUT:$@

!IF "$(CREATEDEPS)"=="YES"
all : $(DEP_TARGETS)
  $(MAKE) -f $(MAKEFILE_NAME) all
!ELSE
all : $(LIB_NAME) $(DEP_TARGETS)
!ENDIF

#
# Remove the generated temp and final files for this modules.
#
clean :
!IF ("$(FILE_GUID)" != "")
  @if exist $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).* del $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).*
!ENDIF
  @if exist $(BIN_DIR)\$(BASE_NAME).* del $(BIN_DIR)\$(BASE_NAME).*
  @del /q $(DEST_OUTPUT_DIRS) 

[=============================================================================]
#
# This is the Build.$(PROCESSOR).$(COMPONENT_TYPE) section that tells how to
# convert a firmware volume into an FV FFS file. Simply run it through
# GenFfsFile with the appropriate package file. SOURCE_FV must be defined
# in the component INF file Defines section.
#
[=============================================================================]
[Build.Ia32.FvImageFile,Build.x64.FvImageFile,Build.Ipf.FvImageFile]

all : $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).Fvi

#
# Run GenFfsFile on the package file and FV file to create the firmware 
# volume FFS file. This FFS file maybe contain one pad section for alignment requirement.
#
$(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).Fvi : $(DEST_DIR)\$(SOURCE_FV)Fv.sec $(PACKAGE_FILENAME) $(PAD_SECTION) 
  $(GENFFSFILE) -B $(DEST_DIR) -P1 $(PACKAGE_FILENAME) -V

#
# Remove the generated temp and final files for this modules.
#
clean :
!IF ("$(FILE_GUID)" != "")
  @if exist $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).* del $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).*
!ENDIF
  @if exist $(BIN_DIR)\$(BASE_NAME).* del $(BIN_DIR)\$(BASE_NAME).*
  @del /q $(DEST_OUTPUT_DIRS) 

[=============================================================================]
#
# Since many of the steps are the same for the different component types, we 
# share this section for BS_DRIVER, RT_DRIVER, .... and IFDEF the parts that 
# differ.  The entire section gets dumped to the output makefile.
#
[=============================================================================]
[Build.Ia32.BS_DRIVER|RT_DRIVER|SAL_RT_DRIVER|PE32_PEIM|PEI_CORE|PIC_PEIM|RELOCATABLE_PEIM|DXE_CORE|APPLICATION|COMBINED_PEIM_DRIVER, Build.Ipf.BS_DRIVER|RT_DRIVER|SAL_RT_DRIVER|PEI_CORE|PE32_PEIM|PIC_PEIM|DXE_CORE|APPLICATION|COMBINED_PEIM_DRIVER, Build.x64.BS_DRIVER|RT_DRIVER|SAL_RT_DRIVER|PE32_PEIM|PEI_CORE|PIC_PEIM|RELOCATABLE_PEIM|DXE_CORE|APPLICATION|COMBINED_PEIM_DRIVER]

!IF "$(LOCALIZE)" == "YES"

!IF (("$(EFI_GENERATE_HII_RESOURCE)" == "YES") && ("$(EFI_SPECIFICATION_VERSION)" >= "0x0002000A"))
#
# This will generate HII resource section in PE/COFF image.
#
# Note: when HII package list is built into resource section, Driver no longer
# refer to C array generated by VfrCompiler ($(FILE_NAME)Bin) and StrGather
# ($(BASE_NAME)Strings); while in current build rule, those C array objects
# will still be linked with the Driver, so please turn on link flag "/OPT:REF"
# to minimize the code size.
#
HII_PACK_FILES   = $(HII_PACK_FILES) $(DEST_DIR)\$(BASE_NAME)Strings.hpk
LOCALIZE_TARGETS = $(LOCALIZE_TARGETS) $(DEST_DIR)\$(BASE_NAME).res
LINK_FLAGS_DLL   = $(LINK_FLAGS_DLL) $(DEST_DIR)\$(BASE_NAME).res

$(DEST_DIR)\$(BASE_NAME).rc : $(HII_PACK_FILES)
  $(HIIPACK) -g $(FILE_GUID) $(HII_PACK_FILES) -rc $(DEST_DIR)\$(BASE_NAME).rc -hii $(DEST_DIR)\$(BASE_NAME).hii

$(DEST_DIR)\$(BASE_NAME).res : $(DEST_DIR)\$(BASE_NAME).rc
  $(RC) /fo $(DEST_DIR)\$(BASE_NAME).res $(DEST_DIR)\$(BASE_NAME).rc

!ENDIF

!IF (("$(EFI_GENERATE_HII_EXPORT)" == "YES") && ("$(EFI_SPECIFICATION_VERSION)" < "0x0002000A"))
#
# There will be one HII pack containing all the strings. Add that file
# to the list of HII pack files we'll use to create our final HII export file.
#
HII_PACK_FILES    = $(HII_PACK_FILES) $(DEST_DIR)\$(BASE_NAME)Strings.hpk
LOCALIZE_TARGETS  = $(LOCALIZE_TARGETS) $(DEST_DIR)\$(BASE_NAME).hii
!ENDIF

!IF ("$(EFI_SPECIFICATION_VERSION)" >= "0x0002000A")
#
# Note: currently -ppflag option is only available for UefiStrGather
# Note: /GS- will cause warning for preprocess, so filter it out from STRGATHER_PPFLAG
#
STRGATHER_PPFLAG = $(C_FLAGS: /GS-=)
STRGATHER_FLAGS  = $(STRGATHER_FLAGS) -ppflag "$(STRGATHER_PPFLAG)" -oh $(DEST_DIR)\$(BASE_NAME)StrDefs.h
!ENDIF

$(DEST_DIR)\$(BASE_NAME).sdb : $(SDB_FILES) $(SOURCE_FILES)
  $(STRGATHER) -scan -vdbr $(STRGATHER_FLAGS) -od $(DEST_DIR)\$(BASE_NAME).sdb \
    -skipext .uni -skipext .h $(SOURCE_FILES)

$(DEST_DIR)\$(BASE_NAME)Strings.c : $(DEST_DIR)\$(BASE_NAME).sdb
  $(STRGATHER) -dump $(LANGUAGE_FLAGS) -bn $(BASE_NAME)Strings -db $(DEST_DIR)\$(BASE_NAME).sdb \
    -oc $(DEST_DIR)\$(BASE_NAME)Strings.c

$(DEST_DIR)\$(BASE_NAME)StrDefs.h : $(DEST_DIR)\$(BASE_NAME).sdb
  $(STRGATHER) -dump $(LANGUAGE_FLAGS) -bn $(BASE_NAME)Strings -db $(DEST_DIR)\$(BASE_NAME).sdb \
    -oh $(DEST_DIR)\$(BASE_NAME)StrDefs.h

$(DEST_DIR)\$(BASE_NAME)Strings.hpk : $(DEST_DIR)\$(BASE_NAME).sdb
  $(STRGATHER) -dump $(LANGUAGE_FLAGS) -bn $(BASE_NAME)Strings -db $(DEST_DIR)\$(BASE_NAME).sdb \
    -hpk $(DEST_DIR)\$(BASE_NAME)Strings.hpk

OBJECTS = $(OBJECTS) $(DEST_DIR)\$(BASE_NAME)Strings.obj

$(DEST_DIR)\$(BASE_NAME)Strings.obj : $(DEST_DIR)\$(BASE_NAME)Strings.c $(INF_FILENAME) $(ALL_DEPS)
  $(CC) $(C_FLAGS) $(DEST_DIR)\$(BASE_NAME)Strings.c

LOCALIZE_TARGETS = $(DEST_DIR)\$(BASE_NAME)StrDefs.h $(LOCALIZE_TARGETS)

!ENDIF

#
# If we have any objects associated with this component, then we're
# going to build a local library from them.
#
!IFNDEF OBJECTS
!ERROR No source files to build were defined in the INF file
!ENDIF

TARGET_LOCAL_LIB  = $(DEST_DIR)\$(BASE_NAME)Local.lib

#
# LIB all the object files into our (local) target lib file. Put
# a dependency on the component's INF file in case it changes.
#
$(TARGET_LOCAL_LIB) : $(OBJECTS)  $(INF_FILENAME) $(ENV_DEPS)
  $(LIB) $(LIB_FLAGS) $(OBJECTS) /OUT:$@

#
# Defines for standard intermediate files and build targets
#
TARGET_DLL      = $(BIN_DIR)\$(BASE_NAME).dll
TARGET_EFI      = $(BIN_DIR)\$(BASE_NAME).efi
TARGET_DPX      = $(DEST_DIR)\$(BASE_NAME).dpx
TARGET_UI       = $(DEST_DIR)\$(BASE_NAME).ui
TARGET_VER      = $(DEST_DIR)\$(BASE_NAME).ver
TARGET_MAP      = $(BIN_DIR)\$(BASE_NAME).map
TARGET_PDB      = $(BIN_DIR)\$(BASE_NAME).pdb
TARGET_SYM      = $(BIN_DIR)\$(BASE_NAME).sym

#
# Target executable section extension depends on the component type.
# Only define "TARGET_DXE_DPX" if it's a combined peim driver.
#
!IF "$(COMPONENT_TYPE)" == "PIC_PEIM"
TARGET_PE32 = $(DEST_DIR)\$(BASE_NAME).pic
!ELSE
TARGET_PE32 = $(DEST_DIR)\$(BASE_NAME).pe32
!ENDIF

#
# Target FFS file extension depends on the component type
# Also define "TARGET_DXE_DPX" if it's a combined PEIM driver.
#
SUBSYSTEM = EFI_BOOT_SERVICE_DRIVER

!IF "$(COMPONENT_TYPE)" == "APPLICATION"
TARGET_FFS_FILE = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).app
SUBSYSTEM       = EFI_APPLICATION
!ELSE IF "$(COMPONENT_TYPE)" == "PEI_CORE"
TARGET_FFS_FILE = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).pei
!ELSE IF "$(COMPONENT_TYPE)" == "PE32_PEIM"
TARGET_FFS_FILE = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).pei
!ELSE IF "$(COMPONENT_TYPE)" == "RELOCATABLE_PEIM"
TARGET_FFS_FILE = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).pei
!ELSE IF "$(COMPONENT_TYPE)" == "PIC_PEIM"
TARGET_FFS_FILE = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).pei
!ELSE IF "$(COMPONENT_TYPE)" == "COMBINED_PEIM_DRIVER"
TARGET_FFS_FILE = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).pei
TARGET_DXE_DPX  = $(DEST_DIR)\$(BASE_NAME).dpxd
!ELSE
TARGET_FFS_FILE = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).dxe
!ENDIF

#
# Different methods to build section based on if PIC_PEIM
#
!IF "$(COMPONENT_TYPE)" == "PIC_PEIM"

$(TARGET_PE32) : $(TARGET_DLL)
  $(PE2BIN) $(TARGET_DLL) $(DEST_DIR)\$(BASE_NAME).TMP
#
# BUGBUG: Build PEIM header, needs to go away with new PEI.
#
  $(TEMPGENSECTION) -P $(SOURCE_DIR)\$(BASE_NAME).INF -I $(DEST_DIR)\$(BASE_NAME).TMP -O $(TARGET_PIC_PEI).tmp -M $(TARGET_MAP) -S EFI_SECTION_TYPE_NO_HEADER
  $(GENSECTION) -I $(TARGET_PIC_PEI).tmp -O $(TARGET_PE32) -S EFI_SECTION_PIC
  del $(DEST_DIR)\$(BASE_NAME).TMP

!ELSE

$(TARGET_PE32) : $(TARGET_EFI)
  $(GENSECTION) -I $(TARGET_EFI) -O $(TARGET_PE32) -S EFI_SECTION_PE32

#
# $(DEP_TARGETS) are not needed for binary build.
#
!IF "$(BINARY)" == "TRUE"
DEP_TARGETS=
CREATEDEPS=
!ENDIF

#
# Build module to generate *.efi file from source code or binary file. 
#
!IF (("$(BINARY)" == "TRUE") && EXIST($(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).efi))
LOCALIZE_TARGETS=
$(TARGET_EFI) : $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).efi
  copy $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).efi $(TARGET_EFI) /Y
  if exist $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).pdb \
  copy $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).pdb $(TARGET_PDB) /Y
!ELSE
$(TARGET_EFI) : $(TARGET_DLL) $(INF_FILENAME)
  $(FWIMAGE) -t 0 $(COMPONENT_TYPE) $(TARGET_DLL) $(TARGET_EFI)
!IF ("$(EFI_BINARY_BUILD)" == "YES")
  if not exist $(EFI_PLATFORM_BIN)\$(PROCESSOR) mkdir $(EFI_PLATFORM_BIN)\$(PROCESSOR)
  if exist $(TARGET_EFI) copy $(TARGET_EFI) $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).efi /Y
  if exist $(TARGET_PDB) copy $(TARGET_PDB) $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).pdb /Y
!ENDIF
!ENDIF

!ENDIF

#
# Link all objects and libs to create the executable
#
$(TARGET_DLL) : $(TARGET_LOCAL_LIB) $(LIBS) $(INF_FILENAME) $(ENV_DEPS)
  $(LINK) $(LINK_FLAGS_DLL) $(LIBS) /ENTRY:$(IMAGE_ENTRY_POINT) \
     $(TARGET_LOCAL_LIB) /OUT:$(TARGET_DLL) /MAP:$(TARGET_MAP) \
     /PDB:$(TARGET_PDB) 
  $(SETSTAMP) $(TARGET_DLL) $(BUILD_DIR)\GenStamp.txt
!IF "$(EFI_GENERATE_SYM_FILE)" == "YES"
  if exist $(TARGET_PDB) $(PE2SYM) $(TARGET_PDB) $(TARGET_SYM)
!ENDIF

!IF "$(EFI_ZERO_DEBUG_DATA)" == "YES"
  $(ZERODEBUGDATA) $(TARGET_DLL)
!ENDIF

#
# Create the user interface section
#
$(TARGET_UI) : $(INF_FILENAME)
  $(GENSECTION) -O $(TARGET_UI) -S EFI_SECTION_USER_INTERFACE -A "$(BASE_NAME)"

#
# Create the version section
#
!IF "$(BUILD_NUMBER)" != ""
!IF "$(VERSION_STRING)" != ""
$(TARGET_VER) : $(INF_FILENAME)
  $(GENSECTION) -O $(TARGET_VER) -S EFI_SECTION_VERSION -V $(BUILD_NUMBER) -A "$(VERSION_STRING)"
!ELSE
$(TARGET_VER) : $(INF_FILENAME)
  $(GENSECTION) -O $(TARGET_VER) -S EFI_SECTION_VERSION -V $(BUILD_NUMBER)
!ENDIF
!ELSE
$(TARGET_VER) : $(INF_FILENAME)
  echo.>$(TARGET_VER)
  type $(TARGET_VER)>$(TARGET_VER)
!ENDIF

#
# Makefile entries to create the dependency expression section.
# Use the DPX file from the source directory unless an override file
# was specified.
# If no DPX source file was specified, then create an empty file to
# be used.
#
!IF "$(DPX_SOURCE)" != ""
DPX_SOURCE_FILE = $(SOURCE_DIR)\$(DPX_SOURCE)
!ENDIF

!IF "$(DPX_SOURCE_OVERRIDE)" != ""
DPX_SOURCE_FILE = $(DPX_SOURCE_OVERRIDE)
!ENDIF

!IF "$(DPX_SOURCE_FILE)" != ""
!IF EXIST ($(DPX_SOURCE_FILE))

#
# Add build dependency check
#
DEP_FILE    = $(DEST_DIR)\$(BASE_NAME)dxs.dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"

!IF EXIST($(TARGET_DPX))
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(BASE_NAME)dxs.dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(TARGET_DPX)
  $(MAKEDEPS) -f $(DPX_SOURCE_FILE) $(DEP_FLAGS)

!ENDIF

$(TARGET_DPX) : $(DPX_SOURCE_FILE) $(INF_FILENAME)
!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"
  $(CC) $(C_FLAGS_DPX) $(DPX_SOURCE_FILE) > $*.tmp1
!ELSE
  -$(CC) $(C_FLAGS_DPX) $(DPX_SOURCE_FILE) /showIncludes > $*.tmp1 2> $(DEST_DIR)\$(BASE_NAME)dxs.cl
  @$(MAKEDEPS) -f $(DEST_DIR)\$(BASE_NAME)dxs.cl $(DEP_FLAGS2)
!ENDIF
  $(GENDEPEX) -I $*.tmp1 -O $*.tmp2
  $(GENSECTION) -I $*.tmp2 -O $@ -S $(DEPEX_TYPE)
  del $*.tmp1 > NUL
  del $*.tmp2 > NUL
!ELSE
!ERROR Dependency expression source file "$(DPX_SOURCE_FILE)" does not exist.
!ENDIF
!ELSE
$(TARGET_DPX) : $(INF_FILENAME)
  echo. > $(TARGET_DPX)
  type $(TARGET_DPX) > $(TARGET_DPX)
!ENDIF

#
# Makefile entries for DXE DPX for combined PEIM drivers.
# If a DXE_DPX_SOURCE file was specified in the INF file, use it. Otherwise 
# create an empty file and use it as a DPX file.
#
!IF "$(COMPONENT_TYPE)" == "COMBINED_PEIM_DRIVER"
!IF "$(DXE_DPX_SOURCE)" != ""
!IF EXIST ($(SOURCE_DIR)\$(DXE_DPX_SOURCE))

#
# Add build dependency check
#
DEP_FILE    = $(DEST_DIR)\$(BASE_NAME)dxs2.dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"

!IF EXIST($(TARGET_DXE_DPX))
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(BASE_NAME)dxs2.dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(TARGET_DXE_DPX)
  $(MAKEDEPS) -f $(SOURCE_DIR)\$(DXE_DPX_SOURCE) $(DEP_FLAGS)

!ENDIF

$(TARGET_DXE_DPX) : $(SOURCE_DIR)\$(DXE_DPX_SOURCE) $(INF_FILENAME)
!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"
  $(CC) $(C_FLAGS_DPX) $(SOURCE_DIR)\$(DXE_DPX_SOURCE) > $*.tmp1
!ELSE
  -$(CC) $(C_FLAGS_DPX) $(SOURCE_DIR)\$(DXE_DPX_SOURCE) /showIncludes > $*.tmp1 2> $(DEST_DIR)\$(BASE_NAME)dxs2.cl
  @$(MAKEDEPS) -f $(DEST_DIR)\$(BASE_NAME)dxs2.cl $(DEP_FLAGS2)
!ENDIF
  $(GENDEPEX) -I $*.tmp1 -O $*.tmp2
  $(GENSECTION) -I $*.tmp2 -O $@ -S EFI_SECTION_DXE_DEPEX
  del $*.tmp1 > NUL
  del $*.tmp2 > NUL
!ELSE
!ERROR Dependency expression source file "$(SOURCE_DIR)\$(DXE_DPX_SOURCE)" does not exist.
!ENDIF
!ELSE
$(TARGET_DXE_DPX) : $(INF_FILENAME)
  echo. > $(TARGET_DXE_DPX)
  type $(TARGET_DXE_DPX) > $(TARGET_DXE_DPX)
!ENDIF
!ENDIF

#
# Describe how to build the HII export file from all the input HII pack files.
# Use the FFS file GUID for the package GUID in the export file. Only used
# when multiple VFR share strings.
#
$(DEST_DIR)\$(BASE_NAME).hii : $(HII_PACK_FILES)
  $(HIIPACK) create -g $(FILE_GUID) -p $(HII_PACK_FILES) -o $(DEST_DIR)\$(BASE_NAME).hii

#
# If the build calls for creating an FFS file with the IFR included as
# a separate binary (not compiled into the driver), then build the binary
# section now. Note that the PACKAGE must be set correctly to actually get
# this IFR section pulled into the FFS file.
#
!IF ("$(HII_IFR_PACK_FILES)" != "")

$(DEST_DIR)\$(BASE_NAME)IfrBin.sec : $(HII_IFR_PACK_FILES)
  $(HIIPACK) create -novarpacks -p $(HII_IFR_PACK_FILES) -o $(DEST_DIR)\$(BASE_NAME)IfrBin.hii
  $(GENSECTION) -I $(DEST_DIR)\$(BASE_NAME)IfrBin.hii -O $(DEST_DIR)\$(BASE_NAME)IfrBin.sec -S EFI_SECTION_RAW

BIN_TARGETS = $(BIN_TARGETS) $(DEST_DIR)\$(BASE_NAME)IfrBin.sec

!ENDIF

#
# Build a FFS file from the sections and package
#
$(TARGET_FFS_FILE) : $(TARGET_PE32) $(TARGET_DPX) $(TARGET_UI) $(TARGET_VER) $(TARGET_DXE_DPX) $(PACKAGE_FILENAME)
#
# Some of our components require padding to align code
#
!IF "$(PROCESSOR)" == "IPF"
!IF "$(COMPONENT_TYPE)" == "PIC_PEIM" || "$(COMPONENT_TYPE)" == "PE32_PEIM" || "$(COMPONENT_TYPE)" == "RELOCATABLE_PEIM" || "$(COMPONENT_TYPE)" == "SECURITY_CORE" || "$(COMPONENT_TYPE)" == "PEI_CORE" || "$(COMPONENT_TYPE)" == "COMBINED_PEIM_DRIVER"
  copy $(BIN_DIR)\Blank.pad $(DEST_DIR)
!ENDIF
!ENDIF
  $(GENFFSFILE) -B $(DEST_DIR) -P1 $(PACKAGE_FILENAME) -V

!IF "$(CREATEDEPS)"=="YES"
all : $(DEP_TARGETS)
  $(MAKE) -f $(MAKEFILE_NAME) all
!ELSE
all : $(LOCALIZE_TARGETS) $(BIN_TARGETS) $(TARGET_FFS_FILE) $(DEP_TARGETS)
!ENDIF

#
# Remove the generated temp and final files for this modules.
#
clean :
!IF ("$(FILE_GUID)" != "")
  @if exist $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).* del $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).*
!ENDIF
  @if exist $(BIN_DIR)\$(BASE_NAME).* del $(BIN_DIR)\$(BASE_NAME).*
  @del /q $(DEST_OUTPUT_DIRS) 

[=============================================================================]
[Build.Ia32.TE_PEIM,Build.Ipf.TE_PEIM,Build.x64.TE_PEIM]
#
# Define the library file we'll build if we have any objects defined.
#
!IFDEF OBJECTS
TARGET_LOCAL_LIB  = $(DEST_DIR)\$(BASE_NAME)Local.lib
#
# LIB all the object files into our (local) target lib file. Put
# a dependency on the component's INF file in case it changes.
#
$(TARGET_LOCAL_LIB) : $(OBJECTS)  $(INF_FILENAME) $(ENV_DEPS)
  $(LIB) $(LIB_FLAGS) $(OBJECTS) /OUT:$@

!ELSE
!ERROR No source files to build were defined in the INF file
!ENDIF

#
# Defines for standard intermediate files and build targets
#
TARGET_DLL        = $(BIN_DIR)\$(BASE_NAME).dll
TARGET_EFI        = $(BIN_DIR)\$(BASE_NAME).efi
TARGET_DPX        = $(DEST_DIR)\$(BASE_NAME).dpx
TARGET_UI         = $(DEST_DIR)\$(BASE_NAME).ui
TARGET_VER        = $(DEST_DIR)\$(BASE_NAME).ver
TARGET_MAP        = $(BIN_DIR)\$(BASE_NAME).map
TARGET_PDB        = $(BIN_DIR)\$(BASE_NAME).pdb
TARGET_SYM        = $(BIN_DIR)\$(BASE_NAME).sym
TARGET_TE         = $(BIN_DIR)\$(BASE_NAME).te
TARGET_PE32       = $(DEST_DIR)\$(BASE_NAME).pe32
TARGET_TES        = $(DEST_DIR)\$(BASE_NAME).tes
TARGET_FFS_FILE   = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).pei

#
# Create our TE section from our TE file
#
$(TARGET_TES) : $(TARGET_TE)
  $(GENSECTION) -I $(TARGET_TE) -O $(TARGET_TES) -S EFI_SECTION_TE

#
# $(DEP_TARGETS) are not needed for binary build.
#
!IF "$(BINARY)" == "TRUE"
DEP_TARGETS=
CREATEDEPS=
!ENDIF

#
# Build module to generate *.efi file from source code or binary file.
#
!IF (("$(BINARY)" == "TRUE") && EXIST($(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).efi))
$(TARGET_EFI) : $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).efi
  copy $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).efi $(TARGET_EFI) /Y
  if exist $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).pdb \
  copy $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).pdb $(TARGET_PDB) /Y
!ELSE
$(TARGET_EFI) : $(TARGET_DLL) $(INF_FILENAME)
  $(FWIMAGE) $(COMPONENT_TYPE) $(TARGET_DLL) $(TARGET_EFI)
!IF ("$(EFI_BINARY_BUILD)" == "YES")
  if not exist $(EFI_PLATFORM_BIN)\$(PROCESSOR) mkdir $(EFI_PLATFORM_BIN)\$(PROCESSOR)
  if exist $(TARGET_EFI) copy $(TARGET_EFI) $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).efi /Y
  if exist $(TARGET_PDB) copy $(TARGET_PDB) $(EFI_PLATFORM_BIN)\$(PROCESSOR)\$(BASE_NAME).pdb /Y
!ENDIF
!ENDIF

#
# Run GenTEImage on the built .efi file to create our TE file.
#
$(TARGET_TE) : $(TARGET_EFI) 
  $(GENTEIMAGE) -o $(TARGET_TE) $(TARGET_EFI)

#
# Link all objects and libs to create the executable
#
$(TARGET_DLL) : $(TARGET_LOCAL_LIB) $(LIBS) $(INF_FILENAME) $(ENV_DEPS)
  $(LINK) $(LINK_FLAGS_DLL) $(LIBS) /ENTRY:$(IMAGE_ENTRY_POINT) \
     $(TARGET_LOCAL_LIB) /OUT:$(TARGET_DLL) /MAP:$(TARGET_MAP) \
     /PDB:$(TARGET_PDB)
  $(SETSTAMP) $(TARGET_DLL) $(BUILD_DIR)\GenStamp.txt
!IF "$(EFI_GENERATE_SYM_FILE)" == "YES"
  if exist $(TARGET_PDB) $(PE2SYM) $(TARGET_PDB) $(TARGET_SYM)
!ENDIF

!IF "$(EFI_ZERO_DEBUG_DATA)" == "YES"
  $(ZERODEBUGDATA) $(TARGET_DLL)
!ENDIF

#
# Create the user interface section
#
$(TARGET_UI) : $(INF_FILENAME)
  $(GENSECTION) -O $(TARGET_UI) -S EFI_SECTION_USER_INTERFACE -A "$(BASE_NAME)"

#
# Create the version section
#
!IF "$(BUILD_NUMBER)" != ""
!IF "$(VERSION_STRING)" != ""
$(TARGET_VER) : $(INF_FILENAME)
  $(GENSECTION) -O $(TARGET_VER) -S EFI_SECTION_VERSION -V $(BUILD_NUMBER) -A "$(VERSION_STRING)"
!ELSE
$(TARGET_VER) : $(INF_FILENAME)
  $(GENSECTION) -O $(TARGET_VER) -S EFI_SECTION_VERSION -V $(BUILD_NUMBER)
!ENDIF
!ELSE
$(TARGET_VER) : $(INF_FILENAME)
  echo.>$(TARGET_VER)
  type $(TARGET_VER)>$(TARGET_VER)
!ENDIF

#
# Makefile entries to create the dependency expression section.
# Use the DPX file from the source directory unless an override file
# was specified.
# If no DPX source file was specified, then create an empty file to
# be used.
#
!IF "$(DPX_SOURCE)" != ""
DPX_SOURCE_FILE = $(SOURCE_DIR)\$(DPX_SOURCE)
!ENDIF

!IF "$(DPX_SOURCE_OVERRIDE)" != ""
DPX_SOURCE_FILE = $(DPX_SOURCE_OVERRIDE)
!ENDIF

!IF "$(DPX_SOURCE_FILE)" != ""
!IF EXIST ($(DPX_SOURCE_FILE))

#
# Add build dependency check
#
DEP_FILE    = $(DEST_DIR)\$(BASE_NAME)dxs.dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"

!IF EXIST($(TARGET_DPX))
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(BASE_NAME)dxs.dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(TARGET_DPX)
  $(MAKEDEPS) -f $(DPX_SOURCE_FILE) $(DEP_FLAGS)

!ENDIF

$(TARGET_DPX) : $(DPX_SOURCE_FILE) $(INF_FILENAME)
!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"
  $(CC) $(C_FLAGS_DPX) $(DPX_SOURCE_FILE) > $*.tmp1
!ELSE
  -$(CC) $(C_FLAGS_DPX) $(DPX_SOURCE_FILE) /showIncludes > $*.tmp1 2> $(DEST_DIR)\$(BASE_NAME)dxs.cl
  @$(MAKEDEPS) -f $(DEST_DIR)\$(BASE_NAME)dxs.cl $(DEP_FLAGS2)
!ENDIF
  $(GENDEPEX) -I $*.tmp1 -O $*.tmp2
  $(GENSECTION) -I $*.tmp2 -O $@ -S $(DEPEX_TYPE)
  del $*.tmp1 > NUL
  del $*.tmp2 > NUL
!ELSE
!ERROR Dependency expression source file "$(DPX_SOURCE_FILE)" does not exist.
!ENDIF
!ELSE
$(TARGET_DPX) : $(INF_FILENAME)
  echo. > $(TARGET_DPX)
  type $(TARGET_DPX) > $(TARGET_DPX)
!ENDIF

#
# Build an FFS file from the sections and package
#
$(TARGET_FFS_FILE) : $(TARGET_TES) $(TARGET_DPX) $(TARGET_UI) $(TARGET_VER) $(PACKAGE_FILENAME)
  $(GENFFSFILE) -B $(DEST_DIR) -P1 $(PACKAGE_FILENAME) -V

!IF "$(CREATEDEPS)"=="YES"
all : $(DEP_TARGETS)
  $(MAKE) -f $(MAKEFILE_NAME) all
!ELSE
all : $(TARGET_FFS_FILE) $(DEP_TARGETS)
!ENDIF

#
# Remove the generated temp and final files for this modules.
#
clean :
!IF ("$(FILE_GUID)" != "")
  @if exist $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).* del $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).*
!ENDIF
  @if exist $(BIN_DIR)\$(BASE_NAME).* del $(BIN_DIR)\$(BASE_NAME).*
  @del /q $(DEST_OUTPUT_DIRS) 

[=============================================================================]
#
# These are the commands to build EBC EFI targets
#
[=============================================================================]
[Build.Ebc.BS_DRIVER|RT_DRIVER|APPLICATION]

#
# Add the EBC library to our list of libs
#
LIBS = $(LIBS) $(EBC_TOOLS_PATH)\lib\EbcLib.lib 

!IF "$(LOCALIZE)" == "YES"

!IF (("$(EFI_GENERATE_HII_RESOURCE)" == "YES") && ("$(EFI_SPECIFICATION_VERSION)" >= "0x0002000A"))
#
# This will generate HII resource section in PE/COFF image.
#
# Note: when HII package list is built into resource section, Driver no longer
# refer to C array generated by VfrCompiler ($(FILE_NAME)Bin) and StrGather
# ($(BASE_NAME)Strings); while in current build rule, those C array objects
# will still be linked with the Driver, so please turn on link flag "/OPT:REF"
# to minimize the code size.
#
HII_PACK_FILES   = $(HII_PACK_FILES) $(DEST_DIR)\$(BASE_NAME)Strings.hpk
LOCALIZE_TARGETS = $(LOCALIZE_TARGETS) $(DEST_DIR)\$(BASE_NAME).res
OBJECTS          = $(OBJECTS) $(DEST_DIR)\$(BASE_NAME).res

$(DEST_DIR)\$(BASE_NAME).rc : $(HII_PACK_FILES)
  $(HIIPACK) -g $(FILE_GUID) $(HII_PACK_FILES) -rc $(DEST_DIR)\$(BASE_NAME).rc -hii $(DEST_DIR)\$(BASE_NAME).hii

$(DEST_DIR)\$(BASE_NAME).res : $(DEST_DIR)\$(BASE_NAME).rc
  $(RC) /fo $(DEST_DIR)\$(BASE_NAME).res $(DEST_DIR)\$(BASE_NAME).rc

!ENDIF

!IF (("$(EFI_GENERATE_HII_EXPORT)" == "YES") && ("$(EFI_SPECIFICATION_VERSION)" < "0x0002000A"))
#
# There will be one HII pack containing all the strings. Add that file
# to the list of HII pack files we'll use to create our final HII export file.
#
HII_PACK_FILES = $(HII_PACK_FILES) $(DEST_DIR)\$(BASE_NAME)Strings.hpk

LOCALIZE_TARGETS  = $(LOCALIZE_TARGETS) $(DEST_DIR)\$(BASE_NAME).hii
!ENDIF

!IF ("$(EFI_SPECIFICATION_VERSION)" >= "0x0002000A")
#
# Note: currently -ppflag option is only available for UefiStrGather
# Note: /GS- will cause warning for preprocess, so filter it out from STRGATHER_PPFLAG
#
STRGATHER_PPFLAG = $(EBC_C_FLAGS: /GS-=)
STRGATHER_FLAGS  = $(STRGATHER_FLAGS) -ppflag "$(STRGATHER_PPFLAG)" -oh $(DEST_DIR)\$(BASE_NAME)StrDefs.h
!ENDIF

$(DEST_DIR)\$(BASE_NAME).sdb : $(SDB_FILES) $(SOURCE_FILES)
  $(STRGATHER) -scan -vdbr $(STRGATHER_FLAGS) -od $(DEST_DIR)\$(BASE_NAME).sdb \
    -skipext .uni -skipext .h $(SOURCE_FILES)

$(DEST_DIR)\$(BASE_NAME)Strings.c : $(DEST_DIR)\$(BASE_NAME).sdb
  $(STRGATHER) -dump $(LANGUAGE_FLAGS) -bn $(BASE_NAME)Strings -db $(DEST_DIR)\$(BASE_NAME).sdb \
    -oc $(DEST_DIR)\$(BASE_NAME)Strings.c

$(DEST_DIR)\$(BASE_NAME)StrDefs.h : $(DEST_DIR)\$(BASE_NAME).sdb
  $(STRGATHER) -dump $(LANGUAGE_FLAGS) -bn $(BASE_NAME)Strings -db $(DEST_DIR)\$(BASE_NAME).sdb \
    -oh $(DEST_DIR)\$(BASE_NAME)StrDefs.h

$(DEST_DIR)\$(BASE_NAME)Strings.hpk : $(DEST_DIR)\$(BASE_NAME).sdb
  $(STRGATHER) -dump $(LANGUAGE_FLAGS) -bn $(BASE_NAME)Strings -db $(DEST_DIR)\$(BASE_NAME).sdb \
    -hpk $(DEST_DIR)\$(BASE_NAME)Strings.hpk

OBJECTS = $(OBJECTS) $(DEST_DIR)\$(BASE_NAME)Strings.obj

$(DEST_DIR)\$(BASE_NAME)Strings.obj : $(DEST_DIR)\$(BASE_NAME)Strings.c $(INF_FILENAME) $(ALL_DEPS)
  $(EBC_CC) $(EBC_C_FLAGS) $(DEST_DIR)\$(BASE_NAME)Strings.c

LOCALIZE_TARGETS = $(DEST_DIR)\$(BASE_NAME)StrDefs.h $(LOCALIZE_TARGETS)

!ENDIF

#
# If building an application, then the target is a .app, not .dxe
#
!IF "$(COMPONENT_TYPE)" == "APPLICATION"
TARGET_FFS_FILE = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).app
SUBSYSTEM       = EFI_APPLICATION
!ELSE
TARGET_FFS_FILE = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).dxe
SUBSYSTEM       = EFI_BOOT_SERVICE_DRIVER
!ENDIF

#
# Defines for standard intermediate files and build targets
#
TARGET_EFI  = $(BIN_DIR)\$(BASE_NAME).efi
TARGET_DPX  = $(DEST_DIR)\$(BASE_NAME).dpx
TARGET_UI   = $(DEST_DIR)\$(BASE_NAME).ui
TARGET_VER  = $(DEST_DIR)\$(BASE_NAME).ver
TARGET_MAP  = $(BIN_DIR)\$(BASE_NAME).map
TARGET_PDB  = $(BIN_DIR)\$(BASE_NAME).pdb
TARGET_PE32 = $(DEST_DIR)\$(BASE_NAME).pe32
TARGET_DLL  = $(BIN_DIR)\$(BASE_NAME).dll

#
# First link all the objects and libs together to make a .dll file
#
$(TARGET_DLL) : $(OBJECTS) $(LIBS) $(INF_FILENAME) $(ENV_DEPS)
  $(EBC_LINK) $(EBC_LINK_FLAGS) /SUBSYSTEM:$(SUBSYSTEM) /ENTRY:EfiStart \
    $(OBJECTS) $(LIBS) /OUT:$(TARGET_DLL) /MAP:$(TARGET_MAP)
  $(SETSTAMP) $(TARGET_DLL) $(BUILD_DIR)\GenStamp.txt
!IF "$(EFI_ZERO_DEBUG_DATA)" == "YES"
  $(ZERODEBUGDATA) $(TARGET_DLL)
!ENDIF

#
# Now take the .dll file and make a .efi file
#
$(TARGET_EFI) : $(TARGET_DLL) $(INF_FILENAME)
  $(FWIMAGE) -t 0 $(COMPONENT_TYPE) $(TARGET_DLL) $(TARGET_EFI)

#
# Now take the .efi file and make a .pe32 section
#
$(TARGET_PE32) : $(TARGET_EFI) 
  $(GENSECTION) -I $(TARGET_EFI) -O $(TARGET_PE32) -S EFI_SECTION_PE32

#
# Create the user interface section
#
$(TARGET_UI) : $(INF_FILENAME)
  $(GENSECTION) -O $(TARGET_UI) -S EFI_SECTION_USER_INTERFACE -A "$(BASE_NAME)"

#
# Create the version section
#
!IF "$(BUILD_NUMBER)" != ""
!IF "$(VERSION_STRING)" != ""
$(TARGET_VER) : $(INF_FILENAME)
  $(GENSECTION) -O $(TARGET_VER) -S EFI_SECTION_VERSION -V $(BUILD_NUMBER) -A "$(VERSION_STRING)"
!ELSE
$(TARGET_VER) : $(INF_FILENAME)
  $(GENSECTION) -O $(TARGET_VER) -S EFI_SECTION_VERSION -V $(BUILD_NUMBER)
!ENDIF
!ELSE
$(TARGET_VER) : $(INF_FILENAME)
  echo. > $(TARGET_VER)
  type $(TARGET_VER) > $(TARGET_VER)
!ENDIF

#
# Makefile entries to create the dependency expression section.
# Use the DPX file from the source directory unless an override file
# was specified.
# If no DPX source file was specified, then create an empty file to
# be used.
#
!IF "$(DPX_SOURCE)" != ""
DPX_SOURCE_FILE = $(SOURCE_DIR)\$(DPX_SOURCE)
!ENDIF

!IF "$(DPX_SOURCE_OVERRIDE)" != ""
DPX_SOURCE_FILE = $(DPX_SOURCE_OVERRIDE)
!ENDIF

!IF "$(DPX_SOURCE_FILE)" != ""
!IF EXIST ($(DPX_SOURCE_FILE))

#
# Add build dependency check
#
DEP_FILE    = $(DEST_DIR)\$(BASE_NAME)dxs.dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"

!IF EXIST($(TARGET_DPX))
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(BASE_NAME)dxs.dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(TARGET_DPX)
  $(MAKEDEPS) -f $(DPX_SOURCE_FILE) $(DEP_FLAGS)

!ENDIF

$(TARGET_DPX) : $(DPX_SOURCE_FILE) $(INF_FILENAME)
!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"
  $(CC) $(C_FLAGS_DPX) $(DPX_SOURCE_FILE) > $*.tmp1
!ELSE
  -$(CC) $(C_FLAGS_DPX) $(DPX_SOURCE_FILE) /showIncludes > $*.tmp1 2> $(DEST_DIR)\$(BASE_NAME)dxs.cl
  @$(MAKEDEPS) -f $(DEST_DIR)\$(BASE_NAME)dxs.cl $(DEP_FLAGS2)
!ENDIF
  $(GENDEPEX) -I $*.tmp1 -O $*.tmp2
  $(GENSECTION) -I $*.tmp2 -O $@ -S $(DEPEX_TYPE)
  del $*.tmp1 > NUL
  del $*.tmp2 > NUL
!ELSE
!ERROR Dependency expression source file "$(DPX_SOURCE_FILE)" does not exist.
!ENDIF
!ELSE
$(TARGET_DPX) : $(INF_FILENAME)
  echo. > $(TARGET_DPX)
  type $(TARGET_DPX) > $(TARGET_DPX)
!ENDIF

#
# Describe how to build the HII export file from all the input HII pack files.
# Use the FFS file GUID for the package GUID in the export file. Only used
# when multiple VFR share strings.
#
$(DEST_DIR)\$(BASE_NAME).hii : $(HII_PACK_FILES)
  $(HIIPACK) create -g $(FILE_GUID) -p $(HII_PACK_FILES) -o $(DEST_DIR)\$(BASE_NAME).hii

#
# If the build calls for creating an FFS file with the IFR included as
# a separate binary (not compiled into the driver), then build the binary
# section now. Note that the PACKAGE must be set correctly to actually get
# this IFR section pulled into the FFS file.
#
!IF ("$(HII_IFR_PACK_FILES)" != "")

$(DEST_DIR)\$(BASE_NAME)IfrBin.sec : $(HII_IFR_PACK_FILES)
  $(HIIPACK) create -novarpacks -p $(HII_IFR_PACK_FILES) -o $(DEST_DIR)\$(BASE_NAME)IfrBin.hii
  $(GENSECTION) -I $(DEST_DIR)\$(BASE_NAME)IfrBin.hii -O $(DEST_DIR)\$(BASE_NAME)IfrBin.sec -S EFI_SECTION_RAW

BIN_TARGETS = $(BIN_TARGETS) $(DEST_DIR)\$(BASE_NAME)IfrBin.sec

!ENDIF

#
# Build an FFS file from the sections and package
#
$(TARGET_FFS_FILE) : $(TARGET_PE32) $(TARGET_DPX) $(TARGET_UI) $(TARGET_VER) $(PACKAGE_FILENAME)
  $(GENFFSFILE) -B $(DEST_DIR) -P1 $(PACKAGE_FILENAME) -V

!IF "$(CREATEDEPS)"=="YES"
all : $(DEP_TARGETS)
  $(MAKE) -f $(MAKEFILE_NAME) all
!ELSE
all : $(LOCALIZE_TARGETS) $(BIN_TARGETS) $(TARGET_FFS_FILE) $(DEP_TARGETS)
!ENDIF

#
# Remove the generated temp and final files for this modules.
#
clean :
!IF ("$(FILE_GUID)" != "")
  @if exist $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).* del $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).*
!ENDIF
  @if exist $(BIN_DIR)\$(BASE_NAME).* del $(BIN_DIR)\$(BASE_NAME).*
  @del /q $(DEST_OUTPUT_DIRS) 

[=============================================================================]
#
# These are the commands to build vendor-provided *.EFI files into an FV.
# To use them, create an INF file with BUILD_TYPE=BS_DRIVER_EFI.
# This section, as it now exists, only supports boot service drivers.
#
[=============================================================================]
[Build.Ia32.BS_DRIVER_EFI|RT_DRIVER_EFI|APPLICATION_EFI|PE32_PEIM_EFI,Build.Ipf.BS_DRIVER_EFI|RT_DRIVER_EFI|APPLICATION_EFI|PE32_PEIM_EFI,Build.Ebc.BS_DRIVER_EFI|RT_DRIVER_EFI|APPLICATION_EFI,Build.x64.BS_DRIVER_EFI|RT_DRIVER_EFI|APPLICATION_EFI|PE32_PEIM_EFI]
#
# Defines for standard intermediate files and build targets. For the source
# .efi file, take the one in the source directory if it exists. If there's not
# one there, look for one in the processor-specfic subdirectory.
#
!IF EXIST ("$(SOURCE_DIR)\$(BASE_NAME).efi")
TARGET_EFI        = $(SOURCE_DIR)\$(BASE_NAME).efi
!ELSEIF EXIST ("$(SOURCE_DIR)\$(PROCESSOR)\$(BASE_NAME).efi")
TARGET_EFI        = $(SOURCE_DIR)\$(PROCESSOR)\$(BASE_NAME).efi
!ELSE
!ERROR Pre-existing $(BASE_NAME).efi file not found in $(SOURCE_DIR) nor $(SOURCE_DIR)\$(PROCESSOR)
!ENDIF

TARGET_DPX        = $(DEST_DIR)\$(BASE_NAME).dpx
TARGET_UI         = $(DEST_DIR)\$(BASE_NAME).ui
TARGET_VER        = $(DEST_DIR)\$(BASE_NAME).ver
TARGET_MAP        = $(BIN_DIR)\$(BASE_NAME).map
TARGET_PDB        = $(BIN_DIR)\$(BASE_NAME).pdb
TARGET_PE32       = $(DEST_DIR)\$(BASE_NAME).pe32
TARGET_DLL        = $(BIN_DIR)\$(BASE_NAME).dll

#
# If building an application, then the target is a .app, not .dxe
#
!IF "$(COMPONENT_TYPE)" == "APPLICATION"
TARGET_FFS_FILE = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).app
!ELSE IF "$(COMPONENT_TYPE)" == "PE32_PEIM"
TARGET_FFS_FILE = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).pei
!ELSE
TARGET_FFS_FILE = $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).dxe
!ENDIF

#
# Take the .efi file and make a .pe32 file
#
$(TARGET_PE32) : $(TARGET_EFI) 
  $(GENSECTION) -I $(TARGET_EFI) -O $(TARGET_PE32) -S EFI_SECTION_PE32

#
# Create the user interface section
#
$(TARGET_UI) : $(INF_FILENAME)
  $(GENSECTION) -O $(TARGET_UI) -S EFI_SECTION_USER_INTERFACE -A "$(BASE_NAME)"

#
# Create the version section
#
!IF "$(BUILD_NUMBER)" != ""
!IF "$(VERSION_STRING)" != ""
$(TARGET_VER) : $(INF_FILENAME)
  $(GENSECTION) -O $(TARGET_VER) -S EFI_SECTION_VERSION -V $(BUILD_NUMBER) -A "$(VERSION_STRING)"
!ELSE
$(TARGET_VER) : $(INF_FILENAME)
  $(GENSECTION) -O $(TARGET_VER) -S EFI_SECTION_VERSION -V $(BUILD_NUMBER)
!ENDIF
!ELSE
$(TARGET_VER) : $(INF_FILENAME)
  echo. > $(TARGET_VER)
  type $(TARGET_VER) > $(TARGET_VER)
!ENDIF

#
# Makefile entries to create the dependency expression section.
# Use the DPX file from the source directory unless an override file
# was specified.
# If no DPX source file was specified, then create an empty file to
# be used.
#
!IF "$(DPX_SOURCE)" != ""
DPX_SOURCE_FILE = $(SOURCE_DIR)\$(DPX_SOURCE)
!ENDIF

!IF "$(DPX_SOURCE_OVERRIDE)" != ""
DPX_SOURCE_FILE = $(DPX_SOURCE_OVERRIDE)
!ENDIF

!IF "$(DPX_SOURCE_FILE)" != ""
!IF EXIST ($(DPX_SOURCE_FILE))

#
# Add build dependency check
#
DEP_FILE    = $(DEST_DIR)\$(BASE_NAME)dxs.dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"

!IF EXIST($(TARGET_DPX))
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(BASE_NAME)dxs.dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(TARGET_DPX)
  $(MAKEDEPS) -f $(DPX_SOURCE_FILE) $(DEP_FLAGS)

!ENDIF

$(TARGET_DPX) : $(DPX_SOURCE_FILE) $(INF_FILENAME)
!IF "$(EFI_USE_CL_FOR_DEP)" != "YES"
  $(CC) $(C_FLAGS_DPX) $(DPX_SOURCE_FILE) > $*.tmp1
!ELSE
  -$(CC) $(C_FLAGS_DPX) $(DPX_SOURCE_FILE) /showIncludes > $*.tmp1 2> $(DEST_DIR)\$(BASE_NAME)dxs.cl
  @$(MAKEDEPS) -f $(DEST_DIR)\$(BASE_NAME)dxs.cl $(DEP_FLAGS2)
!ENDIF
  $(GENDEPEX) -I $*.tmp1 -O $*.tmp2
  $(GENSECTION) -I $*.tmp2 -O $@ -S $(DEPEX_TYPE)
  del $*.tmp1 > NUL
  del $*.tmp2 > NUL
!ELSE
!ERROR Dependency expression source file "$(DPX_SOURCE_FILE)" does not exist.
!ENDIF
!ELSE
$(TARGET_DPX) : $(INF_FILENAME)
  echo. > $(TARGET_DPX)
  type $(TARGET_DPX) > $(TARGET_DPX)
!ENDIF

#
# Build a FFS file from the sections and package
#
$(TARGET_FFS_FILE) : $(TARGET_PE32) $(TARGET_DPX) $(TARGET_UI) $(TARGET_VER) $(PACKAGE_FILENAME)
  $(GENFFSFILE) -B $(DEST_DIR) -P1 $(PACKAGE_FILENAME) -V

all : $(TARGET_FFS_FILE)

#
# Remove the generated temp and final files for this modules.
#
clean :
!IF ("$(FILE_GUID)" != "")
  @if exist $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).* del $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).*
!ENDIF
  @if exist $(BIN_DIR)\$(BASE_NAME).* del $(BIN_DIR)\$(BASE_NAME).*
  @del /q $(DEST_OUTPUT_DIRS) 

[=============================================================================]
[Compile.Ia32.Bin|Bmp,Compile.x64.Bin|Bmp,Compile.Ipf.Bin|Bmp]
#
# We simply define the binary source file name
#
BINARY_SOURCE_FILE = $(SOURCE_FILE_NAME)

[=============================================================================]
[Build.Ia32.BINARY|Legacy16|Logo,Build.Ipf.BINARY|Legacy16|Logo,Build.x64.BINARY|Legacy16|Logo]
#
# Use GenFfsFile to convert it to an FFS file
#
$(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).ffs : $(BINARY_SOURCE_FILE) $(PACKAGE_FILENAME) $(INF_FILENAME)
  $(GENSECTION) -I $(BINARY_SOURCE_FILE) -O $(DEST_DIR)\$(BASE_NAME).sec -S EFI_SECTION_RAW
  $(GENFFSFILE) -B $(DEST_DIR) -P1 $(PACKAGE_FILENAME) -V

all : $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).ffs

#
# Remove the generated temp and final files for this modules.
#
clean :
!IF ("$(FILE_GUID)" != "")
  @if exist $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).* del $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).*
!ENDIF
  @if exist $(BIN_DIR)\$(BASE_NAME).* del $(BIN_DIR)\$(BASE_NAME).*
  @del /q $(DEST_OUTPUT_DIRS) 

[=============================================================================]
[Build.Ia32.RAWFILE|CONFIG,Build.Ipf.RAWFILE|CONFIG,Build.x64.RAWFILE|CONFIG]
#
# Use GenFfsFile to convert it to an raw FFS file
#
$(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).raw : $(BINARY_SOURCE_FILE) $(PACKAGE_FILENAME) $(INF_FILENAME)
  copy $(BINARY_SOURCE_FILE) $(DEST_DIR)\$(BASE_NAME).bin /Y
  $(GENFFSFILE) -B $(DEST_DIR) -P1 $(PACKAGE_FILENAME) -V

all : $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).raw

#
# Remove the generated temp and final files for this modules.
#
clean :
!IF ("$(FILE_GUID)" != "")
  @if exist $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).* del $(BIN_DIR)\$(FILE_GUID)-$(BASE_NAME).*
!ENDIF
  @if exist $(BIN_DIR)\$(BASE_NAME).* del $(BIN_DIR)\$(BASE_NAME).*
  @del /q $(DEST_OUTPUT_DIRS) 

[=============================================================================]
# 
# These are commands to compile unicode .uni files.
#
[=============================================================================]
[Compile.Ia32.Uni,Compile.Ipf.Uni,Compile.Ebc.Uni,Compile.x64.Uni]
#
# Emit an error message if the file's base name is the same as the
# component base name. This causes build issues.
#
!IF "$(FILE)" == "$(BASE_NAME)"
!ERROR Component Unicode string file name cannot be the same as the component BASE_NAME.
!ENDIF

#
# Always create dep file for uni file as it can be created at the same time when 
# strgather is parsing uni file.
#
DEP_FILE    = $(DEST_DIR)\$(FILE)Uni.dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

$(DEST_DIR)\$(FILE).sdb : $(SOURCE_FILE_NAME) $(INF_FILENAME)
  $(STRGATHER) -parse -newdb -db $(DEST_DIR)\$(FILE).sdb -dep $(DEP_FILE) $(INC) $(SOURCE_FILE_NAME)

SDB_FILES       = $(SDB_FILES) $(DEST_DIR)\$(FILE).sdb
STRGATHER_FLAGS = $(STRGATHER_FLAGS) -db $(DEST_DIR)\$(FILE).sdb
LOCALIZE        = YES

[=============================================================================]
[Compile.Ia32.hfr,Compile.Ipf.hfr,Compile.Ebc.hfr,Compile.x64.hfr]
[=============================================================================]
[Compile.Ia32.Vfr,Compile.Ipf.Vfr,Compile.x64.Vfr]

#
# Add build dependency check
#
DEP_FILE    = $(DEST_DIR)\$(FILE)Vfr.dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

!IF EXIST($(DEST_DIR)\$(FILE).obj)
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(FILE)Vfr.dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(DEST_DIR)\$(FILE).obj
  $(MAKEDEPS) -f $(SOURCE_FILE_NAME) $(DEP_FLAGS)

HII_PACK_FILES  = $(HII_PACK_FILES) $(DEST_DIR)\$(FILE).hpk

#
# Add a dummy command for building the HII pack file. In reality, it's built 
# below, but the C_FLAGS macro reference the target as $@, so you can't specify
# the obj and hpk files as dual targets of the same command.
#
$(DEST_DIR)\$(FILE).hpk : $(DEST_DIR)\$(FILE).obj
  
$(DEST_DIR)\$(FILE).obj : $(SOURCE_FILE_NAME) $(INF_FILENAME) $(ALL_DEPS)
  $(VFRCOMPILE) $(VFRCOMPILE_FLAGS) $(INC) -ibin -od $(DEST_DIR)\$(SOURCE_RELATIVE_PATH) \
    -l $(VFR_FLAGS) $(SOURCE_FILE_NAME)
  $(CC) $(C_FLAGS) $(DEST_DIR)\$(FILE).c

[=============================================================================]
[Compile.Ebc.Vfr]

DEP_FILE    = $(DEST_DIR)\$(FILE)Vfr.dep

!IF EXIST($(DEST_DIR)\$(FILE).obj)
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(FILE)Vfr.dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(DEST_DIR)\$(FILE).obj
  $(MAKEDEPS) -f $(SOURCE_FILE_NAME) $(DEP_FLAGS)

HII_PACK_FILES  = $(HII_PACK_FILES) $(DEST_DIR)\$(FILE).hpk

#
# Add a dummy command for building the HII pack file. In reality, it's built 
# below, but the C_FLAGS macro reference the target as $@, so you can't specify
# the obj and hpk files as dual targets of the same command.
#
$(DEST_DIR)\$(FILE).hpk : $(DEST_DIR)\$(FILE).obj
  
$(DEST_DIR)\$(FILE).obj : $(SOURCE_FILE_NAME) $(INF_FILENAME) $(ALL_DEPS)
  $(VFRCOMPILE) $(VFRCOMPILE_FLAGS) $(INC) -ibin -od $(DEST_DIR)\$(SOURCE_RELATIVE_PATH) \
    -l $(VFR_FLAGS) $(SOURCE_FILE_NAME)
  $(EBC_CC) $(EBC_C_FLAGS) $(DEST_DIR)\$(FILE).c

[=============================================================================]
#
# Commands for building IFR as uncompressed binary into the FFS file. To 
# use it, set COMPILE_SELECT=.vfr=Ifr_Bin for the component in the DSC file.
#
[=============================================================================]
[Compile.Ia32.Ifr_Bin,Compile.Ipf.Ifr_Bin,Compile.x64.Ifr_Bin]

#
# Add build dependency check
#
DEP_FILE    = $(DEST_DIR)\$(FILE)Vfr.dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

!IF EXIST($(DEST_DIR)\$(FILE).obj)
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(FILE)Vfr.dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(DEST_DIR)\$(FILE).obj
  $(MAKEDEPS) -f $(SOURCE_FILE_NAME) $(DEP_FLAGS)

HII_PACK_FILES  = $(HII_PACK_FILES) $(DEST_DIR)\$(FILE).hpk

#
# Add a dummy command for building the HII pack file. In reality, it's built 
# below, but the C_FLAGS macro reference the target as $@, so you can't specify
# the obj and hpk files as dual targets of the same command.
#
$(DEST_DIR)\$(FILE).hpk : $(DEST_DIR)\$(FILE).obj
  
$(DEST_DIR)\$(FILE).obj : $(SOURCE_FILE_NAME) $(INF_FILENAME) $(ALL_DEPS)
  $(VFRCOMPILE) $(VFRCOMPILE_FLAGS) $(INC) -ibin -od $(DEST_DIR)\$(SOURCE_RELATIVE_PATH) \
    -l $(VFR_FLAGS) $(SOURCE_FILE_NAME)
  $(CC) $(C_FLAGS) $(DEST_DIR)\$(FILE).c

#
# Add to the variable that contains the list of VFR binary files we're going
# to merge together at the end of the build. 
#
HII_IFR_PACK_FILES = $(HII_IFR_PACK_FILES) $(DEST_DIR)\$(FILE).hpk

[=============================================================================]
#
# Commands for building IFR as uncompressed binary into the FFS file. To 
# use it, set COMPILE_SELECT=.vfr=Ifr_Bin for the component in the DSC file.
#
[=============================================================================]
[Compile.Ebc.Ifr_Bin]

#
# Add build dependency check
#
DEP_FILE    = $(DEST_DIR)\$(FILE)Vfr.dep

!IF EXIST($(DEP_FILE))
!INCLUDE $(DEP_FILE)
!ENDIF

!IF EXIST($(DEST_DIR)\$(FILE).obj)
DEP_TARGETS = $(DEP_TARGETS) $(DEST_DIR)\$(FILE)Vfr.dep
!IF !EXIST($(DEP_FILE))
CREATEDEPS = YES
!ENDIF
!ENDIF

#
# Update dep file for next round incremental build
#
$(DEP_FILE) : $(DEST_DIR)\$(FILE).obj
  $(MAKEDEPS) -f $(SOURCE_FILE_NAME) $(DEP_FLAGS)

HII_PACK_FILES  = $(HII_PACK_FILES) $(DEST_DIR)\$(FILE).hpk

#
# Add a dummy command for building the HII pack file. In reality, it's built 
# below, but the C_FLAGS macro reference the target as $@, so you can't specify
# the obj and hpk files as dual targets of the same command.
#
$(DEST_DIR)\$(FILE).hpk : $(DEST_DIR)\$(FILE).obj
  
$(DEST_DIR)\$(FILE).obj : $(SOURCE_FILE_NAME) $(INF_FILENAME) $(ALL_DEPS)
  $(VFRCOMPILE) $(VFRCOMPILE_FLAGS) $(INC) -ibin -od $(DEST_DIR)\$(SOURCE_RELATIVE_PATH) \
    -l $(VFR_FLAGS) $(SOURCE_FILE_NAME)
  $(EBC_CC) $(EBC_C_FLAGS) $(DEST_DIR)\$(FILE).c

#
# Add to the variable that contains the list of VFR binary files we're going
# to merge together at the end of the build. 
#
HII_IFR_PACK_FILES = $(HII_IFR_PACK_FILES) $(DEST_DIR)\$(FILE).hpk

[=============================================================================]
[Compile.Ia32.Fv,Compile.Ipf.Fv,Compile.x64.Fv]
#
# Run GenSection on the firmware volume image.
#
$(DEST_DIR)\$(SOURCE_FV)Fv.sec : $(SOURCE_FILE_NAME) $(INF_FILENAME)
  $(GENSECTION) -I $(SOURCE_FILE_NAME) -O $(DEST_DIR)\$(SOURCE_FV)Fv.sec -S EFI_SECTION_FIRMWARE_VOLUME_IMAGE

[=============================================================================]
