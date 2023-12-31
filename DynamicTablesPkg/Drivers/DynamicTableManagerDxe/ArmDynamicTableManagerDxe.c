/** @file
  ARM Dynamic Table Manager Dxe

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <DeviceTreeTableGenerator.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include "DynamicTableManagerDxe.h"

///
/// Order of ACPI table being verified during presence inspection.
///
#define ACPI_TABLE_VERIFY_FADT   0
#define ACPI_TABLE_VERIFY_MADT   1
#define ACPI_TABLE_VERIFY_GTDT   2
#define ACPI_TABLE_VERIFY_DSDT   3
#define ACPI_TABLE_VERIFY_DBG2   4
#define ACPI_TABLE_VERIFY_SPCR   5
#define ACPI_TABLE_VERIFY_COUNT  6

///
/// We require the FADT, MADT, GTDT and the DSDT tables to boot.
/// This list also include optional ACPI tables: DBG2, SPCR.
///
ACPI_TABLE_PRESENCE_INFO  mAcpiVerifyTables[ACPI_TABLE_VERIFY_COUNT] = {
  { EStdAcpiTableIdFadt, EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,            "FADT", TRUE,  0 },
  { EStdAcpiTableIdMadt, EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,         "MADT", TRUE,  0 },
  { EStdAcpiTableIdGtdt, EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE,         "GTDT", TRUE,  0 },
  { EStdAcpiTableIdDsdt, EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE, "DSDT", TRUE,  0 },
  { EStdAcpiTableIdDbg2, EFI_ACPI_6_2_DEBUG_PORT_2_TABLE_SIGNATURE,                      "DBG2", FALSE, 0 },
  { EStdAcpiTableIdSpcr, EFI_ACPI_6_2_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,   "SPCR", FALSE, 0 },
};

/** The function checks if the Configuration Manager has provided the
    mandatory ACPI tables for installation.

  @param [in]  AcpiTableInfo      Pointer to the ACPI Table Info list.
  @param [in]  AcpiTableCount     Count of ACPI Table Info.

  @retval EFI_SUCCESS           Success.
  @retval EFI_NOT_FOUND         If mandatory table is not found.
  @retval EFI_ALREADY_STARTED   If mandatory table found in AcpiTableInfo is already installed.
**/
EFI_STATUS
EFIAPI
VerifyMandatoryTablesArePresent (
  IN CONST CM_STD_OBJ_ACPI_TABLE_INFO  *CONST  AcpiTableInfo,
  IN       UINT32                              AcpiTableCount
  )
{
  EFI_STATUS                   Status;
  UINTN                        Handle;
  UINTN                        Index;
  UINTN                        InstalledTableIndex;
  EFI_ACPI_DESCRIPTION_HEADER  *DescHeader;
  EFI_ACPI_TABLE_VERSION       Version;
  EFI_ACPI_SDT_PROTOCOL        *AcpiSdt;

  ASSERT (AcpiTableInfo != NULL);

  Status = EFI_SUCCESS;

  // Check against the statically initialized ACPI tables to see if they are in ACPI info list
  while (AcpiTableCount-- != 0) {
    for (Index = 0; Index < ACPI_TABLE_VERIFY_COUNT; Index++) {
      if (AcpiTableInfo[AcpiTableCount].AcpiTableSignature == mAcpiVerifyTables[Index].AcpiTableSignature) {
        mAcpiVerifyTables[Index].Presence |= ACPI_TABLE_PRESENT_INFO_LIST;
        // Found this table, skip the rest.
        break;
      }
    }
  }

  // They also might be published already, so we can search from there
  if (FeaturePcdGet (PcdInstallAcpiSdtProtocol)) {
    AcpiSdt = NULL;
    Status  = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&AcpiSdt);

    if (EFI_ERROR (Status) || (AcpiSdt == NULL)) {
      DEBUG ((DEBUG_ERROR, "ERROR: Failed to locate ACPI SDT protocol (0x%p) - %r\n", AcpiSdt, Status));
      return Status;
    }

    for (Index = 0; Index < ACPI_TABLE_VERIFY_COUNT; Index++) {
      Handle              = 0;
      InstalledTableIndex = 0;
      do {
        Status = AcpiSdt->GetAcpiTable (InstalledTableIndex, (EFI_ACPI_SDT_HEADER **)&DescHeader, &Version, &Handle);
        if (EFI_ERROR (Status)) {
          break;
        }

        InstalledTableIndex++;
      } while (DescHeader->Signature != mAcpiVerifyTables[Index].AcpiTableSignature);

      if (!EFI_ERROR (Status)) {
        mAcpiVerifyTables[Index].Presence |= ACPI_TABLE_PRESENT_INSTALLED;
      }
    }
  }

  // Reset the return Status value to EFI_SUCCESS. We do not fully care if the table look up has failed.
  Status = EFI_SUCCESS;
  for (Index = 0; Index < ACPI_TABLE_VERIFY_COUNT; Index++) {
    if (mAcpiVerifyTables[Index].Presence == 0) {
      if (mAcpiVerifyTables[Index].IsMandatory) {
        DEBUG ((DEBUG_ERROR, "ERROR: %a Table not found.\n", mAcpiVerifyTables[Index].AcpiTableName));
        Status = EFI_NOT_FOUND;
      } else {
        DEBUG ((DEBUG_WARN, "WARNING: %a Table not found.\n", mAcpiVerifyTables[Index].AcpiTableName));
      }
    } else if (mAcpiVerifyTables[Index].Presence ==
               (ACPI_TABLE_PRESENT_INFO_LIST | ACPI_TABLE_PRESENT_INSTALLED))
    {
      DEBUG ((DEBUG_ERROR, "ERROR: %a Table found while already published.\n", mAcpiVerifyTables[Index].AcpiTableName));
      Status = EFI_ALREADY_STARTED;
    }
  }

  return Status;
}

/** The function checks if the FADT table is present and installed

  @retval TRUE          FADT is present and installed.
  @retval FALSE         FADT is not present and installed.
**/
BOOLEAN
IsFadtPresentInstalled (
  VOID
  )
{
  if ((mAcpiVerifyTables[ACPI_TABLE_VERIFY_FADT].Presence & ACPI_TABLE_PRESENT_INSTALLED) == 0) {
    return FALSE;
  }

  return TRUE;
}
