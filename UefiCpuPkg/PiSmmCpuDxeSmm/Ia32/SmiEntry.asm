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

    .686p
    .model  flat,C
    .xmm

MSR_IA32_MISC_ENABLE  EQU     1A0h
MSR_EFER      EQU     0c0000080h
MSR_EFER_XD   EQU     0800h

;
; Constants relating to PROCESSOR_SMM_DESCRIPTOR
;
DSC_OFFSET    EQU     0fb00h
DSC_GDTPTR    EQU     30h
DSC_GDTSIZ    EQU     38h
DSC_CS        EQU     14
DSC_DS        EQU     16
DSC_SS        EQU     18
DSC_OTHERSEG  EQU     20

PROTECT_MODE_CS EQU   08h
PROTECT_MODE_DS EQU   20h
TSS_SEGMENT     EQU   40h

SmiRendezvous      PROTO   C
CpuSmmDebugEntry   PROTO   C
CpuSmmDebugExit    PROTO   C

EXTERNDEF   gcSmiHandlerTemplate:BYTE
EXTERNDEF   gcSmiHandlerSize:WORD
EXTERNDEF   gSmiCr3:DWORD
EXTERNDEF   gSmiStack:DWORD
EXTERNDEF   gSmbase:DWORD
EXTERNDEF   mXdSupported:BYTE
EXTERNDEF   FeaturePcdGet (PcdCpuSmmStackGuard):BYTE
EXTERNDEF   gSmiHandlerIdtr:FWORD

    .code

gcSmiHandlerTemplate    LABEL   BYTE

_SmiEntryPoint:
    DB      0bbh                        ; mov bx, imm16
    DW      offset _GdtDesc - _SmiEntryPoint + 8000h
    DB      2eh, 0a1h                   ; mov ax, cs:[offset16]
    DW      DSC_OFFSET + DSC_GDTSIZ
    dec     eax
    mov     cs:[edi], eax               ; mov cs:[bx], ax
    DB      66h, 2eh, 0a1h              ; mov eax, cs:[offset16]
    DW      DSC_OFFSET + DSC_GDTPTR
    mov     cs:[edi + 2], ax            ; mov cs:[bx + 2], eax
    mov     bp, ax                      ; ebp = GDT base
    DB      66h
    lgdt    fword ptr cs:[edi]          ; lgdt fword ptr cs:[bx]
; Patch ProtectedMode Segment
    DB      0b8h                        ; mov ax, imm16
    DW      PROTECT_MODE_CS             ; set AX for segment directly
    mov     cs:[edi - 2], eax           ; mov cs:[bx - 2], ax
; Patch ProtectedMode entry
    DB      66h, 0bfh                   ; mov edi, SMBASE
gSmbase    DD    ?
    DB      67h
    lea     ax, [edi + (@32bit - _SmiEntryPoint) + 8000h]
    mov     cs:[edi - 6], ax            ; mov cs:[bx - 6], eax
    mov     ebx, cr0
    DB      66h
    and     ebx, 9ffafff3h
    DB      66h
    or      ebx, 23h
    mov     cr0, ebx
    DB      66h, 0eah
    DD      ?
    DW      ?
_GdtDesc    FWORD   ?

@32bit:
    mov     ax, PROTECT_MODE_DS
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    DB      0bch                   ; mov esp, imm32
gSmiStack   DD      ?
    mov     eax, offset gSmiHandlerIdtr
    lidt    fword ptr [eax]
    jmp     ProtFlatMode

ProtFlatMode:
    DB      0b8h                        ; mov eax, imm32
gSmiCr3     DD      ?
    mov     cr3, eax
;
; Need to test for CR4 specific bit support
;
    mov     eax, 1
    cpuid                               ; use CPUID to determine if specific CR4 bits are supported
    xor     eax, eax                    ; Clear EAX
    test    edx, BIT2                   ; Check for DE capabilities
    jz      @f
    or      eax, BIT3
@@:
    test    edx, BIT6                   ; Check for PAE capabilities
    jz      @f
    or      eax, BIT5
@@:
    test    edx, BIT7                   ; Check for MCE capabilities
    jz      @f
    or      eax, BIT6
@@:
    test    edx, BIT24                  ; Check for FXSR capabilities
    jz      @f
    or      eax, BIT9
@@:
    test    edx, BIT25                  ; Check for SSE capabilities
    jz      @f
    or      eax, BIT10
@@:                                     ; as cr4.PGE is not set here, refresh cr3
    mov     cr4, eax                    ; in PreModifyMtrrs() to flush TLB.

    cmp     FeaturePcdGet (PcdCpuSmmStackGuard), 0
    jz      @F
; Load TSS
    mov     byte ptr [ebp + TSS_SEGMENT + 5], 89h ; clear busy flag
    mov     eax, TSS_SEGMENT
    ltr     ax
@@:

; enable NXE if supported
    DB      0b0h                        ; mov al, imm8
mXdSupported     DB      1
    cmp     al, 0
    jz      @SkipXd
;
; Check XD disable bit
;
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    push    edx                        ; save MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2                  ; MSR_IA32_MISC_ENABLE[34]
    jz      @f
    and     dx, 0FFFBh                 ; clear XD Disable bit if it is set
    wrmsr
@@:
    mov     ecx, MSR_EFER
    rdmsr
    or      ax, MSR_EFER_XD             ; enable NXE
    wrmsr
    jmp     @XdDone
@SkipXd:
    sub     esp, 4
@XdDone:

    mov     ebx, cr0
    or      ebx, 080010023h             ; enable paging + WP + NE + MP + PE
    mov     cr0, ebx
    lea     ebx, [edi + DSC_OFFSET]
    mov     ax, [ebx + DSC_DS]
    mov     ds, eax
    mov     ax, [ebx + DSC_OTHERSEG]
    mov     es, eax
    mov     fs, eax
    mov     gs, eax
    mov     ax, [ebx + DSC_SS]
    mov     ss, eax

;   jmp     _SmiHandler                 ; instruction is not needed

_SmiHandler PROC
    mov     ebx, [esp + 4]                  ; CPU Index
    push    ebx
    mov     eax, CpuSmmDebugEntry
    call    eax
    add     esp, 4

    push    ebx
    mov     eax, SmiRendezvous
    call    eax
    add     esp, 4

    push    ebx
    mov     eax, CpuSmmDebugExit
    call    eax
    add     esp, 4

    mov     eax, offset mXdSupported
    mov     al, [eax]
    cmp     al, 0
    jz      @f
    pop     edx                       ; get saved MSR_IA32_MISC_ENABLE[63-32]
    test    edx, BIT2
    jz      @f
    mov     ecx, MSR_IA32_MISC_ENABLE
    rdmsr
    or      dx, BIT2                  ; set XD Disable bit if it was set before entering into SMM
    wrmsr

@@:
    rsm
_SmiHandler ENDP

gcSmiHandlerSize    DW      $ - _SmiEntryPoint

    END
