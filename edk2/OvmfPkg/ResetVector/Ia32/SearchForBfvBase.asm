;------------------------------------------------------------------------------
;
; Copyright (c) 2008, Intel Corporation
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
;   SearchForBfvBase.asm
;
; Abstract:
;
;   Search for the Boot FV Base Address
;
;------------------------------------------------------------------------------

;#define EFI_FIRMWARE_FILE_SYSTEM2_GUID \
;  { 0x8c8ce578, 0x8a3d, 0x4f1c, { 0x99, 0x35, 0x89, 0x61, 0x85, 0xc3, 0x2d, 0xd3 } }
%define FFS_GUID_DWORD0 0x8c8ce578
%define FFS_GUID_DWORD1 0x4f1c8a3d
%define FFS_GUID_DWORD2 0x61893599
%define FFS_GUID_DWORD3 0xd32dc385

BITS    32

;
; Input:
;   None
;
; Output:
;   EBP - BFV Base Address
;
; Modified:
;   EAX, EBX
;
Flat32SearchForBfvBase:

    xor     eax, eax
searchingForBfvHeaderLoop:
    sub     eax, 0x1000
    cmp     eax, 0xff800000
    jb      searchedForBfvHeaderButNotFound

    ;
    ; Check FFS GUID
    ;
    cmp     dword [eax + 0x10], FFS_GUID_DWORD0
    jne     searchingForBfvHeaderLoop
    cmp     dword [eax + 0x14], FFS_GUID_DWORD1
    jne     searchingForBfvHeaderLoop
    cmp     dword [eax + 0x18], FFS_GUID_DWORD2
    jne     searchingForBfvHeaderLoop
    cmp     dword [eax + 0x1c], FFS_GUID_DWORD3
    jne     searchingForBfvHeaderLoop

    ;
    ; Check FV Length
    ;
    cmp     dword [eax + 0x24], 0
    jne     searchingForBfvHeaderLoop
    mov     ebx, eax
    add     ebx, dword [eax + 0x20]
    jnz     searchingForBfvHeaderLoop

    jmp     searchedForBfvHeaderAndItWasFound

searchedForBfvHeaderButNotFound:
    writeToSerialPort '!'
    xor     eax, eax

searchedForBfvHeaderAndItWasFound:
    mov     ebp, eax

    writeToSerialPort 'B'
    writeToSerialPort 'F'
    writeToSerialPort 'V'
    writeToSerialPort ' '

    or      ebp, ebp
    jz      $

    OneTimeCallRet Flat32SearchForBfvBase

