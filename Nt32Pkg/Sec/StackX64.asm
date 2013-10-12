;------------------------------------------------------------------------------
;
; Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   Stack.asm
;
; Abstract:
;
;   Switch the stack from temporary memory to permenent memory.
;
;------------------------------------------------------------------------------

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
    mov [rsp + 08h], rcx
    mov [rsp + 10h], rdx

    ;
    ; Save three register: eax, ebx, ecx
    ;
    push  rax
    push  rbx
    push  rcx
    push  rdx
    
    ;
    ; !!CAUTION!! this function address's is pushed into stack after
    ; migration of whole temporary memory, so need save it to permenent
    ; memory at first!
    ;
    
    mov   rbx, [rsp + 28h]          ; Save the first parameter
    mov   rcx, [rsp + 30h]          ; Save the second parameter
    
    ;
    ; Save this function's return address into permenent memory at first.
    ; Then, Fixup the esp point to permenent memory
    ;
    mov   rax, rsp
    sub   rax, rbx
    add   rax, rcx
    mov   rdx, qword ptr [rsp]         ; copy pushed register's value to permenent memory
    mov   qword ptr [rax], rdx    
    mov   rdx, qword ptr [rsp + 8]
    mov   qword ptr [rax + 8], rdx    
    mov   rdx, qword ptr [rsp + 10h]
    mov   qword ptr [rax + 10h], rdx    
    mov   rdx, qword ptr [rsp + 18h]
    mov   qword ptr [rax + 18h], rdx    
    mov   rdx, qword ptr [rsp + 20h]    ; Update this function's return address into permenent memory
    mov   qword ptr [rax + 20h], rdx    
    mov   rsp, rax                     ; From now, esp is pointed to permenent memory
        
    ;
    ; Fixup the ebp point to permenent memory
    ;
    mov   rax, rbp
    sub   rax, rbx
    add   rax, rcx
    mov   rbp, rax                ; From now, ebp is pointed to permenent memory
    
    pop   rdx
    pop   rcx
    pop   rbx
    pop   rax
    ret
SecSwitchStack   ENDP

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; PeiSwitchStacks (
;   IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
;   IN      VOID                      *Context1,  OPTIONAL
;   IN      VOID                      *Context2,  OPTIONAL
;   IN      VOID                      *Context3,  OPTIONAL
;   IN      VOID                      *NewStack
;   )
;------------------------------------------------------------------------------
PeiSwitchStacks   PROC
    mov  rax, rcx
    mov  rcx, rdx
    mov  rdx, r8
    mov  r8, r9
    mov  rsp, [rsp + 28h]
    sub  rsp, 20h
    call rax
    jmp $
    ret
PeiSwitchStacks   ENDP

    END
