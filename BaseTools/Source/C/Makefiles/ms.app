## @file
# Makefile
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.    The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

!INCLUDE ..\Makefiles\ms.common

APPLICATION = $(BIN_PATH)\$(APPNAME).exe

all: $(APPLICATION) 

$(APPLICATION) : $(OBJECTS) 
	-@if not exist $(BIN_PATH) mkdir $(BIN_PATH)
	$(LD) /nologo /debug /incremental:no /nodefaultlib:libc.lib /out:$@ $(LIBS) $**

$(OBJECTS) : ..\Include\Common\BuildVersion.h

.PHONY:clean
.PHONY:cleanall

clean:
	del /f /q $(OBJECTS) *.pdb > nul

cleanall:
	del /f /q $(OBJECTS) $(APPLICATION) *.pdb $(BIN_PATH)\$(APPNAME).pdb > nul

!INCLUDE ..\Makefiles\ms.rule

