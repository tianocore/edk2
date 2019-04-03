## @file
# GNU/Linux makefile for Base Tools project build.
#
# Copyright (c) 2009 - 2017, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
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

Tests: $(SOURCE_SUBDIRS)

.PHONY: $(CLEAN_SUBDIRS)
$(CLEAN_SUBDIRS):
	-$(MAKE) -C $(@:-clean=) clean

clean:  $(CLEAN_SUBDIRS)
