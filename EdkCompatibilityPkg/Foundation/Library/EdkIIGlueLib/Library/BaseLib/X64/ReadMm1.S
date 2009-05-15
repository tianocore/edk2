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
#   ReadMm1.S
#
# Abstract:
#
#   AsmReadMm1 function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# AsmReadMm1 (
#   VOID
#   );
#------------------------------------------------------------------------------
.global ASM_PFX(AsmReadMm1)
.align 16
ASM_PFX(AsmReadMm1):
    #DB      48h, 0fh, 7eh, 0c8h
    movd	%mm1, %rax
    ret
