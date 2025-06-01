## @file
# Makefiles
#
# Copyright (c) 2007 - 2025, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

MAKEROOT ?= ../..

include $(MAKEROOT)/Makefiles/header.makefile

APPLICATION = $(MAKEROOT)/bin/$(APPNAME)

.PHONY:all
all: $(MAKEROOT)/bin $(APPLICATION)

$(APPLICATION): $(OBJECTS)
	$(LINKER) -o $(APPLICATION) $(LDFLAGS) $(OBJECTS) -L$(MAKEROOT)/libs $(LIBS)
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
	$(CP) $(APPLICATION).exe $(BIN_PATH)
endif

$(OBJECTS): $(MAKEROOT)/Include/Common/BuildVersion.h

clean: appClean

appClean:
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
	$(RM) $(BIN_PATH)/$(APPNAME).exe
endif

include $(MAKEROOT)/Makefiles/footer.makefile
