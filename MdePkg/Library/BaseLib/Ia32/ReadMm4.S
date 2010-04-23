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
#   ReadMm4.S
#
# Abstract:
#
#   AsmReadMm4 function
#
# Notes:
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(AsmReadMm4)

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmReadMm4 (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmReadMm4):
    push    %eax
    push    %eax
    movq    %mm4, (%esp)
    pop     %eax
    pop     %edx
    ret
