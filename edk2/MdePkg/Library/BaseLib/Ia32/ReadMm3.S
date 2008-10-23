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
#   ReadMm3.S
#
# Abstract:
#
#   AsmReadMm3 function
#
# Notes:
#
#------------------------------------------------------------------------------

.globl ASM_PFX(AsmReadMm3)

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmReadMm3 (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmReadMm3):
    push    %eax
    push    %eax
    movq    %mm3, (%esp)
    pop     %eax
    pop     %edx
    ret
