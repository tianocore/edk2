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
#   ReadIdtr.Asm
#
# Abstract:
#
#   AsmPeiSevicesTablePointer function
#
# Notes:
#
#------------------------------------------------------------------------------

.globl ASM_PFX(AsmPeiSevicesTablePointer)

#------------------------------------------------------------------------------
# EFI_PEI_SERVICES **
# EFIAPI
# AsmPeiSevicesTablePointer (
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmPeiSevicesTablePointer):
    movl    4(%esp), %eax
    sidt    (%eax)
    movl    -4(%eax), %eax
    ret
