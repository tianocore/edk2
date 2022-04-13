/** @file

  Driver Binding code and its private helpers for the virtio-net driver.

  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "VirtioNet.h"

#define RECEIVE_FILTERS_NO_MCAST  ((UINT32) (      \
          EFI_SIMPLE_NETWORK_RECEIVE_UNICAST     | \
          EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST   | \
          EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS   \
          ))

/*
  Temporarily enable then reset the virtio-net device in order to retrieve
  configuration values needed by Simple Network Protocol and Simple Network
  Mode fields.

  Only VirtioNetSnpPopulate() may call this function.

  If the function fails for any reason, the virtio-net device is moved to
  VSTAT_FAILED instead of being reset. This serves only informative purposes
  for the host side.

  param[in,out] Dev                 The VNET_DEV structure being created for
                                    the virtio-net device.
  param[out] MacAddress             MAC address configured by the host.
  param[out] MediaPresentSupported  Link status is made available by the host.
  param[out] MediaPresent           If link status is made available by the
                                    host, the current link status is stored in
                                    *MediaPresent. Otherwise MediaPresent is
                                    unused.

  @retval EFI_UNSUPPORTED           The host doesn't supply a MAC address.
  @return                           Status codes from VirtIo protocol members.
  @retval EFI_SUCCESS               Configuration values retrieved.
*/
STATIC
EFI_STATUS
EFIAPI
VirtioNetGetFeatures (
  IN OUT  VNET_DEV         *Dev,
  OUT     EFI_MAC_ADDRESS  *MacAddress,
  OUT     BOOLEAN          *MediaPresentSupported,
  OUT     BOOLEAN          *MediaPresent
  )
{
  EFI_STATUS  Status;
  UINT8       NextDevStat;
  UINT64      Features;
  UINTN       MacIdx;
  UINT16      LinkStatus;

  //
  // Interrogate the device for features (virtio-0.9.5, 2.2.1 Device
  // Initialization Sequence), but don't complete setting it up.
  //
  NextDevStat = 0;             // step 1 -- reset device
  Status      = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NextDevStat |= VSTAT_ACK;    // step 2 -- acknowledge device presence
  Status       = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto YieldDevice;
  }

  NextDevStat |= VSTAT_DRIVER; // step 3 -- we know how to drive it
  Status       = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto YieldDevice;
  }

  //
  // step 4a -- retrieve and validate features
  //
  Status = Dev->VirtIo->GetDeviceFeatures (Dev->VirtIo, &Features);
  if (EFI_ERROR (Status)) {
    goto YieldDevice;
  }

  //
  // get MAC address byte-wise
  //
  if ((Features & VIRTIO_NET_F_MAC) == 0) {
    Status = EFI_UNSUPPORTED;
    goto YieldDevice;
  }

  for (MacIdx = 0; MacIdx < SIZE_OF_VNET (Mac); ++MacIdx) {
    Status = Dev->VirtIo->ReadDevice (
                            Dev->VirtIo,
                            OFFSET_OF_VNET (Mac) + MacIdx, // Offset
                            1,                             // FieldSize
                            1,                             // BufferSize
                            &MacAddress->Addr[MacIdx]      // Buffer
                            );
    if (EFI_ERROR (Status)) {
      goto YieldDevice;
    }
  }

  //
  // check if link status is reported, and if so, what the link status is
  //
  if ((Features & VIRTIO_NET_F_STATUS) == 0) {
    *MediaPresentSupported = FALSE;
  } else {
    *MediaPresentSupported = TRUE;
    Status                 = VIRTIO_CFG_READ (Dev, LinkStatus, &LinkStatus);
    if (EFI_ERROR (Status)) {
      goto YieldDevice;
    }

    *MediaPresent = (BOOLEAN)((LinkStatus & VIRTIO_NET_S_LINK_UP) != 0);
  }

YieldDevice:
  Dev->VirtIo->SetDeviceStatus (
                 Dev->VirtIo,
                 EFI_ERROR (Status) ? VSTAT_FAILED : 0
                 );

  return Status;
}

/**
  Set up the Simple Network Protocol fields, the Simple Network Mode fields,
  and the Exit Boot Services Event of the virtio-net driver instance.

  This function may only be called by VirtioNetDriverBindingStart().

  @param[in,out] Dev  The VNET_DEV driver instance being created for the
                      virtio-net device.

  @return              Status codes from the CreateEvent() boot service or the
                       VirtioNetGetFeatures() function.
  @retval EFI_SUCCESS  Configuration successful.
*/
STATIC
EFI_STATUS
EFIAPI
VirtioNetSnpPopulate (
  IN OUT VNET_DEV  *Dev
  )
{
  EFI_STATUS  Status;

  //
  // We set up a function here that is asynchronously callable by an
  // external application to check if there are any packets available for
  // reception. The least urgent task priority level we can specify for such a
  // "software interrupt" is TPL_CALLBACK.
  //
  // TPL_CALLBACK is also the maximum TPL an SNP implementation is allowed to
  // run at (see 6.1 Event, Timer, and Task Priority Services in the UEFI
  // Specification 2.3.1+errC).
  //
  // Since we raise our TPL to TPL_CALLBACK in every single function that
  // accesses the device, and the external application also queues its interest
  // for received packets at the same TPL_CALLBACK, in effect the
  // VirtioNetIsPacketAvailable() function will never interrupt any
  // device-accessing driver function, it will be scheduled in isolation.
  //
  // TPL_CALLBACK (which basically this entire driver runs at) is allowed
  // for "[l]ong term operations (such as file system operations and disk
  // I/O)". Because none of our functions block, we'd satisfy an even stronger
  // requirement.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_CALLBACK,
                  &VirtioNetIsPacketAvailable,
                  Dev,
                  &Dev->Snp.WaitForPacket
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dev->Snp.Revision       = EFI_SIMPLE_NETWORK_PROTOCOL_REVISION;
  Dev->Snp.Start          = &VirtioNetStart;
  Dev->Snp.Stop           = &VirtioNetStop;
  Dev->Snp.Initialize     = &VirtioNetInitialize;
  Dev->Snp.Reset          = &VirtioNetReset;
  Dev->Snp.Shutdown       = &VirtioNetShutdown;
  Dev->Snp.ReceiveFilters = &VirtioNetReceiveFilters;
  Dev->Snp.StationAddress = &VirtioNetStationAddress;
  Dev->Snp.Statistics     = &VirtioNetStatistics;
  Dev->Snp.MCastIpToMac   = &VirtioNetMcastIpToMac;
  Dev->Snp.NvData         = &VirtioNetNvData;
  Dev->Snp.GetStatus      = &VirtioNetGetStatus;
  Dev->Snp.Transmit       = &VirtioNetTransmit;
  Dev->Snp.Receive        = &VirtioNetReceive;
  Dev->Snp.Mode           = &Dev->Snm;

  Dev->Snm.State           = EfiSimpleNetworkStopped;
  Dev->Snm.HwAddressSize   = SIZE_OF_VNET (Mac);
  Dev->Snm.MediaHeaderSize = SIZE_OF_VNET (Mac) +       // dst MAC
                             SIZE_OF_VNET (Mac) +       // src MAC
                             2;                         // Ethertype
  Dev->Snm.MaxPacketSize        = 1500;
  Dev->Snm.NvRamSize            = 0;
  Dev->Snm.NvRamAccessSize      = 0;
  Dev->Snm.ReceiveFilterMask    = RECEIVE_FILTERS_NO_MCAST;
  Dev->Snm.ReceiveFilterSetting = RECEIVE_FILTERS_NO_MCAST;
  Dev->Snm.MaxMCastFilterCount  = 0;
  Dev->Snm.MCastFilterCount     = 0;
  Dev->Snm.IfType               = 1;  // ethernet
  Dev->Snm.MacAddressChangeable = FALSE;
  Dev->Snm.MultipleTxSupported  = TRUE;

  ASSERT (SIZE_OF_VNET (Mac) <= sizeof (EFI_MAC_ADDRESS));

  Status = VirtioNetGetFeatures (
             Dev,
             &Dev->Snm.CurrentAddress,
             &Dev->Snm.MediaPresentSupported,
             &Dev->Snm.MediaPresent
             );
  if (EFI_ERROR (Status)) {
    goto CloseWaitForPacket;
  }

  CopyMem (
    &Dev->Snm.PermanentAddress,
    &Dev->Snm.CurrentAddress,
    SIZE_OF_VNET (Mac)
    );
  SetMem (&Dev->Snm.BroadcastAddress, SIZE_OF_VNET (Mac), 0xFF);

  //
  // VirtioNetExitBoot() is queued by ExitBootServices(); its purpose is to
  // cancel any pending virtio requests. The TPL_CALLBACK reasoning is
  // identical to the one above. There's one difference: this kind of
  // event is "globally visible", which means it can be signalled as soon as
  // we create it. We haven't raised our TPL here, hence VirtioNetExitBoot()
  // could be entered immediately. VirtioNetExitBoot() checks Dev->Snm.State,
  // so we're safe.
  //
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  &VirtioNetExitBoot,
                  Dev,
                  &Dev->ExitBoot
                  );
  if (EFI_ERROR (Status)) {
    goto CloseWaitForPacket;
  }

  return EFI_SUCCESS;

CloseWaitForPacket:
  gBS->CloseEvent (Dev->Snp.WaitForPacket);
  return Status;
}

/**
  Release any resources allocated by VirtioNetSnpPopulate().

  This function may only be called by VirtioNetDriverBindingStart(), when
  rolling back a partial, failed driver instance creation, and by
  VirtioNetDriverBindingStop(), when disconnecting a virtio-net device from the
  driver.

  @param[in,out] Dev  The VNET_DEV driver instance being destroyed.
*/
STATIC
VOID
EFIAPI
VirtioNetSnpEvacuate (
  IN OUT VNET_DEV  *Dev
  )
{
  //
  // This function runs either at TPL_CALLBACK already (from
  // VirtioNetDriverBindingStop()), or it is part of a teardown following
  // a partial, failed construction in VirtioNetDriverBindingStart(), when
  // WaitForPacket was never accessible to the world.
  //
  gBS->CloseEvent (Dev->ExitBoot);
  gBS->CloseEvent (Dev->Snp.WaitForPacket);
}

/**
  Tests to see if this driver supports a given controller. If a child device is
  provided, it further tests to see if this driver supports creating a handle
  for the specified child device.

  This function checks to see if the driver specified by This supports the
  device specified by ControllerHandle. Drivers will typically use the device
  path attached to ControllerHandle and/or the services from the bus I/O
  abstraction attached to ControllerHandle to determine if the driver supports
  ControllerHandle. This function may be called many times during platform
  initialization. In order to reduce boot times, the tests performed by this
  function must be very small, and take as little time as possible to execute.
  This function must not change the state of any hardware devices, and this
  function must be aware that the device specified by ControllerHandle may
  already be managed by the same driver or a different driver. This function
  must match its calls to AllocatePages() with FreePages(), AllocatePool() with
  FreePool(), and OpenProtocol() with CloseProtocol(). Because ControllerHandle
  may have been previously started by the same driver, if a protocol is already
  in the opened state, then it must not be closed with CloseProtocol(). This is
  required to guarantee the state of ControllerHandle is not modified by this
  function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                   instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This
                                   handle must support a protocol interface
                                   that supplies an I/O abstraction to the
                                   driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a
                                   device path.  This parameter is ignored by
                                   device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter
                                   is not NULL, then the bus driver must
                                   determine if the bus controller specified by
                                   ControllerHandle and the child controller
                                   specified by RemainingDevicePath are both
                                   supported by this bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the
                                   driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed
                                   by the driver specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed
                                   by a different driver or an application that
                                   requires exclusive access. Currently not
                                   implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the
                                   driver specified by This.
**/
STATIC
EFI_STATUS
EFIAPI
VirtioNetDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS              Status;
  VIRTIO_DEVICE_PROTOCOL  *VirtIo;

  //
  // Attempt to open the device with the VirtIo set of interfaces. On success,
  // the protocol is "instantiated" for the VirtIo device. Covers duplicate open
  // attempts (EFI_ALREADY_STARTED).
  //
  Status = gBS->OpenProtocol (
                  DeviceHandle,               // candidate device
                  &gVirtioDeviceProtocolGuid, // for generic VirtIo access
                  (VOID **)&VirtIo,           // handle to instantiate
                  This->DriverBindingHandle,  // requestor driver identity
                  DeviceHandle,               // ControllerHandle, according to
                                              // the UEFI Driver Model
                  EFI_OPEN_PROTOCOL_BY_DRIVER // get exclusive VirtIo access to
                                              // the device; to be released
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (VirtIo->SubSystemDeviceId != VIRTIO_SUBSYSTEM_NETWORK_CARD) {
    Status = EFI_UNSUPPORTED;
  }

  //
  // We needed VirtIo access only transitorily, to see whether we support the
  // device or not.
  //
  gBS->CloseProtocol (
         DeviceHandle,
         &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );
  return Status;
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service
  ConnectController(). As a result, much of the error checking on the
  parameters to Start() has been moved into this  common boot service. It is
  legal to call Start() from other locations,  but the following calling
  restrictions must be followed, or the system behavior will not be
  deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a
     naturally aligned EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver
     specified by This must have been called with the same calling parameters,
     and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                   instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This
                                   handle  must support a protocol interface
                                   that supplies  an I/O abstraction to the
                                   driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a
                                   device path.  This  parameter is ignored by
                                   device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter
                                   is NULL, then handles  for all the children
                                   of Controller are created by this driver.
                                   If this parameter is not NULL and the first
                                   Device Path Node is  not the End of Device
                                   Path Node, then only the handle for the
                                   child device specified by the first Device
                                   Path Node of  RemainingDevicePath is created
                                   by this driver. If the first Device Path
                                   Node of RemainingDevicePath is  the End of
                                   Device Path Node, no child handle is created
                                   by this driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a
                                   device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a
                                   lack of resources.
  @retval Others                   The driver failed to start the device.

**/
STATIC
EFI_STATUS
EFIAPI
VirtioNetDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  VNET_DEV                  *Dev;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  MAC_ADDR_DEVICE_PATH      MacNode;
  VOID                      *ChildVirtIo;

  //
  // allocate space for the driver instance
  //
  Dev = (VNET_DEV *)AllocateZeroPool (sizeof *Dev);
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Dev->Signature = VNET_SIG;

  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gVirtioDeviceProtocolGuid,
                  (VOID **)&Dev->VirtIo,
                  This->DriverBindingHandle,
                  DeviceHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreeVirtioNet;
  }

  //
  // now we can run a basic one-shot virtio-net initialization required to
  // retrieve the MAC address
  //
  Status = VirtioNetSnpPopulate (Dev);
  if (EFI_ERROR (Status)) {
    goto CloseVirtIo;
  }

  //
  // get the device path of the virtio-net device -- one-shot open
  //
  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  DeviceHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    goto Evacuate;
  }

  //
  // create another device path that has the MAC address appended
  //
  MacNode.Header.Type    = MESSAGING_DEVICE_PATH;
  MacNode.Header.SubType = MSG_MAC_ADDR_DP;
  SetDevicePathNodeLength (&MacNode, sizeof MacNode);
  CopyMem (
    &MacNode.MacAddress,
    &Dev->Snm.CurrentAddress,
    sizeof (EFI_MAC_ADDRESS)
    );
  MacNode.IfType = Dev->Snm.IfType;

  Dev->MacDevicePath = AppendDevicePathNode (DevicePath, &MacNode.Header);
  if (Dev->MacDevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Evacuate;
  }

  //
  // create a child handle with the Simple Network Protocol and the new
  // device path installed on it
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Dev->MacHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  &Dev->Snp,
                  &gEfiDevicePathProtocolGuid,
                  Dev->MacDevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto FreeMacDevicePath;
  }

  //
  // make a note that we keep this device open with VirtIo for the sake of this
  // child
  //
  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gVirtioDeviceProtocolGuid,
                  &ChildVirtIo,
                  This->DriverBindingHandle,
                  Dev->MacHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto UninstallMultiple;
  }

  return EFI_SUCCESS;

UninstallMultiple:
  gBS->UninstallMultipleProtocolInterfaces (
         Dev->MacHandle,
         &gEfiDevicePathProtocolGuid,
         Dev->MacDevicePath,
         &gEfiSimpleNetworkProtocolGuid,
         &Dev->Snp,
         NULL
         );

FreeMacDevicePath:
  FreePool (Dev->MacDevicePath);

Evacuate:
  VirtioNetSnpEvacuate (Dev);

CloseVirtIo:
  gBS->CloseProtocol (
         DeviceHandle,
         &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

FreeVirtioNet:
  FreePool (Dev);

  return Status;
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service
  DisconnectController().  As a result, much of the error checking on the
  parameters to Stop() has been moved  into this common boot service. It is
  legal to call Stop() from other locations,  but the following calling
  restrictions must be followed, or the system behavior will not be
  deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous
     call to this same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a
     valid EFI_HANDLE. In addition, all of these handles must have been created
     in this driver's Start() function, and the Start() function must have
     called OpenProtocol() on ControllerHandle with an Attribute of
     EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The
                                handle must  support a bus specific I/O
                                protocol for the driver  to use to stop the
                                device.
  @param[in]  NumberOfChildren  The number of child device handles in
                                ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be
                                NULL  if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device
                                error.

**/
STATIC
EFI_STATUS
EFIAPI
VirtioNetDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  if (NumberOfChildren > 0) {
    //
    // free all resources for whose access we need the child handle, because
    // the child handle is going away
    //
    EFI_STATUS                   Status;
    EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;
    VNET_DEV                     *Dev;
    EFI_TPL                      OldTpl;

    ASSERT (NumberOfChildren == 1);

    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[0],
                    &gEfiSimpleNetworkProtocolGuid,
                    (VOID **)&Snp,
                    This->DriverBindingHandle,
                    DeviceHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    ASSERT_EFI_ERROR (Status);
    Dev = VIRTIO_NET_FROM_SNP (Snp);

    //
    // prevent any interference with WaitForPacket
    //
    OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

    ASSERT (Dev->MacHandle == ChildHandleBuffer[0]);
    if (Dev->Snm.State != EfiSimpleNetworkStopped) {
      //
      // device in use, cannot stop driver instance
      //
      Status = EFI_DEVICE_ERROR;
    } else {
      gBS->CloseProtocol (
             DeviceHandle,
             &gVirtioDeviceProtocolGuid,
             This->DriverBindingHandle,
             Dev->MacHandle
             );
      gBS->UninstallMultipleProtocolInterfaces (
             Dev->MacHandle,
             &gEfiDevicePathProtocolGuid,
             Dev->MacDevicePath,
             &gEfiSimpleNetworkProtocolGuid,
             &Dev->Snp,
             NULL
             );
      FreePool (Dev->MacDevicePath);
      VirtioNetSnpEvacuate (Dev);
      FreePool (Dev);
    }

    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  //
  // release remaining resources, tied directly to the parent handle
  //
  gBS->CloseProtocol (
         DeviceHandle,
         &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

  return EFI_SUCCESS;
}

EFI_DRIVER_BINDING_PROTOCOL  gVirtioNetDriverBinding = {
  &VirtioNetDriverBindingSupported,
  &VirtioNetDriverBindingStart,
  &VirtioNetDriverBindingStop,
  0x10,
  NULL,
  NULL
};
