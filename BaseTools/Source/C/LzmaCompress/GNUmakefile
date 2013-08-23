## @file
# GNU/Linux makefile for 'LzmaCompress' module build.
#
# Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
ARCH ?= IA32
MAKEROOT ?= ..

APPNAME = LzmaCompress

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

include $(MAKEROOT)/Makefiles/app.makefile

