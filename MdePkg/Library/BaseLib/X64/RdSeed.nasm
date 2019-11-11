;------------------------------------------------------------------------------
;
; Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   RdSeed.nasm
;
; Abstract:
;
;   Generates random seed through CPU RdSeed instruction under 64-bit platform.
;
; Notes:
;
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

;------------------------------------------------------------------------------
;  Generates a 16 bit random seed through RDSEED instruction.
;  Return TRUE if Seed generated successfully, or FALSE if not.
;
;  BOOLEAN EFIAPI InternalX86RdSeed16 (UINT16 *Seed);
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86RdSeed16)
ASM_PFX(InternalX86RdSeed16):
    ; rdseed   ax                  ; generate a 16 bit RN into eax,
                                   ; CF=1 if RN generated ok, otherwise CF=0
    db     0xf, 0xc7, 0xf8         ; rdseed r16: "0f c7 /7  ModRM:r/m(w)"
    jc     rn16_ok                 ; jmp if CF=1
    xor    rax, rax                ; reg=0 if CF=0
    pause
    ret                            ; return with failure status
rn16_ok:
    mov    [rcx], ax
    mov    rax,  1
    ret

;------------------------------------------------------------------------------
;  Generates a 32 bit random seed through RDSEED instruction.
;  Return TRUE if Seed generated successfully, or FALSE if not.
;
;  BOOLEAN EFIAPI InternalX86RdSeed32 (UINT32 *Seed);
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86RdSeed32)
ASM_PFX(InternalX86RdSeed32):
    ; rdseed   eax                 ; generate a 32 bit RN into eax,
                                   ; CF=1 if RN generated ok, otherwise CF=0
    db     0xf, 0xc7, 0xf8         ; rdseed r32: "0f c7 /7  ModRM:r/m(w)"
    jc     rn32_ok                 ; jmp if CF=1
    xor    rax, rax                ; reg=0 if CF=0
    pause
    ret                            ; return with failure status
rn32_ok:
    mov    [rcx], eax
    mov    rax,  1
    ret

;------------------------------------------------------------------------------
;  Generates a 64 bit random seed through one RDSEED instruction.
;  Return TRUE if Seed generated successfully, or FALSE if not.
;
;  BOOLEAN EFIAPI InternalX86RdSeed64 (UINT64 *Seed);
;------------------------------------------------------------------------------
global ASM_PFX(InternalX86RdSeed64)
ASM_PFX(InternalX86RdSeed64):
    ; rdseed   rax                 ; generate a 64 bit RN into rax,
                                   ; CF=1 if RN generated ok, otherwise CF=0
    db     0x48, 0xf, 0xc7, 0xf8   ; rdseed r64: "REX.W + 0f c7 /7 ModRM:r/m(w)"
    jc     rn64_ok                 ; jmp if CF=1
    xor    rax, rax                ; reg=0 if CF=0
    pause
    ret                            ; return with failure status
rn64_ok:
    mov    [rcx], rax
    mov    rax, 1
    ret

