## @file
# Windows makefile for Base Tools project build.
#
# Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

SUBDIRS = Source\C Source\Python

all: c python

c :
	@Source\C\Makefiles\NmakeSubdirs.bat all Source\C

python:
	@Source\C\Makefiles\NmakeSubdirs.bat all Source\Python

subdirs: $(SUBDIRS)
	@Source\C\Makefiles\NmakeSubdirs.bat all $**

.PHONY: clean
clean:
	@Source\C\Makefiles\NmakeSubdirs.bat clean $(SUBDIRS)

.PHONY: cleanall
cleanall:
	@Source\C\Makefiles\NmakeSubdirs.bat cleanall $(SUBDIRS)

