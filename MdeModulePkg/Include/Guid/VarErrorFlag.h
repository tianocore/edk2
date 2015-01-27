/** @file
  Variable error flag definitions.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VARIABLE_ERROR_FLAG_H_
#define _VARIABLE_ERROR_FLAG_H_

//
// Before EndOfDxe, the variable indicates the last boot variable error flag,
// then it means the last boot variable error flag must be got before EndOfDxe.
// After EndOfDxe, the variable indicates the current boot variable error flag,
// then it means the current boot variable error flag must be got after EndOfDxe.
//
// If the variable is not present, it has the same meaning with VAR_ERROR_FLAG_NO_ERROR.
//
#define VAR_ERROR_FLAG_NAME             L"VarErrorFlag"

#define VAR_ERROR_FLAG_NO_ERROR         0xFF // 1111-1111
#define VAR_ERROR_FLAG_SYSTEM_ERROR     0xEF // 1110-1111
#define VAR_ERROR_FLAG_USER_ERROR       0xFE // 1111-1110

typedef UINT8 VAR_ERROR_FLAG;

#define EDKII_VAR_ERROR_FLAG_GUID { \
  0x4b37fe8, 0xf6ae, 0x480b, { 0xbd, 0xd5, 0x37, 0xd9, 0x8c, 0x5e, 0x89, 0xaa } \
};

extern EFI_GUID gEdkiiVarErrorFlagGuid;

#endif

