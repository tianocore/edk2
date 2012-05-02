## @file
# Windows makefile for Python tools build.
#
# Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

!IFNDEF PYTHON_FREEZER_PATH
!ERROR PYTHON_FREEZER_PATH must be defined!
!ENDIF

FREEZE=$(PYTHON_FREEZER_PATH)\FreezePython.exe

MODULES=encodings.cp437,encodings.gbk,encodings.utf_16,encodings.utf_8,encodings.utf_16_le,encodings.latin_1,encodings.ascii

BIN_DIR=$(EDK_TOOLS_PATH)\Bin\Win32

APPLICATIONS=$(BIN_DIR)\build.exe $(BIN_DIR)\GenFds.exe $(BIN_DIR)\Trim.exe $(BIN_DIR)\TargetTool.exe $(BIN_DIR)\GenDepex.exe $(BIN_DIR)\GenPatchPcdTable.exe $(BIN_DIR)\PatchPcdValue.exe $(BIN_DIR)\BPDG.exe $(BIN_DIR)\UPT.exe

COMMON_PYTHON=$(BASE_TOOLS_PATH)\Source\Python\Common\BuildToolError.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\Database.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\DataType.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\DecClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\Dictionary.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\DscClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\EdkIIWorkspace.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\EdkIIWorkspaceBuild.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\EdkLogger.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\FdfClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\FdfParserLite.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\GlobalData.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\Identification.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\InfClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\MigrationUtilities.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\Misc.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\Parsing.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\String.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\TargetTxtClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\ToolDefClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\VpdInfoFile.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\__init__.py \
              $(BASE_TOOLS_PATH)\Source\Python\Common\BuildVersion.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\BuildClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\MetaDataTable.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\MetaFileParser.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\MetaFileTable.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\WorkspaceDatabase.py \
              $(BASE_TOOLS_PATH)\Source\Python\Workspace\__init__.py \
              $(BASE_TOOLS_PATH)\Source\Python\Autogen\AutoGen.py \
              $(BASE_TOOLS_PATH)\Source\Python\Autogen\BuildEngine.py \
              $(BASE_TOOLS_PATH)\Source\Python\Autogen\GenC.py \
              $(BASE_TOOLS_PATH)\Source\Python\Autogen\GenDepex.py \
              $(BASE_TOOLS_PATH)\Source\Python\Autogen\GenMake.py \
              $(BASE_TOOLS_PATH)\Source\Python\Autogen\StrGather.py \
              $(BASE_TOOLS_PATH)\Source\Python\Autogen\UniClassObject.py \
              $(BASE_TOOLS_PATH)\Source\Python\Autogen\__init__.py

UPT_BUILDVERSION_PYTHON=$(BASE_TOOLS_PATH)\Source\Python\UPT\BuildVersion.py
              
all: SetPythonPath $(APPLICATIONS)

SetPythonPath:
  set PYTHONPATH=$(BASE_TOOLS_PATH)\Source\Python

$(BIN_DIR)\build.exe: $(BASE_TOOLS_PATH)\Source\Python\build\build.py $(COMMON_PYTHON)
  @pushd . & @cd build & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) build.py & @popd

$(BIN_DIR)\GenFds.exe: $(BASE_TOOLS_PATH)\Source\Python\GenFds\GenFds.py $(COMMON_PYTHON)
  @pushd . & @cd GenFds & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) GenFds.py & @popd

$(BIN_DIR)\Trim.exe: $(BASE_TOOLS_PATH)\Source\Python\Trim\Trim.py $(COMMON_PYTHON)
  @pushd . & @cd Trim & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) Trim.py & @popd

$(BIN_DIR)\GenDepex.exe: $(BASE_TOOLS_PATH)\Source\Python\AutoGen\GenDepex.py $(COMMON_PYTHON)
  @pushd . & @cd AutoGen & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) GenDepex.py & @popd

$(BIN_DIR)\TargetTool.exe: $(BASE_TOOLS_PATH)\Source\Python\TargetTool\TargetTool.py $(COMMON_PYTHON)
  @pushd . & @cd TargetTool & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) TargetTool.py & @popd

$(BIN_DIR)\GenPatchPcdTable.exe: $(BASE_TOOLS_PATH)\Source\Python\GenPatchPcdTable\GenPatchPcdTable.py $(COMMON_PYTHON)
  @pushd . & @cd GenPatchPcdTable & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) GenPatchPcdTable.py & @popd

$(BIN_DIR)\PatchPcdValue.exe: $(BASE_TOOLS_PATH)\Source\Python\PatchPcdValue\PatchPcdValue.py $(COMMON_PYTHON)
  @pushd . & @cd PatchPcdValue & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) PatchPcdValue.py & @popd

$(BIN_DIR)\BPDG.exe: $(BASE_TOOLS_PATH)\Source\Python\BPDG\BPDG.py $(COMMON_PYTHON)
  @pushd . & @cd BPDG & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) BPDG.py & @popd

$(BIN_DIR)\UPT.exe: $(BASE_TOOLS_PATH)\Source\Python\UPT\UPT.py $(UPT_BUILDVERSION_PYTHON)
  @pushd . & @cd UPT & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) UPT.py & @popd

clean:
cleanall:  
  @del /f /q $(BIN_DIR)\*.pyd $(BIN_DIR)\*.dll
  @for %%i in ($(APPLICATIONS)) do @del /f /q %%i

