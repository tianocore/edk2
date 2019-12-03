/** @file
  The prototype of driver binding and service binding protocol for TCP driver.

  Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TCP_DRIVER_H_
#define _TCP_DRIVER_H_

#define TCP_DRIVER_SIGNATURE   SIGNATURE_32 ('T', 'C', 'P', 'D')

#define TCP_PORT_KNOWN         1024
#define TCP_PORT_USER_RESERVED 65535

typedef struct _TCP_HEARTBEAT_TIMER {
  EFI_EVENT  TimerEvent;
  INTN       RefCnt;
} TCP_HEARTBEAT_TIMER;

typedef struct _TCP_SERVICE_DATA {
  UINT32                        Signature;
  EFI_HANDLE                    ControllerHandle;
  EFI_HANDLE                    DriverBindingHandle;
  UINT8                         IpVersion;
  IP_IO                         *IpIo;
  EFI_SERVICE_BINDING_PROTOCOL  ServiceBinding;
  LIST_ENTRY                    SocketList;
} TCP_SERVICE_DATA;

typedef struct _TCP_PROTO_DATA {
  TCP_SERVICE_DATA  *TcpService;
  TCP_CB            *TcpPcb;
} TCP_PROTO_DATA;

#define TCP_SERVICE_FROM_THIS(a) \
  CR ( \
  (a), \
  TCP_SERVICE_DATA, \
  ServiceBinding, \
  TCP_DRIVER_SIGNATURE \
  )

//
// Function prototype for the driver's entry point
//

/**
  The entry point for Tcp driver, used to install Tcp driver on the ImageHandle.

  @param[in]  ImageHandle   The firmware allocated handle for this driver image.
  @param[in]  SystemTable   Pointer to the EFI system table.

  @retval EFI_SUCCESS   The driver loaded.
  @retval other         The driver did not load.

**/
EFI_STATUS
EFIAPI
TcpDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

//
// Function prototypes for the Driver Binding Protocol
//

/**
  Test to see if this driver supports ControllerHandle.

  @param[in]  This                Protocol instance pointer.
  @param[in]  ControllerHandle    Handle of the device to test.
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific
                                  child device to start.

  @retval EFI_SUCCESS             This driver supports this device.
  @retval EFI_ALREADY_STARTED     This driver is already running on this device.
  @retval other                   This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Tcp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle.

  @param[in]  This                   Protocol instance pointer.
  @param[in]  ControllerHandle       Handle of device to bind driver to.
  @param[in]  RemainingDevicePath    Optional parameter use to pick a specific child
                                     device to start.

  @retval EFI_SUCCESS            The driver was added to ControllerHandle.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources to start the
                                 driver.
  @retval other                  The driver cannot be added to ControllerHandle.

**/
EFI_STATUS
EFIAPI
Tcp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
Tcp4DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  );

/**
  Test to see if this driver supports ControllerHandle.

  @param[in]  This                Protocol instance pointer.
  @param[in]  ControllerHandle    Handle of the device to test.
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific
                                  child device to start.

  @retval EFI_SUCCESS             This driver supports this device.
  @retval EFI_ALREADY_STARTED     This driver is already running on this device.
  @retval other                   This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Tcp6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle.

  @param[in]  This                   Protocol instance pointer.
  @param[in]  ControllerHandle       Handle of device to bind driver to.
  @param[in]  RemainingDevicePath    Optional parameter use to pick a specific child
                                     device to start.

  @retval EFI_SUCCESS            The driver was added to ControllerHandle.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources to start the
                                 driver.
  @retval other                  The driver cannot be added to ControllerHandle.

**/
EFI_STATUS
EFIAPI
Tcp6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
Tcp6DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  );

/**
  The Callback funtion called after the TCP socket is created.

  @param[in]  This            Pointer to the socket just created.
  @param[in]  Context         The context of the socket.

  @retval EFI_SUCCESS         This protocol is installed successfully.
  @retval other               An error occured.

**/
EFI_STATUS
TcpCreateSocketCallback (
  IN SOCKET  *This,
  IN VOID    *Context
  );

/**
  The callback function called before the TCP socket is to be destroyed.

  @param[in]  This                   The TCP socket to be destroyed.
  @param[in]  Context                The context of the socket.

**/
VOID
TcpDestroySocketCallback (
  IN SOCKET  *This,
  IN VOID    *Context
  );

//
// Function prototypes for the ServiceBinding Protocol
//

/**
  Creates a child handle with a set of TCP services.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param[in]      This          Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in, out] ChildHandle   Pointer to the handle of the child to create.
                                If it is NULL, then a new handle is created.
                                If it is a pointer to an existing UEFI handle,
                                then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to create
                                the child.
  @retval other                 The child handle was not created.

**/
EFI_STATUS
EFIAPI
TcpServiceBindingCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  );

/**
  Destroys a child handle with a set of TCP services.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Handle of the child to destroy.

  @retval EFI_SUCCES            The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER The child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
TcpServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  );

#endif
