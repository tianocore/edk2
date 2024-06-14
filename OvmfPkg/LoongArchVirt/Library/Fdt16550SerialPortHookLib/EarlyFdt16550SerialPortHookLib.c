/** @file
  PEI Phase Early Platform Hook Library instance for 16550 Uart.

  Copyright (c) 2020 - 2023, Arm Ltd. All rights reserved.<BR>
  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/FdtSerialPortAddressLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformHookLib.h>
#include <Register/LoongArch64/Csr.h>

/** Platform hook to retrieve the 16550 UART base address from the platform
    Device tree and store it in the reigster LOONGARCH_CSR_KS1.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter was invalid.
  @retval RETURN_NOT_FOUND          Serial port information not found.

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

  CsrWrite (LOONGARCH_CSR_KS1, (UINTN)SerialConsoleAddress);

  return RETURN_SUCCESS;
}
