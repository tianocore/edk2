;------------------------------------------------------------------------------ ;
; Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   ExceptionHandlerAsm.Asm
;
; Abstract:
;
;   IA32 CPU Exception Handler
;
; Notes:
;
;------------------------------------------------------------------------------

;
; CommonExceptionHandler()
;
extern ASM_PFX(CommonExceptionHandler)

SECTION .data

extern ASM_PFX(mErrorCodeFlag)            ; Error code flags for exceptions
extern ASM_PFX(mDoFarReturnFlag)          ; Do far return flag

SECTION .text

ALIGN   8

;
; exception handler stub table
;
AsmIdtVectorBegin:
%rep  32
    db      0x6a        ; push  #VectorNum
    db      ($ - AsmIdtVectorBegin) / ((AsmIdtVectorEnd - AsmIdtVectorBegin) / 32) ; VectorNum
    push    eax
    mov     eax, ASM_PFX(CommonInterruptEntry)
    jmp     eax
%endrep
AsmIdtVectorEnd:

HookAfterStubBegin:
    db      0x6a        ; push
VectorNum:
    db      0          ; 0 will be fixed
    push    eax
    mov     eax, HookAfterStubHeaderEnd
    jmp     eax
HookAfterStubHeaderEnd:
    pop     eax
    sub     esp, 8     ; reserve room for filling exception data later
    push    dword [esp + 8]
    xchg    ecx, [esp] ; get vector number
    bt      [ASM_PFX(mErrorCodeFlag)], ecx
    jnc     .0
    push    dword [esp]      ; addition push if exception data needed
.0:
    xchg    ecx, [esp] ; restore ecx
    push    eax

;----------------------------------------------------------------------------;
; CommonInterruptEntry                                                               ;
;----------------------------------------------------------------------------;
; The follow algorithm is used for the common interrupt routine.
; Entry from each interrupt with a push eax and eax=interrupt number
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
global ASM_PFX(CommonInterruptEntry)
ASM_PFX(CommonInterruptEntry):
    cli
    pop    eax
    ;
    ; All interrupt handlers are invoked through interrupt gates, so
    ; IF flag automatically cleared at the entry point
    ;

    ;
    ; Get vector number from top of stack
    ;
    xchg    ecx, [esp]
    and     ecx, 0xFF       ; Vector number should be less than 256
    cmp     ecx, 32         ; Intel reserved vector for exceptions?
    jae     NoErrorCode
    bt      [ASM_PFX(mErrorCodeFlag)], ecx
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
    and     esp, 0xfffffff0
    sub     esp, 12

    sub     esp, 8
    push    0            ; clear EXCEPTION_HANDLER_CONTEXT.OldIdtHandler
    push    0            ; clear EXCEPTION_HANDLER_CONTEXT.ExceptionDataFlag

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    push    eax
    push    ecx
    push    edx
    push    ebx
    lea     ecx, [ebp + 6 * 4]
    push    ecx                          ; ESP
    push    dword [ebp]              ; EBP
    push    esi
    push    edi

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
    mov     eax, ss
    push    eax
    movzx   eax, word [ebp + 4 * 4]
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
    and     eax, 0xFFFF
    mov     [esp+4], eax

    sub     esp, 8
    sgdt    [esp]
    mov     eax, [esp + 2]
    xchg    eax, [esp]
    and     eax, 0xFFFF
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
    mov     eax, 1
    push    ebx         ; temporarily save value of ebx on stack
    cpuid               ; use CPUID to determine if FXSAVE/FXRESTOR and DE
                        ; are supported
    pop     ebx         ; retore value of ebx that was overwritten by CPUID
    mov     eax, cr4
    push    eax         ; push cr4 firstly
    test    edx, BIT24  ; Test for FXSAVE/FXRESTOR support
    jz      .1
    or      eax, BIT9   ; Set CR4.OSFXSR
.1:
    test    edx, BIT2   ; Test for Debugging Extensions support
    jz      .2
    or      eax, BIT3   ; Set CR4.DE
.2:
    mov     cr4, eax
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
    test    edx, BIT24  ; Test for FXSAVE/FXRESTOR support.
                        ; edx still contains result from CPUID above
    jz      .3
    db      0xf, 0xae, 0x7 ;fxsave [edi]
.3:

;; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

;; UINT32  ExceptionData;
    push    dword [ebp + 2 * 4]

;; Prepare parameter and call
    mov     edx, esp
    push    edx
    mov     edx, dword [ebp + 1 * 4]
    push    edx

    ;
    ; Call External Exception Handler
    ;
    mov     eax, ASM_PFX(CommonExceptionHandler)
    call    eax
    add     esp, 8

    cli
;; UINT32  ExceptionData;
    add     esp, 4

;; FX_SAVE_STATE_IA32 FxSaveState;
    mov     esi, esp
    mov     eax, 1
    cpuid               ; use CPUID to determine if FXSAVE/FXRESTOR
                        ; are supported
    test    edx, BIT24  ; Test for FXSAVE/FXRESTOR support
    jz      .4
    db      0xf, 0xae, 0xe ; fxrstor [esi]
.4:
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
    pop     dword [ebp + 5 * 4]

;; UINT32  Ldtr, Tr;
;; UINT32  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
    add     esp, 24

;; UINT32  Eip;
    pop     dword [ebp + 3 * 4]

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
;; NOTE - modified segment registers could hang the debugger...  We
;;        could attempt to insulate ourselves against this possibility,
;;        but that poses risks as well.
;;
    pop     gs
    pop     fs
    pop     es
    pop     ds
    pop     dword [ebp + 4 * 4]
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

    pop     dword [ebp - 8]
    pop     dword [ebp - 4]
    mov     esp, ebp
    pop     ebp
    add     esp, 8
    cmp     dword [esp - 16], 0   ; check EXCEPTION_HANDLER_CONTEXT.OldIdtHandler
    jz      DoReturn
    cmp     dword [esp - 20], 1   ; check EXCEPTION_HANDLER_CONTEXT.ExceptionDataFlag
    jz      ErrorCode
    jmp     dword [esp - 16]
ErrorCode:
    sub     esp, 4
    jmp     dword [esp - 12]

DoReturn:
    cmp     dword [ASM_PFX(mDoFarReturnFlag)], 0   ; Check if need to do far return instead of IRET
    jz      DoIret
    push    dword [esp + 8]    ; save EFLAGS
    add     esp, 16
    push    dword [esp - 8]    ; save CS in new location
    push    dword [esp - 8]    ; save EIP in new location
    push    dword [esp - 8]    ; save EFLAGS in new location
    popfd                ; restore EFLAGS
    retf                 ; far return

DoIret:
    iretd

;---------------------------------------;
; _AsmGetTemplateAddressMap                  ;
;----------------------------------------------------------------------------;
;
; Protocol prototype
;   AsmGetTemplateAddressMap (
;     EXCEPTION_HANDLER_TEMPLATE_MAP *AddressMap
;   );
;
; Routine Description:
;
;  Return address map of interrupt handler template so that C code can generate
;  interrupt table.
;
; Arguments:
;
;
; Returns:
;
;   Nothing
;
;
; Input:  [ebp][0]  = Original ebp
;         [ebp][4]  = Return address
;
; Output: Nothing
;
; Destroys: Nothing
;-----------------------------------------------------------------------------;
global ASM_PFX(AsmGetTemplateAddressMap)
ASM_PFX(AsmGetTemplateAddressMap):
    push    ebp                 ; C prolog
    mov     ebp, esp
    pushad

    mov ebx, dword [ebp + 0x8]
    mov dword [ebx],      AsmIdtVectorBegin
    mov dword [ebx + 0x4], (AsmIdtVectorEnd - AsmIdtVectorBegin) / 32
    mov dword [ebx + 0x8], HookAfterStubBegin

    popad
    pop     ebp
    ret

;-------------------------------------------------------------------------------------
;  AsmVectorNumFixup (*NewVectorAddr, VectorNum, *OldVectorAddr);
;-------------------------------------------------------------------------------------
global ASM_PFX(AsmVectorNumFixup)
ASM_PFX(AsmVectorNumFixup):
    mov     eax, dword [esp + 8]
    mov     ecx, [esp + 4]
    mov     [ecx + (VectorNum - HookAfterStubBegin)], al
    ret
