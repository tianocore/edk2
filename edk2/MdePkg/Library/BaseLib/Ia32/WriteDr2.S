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
#   WriteDr2.Asm
#
# Abstract:
#
#   AsmWriteDr2 function
#
# Notes:
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmWriteDr2 (
#   IN UINTN Value
#   );
#------------------------------------------------------------------------------
.globl _AsmWriteDr2
_AsmWriteDr2:
    movl    4(%esp), %eax
    movl    %eax, %dr2
    ret
