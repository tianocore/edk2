#  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
#  SPDX-License-Identifier: BSD-3-Clause-Clear
#  @file
#  GNU/Linux makefile for 'Lz4Compress' module build.
#

MAKEROOT ?= ..

APPNAME = Lz4Compress

LIBS = -lCommon

LZ4LIB = lz4/lib

OBJECTS = \
  Lz4Compress.o \
  $(LZ4LIB)/lz4.o \
  $(LZ4LIB)/lz4hc.o

TOOL_INCLUDE = -I lz4/lib

include $(MAKEROOT)/Makefiles/app.makefile
