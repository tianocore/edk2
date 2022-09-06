## @file
#
# The makefile can be invoked with
# HOST_ARCH = x86_64 or x64 for EM64T build
# HOST_ARCH = ia32 or IA32 for IA32 build
# HOST_ARCH = Arm or ARM for ARM build
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

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

CYGWIN:=$(findstring CYGWIN, $(shell uname -s))
LINUX:=$(findstring Linux, $(shell uname -s))
DARWIN:=$(findstring Darwin, $(shell uname -s))
ifeq ($(CXX), llvm)
BUILD_CC ?= $(CLANG_BIN)clang
BUILD_CXX ?= $(CLANG_BIN)clang++
BUILD_AS ?= $(CLANG_BIN)clang
BUILD_AR ?= $(CLANG_BIN)llvm-ar
BUILD_LD ?= $(CLANG_BIN)llvm-ld
else
BUILD_CC ?= gcc
BUILD_CXX ?= g++
BUILD_AS ?= gcc
BUILD_AR ?= ar
BUILD_LD ?= ld
endif
LINKER ?= $(BUILD_CC)
ifeq ($(HOST_ARCH), IA32)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/Ia32/

else ifeq ($(HOST_ARCH), X64)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/X64/

else ifeq ($(HOST_ARCH), ARM)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/Arm/

else ifeq ($(HOST_ARCH), AARCH64)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/AArch64/

else ifeq ($(HOST_ARCH), RISCV64)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/RiscV64/

else ifeq ($(HOST_ARCH), LOONGARCH64)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/LoongArch64/

else
$(error Bad HOST_ARCH)
endif

INCLUDE = $(TOOL_INCLUDE) -I $(MAKEROOT) -I $(MAKEROOT)/Include/Common -I $(MAKEROOT)/Include/ -I $(MAKEROOT)/Include/IndustryStandard -I $(MAKEROOT)/Common/ -I .. -I . $(ARCH_INCLUDE)
BUILD_CPPFLAGS = $(INCLUDE)

# keep EXTRA_OPTFLAGS last
BUILD_OPTFLAGS = -O2 $(EXTRA_OPTFLAGS)

ifeq ($(DARWIN),Darwin)
# assume clang or clang compatible flags on OS X
BUILD_CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -Wall -Werror \
-Wno-deprecated-declarations -Wno-self-assign -Wno-unused-result -nostdlib -g
else
ifeq ($(CXX), llvm)
BUILD_CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -fwrapv \
-fno-delete-null-pointer-checks -Wall -Werror \
-Wno-deprecated-declarations -Wno-self-assign \
-Wno-unused-result -nostdlib -g
else
BUILD_CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -fwrapv \
-fno-delete-null-pointer-checks -Wall -Werror \
-Wno-deprecated-declarations -Wno-stringop-truncation -Wno-restrict \
-Wno-unused-result -nostdlib -g
endif
endif
ifeq ($(CXX), llvm)
BUILD_LFLAGS =
BUILD_CXXFLAGS = -Wno-deprecated-register -Wno-unused-result
else
BUILD_LFLAGS =
BUILD_CXXFLAGS = -Wno-unused-result
endif
ifeq ($(HOST_ARCH), IA32)
#
# Snow Leopard  is a 32-bit and 64-bit environment. uname -m returns i386, but gcc defaults
#  to x86_64. So make sure tools match uname -m. You can manual have a 64-bit kernal on Snow Leopard
#  so only do this is uname -m returns i386.
#
ifeq ($(DARWIN),Darwin)
  BUILD_CFLAGS   += -arch i386
  BUILD_CPPFLAGS += -arch i386
  BUILD_LFLAGS   += -arch i386
endif
endif

# keep BUILD_OPTFLAGS last
BUILD_CFLAGS   += $(BUILD_OPTFLAGS)
BUILD_CXXFLAGS += $(BUILD_OPTFLAGS)

# keep EXTRA_LDFLAGS last
BUILD_LFLAGS += $(EXTRA_LDFLAGS)

.PHONY: all
.PHONY: install
.PHONY: clean

all:

$(MAKEROOT)/libs:
	mkdir $(MAKEROOT)/libs

$(MAKEROOT)/bin:
	mkdir $(MAKEROOT)/bin
