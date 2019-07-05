## @file GNUmakefile
#
# GNU makefile for 'FCE' module build.
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

ifndef ARCH
  #
  # If ARCH is not defined, then we use 'uname -m' to attempt
  # try to figure out the appropriate ARCH.
  #
  uname_m = $(shell uname -m)
  $(info Attempting to detect ARCH from 'uname -m': $(uname_m))
  ifneq (,$(strip $(filter $(uname_m), x86_64 amd64)))
    ARCH=X64
  endif
  ifeq ($(patsubst i%86,IA32,$(uname_m)),IA32)
    ARCH=IA32
  endif
  ifneq (,$(findstring aarch64,$(uname_m)))
    ARCH=AARCH64
  endif
  ifneq (,$(findstring arm,$(uname_m)))
    ARCH=ARM
  endif
  ifndef ARCH
    $(info Could not detected ARCH from uname results)
    $(error ARCH is not defined!)
  endif
  $(info Detected ARCH of $(ARCH) using uname.)
endif

export ARCH
export HOST_ARCH=$(ARCH)

MAKEROOT ?= ..

APPNAME = FCE

OBJECTS = Fce.o Variable.o TimeBasedVariable.o MonotonicBasedVariable.o IfrParse.o Common.o BinaryParse.o BinaryCreate.o Expression.o

include $(MAKEROOT)/Makefiles/app.makefile

LIBS = -lCommon -lm


