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
#   WriteMm4.Asm
#
# Abstract:
#
#   AsmWriteMm4 function
#
# Notes:
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# AsmWriteMm4 (
#   IN UINT64   Value
#   );
#------------------------------------------------------------------------------
.globl _AsmWriteMm4
_AsmWriteMm4:
    movq    4(%esp), %mm4
    ret
