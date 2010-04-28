; Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
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
;   DisablePaging64.Asm
;
; Abstract:
;
;   AsmDisablePaging64 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; InternalX86DisablePaging64 (
;   IN      UINT16                    Cs,
;   IN      UINT32                    EntryPoint,
;   IN      UINT32                    Context1,  OPTIONAL
;   IN      UINT32                    Context2,  OPTIONAL
;   IN      UINT32                    NewStack
;   );
;------------------------------------------------------------------------------
InternalX86DisablePaging64    PROC
    cli
    shl     rcx, 32                     ; rcx[32..47] <- Cs
    lea     eax, @F
    mov     esi, r8d
    or      rcx, rax                    ; rcx[0..47] <- Cs:@F
    mov     edi, r9d
    mov     eax, [rsp + 28h]            ; eax <- New Stack
    push    rcx
    retf                                ; switch to compatibility mode
@@:
    mov     esp, eax                    ; set up new stack
    mov     rax, cr0
    btr     eax, 31
    mov     cr0, rax                    ; disable paging
    mov     ecx, 0c0000080h
    rdmsr
    and     ah, NOT 1                   ; clear LME
    wrmsr
    mov     rax, cr4
    and     al, NOT (1 SHL 5)           ; clear PAE
    mov     cr4, rax
    push    rdi                         ; push Context2
    push    rsi                         ; push Context1
    call    rdx                         ; transfer control to EntryPoint
    hlt                                 ; no one should get here
InternalX86DisablePaging64    ENDP

    END
