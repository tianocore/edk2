## @file
#  GNU Make makefile for C tools build.
#
#  Copyright (c) 2007 - 2012, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

ifndef ARCH
  #
  # If ARCH is not defined, then we use 'uname -m' to attempt
  # try to figure out the appropriate ARCH.
  #
  uname_m = $(shell uname -m)
  $(info Attempting to detect ARCH from 'uname -m': $(uname_m))
  ifeq ($(uname_m),x86_64)
    ARCH=X64
  endif
  ifeq ($(patsubst i%86,IA32,$(uname_m)),IA32)
    ARCH=IA32
  endif
  ifndef ARCH
    $(info Could not detected ARCH from uname results)
    $(error ARCH is not defined!)
  endif
  $(info Detected ARCH of $(ARCH) using uname.)
endif

export ARCH

MAKEROOT = .

include Makefiles/header.makefile

all: makerootdir subdirs $(MAKEROOT)/libs
	@echo Finished building BaseTools C Tools with ARCH=$(ARCH)

LIBRARIES = Common
# NON_BUILDABLE_APPLICATIONS = GenBootSector BootSectImage
APPLICATIONS = \
  GnuGenBootSector \
  BootSectImage \
  EfiLdrImage \
  EfiRom \
  GenFfs \
  GenFv \
  GenFw \
  GenPage \
  GenSec \
  GenCrc32 \
  GenVtf \
  LzmaCompress \
  Split \
  TianoCompress \
  VolInfo \
  VfrCompile

SUBDIRS := $(LIBRARIES) $(APPLICATIONS)

.PHONY: outputdirs
makerootdir:
	-mkdir -p $(MAKEROOT)

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: $(patsubst %,%-clean,$(sort $(SUBDIRS)))
$(patsubst %,%-clean,$(sort $(SUBDIRS))):
	-$(MAKE) -C $(@:-clean=) clean

clean:  $(patsubst %,%-clean,$(sort $(SUBDIRS)))

clean: localClean

localClean:
	rm -f $(MAKEROOT)/bin/*
	-rmdir $(MAKEROOT)/libs $(MAKEROOT)/bin

include Makefiles/footer.makefile
