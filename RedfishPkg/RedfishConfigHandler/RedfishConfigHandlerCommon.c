/** @file
  The common code of EDKII Redfish Configuration Handler driver.

  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishConfigHandlerCommon.h"

REDFISH_CONFIG_DRIVER_DATA  gRedfishConfigData;     // Only one Redfish service supported
                                                    // on platform for the BIOS
                                                    // Redfish configuration.
EFI_EVENT                          gEndOfDxeEvent        = NULL;
EFI_EVENT                          gExitBootServiceEvent = NULL;
EDKII_REDFISH_CREDENTIAL_PROTOCOL  *gCredential          = NULL;

/**
  Callback function executed when the EndOfDxe event group is signaled.

  @param[in]   Event    Event whose notification function is being invoked.
  @param[out]  Context  Pointer to the Context buffer.

**/
VOID
EFIAPI
RedfishConfigOnEndOfDxe (
  IN  EFI_EVENT  Event,
  OUT VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = gCredential->StopService (gCredential, ServiceStopTypeSecureBootDisabled);
  if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
    DEBUG ((DEBUG_ERROR, "Redfish credential protocol failed to stop service on EndOfDxe: %r", Status));
  }

  //
  // Close event, so it will not be invoked again.
  //
  gBS->CloseEvent (gEndOfDxeEvent);
  gEndOfDxeEvent = NULL;
}

/**
  Callback function executed when the ExitBootService event group is signaled.

  @param[in]   Event    Event whose notification function is being invoked.
  @param[out]  Context  Pointer to the Context buffer

**/
VOID
EFIAPI
RedfishConfigOnExitBootService (
  IN  EFI_EVENT  Event,
  OUT VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = gCredential->StopService (gCredential, ServiceStopTypeExitBootService);
  if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
    DEBUG ((DEBUG_ERROR, "Redfish credential protocol failed to stop service on ExitBootService: %r", Status));
  }
}

/**
  Unloads an image.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.

**/
EFI_STATUS
RedfishConfigDriverCommonUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  if (gEndOfDxeEvent != NULL) {
    gBS->CloseEvent (gEndOfDxeEvent);
    gEndOfDxeEvent = NULL;
  }

  if (gExitBootServiceEvent != NULL) {
    gBS->CloseEvent (gExitBootServiceEvent);
    gExitBootServiceEvent = NULL;
  }

  if (gRedfishConfigData.Event != NULL) {
    gBS->CloseEvent (gRedfishConfigData.Event);
    gRedfishConfigData.Event = NULL;
  }

  return EFI_SUCCESS;
}

/**
  This is the common code for Redfish configuration UEFI and DXE driver
  initialization.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.
**/
EFI_STATUS
RedfishConfigCommonInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Locate Redfish Credential Protocol to get credential for
  // accessing to Redfish service.
  //
  Status = gBS->LocateProtocol (&gEdkIIRedfishCredentialProtocolGuid, NULL, (VOID **)&gCredential);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: No Redfish Credential Protocol is installed on system.", __func__));
    return Status;
  }

  //
  // Create EndOfDxe Event.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  RedfishConfigOnEndOfDxe,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &gEndOfDxeEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to register End Of DXE event.", __func__));
    return Status;
  }

  //
  // Create Exit Boot Service event.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  RedfishConfigOnExitBootService,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &gExitBootServiceEvent
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (gEndOfDxeEvent);
    gEndOfDxeEvent = NULL;
    DEBUG ((DEBUG_ERROR, "%a: Fail to register Exit Boot Service event.", __func__));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This is the common code to stop EDK2 Redfish feature driver.

  @retval EFI_SUCCESS    All EDK2 Redfish feature drivers are
                         stopped.
  @retval Others         An unexpected error occurred.
**/
EFI_STATUS
RedfishConfigCommonStop (
  VOID
  )
{
  EFI_STATUS                             Status;
  EFI_HANDLE                             *HandleBuffer;
  UINTN                                  NumberOfHandles;
  UINTN                                  Index;
  EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL  *ConfigHandler;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEdkIIRedfishConfigHandlerProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    return Status;
  }

  Status = EFI_SUCCESS;
  for (Index = 0; Index < NumberOfHandles; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEdkIIRedfishConfigHandlerProtocolGuid,
                    (VOID **)&ConfigHandler
                    );
    ASSERT_EFI_ERROR (Status);

    Status = ConfigHandler->Stop (ConfigHandler);
    if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
      DEBUG ((DEBUG_ERROR, "ERROR: Failed to stop Redfish config handler %p.\n", ConfigHandler));
      break;
    }
  }

  return Status;
}

/**
  Callback function executed when a Redfish Config Handler Protocol is installed
  by EDK2 Redfish Feature Drivers.

**/
VOID
RedfishConfigHandlerInitialization (
  VOID
  )
{
  EFI_STATUS                             Status;
  EFI_HANDLE                             *HandleBuffer;
  UINTN                                  NumberOfHandles;
  EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL  *ConfigHandler;
  UINTN                                  Index;
  UINT32                                 *Id;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEdkIIRedfishConfigHandlerProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < NumberOfHandles; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiCallerIdGuid,
                    (VOID **)&Id
                    );
    if (!EFI_ERROR (Status)) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEdkIIRedfishConfigHandlerProtocolGuid,
                    (VOID **)&ConfigHandler
                    );
    ASSERT_EFI_ERROR (Status);
    Status = ConfigHandler->Init (ConfigHandler, &gRedfishConfigData.RedfishServiceInfo);
    if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
      DEBUG ((DEBUG_ERROR, "ERROR: Failed to init Redfish config handler %p.\n", ConfigHandler));
      continue;
    }

    //
    // Install caller ID to indicate Redfish Configure Handler is initialized.
    //
    Status = gBS->InstallProtocolInterface (
                    &HandleBuffer[Index],
                    &gEfiCallerIdGuid,
                    EFI_NATIVE_INTERFACE,
                    (VOID *)&gRedfishConfigData.CallerId
                    );
    ASSERT_EFI_ERROR (Status);
  }
}
