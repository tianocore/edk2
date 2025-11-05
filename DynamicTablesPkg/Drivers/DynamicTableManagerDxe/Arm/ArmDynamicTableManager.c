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
/// Array containing the ACPI tables to check.
/// We require the FADT, MADT, GTDT and the DSDT tables to boot.
/// This list also include optional ACPI tables: DBG2, SPCR.
/// The FADT table must be placed at index 0.
///
STATIC ACPI_TABLE_PRESENCE_INFO  mAcpiVerifyTables[] = {
  { EStdAcpiTableIdFadt, EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,            "FADT", TRUE,  0 },
  { EStdAcpiTableIdMadt, EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,         "MADT", TRUE,  0 },
  { EStdAcpiTableIdGtdt, EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE,         "GTDT", TRUE,  0 },
  { EStdAcpiTableIdDsdt, EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE, "DSDT", TRUE,  0 },
  { EStdAcpiTableIdDbg2, EFI_ACPI_6_2_DEBUG_PORT_2_TABLE_SIGNATURE,                      "DBG2", FALSE, 0 },
  { EStdAcpiTableIdSpcr, EFI_ACPI_6_2_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,   "SPCR", FALSE, 0 },
};

/** Get the arch specific ACPI table presence information.

  @param [out] PresenceArray      Array containing the ACPI tables to check.
  @param [out] PresenceArrayCount Count of elements in the PresenceArray.
  @param [out] FadtIndex          Index of the FADT table in the PresenceArray.
                                  -1 if absent.

  @retval EFI_SUCCESS           Success.
**/
EFI_STATUS
EFIAPI
GetAcpiTablePresenceInfo (
  OUT ACPI_TABLE_PRESENCE_INFO  **PresenceArray,
  OUT UINT32                    *PresenceArrayCount,
  OUT INT32                     *FadtIndex
  )
{
  *PresenceArray      = mAcpiVerifyTables;
  *PresenceArrayCount = ARRAY_SIZE (mAcpiVerifyTables);
  *FadtIndex          = ACPI_TABLE_VERIFY_FADT;

  return EFI_SUCCESS;
}
