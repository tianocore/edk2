/** @file
  Platform Hook Library instance for 16550 Uart.

  Copyright (c) 2020, ARM Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>

#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>

#include <Guid/Early16550UartBaseAddress.h>
#include <Guid/Fdt.h>
#include <Guid/FdtHob.h>

#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformHookLib.h>

/** Platform hook to retrieve the 16550 UART base address from the GUID Hob
    that caches the UART base address from early boot stage and store it in
    PcdSerialRegisterBase.

  @retval RETURN_SUCCESS    Success.
  @retval RETURN_NOT_FOUND  Serial Port information not found.

**/
RETURN_STATUS
EFIAPI
PlatformHookSerialPortInitialize (
  VOID
  )
{
  VOID            *Hob;
  UINT64          *UartBase;

  if (PcdGet64 (PcdSerialRegisterBase) != 0) {
    return RETURN_SUCCESS;
  }

  Hob = GetFirstGuidHob (&gEarly16550UartBaseAddressGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (*UartBase))) {
    return RETURN_NOT_FOUND;
  }

  UartBase = GET_GUID_HOB_DATA (Hob);
  if ((UINTN)*UartBase == 0) {
    return RETURN_NOT_FOUND;
  }

  return (RETURN_STATUS)PcdSet64S (PcdSerialRegisterBase, (UINTN)*UartBase);
}
