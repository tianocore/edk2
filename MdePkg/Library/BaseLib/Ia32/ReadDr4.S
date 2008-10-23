#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2008, Intel Corporation
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
#   ReadDr4.S
#
# Abstract:
#
#   AsmReadDr4 function
#
# Notes:
#
#------------------------------------------------------------------------------

.globl ASM_PFX(AsmReadDr4)

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmReadDr4 (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmReadDr4):
    movl    %dr4, %eax
    ret
