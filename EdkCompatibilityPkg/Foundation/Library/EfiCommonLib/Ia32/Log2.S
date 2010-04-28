#/*++
#
#Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
#This program and the accompanying materials                          
#are licensed and made available under the terms and conditions of the BSD License         
#which accompanies this distribution.  The full text of the license may be found at        
#http://opensource.org/licenses/bsd-license.php                                            
#                                                                                          
#THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#
#Module Name:
#
#  Log2.c
#
#Abstract:
#
#  64-bit integer logarithm function for IA-32
#
#--*/
#
#---------------------------------------------------------------------------
#include "EfiBind.h" //For ASM_PFX
#---------------------------------------------------------------------------

.globl ASM_PFX(Log2)

#UINT8
#Log2 (
#  IN UINT64   Operand
#  )
#/*++
#
#Routine Description:
# 
#  Calculates and floors logarithms based on 2
#
#Arguments:
#
#  Operand - value to calculate logarithm
#  
#Returns:
#
#  The largest integer that is less than or equal
#  to the logarithm of Operand based on 2 
#
#--*/
ASM_PFX(Log2):
  movl   $64, %ecx

  cmpl   $0, 4(%esp)             # (UINT32 *(&Operand))
  jne    _Log2_Wend
  cmpl   $0, 8(%esp)             # (UINT32 *(&Operand)) + 1
  jne    _Log2_Wend
  movb   $0xFF, %cl
  jmp    _Log2_Done

_Log2_Wend: 
  decl   %ecx
  cmpl   $32, %ecx
  jae    _Log2_Higher
  btl    %ecx, 4(%esp)  # (UINT32 *(&Operand))
  jmp    _Log2_Bit

_Log2_Higher: 
  movl   %ecx, %eax
  subl   $32, %eax
  btl    %eax, 8(%esp)  # (UINT32 *(&Operand)) + 1

_Log2_Bit: 
  jc     _Log2_Done
  jmp    _Log2_Wend

_Log2_Done: 
  movb   %cl, %al

  ret

