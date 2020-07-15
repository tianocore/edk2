;------------------------------------------------------------------------------
;
; Copyright (c) 2007 - 2012, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   Stack.asm
;
; Abstract:
;
;   Switch the stack from temporary memory to permanent memory.
;
;------------------------------------------------------------------------------

    .586p
    .model  flat,C
    .code

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; SecSwitchStack (
;   UINT32   TemporaryMemoryBase,
;   UINT32   PermenentMemoryBase
;   );
;------------------------------------------------------------------------------
SecSwitchStack   PROC
    ;
    ; Save three register: eax, ebx, ecx
    ;
    push  eax
    push  ebx
    push  ecx
    push  edx

    ;
    ; !!CAUTION!! this function address's is pushed into stack after
    ; migration of whole temporary memory, so need save it to permanent
    ; memory at first!
    ;

    mov   ebx, [esp + 20]          ; Save the first parameter
    mov   ecx, [esp + 24]          ; Save the second parameter

    ;
    ; Save this function's return address into permanent memory at first.
    ; Then, Fixup the esp point to permanent memory
    ;
    mov   eax, esp
    sub   eax, ebx
    add   eax, ecx
    mov   edx, dword ptr [esp]         ; copy pushed register's value to permanent memory
    mov   dword ptr [eax], edx
    mov   edx, dword ptr [esp + 4]
    mov   dword ptr [eax + 4], edx
    mov   edx, dword ptr [esp + 8]
    mov   dword ptr [eax + 8], edx
    mov   edx, dword ptr [esp + 12]
    mov   dword ptr [eax + 12], edx
    mov   edx, dword ptr [esp + 16]    ; Update this function's return address into permanent memory
    mov   dword ptr [eax + 16], edx
    mov   esp, eax                     ; From now, esp is pointed to permanent memory

    ;
    ; Fixup the ebp point to permanent memory
    ;
    mov   eax, ebp
    sub   eax, ebx
    add   eax, ecx
    mov   ebp, eax                ; From now, ebp is pointed to permanent memory

    ;
    ; Fixup callee's ebp point for PeiDispatch
    ;
    mov   eax, dword ptr [ebp]
    sub   eax, ebx
    add   eax, ecx
    mov   dword ptr [ebp], eax    ; From now, Temporary's PPI caller's stack is in permanent memory

    pop   edx
    pop   ecx
    pop   ebx
    pop   eax
    ret
SecSwitchStack   ENDP

    END
