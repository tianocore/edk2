;------------------------------------------------------------------------------
; Include file for IA32 MpFuncs.asm
;
; Copyright (c) 2009, Intel Corporation
; All rights reserved. This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

VacantFlag             Equ   00h
NotVacantFlag          Equ   0ffh

LockLocation        equ        RendezvousFunnelProcEnd - RendezvousFunnelProcStart
StackStart          equ        LockLocation + 4h
StackSize           equ        LockLocation + 8h
RendezvousProc      equ        LockLocation + 0Ch
GdtrProfile         equ        LockLocation + 10h
BufferStart         equ        LockLocation + 18h

;-------------------------------------------------------------------------------
