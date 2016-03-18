## @file
# Windows makefile for C tools build.
#
# Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
ARCH = IA32

!INCLUDE Makefiles\ms.common

LIBRARIES = Common
APPLICATIONS = \
  BootSectImage \
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
  VfrCompile

all: libs apps install

libs: $(LIBRARIES)
	@echo.
	@echo ######################
	@echo # Build libraries
	@echo ######################
	@if not exist $(LIB_PATH) mkdir $(LIB_PATH)
	@Makefiles\NmakeSubdirs.bat all $**

apps: $(APPLICATIONS)
	@echo.
	@echo ######################
	@echo # Build executables
	@echo ######################
	@if not exist $(BIN_PATH) mkdir $(BIN_PATH)
	@Makefiles\NmakeSubdirs.bat all $**

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
  @Makefiles\NmakeSubdirs.bat clean $(LIBRARIES) $(APPLICATIONS)

.PHONY: cleanall
cleanall:
  @Makefiles\NmakeSubdirs.bat cleanall $(LIBRARIES) $(APPLICATIONS)

!INCLUDE Makefiles\ms.rule

