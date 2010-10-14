## @file
# Windows makefile for Base Tools project build.
#
# Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

all: subdirs

LANGUAGES = C Python

SOURCE_SUBDIRS := $(patsubst %,Source/%,$(sort $(LANGUAGES)))
SUBDIRS := $(SOURCE_SUBDIRS) Tests
CLEAN_SUBDIRS := $(patsubst %,%-clean,$(sort $(SUBDIRS)))

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: $(CLEAN_SUBDIRS)
$(CLEAN_SUBDIRS):
	-$(MAKE) -C $(@:-clean=) clean

clean:  $(CLEAN_SUBDIRS)

test:
	@$(MAKE) -C Tests

