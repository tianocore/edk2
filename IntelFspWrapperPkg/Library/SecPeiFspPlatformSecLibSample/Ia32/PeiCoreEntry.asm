;------------------------------------------------------------------------------
;
; Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;  PeiCoreEntry.asm
;
; Abstract:
;
;   Find and call SecStartup
;
;------------------------------------------------------------------------------

.686p
.xmm
.model flat, c
.code

EXTRN   SecStartup:NEAR
EXTRN   PlatformInit:NEAR

CallPeiCoreEntryPoint   PROC PUBLIC
  ;
  ; Obtain the hob list pointer
  ;
  mov     eax, [esp+4]
  ;
  ; Obtain the stack information
  ;   ECX: start of range
  ;   EDX: end of range
  ;
  mov     ecx, [esp+8]
  mov     edx, [esp+0Ch]

  ;
  ; Platform init
  ;
  pushad
  push edx
  push ecx
  push eax
  call PlatformInit
  pop  eax
  pop  eax
  pop  eax
  popad

  ;
  ; Set stack top pointer
  ;
  mov     esp, edx

  ;
  ; Push the hob list pointer
  ;
  push    eax

  ;
  ; Save the value
  ;   ECX: start of range
  ;   EDX: end of range
  ;
  mov     ebp, esp
  push    ecx
  push    edx

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
  push    ebx

  ;
  ; We need to implement a long-term solution for BIST capture.  For now, we just copy BSP BIST
  ; for all processor threads
  ;
  xor     ecx, ecx
  mov     cl, bl
PushBist:
  movd    eax, mm0
  push    eax
  loop    PushBist

  ; Save Time-Stamp Counter
  movd eax, mm5
  push eax

  movd eax, mm6
  push eax

  ;
  ; Pass entry point of the PEI core
  ;
  mov     edi, 0FFFFFFE0h
  push    DWORD PTR ds:[edi]

  ;
  ; Pass BFV into the PEI Core
  ;
  mov     edi, 0FFFFFFFCh
  push    DWORD PTR ds:[edi]

  ;
  ; Pass stack size into the PEI Core
  ;
  mov     ecx, [ebp - 4]
  mov     edx, [ebp - 8]
  push    ecx       ; RamBase

  sub     edx, ecx
  push    edx       ; RamSize

  ;
  ; Pass Control into the PEI Core
  ;
  call SecStartup
CallPeiCoreEntryPoint   ENDP

END
