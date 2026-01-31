## @file
# Makefiles
#
# Copyright (c) 2007 - 2025, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

MAKEROOT ?= ../..
BUILDROOT ?= $(MAKEROOT)
OBJDIR ?= .
OBJECTS := $(addprefix $(OBJDIR)/,$(OBJECTS))

include $(MAKEROOT)/Makefiles/header.makefile

APPLICATION = $(BUILDROOT)/bin/$(APPNAME)

.PHONY:all
all: $(BUILDROOT)/bin $(APPLICATION)

$(APPLICATION): $(OBJECTS)
	$(LINKER) -o $(APPLICATION) $(LDFLAGS) $(OBJECTS) -L$(BUILDROOT)/libs $(LIBS)
ifeq (Windows, $(findstring Windows,$(OS)))
	$(CP) $(APPLICATION).exe $(BIN_PATH)
endif

$(OBJECTS): $(MAKEROOT)/Include/Common/BuildVersion.h

clean: appClean

appClean:
ifeq (Windows, $(findstring Windows,$(OS)))
	$(RM) $(BIN_PATH)/$(APPNAME).exe
endif

include $(MAKEROOT)/Makefiles/footer.makefile
