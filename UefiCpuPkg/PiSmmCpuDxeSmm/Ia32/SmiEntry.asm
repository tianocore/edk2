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
    mov     ebx, cr0
    or      ebx, 080010000h             ; enable paging + WP
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

    cmp     FeaturePcdGet (PcdCpuSmmStackGuard), 0
    jz      @F

; Load TSS
    mov     byte ptr [ebp + TSS_SEGMENT + 5], 89h ; clear busy flag
    mov     eax, TSS_SEGMENT
    ltr     ax
@@:
;   jmp     _SmiHandler                 ; instruction is not needed

_SmiHandler PROC
    mov     ebx, [esp]                  ; CPU Index

    push    ebx
    mov     eax, CpuSmmDebugEntry
    call    eax
    pop     ecx

    push    ebx
    mov     eax, SmiRendezvous
    call    eax
    pop     ecx
    
    push    ebx
    mov     eax, CpuSmmDebugExit
    call    eax
    pop     ecx

    rsm
_SmiHandler ENDP

gcSmiHandlerSize    DW      $ - _SmiEntryPoint

    END
