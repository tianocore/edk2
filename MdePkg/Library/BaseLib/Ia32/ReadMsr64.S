#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   ReadMsr64.S
#
# Abstract:
#
#   AsmReadMsr64 function
#
# Notes:
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(AsmReadMsr64)

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
