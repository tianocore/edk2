/**@file
  Defined the platform specific device path which will be filled to
  ConIn/ConOut variables.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PlatformBootManager.h"

///
/// Predefined platform default console device path
///
PLATFORM_CONSOLE_CONNECT_ENTRY   gPlatformConsole[] = {
  {
    NULL,
    0
  }
};
