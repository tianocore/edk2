;; @file
;  Run before FSP SecCore to rebase SecCore and PeiCore
;
; Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;
    DEFAULT  REL
    SECTION .text

%include    "PushPopRegsNasm.inc"

;
; Following functions will be provided in C
;
extern ASM_PFX(FspPatchItself)

;----------------------------------------------------------------------------
; PreFspSecCommon API
;
; This is the PreFspSec common entry point to resume the FSP execution
; RAX keeps the index
;
;----------------------------------------------------------------------------
global ASM_PFX(PreFspSecCommon)
ASM_PFX(PreFspSecCommon):
  PUSHA_64
  call   ASM_PFX(FspPatchItself)
  POPA_64
  push rax
  call   ASM_PFX(AsmGetRuntimeSecEntry)
  mov rbx, rax
  pop rax
  jmp rbx


global ASM_PFX(AsmGetFspInfoHeader)
ASM_PFX(AsmGetFspInfoHeader):
   lea   rax, [ASM_PFX(AsmGetFspInfoHeader)]
   DB    0x48, 0x2d               ; sub rax, 0x????????
global ASM_PFX(FspInfoHeaderRelativeOff)
ASM_PFX(FspInfoHeaderRelativeOff):
   DD    0x12345678               ; This value must be patched by the build script
   ret

global ASM_PFX(AsmGetRuntimeFspBaseAddress)
ASM_PFX(AsmGetRuntimeFspBaseAddress):
   lea   rax, [ASM_PFX(FspBaseAddressRelativeOff)]
   DB    0x48, 0x2d               ; sub rax, 0x????????
global ASM_PFX(FspBaseAddressRelativeOff)
ASM_PFX(FspBaseAddressRelativeOff):
   DD    0x12345678               ; This value must be patched by the build script
   ret

global ASM_PFX(AsmGetRuntimeSecCoreAddress)
ASM_PFX(AsmGetRuntimeSecCoreAddress):
   lea   rax, [ASM_PFX(SecCoreAddressRelativeOff)]
   mov   rcx, rax
   xor   rdx, rdx
   DB    0x48, 0x81, 0xe9         ; sub rcx, 0x????????
global ASM_PFX(SecCoreAddressRelativeOff)
ASM_PFX(SecCoreAddressRelativeOff):
   DD    0                        ; This value can be patched by the build script if need to rebase SecCore
   CMPXCHG rcx, rdx               ; if not patched, set rcx as zero(rdx is zero)
   mov     rax, rcx
   ret

global ASM_PFX(AsmGetRuntimePeiCoreAddress)
ASM_PFX(AsmGetRuntimePeiCoreAddress):
   lea   rax, [ASM_PFX(PeiCoreAddressRelativeOff)]
   mov   rcx, rax
   xor   rdx, rdx
   DB    0x48, 0x81, 0xe9         ; sub rcx, 0x????????
global ASM_PFX(PeiCoreAddressRelativeOff)
ASM_PFX(PeiCoreAddressRelativeOff):
   DD    0                        ; This value can be patched by the build script if need to rebase PeiCore
   CMPXCHG rcx, rdx               ; if not patched, set rcx as zero(rdx is zero)
   mov     rax, rcx
   ret

global ASM_PFX(AsmGetRuntimeSecEntry)
ASM_PFX(AsmGetRuntimeSecEntry):
   lea   rax, [ASM_PFX(SecEntryAddressRelativeOff)]
   DB    0x48, 0x2d               ; sub rax, 0x????????
global ASM_PFX(SecEntryAddressRelativeOff)
ASM_PFX(SecEntryAddressRelativeOff):
   DD    0x12345678               ; This value must be patched by the build script
   ret
