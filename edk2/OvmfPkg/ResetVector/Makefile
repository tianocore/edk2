## @file
#  Makefile to create FFS Raw sections for VTF images.
#
#  Copyright (c) 2008, Intel Corporation
#
#  All rights reserved. This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

TARGETS = Bin/ResetVector.ia32.raw Bin/ResetVector.x64.raw
ASM = nasm

COMMON_DEPS = \
  Ia16/16RealTo32Flat.asm \
  Ia32/32FlatTo64Flat.asm \
  JumpToSec.asm \
  Ia16/ResetVectorVtf0.asm \
  Ia32/SearchForBfvBase.asm \
  Ia32/SearchForSecAndPeiEntries.asm \
  SerialDebug.asm \
  Makefile \
  Tools/FixupForRawSection.py

.PHONY: all
all: $(TARGETS)

Bin/ResetVector.ia32.raw: $(COMMON_DEPS) ResetVectorCode.asm
	nasm -D ARCH_IA32 -o $@ ResetVectorCode.asm
	python Tools/FixupForRawSection.py $@

Bin/ResetVector.x64.raw: $(COMMON_DEPS) ResetVectorCode.asm
	nasm -D ARCH_X64 -o $@ ResetVectorCode.asm
	python Tools/FixupForRawSection.py $@

clean:
	-rm $(TARGETS)

