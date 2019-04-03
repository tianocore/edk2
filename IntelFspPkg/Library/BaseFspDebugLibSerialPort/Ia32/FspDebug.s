#------------------------------------------------------------------------------
#
# Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Abstract:
#
#   FSP Debug functions
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# UINT32 *
# EFIAPI
# GetStackFramePointer (
#   VOID
#   )
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(GetStackFramePointer)
ASM_PFX(GetStackFramePointer):
    mov    %ebp, %eax
    ret


