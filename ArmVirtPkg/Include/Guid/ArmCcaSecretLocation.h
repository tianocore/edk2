/** @file
  Arm CCA Secret Location GUID and defnitions.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi/UefiBaseType.h>

/**
  A structure representing the Arm CCA Secret Location.
**/
typedef struct {
  /// Base address of the secret location.
  EFI_PHYSICAL_ADDRESS    Base;
  /// Length of the secret location.
  UINT64                  Size;
} ARM_CCA_SECRET_LOCATION;

/**
  A GUID identifying the Arm CCA Secret Location.
*/
extern EFI_GUID  gArmBootSyncSecretMemoryLocationGuid;
