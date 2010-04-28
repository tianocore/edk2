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
;  EfiZeroMem.c
;
;Abstract:
;
;  This is the code that supports IA32-optimized ZeroMem service
;
;--*/
;---------------------------------------------------------------------------
    .686
    .model  flat,C
    .mmx
    .code

;---------------------------------------------------------------------------
;VOID
;EfiCommonLibZeroMem (
;  IN VOID   *Buffer,
;  IN UINTN  Count
;  )
;/*++
;
;Input:  VOID   *Buffer - Pointer to buffer to clear
;        UINTN  Count  - Number of bytes to clear
;
;Output: None.
;
;Saves:
;
;Modifies:
;
;Description:  This function is an optimized zero-memory function.
;
;Notes:  This function tries to zero memory 8 bytes at a time. As a result, 
;        it first picks up any misaligned bytes, then words, before getting 
;        in the main loop that does the 8-byte clears.
;
;--*/
EfiCommonLibZeroMem PROC
;  UINT64 MmxSave;
    push  ebp
    mov   ebp, esp
    push  ecx  ; Reserve space for local variable MmxSave
    push  ecx
    push  edi
    
    mov   ecx, [ebp + 0Ch] ; Count
    mov   edi, [ebp + 8]; Buffer

    ; Pick up misaligned start bytes (get pointer 4-byte aligned)
_StartByteZero:
    mov   eax, edi    
    and   al, 3                       ; check lower 2 bits of address
    test  al, al
    je    _ZeroBlocks                 ; already aligned?
    cmp   ecx, 0
    je    _ZeroMemDone

    ; Clear the byte memory location
    mov   BYTE PTR [edi], 0           
    inc    edi

    ; Decrement our count
    dec    ecx
    jmp   _StartByteZero        ; back to top of loop

_ZeroBlocks:

    ; Compute how many 64-byte blocks we can clear 
    mov   edx, ecx
    shr   ecx, 6                      ; convert to 64-byte count
    shl   ecx, 6                      ; convert back to bytes
    sub   edx, ecx                    ; subtract from the original count
    shr   ecx, 6                      ; and this is how many 64-byte blocks

    ; If no 64-byte blocks, then skip 
    cmp    ecx, 0
    je    _ZeroRemaining

    ; Save mm0
    movq  [ebp - 8], mm0  ; Save mm0 to MmxSave

    pxor  mm0, mm0          ; Clear mm0

_B:
    movq  QWORD PTR ds:[edi], mm0
    movq  QWORD PTR ds:[edi+8], mm0
    movq  QWORD PTR ds:[edi+16], mm0
    movq  QWORD PTR ds:[edi+24], mm0
    movq  QWORD PTR ds:[edi+32], mm0
    movq  QWORD PTR ds:[edi+40], mm0
    movq  QWORD PTR ds:[edi+48], mm0
    movq  QWORD PTR ds:[edi+56], mm0
   
    add    edi, 64
    dec    ecx
    jnz    _B
  
; Restore mm0
    movq  mm0, [ebp - 8] ; Restore mm0 from MmxSave
    emms                                 ; Exit MMX Instruction

_ZeroRemaining:
    ; Zero out as many DWORDS as possible
    mov   ecx, edx
    shr   ecx, 2
    xor   eax, eax

    rep stosd

    ; Zero out remaining as bytes
    mov   ecx, edx
    and   ecx, 03

    rep   stosb
 
_ZeroMemDone:

    pop    edi
    leave
    ret
EfiCommonLibZeroMem ENDP	
	END
