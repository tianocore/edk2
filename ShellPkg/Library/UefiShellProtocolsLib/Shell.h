/** @file
  Provides interface to the shell protocols.

  Copyright (c) 2024, 9elements GmbH.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SHELL_INTERNAL_HEADER_
#define _SHELL_INTERNAL_HEADER_

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellProtocolsLib.h>

#include <Protocol/Shell.h>
#include <Protocol/ShellParameters.h>

#include <ShellInternals.h>

/**
  This function will populate the 2 device path protocol parameters based on the
  global gImageHandle.  the DevPath will point to the device path for the handle that has
  loaded image protocol installed on it.  the FilePath will point to the device path
  for the file that was loaded.

  @param[in, out] DevPath       on a successful return the device path to the loaded image
  @param[in, out] FilePath      on a successful return the device path to the file

  @retval EFI_SUCCESS           the 2 device paths were successfully returned.
  @return other                 a error from gBS->HandleProtocol

  @sa HandleProtocol
**/
EFI_STATUS
GetDevicePathsForImageAndFile (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevPath,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
  );

#endif
