;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
; Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

    EXPORT SecSwitchStack

    AREA   Switch_Stack, CODE, READONLY

;/**
;  This allows the caller to switch the stack and return
;
; @param      StackDelta     Signed amount by which to modify the stack pointer
;
; @return     Nothing. Goes to the Entry Point passing in the new parameters
;
;**/
;VOID
;EFIAPI
;SecSwitchStack (
;  VOID  *StackDelta
;  );
;
SecSwitchStack
    MOV   R1, SP
    ADD   R1, R0, R1
    MOV   SP, R1
    BX    LR
    END
