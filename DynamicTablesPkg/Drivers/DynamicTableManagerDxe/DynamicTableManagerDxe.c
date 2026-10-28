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
#include "HiiFormsBuilder.h"

STATIC VOID  *mAcpiTableProtocolRegistration;
STATIC VOID  *mSmbiosProtocolRegistration;

EFI_EVENT  mAcpiEvent;
EFI_EVENT  mSmbiosEvent;

/** Register ACPI table protocol notification handler.

  @retval EFI_SUCCESS           Success.
  @retval EFI_OUT_OF_RESOURCES  Event allocation failed.
  @retval Others                Failure returned by boot services.

**/
STATIC
EFI_STATUS
RegisterAcpiTableHandler (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  AcpiTableProtocolReady,
                  NULL,
                  &mAcpiEvent
                  );
  if (EFI_ERROR (Status)) {
    mAcpiEvent = NULL;
    return Status;
  }

  Status = gBS->RegisterProtocolNotify (
                  &gEfiAcpiTableProtocolGuid,
                  mAcpiEvent,
                  &mAcpiTableProtocolRegistration
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (mAcpiEvent);
    mAcpiEvent = NULL;
    return Status;
  }

  Status = gBS->SignalEvent (mAcpiEvent);
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (mAcpiEvent);
    mAcpiEvent = NULL;
  }

  return Status;
}

/** Register SMBIOS table protocol notification handler.

  @retval EFI_SUCCESS           Success.
  @retval EFI_OUT_OF_RESOURCES  Event allocation failed.
  @retval Others                Failure returned by boot services.

**/
STATIC
EFI_STATUS
RegisterSmbiosTableHandler (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SmbiosProtocolReady,
                  NULL,
                  &mSmbiosEvent
                  );
  if (EFI_ERROR (Status)) {
    mSmbiosEvent = NULL;
    return Status;
  }

  Status = gBS->RegisterProtocolNotify (
                  &gEfiSmbiosProtocolGuid,
                  mSmbiosEvent,
                  &mSmbiosProtocolRegistration
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (mSmbiosEvent);
    mSmbiosEvent = NULL;
    return Status;
  }

  Status = gBS->SignalEvent (mSmbiosEvent);
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (mSmbiosEvent);
    mSmbiosEvent = NULL;
  }

  return Status;
}

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
  EFI_STATUS  Status;

  Status = RegisterAcpiTableHandler ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to register ACPI protocol notification event. Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = RegisterSmbiosTableHandler ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to register SMBIOS protocol notification event. Status = %r\n",
      Status
      ));
    if (mAcpiEvent != NULL) {
      gBS->CloseEvent (mAcpiEvent);
      mAcpiEvent = NULL;
    }

    return Status;
  }

  DynamicHiiFormsProcess ();

  return EFI_SUCCESS;
}

/** Cleanup of Dynamic Table Manager Dxe.

  This function is invoked when UnloadImage() is called
  for this module. It is cleaning up the dynamic HII form
  resources.

  @param  ImageHandle   Handle to the image.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Function parameter is NULL.

**/
EFI_STATUS
EFIAPI
DynamicTableManagerDxeUnInitialize (
  IN  EFI_HANDLE  ImageHandle
  )
{
  if (ImageHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FreeHiiFormResources ();

  if (mSmbiosEvent != NULL) {
    gBS->CloseEvent (mSmbiosEvent);
  }

  if (mAcpiEvent != NULL) {
    gBS->CloseEvent (mAcpiEvent);
  }

  return EFI_SUCCESS;
}
