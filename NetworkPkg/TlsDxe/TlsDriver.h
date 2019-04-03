/** @file
  Header file of the Driver Binding and Service Binding Protocol for TlsDxe driver.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_TLS_DRIVER_H__
#define __EFI_TLS_DRIVER_H__

#include <Uefi.h>

//
// Driver Protocols
//
#include <Protocol/ServiceBinding.h>

//
// Driver Version
//
#define TLS_VERSION  0x00000000

#define TLS_SERVICE_SIGNATURE    SIGNATURE_32 ('T', 'L', 'S', 'S')

#define TLS_INSTANCE_SIGNATURE   SIGNATURE_32 ('T', 'L', 'S', 'I')

///
/// TLS Service Data
///
typedef struct _TLS_SERVICE  TLS_SERVICE;

///
/// TLS Instance Data
///
typedef struct _TLS_INSTANCE TLS_INSTANCE;


struct _TLS_SERVICE {
  UINT32                          Signature;
  EFI_SERVICE_BINDING_PROTOCOL    ServiceBinding;

  UINT16                          TlsChildrenNum;
  LIST_ENTRY                      TlsChildrenList;

  //
  // Handle to install TlsServiceBinding protocol.
  //
  EFI_HANDLE                      Handle;
  EFI_HANDLE                      ImageHandle;

  //
  // Main SSL Context object which is created by a server or client once per program
  // life-time and which holds mainly default values for the SSL object which are later
  // created for the connections.
  //
  VOID                            *TlsCtx;
};

struct _TLS_INSTANCE {
  UINT32                          Signature;
  LIST_ENTRY                      Link;

  BOOLEAN                         InDestroy;

  TLS_SERVICE                     *Service;
  EFI_HANDLE                      ChildHandle;

  EFI_TLS_PROTOCOL                Tls;
  EFI_TLS_CONFIGURATION_PROTOCOL  TlsConfig;

  EFI_TLS_SESSION_STATE           TlsSessionState;

  //
  // Main SSL Connection which is created by a server or a client
  // per established connection.
  //
  VOID                            *TlsConn;
};


#define TLS_SERVICE_FROM_THIS(a)   \
  CR (a, TLS_SERVICE, ServiceBinding, TLS_SERVICE_SIGNATURE)

#define TLS_INSTANCE_FROM_PROTOCOL(a)  \
  CR (a, TLS_INSTANCE, Tls, TLS_INSTANCE_SIGNATURE)

#define TLS_INSTANCE_FROM_CONFIGURATION(a)  \
  CR (a, TLS_INSTANCE, TlsConfig, TLS_INSTANCE_SIGNATURE)


/**
  Release all the resources used by the TLS instance.

  @param[in]  Instance        The TLS instance data.

**/
VOID
TlsCleanInstance (
  IN TLS_INSTANCE           *Instance
  );

/**
  Create the TLS instance and initialize it.

  @param[in]  Service              The pointer to the TLS service.
  @param[out] Instance             The pointer to the TLS instance.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.
  @retval EFI_SUCCESS            The TLS instance is created.

**/
EFI_STATUS
TlsCreateInstance (
  IN  TLS_SERVICE         *Service,
  OUT TLS_INSTANCE        **Instance
  );

/**
  Release all the resources used by the TLS service binding instance.

  @param[in]  Service        The TLS service data.

**/
VOID
TlsCleanService (
  IN TLS_SERVICE     *Service
  );

/**
  Create then initialize a TLS service.

  @param[in]  Image                  ImageHandle of the TLS driver
  @param[out] Service                The service for TLS driver

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resource to create the service.
  @retval EFI_SUCCESS            The service is created for the driver.

**/
EFI_STATUS
TlsCreateService (
  IN  EFI_HANDLE            Image,
  OUT TLS_SERVICE           **Service
  );

/**
  Unloads an image.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
  @retval EFI_INVALID_PARAMETER ImageHandle is not a valid image handle.

**/
EFI_STATUS
EFIAPI
TlsUnload (
  IN EFI_HANDLE  ImageHandle
  );

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
  );

/**
  Creates a child handle and installs a protocol.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param[in] This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in] ChildHandle Pointer to the handle of the child to create. If it is NULL,
                         then a new handle is created. If it is a pointer to an existing UEFI handle,
                         then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
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
  );

/**
  Destroys a child handle with a protocol installed on it.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Handle of the child to destroy.

  @retval EFI_SUCCES            The protocol was removed from ChildHandle.
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
  );

#endif

