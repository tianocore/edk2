/** @file
  RISC-V FADT Table Helpers

  Copyright (c) 2024-2025, Ventana Micro Systems Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.6 Specification, May, 2025

**/

#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include "FadtGenerator.h"

/** This macro defines the FADT flag options for RISC-V Platforms.
 *  REVISIT: We should use FADT v6.6 for RISC-V but it needs entire
 *  DynamicTablesPkg to move to ACPI 6.6. The flags are same in 6.5
 *  and 6.6. So, use 6.6 flags itself to avoid future change.
*/
#define FADT_FLAGS  (EFI_ACPI_6_6_HW_REDUCED_ACPI |          \
                     EFI_ACPI_6_6_LOW_POWER_S0_IDLE_CAPABLE)

/** Updates the Architecture specific information in the FADT Table.

  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.
  @param [in, out] Fadt       Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
EFI_STATUS
EFIAPI
FadtArchUpdate (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN   OUT EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE      *Fadt
  )
{
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Fadt != NULL);

  Fadt->Flags = FADT_FLAGS;

  return EFI_SUCCESS;
}
