/** @file
  Common Dynamic Table Manager Dxe

  Copyright (c) 2024, Arm Limited. All rights reserved.

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
/// This is a dummy list only existing for build purpose.
/// The FADT table must be placed at index 0.
///
ACPI_TABLE_PRESENCE_INFO  mAcpiVerifyTables[] = {
  { EStdAcpiTableIdFadt, EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE, "FADT", TRUE, 0 },
};

/** Get the arch specific ACPI table presence information.

  @param [out] PresenceArray      Array containing the ACPI tables to check.
  @param [out] PresenceArrayCount Count of elements in the PresenceArray.
  @param [out] FadtIndex          Index of the FADT table in the PresenceArray.
                                  -1 if absent.

  @retval EFI_SUCCESS           Success.
  @retval EFI_UNSUPPORTED       Unsupported.
**/
EFI_STATUS
EFIAPI
GetAcpiTablePresenceInfo (
  OUT ACPI_TABLE_PRESENCE_INFO  **PresenceArray,
  OUT UINT32                    *PresenceArrayCount,
  OUT INT32                     *FadtIndex
  )
{
  // Dummy function - Not Implemented.
  *PresenceArray      = mAcpiVerifyTables;
  *PresenceArrayCount = ARRAY_SIZE (mAcpiVerifyTables);
  *FadtIndex          = ACPI_TABLE_VERIFY_FADT;

  return EFI_UNSUPPORTED;
}
