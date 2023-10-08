/** @file
  Determine the base addresses of serial ports from the Device Tree.

  Copyright (C) Red Hat

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef FDT_SERIAL_PORT_ADDRESS_LIB_H_
#define FDT_SERIAL_PORT_ADDRESS_LIB_H_

#include <Base.h>

typedef struct {
  UINTN     NumberOfPorts;
  UINT64    BaseAddress[2];
} FDT_SERIAL_PORTS;

/**
  Collect the first ARRAY_SIZE (Ports->BaseAddress) serial ports into Ports from
  DeviceTree.

  @param[in] DeviceTree  The flat device tree (FDT) to scan.

  @param[in] Compatible  Look for Compatible in the "compatible" property of the
                         scanned nodes.

  @param[out] Ports      On successful return, Ports->NumberOfPorts contains the
                         number of serial ports found; it is (a) positive and
                         (b) at most ARRAY_SIZE (Ports->BaseAddress). If the FDT
                         had more serial ports, those are not reported. On
                         error, the contents of Ports are indeterminate.

  @retval RETURN_INVALID_PARAMETER  DeviceTree does not point to a valid FDT
                                    header.

  @retval RETURN_NOT_FOUND          No compatible and enabled serial port has
                                    been found.

  @retval RETURN_SUCCESS            At least one compatible and enabled serial
                                    port has been found; Ports has been filled
                                    in.
**/
RETURN_STATUS
EFIAPI
FdtSerialGetPorts (
  IN  CONST VOID        *DeviceTree,
  IN  CONST CHAR8       *Compatible,
  OUT FDT_SERIAL_PORTS  *Ports
  );

/**
  Fetch the base address of the serial port identified in the "stdout-path"
  property of the "/chosen" node in DeviceTree.

  @param[in] DeviceTree    The flat device tree (FDT) to scan.

  @param[out] BaseAddress  On success, the base address of the preferred serial
                           port (to be used as console). On error, BaseAddress
                           is not modified.

  @retval RETURN_INVALID_PARAMETER  DeviceTree does not point to a valid FDT
                                    header.

  @retval RETURN_NOT_FOUND          No enabled console port has been found.

  @retval RETURN_PROTOCOL_ERROR     The first (or only) node path in the
                                    "stdout-path" property is an empty string.

  @retval RETURN_PROTOCOL_ERROR     The console port has been found in the FDT,
                                    but its base address is not correctly
                                    represented.

  @retval RETURN_SUCCESS            BaseAddress has been populated.
**/
RETURN_STATUS
EFIAPI
FdtSerialGetConsolePort (
  IN  CONST VOID  *DeviceTree,
  OUT UINT64      *BaseAddress
  );

#endif
