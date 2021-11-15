## @file
#  GNU/Linux makefile for C tools build.
#
#  Copyright (c) 2007 - 2017, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

ifndef HOST_ARCH
  #
  # If HOST_ARCH is not defined, then we use 'uname -m' to attempt
  # try to figure out the appropriate HOST_ARCH.
  #
  uname_m = $(shell uname -m)
  $(info Attempting to detect HOST_ARCH from 'uname -m': $(uname_m))
  ifneq (,$(strip $(filter $(uname_m), x86_64 amd64)))
    HOST_ARCH=X64
  endif
  ifeq ($(patsubst i%86,IA32,$(uname_m)),IA32)
    HOST_ARCH=IA32
  endif
  ifneq (,$(findstring aarch64,$(uname_m)))
    HOST_ARCH=AARCH64
  else ifneq (,$(findstring arm64,$(uname_m)))
    HOST_ARCH=AARCH64
  else ifneq (,$(findstring arm,$(uname_m)))
    HOST_ARCH=ARM
  endif
  ifneq (,$(findstring riscv64,$(uname_m)))
    HOST_ARCH=RISCV64
  endif
  ifneq (,$(findstring loongarch64,$(uname_m)))
    HOST_ARCH=LOONGARCH64
  endif
  ifndef HOST_ARCH
    $(info Could not detected HOST_ARCH from uname results)
    $(error HOST_ARCH is not defined!)
  endif
  $(info Detected HOST_ARCH of $(HOST_ARCH) using uname.)
endif

export HOST_ARCH

MAKEROOT = .

include Makefiles/header.makefile

all: makerootdir subdirs
	@echo Finished building BaseTools C Tools with HOST_ARCH=$(HOST_ARCH)

LIBRARIES = Common
VFRAUTOGEN = VfrCompile/VfrLexer.h
APPLICATIONS = \
  BrotliCompress \
  VfrCompile \
  EfiRom \
  GenFfs \
  GenFv \
  GenFw \
  GenSec \
  GenCrc32 \
  LzmaCompress \
  TianoCompress \
  VolInfo \
  DevicePath

SUBDIRS := $(LIBRARIES) $(APPLICATIONS)

$(LIBRARIES): $(MAKEROOT)/libs
$(APPLICATIONS): $(LIBRARIES) $(MAKEROOT)/bin $(VFRAUTOGEN)

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

$(VFRAUTOGEN): VfrCompile/VfrSyntax.g
	$(MAKE) -C VfrCompile VfrLexer.h

clean:  $(patsubst %,%-clean,$(sort $(SUBDIRS)))

clean: localClean

localClean:
	rm -f $(MAKEROOT)/bin/*
	-rmdir $(MAKEROOT)/libs $(MAKEROOT)/bin

include Makefiles/footer.makefile
