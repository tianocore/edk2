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
#  Power10U64.c
#
#Abstract:
#
#  Calculates Operand * 10 ^ Power
#
#--*/
#
##include "Tiano.h"
#include "EfiBind.h"
#---------------------------------------------------------------------------
    .686: 
    #.MODEL flat,C
    .code: 

.globl ASM_PFX(DivU64x32)
#---------------------------------------------------------------------------
#
#UINT64
#MultU64x32 (
#  IN UINT64   Multiplicand,
#  IN UINTN    Multiplier
#  );
#
#UINT64
#Power10U64 (
#  IN UINT64   Operand,
#  IN UINTN    Power
#  )
#/*++
#
#Routine Description:
#
#  Raise 10 to the power of Power, and multiply the result with Operand
#
#Arguments:
#
#  Operand  - multiplicand
#  Power    - power
#
#Returns:
#
#  Operand * 10 ^ Power
#
#--*/
ASM_PFX(Power10U64):
  pushl  %ebp
  movl   %esp, %ebp
  movl   8(%ebp), %eax           # dword ptr Operand[0]
  movl   0xC(%ebp), %edx           # dword ptr Operand[4]
  movl   0x10(%ebp), %ecx           #Power
  jcxz   _Power10U64_Done

_Power10U64_Wend: 
  pushl  %ecx
  pushl  $10
  push   0xC(%ebp)
  push   0x8(%ebp)
  call   ASM_PFX(MultU64x32)
  addl   $0xc, %esp
  popl   %ecx
  movl   %eax, 8(%ebp)  # dword ptr Operand[0]
  movl   %edx, 0xC(%ebp)  # dword ptr Operand[4]
  loopl  _Power10U64_Wend

_Power10U64_Done: 

  popl   %ebp
  ret
#Power10U64 ENDP

