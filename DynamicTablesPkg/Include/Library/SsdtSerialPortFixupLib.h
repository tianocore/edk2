/** @file
  Ssdt Serial Port Fixup Library

  Copyright (c) 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SSDT_SERIAL_PORT_LIB_H_
#define SSDT_SERIAL_PORT_LIB_H_

/** Build a SSDT table describing the input serial port.

  The table created by this function must be freed by FreeSsdtSerialTable.

  @param [in]  AcpiTableInfo    Pointer to the ACPI table information.
  @param [in]  SerialPortInfo   Serial port to describe in the SSDT table.
  @param [in]  Name             The Name to give to the Device.
                                Must be a NULL-terminated ASL NameString
                                e.g.: "DEV0", "DV15.DEV0", etc.
  @param [in]  Uid              UID for the Serial Port.
  @param [out] Table            If success, pointer to the created SSDT table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
EFI_STATUS
EFIAPI
BuildSsdtSerialPortTable (
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO       *AcpiTableInfo,
  IN  CONST CM_ARCH_COMMON_SERIAL_PORT_INFO  *SerialPortInfo,
  IN  CONST CHAR8                            *Name,
  IN  CONST UINT64                           Uid,
  OUT       EFI_ACPI_DESCRIPTION_HEADER      **Table
  );

/** Free an SSDT table previously created by
    the BuildSsdtSerialTable function.

  @param [in] Table   Pointer to a SSDT table allocated by
                      the BuildSsdtSerialTable function.

  @retval EFI_SUCCESS           Success.
**/
EFI_STATUS
EFIAPI
FreeSsdtSerialPortTable (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Table
  );

/** Validate the Serial Port Information.

  @param [in]  SerialPortInfoTable    Table of CM_ARCH_COMMON_SERIAL_PORT_INFO.
  @param [in]  SerialPortCount        Count of SerialPort in the table.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
ValidateSerialPortInfo (
  IN  CONST CM_ARCH_COMMON_SERIAL_PORT_INFO  *SerialPortInfoTable,
  IN        UINT32                           SerialPortCount
  );

#endif // SSDT_SERIAL_PORT_LIB_H_
