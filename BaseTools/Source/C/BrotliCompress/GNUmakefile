## @file
# GNU/Linux makefile for 'Brotli' module build.
#
# Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
MAKEROOT ?= ..

APPNAME = Brotli

OBJECTS = \
  tools/brotli.o \
  common/dictionary.o \
  common/transform.o \
  dec/bit_reader.o \
  dec/decode.o \
  dec/huffman.o \
  dec/state.o \
  enc/backward_references.o \
  enc/backward_references_hq.o \
  enc/bit_cost.o \
  enc/block_splitter.o \
  enc/brotli_bit_stream.o \
  enc/cluster.o \
  enc/compress_fragment.o \
  enc/compress_fragment_two_pass.o \
  enc/dictionary_hash.o \
  enc/encode.o \
  enc/encoder_dict.o \
  enc/entropy_encode.o \
  enc/histogram.o \
  enc/literal_cost.o \
  enc/memory.o \
  enc/metablock.o \
  enc/static_dict.o \
  enc/utf8_util.o

include $(MAKEROOT)/Makefiles/app.makefile

TOOL_INCLUDE = -I ./include
LIBS += -lm
