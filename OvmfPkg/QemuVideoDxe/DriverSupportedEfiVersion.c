/** @file
  Driver supported version protocol for the QEMU video driver.

  Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "Qemu.h"

EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL gQemuVideoDriverSupportedEfiVersion = {
  sizeof (EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL), // Size of Protocol structure.
  0                                                   // Version number to be filled at start up.
};

