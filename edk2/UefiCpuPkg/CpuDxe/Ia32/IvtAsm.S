#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2009, Intel Corporation
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
#   IvtAsm.S
#
# Abstract:
#
#   Interrupt Vector Table
#
#------------------------------------------------------------------------------

#
# Interrupt Vector Table
#

.macro SingleIdtVectorMacro vectorNum
    call    ASM_PFX(CommonInterruptEntry)
    .short  \vectorNum
    nop
.endm

.macro EightIdtVectors firstVectorNum
    SingleIdtVectorMacro \firstVectorNum
    SingleIdtVectorMacro "(\firstVectorNum+1)"
    SingleIdtVectorMacro "(\firstVectorNum+2)"
    SingleIdtVectorMacro "(\firstVectorNum+3)"
    SingleIdtVectorMacro "(\firstVectorNum+4)"
    SingleIdtVectorMacro "(\firstVectorNum+5)"
    SingleIdtVectorMacro "(\firstVectorNum+6)"
    SingleIdtVectorMacro "(\firstVectorNum+7)"
.endm

.macro SixtyFourIdtVectors firstVectorNum
    EightIdtVectors \firstVectorNum
    EightIdtVectors "(\firstVectorNum+0x08)"
    EightIdtVectors "(\firstVectorNum+0x10)"
    EightIdtVectors "(\firstVectorNum+0x18)"
    EightIdtVectors "(\firstVectorNum+0x20)"
    EightIdtVectors "(\firstVectorNum+0x28)"
    EightIdtVectors "(\firstVectorNum+0x30)"
    EightIdtVectors "(\firstVectorNum+0x38)"
.endm

ASM_GLOBAL ASM_PFX(AsmIdtVector00)
.align  8
ASM_PFX(AsmIdtVector00):
    SixtyFourIdtVectors 0x00
    SixtyFourIdtVectors 0x40
    SixtyFourIdtVectors 0x80
    SixtyFourIdtVectors 0xC0
ASM_GLOBAL ASM_PFX(AsmCommonIdtEnd)
ASM_PFX(AsmCommonIdtEnd):
    .byte 0


