## @file
# Windows makefile for Base Tools project build.
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

!IFNDEF BASE_TOOLS_PATH
!ERROR "BASE_TOOLS_PATH is not set! Please run toolsetup.bat first!"
!ENDIF

SUBDIRS = $(BASE_TOOLS_PATH)\Source\C $(BASE_TOOLS_PATH)\Source\Python

all: c

c :
  @$(PYTHON_HOME)\python.exe $(BASE_TOOLS_PATH)\Source\C\Makefiles\NmakeSubdirs.py  all $(BASE_TOOLS_PATH)\Source\C


subdirs: $(SUBDIRS)
  @$(PYTHON_HOME)\python.exe $(BASE_TOOLS_PATH)\Source\C\Makefiles\NmakeSubdirs.py  all $**

.PHONY: clean
clean:
  $(PYTHON_HOME)\python.exe $(BASE_TOOLS_PATH)\Source\C\Makefiles\NmakeSubdirs.py clean $(SUBDIRS)

.PHONY: cleanall
cleanall:
  $(PYTHON_HOME)\python.exe $(BASE_TOOLS_PATH)\Source\C\Makefiles\NmakeSubdirs.py  cleanall $(SUBDIRS)

