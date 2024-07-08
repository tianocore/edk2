;------------------------------------------------------------------------------ ;
; Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   SmiException.nasm
;
; Abstract:
;
;   Exception handlers used in SM mode
;
;-------------------------------------------------------------------------------

extern  ASM_PFX(SmiPFHandler)
extern  ASM_PFX(mSetupDebugTrap)

global  ASM_PFX(gcSmiIdtr)
global  ASM_PFX(gcSmiGdtr)
global  ASM_PFX(gTaskGateDescriptor)
global  ASM_PFX(gcPsd)

    SECTION .data

NullSeg: DQ 0                   ; reserved by architecture
CodeSeg32:
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      0x9b
            DB      0xcf                ; LimitHigh
            DB      0                   ; BaseHigh
ProtModeCodeSeg32:
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      0x9b
            DB      0xcf                ; LimitHigh
            DB      0                   ; BaseHigh
ProtModeSsSeg32:
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      0x93
            DB      0xcf                ; LimitHigh
            DB      0                   ; BaseHigh
DataSeg32:
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      0x93
            DB      0xcf                ; LimitHigh
            DB      0                   ; BaseHigh
CodeSeg16:
            DW      -1
            DW      0
            DB      0
            DB      0x9b
            DB      0x8f
            DB      0
DataSeg16:
            DW      -1
            DW      0
            DB      0
            DB      0x93
            DB      0x8f
            DB      0
CodeSeg64:
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      0x9b
            DB      0xaf                ; LimitHigh
            DB      0                   ; BaseHigh
GDT_SIZE equ $ - NullSeg

TssSeg:
            DW      TSS_DESC_SIZE       ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      0x89
            DB      0x80                ; LimitHigh
            DB      0                   ; BaseHigh
ExceptionTssSeg:
            DW      EXCEPTION_TSS_DESC_SIZE       ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      0x89
            DB      0x80                ; LimitHigh
            DB      0                   ; BaseHigh

CODE_SEL          equ CodeSeg32 - NullSeg
DATA_SEL          equ DataSeg32 - NullSeg
TSS_SEL           equ TssSeg - NullSeg
EXCEPTION_TSS_SEL equ ExceptionTssSeg - NullSeg

struc IA32_TSS
                    resw 1
                    resw 1
  .ESP0: resd 1
  .SS0:  resw 1
                    resw 1
  .ESP1: resd 1
  .SS1:  resw 1
                    resw 1
  .ESP2: resd 1
  .SS2:  resw 1
                    resw 1
  ._CR3: resd 1
  .EIP:  resd 1
  .EFLAGS: resd 1
  ._EAX: resd 1
  ._ECX: resd 1
  ._EDX: resd 1
  ._EBX: resd 1
  ._ESP: resd 1
  ._EBP: resd 1
  ._ESI: resd 1
  ._EDI: resd 1
  ._ES:  resw 1
                    resw 1
  ._CS: resw 1
                    resw 1
  ._SS: resw 1
                    resw 1
  ._DS: resw 1
                    resw 1
  ._FS: resw 1
                    resw 1
  ._GS: resw 1
                    resw 1
  .LDT: resw 1
                    resw 1
                    resw 1
                    resw 1
endstruc

; Create 2 TSS segments just after GDT
TssDescriptor:
            DW      0                   ; PreviousTaskLink
            DW      0                   ; Reserved
            DD      0                   ; ESP0
            DW      0                   ; SS0
            DW      0                   ; Reserved
            DD      0                   ; ESP1
            DW      0                   ; SS1
            DW      0                   ; Reserved
            DD      0                   ; ESP2
            DW      0                   ; SS2
            DW      0                   ; Reserved
            DD      0                   ; CR3
            DD      0                   ; EIP
            DD      0                   ; EFLAGS
            DD      0                   ; EAX
            DD      0                   ; ECX
            DD      0                   ; EDX
            DD      0                   ; EBX
            DD      0                   ; ESP
            DD      0                   ; EBP
            DD      0                   ; ESI
            DD      0                   ; EDI
            DW      0                   ; ES
            DW      0                   ; Reserved
            DW      0                   ; CS
            DW      0                   ; Reserved
            DW      0                   ; SS
            DW      0                   ; Reserved
            DW      0                   ; DS
            DW      0                   ; Reserved
            DW      0                   ; FS
            DW      0                   ; Reserved
            DW      0                   ; GS
            DW      0                   ; Reserved
            DW      0                   ; LDT Selector
            DW      0                   ; Reserved
            DW      0                   ; T
            DW      0                   ; I/O Map Base
TSS_DESC_SIZE equ $ - TssDescriptor

ExceptionTssDescriptor:
            DW      0                   ; PreviousTaskLink
            DW      0                   ; Reserved
            DD      0                   ; ESP0
            DW      0                   ; SS0
            DW      0                   ; Reserved
            DD      0                   ; ESP1
            DW      0                   ; SS1
            DW      0                   ; Reserved
            DD      0                   ; ESP2
            DW      0                   ; SS2
            DW      0                   ; Reserved
            DD      0                   ; CR3
            DD      PFHandlerEntry ; EIP
            DD      00000002            ; EFLAGS
            DD      0                   ; EAX
            DD      0                   ; ECX
            DD      0                   ; EDX
            DD      0                   ; EBX
            DD      0                   ; ESP
            DD      0                   ; EBP
            DD      0                   ; ESI
            DD      0                   ; EDI
            DW      DATA_SEL            ; ES
            DW      0                   ; Reserved
            DW      CODE_SEL            ; CS
            DW      0                   ; Reserved
            DW      DATA_SEL            ; SS
            DW      0                   ; Reserved
            DW      DATA_SEL            ; DS
            DW      0                   ; Reserved
            DW      DATA_SEL            ; FS
            DW      0                   ; Reserved
            DW      DATA_SEL            ; GS
            DW      0                   ; Reserved
            DW      0                   ; LDT Selector
            DW      0                   ; Reserved
            DW      0                   ; T
            DW      0                   ; I/O Map Base
            DD      0                   ; SSP
EXCEPTION_TSS_DESC_SIZE equ $ - ExceptionTssDescriptor

ASM_PFX(gcPsd):
            DB      'PSDSIG  '
            DW      PSD_SIZE
            DW      2
            DW      1 << 2
            DW      CODE_SEL
            DW      DATA_SEL
            DW      DATA_SEL
            DW      DATA_SEL
            DW      0
            DQ      0
            DQ      0
            DQ      0
            DD      0
            DD      NullSeg
            DD      GDT_SIZE
            DD      0
            times   24 DB 0
            DD      0
            DD      0
PSD_SIZE  equ $ - ASM_PFX(gcPsd)

ASM_PFX(gcSmiGdtr):
    DW      GDT_SIZE - 1
    DD      NullSeg

ASM_PFX(gcSmiIdtr):
    DW      0
    DD      0

ASM_PFX(gTaskGateDescriptor):
    DW      0                           ; Reserved
    DW      EXCEPTION_TSS_SEL           ; TSS Segment selector
    DB      0                           ; Reserved
    DB      0x85                         ; Task Gate, present, DPL = 0
    DW      0                           ; Reserved

    SECTION .text
;------------------------------------------------------------------------------
; PageFaultIdtHandlerSmmProfile is the entry point page fault only
;
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
;------------------------------------------------------------------------------
global ASM_PFX(PageFaultIdtHandlerSmmProfile)
ASM_PFX(PageFaultIdtHandlerSmmProfile):
    push    0xe                         ; Page Fault

    push    ebp
    mov     ebp, esp

    ;
    ; Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
    ; is 16-byte aligned
    ;
    and     esp, 0xfffffff0
    sub     esp, 12

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
    mov     eax, cr4
    or      eax, 0x208
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
    fxsave  [edi]

; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

;; UINT32  ExceptionData;
    push    dword [ebp + 2 * 4]

;; call into exception handler

;; Prepare parameter and call
    mov     edx, esp
    push    edx
    mov     edx, dword [ebp + 1 * 4]
    push    edx

    ;
    ; Call External Exception Handler
    ;
    mov     eax, ASM_PFX(SmiPFHandler)
    call    eax
    add     esp, 8

;; UINT32  ExceptionData;
    add     esp, 4

;; FX_SAVE_STATE_IA32 FxSaveState;
    mov     esi, esp
    fxrstor [esi]
    add     esp, 512

;; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
;; Skip restoration of DRx registers to support debuggers
;; that set breakpoint in interrupt/exception context
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

    mov     esp, ebp
    pop     ebp

; Enable TF bit after page fault handler runs
    bts     dword [esp + 16], 8  ; EFLAGS

    add     esp, 8                      ; skip INT# & ErrCode
Return:
    iretd
;
; Page Fault Exception Handler entry when SMM Stack Guard is enabled
; Executiot starts here after a task switch
;
PFHandlerEntry:
;
; Get this processor's TSS
;
    sub     esp, 8
    sgdt    [esp + 2]
    mov     eax, [esp + 4]              ; GDT base
    add     esp, 8
    mov     ecx, [eax + TSS_SEL + 2]
    shl     ecx, 8
    mov     cl, [eax + TSS_SEL + 7]
    ror     ecx, 8                      ; ecx = TSS base

    mov     ebp, esp

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
    mov     eax, TSS_SEL
    push    eax
    movzx   eax, word [ecx + IA32_TSS.LDT]
    push    eax

;; UINT32  EFlags;
    push    dword [ecx + IA32_TSS.EFLAGS]

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    mov     eax, cr4
    or      eax, 0x208
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
;; Clear TS bit in CR0 to avoid Device Not Available Exception (#NM)
;; when executing fxsave/fxrstor instruction
    clts
    sub     esp, 512
    mov     edi, esp
    fxsave  [edi]

; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

;; UINT32  ExceptionData;
    push    dword [ebp]

;; call into exception handler
    mov     ebx, ecx
    mov     eax, ASM_PFX(SmiPFHandler)

;; Prepare parameter and call
    mov     edx, esp
    push    edx
    mov     edx, 14
    push    edx

    ;
    ; Call External Exception Handler
    ;
    call    eax
    add     esp, 8

    mov     ecx, ebx
;; UINT32  ExceptionData;
    add     esp, 4

;; FX_SAVE_STATE_IA32 FxSaveState;
    mov     esi, esp
    fxrstor [esi]
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

    mov     esp, ebp

; Set single step DB# if SMM profile is enabled and page fault exception happens
    cmp     byte [dword ASM_PFX(mSetupDebugTrap)], 0
    jz      @Done2

; Create return context for iretd in stub function
    mov    eax, dword [ecx + IA32_TSS._ESP]        ; Get old stack pointer
    mov    ebx, dword [ecx + IA32_TSS.EIP]
    mov    [eax - 0xc], ebx                      ; create EIP in old stack
    movzx  ebx, word [ecx + IA32_TSS._CS]
    mov    [eax - 0x8], ebx                      ; create CS in old stack
    mov    ebx, dword [ecx + IA32_TSS.EFLAGS]
    bts    ebx, 8
    mov    [eax - 0x4], ebx                      ; create eflags in old stack
    mov    eax, dword [ecx + IA32_TSS._ESP]        ; Get old stack pointer
    sub    eax, 0xc                              ; minus 12 byte
    mov    dword [ecx + IA32_TSS._ESP], eax        ; Set new stack pointer
; Replace the EIP of interrupted task with stub function
    mov    eax, ASM_PFX(PageFaultStubFunction)
    mov    dword [ecx + IA32_TSS.EIP], eax
; Jump to the iretd so next page fault handler as a task will start again after iretd.
@Done2:
    add     esp, 4                      ; skip ErrCode

    jmp     Return

global ASM_PFX(PageFaultStubFunction)
ASM_PFX(PageFaultStubFunction):
;
; we need clean TS bit in CR0 to execute
; x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions.
;
    clts
    iretd

