## @file
# GNU/Linux makefile for 'LzmaCompress' module build.
#
# Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
MAKEROOT ?= ..

APPNAME = LzmaCompress

LIBS = -lCommon

SDK_C = Sdk/C

OBJECTS = \
  LzmaCompress.o \
  $(SDK_C)/Alloc.o \
  $(SDK_C)/LzFind.o \
  $(SDK_C)/LzmaDec.o \
  $(SDK_C)/LzmaEnc.o \
  $(SDK_C)/7zFile.o \
  $(SDK_C)/7zStream.o \
  $(SDK_C)/Bra86.o

override NINJA_CSRCS := \
  LzmaCompress.c \
  $(SDK_C)/Alloc.c \
  $(SDK_C)/LzFind.c \
  $(SDK_C)/LzmaDec.c \
  $(SDK_C)/LzmaEnc.c \
  $(SDK_C)/7zFile.c \
  $(SDK_C)/7zStream.c \
  $(SDK_C)/Bra86.c

override NINJA_CXXSRCS :=

NINJA_LIBS_DEPS = Common

include $(MAKEROOT)/Makefiles/app.makefile

CFLAGS += -D_7ZIP_ST
