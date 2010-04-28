/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiSetMem.c

Abstract:

  This is the code that supports IA32-optimized SetMem service

--*/

#include "Tiano.h"

VOID
EfiCommonLibSetMem (
  IN VOID   *Buffer,
  IN UINTN  Count,
  IN UINT8  Value
  )
/*++

Input:  VOID   *Buffer - Pointer to buffer to write
        UINTN  Count   - Number of bytes to write
        UINT8  Value   - Value to write

Output: None.

Saves:

Modifies:

Description:  This function is an optimized set-memory function.

Notes:  This function tries to set memory 8 bytes at a time. As a result, 
        it first picks up any misaligned bytes, then words, before getting 
        in the main loop that does the 8-byte clears.

--*/
{
  UINT64 QWordValue;
  UINT64 MmxSave;
  __asm {
    mov edx, Count
    test edx, edx
    je _SetMemDone

    push ebx
  
    mov eax, Buffer
    mov bl, Value
    mov edi, eax
    mov bh, bl
  
    cmp edx, 256
    jb _SetRemindingByte
  
    and al, 07h
    test al, al
    je _SetBlock
  
    mov eax, edi
    shr eax, 3
    inc eax
    shl eax, 3
    sub eax, edi
    cmp eax, edx
    jnb _SetRemindingByte
  
    sub edx, eax
    mov ecx, eax

    mov al, bl
    rep stosb

_SetBlock:
    mov eax, edx
    shr eax, 6
    test eax, eax
    je _SetRemindingByte

    shl eax, 6
    sub edx, eax
    shr eax, 6

    mov WORD PTR QWordValue[0], bx
    mov WORD PTR QWordValue[2], bx
    mov WORD PTR QWordValue[4], bx
    mov WORD PTR QWordValue[6], bx
 
  
    movq  MmxSave, mm0
    movq  mm0, QWordValue

_B:
    movq  QWORD PTR ds:[edi], mm0
    movq  QWORD PTR ds:[edi+8], mm0
    movq  QWORD PTR ds:[edi+16], mm0
    movq  QWORD PTR ds:[edi+24], mm0
    movq  QWORD PTR ds:[edi+32], mm0
    movq  QWORD PTR ds:[edi+40], mm0
    movq  QWORD PTR ds:[edi+48], mm0
    movq  QWORD PTR ds:[edi+56], mm0
    add edi, 64
    dec eax
    jnz _B
  
; Restore mm0
    movq  mm0, MmxSave
    emms                                 ; Exit MMX Instruction
  
_SetRemindingByte:
    mov ecx, edx

    mov eax, ebx
    shl eax, 16
    mov ax, bx
    shr ecx, 2
    rep stosd
  
    mov ecx, edx
    and ecx, 3
    rep stosb
  
    pop ebx

_SetMemDone:

  }
}
