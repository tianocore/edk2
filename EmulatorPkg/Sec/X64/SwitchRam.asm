;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
; Portions copyright (c) 2011, Apple Inc. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

EXTERN CopyMem:PROC
EXTERN ZeroMem:PROC

    .code

;------------------------------------------------------------------------------
;  EFI_STATUS
;  EFIAPI
;  SecTemporaryRamSupport (
;    IN CONST EFI_PEI_SERVICES   **PeiServices,         // %rcx
;    IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,   // %rdx
;    IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,   // %r8
;    IN UINTN                    CopySize               // %r9
;    )
;------------------------------------------------------------------------------
SecTemporaryRamSupport PROC
  ; Adjust callers %rbp to account for stack move
  sub     rbp, rdx      ; Calc offset of %rbp in Temp Memory
  add     rbp, r8       ; add in permanent base to offset

  push    rbp           ; stack frame is for the debugger
  mov     rbp, rsp

  push    rdx           ; Save TemporaryMemoryBase
  push    r8            ; Save PermanentMemoryBase
  push    r9            ; Save CopySize

  ;
  ; Copy all of temp RAM to permanent memory, including stack
  ;
  ; CopyMem (PermanentMemoryBase, TemporaryMemoryBase, CopySize);
  ;          %rcx,                %rdx,                %r8
  mov     rcx, r8       ; Shift arguments
  mov     r8, r9
  sub     rsp, 028h     ; Allocate register spill area & 16-byte align stack
  call    CopyMem
  ; Temp mem stack now copied to permanent location. %esp still in temp memory
  add     rsp, 028h

  pop     r9            ; CopySize (old stack)
  pop     r8            ; PermanentMemoryBase (old stack)
  pop     rdx           ; TemporaryMemoryBase (old stack)

  mov     rcx, rsp      ; Move to new stack
  sub     rcx, rdx      ; Calc offset of stack in Temp Memory
  add     rcx, r8       ; Calc PermanentMemoryBase address
  mov     rsp, rcx      ; Update stack
  ; Stack now points to permanent memory

  ; ZeroMem (TemporaryMemoryBase /* rcx */, CopySize /* rdx */);
  mov     rcx, rdx
  mov     rdx, r9
  sub     rsp, 028h     ; Allocate register spill area & 16-byte align stack
  call    ZeroMem
  add     rsp, 028h

  ; This data comes off the NEW stack
  pop     rbp
  ret
SecTemporaryRamSupport ENDP

  END
