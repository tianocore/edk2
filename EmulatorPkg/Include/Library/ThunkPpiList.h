/** @file
  All 3rd parties to register the PPIs passed into PEI Core

  Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

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
