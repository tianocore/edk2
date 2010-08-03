#      TITLE   SecEntry.asm
#------------------------------------------------------------------------------
#*
#*   Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
#*   This program and the accompanying materials
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


#include <Base.h>

#EXTERN ASM_PFX(SecCoreStartupWithStack)

#
# SecCore Entry Point
#
# Processor is in flat protected mode
#
# @param[in]  EAX   Initial value of the EAX register (BIST: Built-in Self Test)
# @param[in]  DI    'BP': boot-strap processor, or 'AP': application processor
# @param[in]  EBP   Pointer to the start of the Boot Firmware Volume
#
# @return     None  This routine does not return
#
ASM_GLOBAL ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):

    #
    # Load temporary stack top at very low memory.  The C code
    # can reload to a better address.
    #
    movl    $BASE_512KB, %eax 
    movl    %eax, %esp
    nop

    #
    # Call into C code
    #
    pushl   %eax
    pushl   %ebp
    call    ASM_PFX(SecCoreStartupWithStack)


#END

