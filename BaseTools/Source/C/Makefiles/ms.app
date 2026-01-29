## @file
# Makefile
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

APPLICATION = $(BIN_PATH)\$(APPNAME).exe

all: $(APPLICATION) 

$(APPLICATION) : $(OBJECTS) 
	-@if not exist $(BIN_PATH) mkdir $(BIN_PATH)
	$(LD) /nologo /debug /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib /out:$@ $(LIBS) $**

$(OBJECTS) : $(SOURCE_PATH)\Include\Common\BuildVersion.h

.PHONY:clean
.PHONY:cleanall

clean:
	del /f /q $(OBJECTS) *.pdb > nul

cleanall:
	del /f /q $(OBJECTS) $(APPLICATION) *.pdb $(BIN_PATH)\$(APPNAME).pdb > nul

!INCLUDE $(SOURCE_PATH)\Makefiles\ms.rule

