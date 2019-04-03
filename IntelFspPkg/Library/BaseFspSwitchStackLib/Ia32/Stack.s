#------------------------------------------------------------------------------
#
# Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Abstract:
#
#   Switch the stack from temporary memory to permenent memory.
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# UINT32
# EFIAPI
# Pei2LoaderSwitchStack (
#   VOID
#   )
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(Pei2LoaderSwitchStack)
ASM_PFX(Pei2LoaderSwitchStack):
    xorl    %eax, %eax
    jmp     ASM_PFX(FspSwitchStack)

#------------------------------------------------------------------------------
# UINT32
# EFIAPI
# Loader2PeiSwitchStack (
#   VOID
#   )
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(Loader2PeiSwitchStack)
ASM_PFX(Loader2PeiSwitchStack):
    jmp     ASM_PFX(FspSwitchStack)

#------------------------------------------------------------------------------
# UINT32
# EFIAPI
# FspSwitchStack (
#   VOID
#   )
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(FspSwitchStack)
ASM_PFX(FspSwitchStack):
    #
    #Save current contexts
    #
    push    %eax
    pushf
    cli
    pusha
    sub     $0x08, %esp
    sidt    (%esp)

    #
    # Load new stack
    #
    push   %esp
    call   ASM_PFX(SwapStack)
    movl   %eax, %esp

    #
    # Restore previous contexts
    #
    lidt    (%esp)
    add     $0x08,%esp
    popa
    popf
    add     $0x04,%esp
    ret


