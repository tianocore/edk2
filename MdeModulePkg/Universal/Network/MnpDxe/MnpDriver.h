/** @file

Copyright (c) 2005 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  MnpDriver.h

Abstract:


**/

#ifndef _MNP_DRIVER_H_
#define _MNP_DRIVER_H_
#include <PiDxe.h>

#include <Protocol/ManagedNetwork.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/ServiceBinding.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/NetLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

//
// Required Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gMnpDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gMnpComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gMnpComponentName2;

#define MNP_SERVICE_DATA_SIGNATURE  EFI_SIGNATURE_32 ('M', 'n', 'p', 'S')

typedef struct _MNP_SERVICE_DATA {
  UINT32                        Signature;

  EFI_HANDLE                    ControllerHandle;

  EFI_SERVICE_BINDING_PROTOCOL  ServiceBinding;
  EFI_SIMPLE_NETWORK_PROTOCOL   *Snp;

  UINT32                        Mtu;

  LIST_ENTRY                    ChildrenList;
  UINTN                         ChildrenNumber;
  UINTN                         ConfiguredChildrenNumber;

  LIST_ENTRY                    GroupAddressList;
  UINT32                        GroupAddressCount;

  EFI_EVENT                     TxTimeoutEvent;

  NET_BUF_QUEUE                 FreeNbufQue;
  INTN                          NbufCnt;

  EFI_EVENT                     PollTimer;
  BOOLEAN                       EnableSystemPoll;

  EFI_EVENT                     TimeoutCheckTimer;

  UINT32                        UnicastCount;
  UINT32                        BroadcastCount;
  UINT32                        MulticastCount;
  UINT32                        PromiscuousCount;

  //
  // The size of the data buffer in the MNP_PACKET_BUFFER used to
  // store a packet.
  //
  UINT32                        BufferLength;
  UINT32                        PaddingSize;
  NET_BUF                       *RxNbufCache;
  UINT8                         *TxBuf;
} MNP_SERVICE_DATA;

#define MNP_SERVICE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  MNP_SERVICE_DATA, \
  ServiceBinding, \
  MNP_SERVICE_DATA_SIGNATURE \
  )

/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

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
  EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

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
  EFI boot service DisconnectController(). In order to
  make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController()
  must follow these calling restrictions. If any other agent wishes
  to call Stop() it must also follow these calling restrictions.
  
  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
MnpDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  );

/**
  Creates a child handle with a set of I/O services.

  @param  This                   Protocol instance pointer.
  @param  ChildHandle            Pointer to the handle of the child to create. If
                                 it is NULL, then a new handle is created. If it is
                                 not NULL, then the I/O services are  added to the
                                 existing child handle.

  @retval EFI_SUCCES             The child handle was created with the I/O
                                 services.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources availabe to create
                                 the child.
  @retval other                  The child handle was not created.

**/
EFI_STATUS
EFIAPI
MnpServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  );

/**
  Destroys a child handle with a set of I/O services.

  @param  This                   Protocol instance pointer.
  @param  ChildHandle            Handle of the child to destroy.

  @retval EFI_SUCCES             The I/O services were removed from the child
                                 handle.
  @retval EFI_UNSUPPORTED        The child handle does not support the I/O services
                                  that are being removed.
  @retval EFI_INVALID_PARAMETER  Child handle is not a valid EFI Handle.
  @retval EFI_ACCESS_DENIED      The child handle could not be destroyed because
                                 its  I/O services are being used.
  @retval other                  The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
MnpServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  );

#endif
