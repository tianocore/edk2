/** @file
  UEFI OS based application for unit testing the DevicePathLib.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <PiPei.h>
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>
#include <Protocol/DevicePath.h>
#include <Library/DevicePathLib.h>
#include <stdint.h>

EFI_STATUS
CreateDevicePathStringConversionsTestSuite (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework
  );
