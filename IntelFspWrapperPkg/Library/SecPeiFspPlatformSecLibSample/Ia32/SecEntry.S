#------------------------------------------------------------------------------
#
# Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Module Name:
#
#  SecEntry.S
#
# Abstract:
#
#  This is the code that goes from real-mode to protected mode.
#  It consumes the reset vector, calls TempRamInit API from FSP binary.
#
#------------------------------------------------------------------------------

#include "Fsp.h"

ASM_GLOBAL    ASM_PFX(_gPcd_FixedAtBuild_PcdFlashFvFspBase)
ASM_GLOBAL    ASM_PFX(_gPcd_FixedAtBuild_PcdFlashFvFspSize)

ASM_GLOBAL ASM_PFX(_TEXT_REALMODE)
ASM_PFX(_TEXT_REALMODE):
#----------------------------------------------------------------------------
#
# Procedure:    _ModuleEntryPoint
#
# Input:        None
#
# Output:       None
#
# Destroys:     Assume all registers
#
# Description:
#
#   Transition to non-paged flat-model protected mode from a
#   hard-coded GDT that provides exactly two descriptors.
#   This is a bare bones transition to protected mode only
#   used for a while in PEI and possibly DXE.
#
#   After enabling protected mode, a far jump is executed to
#   transfer to PEI using the newly loaded GDT.
#
# Return:       None
#
#  MMX Usage:
#              MM0 = BIST State
#              MM5 = Save time-stamp counter value high32bit
#              MM6 = Save time-stamp counter value low32bit.
#
#----------------------------------------------------------------------------

.align 4
ASM_GLOBAL ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
  fninit                                # clear any pending Floating point exceptions
  #
  # Store the BIST value in mm0
  #
  movd    %eax, %mm0

  #
  # Save time-stamp counter value
  # rdtsc load 64bit time-stamp counter to EDX:EAX
  #
  rdtsc
  movd    %edx, %mm5
  movd    %ecx, %mm6

  #
  # Load the GDT table in GdtDesc
  #
  movl    $GdtDesc, %esi
  .byte   0x66
  lgdt    %cs:(%si)

  #
  # Transition to 16 bit protected mode
  #
  movl    %cr0, %eax                 # Get control register 0
  orl     $0x00000003, %eax          # Set PE bit (bit #0) & MP bit (bit #1)
  movl    %eax, %cr0                 # Activate protected mode

  movl    %cr4, %eax                 # Get control register 4
  orl     $0x00000600, %eax          # Set OSFXSR bit (bit #9) & OSXMMEXCPT bit (bit #10)
  movl    %eax, %cr4

  #
  # Now we're in 16 bit protected mode
  # Set up the selectors for 32 bit protected mode entry
  #
  movw    SYS_DATA_SEL, %ax
  movw    %ax, %ds
  movw    %ax, %es
  movw    %ax, %fs
  movw    %ax, %gs
  movw    %ax, %ss

  #
  # Transition to Flat 32 bit protected mode
  # The jump to a far pointer causes the transition to 32 bit mode
  #
  movl    ASM_PFX(ProtectedModeEntryLinearAddress), %esi
  jmp     *%cs:(%si)

ASM_GLOBAL ASM_PFX(_TEXT_PROTECTED_MODE)
ASM_PFX(_TEXT_PROTECTED_MODE):

#----------------------------------------------------------------------------
#
# Procedure:    ProtectedModeEntryPoint
#
# Input:        None
#
# Output:       None
#
# Destroys:     Assume all registers
#
# Description:
#
# This function handles:
#   Call two basic APIs from FSP binary
#   Initializes stack with some early data (BIST, PEI entry, etc)
#
# Return:       None
#
#----------------------------------------------------------------------------

.align 4
ASM_GLOBAL ASM_PFX(ProtectedModeEntryPoint)
ASM_PFX(ProtectedModeEntryPoint):

  # Find the fsp info header
  movl ASM_PFX(_gPcd_FixedAtBuild_PcdFlashFvFspBase), %edi
  movl ASM_PFX(_gPcd_FixedAtBuild_PcdFlashFvFspSize), %ecx

  movl FVH_SIGINATURE_OFFSET(%edi), %eax
  cmp  $FVH_SIGINATURE_VALID_VALUE, %eax
  jnz  FspHeaderNotFound

  xorl %eax, %eax
  movw FVH_EXTHEADER_OFFSET_OFFSET(%edi), %ax
  cmp  %ax, 0
  jnz  FspFvExtHeaderExist

  xorl %eax, %eax
  movw FVH_HEADER_LENGTH_OFFSET(%edi), %ax   # Bypass Fv Header
  addl %eax, %edi
  jmp  FspCheckFfsHeader

FspFvExtHeaderExist:
  addl %eax, %edi
  movl FVH_EXTHEADER_SIZE_OFFSET(%edi), %eax  # Bypass Ext Fv Header
  addl %eax, %edi

  # Round up to 8 byte alignment
  movl %edi, %eax
  andb $0x07, %al
  jz FspCheckFfsHeader

  and  $0xFFFFFFF8, %edi
  add  $0x08, %edi

FspCheckFfsHeader:
  # Check the ffs guid
  movl (%edi), %eax
  cmp  $FSP_HEADER_GUID_DWORD1, %eax
  jnz  FspHeaderNotFound

  movl 0x4(%edi), %eax
  cmp  $FSP_HEADER_GUID_DWORD2, %eax
  jnz  FspHeaderNotFound

  movl 0x08(%edi), %eax
  cmp  $FSP_HEADER_GUID_DWORD3, %eax
  jnz  FspHeaderNotFound

  movl 0x0c(%edi), %eax
  cmp  $FSP_HEADER_GUID_DWORD4, %eax
  jnz  FspHeaderNotFound

  add  $FFS_HEADER_SIZE_VALUE, %edi        # Bypass the ffs header

  # Check the section type as raw section
  movb SECTION_HEADER_TYPE_OFFSET(%edi), %al
  cmp  $0x19, %al
  jnz  FspHeaderNotFound

  addl $RAW_SECTION_HEADER_SIZE_VALUE, %edi  # Bypass the section header
  jmp  FspHeaderFound

FspHeaderNotFound:
  jmp  .

FspHeaderFound:
  # Get the fsp TempRamInit Api address
  movl FSP_HEADER_IMAGEBASE_OFFSET(%edi), %eax
  addl FSP_HEADER_TEMPRAMINIT_OFFSET(%edi), %eax

  # Setup the hardcode stack
  movl $TempRamInitStack, %esp

  # Call the fsp TempRamInit Api
  jmp  *%eax

TempRamInitDone:
  cmp  $0x8000000E, %eax   #Check if EFI_NOT_FOUND returned. Error code for Microcode Update not found.
  je   CallSecFspInit      #If microcode not found, don't hang, but continue.

  cmp  $0x0, %eax
  jnz  FspApiFailed

  #   ECX: start of range
  #   EDX: end of range
CallSecFspInit:
  xorl    %eax, %eax
  movl    %edx, %esp

  # Align the stack at DWORD
  addl  $3, %esp
  andl  $0xFFFFFFFC, %esp

  pushl   %edx
  pushl   %ecx
  pushl   %eax # zero - no hob list yet
  call ASM_PFX(CallPeiCoreEntryPoint)

FspApiFailed:
  jmp .

.align 0x10
TempRamInitStack:
    .long  TempRamInitDone
    .long  ASM_PFX(TempRamInitParams)

#
# ROM-based Global-Descriptor Table for the Tiano PEI Phase
#
.align 16

#
# GDT[0]: 0x00: Null entry, never used.
#
.equ NULL_SEL,             . - GDT_BASE    # Selector [0]
GDT_BASE:
BootGdtTable:       .long  0
                    .long  0
#
# Linear data segment descriptor
#
.equ LINEAR_SEL,           . - GDT_BASE    # Selector [0x8]
    .word  0xFFFF                          # limit 0xFFFFF
    .word  0                               # base 0
    .byte  0
    .byte  0x92                            # present, ring 0, data, expand-up, writable
    .byte  0xCF                            # page-granular, 32-bit
    .byte  0
#
# Linear code segment descriptor
#
.equ LINEAR_CODE_SEL,      . - GDT_BASE    # Selector [0x10]
    .word  0xFFFF                          # limit 0xFFFFF
    .word  0                               # base 0
    .byte  0
    .byte  0x9B                            # present, ring 0, data, expand-up, not-writable
    .byte  0xCF                            # page-granular, 32-bit
    .byte  0
#
# System data segment descriptor
#
.equ SYS_DATA_SEL,         . - GDT_BASE    # Selector [0x18]
    .word  0xFFFF                          # limit 0xFFFFF
    .word  0                               # base 0
    .byte  0
    .byte  0x93                            # present, ring 0, data, expand-up, not-writable
    .byte  0xCF                            # page-granular, 32-bit
    .byte  0

#
# System code segment descriptor
#
.equ SYS_CODE_SEL,         . - GDT_BASE    # Selector [0x20]
    .word  0xFFFF                          # limit 0xFFFFF
    .word  0                               # base 0
    .byte  0
    .byte  0x9A                            # present, ring 0, data, expand-up, writable
    .byte  0xCF                            # page-granular, 32-bit
    .byte  0
#
# Spare segment descriptor
#
.equ SYS16_CODE_SEL,       . - GDT_BASE    # Selector [0x28]
    .word  0xFFFF                          # limit 0xFFFFF
    .word  0                               # base 0
    .byte  0x0E                            # Changed from F000 to E000.
    .byte  0x9B                            # present, ring 0, code, expand-up, writable
    .byte  0x00                            # byte-granular, 16-bit
    .byte  0
#
# Spare segment descriptor
#
.equ SYS16_DATA_SEL,       . - GDT_BASE    # Selector [0x30]
    .word  0xFFFF                          # limit 0xFFFF
    .word  0                               # base 0
    .byte  0
    .byte  0x93                            # present, ring 0, data, expand-up, not-writable
    .byte  0x00                            # byte-granular, 16-bit
    .byte  0

#
# Spare segment descriptor
#
.equ SPARE5_SEL,           . - GDT_BASE    # Selector [0x38]
    .word  0                               # limit 0
    .word  0                               # base 0
    .byte  0
    .byte  0                               # present, ring 0, data, expand-up, writable
    .byte  0                               # page-granular, 32-bit
    .byte  0
.equ GDT_SIZE,             . - BootGdtTable    # Size, in bytes

#
# GDT Descriptor
#
GdtDesc:                                # GDT descriptor
    .word  GDT_SIZE - 1                    # GDT limit
    .long  BootGdtTable                    # GDT base address

ASM_PFX(ProtectedModeEntryLinearAddress):
ProtectedModeEntryLinearOffset:
  .long      ASM_PFX(ProtectedModeEntryPoint)  # Offset of our 32 bit code
  .word      LINEAR_CODE_SEL
