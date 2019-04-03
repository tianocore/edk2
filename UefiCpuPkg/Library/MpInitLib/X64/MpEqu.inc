;------------------------------------------------------------------------------ ;
; Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
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

VacantFlag                    equ        00h
NotVacantFlag                 equ        0ffh

CPU_SWITCH_STATE_IDLE         equ        0
CPU_SWITCH_STATE_STORED       equ        1
CPU_SWITCH_STATE_LOADED       equ        2

LockLocation                  equ        (RendezvousFunnelProcEnd - RendezvousFunnelProcStart)
StackStartAddressLocation     equ        LockLocation + 08h
StackSizeLocation             equ        LockLocation + 10h
ApProcedureLocation           equ        LockLocation + 18h
GdtrLocation                  equ        LockLocation + 20h
IdtrLocation                  equ        LockLocation + 2Ah
BufferStartLocation           equ        LockLocation + 34h
ModeOffsetLocation            equ        LockLocation + 3Ch
ApIndexLocation               equ        LockLocation + 44h
CodeSegmentLocation           equ        LockLocation + 4Ch
DataSegmentLocation           equ        LockLocation + 54h
EnableExecuteDisableLocation  equ        LockLocation + 5Ch
Cr3Location                   equ        LockLocation + 64h
InitFlagLocation              equ        LockLocation + 6Ch
CpuInfoLocation               equ        LockLocation + 74h
NumApsExecutingLocation       equ        LockLocation + 7Ch
InitializeFloatingPointUnitsAddress equ  LockLocation + 8Ch
ModeTransitionMemoryLocation        equ  LockLocation + 94h
ModeTransitionSegmentLocation       equ  LockLocation + 98h
ModeHighMemoryLocation              equ  LockLocation + 9Ah
ModeHighSegmentLocation             equ  LockLocation + 9Eh
