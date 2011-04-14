## @file
# Windows makefile for 'LzmaCompress' module build.
#
# Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
  $(SDK_C)\7zStream.obj

!INCLUDE ..\Makefiles\ms.app

