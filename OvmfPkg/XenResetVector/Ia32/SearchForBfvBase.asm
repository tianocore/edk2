;------------------------------------------------------------------------------
; @file
; Search for the Boot Firmware Volume (BFV) base address
;
; Copyright (c) 2008 - 2009, Intel Corporation. All rights reserved.<BR>
; Copyright (c) 2019, Citrix Systems, Inc.
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
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
; Modified:  EAX, EBX, ECX
; Preserved: EDI, ESP
;
; @param[in]   EAX  Start search from here
; @param[out]  EBP  Address of Boot Firmware Volume (BFV)
;
Flat32SearchForBfvBase:

    mov     ecx, eax
searchingForBfvHeaderLoop:
    ;
    ; We check for a firmware volume at every 4KB address in the 16MB
    ; just below where we started, ECX.
    ;
    sub     eax, 0x1000
    mov     ebx, ecx
    sub     ebx, eax
    cmp     ebx, 0x01000000
    ; if ECX-EAX > 16MB; jump notfound
    ja      searchedForBfvHeaderButNotFound

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
    cmp     ebx, ecx
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

