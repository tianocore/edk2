;------------------------------------------------------------------------------
; @file
; Relocate the SMBASE on a hot-added CPU when it services its first SMI.
;
; Copyright (c) 2020, Red Hat, Inc.
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; The routine runs on the hot-added CPU in the following "big real mode",
; 16-bit environment; per "SMI HANDLER EXECUTION ENVIRONMENT" in the Intel SDM
; (table "Processor Register Initialization in SMM"):
;
;  - CS selector: 0x3000 (most significant 16 bits of SMM_DEFAULT_SMBASE).
;
;  - CS limit: 0xFFFF_FFFF.
;
;  - CS base: SMM_DEFAULT_SMBASE (0x3_0000).
;
;  - IP: SMM_HANDLER_OFFSET (0x8000).
;
;  - ES, SS, DS, FS, GS selectors: 0.
;
;  - ES, SS, DS, FS, GS limits: 0xFFFF_FFFF.
;
;  - ES, SS, DS, FS, GS bases: 0.
;
;  - Operand-size and address-size override prefixes can be used to access the
;    address space beyond 1MB.
;------------------------------------------------------------------------------

SECTION .data
BITS 16

;
; Bring in SMM_DEFAULT_SMBASE from
; "MdePkg/Include/Register/Intel/SmramSaveStateMap.h".
;
SMM_DEFAULT_SMBASE: equ 0x3_0000

;
; Field offsets in FIRST_SMI_HANDLER_CONTEXT, which resides at
; SMM_DEFAULT_SMBASE.
;
ApicIdGate:              equ  0 ; UINT64
NewSmbase:               equ  8 ; UINT32
FeatureControlHighValue: equ 12 ; UINT32
FeatureControlLowValue:  equ 16 ; UINT32
FeatureControl:          equ 20 ; UINT8
AboutToLeaveSmm:         equ 21 ; UINT8

;
; SMRAM Save State Map field offsets, per the AMD (not Intel) layout that QEMU
; implements. Relative to SMM_DEFAULT_SMBASE.
;
SaveStateRevId:    equ 0xFEFC ; UINT32
SaveStateSmbase:   equ 0xFEF8 ; UINT32
SaveStateSmbase64: equ 0xFF00 ; UINT32

;
; CPUID constants, from "MdePkg/Include/Register/Intel/Cpuid.h".
;
CPUID_SIGNATURE:         equ 0x00
CPUID_EXTENDED_TOPOLOGY: equ 0x0B
CPUID_VERSION_INFO:      equ 0x01

;
; MSR constants, from "MdePkg/Include/Register/Intel/ArchitecturalMsr.h".
;
MSR_IA32_FEATURE_CONTROL: equ 0x0000003A

GLOBAL ASM_PFX (mFirstSmiHandler)     ; UINT8[]
GLOBAL ASM_PFX (mFirstSmiHandlerSize) ; UINT16

ASM_PFX (mFirstSmiHandler):
  ;
  ; Get our own APIC ID first, so we can contend for ApicIdGate.
  ;
  ; This basically reimplements GetInitialApicId() from
  ; "UefiCpuPkg/Library/BaseXApicLib/BaseXApicLib.c".
  ;
  mov eax, CPUID_SIGNATURE
  cpuid
  cmp eax, CPUID_EXTENDED_TOPOLOGY
  jb GetApicIdFromVersionInfo

  mov eax, CPUID_EXTENDED_TOPOLOGY
  mov ecx, 0
  cpuid
  test ebx, 0xFFFF
  jz GetApicIdFromVersionInfo

  ;
  ; EDX has the APIC ID, save it to ESI.
  ;
  mov esi, edx
  jmp KnockOnGate

GetApicIdFromVersionInfo:
  mov eax, CPUID_VERSION_INFO
  cpuid
  shr ebx, 24
  ;
  ; EBX has the APIC ID, save it to ESI.
  ;
  mov esi, ebx

KnockOnGate:
  ;
  ; See if ApicIdGate shows our own APIC ID. If so, swap it to MAX_UINT64
  ; (close the gate), and advance. Otherwise, keep knocking.
  ;
  ; InterlockedCompareExchange64():
  ; - Value                   := &FIRST_SMI_HANDLER_CONTEXT.ApicIdGate
  ; - CompareValue  (EDX:EAX) := APIC ID (from ESI)
  ; - ExchangeValue (ECX:EBX) := MAX_UINT64
  ;
  mov edx, 0
  mov eax, esi
  mov ecx, 0xFFFF_FFFF
  mov ebx, 0xFFFF_FFFF
  lock cmpxchg8b [ds : dword (SMM_DEFAULT_SMBASE + ApicIdGate)]
  jz ApicIdMatch
  pause
  jmp KnockOnGate

ApicIdMatch:
  ;
  ; Update the SMBASE field in the SMRAM Save State Map.
  ;
  ; First, calculate the address of the SMBASE field, based on the SMM Revision
  ; ID; store the result in EBX.
  ;
  mov eax, dword [ds : dword (SMM_DEFAULT_SMBASE + SaveStateRevId)]
  test eax, 0xFFFF
  jz LegacySaveStateMap

  mov ebx, SMM_DEFAULT_SMBASE + SaveStateSmbase64
  jmp UpdateSmbase

LegacySaveStateMap:
  mov ebx, SMM_DEFAULT_SMBASE + SaveStateSmbase

UpdateSmbase:
  ;
  ; Load the new SMBASE value into EAX.
  ;
  mov eax, dword [ds : dword (SMM_DEFAULT_SMBASE + NewSmbase)]
  ;
  ; Save it to the SMBASE field whose address we calculated in EBX.
  ;
  mov dword [ds : dword ebx], eax

  ;
  ; Set MSR_IA32_FEATURE_CONTROL if requested.
  ;
  cmp byte [ds : dword (SMM_DEFAULT_SMBASE + FeatureControl)], 0
  je NoFeatureControl
  mov ecx, MSR_IA32_FEATURE_CONTROL
  mov edx, dword [ds : dword (SMM_DEFAULT_SMBASE + FeatureControlHighValue)]
  mov eax, dword [ds : dword (SMM_DEFAULT_SMBASE + FeatureControlLowValue)]
  wrmsr

NoFeatureControl:
  ;
  ; Set AboutToLeaveSmm.
  ;
  mov byte [ds : dword (SMM_DEFAULT_SMBASE + AboutToLeaveSmm)], 1
  ;
  ; We're done; leave SMM and continue to the pen.
  ;
  rsm

ASM_PFX (mFirstSmiHandlerSize):
  dw $ - ASM_PFX (mFirstSmiHandler)
