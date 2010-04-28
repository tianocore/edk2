#---------------------------------------------------------------------------
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
#  DivU64x32.c
#
#Abstract:
#
#  64-bit division function for IA-32
#
#--*/

#---------------------------------------------------------------------------
#include "EfiBind.h" //For ASM_PFX
#---------------------------------------------------------------------------

#---------------------------------------------------------------------------
    .386: 
    .code: 

#---------------------------------------------------------------------------

.globl ASM_PFX(DivU64x32)

#UINT64
#DivU64x32 (
#  IN UINT64   Dividend,
#  IN UINTN    Divisor,
#  OUT UINTN   *Remainder OPTIONAL
#  )
#/*++

#Routine Description:

#  This routine allows a 64 bit value to be divided with a 32 bit value returns 
#  64bit result and the Remainder.
#
#Arguments:

#  Dividend  - dividend
#  Divisor   - divisor
#  Remainder - buffer for remainder
#  
#Returns:

#  Dividend  / Divisor
#  Remainder = Dividend mod Divisor
#  
#N.B. only works for 31bit divisors!!
#
#--*/
#---------------------------------------------------------------------------

ASM_PFX(DivU64x32):
    pushl %ebp
    movl %esp, %ebp
    xorl %edx, %edx                 # Clear EDX

    movl 0xC(%ebp), %eax            # Put high 32 bits of 64-bit dividend in EAX
    movl 0x10(%ebp), %ecx           # Put 32 bits divisor in ECX
    divl %ecx                       # Dividend   Divisor  Quoitent...Remainder
                                    #  0:EAX  /  ECX   =  EAX      EDX   

    pushl %eax                      # Push quoitent in stack

    movl 8(%ebp), %eax              # Put low 32 bits of 64-bit dividend in EAX              
    divl %ecx                       # Leave the REMAINDER in EDX as High 32-bit of new dividend
                                    # Dividend   Divisor  Quoitent...Remainder              
                                    #  EDX:EAX  /  ECX   =  EAX      EDX               

    movl 0x14(%ebp), %ecx            # Put &REMAINDER to ecx

    jecxz Label1                    # If ecx == 0, no remainder exist, return with quoitent in EDX directly 
    movl %edx, (%ecx)               # Put EDX through REMAINDER pointer in ECX 

Label1: 
    popl %edx                       # Pop High 32-bit QUOITENT to EDX
    popl %ebp

    ret

