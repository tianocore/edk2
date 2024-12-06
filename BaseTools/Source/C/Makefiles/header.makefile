## @file
#
# The makefile can be invoked with
# HOST_ARCH = x86_64 or x64 for EM64T build
# HOST_ARCH = ia32 or IA32 for IA32 build
# HOST_ARCH = Arm or ARM for ARM build
#
# Copyright (c) 2007 - 2024, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

# Set SEP to the platform specific path seperator
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
SEP:=$(shell echo \)
else
SEP:=/
endif

EDK2_PATH ?= $(MAKEROOT)$(SEP)..$(SEP)..$(SEP)..

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

#Set up BaseTool binary path for Windows builds
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
  ifndef BIN_PATH
    BIN_PATH_BASE=$(MAKEROOT)$(SEP)..$(SEP)..$(SEP)Bin
    ifeq ($(HOST_ARCH),X64)
      BIN_PATH=$(BIN_PATH_BASE)$(SEP)Win64
    else
      BIN_PATH=$(BIN_PATH_BASE)$(SEP)Win32
    endif
  endif
endif

ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
  CYGWIN:=$(findstring CYGWIN, $(shell uname -s))
  LINUX:=$(findstring Linux, $(shell uname -s))
  DARWIN:=$(findstring Darwin, $(shell uname -s))
else
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
CC = gcc
CXX = g++
AS = gcc
AR = ar
LD = ld
endif
LINKER ?= $(CC)
ifeq ($(HOST_ARCH), IA32)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/Ia32/

else ifeq ($(HOST_ARCH), X64)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/X64/

else ifeq ($(HOST_ARCH), ARM)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/Arm/

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
-Wno-unused-result -nostdlib -Wno-unused-function -g
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
	mkdir $(MAKEROOT)$(SEP)libs

$(MAKEROOT)/bin:
	mkdir $(MAKEROOT)$(SEP)bin
