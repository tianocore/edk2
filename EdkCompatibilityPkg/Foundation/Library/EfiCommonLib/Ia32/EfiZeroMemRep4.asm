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
  ;EfiZeroMemRep4.asm
;
;Abstract:
;
  ;This is the code that uses rep stosd ZeroMem service
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
;EfiCommonLibZeroMem (
  ;IN VOID   *Buffer,
  ;IN UINTN  Count
  ;)
;/*++
;
;Input:  VOID   *Buffer - Pointer to buffer to clear
        ;UINTN  Count  - Number of bytes to clear
;
;Output: None.
;
;Saves:
;
;Modifies:
;
;Description:  This function uses rep stosd to zero memory.
;
;--*/
EfiCommonLibZeroMem PROC
    push        ebp
    mov         ebp,esp
    push        edi
    mov         ecx,dword ptr [ebp+0Ch]
    test        ecx, ecx
    je          Exit
    xor         eax, eax
    mov         edi,dword ptr [ebp+8]
    mov         edx, ecx
    shr         ecx, 2
    and         edx, 3
    rep         stosd
    mov         ecx, edx
    rep         stosb
Exit:
    pop         edi
    pop         ebp
    ret

EfiCommonLibZeroMem ENDP	
	END
