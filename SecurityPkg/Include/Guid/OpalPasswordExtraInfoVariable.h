/** @file
  Defines Name GUIDs to represent an Opal device variable guid for Opal Security Feature.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _OPAL_PASSWORD_EXTRA_INFO_VARIABLE_H_
#define _OPAL_PASSWORD_EXTRA_INFO_VARIABLE_H_

#define OPAL_EXTRA_INFO_VAR_NAME L"OpalExtraInfo"

typedef struct {
  UINT8   EnableBlockSid;
} OPAL_EXTRA_INFO_VAR;

extern EFI_GUID gOpalExtraInfoVariableGuid;

#endif // _OPAL_PASSWORD_SECURITY_VARIABLE_H_

