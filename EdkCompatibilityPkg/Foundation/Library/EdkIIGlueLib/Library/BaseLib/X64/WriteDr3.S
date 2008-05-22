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
#   WriteDr3.S
#
# Abstract:
#
#   AsmWriteDr3 function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmWriteDr3 (
#   UINTN  Value
#   );
#------------------------------------------------------------------------------
.global _AsmWriteDr3;
.align 16;
_AsmWriteDr3:
    mov     %rcx, %dr3
    mov     %rcx, %rax
    ret
