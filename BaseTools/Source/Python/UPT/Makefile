## @file
# Windows makefile for UPT tools build.
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
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

BIN_DIR = .\Bin
SOURCES_PATH = .

APPLICATIONS=$(BIN_DIR)\UPT.exe

UPT_BUILDVERSION_PYTHON=$(SOURCES_PATH)\BuildVersion.py

all: SetPythonPath $(APPLICATIONS)

SetPythonPath:
  set PYTHONPATH= $(SOURCES_PATH)

$(BIN_DIR)\UPT.exe: $(SOURCES_PATH)\UPT.py $(UPT_BUILDVERSION_PYTHON)
  @pushd . & @cd build & @$(FREEZE) --include-modules=$(MODULES) --install-dir=$(BIN_DIR) UPT.py & @popd
  @pushd . & @copy .\Dll\sqlite3.dll .\Bin\Sqlite3.dll & @popd 
clean:
cleanall:  
  @del /f /q $(BIN_DIR)\*.pyd $(BIN_DIR)\*.dll
  @for %%i in ($(APPLICATIONS)) do @del /f /q %%i

