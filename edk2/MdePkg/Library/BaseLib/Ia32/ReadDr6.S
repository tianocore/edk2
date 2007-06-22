//
// Include common header file for this module.
//
#include "CommonHeader.h"

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
#   ReadDr6.Asm
#
# Abstract:
#
#   AsmReadDr6 function
#
# Notes:
#
#------------------------------------------------------------------------------

.globl ASM_PFX(AsmReadDr6)

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmReadDr6 (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmReadDr6):
    movl    %dr6, %eax
    ret
