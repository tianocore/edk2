;------------------------------------------------------------------------------
; @file
;   Tdx Initialization.
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS 32

InitTdx:
    nop
doneTdxInit:
    OneTimeCallRet InitTdx
