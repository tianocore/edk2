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
  ;EfiSetMemRep4.c
;
;Abstract:
;
  ;This is the code that uses rep stosd SetMem service
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
;EfiCommonLibSetMem (
  ;IN VOID   *Buffer,
  ;IN UINTN  Count,
  ;IN UINT8  Value
  ;)
;/*++
;
;Input:  VOID   *Buffer - Pointer to buffer to write
        ;UINTN  Count   - Number of bytes to write
        ;UINT8  Value   - Value to write
;
;Output: None.
;
;Saves:
;
;Modifies:
;
;Description:  This function uses rep stosd to set memory.
;
;--*/
EfiCommonLibSetMem PROC
    push        ebp
    mov         ebp,esp
    push        edi
    mov         ecx,dword ptr [ebp+0Ch]
    test        ecx, ecx
    je          Exit
    mov         al,byte ptr [ebp+10h]
    mov         ah,  al
    shrd        edx, eax, 16
    shld        eax, edx, 16
    mov         edx, ecx
    mov         edi,dword ptr [ebp+8]
    shr         ecx, 2
    rep stosd
    mov         ecx, edx
    and         ecx, 3
    rep stosb
Exit:
    pop         edi
    pop         ebp
    ret

EfiCommonLibSetMem ENDP
	END
