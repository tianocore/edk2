## @file
# Makefiles
#
# Copyright (c) 2007 - 2024, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

MAKEROOT ?= ..$(SEP)..

include $(MAKEROOT)/Makefiles/header.makefile

APPLICATION = $(MAKEROOT)$(SEP)bin$(SEP)$(APPNAME)

.PHONY:all
all: $(MAKEROOT)$(SEP)bin $(APPLICATION)

$(APPLICATION): $(OBJECTS)
	$(LINKER) -o $(APPLICATION) $(LDFLAGS) $(OBJECTS) -L$(MAKEROOT)/libs $(LIBS)
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
	copy /Y $(APPLICATION).exe $(BIN_PATH)
endif

$(OBJECTS): $(MAKEROOT)/Include/Common/BuildVersion.h

include $(MAKEROOT)/Makefiles/footer.makefile
