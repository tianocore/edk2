## @file
# Makefiles
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

MAKEROOT ?= ../..

include $(MAKEROOT)/Makefiles/header.makefile

APPLICATION = $(MAKEROOT)/bin/$(APPNAME)

NINJA_BUILD_DEPS =
ifneq ($(NINJA_LIBS_DEPS),)
  NINJA_BUILD_DEPS +=  || $(foreach lib,$(NINJA_LIBS_DEPS),"$(MAKEROOT)/libs/lib$(subst -l,,$(lib)).a")
endif

.PHONY:all ninja
all: $(MAKEROOT)/bin $(APPLICATION)

$(APPLICATION): $(OBJECTS)
	$(LINKER) -o $(APPLICATION) $(LDFLAGS) $(OBJECTS) -L$(MAKEROOT)/libs $(LIBS)

$(OBJECTS): $(MAKEROOT)/Include/Common/BuildVersion.h

ninja_app: ninja_build
	@echo "" >> build.ninja
	@echo "rule link_$(NINJA_ID)" >> build.ninja
	@echo "  pool = link_pool" >> build.ninja
	@echo "  command = $(LINKER) -o \044out $(LDFLAGS) \044in -L$(MAKEROOT)/libs $(LIBS)" >> build.ninja
	@echo "" >> build.ninja
	@echo "build $(APPLICATION): link_$(NINJA_ID) $(NINJA_OBJECTS)$(NINJA_BUILD_DEPS)" >> build.ninja

include $(MAKEROOT)/Makefiles/footer.makefile
