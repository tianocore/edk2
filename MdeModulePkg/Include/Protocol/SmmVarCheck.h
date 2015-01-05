/** @file
  SMM variable check definitions, it reuses the interface definitions of variable check.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SMM_VAR_CHECK_H__
#define __SMM_VAR_CHECK_H__

#include <Protocol/VarCheck.h>

#define EDKII_SMM_VAR_CHECK_PROTOCOL_GUID \
  { \
    0xb0d8f3c1, 0xb7de, 0x4c11, { 0xbc, 0x89, 0x2f, 0xb5, 0x62, 0xc8, 0xc4, 0x11 } \
  };

typedef struct _EDKII_SMM_VAR_CHECK_PROTOCOL EDKII_SMM_VAR_CHECK_PROTOCOL;

struct _EDKII_SMM_VAR_CHECK_PROTOCOL {
  EDKII_VAR_CHECK_REGISTER_SET_VARIABLE_CHECK_HANDLER   SmmRegisterSetVariableCheckHandler;
  EDKII_VAR_CHECK_VARIABLE_PROPERTY_SET                 SmmVariablePropertySet;
  EDKII_VAR_CHECK_VARIABLE_PROPERTY_GET                 SmmVariablePropertyGet;
};

extern EFI_GUID gEdkiiSmmVarCheckProtocolGuid;

#endif

