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
#   ReadDr2.S
#
# Abstract:
#
#   AsmReadDr2 function
#
# Notes:
#
#------------------------------------------------------------------------------

.globl ASM_PFX(AsmReadDr2)

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmReadDr2 (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmReadDr2):
    movl    %dr2, %eax
    ret
