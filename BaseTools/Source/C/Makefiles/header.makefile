## @file
#
# The makefile can be invoked with
# HOST_ARCH = x86_64 or x64 for EM64T build
# HOST_ARCH = ia32 or IA32 for IA32 build
# HOST_ARCH = ia64 or IA64 for IA64 build
# HOST_ARCH = Arm or ARM for ARM build
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.    The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  endif
  ifneq (,$(findstring arm,$(uname_m)))
    HOST_ARCH=ARM
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

BUILD_CC ?= gcc
BUILD_CXX ?= g++
BUILD_AS ?= gcc
BUILD_AR ?= ar
BUILD_LD ?= ld
LINKER ?= $(BUILD_CC)
ifeq ($(HOST_ARCH), IA32)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/Ia32/

else ifeq ($(HOST_ARCH), X64)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/X64/

else ifeq ($(HOST_ARCH), ARM)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/Arm/

else ifeq ($(HOST_ARCH), AARCH64)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/AArch64/

else
$(error Bad HOST_ARCH)
endif

INCLUDE = $(TOOL_INCLUDE) -I $(MAKEROOT) -I $(MAKEROOT)/Include/Common -I $(MAKEROOT)/Include/ -I $(MAKEROOT)/Include/IndustryStandard -I $(MAKEROOT)/Common/ -I .. -I . $(ARCH_INCLUDE) 
BUILD_CPPFLAGS = $(INCLUDE) -O2
ifeq ($(DARWIN),Darwin)
# assume clang or clang compatible flags on OS X
BUILD_CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -Wall -Werror -Wno-deprecated-declarations -Wno-self-assign -Wno-unused-result -nostdlib -g
else
BUILD_CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -Wall -Werror -Wno-deprecated-declarations -Wno-stringop-truncation -Wno-restrict -Wno-unused-result -nostdlib -g
endif
BUILD_LFLAGS =
BUILD_CXXFLAGS = -Wno-unused-result

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

  
.PHONY: all
.PHONY: install
.PHONY: clean

all:

$(MAKEROOT)/libs:
	mkdir $(MAKEROOT)/libs 

$(MAKEROOT)/bin:
	mkdir $(MAKEROOT)/bin
