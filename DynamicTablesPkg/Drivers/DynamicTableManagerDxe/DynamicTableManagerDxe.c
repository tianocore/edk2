/** @file
  Dynamic Table Manager Dxe

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <DeviceTreeTableGenerator.h>
#include <Library/TableHelperLib.h>
#include <Library/MetadataHandlerLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>

#include "DynamicTableManagerDxe.h"

STATIC VOID  *mAcpiTableProtocolRegistration;

/** Entrypoint of Dynamic Table Manager Dxe.

  The Dynamic Table Manager uses the Configuration Manager Protocol
  to get the list of ACPI and SMBIOS tables to install. For each table
  in the list it requests the corresponding ACPI/SMBIOS table factory for
  a generator capable of building the ACPI/SMBIOS table.
  If a suitable table generator is found, it invokes the generator interface
  to build the table. The Dynamic Table Manager then installs the
  table and invokes another generator interface to free any resources
  allocated for building the table.

  Since platforms may support generation of either ACPI or SMBIOS or both
  tables, register for notification of the corresponding protocols being
  ready and dispatch their table builds accordingly.

  @param  ImageHandle   Handle to the image.
  @param  SystemTable   Pointer to the system table.

  @retval EFI_SUCCESS           Success.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
EFI_STATUS
EFIAPI
DynamicTableManagerDxeInitialize (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_EVENT  AcpiEvent;

  AcpiEvent = EfiCreateProtocolNotifyEvent (
                &gEfiAcpiTableProtocolGuid,
                TPL_CALLBACK,
                AcpiTableProtocolReady,
                NULL,
                &mAcpiTableProtocolRegistration
                );
  if (AcpiEvent == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to register ACPI protocol notification event.\n"
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}
