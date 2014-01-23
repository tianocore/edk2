#------------------------------------------------------------------------------
#
# Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials                          
# are licensed and made available under the terms and conditions of the BSD License         
# which accompanies this distribution.  The full text of the license may be found at        
# http://opensource.org/licenses/bsd-license.php                                            
#                                                                                           
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.     
#
# Module Name:
#
#   SupportItpDebug.S
# 
# Abstract:
# 
#   This is the code for debuging X64, to add a break hook at loading every module
#
#------------------------------------------------------------------------------
#include <EfiBind.h>

  .text

.globl ASM_PFX(AsmEfiSetBreakSupport)

#------------------------------------------------------------------------------
#  VOID
#  AsmEfiSetBreakSupport (
#    IN UINTN  LoadAddr    // rcx
#    )
#------------------------------------------------------------------------------

ASM_PFX(AsmEfiSetBreakSupport):

    movw $60000, %dx
    outl %eax, %dx
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    ret




