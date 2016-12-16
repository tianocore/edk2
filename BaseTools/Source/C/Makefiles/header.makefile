## @file
#
# The makefile can be invoked with
# ARCH = x86_64 or x64 for EM64T build
# ARCH = ia32 or IA32 for IA32 build
# ARCH = ia64 or IA64 for IA64 build
# ARCH = Arm or ARM for ARM build
#
# Copyright (c) 2007 - 2016, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.    The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

ARCH ?= IA32

CYGWIN:=$(findstring CYGWIN, $(shell uname -s))
LINUX:=$(findstring Linux, $(shell uname -s))
DARWIN:=$(findstring Darwin, $(shell uname -s))

BUILD_CC ?= gcc
BUILD_CXX ?= g++
BUILD_AS ?= gcc
BUILD_AR ?= ar
BUILD_LD ?= ld
LINKER ?= $(BUILD_CC)
ifeq ($(ARCH), IA32)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/Ia32/
endif

ifeq ($(ARCH), X64)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/X64/
endif

ifeq ($(ARCH), ARM)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/Arm/
endif

ifeq ($(ARCH), AARCH64)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/AArch64/
endif

INCLUDE = $(TOOL_INCLUDE) -I $(MAKEROOT) -I $(MAKEROOT)/Include/Common -I $(MAKEROOT)/Include/ -I $(MAKEROOT)/Include/IndustryStandard -I $(MAKEROOT)/Common/ -I .. -I . $(ARCH_INCLUDE) 
BUILD_CPPFLAGS = $(INCLUDE) -O2
ifeq ($(DARWIN),Darwin)
# assume clang or clang compatible flags on OS X
BUILD_CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -Wall -Werror -Wno-deprecated-declarations -Wno-self-assign -Wno-unused-result -nostdlib -c -g
else
BUILD_CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -Wall -Werror -Wno-deprecated-declarations -Wno-unused-result -nostdlib -c -g
endif
BUILD_LFLAGS =
BUILD_CXXFLAGS = -Wno-unused-result

ifeq ($(ARCH), IA32)
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
