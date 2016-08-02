;------------------------------------------------------------------------------
;
; Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Abstract:
;
;   Switch the stack from temporary memory to permanent memory.
;
;------------------------------------------------------------------------------

SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; SecSwitchStack (
;   UINT32   TemporaryMemoryBase,
;   UINT32   PermenentMemoryBase
;   );
;------------------------------------------------------------------------------
global ASM_PFX(SecSwitchStack)
ASM_PFX(SecSwitchStack):
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

    mov   ebx,  [esp + 20]          ; Save the first parameter
    mov   ecx,  [esp + 24]          ; Save the second parameter

    ;
    ; Save this function's return address into permanent memory at first.
    ; Then, Fixup the esp point to permanent memory
    ;
    mov   eax,  esp
    sub   eax,  ebx
    add   eax,  ecx
    mov   edx,  [esp]               ; copy pushed register's value to permanent memory
    mov   [eax], edx
    mov   edx,  [esp + 4]
    mov   [eax + 4], edx
    mov   edx,  [esp + 8]
    mov   [eax + 8], edx
    mov   edx,  [esp + 12]
    mov   [eax + 12], edx
    mov   edx,  [esp + 16]          ; Update return address into permanent memory
    mov   [eax + 16], edx
    mov   esp,  eax                 ; From now, esp is pointed to permanent memory

    ;
    ; Fixup the ebp point to permenent memory
    ;
    mov   eax,  ebp
    sub   eax,  ebx
    add   eax,  ecx
    mov   ebp,  eax                 ; From now, ebp is pointed to permanent memory

    pop   edx
    pop   ecx
    pop   ebx
    pop   eax
    ret
