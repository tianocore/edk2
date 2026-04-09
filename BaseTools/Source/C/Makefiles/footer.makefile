## @file
# Makefile
#
# Copyright (c) 2007 - 2025, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

DEPFILES = $(OBJECTS:%.o=%.d)

$(MAKEROOT)/libs-$(HOST_ARCH):
	$(MD) $(MAKEROOT)/libs-$(HOST_ARCH)

.PHONY: install
install: $(MAKEROOT)/libs-$(HOST_ARCH) $(LIBRARY)
	$(CP) $(LIBRARY) $(MAKEROOT)/libs-$(HOST_ARCH)

$(LIBRARY): $(OBJECTS)
	$(AR) crs $@ $^

%.o : %.c
	$(CC)  -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	$(RM) $(OBJECTS) $(LIBRARY) $(DEPFILES)

-include $(DEPFILES)
