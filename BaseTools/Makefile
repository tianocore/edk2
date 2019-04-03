## @file
# Windows makefile for Base Tools project build.
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

!IFNDEF BASE_TOOLS_PATH
!ERROR "BASE_TOOLS_PATH is not set! Please run toolsetup.bat first!"
!ENDIF

SUBDIRS = $(BASE_TOOLS_PATH)\Source\C $(BASE_TOOLS_PATH)\Source\Python

all: c

c :
  @if defined PYTHON_COMMAND $(PYTHON_COMMAND) $(BASE_TOOLS_PATH)\Source\C\Makefiles\NmakeSubdirs.py  all $(BASE_TOOLS_PATH)\Source\C
  @if not defined PYTHON_COMMAND $(PYTHON_HOME)\python.exe $(BASE_TOOLS_PATH)\Source\C\Makefiles\NmakeSubdirs.py  all $(BASE_TOOLS_PATH)\Source\C


subdirs: $(SUBDIRS)
  @if defined PYTHON_COMMAND $(PYTHON_COMMAND) $(BASE_TOOLS_PATH)\Source\C\Makefiles\NmakeSubdirs.py  all $**
  @if not defined PYTHON_COMMAND $(PYTHON_HOME)\python.exe $(BASE_TOOLS_PATH)\Source\C\Makefiles\NmakeSubdirs.py  all $**

.PHONY: clean
clean:
  @if defined PYTHON_COMMAND $(PYTHON_COMMAND) $(BASE_TOOLS_PATH)\Source\C\Makefiles\NmakeSubdirs.py clean $(SUBDIRS)
  @if not defined PYTHON_COMMAND $(PYTHON_HOME)\python.exe $(BASE_TOOLS_PATH)\Source\C\Makefiles\NmakeSubdirs.py clean $(SUBDIRS)

.PHONY: cleanall
cleanall:
  @if defined PYTHON_COMMAND $(PYTHON_COMMAND) $(BASE_TOOLS_PATH)\Source\C\Makefiles\NmakeSubdirs.py  cleanall $(SUBDIRS)
  @if not defined PYTHON_COMMAND $(PYTHON_HOME)\python.exe $(BASE_TOOLS_PATH)\Source\C\Makefiles\NmakeSubdirs.py  cleanall $(SUBDIRS)

