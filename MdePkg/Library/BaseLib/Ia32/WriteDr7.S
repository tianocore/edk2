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
#   WriteDr7.S
#
# Abstract:
#
#   AsmWriteDr7 function
#
# Notes:
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmWriteDr7 (
#   IN UINTN Value
#   );
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(AsmWriteDr7)
ASM_PFX(AsmWriteDr7):
    movl    4(%esp), %eax
    movl    %eax, %dr7
    ret
