#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2009, Intel Corporation
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
#   ReadMm7.S
#
# Abstract:
#
#   AsmReadMm7 function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# AsmReadMm7 (
#   VOID
#   );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(AsmReadMm7)
ASM_PFX(AsmReadMm7):
    movd  %mm7, %rax
    ret
