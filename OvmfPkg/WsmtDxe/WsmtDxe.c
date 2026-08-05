/** @file
  Install the WSMT (Windows SMM Security Mitigation Table) ACPI table.

  The WSMT table informs the OS that the firmware's SMM implementation
  satisfies specific security properties.

  Copyright (c) 2026, Nutanix, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/WindowsSmmSecurityMitigationTable.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>

STATIC EFI_ACPI_WSMT_TABLE  mWsmtTable = {
  {
    EFI_ACPI_WINDOWS_SMM_SECURITY_MITIGATION_TABLE_SIGNATURE,
    sizeof (EFI_ACPI_WSMT_TABLE),        // Length of the WSMT table, must be 40 for Revision 1
    EFI_WSMT_TABLE_REVISION,             // Revision 1 of the WSMT table
    0,                                   // Checksum - filled by InstallAcpiTable
    //
    // It is expected that these values will be updated at EntryPoint.
    //
    { 0x00 },   // OEM ID is a 6 bytes long field
    0x00,       // OEM Table ID (8 bytes long)
    0x00,       // OEM Revision
    0x00,       // Creator ID
    0x00,       // Creator Revision
  },
  // Protection Flags - updated at EntryPoint.
  0
};

STATIC_ASSERT (
  sizeof (EFI_ACPI_WSMT_TABLE) == 40,
  "EFI_ACPI_WSMT_TABLE must be 40 for Revision 1"
  );

/**
  Entry point for the WSMT DXE driver.

  Installs the WSMT ACPI table when firmware can attest to supported
  MM communication buffer protections.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS      The WSMT table was installed successfully.
  @retval EFI_UNSUPPORTED  No supported WSMT protection is enabled.
  @retval Others           Error from protocol location or table installation.
**/
EFI_STATUS
EFIAPI
WsmtDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;
  EFI_STATUS               Status;
  UINTN                    TableKey;

  if (!FeaturePcdGet (PcdSmmSmramRequire) &&
      !FeaturePcdGet (PcdQemuVarsRequire))
  {
    return EFI_UNSUPPORTED;
  }

  mWsmtTable.ProtectionFlags =
    EFI_WSMT_PROTECTION_FLAGS_FIXED_COMM_BUFFERS |
    EFI_WSMT_PROTECTION_FLAGS_COMM_BUFFER_NESTED_PTR_PROTECTION;

  // Update the WSMT table header with the PCDs.
  CopyMem (
    mWsmtTable.Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (mWsmtTable.Header.OemId)
    );
  mWsmtTable.Header.OemTableId      = PcdGet64 (PcdAcpiDefaultOemTableId);
  mWsmtTable.Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  mWsmtTable.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  mWsmtTable.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTable
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TableKey = 0;
  Status   = AcpiTable->InstallAcpiTable (
                          AcpiTable,
                          &mWsmtTable,
                          sizeof (mWsmtTable),
                          &TableKey
                          );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Installed WSMT table (ProtectionFlags=0x%x)\n",
    __func__,
    mWsmtTable.ProtectionFlags
    ));

  return EFI_REQUEST_UNLOAD_IMAGE;
}
