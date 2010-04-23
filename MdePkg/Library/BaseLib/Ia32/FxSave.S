#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   FxSave.S
#
# Abstract:
#
#   InternalX86FxSave function
#
# Notes:
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(InternalX86FxSave)

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# InternalX86FxSave (
#   OUT IA32_FX_BUFFER *Buffer
#   );
#------------------------------------------------------------------------------
ASM_PFX(InternalX86FxSave):
    movl    4(%esp), %eax               # Buffer must be 16-byte aligned
    fxsave  (%eax)
    ret
