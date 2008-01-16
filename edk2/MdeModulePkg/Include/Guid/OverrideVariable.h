/*++

Copyright (c) 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  OverrideVariable.h

Abstract:

  Platform Override Variable Guid definitions

--*/

#ifndef __EFI_OVERRIDE_VARIABLE_GUID_H__
#define __EFI_OVERRIDE_VARIABLE_GUID_H__

//
// This guid is used for a platform driver override variable
//
#define EFI_OVERRIDE_VARIABLE_GUID  \
  { 0x8e3d4ad5, 0xf762, 0x438a, { 0xa1, 0xc1, 0x5b, 0x9f, 0xe6, 0x8c, 0x6b, 0x15 }}

extern EFI_GUID gEfiOverrideVariableGuid;


#endif // #ifndef __EFI_OVERRIDE_VARIABLE_GUID_H__
