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
#   MultU64x32.c
#
#Abstract:
#
#  64-bit Multiplication function for IA-32
#
#--*/
#include "EfiBind.h"
#---------------------------------------------------------------------------
    .686: 
    #.MODEL flat,C
    .code: 

#---------------------------------------------------------------------------
.globl ASM_PFX(MultU64x32)
#UINT64
#MultU64x32 (
#  IN UINT64   Multiplicand,
#  IN UINTN    Multiplier
#  )
#/*++
#
#Routine Description:
#
#  This routine allows a 64 bit value to be multiplied with a 32 bit 
#  value returns 64bit result.
#  No checking if the result is greater than 64bits
#
#Arguments:
#
#  Multiplicand  - multiplicand
#  Multiplier    - multiplier
#
#Returns:
#
#  Multiplicand * Multiplier
#
#--*/
ASM_PFX(MultU64x32):

    movl   4(%esp), %eax # dword ptr Multiplicand[0]
    mull   0xC(%esp)             # Multiplier
    pushl  %eax
    pushl  %edx
    movl   0x10(%esp), %eax # dword ptr Multiplicand[4]
    mull   0x14(%esp)           # Multiplier
    #
    # The value in edx stored by second multiplication overflows
    # the output and should be discarded. So here we overwrite it
    # with the edx value of first multiplication.
    #
    popl   %edx
    addl   %eax, %edx
    popl   %eax

    ret
#MultU64x32 ENDP

