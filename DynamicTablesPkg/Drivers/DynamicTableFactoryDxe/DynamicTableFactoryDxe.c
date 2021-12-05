/** @file
  Dynamic Table Factory Dxe

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <DeviceTreeTableGenerator.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <SmbiosTableGenerator.h>

#include "DynamicTableFactory.h"

/** The Dynamic Table Factory protocol structure that holds the
    list of registered ACPI and SMBIOS table generators.
*/
EDKII_DYNAMIC_TABLE_FACTORY_INFO  TableFactoryInfo;

/** A structure describing the Dynamic Table Factory protocol.
*/
STATIC
CONST
EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  DynamicTableFactoryProtocol = {
  CREATE_REVISION (1,             0),
  GetAcpiTableGenerator,
  RegisterAcpiTableGenerator,
  DeregisterAcpiTableGenerator,
  GetSmbiosTableGenerator,
  RegisterSmbiosTableGenerator,
  DeregisterSmbiosTableGenerator,
  GetDtTableGenerator,
  RegisterDtTableGenerator,
  DeregisterDtTableGenerator,
  &TableFactoryInfo
};

/** Entrypoint for Dynamic Table Factory Dxe.

  @param  ImageHandle
  @param  SystemTable

  @retval EFI_SUCCESS           Success.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval EFI_NOT_FOUND         Required interface/object was not found.
  @retval EFI_INVALID_PARAMETER Some parameter is incorrect/invalid.
**/
EFI_STATUS
EFIAPI
DynamicTableFactoryDxeInitialize (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEdkiiDynamicTableFactoryProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)&DynamicTableFactoryProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to install the Dynamic Table Factory Protocol." \
      " Status = %r\n",
      Status
      ));
  }

  return Status;
}
