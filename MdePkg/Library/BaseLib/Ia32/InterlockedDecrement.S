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
#   InterlockedDecrement.Asm
#
# Abstract:
#
#   InternalSyncDecrement function
#
# Notes:
#
#------------------------------------------------------------------------------

.globl ASM_PFX(InternalSyncDecrement)

#------------------------------------------------------------------------------
# UINT32
# EFIAPI
# InternalSyncDecrement (
#   IN      UINT32                    *Value
#   );
#------------------------------------------------------------------------------
ASM_PFX(InternalSyncDecrement):
    movl    4(%esp), %eax
    lock
    decl    (%eax)
    movl    (%eax), %eax
    ret
