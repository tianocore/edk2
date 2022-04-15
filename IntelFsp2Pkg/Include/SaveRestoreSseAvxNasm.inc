;------------------------------------------------------------------------------
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   Provide macro for register save/restore using SSE registers
;
;------------------------------------------------------------------------------

;
; Define SSE and AVX instruction set
;
;
; Define SSE macros using SSE 4.1 instructions
; args 1:XMM, 2:IDX, 3:REG
;
%macro SXMMN        3
            pinsrq  %1, %3, (%2 & 3)
            %endmacro

;
; args 1:XMM, 2:REG, 3:IDX
;
%macro LXMMN        3
            pextrq  %2, %1, (%3 & 3)
            %endmacro

;
; Define AVX macros using AVX instructions
; Save XMM to YMM
; args 1:YMM, 2:IDX (0 - lower 128bits, 1 - upper 128bits), 3:XMM
;
%macro SYMMN        3
            vinsertf128  %1, %1, %3, %2
            %endmacro

;
; Restore XMM from YMM
; args 1:YMM, 2:XMM, 3:IDX (0 - lower 128bits, 1 - upper 128bits)
;
%macro LYMMN        3
            vextractf128  %2, %1, %3
            %endmacro

;
; Upper half of YMM7 to save RBP and RBX. Upper half of YMM8 to save RSI and RDI.
; Modified: XMM5, YMM6, YMM7 and YMM8
;
%macro SAVE_REGS    0
            SXMMN   xmm5, 0, rbp
            SXMMN   xmm5, 1, rbx
            SYMMN   ymm7, 1, xmm5
            SXMMN   xmm5, 0, rsi
            SXMMN   xmm5, 1, rdi
            SYMMN   ymm8, 1, xmm5
            SAVE_RSP
            %endmacro

;
; Upper half of YMM7 to restore RBP and RBX. Upper half of YMM8 to restore RSI and RDI.
; Modified: XMM5, RBP, RBX, RSI, RDI and RSP
;
%macro LOAD_REGS    0
            LYMMN   ymm7, xmm5, 1
            LXMMN   xmm5, rbp, 0
            LXMMN   xmm5, rbx, 1
            LYMMN   ymm8, xmm5, 1
            LXMMN   xmm5, rsi, 0
            LXMMN   xmm5, rdi, 1
            LOAD_RSP
            %endmacro
;
; Restore RBP from YMM7[128:191]
; Modified: XMM5 and RBP
;
%macro LOAD_RBP     0
            LYMMN   ymm7, xmm5, 1
            movq    rbp,  xmm5
            %endmacro

;
; Restore RBX from YMM7[192:255]
; Modified: XMM5 and RBX
;
%macro LOAD_RBX     0
            LYMMN   ymm7, xmm5, 1
            LXMMN   xmm5, rbx, 1
            %endmacro

;
; Upper half of YMM6 to save/restore Time Stamp, RSP
;
;
; Save Time Stamp to YMM6[192:255]
; arg 1:general purpose register which holds time stamp
; Modified: XMM5 and YMM6
;
%macro SAVE_TS      1
            LYMMN   ymm6, xmm5, 1
            SXMMN   xmm5, 1, %1
            SYMMN   ymm6, 1, xmm5
            %endmacro

;
; Restore Time Stamp from YMM6[192:255]
; arg 1:general purpose register where to save time stamp
; Modified: XMM5 and %1
;
%macro LOAD_TS      1
            LYMMN   ymm6, xmm5, 1
            LXMMN   xmm5, %1, 1
            %endmacro

;
; Save RSP to YMM6[128:191]
; Modified: XMM5 and YMM6
;
%macro SAVE_RSP     0
            LYMMN   ymm6, xmm5, 1
            SXMMN   xmm5, 0, rsp
            SYMMN   ymm6, 1, xmm5
            %endmacro

;
; Restore RSP from YMM6[128:191]
; Modified: XMM5 and RSP
;
%macro LOAD_RSP     0
            LYMMN   ymm6, xmm5, 1
            movq    rsp,  xmm5
            %endmacro

;
; Upper half of YMM9 to save/restore UCODE status, BFV address
;
;
; Save uCode status to YMM9[192:255]
; arg 1:general purpose register which holds uCode status
; Modified: XMM5 and YMM9
;
%macro SAVE_UCODE_STATUS     1
            LYMMN   ymm9, xmm5, 1
            SXMMN   xmm5, 0, %1
            SYMMN   ymm9, 1, xmm5
            %endmacro

;
; Restore uCode status from YMM9[192:255]
; arg 1:general purpose register where to save uCode status
; Modified: XMM5 and %1
;
%macro LOAD_UCODE_STATUS     1
            LYMMN   ymm9, xmm5, 1
            movq    %1,  xmm5
            %endmacro

;
; Save BFV address to YMM9[128:191]
; arg 1:general purpose register which holds BFV address
; Modified: XMM5 and YMM9
;
%macro SAVE_BFV     1
            LYMMN   ymm9, xmm5, 1
            SXMMN   xmm5, 1, %1
            SYMMN   ymm9, 1, xmm5
            %endmacro

;
; Restore BFV address from YMM9[128:191]
; arg 1:general purpose register where to save BFV address
; Modified: XMM5 and %1
;
%macro LOAD_BFV     1
            LYMMN   ymm9, xmm5, 1
            LXMMN   xmm5, %1, 1
            %endmacro

;
; YMM7[128:191] for calling stack
; arg 1:Entry
; Modified: RSI, XMM5, YMM7
;
%macro CALL_YMM     1
            mov     rsi, %%ReturnAddress
            LYMMN   ymm7, xmm5, 1
            SXMMN   xmm5, 0, rsi
            SYMMN   ymm7, 1, xmm5
            mov     rsi,  %1
            jmp     rsi
%%ReturnAddress:
            %endmacro
;
; Restore RIP from YMM7[128:191]
; Modified: RSI, XMM5
;
%macro RET_YMM      0
            LYMMN   ymm7, xmm5, 1
            movq    rsi, xmm5
            jmp     rsi
            %endmacro

%macro ENABLE_SSE   0
            ;
            ; Initialize floating point units
            ;
            jmp     NextAddress
align 4
            ;
            ; Float control word initial value:
            ; all exceptions masked, double-precision, round-to-nearest
            ;
FpuControlWord       DW      027Fh
            ;
            ; Multimedia-extensions control word:
            ; all exceptions masked, round-to-nearest, flush to zero for masked underflow
            ;
MmxControlWord       DQ      01F80h
SseError:
            ;
            ; Processor has to support SSE
            ;
            jmp     SseError
NextAddress:
            finit
            mov rax, FpuControlWord
            fldcw [rax]

            ;
            ; Use CpuId instruction (CPUID.01H:EDX.SSE[bit 25] = 1) to test
            ; whether the processor supports SSE instruction.
            ;
            mov     rax, 1
            cpuid
            bt      rdx, 25
            jnc     SseError

            ;
            ; SSE 4.1 support
            ;
            bt      ecx, 19
            jnc     SseError

            ;
            ; Set OSFXSR bit (bit #9) & OSXMMEXCPT bit (bit #10)
            ;
            mov     rax, cr4
            or      rax, 00000600h
            mov     cr4, rax

            ;
            ; The processor should support SSE instruction and we can use
            ; ldmxcsr instruction
            ;
            mov  rax, MmxControlWord
            ldmxcsr [rax]
            %endmacro

%macro ENABLE_AVX   0
            mov     eax, 1
            cpuid
            and     ecx, 10000000h
            cmp     ecx, 10000000h ; check AVX feature flag
            je      EnableAvx
AvxError:
            ;
            ; Processor has to support AVX
            ;
            jmp     AvxError
EnableAvx:
            ;
            ; Set OSXSAVE bit (bit #18) to enable xgetbv/xsetbv instruction
            ;
            mov     rax, cr4
            or      rax, 00040000h
            mov     cr4, rax

            mov     rcx, 0         ; index 0
            xgetbv                 ; result in edx:eax
            or      eax, 00000006h ; Set XCR0 bit #1 and bit #2 to enable SSE state and AVX state
            xsetbv
            %endmacro

