;; @file
;  Provide FSP helper function.
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;
    DEFAULT  REL
    SECTION .text

global ASM_PFX(AsmGetFspBaseAddress)
ASM_PFX(AsmGetFspBaseAddress):
   call  ASM_PFX(AsmGetFspInfoHeader)
   add   rax, 0x1C
   mov   eax, [rax]
   ret

global ASM_PFX(AsmGetFspInfoHeader)
ASM_PFX(AsmGetFspInfoHeader):
   lea   rax, [ASM_PFX(AsmGetFspInfoHeader)]
   DB    0x48, 0x2d               ; sub rax, 0x????????
global ASM_PFX(FspInfoHeaderRelativeOff)
ASM_PFX(FspInfoHeaderRelativeOff):
   DD    0x12345678               ; This value must be patched by the build script
   and   rax, 0xffffffff
   ret

global ASM_PFX(AsmGetFspInfoHeaderNoStack)
ASM_PFX(AsmGetFspInfoHeaderNoStack):
   lea   rax, [ASM_PFX(AsmGetFspInfoHeader)]
   lea   rcx, [ASM_PFX(FspInfoHeaderRelativeOff)]
   mov   ecx, [rcx]
   sub   rax, rcx
   and   rax, 0xffffffff
   jmp   rdi
