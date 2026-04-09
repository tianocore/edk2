/** @file
  The Driver Binding and Service Binding Protocol for TlsDxe driver.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TlsImpl.h"

EFI_SERVICE_BINDING_PROTOCOL  mTlsServiceBinding = {
  TlsServiceBindingCreateChild,
  TlsServiceBindingDestroyChild
};

/**
  Release all the resources used by the TLS instance.

  @param[in]  Instance        The TLS instance data.

**/
VOID
TlsCleanInstance (
  IN TLS_INSTANCE  *Instance
  )
{
  if (Instance != NULL) {
    if (Instance->TlsConn != NULL) {
      TlsFree (Instance->TlsConn);
    }

    FreePool (Instance);
  }
}

/**
  Create the TLS instance and initialize it.

  @param[in]  Service              The pointer to the TLS service.
  @param[out] Instance             The pointer to the TLS instance.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.
  @retval EFI_SUCCESS            The TLS instance is created.

**/
EFI_STATUS
TlsCreateInstance (
  IN  TLS_SERVICE   *Service,
  OUT TLS_INSTANCE  **Instance
  )
{
  TLS_INSTANCE  *TlsInstance;

  *Instance = NULL;

  TlsInstance = AllocateZeroPool (sizeof (TLS_INSTANCE));
  if (TlsInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TlsInstance->Signature = TLS_INSTANCE_SIGNATURE;
  InitializeListHead (&TlsInstance->Link);
  TlsInstance->InDestroy = FALSE;
  TlsInstance->Service   = Service;

  CopyMem (&TlsInstance->Tls, &mTlsProtocol, sizeof (TlsInstance->Tls));
  CopyMem (&TlsInstance->TlsConfig, &mTlsConfigurationProtocol, sizeof (TlsInstance->TlsConfig));

  TlsInstance->TlsSessionState = EfiTlsSessionNotStarted;

  *Instance = TlsInstance;

  return EFI_SUCCESS;
}

/**
  Release all the resources used by the TLS service binding instance.

  @param[in]  Service        The TLS service data.

**/
VOID
TlsCleanService (
  IN TLS_SERVICE  *Service
  )
{
  if (Service != NULL) {
    if (Service->TlsCtx != NULL) {
      TlsCtxFree (Service->TlsCtx);
    }

    FreePool (Service);
  }
}

/**
  Create then initialize a TLS service.

  @param[in]  Image                  ImageHandle of the TLS driver
  @param[out] Service                The service for TLS driver

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resource to create the service.
  @retval EFI_SUCCESS            The service is created for the driver.

**/
EFI_STATUS
TlsCreateService (
  IN  EFI_HANDLE   Image,
  OUT TLS_SERVICE  **Service
  )
{
  TLS_SERVICE  *TlsService;

  ASSERT (Service != NULL);

  *Service = NULL;

  //
  // Allocate a TLS Service Data
  //
  TlsService = AllocateZeroPool (sizeof (TLS_SERVICE));
  if (TlsService == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize TLS Service Data
  //
  TlsService->Signature = TLS_SERVICE_SIGNATURE;
  CopyMem (&TlsService->ServiceBinding, &mTlsServiceBinding, sizeof (TlsService->ServiceBinding));
  TlsService->TlsChildrenNum = 0;
  InitializeListHead (&TlsService->TlsChildrenList);
  TlsService->ImageHandle = Image;

  *Service = TlsService;

  return EFI_SUCCESS;
}

/**
  Unloads an image.

  @param[in]  ImageHandle           Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
  @retval EFI_INVALID_PARAMETER ImageHandle is not a valid image handle.

**/
EFI_STATUS
EFIAPI
TlsUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                    Status;
  UINTN                         HandleNum;
  EFI_HANDLE                    *HandleBuffer;
  UINT32                        Index;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  TLS_SERVICE                   *TlsService;

  HandleBuffer   = NULL;
  ServiceBinding = NULL;
  TlsService     = NULL;

  //
  // Locate all the handles with Tls service binding protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiTlsServiceBindingProtocolGuid,
                  NULL,
                  &HandleNum,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleNum; Index++) {
    //
    // Firstly, find ServiceBinding interface
    //
    Status = gBS->OpenProtocol (
                    HandleBuffer[Index],
                    &gEfiTlsServiceBindingProtocolGuid,
                    (VOID **)&ServiceBinding,
                    ImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    TlsService = TLS_SERVICE_FROM_THIS (ServiceBinding);

    //
    // Then, uninstall ServiceBinding interface
    //
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    HandleBuffer[Index],
                    &gEfiTlsServiceBindingProtocolGuid,
                    ServiceBinding,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    TlsCleanService (TlsService);
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return EFI_SUCCESS;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param  ImageHandle           The firmware allocated handle for the UEFI image.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
TlsDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  TLS_SERVICE  *TlsService;

  //
  // Create TLS Service
  //
  Status = TlsCreateService (ImageHandle, &TlsService);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (TlsService != NULL);

  //
  // Initializes the OpenSSL library.
  //
  TlsInitialize ();

  //
  // Create a new SSL_CTX object as framework to establish TLS/SSL enabled
  // connections. TLS 1.0 is used as the default version.
  //
  TlsService->TlsCtx = TlsCtxNew (TLS10_PROTOCOL_VERSION_MAJOR, TLS10_PROTOCOL_VERSION_MINOR);
  if (TlsService->TlsCtx == NULL) {
    FreePool (TlsService);
    return EFI_ABORTED;
  }

  //
  // Install the TlsServiceBinding Protocol onto Handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &TlsService->Handle,
                  &gEfiTlsServiceBindingProtocolGuid,
                  &TlsService->ServiceBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_CLEAN_SERVICE;
  }

  return Status;

ON_CLEAN_SERVICE:
  TlsCleanService (TlsService);

  return Status;
}

/**
  Creates a child handle and installs a protocol.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param[in] This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in] ChildHandle Pointer to the handle of the child to create. If it is NULL,
                         then a new handle is created. If it is a pointer to an existing UEFI handle,
                         then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCESS           The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to create
                                the child.
  @retval other                 The child handle was not created.

**/
EFI_STATUS
EFIAPI
TlsServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  )
{
  TLS_SERVICE   *TlsService;
  TLS_INSTANCE  *TlsInstance;
  EFI_STATUS    Status;
  EFI_TPL       OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TlsService = TLS_SERVICE_FROM_THIS (This);

  Status = TlsCreateInstance (TlsService, &TlsInstance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (TlsInstance != NULL);

  //
  // Create a new TLS connection object.
  //
  TlsInstance->TlsConn = TlsNew (TlsService->TlsCtx);
  if (TlsInstance->TlsConn == NULL) {
    Status = EFI_ABORTED;
    goto ON_ERROR;
  }

  //
  // Set default ConnectionEnd to EfiTlsClient
  //
  Status = TlsSetConnectionEnd (TlsInstance->TlsConn, EfiTlsClient);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Install TLS protocol and configuration protocol onto ChildHandle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiTlsProtocolGuid,
                  &TlsInstance->Tls,
                  &gEfiTlsConfigurationProtocolGuid,
                  &TlsInstance->TlsConfig,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  TlsInstance->ChildHandle = *ChildHandle;

  //
  // Add it to the TLS service's child list.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&TlsService->TlsChildrenList, &TlsInstance->Link);
  TlsService->TlsChildrenNum++;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;

ON_ERROR:
  TlsCleanInstance (TlsInstance);
  return Status;
}

/**
  Destroys a child handle with a protocol installed on it.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Handle of the child to destroy.

  @retval EFI_SUCCESS           The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
TlsServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  TLS_SERVICE   *TlsService;
  TLS_INSTANCE  *TlsInstance;

  EFI_TLS_PROTOCOL                *Tls;
  EFI_TLS_CONFIGURATION_PROTOCOL  *TlsConfig;
  EFI_STATUS                      Status;
  EFI_TPL                         OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TlsService = TLS_SERVICE_FROM_THIS (This);

  //
  // Find TLS protocol interface installed in ChildHandle
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiTlsProtocolGuid,
                  (VOID **)&Tls,
                  TlsService->ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find TLS configuration protocol interface installed in ChildHandle
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiTlsConfigurationProtocolGuid,
                  (VOID **)&TlsConfig,
                  TlsService->ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TlsInstance = TLS_INSTANCE_FROM_PROTOCOL (Tls);

  if (TlsInstance->Service != TlsService) {
    return EFI_INVALID_PARAMETER;
  }

  if (TlsInstance->InDestroy) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  TlsInstance->InDestroy = TRUE;

  //
  // Uninstall the TLS protocol and TLS Configuration Protocol interface installed in ChildHandle.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiTlsProtocolGuid,
                  Tls,
                  &gEfiTlsConfigurationProtocolGuid,
                  TlsConfig,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  RemoveEntryList (&TlsInstance->Link);
  TlsService->TlsChildrenNum--;

  gBS->RestoreTPL (OldTpl);

  TlsCleanInstance (TlsInstance);

  return EFI_SUCCESS;
}
