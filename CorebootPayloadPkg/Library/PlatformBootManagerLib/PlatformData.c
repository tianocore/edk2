/**@file
  Defined the platform specific device path which will be filled to
  ConIn/ConOut variables.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
