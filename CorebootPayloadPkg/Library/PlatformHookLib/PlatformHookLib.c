/** @file
  Platform Hook Library instance for UART device upon coreboot.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <Library/PlatformHookLib.h>
#include <Library/CbParseLib.h>
#include <Library/PcdLib.h>

/**
  Performs platform specific initialization required for the CPU to access
  the hardware associated with a SerialPortLib instance.  This function does
  not intiailzie the serial port hardware itself.  Instead, it initializes
  hardware devices that are required for the CPU to access the serial port
  hardware.  This function may be called more than once.

  @retval RETURN_SUCCESS       The platform specific initialization succeeded.
  @retval RETURN_DEVICE_ERROR  The platform specific initialization could not be completed.

**/
RETURN_STATUS
EFIAPI
PlatformHookSerialPortInitialize (
  VOID
  )
{
  RETURN_STATUS     Status;
  UINT32            SerialRegBase;
  UINT32            SerialRegAccessType;

  Status = CbParseSerialInfo (&SerialRegBase, &SerialRegAccessType, NULL);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  if (SerialRegAccessType == 2) { //MMIO
    Status = PcdSetBoolS (PcdSerialUseMmio, TRUE);
  } else { //IO
    Status = PcdSetBoolS (PcdSerialUseMmio, FALSE);
  }
  if (RETURN_ERROR (Status)) {
    return Status;
  }
  Status = PcdSet64S (PcdSerialRegisterBase, (UINT64) SerialRegBase);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  return RETURN_SUCCESS;
}

