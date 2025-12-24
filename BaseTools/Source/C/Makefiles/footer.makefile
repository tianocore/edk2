## @file
# Makefile
#
# Copyright (c) 2007 - 2025, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

DEPFILES = $(OBJECTS:%.o=%.d)

$(BUILDROOT)/libs-$(HOST_ARCH):
	$(MD) $(BUILDROOT)/libs-$(HOST_ARCH)

.PHONY: install
install: $(BUILDROOT)/libs-$(HOST_ARCH) $(LIBRARY)
	$(CP) $(LIBRARY) $(BUILDROOT)/libs-$(HOST_ARCH)

$(LIBRARY): $(OBJECTS)
	$(AR) crs $@ $^

$(OBJDIR)/%.o : %.c
	@$(MD) $(@D)
	$(CC)  -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	@$(MD) $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJECTS): | $(OBJDIR)

$(OBJDIR)/%.d : %.c
	@set -e; rm -f $@; \
		$(MD) $(@D); \
		$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

.PHONY: clean
clean:
	$(RM) $(OBJECTS) $(LIBRARY) $(DEPFILES)

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPFILES)
endif
