      TITLE   CpuAsm.asm:
;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials
;*   are licensed and made available under the terms and conditions of the BSD License
;*   which accompanies this distribution.  The full text of the license may be found at
;*   http://opensource.org/licenses/bsd-license.php
;*
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;*
;*    CpuAsm.asm
;*
;*   Abstract:
;*
;------------------------------------------------------------------------------

    .686
    .model  flat,C
    .code

EXTRN mErrorCodeFlag:DWORD ; Error code flags for exceptions

;
; point to the external interrupt vector table
;
ExternalVectorTablePtr DWORD 0

InitializeExternalVectorTablePtr PROC PUBLIC
    mov     eax, [esp+4]
    mov     ExternalVectorTablePtr, eax
    ret
InitializeExternalVectorTablePtr ENDP

;------------------------------------------------------------------------------
; VOID
; SetCodeSelector (
;   UINT16 Selector
;   );
;------------------------------------------------------------------------------
SetCodeSelector PROC PUBLIC
    mov     ecx, [esp+4]
    sub     esp, 0x10
    lea     eax, setCodeSelectorLongJump
    mov     [esp], eax
    mov     [esp+4], cx
    jmp     fword ptr [esp]
setCodeSelectorLongJump:
    add     esp, 0x10
    ret
SetCodeSelector ENDP

;------------------------------------------------------------------------------
; VOID
; SetDataSelectors (
;   UINT16 Selector
;   );
;------------------------------------------------------------------------------
SetDataSelectors PROC PUBLIC
    mov     ecx, [esp+4]
    mov     ss, cx
    mov     ds, cx
    mov     es, cx
    mov     fs, cx
    mov     gs, cx
    ret
SetDataSelectors ENDP

;---------------------------------------;
; CommonInterruptEntry                  ;
;---------------------------------------;
; The follow algorithm is used for the common interrupt routine.

CommonInterruptEntry PROC PUBLIC
    cli
    ;
    ; All interrupt handlers are invoked through interrupt gates, so
    ; IF flag automatically cleared at the entry point
    ;

    ;
    ; Calculate vector number
    ;
    ; Get the return address of call, actually, it is the
    ; address of vector number.
    ;
    xchg    ecx, [esp]
    mov     cx, [ecx]
    and     ecx, 0FFFFh
    cmp     ecx, 32         ; Intel reserved vector for exceptions?
    jae     NoErrorCode
    bt      mErrorCodeFlag, ecx
    jc      HasErrorCode

NoErrorCode:

    ;
    ; Stack:
    ; +---------------------+
    ; +    EFlags           +
    ; +---------------------+
    ; +    CS               +
    ; +---------------------+
    ; +    EIP              +
    ; +---------------------+
    ; +    ECX              +
    ; +---------------------+ <-- ESP
    ;
    ; Registers:
    ;   ECX - Vector Number
    ;

    ;
    ; Put Vector Number on stack
    ;
    push    ecx

    ;
    ; Put 0 (dummy) error code on stack, and restore ECX
    ;
    xor     ecx, ecx  ; ECX = 0
    xchg    ecx, [esp+4]

    jmp     ErrorCodeAndVectorOnStack

HasErrorCode:

    ;
    ; Stack:
    ; +---------------------+
    ; +    EFlags           +
    ; +---------------------+
    ; +    CS               +
    ; +---------------------+
    ; +    EIP              +
    ; +---------------------+
    ; +    Error Code       +
    ; +---------------------+
    ; +    ECX              +
    ; +---------------------+ <-- ESP
    ;
    ; Registers:
    ;   ECX - Vector Number
    ;

    ;
    ; Put Vector Number on stack and restore ECX
    ;
    xchg    ecx, [esp]

ErrorCodeAndVectorOnStack:
    push    ebp
    mov     ebp, esp

    ;
    ; Stack:
    ; +---------------------+
    ; +    EFlags           +
    ; +---------------------+
    ; +    CS               +
    ; +---------------------+
    ; +    EIP              +
    ; +---------------------+
    ; +    Error Code       +
    ; +---------------------+
    ; +    Vector Number    +
    ; +---------------------+
    ; +    EBP              +
    ; +---------------------+ <-- EBP
    ;

    ;
    ; Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
    ; is 16-byte aligned
    ;
    and     esp, 0fffffff0h
    sub     esp, 12

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    push    eax
    push    ecx
    push    edx
    push    ebx
    lea     ecx, [ebp + 6 * 4]
    push    ecx                          ; ESP
    push    dword ptr [ebp]              ; EBP
    push    esi
    push    edi

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
    mov     eax, ss
    push    eax
    movzx   eax, word ptr [ebp + 4 * 4]
    push    eax
    mov     eax, ds
    push    eax
    mov     eax, es
    push    eax
    mov     eax, fs
    push    eax
    mov     eax, gs
    push    eax

;; UINT32  Eip;
    mov     eax, [ebp + 3 * 4]
    push    eax

;; UINT32  Gdtr[2], Idtr[2];
    sub     esp, 8
    sidt    [esp]
    mov     eax, [esp + 2]
    xchg    eax, [esp]
    and     eax, 0FFFFh
    mov     [esp+4], eax

    sub     esp, 8
    sgdt    [esp]
    mov     eax, [esp + 2]
    xchg    eax, [esp]
    and     eax, 0FFFFh
    mov     [esp+4], eax

;; UINT32  Ldtr, Tr;
    xor     eax, eax
    str     ax
    push    eax
    sldt    ax
    push    eax

;; UINT32  EFlags;
    mov     eax, [ebp + 5 * 4]
    push    eax

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    mov     eax, cr4
    or      eax, 208h
    mov     cr4, eax
    push    eax
    mov     eax, cr3
    push    eax
    mov     eax, cr2
    push    eax
    xor     eax, eax
    push    eax
    mov     eax, cr0
    push    eax

;; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
    mov     eax, dr7
    push    eax
    mov     eax, dr6
    push    eax
    mov     eax, dr3
    push    eax
    mov     eax, dr2
    push    eax
    mov     eax, dr1
    push    eax
    mov     eax, dr0
    push    eax

;; FX_SAVE_STATE_IA32 FxSaveState;
    sub     esp, 512
    mov     edi, esp
    db      0fh, 0aeh, 07h ;fxsave [edi]

;; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

;; UINT32  ExceptionData;
    push    dword ptr [ebp + 2 * 4]

;; call into exception handler
    mov     eax, ExternalVectorTablePtr  ; get the interrupt vectors base
    or      eax, eax                        ; NULL?
    jz      nullExternalExceptionHandler

    mov     ecx, [ebp + 4]
    mov     eax, [eax + ecx * 4]
    or      eax, eax                        ; NULL?
    jz      nullExternalExceptionHandler

;; Prepare parameter and call
    mov     edx, esp
    push    edx
    mov     edx, dword ptr [ebp + 1 * 4]
    push    edx

    ;
    ; Call External Exception Handler
    ;
    call    eax
    add     esp, 8

nullExternalExceptionHandler:

    cli
;; UINT32  ExceptionData;
    add     esp, 4

;; FX_SAVE_STATE_IA32 FxSaveState;
    mov     esi, esp
    db      0fh, 0aeh, 0eh ; fxrstor [esi]
    add     esp, 512

;; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
;; Skip restoration of DRx registers to support in-circuit emualators
;; or debuggers set breakpoint in interrupt/exception context
    add     esp, 4 * 6

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    pop     eax
    mov     cr0, eax
    add     esp, 4    ; not for Cr1
    pop     eax
    mov     cr2, eax
    pop     eax
    mov     cr3, eax
    pop     eax
    mov     cr4, eax

;; UINT32  EFlags;
    pop     dword ptr [ebp + 5 * 4]

;; UINT32  Ldtr, Tr;
;; UINT32  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
    add     esp, 24

;; UINT32  Eip;
    pop     dword ptr [ebp + 3 * 4]

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
;; NOTE - modified segment registers could hang the debugger...  We
;;        could attempt to insulate ourselves against this possibility,
;;        but that poses risks as well.
;;
    pop     gs
    pop     fs
    pop     es
    pop     ds
    pop     dword ptr [ebp + 4 * 4]
    pop     ss

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    pop     edi
    pop     esi
    add     esp, 4   ; not for ebp
    add     esp, 4   ; not for esp
    pop     ebx
    pop     edx
    pop     ecx
    pop     eax

    mov     esp, ebp
    pop     ebp
    add     esp, 8
    iretd

CommonInterruptEntry ENDP

END
