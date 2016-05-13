;; @file
;  Provide FSP helper function.
;
; Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;;

    SECTION .text

global ASM_PFX(FspInfoHeaderRelativeOff)
ASM_PFX(FspInfoHeaderRelativeOff):
   ;
   ; This value will be pached by the build script
   ;
   DD    0x12345678

global ASM_PFX(AsmGetFspBaseAddress)
ASM_PFX(AsmGetFspBaseAddress):
   mov   eax, ASM_PFX(AsmGetFspInfoHeader)
   sub   eax, dword [ASM_PFX(FspInfoHeaderRelativeOff)]
   add   eax, 0x1C
   mov   eax, dword [eax]
   ret

global ASM_PFX(AsmGetFspInfoHeader)
ASM_PFX(AsmGetFspInfoHeader):
   mov   eax, ASM_PFX(AsmGetFspInfoHeader)
   sub   eax, dword [ASM_PFX(FspInfoHeaderRelativeOff)]
   ret
