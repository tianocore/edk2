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
.model  flat, C
.code
CopyMem  PROTO  Destination:PTR DWORD, Source:PTR DWORD, Count:DWORD
EbcInterpret               PROTO
ExecuteEbcImageEntryPoint  PROTO

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
EbcLLCALLEXNative        PROC        PUBLIC
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
EbcLLCALLEXNative    ENDP

;****************************************************************************
; EbcLLEbcInterpret
;
; Begin executing an EBC image.
;****************************************************************************
; UINT64 EbcLLEbcInterpret(VOID)
EbcLLEbcInterpret PROC PUBLIC
    ;
    ;; mov eax, 0xca112ebc
    ;; mov eax, EbcEntryPoint
    ;; mov ecx, EbcLLEbcInterpret
    ;; jmp ecx
    ;
    ; Caller uses above instruction to jump here
    ; The stack is below:
    ; +-----------+
    ; |  RetAddr  |
    ; +-----------+
    ; |EntryPoint | (EAX)
    ; +-----------+
    ; |   Arg1    | <- EDI
    ; +-----------+
    ; |   Arg2    |
    ; +-----------+
    ; |   ...     |
    ; +-----------+
    ; |   Arg16   |
    ; +-----------+
    ; |   EDI     |
    ; +-----------+
    ; |   ESI     |
    ; +-----------+
    ; |   EBP     | <- EBP
    ; +-----------+
    ; |  RetAddr  | <- ESP is here
    ; +-----------+
    ; |   Arg1    | <- ESI
    ; +-----------+
    ; |   Arg2    |
    ; +-----------+
    ; |   ...     |
    ; +-----------+
    ; |   Arg16   |
    ; +-----------+
    ; 

    ; Construct new stack
    push ebp
    mov  ebp, esp
    push esi
    push edi
    sub  esp, 40h
    push eax
    mov  esi, ebp
    add  esi, 8
    mov  edi, esp
    add  edi, 4
    mov  ecx, 16
    rep  movsd
    
    ; call C-code
    call EbcInterpret
    add  esp, 44h
    pop  edi
    pop  esi
    pop  ebp
    ret
EbcLLEbcInterpret ENDP

;****************************************************************************
; EbcLLExecuteEbcImageEntryPoint
;
; Begin executing an EBC image.
;****************************************************************************
; UINT64 EbcLLExecuteEbcImageEntryPoint(VOID)
EbcLLExecuteEbcImageEntryPoint PROC PUBLIC
    ;
    ;; mov eax, 0xca112ebc
    ;; mov eax, EbcEntryPoint
    ;; mov ecx, EbcLLExecuteEbcImageEntryPoint
    ;; jmp ecx
    ;
    ; Caller uses above instruction to jump here
    ; The stack is below:
    ; +-----------+
    ; |  RetAddr  |
    ; +-----------+
    ; |EntryPoint | (EAX)
    ; +-----------+
    ; |ImageHandle|
    ; +-----------+
    ; |SystemTable|
    ; +-----------+
    ; |  RetAddr  | <- ESP is here
    ; +-----------+
    ; |ImageHandle|
    ; +-----------+
    ; |SystemTable|
    ; +-----------+
    ; 
    
    ; Construct new stack
    mov  [esp - 0Ch], eax
    mov  eax, [esp + 04h]
    mov  [esp - 08h], eax
    mov  eax, [esp + 08h]
    mov  [esp - 04h], eax
    
    ; call C-code
    sub  esp, 0Ch
    call ExecuteEbcImageEntryPoint
    add  esp, 0Ch
    ret
EbcLLExecuteEbcImageEntryPoint ENDP

END
