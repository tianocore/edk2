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
#   ReadMm5.S
#
# Abstract:
#
#   AsmReadMm5 function
#
# Notes:
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(AsmReadMm5)

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmReadMm5 (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmReadMm5):
    push    %eax
    push    %eax
    movq    %mm5, (%esp)
    pop     %eax
    pop     %edx
    ret
