## @file
# Makefiles
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

MAKEROOT ?= ../..

include $(MAKEROOT)/Makefiles/header.makefile

APPLICATION = $(MAKEROOT)/bin/$(APPNAME)

.PHONY:all
all: $(MAKEROOT)/bin $(APPLICATION) 

$(APPLICATION): $(OBJECTS) 
	$(LINKER) -o $(APPLICATION) $(BUILD_LFLAGS) $(OBJECTS) -L$(MAKEROOT)/libs $(LIBS)

$(OBJECTS): $(MAKEROOT)/Include/Common/BuildVersion.h

include $(MAKEROOT)/Makefiles/footer.makefile
