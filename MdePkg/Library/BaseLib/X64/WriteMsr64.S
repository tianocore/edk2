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
#   WriteMsr64.S
#
# Abstract:
#
#   AsmWriteMsr64 function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# AsmWriteMsr64 (
#   IN UINT32  Index,
#   IN UINT64  Value
#   );
#------------------------------------------------------------------------------
.global ASM_PFX(AsmWriteMsr64)
.align 16
ASM_PFX(AsmWriteMsr64):
    mov	%rdx, %rax            # meanwhile, rax <- return value
    shr $0x20, %rdx           # edx:eax contains the value to write
    wrmsr
    ret
