;------------------------------------------------------------------------------ ;
; Copyright (c) 2017 - 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   ExceptionTssEntryAsm.Asm
;
; Abstract:
;
;   IA32 CPU Exception Handler with Separate Stack
;
; Notes:
;
;------------------------------------------------------------------------------

;
; IA32 TSS Memory Layout Description
;
struc IA32_TSS
                    resw 1
                    resw 1
  .ESP0:    resd 1
  .SS0:     resw 1
                    resw 1
  .ESP1:    resd 1
  .SS1:     resw 1
                    resw 1
  .ESP2:    resd 1
  .SS2:     resw 1
                    resw 1
  ._CR3:    resd 1
  .EIP:     resd 1
  .EFLAGS:  resd 1
  ._EAX:    resd 1
  ._ECX:    resd 1
  ._EDX:    resd 1
  ._EBX:    resd 1
  ._ESP:    resd 1
  ._EBP:    resd 1
  ._ESI:    resd 1
  ._EDI:    resd 1
  ._ES:     resw 1
                    resw 1
  ._CS:     resw 1
                    resw 1
  ._SS:     resw 1
                    resw 1
  ._DS:     resw 1
                    resw 1
  ._FS:     resw 1
                    resw 1
  ._GS:     resw 1
                    resw 1
  .LDT:     resw 1
                    resw 1
                    resw 1
                    resw 1
endstruc

;
; CommonExceptionHandler()
;
extern ASM_PFX(CommonExceptionHandler)

SECTION .data

SECTION .text

ALIGN   8

;
; Exception handler stub table
;
AsmExceptionEntryBegin:
%assign Vector 0
%rep  32

DoIret%[Vector]:
    iretd
ASM_PFX(ExceptionTaskSwtichEntry%[Vector]):
    push    byte %[Vector]
    mov     eax, ASM_PFX(CommonTaskSwtichEntryPoint)
    call    eax
    mov     esp, eax    ; Restore stack top
    jmp     DoIret%[Vector]

%assign Vector Vector+1
%endrep
AsmExceptionEntryEnd:

;
; Common part of exception handler
;
global ASM_PFX(CommonTaskSwtichEntryPoint)
ASM_PFX(CommonTaskSwtichEntryPoint):
    ;
    ; Stack:
    ; +---------------------+ <-- EBP - 8
    ; +       TSS Base      +
    ; +---------------------+ <-- EBP - 4
    ; +      CPUID.EDX      +
    ; +---------------------+ <-- EBP
    ; +         EIP         +
    ; +---------------------+ <-- EBP + 4
    ; +    Vector Number    +
    ; +---------------------+ <-- EBP + 8
    ; +    Error Code       +
    ; +---------------------+
    ;

    mov     ebp, esp                    ; Stack frame

; Use CPUID to determine if FXSAVE/FXRESTOR and DE are supported
    mov     eax, 1
    cpuid
    push    edx

; Get TSS base of interrupted task through PreviousTaskLink field in
; current TSS base
    sub     esp, 8
    sgdt    [esp + 2]
    mov     eax, [esp + 4]              ; GDT base
    add     esp, 8

    xor     ebx, ebx
    str     bx                          ; Current TR

    mov     ecx, [eax + ebx + 2]
    shl     ecx, 8
    mov     cl, [eax + ebx + 7]
    ror     ecx, 8                      ; ecx = Current TSS base
    push    ecx                         ; keep it in stack for later use

    movzx   ebx, word [ecx]             ; Previous Task Link
    mov     ecx, [eax + ebx + 2]
    shl     ecx, 8
    mov     cl, [eax + ebx + 7]
    ror     ecx, 8                      ; ecx = Previous TSS base

;
; Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
; is 16-byte aligned
;
    and     esp, 0xfffffff0
    sub     esp, 12

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    push    dword [ecx + IA32_TSS._EAX]
    push    dword [ecx + IA32_TSS._ECX]
    push    dword [ecx + IA32_TSS._EDX]
    push    dword [ecx + IA32_TSS._EBX]
    push    dword [ecx + IA32_TSS._ESP]
    push    dword [ecx + IA32_TSS._EBP]
    push    dword [ecx + IA32_TSS._ESI]
    push    dword [ecx + IA32_TSS._EDI]

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
    movzx   eax, word [ecx + IA32_TSS._SS]
    push    eax
    movzx   eax, word [ecx + IA32_TSS._CS]
    push    eax
    movzx   eax, word [ecx + IA32_TSS._DS]
    push    eax
    movzx   eax, word [ecx + IA32_TSS._ES]
    push    eax
    movzx   eax, word [ecx + IA32_TSS._FS]
    push    eax
    movzx   eax, word [ecx + IA32_TSS._GS]
    push    eax

;; UINT32  Eip;
    push    dword [ecx + IA32_TSS.EIP]

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
    mov     eax, ebx    ; ebx still keeps selector of interrupted task
    push    eax
    movzx   eax, word [ecx + IA32_TSS.LDT]
    push    eax

;; UINT32  EFlags;
    push    dword [ecx + IA32_TSS.EFLAGS]

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    mov     eax, cr4
    push    eax             ; push cr4 firstly

    mov     edx, [ebp - 4]  ; cpuid.edx
    test    edx, BIT24      ; Test for FXSAVE/FXRESTOR support
    jz      .1
    or      eax, BIT9       ; Set CR4.OSFXSR
.1:
    test    edx, BIT2       ; Test for Debugging Extensions support
    jz      .2
    or      eax, BIT3       ; Set CR4.DE
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
;; Clear TS bit in CR0 to avoid Device Not Available Exception (#NM)
;; when executing fxsave/fxrstor instruction
    test    edx, BIT24  ; Test for FXSAVE/FXRESTOR support.
                        ; edx still contains result from CPUID above
    jz      .3
    clts
    sub     esp, 512
    mov     edi, esp
    fxsave  [edi]
.3:

;; UINT32  ExceptionData;
    push    dword [ebp + 8]

;; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

;; call into exception handler
    mov     esi, ecx            ; Keep TSS base to avoid overwrite
    mov     eax, ASM_PFX(CommonExceptionHandler)

;; Prepare parameter and call
    mov     edx, esp
    push    edx                 ; EFI_SYSTEM_CONTEXT
    push    dword [ebp + 4]     ; EFI_EXCEPTION_TYPE (vector number)

    ;
    ; Call External Exception Handler
    ;
    call    eax
    add     esp, 8              ; Restore stack before calling
    mov     ecx, esi            ; Restore TSS base

;; UINT32  ExceptionData;
    add     esp, 4

;; FX_SAVE_STATE_IA32 FxSaveState;
    mov     edx, [ebp - 4]  ; cpuid.edx
    test    edx, BIT24      ; Test for FXSAVE/FXRESTOR support
    jz      .4
    mov     esi, esp
    fxrstor [esi]
.4:
    add     esp, 512

;; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
;; Skip restoration of DRx registers to support debuggers
;; that set breakpoints in interrupt/exception context
    add     esp, 4 * 6

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    pop     eax
    mov     cr0, eax
    add     esp, 4    ; not for Cr1
    pop     eax
    mov     cr2, eax
    pop     eax
    mov     dword [ecx + IA32_TSS._CR3], eax
    pop     eax
    mov     cr4, eax

;; UINT32  EFlags;
    pop     dword [ecx + IA32_TSS.EFLAGS]
    mov     ebx, dword [ecx + IA32_TSS.EFLAGS]
    btr     ebx, 9      ; Do 'cli'
    mov     dword [ecx + IA32_TSS.EFLAGS], ebx

;; UINT32  Ldtr, Tr;
;; UINT32  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
    add     esp, 24

;; UINT32  Eip;
    pop     dword [ecx + IA32_TSS.EIP]

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
;; NOTE - modified segment registers could hang the debugger...  We
;;        could attempt to insulate ourselves against this possibility,
;;        but that poses risks as well.
;;
    pop     eax
o16 mov     [ecx + IA32_TSS._GS], ax
    pop     eax
o16 mov     [ecx + IA32_TSS._FS], ax
    pop     eax
o16 mov     [ecx + IA32_TSS._ES], ax
    pop     eax
o16 mov     [ecx + IA32_TSS._DS], ax
    pop     eax
o16 mov     [ecx + IA32_TSS._CS], ax
    pop     eax
o16 mov     [ecx + IA32_TSS._SS], ax

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    pop     dword [ecx + IA32_TSS._EDI]
    pop     dword [ecx + IA32_TSS._ESI]
    add     esp, 4   ; not for ebp
    add     esp, 4   ; not for esp
    pop     dword [ecx + IA32_TSS._EBX]
    pop     dword [ecx + IA32_TSS._EDX]
    pop     dword [ecx + IA32_TSS._ECX]
    pop     dword [ecx + IA32_TSS._EAX]

; Set single step DB# to allow debugger to able to go back to the EIP
; where the exception is triggered.

;; Create return context for iretd in stub function
    mov    eax, dword [ecx + IA32_TSS._ESP]      ; Get old stack pointer
    mov    ebx, dword [ecx + IA32_TSS.EIP]
    mov    [eax - 0xc], ebx                      ; create EIP in old stack
    movzx  ebx, word [ecx + IA32_TSS._CS]
    mov    [eax - 0x8], ebx                      ; create CS in old stack
    mov    ebx, dword [ecx + IA32_TSS.EFLAGS]
    bts    ebx, 8                                ; Set TF
    mov    [eax - 0x4], ebx                      ; create eflags in old stack
    sub    eax, 0xc                              ; minus 12 byte
    mov    dword [ecx + IA32_TSS._ESP], eax      ; Set new stack pointer

;; Replace the EIP of interrupted task with stub function
    mov    eax, ASM_PFX(SingleStepStubFunction)
    mov    dword [ecx + IA32_TSS.EIP], eax

    mov     ecx, [ebp - 8]                       ; Get current TSS base
    mov     eax, dword [ecx + IA32_TSS._ESP]     ; Return current stack top
    mov     esp, ebp

    ret

global ASM_PFX(SingleStepStubFunction)
ASM_PFX(SingleStepStubFunction):
;
; we need clean TS bit in CR0 to execute
; x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions.
;
    clts
    iretd

global ASM_PFX(AsmGetTssTemplateMap)
ASM_PFX(AsmGetTssTemplateMap):
    push    ebp                 ; C prolog
    mov     ebp, esp
    pushad

    mov ebx, dword [ebp + 0x8]
    mov dword [ebx],       ASM_PFX(ExceptionTaskSwtichEntry0)
    mov dword [ebx + 0x4], (AsmExceptionEntryEnd - AsmExceptionEntryBegin) / 32
    mov dword [ebx + 0x8], 0

    popad
    pop     ebp
    ret

