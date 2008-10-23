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
#   ReadMm6.S
#
# Abstract:
#
#   AsmReadMm6 function
#
# Notes:
#
#------------------------------------------------------------------------------

.globl ASM_PFX(AsmReadMm6)

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmReadMm6 (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmReadMm6):
    push    %eax
    push    %eax
    movq    %mm6, (%esp)
    pop     %eax
    pop     %edx
    ret
