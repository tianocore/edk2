## @file
# Windows makefile for C tools build.
#
# Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
HOST_ARCH = IA32

!INCLUDE Makefiles\ms.common

LIBRARIES = Common
APPLICATIONS = \
  VfrCompile \
  BrotliCompress \
  EfiLdrImage \
  EfiRom \
  GenBootSector \
  GenCrc32 \
  GenFfs \
  GenFv \
  GenFw \
  GenPage \
  GenSec \
  GenVtf \
  LzmaCompress \
  Split \
  TianoCompress \
  VolInfo \
  BootSectImage \
  DevicePath

all: libs apps install

libs: $(LIBRARIES)
	@echo.
	@echo ######################
	@echo # Build libraries
	@echo ######################
	@if not exist $(LIB_PATH) mkdir $(LIB_PATH)
	@$(PYTHON_HOME)\python.exe Makefiles\NmakeSubdirs.py all $**

apps: $(APPLICATIONS)
	@echo.
	@echo ######################
	@echo # Build executables
	@echo ######################
	@if not exist $(BIN_PATH) mkdir $(BIN_PATH)
	@$(PYTHON_HOME)\python.exe Makefiles\NmakeSubdirs.py all $**

install: $(LIB_PATH) $(BIN_PATH)
	@echo.
	@echo ######################
	@echo # Install to $(SYS_LIB_PATH)
	@echo # Install to $(SYS_BIN_PATH)
	@echo ######################
	@-xcopy $(LIB_PATH)\*.lib $(SYS_LIB_PATH) /I /D /E /F /Y > NUL 2>&1
	@-xcopy $(BIN_PATH)\*.exe $(SYS_BIN_PATH) /I /D /E /F /Y > NUL 2>&1
  @-xcopy $(BIN_PATH)\*.bat $(SYS_BIN_PATH) /I /D /E /F /Y > NUL 2>&1

.PHONY: clean
clean:
  @$(PYTHON_HOME)\python.exe Makefiles\NmakeSubdirs.py clean $(LIBRARIES) $(APPLICATIONS)

.PHONY: cleanall
cleanall:
  @$(PYTHON_HOME)\python.exe Makefiles\NmakeSubdirs.py cleanall $(LIBRARIES) $(APPLICATIONS)

!INCLUDE Makefiles\ms.rule

