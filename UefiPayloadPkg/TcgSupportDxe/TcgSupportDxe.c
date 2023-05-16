/** @file
  This module will provide bootloader support TCG configurations.

  Copyright (c) 22023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "TcgSupportDxe.h"

/**
  Uninstall TPM2 SSDT ACPI table

  This performs uninstallation of TPM2 SSDT tables published by
  bootloaders.

  @retval   EFI_SUCCESS     The TPM2 ACPI table is uninstalled successfully if found.
  @retval   Others          Operation error.

**/
EFI_STATUS
UnInstallTpm2SSDTAcpiTables (
  )
{
  UINTN                    TableIndex;
  UINTN                    TableKey;
  EFI_ACPI_TABLE_VERSION   TableVersion;
  VOID                     *TableHeader;
  EFI_STATUS               Status;
  EFI_ACPI_SDT_PROTOCOL    *mAcpiSdtProtocol;
  EFI_ACPI_TABLE_PROTOCOL  *mAcpiTableProtocol;
  CHAR8                    TableIdString[8];
  UINT64                   TableIdSignature;

  //
  // Determine whether there is a TPM2 SSDT already in the ACPI table.
  //
  Status             = EFI_SUCCESS;
  TableIndex         = 0;
  TableKey           = 0;
  TableHeader        = NULL;
  mAcpiTableProtocol = NULL;
  mAcpiSdtProtocol   = NULL;

  //
  // Locate the EFI_ACPI_TABLE_PROTOCOL.
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&mAcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "UnInstallTpm2SSDTAcpiTables: Cannot locate the EFI ACPI Table Protocol \n "
      ));
    return Status;
  }

  //
  // Locate the EFI_ACPI_SDT_PROTOCOL.
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **)&mAcpiSdtProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "UnInstallTpm2SSDTAcpiTables: Cannot locate the EFI ACPI Sdt Protocol, "
      "\n"
      ));
    return Status;
  }

  while (!EFI_ERROR (Status)) {
    Status = mAcpiSdtProtocol->GetAcpiTable (
                                 TableIndex,
                                 (EFI_ACPI_SDT_HEADER **)&TableHeader,
                                 &TableVersion,
                                 &TableKey
                                 );

    if (!EFI_ERROR (Status)) {
      TableIndex++;

      if (((EFI_ACPI_SDT_HEADER *)TableHeader)->Signature == SIGNATURE_32 ('S', 'S', 'D', 'T')) {
        CopyMem ((VOID *)TableIdString, (VOID *)((EFI_ACPI_SDT_HEADER *)TableHeader)->OemTableId, sizeof (TableIdString));

        TableIdSignature = SIGNATURE_64 (
                             TableIdString[0],
                             TableIdString[1],
                             TableIdString[2],
                             TableIdString[3],
                             TableIdString[4],
                             TableIdString[5],
                             TableIdString[6],
                             TableIdString[7]
                             );

        if (TableIdSignature == SIGNATURE_64 ('T', 'p', 'm', '2', 'T', 'a', 'b', 'l')) {
          DEBUG ((DEBUG_INFO, "Found Tpm2 SSDT Table for Physical Presence\n"));
          break;
        }
      }
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // A TPM2 SSDT is already in the ACPI table.
    //
    DEBUG ((
      DEBUG_INFO,
      "A TPM2 SSDT is already exist in the ACPI Table.\n"
      ));

    //
    // Uninstall the origin TPM2 SSDT from the ACPI table.
    //
    Status = mAcpiTableProtocol->UninstallAcpiTable (
                                   mAcpiTableProtocol,
                                   TableKey
                                   );
    ASSERT_EFI_ERROR (Status);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "UnInstall Tpm2SSDTAcpiTables failed \n "));

      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Uninstall TPM2 table

  This performs uninstallation of TPM2 tables published by
  bootloaders.

  @retval   EFI_SUCCESS     The TPM2 table is uninstalled successfully if its found.
  @retval   Others          Operation error.

**/
EFI_STATUS
UnInstallTpm2Tables (
  )
{
  UINTN                    TableIndex;
  UINTN                    TableKey;
  EFI_ACPI_TABLE_VERSION   TableVersion;
  VOID                     *TableHeader;
  EFI_STATUS               Status;
  EFI_ACPI_SDT_PROTOCOL    *mAcpiSdtProtocol;
  EFI_ACPI_TABLE_PROTOCOL  *mAcpiTableProtocol;

  //
  // Determine whether there is a TPM2 SSDT already in the ACPI table.
  //
  Status             = EFI_SUCCESS;
  TableIndex         = 0;
  TableKey           = 0;
  TableHeader        = NULL;
  mAcpiTableProtocol = NULL;
  mAcpiSdtProtocol   = NULL;

  //
  // Locate the EFI_ACPI_TABLE_PROTOCOL.
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&mAcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "UnInstallTpm2Tables: Cannot locate the EFI ACPI Table Protocol \n "
      ));
    return Status;
  }

  //
  // Locate the EFI_ACPI_SDT_PROTOCOL.
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid,
                  NULL,
                  (VOID **)&mAcpiSdtProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "UnInstallTpm2Tables: Cannot locate the EFI ACPI Sdt Protocol, "
      "\n"
      ));
    return Status;
  }

  while (!EFI_ERROR (Status)) {
    Status = mAcpiSdtProtocol->GetAcpiTable (
                                 TableIndex,
                                 (EFI_ACPI_SDT_HEADER **)&TableHeader,
                                 &TableVersion,
                                 &TableKey
                                 );

    if (!EFI_ERROR (Status)) {
      TableIndex++;

      if (((EFI_ACPI_SDT_HEADER *)TableHeader)->Signature == EFI_ACPI_5_0_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE ) {
        DEBUG ((DEBUG_INFO, "Found Tpm2 Table ..\n"));
        break;
      }
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // A TPM2 SSDT is already in the ACPI table.
    //
    DEBUG ((
      DEBUG_INFO,
      "A TPM2 table  is already exist in the ACPI Table.\n"
      ));

    //
    // Uninstall the origin TPM2 SSDT from the ACPI table.
    //
    Status = mAcpiTableProtocol->UninstallAcpiTable (
                                   mAcpiTableProtocol,
                                   TableKey
                                   );
    ASSERT_EFI_ERROR (Status);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "UnInstall Tpm2Tables failed \n "));

      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  The driver's entry point.

  It patches and installs ACPI tables used for handling TPM physical presence
  and Memory Clear requests through ACPI method.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval Others          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
TcgSupportEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Bootloader might pulish the TPM2 ACPT tables
  // Uninstall TPM tables if it exists
  //
  Status = UnInstallTpm2SSDTAcpiTables ();
  ASSERT_EFI_ERROR (Status);

  Status = UnInstallTpm2Tables ();
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
