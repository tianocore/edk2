#------------------------------------------------------------------------------
#
# Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Abstract:
#
#  This is the code that goes from real-mode to protected mode.
#  It consumes the reset vector, configures the stack.
#
#------------------------------------------------------------------------------


#
# Contrary to the name, this file contains 16 bit code as well.
#
.text

ASM_GLOBAL ASM_PFX(SecPlatformInit)
ASM_PFX(SecPlatformInit):
  movd    %mm7, %esi                      # restore ESP from MM7
  jmp     *%esi

#----------------------------------------------------------------------------
#
# Procedure:    ProtectedModeEntryPoint
#
# Input:        Executing in 32 Bit Protected (flat) mode
#               cs: 0-4GB
#               ds: 0-4GB
#               es: 0-4GB
#               fs: 0-4GB
#               gs: 0-4GB
#               ss: 0-4GB
#
# Output:       This function never returns
#
# Destroys:
#               ecx
#               edi
#               esi
#               esp
#
# Description:
#               Perform any essential early platform initilaisation
#               Setup a stack
#
#----------------------------------------------------------------------------
ProtectedModeEntryPoint:
  #
  # Dummy function. Consume 2 API to make sure they can be linked.
  #
  movl   ASM_PFX(TempRamInitApi), %eax
  #
  # Should never return
  #
  jmp     . #'$'

#
# ROM-based Global-Descriptor Table for the PEI Phase
#
.align 16
#
# GDT[0]: 000h: Null entry, never used.
#
.equ   NULL_SEL, . - GDT_BASE         # Selector [0]
GDT_BASE: 
BootGdtTable:   
        .long   0
        .long   0
#
# Linear code segment descriptor
#
.equ     LINEAR_CODE_SEL, . - GDT_BASE         # Selector [08h]
        .word   0xFFFF                      # limit 0FFFFh
        .word   0                           # base 0
        .byte   0
        .byte   0x9B                        # present, ring 0, data, expand-up, not-writable
        .byte   0xCF                        # page-granular, 32-bit
        .byte   0
#
# System data segment descriptor
#
.equ    SYS_DATA_SEL, . - GDT_BASE         # Selector [010h]
        .word   0xFFFF                      # limit 0FFFFh
        .word   0                           # base 0
        .byte   0
        .byte   0x93                        # present, ring 0, data, expand-up, not-writable
        .byte   0xCF                        # page-granular, 32-bit
        .byte   0

.equ            GDT_SIZE, . - BootGdtTable  # Size, in bytes

#
# GDT Descriptor
#
GdtDesc:                                     # GDT descriptor
       .word    GDT_SIZE - 1               
       .long    BootGdtTable        

ProtectedModeEntryLinearAddress:
ProtectedModeEntryLinearOffset:
       .long    ProtectedModeEntryPoint
       .word    LINEAR_CODE_SEL
