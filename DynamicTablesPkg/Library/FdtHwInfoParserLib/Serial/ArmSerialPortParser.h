/** @file
  Arm Serial Port Parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/serial/serial.yaml
  - linux/Documentation/devicetree/bindings/serial/8250.txt
**/

#ifndef ARM_SERIAL_PORT_PARSER_H_
#define ARM_SERIAL_PORT_PARSER_H_

/** SerialPort dispatcher.

  This disptacher populates the CM_ARM_SERIAL_PORT_INFO structure for
  the following CM_OBJ_ID:
   - EArmObjSerialConsolePortInfo
   - EArmObjSerialDebugPortInfo
   - EArmObjSerialPortInfo

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

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
  );

#endif // ARM_SERIAL_PORT_PARSER_H_
