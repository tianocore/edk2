## @file
# Windows makefile for 'Brotli' module build.
#
# Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
!INCLUDE ..\Makefiles\ms.common

INC = -I .\include $(INC)
CFLAGS = $(CFLAGS) /W2

APPNAME = Brotli

#LIBS = $(LIB_PATH)\Common.lib

COMMON_OBJ = common\dictionary.obj common\transform.obj
DEC_OBJ = \
  dec\bit_reader.obj \
  dec\decode.obj \
  dec\huffman.obj \
  dec\state.obj
ENC_OBJ = \
  enc\backward_references.obj \
  enc\backward_references_hq.obj \
  enc\bit_cost.obj \
  enc\block_splitter.obj \
  enc\brotli_bit_stream.obj \
  enc\cluster.obj \
  enc\compress_fragment.obj \
  enc\compress_fragment_two_pass.obj \
  enc\dictionary_hash.obj \
  enc\encode.obj \
  enc\encoder_dict.obj \
  enc\entropy_encode.obj \
  enc\histogram.obj \
  enc\literal_cost.obj \
  enc\memory.obj \
  enc\metablock.obj \
  enc\static_dict.obj \
  enc\utf8_util.obj

OBJECTS = \
  tools\brotli.obj \
  $(COMMON_OBJ) \
  $(DEC_OBJ) \
  $(ENC_OBJ)

!INCLUDE ..\Makefiles\ms.app
