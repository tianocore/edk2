## @file
# GNU/Linux makefile for 'Common' module build.
#
# Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
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

# VPATH = ..

LIBNAME = Common

OBJECTS = \
  BasePeCoff.o \
  BinderFuncs.o \
  CommonLib.o \
  Crc32.o \
  Decompress.o \
  EfiCompress.o \
  EfiUtilityMsgs.o \
  FirmwareVolumeBuffer.o \
  FvLib.o \
  MemoryFile.o \
  MyAlloc.o \
  OsPath.o \
  ParseGuidedSectionTools.o \
  ParseInf.o \
  PeCoffLoaderEx.o \
  SimpleFileParsing.o \
  StringFuncs.o \
  TianoCompress.o

include $(MAKEROOT)/Makefiles/lib.makefile
