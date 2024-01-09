/** @file
  X86 Dynamic Table Manager Dxe

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include "DynamicTableManagerDxe.h"

// TODO: Dummy interfaces for X86 for now.

/** The function checks if the Configuration Manager has provided the
    mandatory ACPI tables for installation.

  @param [in]  AcpiTableInfo      Pointer to the ACPI Table Info list.
  @param [in]  AcpiTableCount     Count of ACPI Table Info.

  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
VerifyMandatoryTablesArePresent (
  IN CONST CM_STD_OBJ_ACPI_TABLE_INFO  *CONST  AcpiTableInfo,
  IN       UINT32                              AcpiTableCount
  )
{
  return EFI_SUCCESS;
}

/** The function checks if the FADT table is present and installed

  @retval TRUE          FADT is present and installed.
**/
BOOLEAN
IsFadtPresentInstalled (
  VOID
  )
{
  return TRUE;
}
