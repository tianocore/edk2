/*++

Copyright (c) 1999 - 2002, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  SmmAccess.c

Abstract:

  This file defines SMM SMRAM abstraction protocol defined by the 
  SMM Component Interface Specification

 
--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (SmmAccess)

EFI_GUID  gEfiSmmAccessProtocolGuid = EFI_SMM_ACCESS_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiSmmAccessProtocolGuid, "SMM Access Protocol", "SMM Access protocol");
