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
#   ReadPmc.S
#
# Abstract:
#
#   AsmReadPmc function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# AsmReadPmc (
#   IN UINT32   PmcIndex
#   );
#------------------------------------------------------------------------------
.global ASM_PFX(AsmReadPmc)
.align 16
ASM_PFX(AsmReadPmc):
    rdpmc
    shl     $0x20, %rdx
    or      %rdx, %rax
    ret
