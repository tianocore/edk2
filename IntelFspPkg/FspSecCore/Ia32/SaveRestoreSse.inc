;------------------------------------------------------------------------------
;
; Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Abstract:
;
;   Provide macro for register save/restore using SSE registers
;
;------------------------------------------------------------------------------

;
; Define SSE instruction set
;
IFDEF USE_SSE41_FLAG
;
; Define SSE macros using SSE 4.1 instructions
;
SXMMN        MACRO   XMM, IDX, REG
             pinsrd  XMM, REG, (IDX AND 3)
             ENDM

LXMMN        MACRO   XMM, REG, IDX
             pextrd  REG, XMM, (IDX AND 3)
             ENDM
ELSE
;
; Define SSE macros using SSE 2 instructions
;
SXMMN        MACRO   XMM, IDX, REG
             pinsrw  XMM, REG, (IDX AND 3) * 2
             ror     REG, 16
             pinsrw  XMM, REG, (IDX AND 3) * 2 + 1
             rol     REG, 16
             ENDM

LXMMN        MACRO   XMM, REG, IDX
             pshufd  XMM, XMM,  (0E4E4E4h SHR (IDX * 2))  AND 0FFh
             movd    REG, XMM
             pshufd  XMM, XMM,  (0E4E4E4h SHR (IDX * 2 + (IDX AND 1) * 4)) AND 0FFh
             ENDM
ENDIF

;
; XMM7 to save/restore EBP, EBX, ESI, EDI
; 
SAVE_REGS    MACRO
  SXMMN      xmm7, 0, ebp
  SXMMN      xmm7, 1, ebx
  SXMMN      xmm7, 2, esi
  SXMMN      xmm7, 3, edi
  SAVE_ESP
             ENDM

LOAD_REGS    MACRO
  LXMMN      xmm7, ebp, 0
  LXMMN      xmm7, ebx, 1
  LXMMN      xmm7, esi, 2
  LXMMN      xmm7, edi, 3
  LOAD_ESP
             ENDM

;
; XMM6 to save/restore EAX, EDX, ECX, ESP
; 
LOAD_EAX     MACRO
  LXMMN      xmm6, eax, 1
             ENDM

SAVE_EAX     MACRO
  SXMMN      xmm6, 1, eax
             ENDM

LOAD_EDX     MACRO
  LXMMN      xmm6, edx, 2
             ENDM

SAVE_EDX     MACRO
  SXMMN      xmm6, 2, edx
             ENDM

SAVE_ECX     MACRO
  SXMMN      xmm6, 3, ecx
             ENDM

LOAD_ECX     MACRO
  LXMMN      xmm6, ecx, 3
             ENDM

SAVE_ESP     MACRO
  SXMMN      xmm6, 0, esp
             ENDM

LOAD_ESP     MACRO
  movd       esp,  xmm6
             ENDM
             
;
; XMM5 for calling stack
;
CALL_XMM     MACRO  Entry
             local   ReturnAddress
             mov     esi, offset ReturnAddress
             pslldq  xmm5, 4
IFDEF USE_SSE41_FLAG
             pinsrd  xmm5, esi, 0
ELSE             
             pinsrw  xmm5, esi, 0
             ror     esi,  16
             pinsrw  xmm5, esi, 1                        
ENDIF             
             mov     esi,  Entry
             jmp     esi
ReturnAddress:             
             ENDM
            
RET_XMM      MACRO               
             movd    esi, xmm5
             psrldq  xmm5, 4
             jmp     esi
             ENDM
             
ENABLE_SSE   MACRO
            ;
            ; Initialize floating point units
            ;
            local   NextAddress            
            jmp     NextAddress
ALIGN 4
            ;
            ; Float control word initial value:
            ; all exceptions masked, double-precision, round-to-nearest
            ;
FpuControlWord       DW      027Fh
            ;
            ; Multimedia-extensions control word:
            ; all exceptions masked, round-to-nearest, flush to zero for masked underflow
            ;
MmxControlWord       DD      01F80h 
SseError:      
            ;
            ; Processor has to support SSE
            ;
            jmp     SseError      
NextAddress:            
            finit
            fldcw   FpuControlWord

            ;
            ; Use CpuId instructuion (CPUID.01H:EDX.SSE[bit 25] = 1) to test
            ; whether the processor supports SSE instruction.
            ;
            mov     eax, 1
            cpuid
            bt      edx, 25
            jnc     SseError

IFDEF USE_SSE41_FLAG
            ;
            ; SSE 4.1 support
            ;
            bt      ecx, 19   
            jnc     SseError
ENDIF

            ;
            ; Set OSFXSR bit (bit #9) & OSXMMEXCPT bit (bit #10)
            ;
            mov     eax, cr4
            or      eax, 00000600h
            mov     cr4, eax

            ;
            ; The processor should support SSE instruction and we can use
            ; ldmxcsr instruction
            ;
            ldmxcsr MmxControlWord
            ENDM
