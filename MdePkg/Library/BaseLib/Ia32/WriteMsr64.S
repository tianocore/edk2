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
ASM_GLOBAL ASM_PFX(AsmWriteMsr64)
ASM_PFX(AsmWriteMsr64):
    movl    12(%esp), %edx
    movl    8(%esp), %eax
    movl    4(%esp), %ecx
    wrmsr
    ret
