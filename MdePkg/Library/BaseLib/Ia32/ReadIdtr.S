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
#   InternalX86ReadIdtr function
#
# Notes:
#
#------------------------------------------------------------------------------

.global _InternalX86ReadIdtr

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# InternalX86ReadIdtr (
#   OUT IA32_DESCRIPTOR  *Idtr
#   );
#------------------------------------------------------------------------------
_InternalX86ReadIdtr:
    movl    4(%esp), %eax
    sidt    (%eax)
    ret
