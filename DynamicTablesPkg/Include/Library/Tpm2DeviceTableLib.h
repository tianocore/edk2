/** @file
  Tpm2 device table generating Library

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TPM2_DEVICE_TABLE_LIB_H_
#define TPM2_DEVICE_TABLE_LIB_H_

/** Build a SSDT table describing the TPM2 device.

  The table created by this function must be freed by FreeTpm2DeviceTable.

  @param [in]  TpmDevInfo       TPM2 Device info to describe in the SSDT table.
  @param [in]  Name             The Name to give to the Device.
                                Must be a NULL-terminated ASL NameString
                                e.g.: "DEV0", "DV15.DEV0", etc.
  @param [in]  Uid              UID for the TPM@ device.
  @param [out] Table            If success, pointer to the created SSDT table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
EFI_STATUS
EFIAPI
BuildTpm2DeviceTable (
  IN  CONST CM_ARCH_COMMON_TPM2_DEVICE_INFO  *TpmDevInfo,
  IN  CONST CHAR8                            *Name,
  IN  CONST UINT64                           Uid,
  OUT       EFI_ACPI_DESCRIPTION_HEADER      **Table
  );

/** Free an Tpm2 device table previously created by
    the BuildTpm2DeviceTable function.

  @param [in] Table   Pointer to a Tpm2 Device table allocated by
                      the BuildTpm2DeviceTable function.

**/
VOID
EFIAPI
FreeTpm2DeviceTable (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Table
  );

#endif // TPM2_DEVICE_TABLE_LIB_H_
