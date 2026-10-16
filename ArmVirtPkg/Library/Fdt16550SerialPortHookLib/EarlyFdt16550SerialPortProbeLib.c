/** @file
  Early FDT 16550 serial port probe constructor.

  Copyright (c) 2026, ARM Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/PlatformHookLib.h>

/**
  Early constructor to probe the FDT for the 16550 UART base address.

  This constructor invokes the platform serial hook so early boot code can
  populate PcdSerialRegisterBase before it is used.

  @retval RETURN_SUCCESS            The serial port base address was already
                                    configured or was found and stored.
  @retval RETURN_INVALID_PARAMETER  A parameter was invalid.
  @retval RETURN_NOT_FOUND          Serial port information was not found.
  @retval RETURN_PROTOCOL_ERROR     Invalid serial port information was found
                                    in the device tree.
**/
RETURN_STATUS
EFIAPI
EarlyFdt16550SerialPortProbeLibConstructor (
  VOID
  )
{
  return PlatformHookSerialPortInitialize ();
}
