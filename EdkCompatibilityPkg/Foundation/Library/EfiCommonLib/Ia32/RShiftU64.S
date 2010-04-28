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
#  RShiftU64.c
#
#Abstract:
#
#  64-bit right shift function for IA-32
#
#--*/
#
##include "Tiano.h"
#
#include "EfiBind.h"
#---------------------------------------------------------------------------
    .686: 
    #.MODEL flat,C
    .code: 

#---------------------------------------------------------------------------
.globl ASM_PFX(RShiftU64)
#UINT64
#RShiftU64 (
#  IN UINT64   Operand,
#  IN UINTN    Count
#  )
#/*++
#
#Routine Description:
#  This routine allows a 64 bit value to be right shifted by 32 bits and returns the 
#  shifted value.
#  Count is valid up 63. (Only Bits 0-5 is valid for Count)
#Arguments:
#  Operand - Value to be shifted
#  Count   - Number of times to shift right.
# 
#Returns:
#
#  Value shifted right identified by the Count.
#
#--*/
ASM_PFX(RShiftU64):

    movl   4(%esp), %eax # dword ptr Operand[0]
    movl   8(%esp), %edx # dword ptr Operand[4]

    #
    # CL is valid from 0 - 31. shld will move EDX:EAX by CL times but EDX is not touched
    # For CL of 32 - 63, it will be shifted 0 - 31 so we will move edx to eax later. 
    #
    movl   0xC(%esp), %ecx  # Count
    andl   $63, %ecx
    shrd   %cl, %edx, %eax
    shrb   %cl, %edx

    cmpl   $32, %ecx
    jc     _RShiftU64_Done

    #
    # Since Count is 32 - 63, edx will have been shifted  by 0 - 31                                                     
    # If shifted by 32 or more, set upper 32 bits to zero.
    #
    movl   %edx, %eax
    xorl   %edx, %edx

_RShiftU64_Done: 

    ret
#RShiftU64 ENDP

