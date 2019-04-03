/** @file
  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  Module Name:  DriverSupportEfiVersion.c

**/
#include "AtapiPassThru.h"

EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL gAtapiScsiPassThruDriverSupportedEfiVersion = {
  sizeof (EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL), // Size of Protocol structure.
  0                                                   // Version number to be filled at start up.
};

