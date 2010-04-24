;------------------------------------------------------------------------------
; @file
; Search for the Boot Firmware Volume (BFV) base address
;
; Copyright (c) 2008 - 2009, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
; Modified:  EAX, EBX
; Preserved: EDI, ESP
;
; @param[out]  EBP  Address of Boot Firmware Volume (BFV)
;
Flat32SearchForBfvBase:

    xor     eax, eax
searchingForBfvHeaderLoop:
    ;
    ; We check for a firmware volume at every 4KB address in the top 16MB
    ; just below 4GB.  (Addresses at 0xffHHH000 where H is any hex digit.)
    ;
    sub     eax, 0x1000
    cmp     eax, 0xff000000
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
    ;
    ; Hang if the SEC entry point was not found
    ;
    debugShowPostCode POSTCODE_BFV_NOT_FOUND

    ;
    ; 0xbfbfbfbf in the EAX & EBP registers helps signal what failed
    ; for debugging purposes.
    ;
    mov     eax, 0xBFBFBFBF
    mov     ebp, eax
    jmp     $

searchedForBfvHeaderAndItWasFound:
    mov     ebp, eax

    debugShowPostCode POSTCODE_BFV_FOUND

    OneTimeCallRet Flat32SearchForBfvBase

