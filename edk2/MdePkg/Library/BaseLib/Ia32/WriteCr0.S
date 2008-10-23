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
#   WriteCr0.S
#
# Abstract:
#
#   AsmWriteCr0 function
#
# Notes:
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmWriteCr0 (
#   IN      UINTN                     Cr0
#   );
#------------------------------------------------------------------------------
.globl ASM_PFX(AsmWriteCr0)
ASM_PFX(AsmWriteCr0):
    movl    4(%esp), %eax
    movl    %eax, %cr0
    ret
