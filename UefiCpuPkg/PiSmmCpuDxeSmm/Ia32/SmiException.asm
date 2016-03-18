;------------------------------------------------------------------------------ ;
; Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   SmiException.asm
;
; Abstract:
;
;   Exception handlers used in SM mode
;
;-------------------------------------------------------------------------------

    .686p
    .model  flat,C

EXTERNDEF   SmiPFHandler:PROC
EXTERNDEF   PageFaultStubFunction:PROC
EXTERNDEF   gSmiMtrrs:QWORD
EXTERNDEF   gcSmiIdtr:FWORD
EXTERNDEF   gcSmiGdtr:FWORD
EXTERNDEF   gcPsd:BYTE
EXTERNDEF   FeaturePcdGet (PcdCpuSmmProfileEnable):BYTE


    .data

NullSeg     DQ      0                   ; reserved by architecture
CodeSeg32   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      9bh
            DB      0cfh                ; LimitHigh
            DB      0                   ; BaseHigh
ProtModeCodeSeg32   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      9bh
            DB      0cfh                ; LimitHigh
            DB      0                   ; BaseHigh
ProtModeSsSeg32     LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      93h
            DB      0cfh                ; LimitHigh
            DB      0                   ; BaseHigh
DataSeg32   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      93h
            DB      0cfh                ; LimitHigh
            DB      0                   ; BaseHigh
CodeSeg16   LABEL   QWORD
            DW      -1
            DW      0
            DB      0
            DB      9bh
            DB      8fh
            DB      0
DataSeg16   LABEL   QWORD
            DW      -1
            DW      0
            DB      0
            DB      93h
            DB      8fh
            DB      0
CodeSeg64   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      9bh
            DB      0afh                ; LimitHigh
            DB      0                   ; BaseHigh
GDT_SIZE = $ - offset NullSeg

TssSeg      LABEL   QWORD
            DW      TSS_DESC_SIZE - 1   ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      89h
            DB      00h                 ; LimitHigh
            DB      0                   ; BaseHigh
ExceptionTssSeg     LABEL   QWORD
            DW      TSS_DESC_SIZE - 1   ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      89h
            DB      00h                 ; LimitHigh
            DB      0                   ; BaseHigh

CODE_SEL          = offset CodeSeg32 - offset NullSeg
DATA_SEL          = offset DataSeg32 - offset NullSeg
TSS_SEL           = offset TssSeg - offset NullSeg
EXCEPTION_TSS_SEL = offset ExceptionTssSeg - offset NullSeg

IA32_TSS STRUC
                    DW ?
                    DW ?
  ESP0              DD ?
  SS0               DW ?
                    DW ?
  ESP1              DD ?
  SS1               DW ?
                    DW ?
  ESP2              DD ?
  SS2               DW ?
                    DW ?
  _CR3              DD ?
  EIP               DD ?
  EFLAGS            DD ?
  _EAX              DD ?
  _ECX              DD ?
  _EDX              DD ?
  _EBX              DD ?
  _ESP              DD ?
  _EBP              DD ?
  _ESI              DD ?
  _EDI              DD ?
  _ES               DW ?
                    DW ?
  _CS               DW ?
                    DW ?
  _SS               DW ?
                    DW ?
  _DS               DW ?
                    DW ?
  _FS               DW ?
                    DW ?
  _GS               DW ?
                    DW ?
  LDT               DW ?
                    DW ?
                    DW ?
                    DW ?
IA32_TSS ENDS

; Create 2 TSS segments just after GDT
TssDescriptor LABEL BYTE
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
TSS_DESC_SIZE = $ - offset TssDescriptor

ExceptionTssDescriptor LABEL BYTE
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
            DD      offset PFHandlerEntry ; EIP
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

gcPsd     LABEL   BYTE
            DB      'PSDSIG  '
            DW      PSD_SIZE
            DW      2
            DW      1 SHL 2
            DW      CODE_SEL
            DW      DATA_SEL
            DW      DATA_SEL
            DW      DATA_SEL
            DW      0
            DQ      0
            DQ      0
            DQ      0
            DQ      offset NullSeg
            DD      GDT_SIZE
            DD      0
            DB      24 dup (0)
            DQ      offset gSmiMtrrs
PSD_SIZE  = $ - offset gcPsd

gcSmiGdtr   LABEL   FWORD
    DW      GDT_SIZE - 1
    DD      offset NullSeg

gcSmiIdtr   LABEL   FWORD
    DW      IDT_SIZE - 1
    DD      offset _SmiIDT

_SmiIDT     LABEL   QWORD
REPEAT      32
    DW      0                           ; Offset 0:15
    DW      CODE_SEL                    ; Segment selector
    DB      0                           ; Unused
    DB      8eh                         ; Interrupt Gate, Present
    DW      0                           ; Offset 16:31
            ENDM
IDT_SIZE = $ - offset _SmiIDT

TaskGateDescriptor LABEL DWORD
    DW      0                           ; Reserved
    DW      EXCEPTION_TSS_SEL           ; TSS Segment selector
    DB      0                           ; Reserved
    DB      85h                         ; Task Gate, present, DPL = 0
    DW      0                           ; Reserved


    .code
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
PageFaultIdtHandlerSmmProfile PROC
    push    0eh                         ; Page Fault

    push    ebp
    mov     ebp, esp


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

; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

;; UINT32  ExceptionData;
    push    dword ptr [ebp + 2 * 4]

;; call into exception handler

;; Prepare parameter and call
    mov     edx, esp
    push    edx
    mov     edx, dword ptr [ebp + 1 * 4]
    push    edx

    ;
    ; Call External Exception Handler
    ;
    mov     eax, SmiPFHandler
    call    eax
    add     esp, 8

;; UINT32  ExceptionData;
    add     esp, 4

;; FX_SAVE_STATE_IA32 FxSaveState;
    mov     esi, esp
    db      0fh, 0aeh, 0eh ; fxrstor [esi]
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

; Enable TF bit after page fault handler runs
    bts     dword ptr [esp + 16], 8  ; EFLAGS

    add     esp, 8                      ; skip INT# & ErrCode
Return:
    iretd
;
; Page Fault Exception Handler entry when SMM Stack Guard is enabled
; Executiot starts here after a task switch
;
PFHandlerEntry::
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
    and     esp, 0fffffff0h
    sub     esp, 12

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    push    (IA32_TSS ptr [ecx])._EAX
    push    (IA32_TSS ptr [ecx])._ECX
    push    (IA32_TSS ptr [ecx])._EDX
    push    (IA32_TSS ptr [ecx])._EBX
    push    (IA32_TSS ptr [ecx])._ESP
    push    (IA32_TSS ptr [ecx])._EBP
    push    (IA32_TSS ptr [ecx])._ESI
    push    (IA32_TSS ptr [ecx])._EDI

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
    movzx   eax, (IA32_TSS ptr [ecx])._SS
    push    eax
    movzx   eax, (IA32_TSS ptr [ecx])._CS
    push    eax
    movzx   eax, (IA32_TSS ptr [ecx])._DS
    push    eax
    movzx   eax, (IA32_TSS ptr [ecx])._ES
    push    eax
    movzx   eax, (IA32_TSS ptr [ecx])._FS
    push    eax
    movzx   eax, (IA32_TSS ptr [ecx])._GS
    push    eax

;; UINT32  Eip;
    push    (IA32_TSS ptr [ecx]).EIP

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
    mov     eax, TSS_SEL
    push    eax
    movzx   eax, (IA32_TSS ptr [ecx]).LDT
    push    eax

;; UINT32  EFlags;
    push    (IA32_TSS ptr [ecx]).EFLAGS

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
;; Clear TS bit in CR0 to avoid Device Not Available Exception (#NM)
;; when executing fxsave/fxrstor instruction
    clts
    sub     esp, 512
    mov     edi, esp
    db      0fh, 0aeh, 07h ;fxsave [edi]

; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

;; UINT32  ExceptionData;
    push    dword ptr [ebp]

;; call into exception handler
    mov     ebx, ecx
    mov     eax, SmiPFHandler

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
    db      0fh, 0aeh, 0eh ; fxrstor [esi]
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
    mov     (IA32_TSS ptr [ecx])._CR3, eax
    pop     eax
    mov     cr4, eax

;; UINT32  EFlags;
    pop     (IA32_TSS ptr [ecx]).EFLAGS

;; UINT32  Ldtr, Tr;
;; UINT32  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
    add     esp, 24

;; UINT32  Eip;
    pop     (IA32_TSS ptr [ecx]).EIP

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
;; NOTE - modified segment registers could hang the debugger...  We
;;        could attempt to insulate ourselves against this possibility,
;;        but that poses risks as well.
;;
    pop     eax
    mov     (IA32_TSS ptr [ecx])._GS, ax
    pop     eax
    mov     (IA32_TSS ptr [ecx])._FS, ax
    pop     eax
    mov     (IA32_TSS ptr [ecx])._ES, ax
    pop     eax
    mov     (IA32_TSS ptr [ecx])._DS, ax
    pop     eax
    mov     (IA32_TSS ptr [ecx])._CS, ax
    pop     eax
    mov     (IA32_TSS ptr [ecx])._SS, ax

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    pop     (IA32_TSS ptr [ecx])._EDI
    pop     (IA32_TSS ptr [ecx])._ESI
    add     esp, 4   ; not for ebp
    add     esp, 4   ; not for esp
    pop     (IA32_TSS ptr [ecx])._EBX
    pop     (IA32_TSS ptr [ecx])._EDX
    pop     (IA32_TSS ptr [ecx])._ECX
    pop     (IA32_TSS ptr [ecx])._EAX

    mov     esp, ebp

; Set single step DB# if SMM profile is enabled and page fault exception happens
    cmp     FeaturePcdGet (PcdCpuSmmProfileEnable), 0
    jz      @Done2

; Create return context for iretd in stub function
    mov    eax, (IA32_TSS ptr [ecx])._ESP        ; Get old stack pointer
    mov    ebx, (IA32_TSS ptr [ecx]).EIP
    mov    [eax - 0ch], ebx                      ; create EIP in old stack
    movzx  ebx, (IA32_TSS ptr [ecx])._CS
    mov    [eax - 08h], ebx                      ; create CS in old stack
    mov    ebx, (IA32_TSS ptr [ecx]).EFLAGS
    bts    ebx, 8
    mov    [eax - 04h], ebx                      ; create eflags in old stack
    mov    eax, (IA32_TSS ptr [ecx])._ESP        ; Get old stack pointer
    sub    eax, 0ch                              ; minus 12 byte
    mov    (IA32_TSS ptr [ecx])._ESP, eax        ; Set new stack pointer
; Replace the EIP of interrupted task with stub function
    mov    eax, PageFaultStubFunction
    mov    (IA32_TSS ptr [ecx]).EIP, eax
; Jump to the iretd so next page fault handler as a task will start again after iretd.
@Done2:
    add     esp, 4                      ; skip ErrCode

    jmp     Return
PageFaultIdtHandlerSmmProfile ENDP

PageFaultStubFunction   PROC
;
; we need clean TS bit in CR0 to execute
; x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions.
;
    clts
    iretd
PageFaultStubFunction   ENDP

InitializeIDTSmmStackGuard   PROC    USES    ebx
;
; If SMM Stack Guard feature is enabled, the Page Fault Exception entry in IDT
; is a Task Gate Descriptor so that when a Page Fault Exception occurs,
; the processors can use a known good stack in case stack is ran out.
;
    lea     ebx, _SmiIDT + 14 * 8
    lea     edx, TaskGateDescriptor
    mov     eax, [edx]
    mov     [ebx], eax
    mov     eax, [edx + 4]
    mov     [ebx + 4], eax
    ret
InitializeIDTSmmStackGuard   ENDP

    END
