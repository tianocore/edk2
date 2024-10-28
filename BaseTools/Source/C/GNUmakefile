## @file
#  GNU/Linux makefile for C tools build.
#
#  Copyright (c) 2007 - 2024, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
MAKEROOT:=$(shell echo %CD%)
SEP:=$(shell echo \)
else
MAKEROOT := .
SEP:=/
endif

ifndef PYTHON_COMMAND
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
#
# PYTHON_HOME is the old method of specifying a Python interperter on Windows.
# Check if an interperter can be found using PYTHON_HOME.
#
ifdef PYTHON_HOME
ifndef (,$(wildcard $(PYTHON_HOME)$(SEP)python.exe)) # Make sure the file exists
PYTHON_COMMAND := $(PYTHON_HOME)$(SEP)python.exe
endif
endif
ifndef PYTHON_COMMAND
#
# An interperter was not found using PYTHON_HOME. Next try using the
# Python Launcher for Windows to find an interperter.
#
CHECK_PY := $(shell where py.exe || echo NotFound)
ifeq ($(CHECK_PY),NotFound)
undefine CHECK_PY
$(error Unable to find a Python interperter, if one is installed, set the PYTHON_COMMAND environment variable!)
endif
PYTHON_COMMAND := $(shell py -3 -c "import sys; print(sys.executable)")
ifdef (,$(wildcard $(PYTHON_COMMAND))) # Make sure the file exists
$(error Unable to find a Python interperter, if one is installed, set the PYTHON_COMMAND environment variable!)
endif
endif
else # UNIX
PYTHON_COMMAND := $(shell /usr/bin/env python3 -c "import sys; print(sys.executable)")
ifdef (,$(wildcard $(PYTHON_COMMAND))) # Make sure the file exists
PYTHON_COMMAND := $(shell /usr/bin/env python -c "import sys; print(sys.executable)")
ifdef (,$(wildcard $(PYTHON_COMMAND))) # Make sure the file exists
undefine PYTHON_COMMAND
endif
endif
ifndef PYTHON_COMMAND
$(error Unable to find a Python interperter, if one is installed, set the PYTHON_COMMAND environment variable!)
endif
endif
export PYTHON_COMMAND
endif

ifndef HOST_ARCH
#
# If HOST_ARCH is not defined, then we use 'GetGnuHostArch.py' to attempt
# try to figure out the appropriate HOST_ARCH.
#
GET_GNU_HOST_ARCH_PY:=$(MAKEROOT)$(SEP)Makefiles$(SEP)GetGnuHostArch.py
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
HOST_ARCH:=$(shell if defined PYTHON_COMMAND $(PYTHON_COMMAND) $(GET_GNU_HOST_ARCH_PY))
ifeq ($(HOST_ARCH),)
HOST_ARCH:=$(shell if not defined PYTHON_COMMAND if defined PYTHON_HOME $(PYTHON_HOME)\python.exe $(GET_GNU_HOST_ARCH_PY))
endif
else
HOST_ARCH:=$(shell if command -v $(PYTHON_COMMAND) >/dev/null 1; then $(PYTHON_COMMAND) $(GET_GNU_HOST_ARCH_PY); else python $(GET_GNU_HOST_ARCH_PY); fi)
endif
ifeq ($(HOST_ARCH),)
$(info HOST_ARCH detection failed.)
undefine HOST_ARCH
endif
ifeq ($(HOST_ARCH),Unknown)
$(info HOST_ARCH detection failed.)
undefine HOST_ARCH
endif
endif
ifndef HOST_ARCH
$(error HOST_ARCH is not defined!)
endif

export HOST_ARCH

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
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
	$(shell if not exist "$(BIN_PATH)" mkdir "$(BIN_PATH)")
	$(shell if not exist "$(MAKEROOT)" mkdir "$(MAKEROOT)")
else
	-$(MKDIR) $(MAKEROOT)
endif

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
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
	$(shell del /F $(MAKEROOT)$(SEP)bin$(SEP)*)
	$(shell rmdir /S /Q $(MAKEROOT)$(SEP)libs $(MAKEROOT)$(SEP)bin)
else
	rm -f $(MAKEROOT)/bin/*
	-rmdir $(MAKEROOT)/libs $(MAKEROOT)/bin
endif

include Makefiles/footer.makefile
