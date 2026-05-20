/** @file
  Header file for Dxe Core Debug Agent Library instance.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <PiDxe.h>

#include <Protocol/SerialIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PiPcd.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

#include "DebugAgent.h"

/**
  Install EFI Serial IO protocol based on Debug Communication Library.

**/
VOID
InstallSerialIo (
  VOID
  );
