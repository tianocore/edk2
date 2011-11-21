;/** @file
;  
;    This code provides low level routines that support the Virtual Machine
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

.XLIST

.LIST

;---------------------------------------------------------------------------
; Assembler options
;---------------------------------------------------------------------------

.686p
.model  flat
.code
CopyMem  PROTO  C Destination:PTR DWORD, Source:PTR DWORD, Count:DWORD

;****************************************************************************
; EbcLLCALLEXNative
;
; This function is called to execute an EBC CALLEX instruction
; to native code.
; This instruction requires that we thunk out to external native
; code. For IA32, we simply switch stacks and jump to the
; specified function. On return, we restore the stack pointer
; to its original location.
;
; Destroys no working registers.
;****************************************************************************
; INT64 EbcLLCALLEXNative(UINTN FuncAddr, UINTN NewStackPointer, VOID *FramePtr)
_EbcLLCALLEXNative        PROC        PUBLIC
      push   ebp
      push   ebx
      mov    ebp, esp              ; standard function prolog

      ; Get function address in a register
      ; mov ecx, FuncAddr => mov ecx, dword ptr [FuncAddr]
      mov    ecx, dword ptr [esp]+0Ch

      ; Set stack pointer to new value
      ; mov eax, NewStackPointer => mov eax, dword ptr [NewSp]
      mov    eax, dword ptr [esp] + 14h
      mov    edx, dword ptr [esp] + 10h
      sub    eax, edx
      sub    esp, eax
      mov    ebx, esp
      push   ecx
      push   eax
      push   edx
      push   ebx
      call   CopyMem
      pop    eax
      pop    eax
      pop    eax
      pop    ecx

      ; Now call the external routine
      call  ecx

      ; ebp is preserved by the callee. In this function it
      ; equals the original esp, so set them equal
      mov    esp, ebp

      ; Standard function epilog
      mov      esp, ebp
      pop      ebx
      pop      ebp
      ret
_EbcLLCALLEXNative    ENDP


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
_EbcLLGetEbcEntryPoint        PROC        PUBLIC
    ; The EbcEntryPoint is saved to EAX, so just return here.
    ret
_EbcLLGetEbcEntryPoint    ENDP

END
