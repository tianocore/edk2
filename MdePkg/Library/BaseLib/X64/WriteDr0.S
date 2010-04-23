#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
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
#   WriteDr0.S
#
# Abstract:
#
#   AsmWriteDr0 function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmWriteDr0 (
#   UINTN  Value
#   );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(AsmWriteDr0)
ASM_PFX(AsmWriteDr0):
    mov     %rcx, %dr0
    mov     %rcx, %rax
    ret
