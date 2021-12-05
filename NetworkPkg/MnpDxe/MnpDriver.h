/** @file
  Declaration of structures and functions for MnpDxe driver.

Copyright (c) 2005 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MNP_DRIVER_H_
#define _MNP_DRIVER_H_

#include <Uefi.h>

#include <Protocol/ManagedNetwork.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/VlanConfig.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/NetLib.h>
#include <Library/DpcLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>

#include "ComponentName.h"

#define MNP_DEVICE_DATA_SIGNATURE  SIGNATURE_32 ('M', 'n', 'p', 'D')

//
// Global Variables
//
extern  EFI_DRIVER_BINDING_PROTOCOL  gMnpDriverBinding;

typedef struct {
  UINT32                         Signature;

  EFI_HANDLE                     ControllerHandle;
  EFI_HANDLE                     ImageHandle;

  EFI_VLAN_CONFIG_PROTOCOL       VlanConfig;
  UINTN                          NumberOfVlan;
  CHAR16                         *MacString;
  EFI_SIMPLE_NETWORK_PROTOCOL    *Snp;

  //
  // List of MNP_SERVICE_DATA
  //
  LIST_ENTRY                     ServiceList;
  //
  // Number of configured MNP Service Binding child
  //
  UINTN                          ConfiguredChildrenNumber;

  LIST_ENTRY                     GroupAddressList;
  UINT32                         GroupAddressCount;

  LIST_ENTRY                     FreeTxBufList;
  LIST_ENTRY                     AllTxBufList;
  UINT32                         TxBufCount;

  NET_BUF_QUEUE                  FreeNbufQue;
  INTN                           NbufCnt;

  EFI_EVENT                      PollTimer;
  BOOLEAN                        EnableSystemPoll;

  EFI_EVENT                      TimeoutCheckTimer;
  EFI_EVENT                      MediaDetectTimer;

  UINT32                         UnicastCount;
  UINT32                         BroadcastCount;
  UINT32                         MulticastCount;
  UINT32                         PromiscuousCount;

  //
  // The size of the data buffer in the MNP_PACKET_BUFFER used to
  // store a packet.
  //
  UINT32                         BufferLength;
  UINT32                         PaddingSize;
  NET_BUF                        *RxNbufCache;
} MNP_DEVICE_DATA;

#define MNP_DEVICE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  MNP_DEVICE_DATA, \
  VlanConfig, \
  MNP_DEVICE_DATA_SIGNATURE \
  )

#define MNP_SERVICE_DATA_SIGNATURE  SIGNATURE_32 ('M', 'n', 'p', 'S')

typedef struct {
  UINT32                          Signature;

  LIST_ENTRY                      Link;

  MNP_DEVICE_DATA                 *MnpDeviceData;
  EFI_HANDLE                      ServiceHandle;
  EFI_SERVICE_BINDING_PROTOCOL    ServiceBinding;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;

  LIST_ENTRY                      ChildrenList;
  UINTN                           ChildrenNumber;

  UINT32                          Mtu;

  UINT16                          VlanId;
  UINT8                           Priority;
} MNP_SERVICE_DATA;

#define MNP_SERVICE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  MNP_SERVICE_DATA, \
  ServiceBinding, \
  MNP_SERVICE_DATA_SIGNATURE \
  )

#define MNP_SERVICE_DATA_FROM_LINK(a) \
  CR ( \
  (a), \
  MNP_SERVICE_DATA, \
  Link, \
  MNP_SERVICE_DATA_SIGNATURE \
  )

/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to test.
  @param[in]  RemainingDevicePath  Optional parameter use to pick a specific
                                   child device to start.

  @retval EFI_SUCCESS              This driver supports this device.
  @retval EFI_ALREADY_STARTED      This driver is already running on this device.
  @retval Others                   This driver does not support this device.

**/
EFI_STATUS
EFIAPI
MnpDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle. This service is called by the
  EFI boot service ConnectController(). In order to make drivers as small
  as possible, there are a few calling restrictions for this service.
  ConnectController() must follow these calling restrictions. If any other
  agent wishes to call Start() it must also follow these calling restrictions.

  @param[in]       This                 Protocol instance pointer.
  @param[in]       ControllerHandle     Handle of device to bind driver to.
  @param[in]       RemainingDevicePath  Optional parameter use to pick a specific
                                        child device to start.

  @retval EFI_SUCCESS           This driver is added to ControllerHandle.
  @retval EFI_ALREADY_STARTED   This driver is already running on ControllerHandle.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for Mnp Service Data.
  @retval Others                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
MnpDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle. This service is called by the
  EFI boot service DisconnectController(). In order to make drivers as
  small as possible, there are a few calling restrictions for this service.
  DisconnectController() must follow these calling restrictions. If any other
  agent wishes to call Stop() it must also follow these calling restrictions.

  @param[in]  This               Protocol instance pointer.
  @param[in]  ControllerHandle   Handle of device to stop driver on.
  @param[in]  NumberOfChildren   Number of Handles in ChildHandleBuffer. If
                                 number of children is zero stop the entire
                                 bus driver.
  @param[in]  ChildHandleBuffer  List of Child Handles to Stop.

  @retval EFI_SUCCESS            This driver is removed ControllerHandle.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
MnpDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  );

/**
  Creates a child handle with a set of I/O services.

  @param[in]       This              Protocol instance pointer.
  @param[in, out]  ChildHandle       Pointer to the handle of the child to create. If
                                     it is NULL, then a new handle is created. If
                                     it is not NULL, then the I/O services are added
                                     to the existing child handle.

  @retval EFI_SUCCESS                The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER      ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES       There are not enough resources available to
                                     create the child.
  @retval Others                     The child handle was not created.

**/
EFI_STATUS
EFIAPI
MnpServiceBindingCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  );

/**
  Destroys a child handle with a set of I/O services.

  The DestroyChild() function does the opposite of CreateChild(). It removes a
  protocol that was installed by CreateChild() from ChildHandle. If the removed
  protocol is the last protocol on ChildHandle, then ChildHandle is destroyed.

  @param[in]  This               Pointer to the EFI_SERVICE_BINDING_PROTOCOL
                                 instance.
  @param[in]  ChildHandle        Handle of the child to destroy.

  @retval EFI_SUCCESS            The protocol was removed from ChildHandle.
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
MnpServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  );

#endif
