## @file
# GNU/Linux makefile for 'Brotli' module build.
#
# Copyright (c) 2017 - 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
MAKEROOT ?= ..

APPNAME = BrotliCompress

OBJECTS = \
  BrotliCompress.o \
  brotli/c/common/dictionary.o \
  brotli/c/common/transform.o \
  brotli/c/dec/bit_reader.o \
  brotli/c/dec/decode.o \
  brotli/c/dec/huffman.o \
  brotli/c/dec/state.o \
  brotli/c/enc/backward_references.o \
  brotli/c/enc/backward_references_hq.o \
  brotli/c/enc/bit_cost.o \
  brotli/c/enc/block_splitter.o \
  brotli/c/enc/brotli_bit_stream.o \
  brotli/c/enc/cluster.o \
  brotli/c/enc/compress_fragment.o \
  brotli/c/enc/compress_fragment_two_pass.o \
  brotli/c/enc/dictionary_hash.o \
  brotli/c/enc/encode.o \
  brotli/c/enc/encoder_dict.o \
  brotli/c/enc/entropy_encode.o \
  brotli/c/enc/histogram.o \
  brotli/c/enc/literal_cost.o \
  brotli/c/enc/memory.o \
  brotli/c/enc/metablock.o \
  brotli/c/enc/static_dict.o \
  brotli/c/enc/utf8_util.o

include $(MAKEROOT)/Makefiles/app.makefile

TOOL_INCLUDE = -I ./brotli/c/include
LIBS += -lm
