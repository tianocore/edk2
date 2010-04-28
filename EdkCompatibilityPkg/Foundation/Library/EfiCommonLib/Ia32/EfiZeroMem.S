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
#  EfiZeroMem.c
#
#Abstract:
#
#  This is the code that supports IA32-optimized ZeroMem service
#
#--*/
#include "EfiBind.h"
#---------------------------------------------------------------------------
    .686: 
    #.MODEL flat,C
    .mmx: 
    .code: 

#---------------------------------------------------------------------------
.globl ASM_PFX(EfiCommonLibZeroMem)
#VOID
#EfiCommonLibZeroMem (
#  IN VOID   *Buffer,
#  IN UINTN  Count
#  )
#/*++
#
#Input:  VOID   *Buffer - Pointer to buffer to clear
#        UINTN  Count  - Number of bytes to clear
#
#Output: None.
#
#Saves:
#
#Modifies:
#
#Description:  This function is an optimized zero-memory function.
#
#Notes:  This function tries to zero memory 8 bytes at a time. As a result, 
#        it first picks up any misaligned bytes, then words, before getting 
#        in the main loop that does the 8-byte clears.
#
#--*/
ASM_PFX(EfiCommonLibZeroMem):
#  UINT64 MmxSave;
    pushl %ebp
    movl  %esp, %ebp
    pushl %ecx # Reserve space for local variable MmxSave
    pushl %ecx
    pushl %edi

    movl  0xC(%ebp), %ecx  # Count
    movl  8(%ebp), %edi # Buffer

    # Pick up misaligned start bytes (get pointer 4-byte aligned)
_StartByteZero: 
    movl  %edi, %eax
    andb  $3, %al                     # check lower 2 bits of address
    testb %al, %al
    je    _ZeroBlocks                 # already aligned?
    cmpl  $0, %ecx
    je    _ZeroMemDone

    # Clear the byte memory location
    movb  $0, (%edi)
    incl   %edi

    # Decrement our count
    decl   %ecx
    jmp   _StartByteZero        # back to top of loop

_ZeroBlocks: 

    # Compute how many 64-byte blocks we can clear 
    movl  %ecx, %edx
    shrl  $6, %ecx                    # convert to 64-byte count
    shll  $6, %ecx                    # convert back to bytes
    subl  %ecx, %edx                  # subtract from the original count
    shrl  $6, %ecx                    # and this is how many 64-byte blocks

    # If no 64-byte blocks, then skip 
    cmpl   $0, %ecx
    je    _ZeroRemaining

    # Save mm0
    movq  %mm0, -8(%ebp)  # Save mm0 to MmxSave

    pxor  %mm0, %mm0        # Clear mm0

_B: 
    movq  %mm0, %ds:(%edi)
    movq  %mm0, %ds:8(%edi)
    movq  %mm0, %ds:16(%edi)
    movq  %mm0, %ds:24(%edi)
    movq  %mm0, %ds:32(%edi)
    movq  %mm0, %ds:40(%edi)
    movq  %mm0, %ds:48(%edi)
    movq  %mm0, %ds:56(%edi)

    addl   $64, %edi
    decl   %ecx
    jnz    _B

# Restore mm0
    movq  -8(%ebp), %mm0 # Restore mm0 from MmxSave
    emms                                 # Exit MMX Instruction

_ZeroRemaining: 
    # Zero out as many DWORDS as possible
    movl  %edx, %ecx
    shrl  $2, %ecx
    xorl  %eax, %eax

    rep
    stosl

    # Zero out remaining as bytes
    movl  %edx, %ecx
    andl  $03, %ecx

    rep
    stosb

_ZeroMemDone: 

    popl   %edi
    leave
    ret
#EfiCommonLibZeroMem ENDP

