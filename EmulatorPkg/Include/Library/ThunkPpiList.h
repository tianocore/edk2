/** @file
  All 3rd parties to register the PPIs passed into PEI Core

  Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>


EFI_PEI_PPI_DESCRIPTOR *
GetThunkPpiList (
  VOID
  );


EFI_STATUS
EFIAPI
AddThunkPpi (
  IN  UINTN     Flags,
  IN  EFI_GUID  *Guid,
  IN  VOID      *Ppi
  );


