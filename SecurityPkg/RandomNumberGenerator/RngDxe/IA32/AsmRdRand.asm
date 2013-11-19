;------------------------------------------------------------------------------
;
; Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   AsmRdRand.Asm
;
; Abstract:
;
;   Implementation for 16-, and 32- invocations of RDRAND instruction under 32bit platform.
;
; Notes:
;
;   Visual Studio coding practices do not use inline asm since multiple compilers and 
;   architectures are supported assembler not recognizing rdrand instruction so using DB's.
;
;------------------------------------------------------------------------------

    .586P
    .model flat, C
    .code
 
;------------------------------------------------------------------------------
;  Generate a 16 bit random number
;  Return TRUE if Rand generated successfully, or FALSE if not
;
;  BOOLEAN EFIAPI RdRand16Step (UINT16 *Rand);   ECX
;------------------------------------------------------------------------------
RdRand16Step  PROC
    ; rdrand   ax                  ; generate a 16 bit RN into ax, CF=1 if RN generated ok, otherwise CF=0
    db     0fh, 0c7h, 0f0h         ; rdrand r16:  "0f c7 /6  ModRM:r/m(w)"
    jb     rn16_ok                 ; jmp if CF=1
    xor    eax, eax                ; reg=0 if CF=0
    ret                            ; return with failure status
rn16_ok:
    mov    [ecx], ax
    mov    eax, 1
    ret
RdRand16Step ENDP

;------------------------------------------------------------------------------
;  Generate a 32 bit random number
;    Return TRUE if Rand generated successfully, or FALSE if not
;
;  BOOLEAN EFIAPI RdRand32Step (UINT32 *Rand);   ECX
;------------------------------------------------------------------------------
RdRand32Step  PROC
    ; rdrand   eax                 ; generate a 32 bit RN into eax, CF=1 if RN generated ok, otherwise CF=0
    db     0fh, 0c7h, 0f0h         ; rdrand r32:  "0f c7 /6  ModRM:r/m(w)"
    jb     rn32_ok                 ; jmp if CF=1
    xor    eax, eax                ; reg=0 if CF=0
    ret                            ; return with failure status
rn32_ok:
    mov    [ecx], eax
    mov    eax,  1
    ret
RdRand32Step  ENDP

    END
