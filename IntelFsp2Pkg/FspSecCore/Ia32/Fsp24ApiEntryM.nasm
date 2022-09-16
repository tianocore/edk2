;; @file
;  Provide FSP API entry points.
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;

    SECTION .text

;
; Following are fixed PCDs
;
extern   ASM_PFX(PcdGet32(PcdTemporaryRamBase))
extern   ASM_PFX(PcdGet32(PcdTemporaryRamSize))
extern   ASM_PFX(PcdGet32(PcdFspTemporaryRamSize))
extern   ASM_PFX(PcdGet8 (PcdFspHeapSizePercentage))

struc FSPM_UPD_COMMON
    ; FSP_UPD_HEADER {
    .FspUpdHeader:            resd    8
    ; }
    ; FSPM_ARCH_UPD {
    .Revision:                resb    1
    .Reserved:                resb    3
    .NvsBufferPtr:            resd    1
    .StackBase:               resd    1
    .StackSize:               resd    1
    .BootLoaderTolumSize:     resd    1
    .BootMode:                resd    1
    .Reserved1:               resb    8
    ; }
    .size:
endstruc

struc FSPM_UPD_COMMON_FSP24
    ; FSP_UPD_HEADER {
    .FspUpdHeader:              resd  8
    ; }
    ; FSPM_ARCH2_UPD {
    .Revision:                  resb  1
    .Reserved:                  resb  3
    .Length                     resd  1
    .NvsBufferPtr               resq  1
    .StackBase:                 resq  1
    .StackSize:                 resq  1
    .BootLoaderTolumSize:       resd  1
    .BootMode:                  resd  1
    .FspEventHandler            resq  1
    .Reserved1:                 resb 16
    ; }
    .size:
endstruc

;
; Following functions will be provided in C
;
extern ASM_PFX(SecStartup)
extern ASM_PFX(FspApiCommon)

;
; Following functions will be provided in PlatformSecLib
;
extern ASM_PFX(AsmGetFspBaseAddress)
extern ASM_PFX(AsmGetFspInfoHeader)
extern ASM_PFX(FspMultiPhaseMemInitApiHandler)

STACK_SAVED_EAX_OFFSET       EQU   4 * 7 ; size of a general purpose register * eax index
API_PARAM1_OFFSET            EQU   34h  ; ApiParam1 [ sub esp,8 + pushad + pushfd + push eax + call]
FSP_HEADER_IMGBASE_OFFSET    EQU   1Ch
FSP_HEADER_CFGREG_OFFSET     EQU   24h

;----------------------------------------------------------------------------
; FspMemoryInit API
;
; This FSP API is called after TempRamInit and initializes the memory.
;
;----------------------------------------------------------------------------
global ASM_PFX(FspMemoryInitApi)
ASM_PFX(FspMemoryInitApi):
  mov    eax,  3 ; FSP_API_INDEX.FspMemoryInitApiIndex
  jmp    ASM_PFX(FspApiCommon)

;----------------------------------------------------------------------------
; FspMultiPhaseMemoryInitApi API
;
; This FSP API provides multi-phase Memory initialization, which brings greater
; modularity beyond the existing FspMemoryInit() API.
; Increased modularity is achieved by adding an extra API to FSP-M.
; This allows the bootloader to add board specific initialization steps throughout
; the MemoryInit flow as needed.
;
;----------------------------------------------------------------------------
global ASM_PFX(FspMultiPhaseMemoryInitApi)
ASM_PFX(FspMultiPhaseMemoryInitApi):
  mov    eax,  8 ; FSP_API_INDEX.FspMultiPhaseMemInitApiIndex
  jmp    ASM_PFX(FspApiCommon)
;----------------------------------------------------------------------------
; TempRamExitApi API
;
; This API tears down temporary RAM
;
;----------------------------------------------------------------------------
global ASM_PFX(TempRamExitApi)
ASM_PFX(TempRamExitApi):
  mov    eax,  4 ; FSP_API_INDEX.TempRamExitApiIndex
  jmp    ASM_PFX(FspApiCommon)

;----------------------------------------------------------------------------
; FspApiCommonContinue API
;
; This is the FSP API common entry point to resume the FSP execution
;
;----------------------------------------------------------------------------
global ASM_PFX(FspApiCommonContinue)
ASM_PFX(FspApiCommonContinue):
  ;
  ; Handle FspMultiPhaseMemInitApiIndex API
  ;
  cmp    eax, 8   ; FspMultiPhaseMemInitApiIndex
  jnz    NotMultiPhaseMemoryInitApi

  pushad
  push   DWORD [esp + (4 * 8 + 4)]  ; push ApiParam
  push   eax                        ; push ApiIdx
  call   ASM_PFX(FspMultiPhaseMemInitApiHandler)
  add    esp, 8
  mov    dword  [esp + STACK_SAVED_EAX_OFFSET], eax
  popad
  ret

NotMultiPhaseMemoryInitApi:

  ;
  ; FspMemoryInit API setup the initial stack frame
  ;

  ;
  ; Place holder to store the FspInfoHeader pointer
  ;
  push   eax

  ;
  ; Update the FspInfoHeader pointer
  ;
  push   eax
  call   ASM_PFX(AsmGetFspInfoHeader)
  mov    [esp + 4], eax
  pop    eax

  ;
  ; Create a Task Frame in the stack for the Boot Loader
  ;
  pushfd     ; 2 pushf for 4 byte alignment
  cli
  pushad

  ; Reserve 8 bytes for IDT save/restore
  sub     esp, 8
  sidt    [esp]


  ;  Get Stackbase and StackSize from FSPM_UPD Param
  mov    edx, [esp + API_PARAM1_OFFSET]
  cmp    edx, 0
  jnz    FspStackSetup

  ; Get UPD default values if FspmUpdDataPtr (ApiParam1) is null
  push   eax
  call   ASM_PFX(AsmGetFspInfoHeader)
  mov    edx, [eax + FSP_HEADER_IMGBASE_OFFSET]
  add    edx, [eax + FSP_HEADER_CFGREG_OFFSET]
  pop    eax

FspStackSetup:
  mov    ecx, [edx + FSPM_UPD_COMMON.Revision]
  cmp    ecx, 3
  jae    FspmUpdCommon2

  ;
  ; StackBase = temp memory base, StackSize = temp memory size
  ;
  mov    edi, [edx + FSPM_UPD_COMMON.StackBase]
  mov    ecx, [edx + FSPM_UPD_COMMON.StackSize]
  jmp    ChkFspHeapSize

FspmUpdCommon2:
  mov    edi, [edx + FSPM_UPD_COMMON_FSP24.StackBase]
  mov    ecx, [edx + FSPM_UPD_COMMON_FSP24.StackSize]

ChkFspHeapSize:
  ;
  ; Keep using bootloader stack if heap size % is 0
  ;
  mov    bl, BYTE [ASM_PFX(PcdGet8 (PcdFspHeapSizePercentage))]
  cmp    bl, 0
  jz     SkipStackSwitch

  ;
  ; Set up a dedicated temp ram stack for FSP if FSP heap size % doesn't equal 0
  ;
  add    edi, ecx
  ;
  ; Switch to new FSP stack
  ;
  xchg   edi, esp                                ; Exchange edi and esp, edi will be assigned to the current esp pointer and esp will be Stack base + Stack size

SkipStackSwitch:
  ;
  ; If heap size % is 0:
  ;   EDI is FSPM_UPD_COMMON.StackBase and will hold ESP later (boot loader stack pointer)
  ;   ECX is FSPM_UPD_COMMON.StackSize
  ;   ESP is boot loader stack pointer (no stack switch)
  ;   BL  is 0 to indicate no stack switch (EBX will hold FSPM_UPD_COMMON.StackBase later)
  ;
  ; If heap size % is not 0
  ;   EDI is boot loader stack pointer
  ;   ECX is FSPM_UPD_COMMON.StackSize
  ;   ESP is new stack (FSPM_UPD_COMMON.StackBase + FSPM_UPD_COMMON.StackSize)
  ;   BL  is NOT 0 to indicate stack has switched
  ;
  cmp    bl, 0
  jnz    StackHasBeenSwitched

  mov    ebx, edi                                ; Put FSPM_UPD_COMMON.StackBase to ebx as temp memory base
  mov    edi, esp                                ; Put boot loader stack pointer to edi
  jmp    StackSetupDone

StackHasBeenSwitched:
  mov    ebx, esp                                ; Put Stack base + Stack size in ebx
  sub    ebx, ecx                                ; Stack base + Stack size - Stack size as temp memory base

StackSetupDone:

  ;
  ; Pass the API Idx to SecStartup
  ;
  push    eax

  ;
  ; Pass the BootLoader stack to SecStartup
  ;
  push    edi

  ;
  ; Pass entry point of the PEI core
  ;
  call    ASM_PFX(AsmGetFspBaseAddress)
  mov     edi, eax
  call    ASM_PFX(AsmGetPeiCoreOffset)
  add     edi, eax
  push    edi

  ;
  ; Pass BFV into the PEI Core
  ; It uses relative address to calculate the actual boot FV base
  ; For FSP implementation with single FV, PcdFspBootFirmwareVolumeBase and
  ; PcdFspAreaBaseAddress are the same. For FSP with multiple FVs,
  ; they are different. The code below can handle both cases.
  ;
  call    ASM_PFX(AsmGetFspBaseAddress)
  push    eax

  ;
  ; Pass stack base and size into the PEI Core
  ;
  push    ebx
  push    ecx

  ;
  ; Pass Control into the PEI Core
  ;
  call    ASM_PFX(SecStartup)
  add     esp, 4
exit:
  ret

global ASM_PFX(FspPeiCoreEntryOff)
ASM_PFX(FspPeiCoreEntryOff):
   ;
   ; This value will be patched by the build script
   ;
   DD    0x12345678

global ASM_PFX(AsmGetPeiCoreOffset)
ASM_PFX(AsmGetPeiCoreOffset):
   mov   eax, dword [ASM_PFX(FspPeiCoreEntryOff)]
   ret

;----------------------------------------------------------------------------
; TempRamInit API
;
; Empty function for WHOLEARCHIVE build option
;
;----------------------------------------------------------------------------
global ASM_PFX(TempRamInitApi)
ASM_PFX(TempRamInitApi):
  jmp $
  ret

;----------------------------------------------------------------------------
; Module Entrypoint API
;----------------------------------------------------------------------------
global ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
  jmp $
