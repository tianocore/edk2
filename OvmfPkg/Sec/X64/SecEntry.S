#      TITLE   SecEntry.asm

#------------------------------------------------------------------------------
#*
#*   Copyright 2006 - 2009, Intel Corporation
#*   All rights reserved. This program and the accompanying materials
#*   are licensed and made available under the terms and conditions of the BSD License
#*   which accompanies this distribution.  The full text of the license may be found at
#*   http://opensource.org/licenses/bsd-license.php
#*
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#*
#*    CpuAsm.asm
#*
#*   Abstract:
#*
#------------------------------------------------------------------------------


#include "SecMain.h"

#EXTERN ASM_PFX(SecCoreStartupWithStack)

#
#  SecCore Entry Point
#
#  Processor is in flat protected mode
#
#  @param  ESI  Pointer to SEC Core Entry Point (this function)
#  @param  EDI  Pointer to PEI Core Entry Point
#  @param  EBP  Pointer to the start of the Boot Firmware Volume
#
#  @return None
#
#
ASM_GLOBAL ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):

    #
    # Load temporary stack top at very low memory.  The C code
    # can reload to a better address.
    #
    movq    $INITIAL_TOP_OF_STACK, %rsp 
    nop

    #
    # Setup parameters and call SecCoreStartupWithStack
    #   rcx: BootFirmwareVolumePtr
    #   rdx: SecCoreEntryPoint
    #   r8:  PeiCoreEntryPoint
    #   r9:  TopOfCurrentStack
    #
    movq    %rbp, %rcx 
    movq    %rsi, %rdx
    movq    %rdi, %r8
    movq    %rsp, %r9
    call    ASM_PFX(SecCoreStartupWithStack)

