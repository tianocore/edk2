## @file
# GNU/Linux makefile for 'Brotli' module build.
#
# Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
MAKEROOT ?= ..

APPNAME = Brotli

OBJECTS = \
  tools/bro.o \
  common/dictionary.o \
  dec/bit_reader.o \
  dec/decode.o \
  dec/huffman.o \
  dec/state.o \
  enc/backward_references.o \
  enc/bit_cost.o \
  enc/block_splitter.o \
  enc/brotli_bit_stream.o \
  enc/cluster.o \
  enc/compress_fragment.o \
  enc/compress_fragment_two_pass.o \
  enc/encode.o \
  enc/entropy_encode.o \
  enc/histogram.o \
  enc/literal_cost.o \
  enc/memory.o \
  enc/metablock.o \
  enc/static_dict.o \
  enc/utf8_util.o

include $(MAKEROOT)/Makefiles/app.makefile

LIBS += -lm
