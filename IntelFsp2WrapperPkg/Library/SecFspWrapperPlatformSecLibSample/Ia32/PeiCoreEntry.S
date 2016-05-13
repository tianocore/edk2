#------------------------------------------------------------------------------
#
# Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#  PeiCoreEntry.S
#
# Abstract:
#
#   Find and call SecStartup
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(CallPeiCoreEntryPoint)
ASM_PFX(CallPeiCoreEntryPoint):
  #
  # Obtain the hob list pointer
  #
  movl    0x4(%esp), %eax
  #
  # Obtain the stack information
  #   ECX: start of range
  #   EDX: end of range
  #
  movl    0x8(%esp), %ecx
  movl    0xC(%esp), %edx

  #
  # Platform init
  #
  pushal
  pushl %edx
  pushl %ecx
  pushl %eax
  call  ASM_PFX(PlatformInit)
  popl  %eax
  popl  %eax
  popl  %eax
  popal

  #
  # Set stack top pointer
  #
  movl    %edx, %esp

  #
  # Push the hob list pointer
  #
  pushl   %eax

  #
  # Save the value
  #   ECX: start of range
  #   EDX: end of range
  #
  movl    %esp, %ebp
  pushl   %ecx
  pushl   %edx

  #
  # Push processor count to stack first, then BIST status (AP then BSP)
  #
  movl    $1, %eax
  cpuid
  shr     $16, %ebx
  andl    $0x000000FF, %ebx
  cmp     $1, %bl
  jae     PushProcessorCount

  #
  # Some processors report 0 logical processors.  Effectively 0 = 1.
  # So we fix up the processor count
  #
  inc     %ebx

PushProcessorCount:
  pushl   %ebx

  #
  # We need to implement a long-term solution for BIST capture.  For now, we just copy BSP BIST
  # for all processor threads
  #
  xorl    %ecx, %ecx
  movb    %bl, %cl
PushBist:
  movd    %mm0, %eax
  pushl   %eax
  loop    PushBist

  # Save Time-Stamp Counter
  movd  %mm5, %eax
  pushl %eax

  movd  %mm6, %eax
  pushl %eax

  #
  # Pass entry point of the PEI core
  #
  movl    $0xFFFFFFE0, %edi
  pushl   %ds:(%edi)

  #
  # Pass BFV into the PEI Core
  #
  movl    $0xFFFFFFFC, %edi
  pushl   %ds:(%edi)

  #
  # Pass stack size into the PEI Core
  #
  movl    -4(%ebp), %ecx
  movl    -8(%ebp), %edx
  pushl   %ecx       # RamBase

  subl    %ecx, %edx
  pushl   %edx       # RamSize

  #
  # Pass Control into the PEI Core
  #
  call ASM_PFX(SecStartup)
