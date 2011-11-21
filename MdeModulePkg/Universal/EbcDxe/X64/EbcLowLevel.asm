;/** @file
;  
;    This code provides low level routines that support the Virtual Machine.
;    for option ROMs.
;  
;  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
;  This program and the accompanying materials
;  are licensed and made available under the terms and conditions of the BSD License
;  which accompanies this distribution.  The full text of the license may be found at
;  http://opensource.org/licenses/bsd-license.php
;  
;  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;  
;**/

  page    ,132
  title   VM ASSEMBLY LANGUAGE ROUTINES

;---------------------------------------------------------------------------
; Equate files needed.
;---------------------------------------------------------------------------

.CODE

CopyMem  PROTO  Destination:PTR DWORD, Source:PTR DWORD, Count:DWORD

;****************************************************************************
; EbcLLCALLEX
;
; This function is called to execute an EBC CALLEX instruction.
; This instruction requires that we thunk out to external native
; code. For x64, we switch stacks, copy the arguments to the stack
; and jump to the specified function.
; On return, we restore the stack pointer to its original location.
;
; Destroys no working registers.
;****************************************************************************
; INT64 EbcLLCALLEXNative(UINTN FuncAddr, UINTN NewStackPointer, VOID *FramePtr)
EbcLLCALLEXNative        PROC    PUBLIC
      push   rbp
      push   rbx
      mov    rbp, rsp
      ; Function prolog

      ; Copy FuncAddr to a preserved register.
      mov    rbx, rcx

      ; Set stack pointer to new value
      sub    r8,  rdx
      sub    rsp, r8
      mov    rcx, rsp
      sub    rsp, 20h
      call   CopyMem
      add    rsp, 20h

      ; Considering the worst case, load 4 potiential arguments
      ; into registers.
      mov    rcx, qword ptr [rsp]
      mov    rdx, qword ptr [rsp+8h]
      mov    r8,  qword ptr [rsp+10h]
      mov    r9,  qword ptr [rsp+18h]

      ; Now call the external routine
      call  rbx

      ; Function epilog
      mov      rsp, rbp
      pop      rbx
      pop      rbp
      ret
EbcLLCALLEXNative    ENDP


; UINTN EbcLLGetEbcEntryPoint(VOID);
; Routine Description:
;   The VM thunk code stuffs an EBC entry point into a processor
;   register. Since we can't use inline assembly to get it from
;   the interpreter C code, stuff it into the return value
;   register and return.
;
; Arguments:
;     None.
;
; Returns:
;     The contents of the register in which the entry point is passed.
;
EbcLLGetEbcEntryPoint        PROC    PUBLIC
    ; The EbcEntryPoint is saved to R10.
    mov  rax, r10
    ret
EbcLLGetEbcEntryPoint    ENDP

END

