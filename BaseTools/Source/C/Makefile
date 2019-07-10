## @file
# Windows makefile for C tools build.
#
# Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
HOST_ARCH = IA32

!INCLUDE Makefiles\ms.common

LIBRARIES = Common
APPLICATIONS = \
  VfrCompile \
  BrotliCompress \
  EfiRom \
  GenCrc32 \
  GenFfs \
  GenFv \
  GenFw \
  GenSec \
  LzmaCompress \
  Split \
  TianoCompress \
  VolInfo \
  DevicePath

all: libs apps install

libs: $(LIBRARIES)
	@echo.
	@echo ######################
	@echo # Build libraries
	@echo ######################
	@if not exist $(LIB_PATH) mkdir $(LIB_PATH)
  @if defined PYTHON_COMMAND $(PYTHON_COMMAND) Makefiles\NmakeSubdirs.py all $**
  @if not defined PYTHON_COMMAND $(PYTHON_HOME)\python.exe Makefiles\NmakeSubdirs.py all $**

apps: $(APPLICATIONS)
	@echo.
	@echo ######################
	@echo # Build executables
	@echo ######################
	@if not exist $(BIN_PATH) mkdir $(BIN_PATH)
  @if defined PYTHON_COMMAND $(PYTHON_COMMAND) Makefiles\NmakeSubdirs.py all $**
  @if not defined PYTHON_COMMAND $(PYTHON_HOME)\python.exe Makefiles\NmakeSubdirs.py all $**

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
  @if defined PYTHON_COMMAND $(PYTHON_COMMAND) Makefiles\NmakeSubdirs.py clean $(LIBRARIES) $(APPLICATIONS)
  @if not defined PYTHON_COMMAND $(PYTHON_HOME)\python.exe Makefiles\NmakeSubdirs.py clean $(LIBRARIES) $(APPLICATIONS)

.PHONY: cleanall
cleanall:
  @if defined PYTHON_COMMAND $(PYTHON_COMMAND) Makefiles\NmakeSubdirs.py cleanall $(LIBRARIES) $(APPLICATIONS)
  @if not defined PYTHON_COMMAND $(PYTHON_HOME)\python.exe Makefiles\NmakeSubdirs.py cleanall $(LIBRARIES) $(APPLICATIONS)

!INCLUDE Makefiles\ms.rule

