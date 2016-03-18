//------------------------------------------------------------------------------
//
// Copyright (c) 2015, Linaro Limited. All rights reserved.
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------

    EXPORT      __aeabi_cfrcmple
    EXPORT      __aeabi_cfcmpeq
    EXPORT      __aeabi_cfcmple
    IMPORT      _softfloat_float32_eq
    IMPORT      _softfloat_float32_lt

    AREA        __aeabi_cfcmp, CODE, READONLY
    PRESERVE8

__aeabi_cfrcmple
    MOV         IP, R0
    MOV         R0, R1
    MOV         R1, IP

__aeabi_cfcmpeq
__aeabi_cfcmple
    PUSH        {R0 - R3, IP, LR}
    BL          _softfloat_float32_eq
    SUB         IP, R0, #1
    CMP         IP, #0                  // sets C and Z if R0 == 1
    POPEQ       {R0 - R3, IP, PC}

    LDM         SP, {R0 - R1}
    BL          _softfloat_float32_lt
    SUB         IP, R0, #1
    CMP         IP, #1                  // sets C if R0 == 0
    POP         {R0 - R3, IP, PC}

    END
