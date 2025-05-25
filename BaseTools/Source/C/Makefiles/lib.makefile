## @file
# Makefiles
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

include $(MAKEROOT)/Makefiles/header.makefile

LIBRARY = $(MAKEROOT)/libs/lib$(LIBNAME).a

all: $(MAKEROOT)/libs $(LIBRARY)

ninja_lib: ninja_build
	@echo "" >> build.ninja
	@echo "rule link_$(NINJA_ID)" >> build.ninja
	@echo "  pool = link_pool" >> build.ninja
	@echo "  command = $(AR) crs \044out \044in" >> build.ninja
	@echo "" >> build.ninja
	@echo "build $(LIBRARY): link_$(NINJA_ID) $(NINJA_OBJECTS)" >> build.ninja

include $(MAKEROOT)/Makefiles/footer.makefile
