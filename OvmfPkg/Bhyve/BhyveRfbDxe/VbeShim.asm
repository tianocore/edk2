;------------------------------------------------------------------------------
; @file
; A minimal Int10h stub that allows the Windows 2008 R2 SP1 UEFI guest's buggy,
; default VGA driver to switch to 1024x768x32.
;
; Copyright (C) 2020, Rebecca Cran <rebecca@bsdio.com>
; Copyright (C) 2015, Nahanni Systems, Inc.
; Copyright (C) 2014, Red Hat, Inc.
; Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

; enable this macro for debug messages
%define DEBUG

%macro DebugLog 1
%ifdef DEBUG
  push       si
  mov        si, %1
  call       PrintStringSi
  pop        si
%endif
%endmacro


BITS 16
ORG 0

VbeInfo:
TIMES 256 nop

VbeModeInfo:
VbeMode1:
TIMES 50  nop
VbeMode2:
TIMES 50  nop
VbeMode3:
TIMES 50  nop
VbeMode4:
TIMES 50  nop
TIMES 56  nop  ; filler for 256 bytes

Handler:
  cmp        ax, 0x4f00
  je         GetInfo
  cmp        ax, 0x4f01
  je         GetModeInfo
  cmp        ax, 0x4f02
  je         SetMode
  cmp        ax, 0x4f03
  je         GetMode
  cmp        ax, 0x4f10
  je         GetPmCapabilities
  cmp        ax, 0x4f15
  je         ReadEdid
  cmp        ah, 0x00
  je         SetModeLegacy
  DebugLog   StrUnkownFunction
Hang:
  jmp        Hang


GetInfo:
  push       es
  push       di
  push       ds
  push       si
  push       cx

  DebugLog   StrEnterGetInfo

  ; target (es:di) set on input
  push       cs
  pop        ds
  mov        si, VbeInfo
  ; source (ds:si) set now

  mov        cx, 256
  cld
  rep movsb

  pop        cx
  pop        si
  pop        ds
  pop        di
  pop        es
  jmp        Success


GetModeInfo:
  push       es
  push       di
  push       ds
  push       si
  push       cx

  DebugLog   StrEnterGetModeInfo

  and        cx, ~0x4000 ; clear potentially set LFB bit in mode number

  cmp        cx, 0x013f
  je         gKnownMode1
  cmp        cx, 0x0140
  je         gKnownMode2
  cmp        cx, 0x0141
  je         gKnownMode3

  DebugLog   StrUnkownMode
  jmp        Hang
gKnownMode1:
  DebugLog   StrMode1
  mov        si, VbeMode1
  jmp        CopyModeInfo
gKnownMode2:
  DebugLog   StrMode2
  mov        si, VbeMode2
  jmp        CopyModeInfo
gKnownMode3:
  DebugLog   StrMode3
  mov        si, VbeMode3
  jmp        CopyModeInfo
gKnownMode4:
  DebugLog   StrMode4
  mov        si, VbeMode4
  jmp        CopyModeInfo

CopyModeInfo:
  ; target (es:di) set on input
  push       cs
  pop        ds
  ;mov        si, VbeModeInfo
  ; source (ds:si) set now

  ;mov        cx, 256
  mov        cx, 50
  cld
  rep movsb

  pop        cx
  pop        si
  pop        ds
  pop        di
  pop        es
  jmp        Success


SetMode:
  push       dx
  push       ax

  DebugLog   StrEnterSetMode

  and        bx, ~0x4000 ; clear potentially set LFB bit in mode number
  cmp        bx, 0x013f
  je         KnownMode1
  cmp        bx, 0x0140
  je         KnownMode2
  cmp        bx, 0x0141
  je         KnownMode3
  DebugLog   StrUnkownMode
  jmp        Hang
KnownMode1:
  DebugLog   StrMode1
  jmp        SetModeDone
KnownMode2:
  DebugLog   StrMode2
  jmp        SetModeDone
KnownMode3:
  DebugLog   StrMode3
  jmp        SetModeDone
KnownMode4:
  DebugLog   StrMode4

SetModeDone:
  mov        [CurMode], bl
  mov        [CurMode+1], bh
  pop        ax
  pop        dx
  jmp        Success


GetMode:
  DebugLog   StrEnterGetMode
  mov        bl, [CurMode]
  mov        bh, [CurMode+1]
  jmp        Success


GetPmCapabilities:
  DebugLog   StrGetPmCapabilities
  mov        bx, 0x0080
  jmp        Success


ReadEdid:
  push       es
  push       di
  push       ds
  push       si
  push       cx

  DebugLog   StrReadEdid

  ; target (es:di) set on input
  push       cs
  pop        ds
  mov        si, Edid
  ; source (ds:si) set now

  mov        cx, 128
  cld
  rep movsb

  pop        cx
  pop        si
  pop        ds
  pop        di
  pop        es
  jmp        Success


SetModeLegacy:
  DebugLog   StrEnterSetModeLegacy

  cmp        al, 0x03
  je         sKnownMode3
  cmp        al, 0x12
  je         sKnownMode4
  DebugLog   StrUnkownMode
  jmp        Hang
sKnownMode3:
  DebugLog   StrLegacyMode3
  mov        al, 0 ; 0x30
  jmp        SetModeLegacyDone
sKnownMode4:
  mov        al, 0 ;0x20
SetModeLegacyDone:
  DebugLog   StrExitSuccess
  iret


Success:
  DebugLog   StrExitSuccess
  mov        ax, 0x004f
  iret


Unsupported:
  DebugLog   StrExitUnsupported
  mov        ax, 0x024f
  iret


%ifdef DEBUG
PrintStringSi:
  pusha
  push       ds ; save original
  push       cs
  pop        ds
  mov        dx, 0x220             ; bhyve debug cons port
  mov        ax, 0
PrintStringSiLoop:
  lodsb
  cmp        al, 0
  je         PrintStringSiDone
  out        dx, al
  jmp        PrintStringSiLoop
PrintStringSiDone:
  pop        ds ; restore original
  popa
  ret


StrExitSuccess:
  db 'vOk', 0x0d, 0x0a, 0

StrExitUnsupported:
  db 'vUnsupported', 0x0d, 0x0a, 0

StrUnkownFunction:
  db 'vUnknown Function', 0x0d, 0x0a, 0

StrEnterGetInfo:
  db 'vGetInfo', 0x0d, 0x0a, 0

StrEnterGetModeInfo:
  db 'vGetModeInfo', 0x0d, 0x0a, 0

StrEnterGetMode:
  db 'vGetMode', 0x0d, 0x0a, 0

StrEnterSetMode:
  db 'vSetMode', 0x0d, 0x0a, 0

StrEnterSetModeLegacy:
  db 'vSetModeLegacy', 0x0d, 0x0a, 0

StrUnkownMode:
  db 'vUnkown Mode', 0x0d, 0x0a, 0

StrGetPmCapabilities:
  db 'vGetPmCapabilities', 0x0d, 0x0a, 0

StrReadEdid:
  db 'vReadEdid', 0x0d, 0x0a, 0

StrLegacyMode3:
  db 'vLegacyMode3', 0x0d, 0x0a, 0


StrMode1:
  db 'mode_640x480x32', 0x0d, 0x0a, 0
StrMode2:
  db 'mode_800x600x32', 0x0d, 0x0a, 0
StrMode3:
  db 'mode_1024x768x32', 0x0d, 0x0a, 0
StrMode4:
  db 'mode_unused', 0x0d, 0x0a, 0
%endif

CurMode:
  db 0x00, 0x00

;
; EDID stores monitor information. For now, just send back an null item.
;
Edid:
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
