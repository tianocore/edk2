;------------------------------------------------------------------------------ ;
; Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
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
StackStartAddressLocation     equ        LockLocation + 04h
StackSizeLocation             equ        LockLocation + 08h
ApProcedureLocation           equ        LockLocation + 0Ch
GdtrLocation                  equ        LockLocation + 10h
IdtrLocation                  equ        LockLocation + 16h
BufferStartLocation           equ        LockLocation + 1Ch
ModeOffsetLocation            equ        LockLocation + 20h
ApIndexLocation               equ        LockLocation + 24h
CodeSegmentLocation           equ        LockLocation + 28h
DataSegmentLocation           equ        LockLocation + 2Ch
EnableExecuteDisableLocation  equ        LockLocation + 30h
Cr3Location                   equ        LockLocation + 34h
InitFlagLocation              equ        LockLocation + 38h
CpuInfoLocation               equ        LockLocation + 3Ch
NumApsExecutingLocation       equ        LockLocation + 40h
InitializeFloatingPointUnitsAddress equ  LockLocation + 48h
ModeTransitionMemoryLocation        equ  LockLocation + 4Ch
ModeTransitionSegmentLocation       equ  LockLocation + 50h
ModeHighMemoryLocation              equ  LockLocation + 52h
ModeHighSegmentLocation             equ  LockLocation + 56h

