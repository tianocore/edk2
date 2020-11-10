## @file
# Windows makefile for 'Brotli' module build.
#
# Copyright (c) 2017 - 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
!INCLUDE ..\Makefiles\ms.common

INC = -I .\brotli\c\include $(INC)
CFLAGS = $(CFLAGS) /W2

APPNAME = BrotliCompress

#LIBS = $(LIB_PATH)\Common.lib

COMMON_OBJ = brotli\c\common\dictionary.obj brotli\c\common\transform.obj
DEC_OBJ = \
  brotli\c\dec\bit_reader.obj \
  brotli\c\dec\decode.obj \
  brotli\c\dec\huffman.obj \
  brotli\c\dec\state.obj
ENC_OBJ = \
  brotli\c\enc\backward_references.obj \
  brotli\c\enc\backward_references_hq.obj \
  brotli\c\enc\bit_cost.obj \
  brotli\c\enc\block_splitter.obj \
  brotli\c\enc\brotli_bit_stream.obj \
  brotli\c\enc\cluster.obj \
  brotli\c\enc\compress_fragment.obj \
  brotli\c\enc\compress_fragment_two_pass.obj \
  brotli\c\enc\dictionary_hash.obj \
  brotli\c\enc\encode.obj \
  brotli\c\enc\encoder_dict.obj \
  brotli\c\enc\entropy_encode.obj \
  brotli\c\enc\histogram.obj \
  brotli\c\enc\literal_cost.obj \
  brotli\c\enc\memory.obj \
  brotli\c\enc\metablock.obj \
  brotli\c\enc\static_dict.obj \
  brotli\c\enc\utf8_util.obj

OBJECTS = \
  BrotliCompress.obj \
  $(COMMON_OBJ) \
  $(DEC_OBJ) \
  $(ENC_OBJ)

!INCLUDE ..\Makefiles\ms.app
