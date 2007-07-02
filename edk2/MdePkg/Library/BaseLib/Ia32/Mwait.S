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
#   Mwait.Asm
#
# Abstract:
#
#   AsmMwait function
#
# Notes:
#
#------------------------------------------------------------------------------

.globl ASM_PFX(AsmMwait)

#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# AsmMwait (
#   IN      UINTN                     Eax,
#   IN      UINTN                     Ecx
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmMwait):
    movl    4(%esp), %eax
    movl    8(%esp), %ecx
    mwait   %eax, %ecx
    ret
