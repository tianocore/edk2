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
#   ReadCr3.S
#
# Abstract:
#
#   AsmReadCr3 function
#
# Notes:
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(AsmReadCr3)

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmReadCr3 (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmReadCr3):
    movl    %cr3, %eax
    ret
