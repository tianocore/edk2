;------------------------------------------------------------------------------
; @file
; Search for the Boot Firmware Volume (BFV) base address
;
; Copyright (c) 2008 - 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

;#define EFI_FIRMWARE_FILE_SYSTEM2_GUID \
;  { 0x8c8ce578, 0x8a3d, 0x4f1c, { 0x99, 0x35, 0x89, 0x61, 0x85, 0xc3, 0x2d, 0xd3 } }
%define FFS2_GUID_DWORD0 0x8c8ce578
%define FFS2_GUID_DWORD1 0x4f1c8a3d
%define FFS2_GUID_DWORD2 0x61893599
%define FFS2_GUID_DWORD3 0xd32dc385

;#define EFI_FIRMWARE_FILE_SYSTEM3_GUID \
;  { 0x8c8ce578, 0x3dcb, 0x4dca, { 0xbd, 0x6f, 0x1e, 0x96, 0x89, 0xe7, 0x34, 0x9a } }
%define FFS3_GUID_DWORD0 0x5473c07a
%define FFS3_GUID_DWORD1 0x4dca3dcb
%define FFS3_GUID_DWORD2 0x961e6fbd
%define FFS3_GUID_DWORD3 0x9a34e789

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
    ; Check FFS3 GUID
    ;
    cmp     dword [eax + 0x10], FFS3_GUID_DWORD0
    jne     searchingForFfs2Guid
    cmp     dword [eax + 0x14], FFS3_GUID_DWORD1
    jne     searchingForFfs2Guid
    cmp     dword [eax + 0x18], FFS3_GUID_DWORD2
    jne     searchingForFfs2Guid
    cmp     dword [eax + 0x1c], FFS3_GUID_DWORD3
    jne     searchingForFfs2Guid
    jmp     checkingFvLength

searchingForFfs2Guid:
    ;
    ; Check FFS2 GUID
    ;
    cmp     dword [eax + 0x10], FFS2_GUID_DWORD0
    jne     searchingForBfvHeaderLoop
    cmp     dword [eax + 0x14], FFS2_GUID_DWORD1
    jne     searchingForBfvHeaderLoop
    cmp     dword [eax + 0x18], FFS2_GUID_DWORD2
    jne     searchingForBfvHeaderLoop
    cmp     dword [eax + 0x1c], FFS2_GUID_DWORD3
    jne     searchingForBfvHeaderLoop

checkingFvLength:
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

