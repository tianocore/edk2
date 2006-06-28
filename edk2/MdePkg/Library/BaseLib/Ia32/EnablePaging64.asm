;------------------------------------------------------------------------------
;
; Copyright (c) 2006, Intel Corporation
; All rights reserved. This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   EnablePaging64.Asm
;
; Abstract:
;
;   AsmEnablePaging64 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .686p
    .model  flat,C
    .code

InternalX86EnablePaging64 PROC
    cli

    mov     ebx, [esp + 4]              ; save CS
    mov     eax, OFFSET cs_address
    mov     word ptr [eax], bx          ; Update CS selector for far jump

    mov     eax, cr4 
    or      al, (1 SHL 5)
    mov     cr4, eax                    ; enable PAE
    mov     ecx, 0c0000080h
    rdmsr
    or      ah, 1                       ; set LME
    wrmsr
    mov     eax, cr0
    bts     eax, 31
    mov     cr0, eax                    ; enable paging
    retf
    jmp     go_to_long_mode
go_to_long_mode:

    ;
    ; This is the next instruction after enabling paging.  Jump to long mode
    ;
    db      067h
    db      0eah                        ;   Far Jump Offset:Selector to reload CS
    dd      OFFSET in_long_mode         ;   Offset is ensuing instruction boundary
cs_address:
    dw      0h                          ;   CS selector will be updated at runtime 

in_long_mode:                           ; now in long mode
    DB      67h, 48h
    mov     ebx, [esp + 8]              ; mov rbx, [esp]
    DB      67h, 48h                   
    mov     ecx, [esp + 10h]            ; mov rcx, [esp + 8]
    DB      67h, 48h
    mov     edx, [esp + 18h]            ; mov rdx, [esp + 10h]
    DB      67h, 48h
    mov     esp, [esp + 20h]            ; mov rsp, [esp + 18h]
    DB      48h
    call    ebx                         ; call rbx
    jmp     $
InternalX86EnablePaging64 ENDP

    END
