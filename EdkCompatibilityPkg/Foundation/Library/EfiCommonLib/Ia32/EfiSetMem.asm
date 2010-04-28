;/*++
;
;Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
;This program and the accompanying materials                          
;are licensed and made available under the terms and conditions of the BSD License         
;which accompanies this distribution.  The full text of the license may be found at        
;http://opensource.org/licenses/bsd-license.php                                            
;                                                                                          
;THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;
;Module Name:
;
;  EfiSetMem.asm
;
;Abstract:
;
;  This is the code that supports IA32-optimized SetMem service
;
;--*/
;---------------------------------------------------------------------------
    .686
    .model  flat,C
    .mmx
    .code

;---------------------------------------------------------------------------
;VOID
;EfiCommonLibSetMem (
;  IN VOID   *Buffer,
;  IN UINTN  Count,
;  IN UINT8  Value
;  )
;/*++
;
;Input:  VOID   *Buffer - Pointer to buffer to write
;        UINTN  Count   - Number of bytes to write
;        UINT8  Value   - Value to write
;
;Output: None.
;
;Saves:
;
;Modifies:
;
;Description:  This function is an optimized set-memory function.
;
;Notes:  This function tries to set memory 8 bytes at a time. As a result, 
;        it first picks up any misaligned bytes, then words, before getting 
;        in the main loop that does the 8-byte clears.
;
;--*/
EfiCommonLibSetMem PROC

    push   ebp
    mov    ebp, esp
    sub    esp, 10h; Reserve space for local variable UINT64 QWordValue @[ebp - 10H] & UINT64 MmxSave @[ebp - 18H]
    push   ebx
    push   edi

    mov edx, [ebp + 0Ch] ; Count
    test edx, edx
    je _SetMemDone

    push ebx
  
    mov eax, [ebp + 8]  ; Buffer
    mov bl, [ebp + 10h] ; Value
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

    mov WORD PTR [ebp - 10H],     bx ; QWordValue[0]
    mov WORD PTR [ebp - 10H + 2], bx ; QWordValue[2]
    mov WORD PTR [ebp - 10H + 4], bx ; QWordValue[4]
    mov WORD PTR [ebp - 10H + 6], bx ; QWordValue[6]
 
  
    movq  [ebp - 8], mm0 ; Save mm0 to MmxSave
    movq  mm0, [ebp - 10H] ; Load QWordValue to mm0

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
    movq  mm0, [ebp - 8] ; Restore MmxSave to mm0
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

    pop edi
    pop ebx
    leave
    ret

EfiCommonLibSetMem ENDP
	END
