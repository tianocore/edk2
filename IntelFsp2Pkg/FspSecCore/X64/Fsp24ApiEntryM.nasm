;; @file
;  Provide FSP API entry points.
;
; Copyright (c) 2022 - 2025, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;;
    DEFAULT REL
    SECTION .text

%include    "PushPopRegsNasm.inc"

;
; Following are fixed PCDs
;
extern   ASM_PFX(PcdGet8 (PcdFspHeapSizePercentage))
extern   ASM_PFX(FeaturePcdGet (PcdFspSaveRestorePageTableEnable))

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

STACK_SAVED_RAX_OFFSET       EQU   8 * 7 ; size of a general purpose register * rax index
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
  mov    rax,  3 ; FSP_API_INDEX.FspMemoryInitApiIndex
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
  mov    rax,  8 ; FSP_API_INDEX.FspMultiPhaseMemInitApiIndex
  jmp    ASM_PFX(FspApiCommon)
;----------------------------------------------------------------------------
; TempRamExitApi API
;
; This API tears down temporary RAM
;
;----------------------------------------------------------------------------
global ASM_PFX(TempRamExitApi)
ASM_PFX(TempRamExitApi):
  mov    rax,  4 ; FSP_API_INDEX.TempRamExitApiIndex
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
  ; Handle FspMultiPhaseMemoInitApiIndex API
  ;
  push   rdx    ; Push a QWORD data for stack alignment

  cmp    rax, 8 ; FspMultiPhaseMemInitApiIndex
  jnz    NotMultiPhaseMemoryInitApi

  PUSHA_64
  mov    rdx, rcx           ; move ApiParam to rdx
  mov    rcx, rax           ; move ApiIdx to rcx
  sub    rsp, 0x20          ; calling C function may need shadow space
  call   ASM_PFX(FspMultiPhaseMemInitApiHandler)
  add    rsp, 0x20          ; restore shadow space
  mov    qword [rsp + STACK_SAVED_RAX_OFFSET], rax
  POPA_64
  add    rsp, 0x08
  ret

NotMultiPhaseMemoryInitApi:
  ; Push RDX and RCX to form CONTEXT_STACK_64
  push   rdx    ; Push API Parameter2 on stack
  push   rcx    ; Push API Parameter1 on stack

  ;
  ; FspMemoryInit API setup the initial stack frame
  ;

  ;
  ; Place holder to store the FspInfoHeader pointer
  ;
  push   rax

  ;
  ; Update the FspInfoHeader pointer
  ;
  push   rax
  call   ASM_PFX(AsmGetFspInfoHeader)
  mov    [rsp + 8], rax
  pop    rax

  ;
  ; Create a Task Frame in the stack for the Boot Loader
  ;
  pushfq
  cli
  PUSHA_64

  ;
  ; Allocate 4x8 bytes on the stack.
  ;
  sub     rsp, 32
  lea     rdx, [ASM_PFX(FeaturePcdGet (PcdFspSaveRestorePageTableEnable))]
  mov     dl, byte [rdx]
  cmp     dl, 0
  jz      SkipPagetableSave

  add     rsp, 32
  ; Save EFER MSR
  push   rcx
  push   rax
  mov    rcx, 0xC0000080
  rdmsr
  shl    rdx, 0x20
  or     rdx, rax
  pop    rax
  pop    rcx
  push   rdx

  ; Save CR registers
  mov    rdx, cr4
  push   rdx
  mov    rdx, cr3
  push   rdx
  mov    rdx, cr0
  push   rdx
SkipPagetableSave:

  ; Save Segment registers
  mov     rdx, ss
  push    rdx
  mov     rdx, gs
  push    rdx
  mov     rdx, fs
  push    rdx
  mov     rdx, es
  push    rdx
  mov     rdx, ds
  push    rdx
  mov     rdx, cs
  push    rdx

  ; Reserve 16 bytes for GDT save/restore
  sub     rsp, 16
  sgdt    [rsp]

  ; Reserve 16 bytes for IDT save/restore
  sub     rsp, 16
  sidt    [rsp]

  ;  Get Stackbase and StackSize from FSPM_UPD Param
  mov    rdx, rcx                                ; Put FSPM_UPD Param to rdx
  cmp    rdx, 0
  jnz    FspStackSetup

  ; Get UPD default values if FspmUpdDataPtr (ApiParam1) is null
  xchg   rbx, rax
  call   ASM_PFX(AsmGetFspInfoHeader)
  mov    edx, [rax + FSP_HEADER_IMGBASE_OFFSET]
  add    edx, [rax + FSP_HEADER_CFGREG_OFFSET]
  xchg   rbx, rax

FspStackSetup:
  mov    cl, [rdx + FSPM_UPD_COMMON_FSP24.Revision]
  cmp    cl, 3
  jae    FspmUpdCommonFsp24

  mov    rax, 08000000000000002h                 ; RETURN_INVALID_PARAMETER
  sub    rsp, 0b8h
  ret

FspmUpdCommonFsp24:
  ;
  ; StackBase = temp memory base, StackSize = temp memory size
  ;
  mov    rdi, [rdx + FSPM_UPD_COMMON_FSP24.StackBase]
  mov    ecx, [rdx + FSPM_UPD_COMMON_FSP24.StackSize]

  ;
  ; Keep using bootloader stack if heap size % is 0
  ;
  mov    rbx, ASM_PFX(PcdGet8 (PcdFspHeapSizePercentage))
  mov    bl,  BYTE [rbx]
  cmp    bl,  0
  jz     SkipStackSwitch

  ;
  ; Set up a dedicated temp ram stack for FSP if FSP heap size % doesn't equal 0
  ;
  add    rdi, rcx
  ;
  ; Switch to new FSP stack
  ;
  xchg   rdi, rsp                                ; Exchange rdi and rsp, rdi will be assigned to the current rsp pointer and rsp will be Stack base + Stack size

SkipStackSwitch:
  ;
  ; If heap size % is 0:
  ;   EDI is FSPM_UPD_COMMON_FSP24.StackBase and will hold ESP later (boot loader stack pointer)
  ;   ECX is FSPM_UPD_COMMON_FSP24.StackSize
  ;   ESP is boot loader stack pointer (no stack switch)
  ;   BL  is 0 to indicate no stack switch (EBX will hold FSPM_UPD_COMMON_FSP24.StackBase later)
  ;
  ; If heap size % is not 0
  ;   EDI is boot loader stack pointer
  ;   ECX is FSPM_UPD_COMMON_FSP24.StackSize
  ;   ESP is new stack (FSPM_UPD_COMMON_FSP24.StackBase + FSPM_UPD_COMMON_FSP24.StackSize)
  ;   BL  is NOT 0 to indicate stack has switched
  ;
  cmp    bl, 0
  jnz    StackHasBeenSwitched

  mov    rbx, rdi                                ; Put FSPM_UPD_COMMON_FSP24.StackBase to rbx as temp memory base
  mov    rdi, rsp                                ; Put boot loader stack pointer to rdi
  jmp    StackSetupDone

StackHasBeenSwitched:
  mov    rbx, rsp                                ; Put Stack base + Stack size in ebx
  sub    rbx, rcx                                ; Stack base + Stack size - Stack size as temp memory base

StackSetupDone:

  ;
  ; Per X64 calling convention, make sure RSP is 16-byte aligned.
  ;
  mov    rdx, rsp
  and    rdx, 0fh
  sub    rsp, rdx

  ;
  ; Pass the API Idx to SecStartup
  ;
  push   rax

  ;
  ; Pass the BootLoader stack to SecStartup
  ;
  push   rdi

  ;
  ; Pass BFV into the PEI Core
  ; It uses relative address to calculate the actual boot FV base
  ; For FSP implementation with single FV, PcdFspBootFirmwareVolumeBase and
  ; PcdFspAreaBaseAddress are the same. For FSP with multiple FVs,
  ; they are different. The code below can handle both cases.
  ;
  call    ASM_PFX(AsmGetFspBaseAddress)
  mov    r8, rax

  ;
  ; Pass entry point of the PEI core
  ;
  call   ASM_PFX(AsmGetPeiCoreOffset)
  lea    r9,  [r8 + rax]

  ;
  ; Pass stack base and size into the PEI Core
  ;
  mov    rcx,  rcx
  mov    rdx,  rbx

  ;
  ; Pass Control into the PEI Core
  ; RCX = SizeOfRam, RDX = TempRamBase, R8 = BFV, R9 = PeiCoreEntry, Last 1 Stack = BL stack, Last 2 Stack = API index
  ; According to X64 calling convention, caller has to allocate 32 bytes as a shadow store on call stack right before
  ; calling the function.
  ;
  sub    rsp, 20h
  call   ASM_PFX(SecStartup)
  add    rsp, 20h
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
   push  rbx
   mov   rbx, ASM_PFX(FspPeiCoreEntryOff)
   mov   eax, dword[ebx]
   pop   rbx
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

