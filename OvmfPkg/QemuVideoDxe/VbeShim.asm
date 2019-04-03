;------------------------------------------------------------------------------
; @file
; A minimal Int10h stub that allows the Windows 2008 R2 SP1 UEFI guest's buggy,
; default VGA driver to switch to 1024x768x32, on the stdvga and QXL video
; cards of QEMU.
;
; Copyright (C) 2014, Red Hat, Inc.
; Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

; enable this macro for debug messages
;%define DEBUG

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
TIMES 256 nop


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
  cmp        cx, 0x00f1
  je         KnownMode1
  DebugLog   StrUnkownMode
  jmp        Hang
KnownMode1:
  ; target (es:di) set on input
  push       cs
  pop        ds
  mov        si, VbeModeInfo
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


%define ATT_ADDRESS_REGISTER   0x03c0
%define VBE_DISPI_IOPORT_INDEX 0x01ce
%define VBE_DISPI_IOPORT_DATA  0x01d0

%define VBE_DISPI_INDEX_XRES        0x1
%define VBE_DISPI_INDEX_YRES        0x2
%define VBE_DISPI_INDEX_BPP         0x3
%define VBE_DISPI_INDEX_ENABLE      0x4
%define VBE_DISPI_INDEX_BANK        0x5
%define VBE_DISPI_INDEX_VIRT_WIDTH  0x6
%define VBE_DISPI_INDEX_VIRT_HEIGHT 0x7
%define VBE_DISPI_INDEX_X_OFFSET    0x8
%define VBE_DISPI_INDEX_Y_OFFSET    0x9

%define VBE_DISPI_ENABLED     0x01
%define VBE_DISPI_LFB_ENABLED 0x40

%macro BochsWrite 2
  push       dx
  push       ax

  mov        dx, VBE_DISPI_IOPORT_INDEX
  mov        ax, %1
  out        dx, ax

  mov        dx, VBE_DISPI_IOPORT_DATA
  mov        ax, %2
  out        dx, ax

  pop        ax
  pop        dx
%endmacro

SetMode:
  push       dx
  push       ax

  DebugLog   StrEnterSetMode

  cmp        bx, 0x40f1
  je         KnownMode2
  DebugLog   StrUnkownMode
  jmp        Hang
KnownMode2:

  ; unblank
  mov        dx, ATT_ADDRESS_REGISTER
  mov        al, 0x20
  out        dx, al

  BochsWrite VBE_DISPI_INDEX_ENABLE,        0
  BochsWrite VBE_DISPI_INDEX_BANK,          0
  BochsWrite VBE_DISPI_INDEX_X_OFFSET,      0
  BochsWrite VBE_DISPI_INDEX_Y_OFFSET,      0
  BochsWrite VBE_DISPI_INDEX_BPP,          32
  BochsWrite VBE_DISPI_INDEX_XRES,       1024
  BochsWrite VBE_DISPI_INDEX_VIRT_WIDTH, 1024
  BochsWrite VBE_DISPI_INDEX_YRES,        768
  BochsWrite VBE_DISPI_INDEX_VIRT_HEIGHT, 768
  BochsWrite VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED

  pop        ax
  pop        dx
  jmp        Success


GetMode:
  DebugLog   StrEnterGetMode
  mov        bx, 0x40f1
  jmp        Success


GetPmCapabilities:
  DebugLog   StrGetPmCapabilities
  jmp        Unsupported


ReadEdid:
  DebugLog   StrReadEdid
  jmp        Unsupported


SetModeLegacy:
  DebugLog   StrEnterSetModeLegacy

  cmp        al, 0x03
  je         KnownMode3
  cmp        al, 0x12
  je         KnownMode4
  DebugLog   StrUnkownMode
  jmp        Hang
KnownMode3:
  mov        al, 0x30
  jmp        SetModeLegacyDone
KnownMode4:
  mov        al, 0x20
SetModeLegacyDone:
  DebugLog   StrExitSuccess
  iret


Success:
  DebugLog   StrExitSuccess
  mov        ax, 0x004f
  iret


Unsupported:
  DebugLog   StrExitUnsupported
  mov        ax, 0x014f
  iret


%ifdef DEBUG
PrintStringSi:
  pusha
  push       ds ; save original
  push       cs
  pop        ds
  mov        dx, 0x0402
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
  db 'Exit', 0x0a, 0

StrExitUnsupported:
  db 'Unsupported', 0x0a, 0

StrUnkownFunction:
  db 'Unknown Function', 0x0a, 0

StrEnterGetInfo:
  db 'GetInfo', 0x0a, 0

StrEnterGetModeInfo:
  db 'GetModeInfo', 0x0a, 0

StrEnterGetMode:
  db 'GetMode', 0x0a, 0

StrEnterSetMode:
  db 'SetMode', 0x0a, 0

StrEnterSetModeLegacy:
  db 'SetModeLegacy', 0x0a, 0

StrUnkownMode:
  db 'Unkown Mode', 0x0a, 0

StrGetPmCapabilities:
  db 'GetPmCapabilities', 0x0a, 0

StrReadEdid:
  db 'ReadEdid', 0x0a, 0
%endif
