## @file
#
# The makefile can be invoked with
# HOST_ARCH = x86_64 or x64 for EM64T build
# HOST_ARCH = ia32 or IA32 for IA32 build
#
# Copyright (c) 2007 - 2025, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

# Set SEP to the platform specific path separator
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
  SHELL := cmd.exe
  SEP:=$(shell echo \)
else
  SEP:=/
endif

EDK2_PATH ?= $(MAKEROOT)/../../..
ifndef PYTHON_COMMAND
  ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
    #
    # Try using the Python Launcher for Windows to find an interpreter.
    #
    CHECK_PY := $(shell where py.exe || echo NotFound)
    ifeq ($(CHECK_PY),NotFound)
      #
      # PYTHON_HOME is the old method of specifying a Python interpreter on Windows.
      # Check if an interpreter can be found using PYTHON_HOME.
      #
      ifdef PYTHON_HOME
        ifndef (,$(wildcard $(PYTHON_HOME)$(SEP)python.exe)) # Make sure the file exists
          PYTHON_COMMAND := $(PYTHON_HOME)$(SEP)python.exe
        else
          $(error Unable to find a Python interpreter, if one is installed, set the PYTHON_COMMAND environment variable!)
        endif
      endif
    else
      PYTHON_COMMAND := $(shell py -3 -c "import sys; print(sys.executable)")
      ifdef (,$(wildcard $(PYTHON_COMMAND))) # Make sure the file exists
        $(error Unable to find a Python interpreter, if one is installed, set the PYTHON_COMMAND environment variable!)
      endif
    endif
    undefine CHECK_PY
    PYTHON_COMMAND := "$(PYTHON_COMMAND)"
  else # UNIX
    PYTHON_COMMAND := $(shell /usr/bin/env python3 -c "import sys; print(sys.executable)")
    ifdef (,$(wildcard $(PYTHON_COMMAND))) # Make sure the file exists
      PYTHON_COMMAND := $(shell /usr/bin/env python -c "import sys; print(sys.executable)")
      ifdef (,$(wildcard $(PYTHON_COMMAND))) # Make sure the file exists
        undefine PYTHON_COMMAND
      endif
    endif
    ifndef PYTHON_COMMAND
      $(error Unable to find a Python interpreter, if one is installed, set the PYTHON_COMMAND environment variable!)
    endif
  endif
  export PYTHON_COMMAND
endif

# GnuMakeUtils.py is able to handle forward slashes in file paths on Windows systems
GNU_MAKE_UTILS_PY := $(PYTHON_COMMAND) $(MAKEROOT)$(SEP)Makefiles$(SEP)GnuMakeUtils.py
CP := $(GNU_MAKE_UTILS_PY) cp
MV := $(GNU_MAKE_UTILS_PY) mv
RM := $(GNU_MAKE_UTILS_PY) rm
MD := $(GNU_MAKE_UTILS_PY) md
RD := $(GNU_MAKE_UTILS_PY) rd

ifndef HOST_ARCH
  #
  # If HOST_ARCH is not defined, then we use 'GnuMakeUtils.py' to attempt
  # try to figure out the appropriate HOST_ARCH.
  #
  GET_GNU_HOST_ARCH_PY:=$(MAKEROOT)$(SEP)Makefiles$(SEP)GnuMakeUtils.py get_host_arch
  ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
    HOST_ARCH:=$(shell if defined PYTHON_COMMAND $(PYTHON_COMMAND) $(GET_GNU_HOST_ARCH_PY))
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

#Set up BaseTools binary path for Windows builds
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
  ifndef BIN_PATH
    BIN_PATH_BASE=$(MAKEROOT)/../../Bin
    ifeq ($(HOST_ARCH),X64)
      BIN_PATH=$(BIN_PATH_BASE)/Win64
    else
      ifeq ($(HOST_ARCH),AARCH64)
        BIN_PATH=$(BIN_PATH_BASE)/Win64
      else
        BIN_PATH=$(BIN_PATH_BASE)/Win32
      endif
    endif
  endif
endif

ifneq ($(findstring cmd,$(SHELL)),cmd)
  CYGWIN:=$(findstring CYGWIN, $(shell uname -s))
  LINUX:=$(findstring Linux, $(shell uname -s))
  DARWIN:=$(findstring Darwin, $(shell uname -s))
else
  #Don't use uname on Windows
  CYGWIN:=
  LINUX:=
  DARWIN:=
endif
CLANG := $(findstring clang,$(shell $(CC) --version))
ifneq ($(CLANG),)
CC ?= $(CLANG_BIN)clang
CXX ?= $(CLANG_BIN)clang++
AS ?= $(CLANG_BIN)clang
AR ?= $(CLANG_BIN)llvm-ar
LD ?= $(CLANG_BIN)llvm-ld
else ifeq ($(origin CC),default)
CC = $(GCC_PREFIX)gcc
CXX = $(GCC_PREFIX)g++
AS = $(GCC_PREFIX)gcc
AR = $(GCC_PREFIX)ar
LD = $(GCC_PREFIX)ld
endif
LINKER ?= $(CC)
ifeq ($(HOST_ARCH), IA32)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/Ia32/

else ifeq ($(HOST_ARCH), X64)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/X64/

else ifeq ($(HOST_ARCH), AARCH64)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/AArch64/

else ifeq ($(HOST_ARCH), RISCV64)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/RiscV64/

else ifeq ($(HOST_ARCH), LOONGARCH64)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/LoongArch64/

else
$(error Bad HOST_ARCH)
endif

INCLUDE = $(TOOL_INCLUDE) -I $(MAKEROOT) -I $(MAKEROOT)/Include/Common -I $(MAKEROOT)/Include/ -I $(MAKEROOT)/Include/IndustryStandard -I $(MAKEROOT)/Common/ -I .. -I . $(ARCH_INCLUDE)
INCLUDE += -I $(EDK2_PATH)/MdePkg/Include
CPPFLAGS = $(INCLUDE)

# keep EXTRA_OPTFLAGS last
BUILD_OPTFLAGS = -O2 $(EXTRA_OPTFLAGS)

ifeq ($(DARWIN),Darwin)
# assume clang or clang compatible flags on OS X
CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -Wall -Werror \
-Wno-deprecated-declarations -Wno-self-assign -Wno-unused-result -nostdlib -g
else
ifneq ($(CLANG),)
CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -fwrapv \
-fno-delete-null-pointer-checks -Wall -Werror \
-Wno-deprecated-declarations -Wno-self-assign \
-Wno-unused-result -nostdlib -g
else
CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -fwrapv \
-fno-delete-null-pointer-checks -Wall -Werror \
-Wno-deprecated-declarations -Wno-stringop-truncation -Wno-restrict \
-Wno-unused-result -nostdlib -g
endif
endif
ifneq ($(CLANG),)
LDFLAGS =
CXXFLAGS = -Wno-deprecated-register -Wno-unused-result -std=c++14
else
LDFLAGS =
CXXFLAGS = -Wno-unused-result
endif
ifeq ($(HOST_ARCH), IA32)
#
# Snow Leopard  is a 32-bit and 64-bit environment. uname -m returns i386, but gcc defaults
#  to x86_64. So make sure tools match uname -m. You can manual have a 64-bit kernal on Snow Leopard
#  so only do this is uname -m returns i386.
#
ifeq ($(DARWIN),Darwin)
  CFLAGS   += -arch i386
  CPPFLAGS += -arch i386
  LDFLAGS  += -arch i386
endif
endif

# keep BUILD_OPTFLAGS last
CFLAGS   += $(BUILD_OPTFLAGS)
CXXFLAGS += $(BUILD_OPTFLAGS)

# keep EXTRA_LDFLAGS last
LDFLAGS += $(EXTRA_LDFLAGS)

.PHONY: all
.PHONY: install
.PHONY: clean

all:

$(MAKEROOT)/libs:
	$(MD) $(MAKEROOT)/libs

$(MAKEROOT)/bin:
	$(MD) $(MAKEROOT)/bin
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
	$(MD) $(BIN_PATH)
endif
