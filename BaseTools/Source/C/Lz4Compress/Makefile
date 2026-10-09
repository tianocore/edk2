#  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#  @file
#  Windows makefile for 'Lz4Compress' module build.
#

!INCLUDE ..\Makefiles\ms.common

APPNAME = Lz4Compress

LIBS = $(LIB_PATH)\Common.lib

LZ4LIB = lz4\lib

OBJECTS = \
  Lz4Compress.obj \
  $(LZ4LIB)\lz4.obj \
  $(LZ4LIB)\lz4hc.obj

INC = $(INC) -I lz4\lib

!INCLUDE ..\Makefiles\ms.app
