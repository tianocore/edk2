/** @file
  Early Platform Hook Library instance for 16550 Uart.

  Copyright (c) 2020 - 2023, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>

#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformHookLib.h>
#include <Library/FdtSerialPortAddressLib.h>

/** Platform hook to retrieve the 16550 UART base address from the platform
    Device tree and store it in PcdSerialRegisterBase.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter was invalid.
  @retval RETURN_NOT_FOUND          Serial port information not found.
  @retval RETURN_PROTOCOL_ERROR     Invalid information in the Device Tree.

**/
RETURN_STATUS
EFIAPI
PlatformHookSerialPortInitialize (
  VOID
  )
{
  RETURN_STATUS  Status;
  VOID           *DeviceTreeBase;
  UINT64         SerialConsoleAddress;

  if (PcdGet64 (PcdSerialRegisterBase) != 0) {
    return RETURN_SUCCESS;
  }

  DeviceTreeBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  if (DeviceTreeBase == NULL) {
    return RETURN_NOT_FOUND;
  }

  Status = FdtSerialGetConsolePort (DeviceTreeBase, &SerialConsoleAddress);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  return (EFI_STATUS)PcdSet64S (PcdSerialRegisterBase, SerialConsoleAddress);
}
