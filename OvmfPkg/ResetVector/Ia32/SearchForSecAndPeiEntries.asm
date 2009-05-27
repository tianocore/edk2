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
;   SearchForSecAndPeiEntry.asm
;
; Abstract:
;
;   Search for the SEC Core and PEI Core entry points
;
;------------------------------------------------------------------------------

BITS    32

%define EFI_FV_FILETYPE_SECURITY_CORE         0x03
%define EFI_FV_FILETYPE_PEI_CORE              0x04

;
; Input:
;   EBP - BFV Base Address
;
; Output:
;   ESI - SEC Core Entry Point Address (or 0 if not found)
;   EDI - PEI Core Entry Point Address (or 0 if not found)
;
; Modified:
;   EAX, EBX, ECX
;
Flat32SearchForSecAndPeiEntries:

    ;
    ; Initialize EBP and ESI to 0
    ;
    xor     ebx, ebx
    mov     esi, ebx
    mov     edi, ebx

    ;
    ; Pass over the BFV header
    ;
    mov     eax, ebp
    mov     bx, [ebp + 0x30]
    add     eax, ebx
    jc      doneSeachingForSecAndPeiEntries

    jmp     searchingForFfsFileHeaderLoop

moveForwardWhileSearchingForFfsFileHeaderLoop:
    ;
    ; Make forward progress in the search
    ;
    inc     eax
    jc      doneSeachingForSecAndPeiEntries

searchingForFfsFileHeaderLoop:
    test    eax, eax
    jz      doneSeachingForSecAndPeiEntries

    ;
    ; Ensure 8 byte alignment
    ;
    add     eax, 7
    jc      doneSeachingForSecAndPeiEntries
    and     al, 0xf8

    ;
    ; Look to see if there is an FFS file at eax
    ;
    mov     bl, [eax + 0x17]
    test    bl, 0x20
    jz      moveForwardWhileSearchingForFfsFileHeaderLoop
    mov     ecx, [eax + 0x14]
    and     ecx, 0x00ffffff
    or      ecx, ecx
    jz      moveForwardWhileSearchingForFfsFileHeaderLoop
;    jmp     $
    add     ecx, eax
    jz      jumpSinceWeFoundTheLastFfsFile
    jc      moveForwardWhileSearchingForFfsFileHeaderLoop
jumpSinceWeFoundTheLastFfsFile:

    ;
    ; There seems to be a valid file at eax
    ;
    mov     bl, [eax + 0x12] ; BL - File Type
    cmp     bl, EFI_FV_FILETYPE_PEI_CORE
    je      fileTypeIsPeiCore
    cmp     bl, EFI_FV_FILETYPE_SECURITY_CORE
    jne     readyToTryFfsFileAtEcx

fileTypeIsSecCore:
    callEdx GetEntryPointOfFfsFileReturnEdx
    test    eax, eax
    jz      readyToTryFfsFileAtEcx

    mov     esi, eax
    jmp     readyToTryFfsFileAtEcx

fileTypeIsPeiCore:
    callEdx GetEntryPointOfFfsFileReturnEdx
    test    eax, eax
    jz      readyToTryFfsFileAtEcx

    mov     edi, eax

readyToTryFfsFileAtEcx:
    mov     eax, ecx
    jmp     searchingForFfsFileHeaderLoop

doneSeachingForSecAndPeiEntries:

    test    esi, esi
    jnz     secCoreEntryPointWasFound
    writeToSerialPort '!'
secCoreEntryPointWasFound:
    writeToSerialPort 'S'
    writeToSerialPort 'E'
    writeToSerialPort 'C'
    writeToSerialPort ' '

    test    edi, edi
    jnz     peiCoreEntryPointWasFound
    writeToSerialPort '!'
peiCoreEntryPointWasFound:
    writeToSerialPort 'P'
    writeToSerialPort 'E'
    writeToSerialPort 'I'
    writeToSerialPort ' '

    OneTimeCallRet Flat32SearchForSecAndPeiEntries


%define EFI_SECTION_PE32                  0x10

;
; Input:
;   EAX - Start of FFS file
;
; Output:
;   EAX - Entry point of PE32 (or 0 if not found)
;
; Modified:
;   EBX
;
GetEntryPointOfFfsFileReturnEdx:
    test    eax, eax
    jz      getEntryPointOfFfsFileErrorReturn

    cmp     byte [eax + 0x1b], EFI_SECTION_PE32
    jne     getEntryPointOfFfsFileErrorReturn

    add     eax, 0x1c       ; EAX = Start of PE32 image

    mov     ebx, eax
    cmp     word [eax], 'MZ'
    jne     thereIsNotAnMzSignature
    movzx   ebx, word [eax + 0x3c]
    add     ebx, eax
thereIsNotAnMzSignature:

    ; if (Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE)
    cmp     word [ebx], 'VZ'
    jne     thereIsNoVzSignature
    ; *EntryPoint = (VOID *)((UINTN)Pe32Data +
    ;   (UINTN)(Hdr.Te->AddressOfEntryPoint & 0x0ffffffff) +
    ;   sizeof(EFI_TE_IMAGE_HEADER) - Hdr.Te->StrippedSize);
    add     eax, [ebx + 0x8]
    add     eax, 0x28
    movzx   ebx, word [ebx + 0x6]
    sub     eax, ebx
    jmp     getEntryPointOfFfsFileReturn

thereIsNoVzSignature:

    ; if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE)
    cmp     dword [ebx], `PE\x00\x00`
    jne     getEntryPointOfFfsFileErrorReturn

    ; *EntryPoint = (VOID *)((UINTN)Pe32Data +
    ;   (UINTN)(Hdr.Pe32->OptionalHeader.AddressOfEntryPoint & 0x0ffffffff));
    add     eax, [ebx + 0x4 + 0x14 + 0x10]
    jmp     getEntryPointOfFfsFileReturn

getEntryPointOfFfsFileErrorReturn:
    mov     eax, 0

getEntryPointOfFfsFileReturn:
    jmp     edx


