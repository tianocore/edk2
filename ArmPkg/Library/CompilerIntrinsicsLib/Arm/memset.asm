//------------------------------------------------------------------------------
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
// Copyright (c) 2014, ARM Ltd. All rights reserved.<BR>
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------


    EXPORT  __aeabi_memset
    EXPORT  __aeabi_memclr
    EXPORT  __aeabi_memclr4

    AREA    Memset, CODE, READONLY

; void __aeabi_memclr4(void *dest, size_t n);
; void __aeabi_memclr(void *dest, size_t n);
__aeabi_memclr
__aeabi_memclr4
  mov   r2, #0

;
;VOID
;EFIAPI
;__aeabi_memset (
; IN  VOID    *Destination,
; IN  UINT32  Size,
; IN  UINT32  Character
; );
;
__aeabi_memset
  cmp  r1, #0
  bxeq lr
  ; args = 0, pretend = 0, frame = 0
  ; frame_needed = 1, uses_anonymous_args = 0
L10
  strb  r2, [r0], #1
  subs  r1, r1, #1
  ; While size is not 0
  bne  L10
  bx   lr

  END
