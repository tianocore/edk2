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
#   WriteDr5.Asm
#
# Abstract:
#
#   AsmWriteDr5 function
#
# Notes:
#
#------------------------------------------------------------------------------



     

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# AsmWriteDr5 (
#   IN UINTN Value
#   );
#------------------------------------------------------------------------------
.global _AsmWriteDr5
_AsmWriteDr5: 
    movl    4(%esp),%eax
    movl    %eax, %dr5
    ret



