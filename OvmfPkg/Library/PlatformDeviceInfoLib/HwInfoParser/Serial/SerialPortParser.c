/** @file
  Serial Port Parser.

  Copyright (c) 2021 - 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/serial/serial.yaml
  - linux/Documentation/devicetree/bindings/serial/8250.txt
  - linux/Documentation/devicetree/bindings/serial/arm_sbsa_uart.txt
  - linux/Documentation/devicetree/bindings/serial/pl011.yaml
**/

#include "FdtUtility.h"
#include "FdtInfoParser.h"
#include "Common/DeviceParser.h"

/** List of "compatible" property values for serial port nodes.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  SerialCompatibleStr[] = {
  { "ns16550a"      },
  { "arm,sbsa-uart" },
  { "arm,pl011"     }
};

/** COMPATIBILITY_INFO structure for the SerialCompatible.
*/
CONST COMPATIBILITY_INFO  SerialCompatibleInfo = {
  ARRAY_SIZE (SerialCompatibleStr),
  SerialCompatibleStr
};

/** SerialPort dispatcher.

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
SerialPortDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  return DeviceDispatcher (
           FdtParserHandle,
           FdtBranch,
           &SerialCompatibleInfo,
           "Serial Port"
           );
}
