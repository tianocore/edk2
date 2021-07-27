;------------------------------------------------------------------------------
; @file
; Transition from 32 bit flat protected mode into 64 bit flat protected mode
;
; Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
; Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    32

;
; Transition from 32 bit flat protected mode into 64 bit flag protected mode
;
; To handle the situations of Tdx/SEV/Legacy guests, Pre/Post routines are
; called. For example, SevPreSetCr3ForPageTables64 check the Sev features
; and set the EAX value. TdxPostSetCr3PageTables64 set the CR0/CR4 and adjust
; the CR3 if GPAW is 52.
;
; But in Tdx guest, memory region cannot be accessed before it is accepted
; (except the case that the memory region is initialized by host VMM before
; the guest is launched.) So in the beginning of Pre/Post routines it would
; check if it is Tdx guest by checking the TDX_WORK_AREA.
;
; Modified:  EAX, EBX, ECX, EDX, ESP
;
Transition32FlatTo64Flat:

    OneTimeCall PreSetCr3ForPageTables64Sev

SetPageTables64:
    OneTimeCall SetCr3ForPageTables64

    OneTimeCall PostSetCr3PageTables64Tdx

    OneTimeCall PostSetCr3PageTables64Sev

    jmp     LINEAR_CODE64_SEL:ADDR_OF(jumpTo64BitAndLandHere)

BITS    64

jumpTo64BitAndLandHere:

    OneTimeCall PostJump64BitAndLandHereSev

    debugShowPostCode POSTCODE_64BIT_MODE

    OneTimeCallRet Transition32FlatTo64Flat

