/** @file
  SMBASE relocation for hot-plugged CPUs.

  Copyright (c) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SMBASE_H_
#define SMBASE_H_

#include <Uefi/UefiBaseType.h> // EFI_STATUS
#include <Uefi/UefiSpec.h>     // EFI_BOOT_SERVICES

EFI_STATUS
SmbaseAllocatePostSmmPen (
  OUT UINT32                  *PenAddress,
  IN  CONST EFI_BOOT_SERVICES *BootServices
  );

VOID
SmbaseReinstallPostSmmPen (
  IN UINT32 PenAddress
  );

VOID
SmbaseReleasePostSmmPen (
  IN UINT32                  PenAddress,
  IN CONST EFI_BOOT_SERVICES *BootServices
  );

#endif // SMBASE_H_
