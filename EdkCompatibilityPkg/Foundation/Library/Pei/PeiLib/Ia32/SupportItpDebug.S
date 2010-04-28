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
#   SupportItpDebug.asm
# 
# Abstract:
# 
#   This is the code for debuging IA32, to add a break hook at loading every module
#
#------------------------------------------------------------------------------
#include <EfiBind.h>

# PROC:PRIVATE
  .686P: 
  .MMX: 
  #.MODEL SMALL
  .CODE: 

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
.globl ASM_PFX(AsmEfiSetBreakSupport)  

#  VOID
#  AsmEfiSetBreakSupport (
#    IN UINTN  LoadAddr
#    )
#------------------------------------------------------------------------------
ASM_PFX(AsmEfiSetBreakSupport):
    push %ebp
    mov  %esp, %ebp
    mov  0x8(%ebp),%eax
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
    ret

#AsmEfiSetBreakSupport ENDP

