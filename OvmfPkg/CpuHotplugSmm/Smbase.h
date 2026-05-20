/** @file
  SMBASE relocation for hot-plugged CPUs.

  Copyright (c) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi/UefiBaseType.h> // EFI_STATUS
#include <Uefi/UefiSpec.h>     // EFI_BOOT_SERVICES

#include "ApicId.h"            // APIC_ID

EFI_STATUS
SmbaseAllocatePostSmmPen (
  OUT UINT32                   *PenAddress,
  IN  CONST EFI_BOOT_SERVICES  *BootServices
  );

VOID
SmbaseReinstallPostSmmPen (
  IN UINT32  PenAddress
  );

VOID
SmbaseReleasePostSmmPen (
  IN UINT32                   PenAddress,
  IN CONST EFI_BOOT_SERVICES  *BootServices
  );

VOID
SmbaseInstallFirstSmiHandler (
  VOID
  );

EFI_STATUS
SmbaseRelocate (
  IN APIC_ID  ApicId,
  IN UINTN    Smbase,
  IN UINT32   PenAddress
  );
