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
#   ReadMsr64.Asm
#
# Abstract:
#
#   AsmReadMsr64 function
#
# Notes:
#
#------------------------------------------------------------------------------

.globl ASM_PFX(AsmReadMsr64)

#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# AsmReadMsr64 (
#   IN UINT32  Index
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmReadMsr64):
    movl    4(%esp), %ecx
    rdmsr
    ret
