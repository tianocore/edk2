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
  brotli/c/common/platform.o \
  brotli/c/common/shared_dictionary.o \
  brotli/c/common/constants.o \
  brotli/c/common/context.o \
  brotli/c/enc/command.o \
  brotli/c/enc/compound_dictionary.o \
  brotli/c/enc/fast_log.o \
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

override NINJA_CSRCS := \
  BrotliCompress.c \
  brotli/c/common/platform.c \
  brotli/c/common/shared_dictionary.c \
  brotli/c/common/constants.c \
  brotli/c/common/context.c \
  brotli/c/enc/command.c \
  brotli/c/enc/compound_dictionary.c \
  brotli/c/enc/fast_log.c \
  brotli/c/common/dictionary.c \
  brotli/c/common/transform.c \
  brotli/c/dec/bit_reader.c \
  brotli/c/dec/decode.c \
  brotli/c/dec/huffman.c \
  brotli/c/dec/state.c \
  brotli/c/enc/backward_references.c \
  brotli/c/enc/backward_references_hq.c \
  brotli/c/enc/bit_cost.c \
  brotli/c/enc/block_splitter.c \
  brotli/c/enc/brotli_bit_stream.c \
  brotli/c/enc/cluster.c \
  brotli/c/enc/compress_fragment.c \
  brotli/c/enc/compress_fragment_two_pass.c \
  brotli/c/enc/dictionary_hash.c \
  brotli/c/enc/encode.c \
  brotli/c/enc/encoder_dict.c \
  brotli/c/enc/entropy_encode.c \
  brotli/c/enc/histogram.c \
  brotli/c/enc/literal_cost.c \
  brotli/c/enc/memory.c \
  brotli/c/enc/metablock.c \
  brotli/c/enc/static_dict.c \
  brotli/c/enc/utf8_util.c

override NINJA_CXXSRCS :=

include $(MAKEROOT)/Makefiles/app.makefile

TOOL_INCLUDE = -I $(MAKEROOT)/$(APPNAME)/brotli/c/include
LIBS += -lm
