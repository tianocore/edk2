## @file
# Makefile
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.    The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

DEPFILES = $(OBJECTS:%.o=%.d)

$(MAKEROOT)/libs-$(ARCH):
	mkdir -p $(MAKEROOT)/libs-$(ARCH)

.PHONY: install
install: $(MAKEROOT)/libs-$(ARCH) $(LIBRARY)
	cp $(LIBRARY) $(MAKEROOT)/libs-$(ARCH)

$(LIBRARY): $(OBJECTS) 
	$(AR) crs $@ $^

%.o : %.c 
	$(CC)  -c $(CFLAGS) $(CPPFLAGS) $< -o $@

%.o : %.S
	$(AS) -c $(ASFLAGS) $< -o $@

.PHONY: clean
clean:
	@rm -f $(OBJECTS) $(LIBRARY) $(DEPFILES)

-include $(DEPFILES)
