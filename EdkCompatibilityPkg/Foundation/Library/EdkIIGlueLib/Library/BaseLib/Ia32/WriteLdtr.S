#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
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
#   WriteLdtr.Asm
#
# Abstract:
#
#   AsmWriteLdtr function
#
# Notes:
#
#------------------------------------------------------------------------------

#include <EdkIIGlueBase.h>

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# AsmWriteLdtr (
#   IN UINT16 Ldtr
#   );
#------------------------------------------------------------------------------
.globl ASM_PFX(AsmWriteLdtr)
ASM_PFX(AsmWriteLdtr):
    movl    4(%esp), %eax
    lldtw   %ax
    ret
