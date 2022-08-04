;------------------------------------------------------------------------------ ;
; Copyright (c) 2015 - 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   MpEqu.inc
;
; Abstract:
;
;   This is the equates file for Multiple Processor support
;
;-------------------------------------------------------------------------------
%include "Nasm.inc"

CPU_SWITCH_STATE_IDLE         equ        0
CPU_SWITCH_STATE_STORED       equ        1
CPU_SWITCH_STATE_LOADED       equ        2

;
; Equivalent NASM structure of MP_ASSEMBLY_ADDRESS_MAP
;
struc MP_ASSEMBLY_ADDRESS_MAP
  .RendezvousFunnelAddress       CTYPE_UINTN 1
  .ModeEntryOffset               CTYPE_UINTN 1
  .RendezvousFunnelSize          CTYPE_UINTN 1
  .RelocateApLoopFuncAddress     CTYPE_UINTN 1
  .RelocateApLoopFuncSize        CTYPE_UINTN 1
  .ModeTransitionOffset          CTYPE_UINTN 1
  .SwitchToRealNoNxOffset        CTYPE_UINTN 1
  .SwitchToRealPM16ModeOffset    CTYPE_UINTN 1
  .SwitchToRealPM16ModeSize      CTYPE_UINTN 1
endstruc

;
; Equivalent NASM structure of IA32_DESCRIPTOR
;
struc IA32_DESCRIPTOR
  .Limit                         CTYPE_UINT16 1
  .Base                          CTYPE_UINTN  1
endstruc

;
; Equivalent NASM structure of CPU_EXCHANGE_ROLE_INFO
;
struc CPU_EXCHANGE_ROLE_INFO
  ; State is defined as UINT8 in C header file
  ; Define it as UINTN here to guarantee the fields that follow State
  ; is naturally aligned. The structure layout doesn't change.
  .State                         CTYPE_UINTN 1
  .StackPointer                  CTYPE_UINTN 1
  .Gdtr                          CTYPE_UINT8 IA32_DESCRIPTOR_size
  .Idtr                          CTYPE_UINT8 IA32_DESCRIPTOR_size
endstruc

;
; Equivalent NASM structure of CPU_INFO_IN_HOB
;
struc CPU_INFO_IN_HOB
  .InitialApicId                 CTYPE_UINT32 1
  .ApicId                        CTYPE_UINT32 1
  .Health                        CTYPE_UINT32 1
  .ApTopOfStack                  CTYPE_UINT64 1
endstruc

;
; Equivalent NASM structure of MP_CPU_EXCHANGE_INFO
;
struc MP_CPU_EXCHANGE_INFO
  .StackStart:                   CTYPE_UINTN 1
  .StackSize:                    CTYPE_UINTN 1
  .CFunction:                    CTYPE_UINTN 1
  .GdtrProfile:                  CTYPE_UINT8 IA32_DESCRIPTOR_size
  .IdtrProfile:                  CTYPE_UINT8 IA32_DESCRIPTOR_size
  .BufferStart:                  CTYPE_UINTN 1
  .ModeOffset:                   CTYPE_UINTN 1
  .ApIndex:                      CTYPE_UINTN 1
  .CodeSegment:                  CTYPE_UINTN 1
  .DataSegment:                  CTYPE_UINTN 1
  .EnableExecuteDisable:         CTYPE_UINTN 1
  .Cr3:                          CTYPE_UINTN 1
  .InitFlag:                     CTYPE_UINTN 1
  .CpuInfo:                      CTYPE_UINTN 1
  .NumApsExecuting:              CTYPE_UINTN 1
  .CpuMpData:                    CTYPE_UINTN 1
  .InitializeFloatingPointUnits: CTYPE_UINTN 1
  .ModeTransitionMemory:         CTYPE_UINT32 1
  .ModeTransitionSegment:        CTYPE_UINT16 1
  .ModeHighMemory:               CTYPE_UINT32 1
  .ModeHighSegment:              CTYPE_UINT16 1
  .Enable5LevelPaging:           CTYPE_BOOLEAN 1
  .SevEsIsEnabled:               CTYPE_BOOLEAN 1
  .SevSnpIsEnabled               CTYPE_BOOLEAN 1
  .GhcbBase:                     CTYPE_UINTN 1
  .ExtTopoAvail:                 CTYPE_BOOLEAN 1
endstruc

MP_CPU_EXCHANGE_INFO_OFFSET equ (Flat32Start - RendezvousFunnelProcStart)
%define MP_CPU_EXCHANGE_INFO_FIELD(Field) (MP_CPU_EXCHANGE_INFO_OFFSET + MP_CPU_EXCHANGE_INFO. %+ Field)
