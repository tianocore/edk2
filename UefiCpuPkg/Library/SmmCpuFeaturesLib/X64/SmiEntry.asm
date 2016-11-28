;------------------------------------------------------------------------------ ;
; Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved.<BR>
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
;   SmiEntry.asm
;
; Abstract:
;
;   Code template of the SMI handler for a particular processor
;
;-------------------------------------------------------------------------------

;
; Variables referenced by C code
;
EXTERNDEF   SmiRendezvous:PROC
EXTERNDEF   CpuSmmDebugEntry:PROC
EXTERNDEF   CpuSmmDebugExit:PROC
EXTERNDEF   gcStmSmiHandlerTemplate:BYTE
EXTERNDEF   gcStmSmiHandlerSize:WORD
EXTERNDEF   gcStmSmiHandlerOffset:WORD
EXTERNDEF   gStmSmiCr3:DWORD
EXTERNDEF   gStmSmiStack:DWORD
EXTERNDEF   gStmSmbase:DWORD
EXTERNDEF   gStmXdSupported:BYTE
EXTERNDEF   gStmSmiHandlerIdtr:FWORD

MSR_IA32_MISC_ENABLE  EQU     1A0h
MSR_EFER      EQU     0c0000080h
MSR_EFER_XD   EQU     0800h

;
; Constants relating to TXT_PROCESSOR_SMM_DESCRIPTOR
;
DSC_OFFSET    EQU     0fb00h
DSC_GDTPTR    EQU     48h
DSC_GDTSIZ    EQU     50h
DSC_CS        EQU     14h
DSC_DS        EQU     16h
DSC_SS        EQU     18h
DSC_OTHERSEG  EQU     1ah
;
; Constants relating to CPU State Save Area
;
SSM_DR6         EQU     0ffd0h
SSM_DR7         EQU     0ffc8h

PROTECT_MODE_CS EQU     08h
PROTECT_MODE_DS EQU     20h
LONG_MODE_CS    EQU     38h
TSS_SEGMENT     EQU     40h
GDT_SIZE        EQU     50h

    .code

gcStmSmiHandlerTemplate    LABEL   BYTE

_StmSmiEntryPoint:
    ;
    ; The encoding of BX in 16-bit addressing mode is the same as of RDI in 64-
    ; bit addressing mode. And that coincidence has been used in the following
    ; "64-bit like" 16-bit code. Be aware that once RDI is referenced as a
    ; base address register, it is actually BX that is referenced.
    ;
    DB      0bbh                        ; mov bx, imm16
    DW      offset _StmGdtDesc - _StmSmiEntryPoint + 8000h  ; bx = GdtDesc offset
; fix GDT descriptor
    DB      2eh, 0a1h                   ; mov ax, cs:[offset16]
    DW      DSC_OFFSET + DSC_GDTSIZ
    DB      48h                         ; dec ax
    DB      2eh
    mov     [rdi], eax                  ; mov cs:[bx], ax
    DB      66h, 2eh, 0a1h              ; mov eax, cs:[offset16]
    DW      DSC_OFFSET + DSC_GDTPTR
    DB      2eh
    mov     [rdi + 2], ax               ; mov cs:[bx + 2], eax
    DB      66h, 2eh
    lgdt    fword ptr [rdi]             ; lgdt fword ptr cs:[bx]
; Patch ProtectedMode Segment
    DB      0b8h                        ; mov ax, imm16
    DW      PROTECT_MODE_CS             ; set AX for segment directly
    DB      2eh
    mov     [rdi - 2], eax              ; mov cs:[bx - 2], ax
; Patch ProtectedMode entry
    DB      66h, 0bfh                   ; mov edi, SMBASE
gStmSmbase    DD    ?
    lea     ax, [edi + (@ProtectedMode - _StmSmiEntryPoint) + 8000h]
    DB      2eh
    mov     [rdi - 6], ax               ; mov cs:[bx - 6], eax
; Switch into @ProtectedMode
    mov     rbx, cr0
    DB      66h
    and     ebx, 9ffafff3h
    DB      66h
    or      ebx, 00000023h

    mov     cr0, rbx
    DB      66h, 0eah
    DD      ?
    DW      ?

_StmGdtDesc    FWORD   ?
@ProtectedMode:
    mov     ax, PROTECT_MODE_DS
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    DB      0bch                   ; mov esp, imm32
gStmSmiStack   DD      ?
    jmp     ProtFlatMode

ProtFlatMode:
    DB      0b8h                        ; mov eax, offset gStmSmiCr3
gStmSmiCr3     DD      ?
    mov     cr3, rax
    mov     eax, 668h                   ; as cr4.PGE is not set here, refresh cr3
    mov     cr4, rax                    ; in PreModifyMtrrs() to flush TLB.
; Load TSS
    sub     esp, 8                      ; reserve room in stack
    sgdt    fword ptr [rsp]
    mov     eax, [rsp + 2]              ; eax = GDT base
    add     esp, 8
    mov     dl, 89h
    mov     [rax + TSS_SEGMENT + 5], dl ; clear busy flag
    mov     eax, TSS_SEGMENT
    ltr     ax

; enable NXE if supported
    DB      0b0h                        ; mov al, imm8
gStmXdSupported     DB      1
    cmp     al, 0
    jz      @SkipXd
;
; Check XD disable bit
;
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    sub     esp, 4
    push    rdx                        ; save MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2                  ; MSR_IA32_MISC_ENABLE[34]
    jz      @f
    and     dx, 0FFFBh                 ; clear XD Disable bit if it is set
    wrmsr
@@:
    mov     ecx, MSR_EFER
    rdmsr
    or      ax, MSR_EFER_XD            ; enable NXE
    wrmsr
    jmp     @XdDone
@SkipXd:
    sub     esp, 8
@XdDone:

; Switch into @LongMode
    push    LONG_MODE_CS                ; push cs hardcore here
    call    Base                       ; push return address for retf later
Base:
    add     dword ptr [rsp], @LongMode - Base; offset for far retf, seg is the 1st arg

    mov     ecx, MSR_EFER
    rdmsr
    or      ah, 1                      ; enable LME
    wrmsr
    mov     rbx, cr0
    or      ebx, 080010023h            ; enable paging + WP + NE + MP + PE
    mov     cr0, rbx
    retf
@LongMode:                              ; long mode (64-bit code) starts here
    mov     rax, offset gStmSmiHandlerIdtr
    lidt    fword ptr [rax]
    lea     ebx, [rdi + DSC_OFFSET]
    mov     ax, [rbx + DSC_DS]
    mov     ds, eax
    mov     ax, [rbx + DSC_OTHERSEG]
    mov     es, eax
    mov     fs, eax
    mov     gs, eax
    mov     ax, [rbx + DSC_SS]
    mov     ss, eax

CommonHandler:
    mov     rbx, [rsp + 0x08]           ; rbx <- CpuIndex

    ;
    ; Save FP registers
    ;
    sub     rsp, 200h
    DB      48h                         ; FXSAVE64
    fxsave  [rsp]

    add     rsp, -20h

    mov     rcx, rbx
    mov     rax, CpuSmmDebugEntry
    call    rax

    mov     rcx, rbx
    mov     rax, SmiRendezvous          ; rax <- absolute addr of SmiRedezvous
    call    rax

    mov     rcx, rbx
    mov     rax, CpuSmmDebugExit
    call    rax

    add     rsp, 20h

    ;
    ; Restore FP registers
    ;
    DB      48h                         ; FXRSTOR64
    fxrstor [rsp]

    add     rsp, 200h

    mov     rax, offset ASM_PFX(gStmXdSupported)
    mov     al, [rax]
    cmp     al, 0
    jz      @f
    pop     rdx                       ; get saved MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2
    jz      @f
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    or      dx, BIT2                  ; set XD Disable bit if it was set before entering into SMM
    wrmsr

@@:
    rsm

_StmSmiHandler:
;
; Check XD disable bit
;
    xor     r8, r8
    mov     rax, offset ASM_PFX(gStmXdSupported)
    mov     al, [rax]
    cmp     al, 0
    jz      @StmXdDone
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    mov     r8, rdx                   ; save MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2                  ; MSR_IA32_MISC_ENABLE[34]
    jz      @f
    and     dx, 0FFFBh                 ; clear XD Disable bit if it is set
    wrmsr
@@:
    mov     ecx, MSR_EFER
    rdmsr
    or      ax, MSR_EFER_XD            ; enable NXE
    wrmsr
@StmXdDone:
    push    r8

    ; below step is needed, because STM does not run above code.
    ; we have to run below code to set IDT/CR0/CR4
    mov     rax, offset gStmSmiHandlerIdtr
    lidt    fword ptr [rax]

    mov     rax, cr0
    or      eax, 80010023h              ; enable paging + WP + NE + MP + PE
    mov     cr0, rax
    mov     rax, cr4
    mov     eax, 668h                   ; as cr4.PGE is not set here, refresh cr3
    mov     cr4, rax                    ; in PreModifyMtrrs() to flush TLB.
    ; STM init finish
    jmp     CommonHandler

gcStmSmiHandlerSize    DW      $ - _StmSmiEntryPoint
gcStmSmiHandlerOffset  DW      _StmSmiHandler - _StmSmiEntryPoint

    END
