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
#   ReadMm0.Asm
#
# Abstract:
#
#   AsmReadMm0 function
#
# Notes:
#
#------------------------------------------------------------------------------

.global _AsmReadMm0

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmReadMm0 (
#   VOID
#   );
#------------------------------------------------------------------------------
_AsmReadMm0:
    push    %eax
    push    %eax
    movq    %mm0, (%esp)
    pop     %eax
    pop     %edx
    ret
