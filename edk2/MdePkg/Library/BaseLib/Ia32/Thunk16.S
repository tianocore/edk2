#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   Thunk.asm
#
# Abstract:
#
#   Real mode thunk
#
#------------------------------------------------------------------------------

.global _m16Start, _m16Size, _mThunk16Attr, _m16GdtBase, _m16Gdt, _m16GdtrBase, _mTransition
                        ##########
                        # FIXME! #
                        ##########
# The following data are INVALID!!
# They just follow GAS syntax.
_m16Start:        .byte   0x00
_m16Size:         .word   0x00
_mThunk16Attr:    .word   0x00
_m16Gdt:          .word   0x00
_m16GdtrBase:     .word   0x00
_mTransition:     .word   0x00

.global _InternalAsmThunk16
_InternalAsmThunk16:

                        ##########
                        # FIXME! #
                        ##########
 # This function won't work for now.
 # it will directly enter dead loop.
 jmp .
 