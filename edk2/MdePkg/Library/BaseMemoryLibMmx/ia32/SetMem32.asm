;------------------------------------------------------------------------------
;
; Copyright (c) 2006, Intel Corporation
; All rights reserved. This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   SetMem32.asm
;
; Abstract:
;
;   SetMem32 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .686
    .model  flat,C
    .xmm
    .code

;------------------------------------------------------------------------------
;  VOID *
;  _mem_SetMem32 (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT32 Value
;    )
;------------------------------------------------------------------------------
InternalMemSetMem32 PROC    USES    edi
    mov     edx, [esp + 12]
    mov     edi, [esp + 8]
    mov     ecx, edx
    shr     ecx, 1
    movd    mm0, [esp + 16]
    mov     eax, edi
    jz      @SetDwords
    pshufw  mm0, mm0, 44h
@@:
    movntq  [edi], mm0
    add     edi, 8
    loop    @B
    mfence
@SetDwords:
    test    dl, 1
    jz      @F
    movd    [edi], mm0
@@:
    ret
InternalMemSetMem32 ENDP

    END
