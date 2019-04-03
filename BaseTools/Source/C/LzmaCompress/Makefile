## @file
# Windows makefile for 'LzmaCompress' module build.
#
# Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
!INCLUDE ..\Makefiles\ms.common

APPNAME = LzmaCompress

#LIBS = $(LIB_PATH)\Common.lib

SDK_C = Sdk\C

OBJECTS = \
  LzmaCompress.obj \
  $(SDK_C)\Alloc.obj \
  $(SDK_C)\LzFind.obj \
  $(SDK_C)\LzmaDec.obj \
  $(SDK_C)\LzmaEnc.obj \
  $(SDK_C)\7zFile.obj \
  $(SDK_C)\7zStream.obj \
  $(SDK_C)\Bra86.obj \
  $(SDK_C)\LzFindMt.obj \
  $(SDK_C)\Threads.obj

!INCLUDE ..\Makefiles\ms.app

all: $(BIN_PATH)\LzmaF86Compress.bat

$(BIN_PATH)\LzmaF86Compress.bat: LzmaF86Compress.bat
  copy LzmaF86Compress.bat $(BIN_PATH)\LzmaF86Compress.bat /Y

cleanall: localCleanall

localCleanall:
  del /f /q $(BIN_PATH)\LzmaF86Compress.bat > nul
