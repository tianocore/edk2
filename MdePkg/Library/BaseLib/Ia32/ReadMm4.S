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
#   ReadMm4.Asm
#
# Abstract:
#
#   AsmReadMm4 function
#
# Notes:
#
#------------------------------------------------------------------------------



     
     

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmReadMm4 (
#   VOID
#   );
#------------------------------------------------------------------------------
.global _AsmReadMm4
_AsmReadMm4: 
    pushl   %eax
    pushl   %eax
    movq    %mm4,(%esp)
    popl    %eax
    popl    %edx
    ret



