## @file
# Makefile
#
# Copyright (c) 2007 - 2016, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

DEPFILES = $(OBJECTS:%.o=%.d)

$(MAKEROOT)/libs-$(HOST_ARCH):
	mkdir -p $(MAKEROOT)/libs-$(HOST_ARCH)

NINJA_CSRCS = $(wildcard *.c)
NINJA_CXXSRCS = $(wildcard *.cpp)
NINJA_COBJECTS := $(foreach file,$(NINJA_CSRCS),"build $(NINJA_DIR)/$(subst .c,,$(file)).o: cc_$(NINJA_ID) $(NINJA_DIR)/$(file)")
NINJA_CXXOBJECTS := $(foreach file,$(NINJA_CXXSRCS),"build $(NINJA_DIR)/$(subst .c,,$(file)).o: cxx_$(NINJA_ID) $(NINJA_DIR)/$(file)")

.PHONY: install ninja
install: $(MAKEROOT)/libs-$(HOST_ARCH) $(LIBRARY)
	cp $(LIBRARY) $(MAKEROOT)/libs-$(HOST_ARCH)

$(LIBRARY): $(OBJECTS)
	$(AR) crs $@ $^

%.o : %.c
	$(CC)  -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

ninja_build:
	@echo "" > build.ninja
	@echo "rule cc_$(NINJA_ID)" >> build.ninja
	@echo "  deps = gcc" >> build.ninja
	@echo "  depfile = \044out.d" >> build.ninja
	@echo "  command = $(CC) -c $(CPPFLAGS) $(CFLAGS) \044in -o \044out" >> build.ninja
	@echo "" >> build.ninja
	@echo "rule cxx_$(NINJA_ID)" >> build.ninja
	@echo "  deps = gcc" >> build.ninja
	@echo "  depfile = \044out.d" >> build.ninja
	@echo "  command = $(CC) -c $(CPPFLAGS) $(CXXFLAGS) \044in -o \044out" >> build.ninja
	@echo "" >> build.ninja
	@printf "%s\n" $(NINJA_COBJECTS) >> build.ninja
	@printf "%s\n" $(NINJA_CXXOBJECTS) >> build.ninja
	@echo "" >> build.ninja

.PHONY: clean
clean:
	@rm -f $(OBJECTS) $(LIBRARY) $(DEPFILES)
	@rm -f build.ninja

-include $(DEPFILES)
