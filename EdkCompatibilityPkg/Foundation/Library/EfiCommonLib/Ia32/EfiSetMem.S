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
#  EfiSetMem.asm
#
#Abstract:
#
#  This is the code that supports IA32-optimized SetMem service
#
#--*/
#include "EfiBind.h"
#---------------------------------------------------------------------------
    .686: 
    #.MODEL flat,C
    .mmx: 
    .code: 

#---------------------------------------------------------------------------
.globl ASM_PFX(EfiCommonLibSetMem)

#VOID
#EfiCommonLibSetMem (
#  IN VOID   *Buffer,
#  IN UINTN  Count,
#  IN UINT8  Value
#  )
#/*++
#
#Input:  VOID   *Buffer - Pointer to buffer to write
#        UINTN  Count   - Number of bytes to write
#        UINT8  Value   - Value to write
#
#Output: None.
#
#Saves:
#
#Modifies:
#
#Description:  This function is an optimized set-memory function.
#
#Notes:  This function tries to set memory 8 bytes at a time. As a result, 
#        it first picks up any misaligned bytes, then words, before getting 
#        in the main loop that does the 8-byte clears.
#
#--*/
ASM_PFX(EfiCommonLibSetMem):

    pushl  %ebp
    movl   %esp, %ebp
    subl   $0x10, %esp # Reserve space for local variable UINT64 QWordValue @[ebp - 10H] & UINT64 MmxSave @[ebp - 18H]
    pushl  %ebx
    pushl  %edi

    movl 0xC(%ebp), %edx # Count
    testl %edx, %edx
    je _SetMemDone

    pushl %ebx

    movl 8(%ebp), %eax  # Buffer
    movb 0x10(%ebp), %bl # Value
    movl %eax, %edi
    movb %bl, %bh

    cmpl $256, %edx
    jb _SetRemindingByte

    andb $0x7, %al
    testb %al, %al
    je _SetBlock

    movl %edi, %eax
    shrl $3, %eax
    incl %eax
    shll $3, %eax
    subl %edi, %eax
    cmpl %edx, %eax
    jnb _SetRemindingByte

    subl %eax, %edx
    movl %eax, %ecx

    movb %bl, %al
    rep
    stosb

_SetBlock: 
    movl %edx, %eax
    shrl $6, %eax
    testl %eax, %eax
    je _SetRemindingByte

    shll $6, %eax
    subl %eax, %edx
    shrl $6, %eax

    movw %bx, -0x10(%ebp)            # QWordValue[0]
    movw %bx, -0x10+2(%ebp)          # QWordValue[2]
    movw %bx, -0x10+4(%ebp)          # QWordValue[4]
    movw %bx, -0x10+6(%ebp)          # QWordValue[6]


    movq  %mm0, -8(%ebp) # Save mm0 to MmxSave
    movq  -0x10(%ebp), %mm0 # Load QWordValue to mm0

_B: 
    movq  %mm0, %ds:(%edi)
    movq  %mm0, %ds:8(%edi)
    movq  %mm0, %ds:16(%edi)
    movq  %mm0, %ds:24(%edi)
    movq  %mm0, %ds:32(%edi)
    movq  %mm0, %ds:40(%edi)
    movq  %mm0, %ds:48(%edi)
    movq  %mm0, %ds:56(%edi)
    addl $64, %edi
    decl %eax
    jnz _B

# Restore mm0
    movq  -8(%ebp), %mm0 # Restore MmxSave to mm0
    emms                                 # Exit MMX Instruction

_SetRemindingByte: 
    movl %edx, %ecx

    movl %ebx, %eax
    shll $16, %eax
    movw %bx, %ax
    shrl $2, %ecx
    rep
    stosl

    movl %edx, %ecx
    andl $3, %ecx
    rep
    stosb

    popl %ebx

_SetMemDone: 

    popl %edi
    popl %ebx
    leave
    ret

#EfiCommonLibSetMem ENDP

