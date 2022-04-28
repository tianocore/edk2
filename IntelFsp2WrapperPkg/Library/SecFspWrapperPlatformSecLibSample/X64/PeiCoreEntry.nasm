;------------------------------------------------------------------------------
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;  PeiCoreEntry.nasm
;
; Abstract:
;
;   Find and call SecStartup
;
;------------------------------------------------------------------------------

SECTION .text

%include    "PushPopRegsNasm.inc"

extern ASM_PFX(SecStartup)
extern ASM_PFX(PlatformInit)

;
; args 1:XMM, 2:REG, 3:IDX
;
%macro LXMMN        3
            pextrq  %2, %1, (%3 & 3)
            %endmacro

;
; args 1:YMM, 2:XMM, 3:IDX (0 - lower 128bits, 1 - upper 128bits)
;
%macro LYMMN        3
            vextractf128  %2, %1, %3
            %endmacro

%macro LOAD_TS      1
            LYMMN   ymm6, xmm5, 1
            LXMMN   xmm5, %1, 1
            %endmacro

global ASM_PFX(CallPeiCoreEntryPoint)
ASM_PFX(CallPeiCoreEntryPoint):
  ;
  ; Per X64 calling convention, make sure RSP is 16-byte aligned.
  ;
  mov     rax, rsp
  and     rax, 0fh
  sub     rsp, rax

  ;
  ; Platform init
  ;
  PUSHA_64
  sub     rsp, 20h
  call    ASM_PFX(PlatformInit)
  add     rsp, 20h
  POPA_64

  ;
  ; Set stack top pointer
  ;
  mov     rsp, r8

  ;
  ; Push the hob list pointer
  ;
  push    rcx

  ;
  ; RBP holds start of BFV passed from Vtf0. Save it to r10.
  ;
  mov     r10, rbp

  ;
  ; Save the value
  ;   RDX: start of range
  ;   r8: end of range
  ;
  mov     rbp, rsp
  push    rdx
  push    r8
  mov     r14, rdx
  mov     r15, r8

  ;
  ; Push processor count to stack first, then BIST status (AP then BSP)
  ;
  mov     eax, 1
  cpuid
  shr     ebx, 16
  and     ebx, 0000000FFh
  cmp     bl, 1
  jae     PushProcessorCount

  ;
  ; Some processors report 0 logical processors.  Effectively 0 = 1.
  ; So we fix up the processor count
  ;
  inc     ebx

PushProcessorCount:
  sub     rsp, 4
  mov     rdi, rsp
  mov     DWORD [rdi], ebx

  ;
  ; We need to implement a long-term solution for BIST capture.  For now, we just copy BSP BIST
  ; for all processor threads
  ;
  xor     ecx, ecx
  mov     cl, bl
PushBist:
  sub     rsp, 4
  mov     rdi, rsp
  movd    eax, mm0
  mov     DWORD [rdi], eax
  loop    PushBist

  ; Save Time-Stamp Counter
  LOAD_TS rax
  push    rax

  ;
  ; Pass entry point of the PEI core
  ;
  mov     rdi, 0FFFFFFE0h
  mov     edi, DWORD [rdi]
  mov     r9, rdi

  ;
  ; Pass BFV into the PEI Core
  ;
  mov     r8, r10

  ;
  ; Pass stack size into the PEI Core
  ;
  mov     rcx, r15  ; Start of TempRam
  mov     rdx, r14  ; End of TempRam

  sub     rcx, rdx  ; Size of TempRam

  ;
  ; Pass Control into the PEI Core
  ;
  sub     rsp, 20h
  call ASM_PFX(SecStartup)

