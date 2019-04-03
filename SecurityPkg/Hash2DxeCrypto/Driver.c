/** @file
  This is service binding for Hash driver.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Driver.h"

EFI_SERVICE_BINDING_PROTOCOL    mHash2ServiceBindingProtocol = {
  Hash2ServiceBindingCreateChild,
  Hash2ServiceBindingDestroyChild
};

/**
  Creates a child handle with a set of I/O services.

  @param[in]       This              Protocol instance pointer.
  @param[in, out]  ChildHandle       Pointer to the handle of the child to create. If
                                     it is NULL, then a new handle is created. If
                                     it is not NULL, then the I/O services are added
                                     to the existing child handle.

  @retval EFI_SUCCES                 The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER      ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES       There are not enough resources available to
                                     create the child.
  @retval Others                     The child handle was not created.

**/
EFI_STATUS
EFIAPI
Hash2ServiceBindingCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL    *This,
  IN OUT EFI_HANDLE                      *ChildHandle
  )
{
  EFI_STATUS          Status;
  HASH2_SERVICE_DATA  *Hash2ServiceData;
  HASH2_INSTANCE_DATA *Instance;
  EFI_TPL             OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Hash2ServiceData = HASH2_SERVICE_DATA_FROM_THIS (This);

  //
  // Allocate buffer for the new instance.
  //
  Instance = AllocateZeroPool (sizeof (HASH2_INSTANCE_DATA));
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Init the instance data.
  //
  Instance->Signature = HASH2_INSTANCE_DATA_SIGNATURE;
  CopyMem (&Instance->Hash2Protocol, &mHash2Protocol, sizeof (Instance->Hash2Protocol));
  Instance->Hash2ServiceData = Hash2ServiceData;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiHash2ProtocolGuid,
                  &Instance->Hash2Protocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Instance);
    return Status;
  }

  Instance->Handle = *ChildHandle;

  //
  // Add the child instance into ChildrenList.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&Hash2ServiceData->ChildrenList, &Instance->InstEntry);

  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  Destroys a child handle with a set of I/O services.

  The DestroyChild() function does the opposite of CreateChild(). It removes a
  protocol that was installed by CreateChild() from ChildHandle. If the removed
  protocol is the last protocol on ChildHandle, then ChildHandle is destroyed.

  @param[in]  This               Pointer to the EFI_SERVICE_BINDING_PROTOCOL
                                 instance.
  @param[in]  ChildHandle        Handle of the child to destroy.

  @retval EFI_SUCCES             The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED        ChildHandle does not support the protocol that
                                 is being removed.
  @retval EFI_INVALID_PARAMETER  ChildHandle is NULL.
  @retval EFI_ACCESS_DENIED      The protocol could not be removed from the
                                 ChildHandle because its services are being
                                 used.
  @retval Others                 The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
Hash2ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                      ChildHandle
  )
{
  EFI_STATUS                     Status;
  HASH2_SERVICE_DATA             *Hash2ServiceData;
  EFI_HASH2_PROTOCOL             *Hash2Protocol;
  HASH2_INSTANCE_DATA            *Instance;
  EFI_TPL                        OldTpl;
  LIST_ENTRY                     *Entry;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Hash2ServiceData = HASH2_SERVICE_DATA_FROM_THIS (This);

  //
  // Check if this ChildHandle is valid
  //
  Instance = NULL;
  for(Entry = (&Hash2ServiceData->ChildrenList)->ForwardLink; Entry != (&Hash2ServiceData->ChildrenList); Entry = Entry->ForwardLink) {
    Instance = HASH2_INSTANCE_DATA_FROM_LINK (Entry);
    if (Instance->Handle == ChildHandle) {
      break;
    } else {
      Instance = NULL;
    }
  }
  if (Instance == NULL) {
    DEBUG ((EFI_D_ERROR, "Hash2ServiceBindingDestroyChild - Invalid handle\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Get HashProtocol
  //
  Status = gBS->HandleProtocol (
                  ChildHandle,
                  &gEfiHash2ProtocolGuid,
                  (VOID **)&Hash2Protocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Hash2Protocol == &Instance->Hash2Protocol);

  //
  // Uninstall the Hash protocol.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiHash2ProtocolGuid,
                  &Instance->Hash2Protocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Remove this instance from the ChildrenList.
  //
  RemoveEntryList (&Instance->InstEntry);

  gBS->RestoreTPL (OldTpl);

  FreePool (Instance);

  return Status;
}

/**
  The entry point for Hash driver which installs the service binding protocol.

  @param[in]  ImageHandle  The image handle of the driver.
  @param[in]  SystemTable  The system table.

  @retval EFI_SUCCES       The service binding protocols is successfully installed.
  @retval Others           Other errors as indicated.

**/
EFI_STATUS
EFIAPI
Hash2DriverEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS         Status;
  HASH2_SERVICE_DATA *Hash2ServiceData;

  //
  // Initialize the Hash Service Data.
  //
  Hash2ServiceData = AllocateZeroPool (sizeof (HASH2_SERVICE_DATA));
  if (Hash2ServiceData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Hash2ServiceData->Signature     = HASH2_SERVICE_DATA_SIGNATURE;
  CopyMem (&Hash2ServiceData->ServiceBinding, &mHash2ServiceBindingProtocol, sizeof (EFI_SERVICE_BINDING_PROTOCOL));
  InitializeListHead (&Hash2ServiceData->ChildrenList);

  //
  // Install the HASH Service Binding Protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Hash2ServiceData->ServiceHandle,
                  &gEfiHash2ServiceBindingProtocolGuid,
                  &Hash2ServiceData->ServiceBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Hash2ServiceData);
  }

  return Status;
}
