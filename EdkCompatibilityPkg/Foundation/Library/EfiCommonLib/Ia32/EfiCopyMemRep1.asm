;/*++
;
;Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
;This program and the accompanying materials                          
;are licensed and made available under the terms and conditions of the BSD License         
;which accompanies this distribution.  The full text of the license may be found at        
;http://opensource.org/licenses/bsd-license.php                                            
;                                                                                          
;THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;
;Module Name:
;
;  EfiCopyMemRep1.asm
;
;Abstract:
;
;  This is the code that uses rep movsb CopyMem service
;
;--*/
;
;---------------------------------------------------------------------------
    .686
    .model  flat,C
    .code

;---------------------------------------------------------------------------
;#include "Tiano.h"
;
;VOID
;EfiCommonLibCopyMem (
;  IN VOID   *Destination,
;  IN VOID   *Source,
;  IN UINTN  Count
;  )
;/*++
;
;Routine Description:
;
;  Copy Length bytes from Source to Destination.
;
;Arguments:
;
;  Destination - Target of copy
;
;  Source      - Place to copy from
;
;  Length      - Number of bytes to copy
;
;Returns:
;
;  None
;
;--*/
EfiCommonLibCopyMem PROC
    push    ebp
    mov     ebp, esp
    push    esi
    push    edi
    mov     esi, dword ptr[ebp + 0Ch]    ; esi <- Source
    mov     edi, dword ptr[ebp + 8]      ; edi <- Destination
    mov     edx, dword ptr[ebp + 10h]    ; edx <- Count
    cmp     esi, edi
    je      _CopyDone
    cmp     edx, 0
    je      _CopyDone
    lea     eax, [esi + edx - 1]         ; eax <- End of Source
    cmp     esi, edi
    jae     _CopyBytes
    cmp     eax, edi
    jb      _CopyBytes                   ; Copy backward if overlapped
    mov     esi, eax                     ; esi <- End of Source
    lea     edi, [edi + edx - 1]         ; edi <- End of Destination
    std
_CopyBytes:
    mov     ecx, edx
    rep     movsb                        ; Copy bytes backward
    cld
_CopyDone:
    pop     edi
    pop     esi
    pop     ebp
    ret
EfiCommonLibCopyMem ENDP
	END

