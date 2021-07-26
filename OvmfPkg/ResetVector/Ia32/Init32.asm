;------------------------------------------------------------------------------
; @file
;   32-bit initialization code
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS 32

;
; Modified:  EAX, EBX, ECX, EDX, EBP, EDI, ESP
;
Init32:
    ;
    ; Save EBX in EBP because EBX will be changed in ReloadFlat32
    ;
    mov     ebp, ebx

    ;
    ; First load the GDT and jump to Flat32 mode
    ;
    OneTimeCall ReloadFlat32

    ;
    ; Initialization of Tdx
    ;
    OneTimeCall  InitTdx

    OneTimeCallRet Init32

