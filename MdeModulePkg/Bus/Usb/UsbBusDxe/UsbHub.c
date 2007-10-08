/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:

    UsbHub.c

  Abstract:

    Unified interface for RootHub and Hub

  Revision History


**/

#include "UsbBus.h"

//
// USB hub class specific requests. Although USB hub
// is related to an interface, these requests are sent
// to the control endpoint of the device.
//


/**
  USB hub control transfer to clear the hub feature

  @param  HubDev                The device of the hub
  @param  Feature               The feature to clear

  @retval EFI_SUCCESS           Feature of the hub is cleared
  @retval Others                Failed to clear the feature

**/
STATIC
EFI_STATUS
UsbHubCtrlClearHubFeature (
  IN USB_DEVICE           *HubDev,
  IN UINT16               Feature
  )
{
  EFI_STATUS              Status;

  Status = UsbCtrlRequest (
             HubDev,
             EfiUsbNoData,
             USB_REQ_TYPE_CLASS,
             USB_HUB_TARGET_HUB,
             USB_HUB_REQ_CLEAR_FEATURE,
             Feature,
             0,
             NULL,
             0
             );

  return Status;
}


/**
  Clear the feature of the device's port

  @param  HubDev                The hub device
  @param  Port                  The port to clear feature
  @param  Feature               The feature to clear

  @retval EFI_SUCCESS           The feature of the port is cleared.
  @retval Others                Failed to clear the feature.

**/
STATIC
EFI_STATUS
UsbHubCtrlClearPortFeature (
  IN USB_DEVICE           *HubDev,
  IN UINT8                Port,
  IN UINT16               Feature
  )
{
  EFI_STATUS              Status;

  //
  // In USB bus, all the port index starts from 0. But HUB
  // indexes its port from 1. So, port number is added one.
  //
  Status = UsbCtrlRequest (
             HubDev,
             EfiUsbNoData,
             USB_REQ_TYPE_CLASS,
             USB_HUB_TARGET_PORT,
             USB_HUB_REQ_CLEAR_FEATURE,
             Feature,
             (UINT16) (Port + 1),
             NULL,
             0
             );

  return Status;
}



/**
  Clear the transaction translate buffer if full/low
  speed control/bulk transfer failed and the transfer
  uses this hub as translator.Remember to clear the TT
  buffer of transaction translator, not that of the
  parent.

  @param  HubDev                The hub device
  @param  Port                  The port of the hub
  @param  DevAddr               Address of the failed transaction
  @param  EpNum                 The endpoint number of the failed transaction
  @param  EpType                The type of failed transaction

  @retval EFI_SUCCESS           The TT buffer is cleared
  @retval Others                Failed to clear the TT buffer

**/
EFI_STATUS
UsbHubCtrlClearTTBuffer (
  IN USB_DEVICE           *HubDev,
  IN UINT8                Port,
  IN UINT16               DevAddr,
  IN UINT16               EpNum,
  IN UINT16               EpType
  )
{
  EFI_STATUS              Status;
  UINT16                  Value;

  //
  // Check USB2.0 spec page 424 for wValue's encoding
  //
  Value = (UINT16) ((EpNum & 0x0F) | (DevAddr << 4) |
          ((EpType & 0x03) << 11) | ((EpNum & 0x80) << 15));

  Status = UsbCtrlRequest (
             HubDev,
             EfiUsbNoData,
             USB_REQ_TYPE_CLASS,
             USB_HUB_TARGET_PORT,
             USB_HUB_REQ_CLEAR_TT,
             Value,
             (UINT16) (Port + 1),
             NULL,
             0
             );

  return Status;
}


/**
  Usb hub control transfer to get the hub descriptor

  @param  HubDev                The hub device
  @param  Buf                   The buffer to hold the descriptor
  @param  Len                   The length to retrieve

  @retval EFI_SUCCESS           The hub descriptor is retrieved
  @retval Others                Failed to retrieve the hub descriptor

**/
STATIC
EFI_STATUS
UsbHubCtrlGetHubDesc (
  IN  USB_DEVICE          *HubDev,
  OUT VOID                *Buf,
  IN  UINTN               Len
  )
{
  EFI_STATUS              Status;

  Status = UsbCtrlRequest (
             HubDev,
             EfiUsbDataIn,
             USB_REQ_TYPE_CLASS,
             USB_HUB_TARGET_HUB,
             USB_HUB_REQ_GET_DESC,
             (UINT16) (USB_DESC_TYPE_HUB << 8),
             0,
             Buf,
             Len
             );

  return Status;
}


/**
  Usb hub control transfer to get the hub status

  @param  HubDev                The hub device
  @param  State                 The variable to return the status

  @retval EFI_SUCCESS           The hub status is returned in State
  @retval Others                Failed to get the hub status

**/
STATIC
EFI_STATUS
UsbHubCtrlGetHubStatus (
  IN  USB_DEVICE          *HubDev,
  OUT UINT32              *State
  )
{
  EFI_STATUS              Status;

  Status = UsbCtrlRequest (
             HubDev,
             EfiUsbDataIn,
             USB_REQ_TYPE_CLASS,
             USB_HUB_TARGET_HUB,
             USB_HUB_REQ_GET_STATUS,
             0,
             0,
             State,
             4
             );

  return Status;
}


/**
  Usb hub control transfer to get the port status

  @param  HubDev                The hub device
  @param  Port                  The port of the hub
  @param  State                 Variable to return the hub port state

  @retval EFI_SUCCESS           The port state is returned in State
  @retval Others                Failed to retrive the port state

**/
STATIC
EFI_STATUS
UsbHubCtrlGetPortStatus (
  IN  USB_DEVICE          *HubDev,
  IN  UINT8               Port,
  OUT VOID                *State
  )
{
  EFI_STATUS              Status;

  //
  // In USB bus, all the port index starts from 0. But HUB
  // indexes its port from 1. So, port number is added one.
  // No need to convert the hub bit to UEFI definition, they
  // are the same
  //
  Status = UsbCtrlRequest (
             HubDev,
             EfiUsbDataIn,
             USB_REQ_TYPE_CLASS,
             USB_HUB_TARGET_PORT,
             USB_HUB_REQ_GET_STATUS,
             0,
             (UINT16) (Port + 1),
             State,
             4
             );

  return Status;
}


/**
  Usb hub control transfer to reset the TT (Transaction Transaltor)

  @param  HubDev                The hub device
  @param  Port                  The port of the hub

  @retval EFI_SUCCESS           The TT of the hub is reset
  @retval Others                Failed to reset the port

**/
EFI_STATUS
UsbHubCtrlResetTT (
  IN  USB_DEVICE          *HubDev,
  IN  UINT8               Port
  )
{
  EFI_STATUS              Status;

  Status = UsbCtrlRequest (
             HubDev,
             EfiUsbNoData,
             USB_REQ_TYPE_CLASS,
             USB_HUB_TARGET_HUB,
             USB_HUB_REQ_RESET_TT,
             0,
             (UINT16) (Port + 1),
             NULL,
             0
             );

  return Status;
}


/**
  Usb hub control transfer to set the hub feature

  @param  HubDev                The hub device
  @param  Feature               The feature to set

  @retval EFI_SUCESS            The feature is set for the hub
  @retval Others                Failed to set the feature

**/
EFI_STATUS
UsbHubCtrlSetHubFeature (
  IN  USB_DEVICE          *HubDev,
  IN  UINT8               Feature
  )
{
  EFI_STATUS              Status;

  Status = UsbCtrlRequest (
             HubDev,
             EfiUsbNoData,
             USB_REQ_TYPE_CLASS,
             USB_HUB_TARGET_HUB,
             USB_HUB_REQ_SET_FEATURE,
             Feature,
             0,
             NULL,
             0
             );

  return Status;
}


/**
  Usb hub control transfer to set the port feature

  @param  HubDev                The Usb hub device
  @param  Port                  The Usb port to set feature for
  @param  Feature               The feature to set

  @retval EFI_SUCCESS           The feature is set for the port
  @retval Others                Failed to set the feature

**/
STATIC
EFI_STATUS
UsbHubCtrlSetPortFeature (
  IN USB_DEVICE           *HubDev,
  IN UINT8                Port,
  IN UINT8                Feature
  )
{
  EFI_STATUS              Status;

  //
  // In USB bus, all the port index starts from 0. But HUB
  // indexes its port from 1. So, port number is added one.
  //
  Status = UsbCtrlRequest (
             HubDev,
             EfiUsbNoData,
             USB_REQ_TYPE_CLASS,
             USB_HUB_TARGET_PORT,
             USB_HUB_REQ_SET_FEATURE,
             Feature,
             (UINT16) (Port + 1),
             NULL,
             0
             );

  return Status;
}


/**
  Read the whole usb hub descriptor. It is necessary
  to do it in two steps because hub descriptor is of
  variable length

  @param  HubDev                The hub device
  @param  HubDesc               The variable to return the descriptor

  @retval EFI_SUCCESS           The hub descriptor is read
  @retval Others                Failed to read the hub descriptor

**/
STATIC
EFI_STATUS
UsbHubReadDesc (
  IN  USB_DEVICE              *HubDev,
  OUT EFI_USB_HUB_DESCRIPTOR  *HubDesc
  )
{
  EFI_STATUS              Status;

  //
  // First get the hub descriptor length
  //
  Status = UsbHubCtrlGetHubDesc (HubDev, HubDesc, 2);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the whole hub descriptor
  //
  Status = UsbHubCtrlGetHubDesc (HubDev, HubDesc, HubDesc->Length);

  return Status;
}



/**
  Ack the hub change bits. If these bits are not ACKed, Hub will
  always return changed bit map from its interrupt endpoint.

  @param  HubDev                The hub device

  @retval EFI_SUCCESS           The hub change status is ACKed
  @retval Others                Failed to ACK the hub status

**/
EFI_STATUS
UsbHubAckHubStatus (
  IN  USB_DEVICE         *HubDev
  )
{
  EFI_USB_PORT_STATUS     HubState;
  EFI_STATUS              Status;

  Status = UsbHubCtrlGetHubStatus (HubDev, (UINT32 *) &HubState);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (USB_BIT_IS_SET (HubState.PortChangeStatus, USB_HUB_STAT_C_LOCAL_POWER)) {
    UsbHubCtrlClearHubFeature (HubDev, USB_HUB_C_HUB_LOCAL_POWER);
  }

  if (USB_BIT_IS_SET (HubState.PortChangeStatus, USB_HUB_STAT_C_OVER_CURRENT)) {
    UsbHubCtrlClearHubFeature (HubDev, USB_HUB_C_HUB_OVER_CURRENT);
  }

  return EFI_SUCCESS;
}


/**
  Test whether the interface is a hub interface.

  @param  UsbIf                 The interface to test

  @retval TRUE                  The interface is a hub interface
  @retval FALSE                 The interface isn't a hub interface

**/
BOOLEAN
UsbIsHubInterface (
  IN USB_INTERFACE        *UsbIf
  )
{
  EFI_USB_INTERFACE_DESCRIPTOR  *Setting;

  //
  // If the hub is a high-speed hub with multiple TT,
  // the hub will has a default setting of single TT.
  //
  Setting = &UsbIf->IfSetting->Desc;

  if ((Setting->InterfaceClass == USB_HUB_CLASS_CODE) &&
      (Setting->InterfaceSubClass == USB_HUB_SUBCLASS_CODE)) {

    return TRUE;
  }

  return FALSE;
}


/**
  The callback function to the USB hub status change
  interrupt endpoint. It is called periodically by
  the underlying host controller.

  @param  Data                  The data read
  @param  DataLength            The length of the data read
  @param  Context               The context
  @param  Result                The result of the last interrupt transfer

  @retval EFI_SUCCESS           The process is OK
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource

**/
STATIC
EFI_STATUS
UsbOnHubInterrupt (
  IN  VOID                *Data,
  IN  UINTN               DataLength,
  IN  VOID                *Context,
  IN  UINT32              Result
  )
{
  USB_INTERFACE               *HubIf;
  EFI_USB_IO_PROTOCOL         *UsbIo;
  EFI_USB_ENDPOINT_DESCRIPTOR *EpDesc;
  EFI_STATUS                  Status;

  HubIf   = (USB_INTERFACE *) Context;
  UsbIo   = &(HubIf->UsbIo);
  EpDesc  = &(HubIf->HubEp->Desc);

  if (Result != EFI_USB_NOERROR) {
    //
    // If endpoint is stalled, clear the stall. Use UsbIo to access
    // the control transfer so internal status are maintained.
    //
    if (USB_BIT_IS_SET (Result, EFI_USB_ERR_STALL)) {
      UsbIoClearFeature (
        UsbIo,
        USB_TARGET_ENDPOINT,
        USB_FEATURE_ENDPOINT_HALT,
        EpDesc->EndpointAddress
        );
    }

    //
    // Delete and submit a new async interrupt
    //
    Status = UsbIo->UsbAsyncInterruptTransfer (
                      UsbIo,
                      EpDesc->EndpointAddress,
                      FALSE,
                      0,
                      0,
                      NULL,
                      NULL
                      );

    if (EFI_ERROR (Status)) {
      DEBUG (( EFI_D_ERROR, "UsbOnHubInterrupt: failed to remove async transfer - %r\n", Status));
      return Status;
    }

    Status = UsbIo->UsbAsyncInterruptTransfer (
                      UsbIo,
                      EpDesc->EndpointAddress,
                      TRUE,
                      USB_HUB_POLL_INTERVAL,
                      HubIf->NumOfPort / 8 + 1,
                      UsbOnHubInterrupt,
                      HubIf
                      );

    if (EFI_ERROR (Status)) {
      DEBUG (( EFI_D_ERROR, "UsbOnHubInterrupt: failed to submit new async transfer - %r\n", Status));
    }

    return Status;
  }

  if ((DataLength == 0) || (Data == NULL)) {
    return EFI_SUCCESS;
  }

  //
  // OK, actually something is changed, save the change map
  // then signal the HUB to do enumeration. This is a good
  // practise since UsbOnHubInterrupt is called in the context
  // of host contrller's AsyncInterrupt monitor.
  //
  HubIf->ChangeMap = AllocateZeroPool (DataLength);

  if (HubIf->ChangeMap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (HubIf->ChangeMap, Data, DataLength);
  gBS->SignalEvent (HubIf->HubNotify);

  return EFI_SUCCESS;
}

//
// Array that maps the change bit to feature value which is
// used to clear these change bit. USB HUB API will clear
// these change bit automatically. For non-root hub, these
// bits determine whether hub will report the port in changed
// bit maps.
//
#define USB_HUB_MAP_SIZE  5

USB_CHANGE_FEATURE_MAP  mHubFeatureMap[USB_HUB_MAP_SIZE] = {
  {USB_PORT_STAT_C_CONNECTION,  EfiUsbPortConnectChange},
  {USB_PORT_STAT_C_ENABLE,      EfiUsbPortEnableChange},
  {USB_PORT_STAT_C_SUSPEND,     EfiUsbPortSuspendChange},
  {USB_PORT_STAT_C_OVERCURRENT, EfiUsbPortOverCurrentChange},
  {USB_PORT_STAT_C_RESET,       EfiUsbPortResetChange},
};

#define USB_ROOT_HUB_MAP_SIZE 5

USB_CHANGE_FEATURE_MAP  mRootHubFeatureMap[USB_ROOT_HUB_MAP_SIZE] = {
  {USB_PORT_STAT_C_CONNECTION,  EfiUsbPortConnectChange},
  {USB_PORT_STAT_C_ENABLE,      EfiUsbPortEnableChange},
  {USB_PORT_STAT_C_SUSPEND,     EfiUsbPortSuspendChange},
  {USB_PORT_STAT_C_OVERCURRENT, EfiUsbPortOverCurrentChange},
  {USB_PORT_STAT_C_RESET,       EfiUsbPortResetChange},
};



/**
  Initialize the device for a non-root hub

  @param  HubIf                 The USB hub interface

  @retval EFI_SUCCESS           The hub is initialized
  @retval EFI_DEVICE_ERROR      Failed to initialize the hub

**/
STATIC
EFI_STATUS
UsbHubInit (
  IN USB_INTERFACE        *HubIf
  )
{
  EFI_USB_HUB_DESCRIPTOR  HubDesc;
  USB_ENDPOINT_DESC       *EpDesc;
  USB_INTERFACE_SETTING   *Setting;
  EFI_USB_IO_PROTOCOL     *UsbIo;
  USB_DEVICE              *HubDev;
  EFI_STATUS              Status;
  UINT8                   Index;

  //
  // Locate the interrupt endpoint for port change map
  //
  HubIf->IsHub  = FALSE;
  Setting       = HubIf->IfSetting;
  HubDev        = HubIf->Device;
  EpDesc        = NULL;

  for (Index = 0; Index < Setting->Desc.NumEndpoints; Index++) {
    ASSERT ((Setting->Endpoints != NULL) && (Setting->Endpoints[Index] != NULL));

    EpDesc = Setting->Endpoints[Index];

    if (USB_BIT_IS_SET (EpDesc->Desc.EndpointAddress, USB_ENDPOINT_DIR_IN) &&
       (USB_ENDPOINT_TYPE (&EpDesc->Desc) == USB_ENDPOINT_INTERRUPT)) {
      break;
    }
  }

  if (Index == Setting->Desc.NumEndpoints) {
    DEBUG (( EFI_D_ERROR, "UsbHubInit: no interrupt endpoint found for hub %d\n", HubDev->Address));
    return EFI_DEVICE_ERROR;
  }

  Status = UsbHubReadDesc (HubDev, &HubDesc);

  if (EFI_ERROR (Status)) {
    DEBUG (( EFI_D_ERROR, "UsbHubInit: failed to read HUB descriptor %r\n", Status));
    return Status;
  }

  HubIf->NumOfPort = HubDesc.NumPorts;

  DEBUG (( EFI_D_INFO, "UsbHubInit: hub %d has %d ports\n", HubDev->Address,HubIf->NumOfPort));

  //
  // Create an event to enumerate the hub's port. On
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  UsbHubEnumeration,
                  HubIf,
                  &HubIf->HubNotify
                  );

  if (EFI_ERROR (Status)) {
    DEBUG (( EFI_D_ERROR, "UsbHubInit: failed to create signal for hub %d - %r\n",
                HubDev->Address, Status));

    return Status;
  }

  //
  // Create AsyncInterrupt to query hub port change endpoint
  // periodically. If the hub ports are changed, hub will return
  // changed port map from the interrupt endpoint. The port map
  // must be able to hold (HubIf->NumOfPort + 1) bits (one bit for
  // host change status).
  //
  UsbIo  = &HubIf->UsbIo;
  Status = UsbIo->UsbAsyncInterruptTransfer (
                    UsbIo,
                    EpDesc->Desc.EndpointAddress,
                    TRUE,
                    USB_HUB_POLL_INTERVAL,
                    HubIf->NumOfPort / 8 + 1,
                    UsbOnHubInterrupt,
                    HubIf
                    );

  if (EFI_ERROR (Status)) {
    DEBUG (( EFI_D_ERROR, "UsbHubInit: failed to queue interrupt transfer for hub %d - %r\n",
                HubDev->Address, Status));

    gBS->CloseEvent (HubIf->HubNotify);
    HubIf->HubNotify = NULL;

    return Status;
  }

  //
  // OK, set IsHub to TRUE. Now usb bus can handle this device
  // as a working HUB. If failed eariler, bus driver will not
  // recognize it as a hub. Other parts of the bus should be able
  // to work.
  //
  HubIf->IsHub  = TRUE;
  HubIf->HubApi = &mUsbHubApi;
  HubIf->HubEp  = EpDesc;

  //
  // Feed power to all the hub ports. It should be ok
  // for both gang/individual powered hubs.
  //
  for (Index = 0; Index < HubDesc.NumPorts; Index++) {
    UsbHubCtrlSetPortFeature (HubIf->Device, Index, (EFI_USB_PORT_FEATURE) USB_HUB_PORT_POWER);
  }

  gBS->Stall (HubDesc.PwrOn2PwrGood * USB_SET_PORT_POWER_STALL);
  UsbHubAckHubStatus (HubIf->Device);

  DEBUG (( EFI_D_INFO, "UsbHubInit: hub %d initialized\n", HubDev->Address));
  return Status;
}



/**
  Get the port status. This function is required to
  ACK the port change bits although it will return
  the port changes in PortState. Bus enumeration code
  doesn't need to ACK the port change bits.

  @param  HubIf                 The hub interface
  @param  Port                  The port of the hub to get state
  @param  PortState             Variable to return the port state

  @retval EFI_SUCCESS           The port status is successfully returned
  @retval Others                Failed to return the status

**/
STATIC
EFI_STATUS
UsbHubGetPortStatus (
  IN  USB_INTERFACE       *HubIf,
  IN  UINT8               Port,
  OUT EFI_USB_PORT_STATUS *PortState
  )
{
  EFI_STATUS              Status;

  Status  = UsbHubCtrlGetPortStatus (HubIf->Device, Port, PortState);

  return Status;
}



/**
  Clear the port change status.

  @param  HubIf                 The hub interface
  @param  Port                  The hub port

  @return None

**/
STATIC
VOID
UsbHubClearPortChange (
  IN USB_INTERFACE        *HubIf,
  IN UINT8                Port
  )
{
  EFI_USB_PORT_STATUS     PortState;
  USB_CHANGE_FEATURE_MAP  *Map;
  UINTN                   Index;
  EFI_STATUS              Status;

  Status = UsbHubGetPortStatus (HubIf, Port, &PortState);

  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // OK, get the usb port status, now ACK the change bits.
  // Don't return error when failed to clear the change bits.
  // It may lead to extra port state report. USB bus should
  // be able to handle this.
  //
  for (Index = 0; Index < USB_HUB_MAP_SIZE; Index++) {
    Map = &mHubFeatureMap[Index];

    if (USB_BIT_IS_SET (PortState.PortChangeStatus, Map->ChangedBit)) {
      UsbHubCtrlClearPortFeature (HubIf->Device, Port, (UINT16) Map->Feature);
    }
  }
}



/**
  Function to set the port feature for non-root hub

  @param  HubIf                 The hub interface
  @param  Port                  The port of the hub
  @param  Feature               The feature of the port to set

  @retval EFI_SUCCESS           The hub port feature is set
  @retval Others                Failed to set the port feature

**/
STATIC
EFI_STATUS
UsbHubSetPortFeature (
  IN USB_INTERFACE        *HubIf,
  IN UINT8                Port,
  IN EFI_USB_PORT_FEATURE Feature
  )
{
  EFI_STATUS              Status;

  Status = UsbHubCtrlSetPortFeature (HubIf->Device, Port, (UINT8) Feature);

  return Status;
}


/**
  Interface function to clear the port feature for non-root hub

  @param  HubIf                 The hub interface
  @param  Port                  The port of the hub to clear feature for
  @param  Feature               The feature to clear

  @retval EFI_SUCCESS           The port feature is cleared
  @retval Others                Failed to clear the port feature

**/
STATIC
EFI_STATUS
UsbHubClearPortFeature (
  IN USB_INTERFACE        *HubIf,
  IN UINT8                Port,
  IN EFI_USB_PORT_FEATURE Feature
  )
{
  EFI_STATUS              Status;

  Status = UsbHubCtrlClearPortFeature (HubIf->Device, Port, (UINT8) Feature);

  return Status;
}


/**
  Interface funtion to reset the port

  @param  HubIf                 The hub interface
  @param  Port                  The port to reset

  @retval EFI_SUCCESS           The hub port is reset
  @retval EFI_TIMEOUT           Failed to reset the port in time
  @retval Others                Failed to reset the port

**/
STATIC
EFI_STATUS
UsbHubResetPort (
  IN USB_INTERFACE        *HubIf,
  IN UINT8                Port
  )
{
  EFI_USB_PORT_STATUS     PortState;
  UINTN                   Index;
  EFI_STATUS              Status;

  Status  = UsbHubSetPortFeature (HubIf, Port, (EFI_USB_PORT_FEATURE) USB_HUB_PORT_RESET);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Drive the reset signal for at least 10ms. Check USB 2.0 Spec
  // section 7.1.7.5 for timing requirements.
  //
  gBS->Stall (USB_SET_PORT_RESET_STALL);

  //
  // USB hub will clear RESET bit if reset is actually finished.
  //
  ZeroMem (&PortState, sizeof (EFI_USB_PORT_STATUS));

  for (Index = 0; Index < USB_WAIT_PORT_STS_CHANGE_LOOP; Index++) {
    Status = UsbHubGetPortStatus (HubIf, Port, &PortState);

    if (!EFI_ERROR (Status) &&
        !USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_RESET)) {

      return EFI_SUCCESS;
    }

    gBS->Stall (USB_WAIT_PORT_STS_CHANGE_STALL);
  }

  return EFI_TIMEOUT;
}


/**
  Release the hub's control of the interface

  @param  HubIf                 The hub interface

  @retval EFI_SUCCESS           The interface is release of hub control

**/
STATIC
EFI_STATUS
UsbHubRelease (
  IN USB_INTERFACE        *HubIf
  )
{
  EFI_USB_IO_PROTOCOL     *UsbIo;
  EFI_STATUS              Status;

  UsbIo  = &HubIf->UsbIo;
  Status = UsbIo->UsbAsyncInterruptTransfer (
                    UsbIo,
                    HubIf->HubEp->Desc.EndpointAddress,
                    FALSE,
                    USB_HUB_POLL_INTERVAL,
                    0,
                    NULL,
                    0
                    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseEvent (HubIf->HubNotify);

  HubIf->IsHub      = FALSE;
  HubIf->HubApi     = NULL;
  HubIf->HubEp      = NULL;
  HubIf->HubNotify  = NULL;

  DEBUG (( EFI_D_INFO, "UsbHubRelease: hub device %d released\n", HubIf->Device->Address));
  return EFI_SUCCESS;
}



/**
  Initialize the interface for root hub

  @param  HubIf                 The root hub interface

  @retval EFI_SUCCESS           The interface is initialied for root hub
  @retval Others                Failed to initialize the hub

**/
STATIC
EFI_STATUS
UsbRootHubInit (
  IN USB_INTERFACE        *HubIf
  )
{
  EFI_STATUS              Status;
  UINT8                   MaxSpeed;
  UINT8                   NumOfPort;
  UINT8                   Support64;

  Status = UsbHcGetCapability (HubIf->Device->Bus, &MaxSpeed, &NumOfPort, &Support64);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG (( EFI_D_INFO, "UsbRootHubInit: root hub %x - max speed %d, %d ports\n",
              HubIf, MaxSpeed, NumOfPort));

  HubIf->IsHub      = TRUE;
  HubIf->HubApi     = &mUsbRootHubApi;
  HubIf->HubEp      = NULL;
  HubIf->MaxSpeed   = MaxSpeed;
  HubIf->NumOfPort  = NumOfPort;
  HubIf->HubNotify  = NULL;

  //
  // Create a timer to poll root hub ports periodically
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  UsbRootHubEnumeration,
                  HubIf,
                  &HubIf->HubNotify
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // It should signal the event immediately here, or device detection
  // by bus enumeration might be delayed by the timer interval.
  //
  gBS->SignalEvent (HubIf->HubNotify);

  Status = gBS->SetTimer (
                  HubIf->HubNotify,
                  TimerPeriodic,
                  USB_ROOTHUB_POLL_INTERVAL
                  );

  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (HubIf->HubNotify);
  }

  return Status;
}



/**
  Get the port status. This function is required to
  ACK the port change bits although it will return
  the port changes in PortState. Bus enumeration code
  doesn't need to ACK the port change bits.

  @param  HubIf                 The root hub interface
  @param  Port                  The root hub port to get the state
  @param  PortState             Variable to return the port state

  @retval EFI_SUCCESS           The port state is returned
  @retval Others                Failed to retrieve the port state

**/
STATIC
EFI_STATUS
UsbRootHubGetPortStatus (
  IN  USB_INTERFACE       *HubIf,
  IN  UINT8               Port,
  OUT EFI_USB_PORT_STATUS *PortState
  )
{
  USB_BUS                 *Bus;
  EFI_STATUS              Status;

  Bus     = HubIf->Device->Bus;
  Status  = UsbHcGetRootHubPortStatus (Bus, Port, PortState);

  return Status;
}


/**
  Clear the port change status.

  @param  HubIf                 The root hub interface
  @param  Port                  The root hub port

  @retval EFI_SUCCESS           The port state is returned
  @retval Others                Failed to retrieve the port state

**/
STATIC
VOID
UsbRootHubClearPortChange (
  IN USB_INTERFACE        *HubIf,
  IN UINT8                Port
  )
{
  EFI_USB_PORT_STATUS     PortState;
  USB_CHANGE_FEATURE_MAP  *Map;
  UINTN                   Index;
  EFI_STATUS              Status;

  Status = UsbRootHubGetPortStatus (HubIf, Port, &PortState);

  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // OK, get the usb port status, now ACK the change bits.
  // Don't return error when failed to clear the change bits.
  // It may lead to extra port state report. USB bus should
  // be able to handle this.
  //
  for (Index = 0; Index < USB_ROOT_HUB_MAP_SIZE; Index++) {
    Map = &mRootHubFeatureMap[Index];

    if (USB_BIT_IS_SET (PortState.PortChangeStatus, Map->ChangedBit)) {
      UsbHcClearRootHubPortFeature (HubIf->Device->Bus, Port, (EFI_USB_PORT_FEATURE) Map->Feature);
    }
  }
}



/**
  Set the root hub port feature

  @param  HubIf                 The Usb hub interface
  @param  Port                  The hub port
  @param  Feature               The feature to set

  @retval EFI_SUCCESS           The root hub port is set with the feature
  @retval Others                Failed to set the feature

**/
STATIC
EFI_STATUS
UsbRootHubSetPortFeature (
  IN USB_INTERFACE        *HubIf,
  IN UINT8                Port,
  IN EFI_USB_PORT_FEATURE Feature
  )
{
  EFI_STATUS              Status;

  Status  = UsbHcSetRootHubPortFeature (HubIf->Device->Bus, Port, Feature);

  return Status;
}


/**
  Clear the root hub port feature

  @param  HubIf                 The root hub interface
  @param  Port                  The root hub port
  @param  Feature               The feature to clear

  @retval EFI_SUCCESS           The root hub port is cleared of the feature
  @retval Others                Failed to clear the feature

**/
STATIC
EFI_STATUS
UsbRootHubClearPortFeature (
  IN USB_INTERFACE        *HubIf,
  IN UINT8                Port,
  IN EFI_USB_PORT_FEATURE Feature
  )
{
  EFI_STATUS              Status;

  Status  = UsbHcClearRootHubPortFeature (HubIf->Device->Bus, Port, Feature);

  return Status;
}


/**
  Interface funtion to reset the root hub port

  @param  RootIf                The root hub interface
  @param  Port                  The port to reset

  @retval EFI_SUCCESS           The hub port is reset
  @retval EFI_TIMEOUT           Failed to reset the port in time
  @retval EFI_NOT_FOUND         The low/full speed device connected to high  speed
                                root hub is released to the companion UHCI
  @retval Others                Failed to reset the port

**/
STATIC
EFI_STATUS
UsbRootHubResetPort (
  IN USB_INTERFACE        *RootIf,
  IN UINT8                Port
  )
{
  USB_BUS                 *Bus;
  EFI_STATUS              Status;
  EFI_USB_PORT_STATUS     PortState;
  UINTN                   Index;

  //
  // Notice: although EHCI requires that ENABLED bit be cleared
  // when reset the port, we don't need to care that here. It
  // should be handled in the EHCI driver.
  //
  Bus     = RootIf->Device->Bus;
  Status  = UsbHcSetRootHubPortFeature (Bus, Port, EfiUsbPortReset);

  if (EFI_ERROR (Status)) {
    DEBUG (( EFI_D_ERROR, "UsbRootHubResetPort: failed to start reset on port %d\n", Port));
    return Status;
  }

  //
  // Drive the reset signal for at least 50ms. Check USB 2.0 Spec
  // section 7.1.7.5 for timing requirements.
  //
  gBS->Stall (USB_SET_ROOT_PORT_RESET_STALL);

  Status = UsbHcClearRootHubPortFeature (Bus, Port, EfiUsbPortReset);

  if (EFI_ERROR (Status)) {
    DEBUG (( EFI_D_ERROR, "UsbRootHubResetPort: failed to clear reset on port %d\n", Port));
    return Status;
  }

  gBS->Stall (USB_CLR_ROOT_PORT_RESET_STALL);

  //
  // USB host controller won't clear the RESET bit until
  // reset is actually finished.
  //
  ZeroMem (&PortState, sizeof (EFI_USB_PORT_STATUS));

  for (Index = 0; Index < USB_WAIT_PORT_STS_CHANGE_LOOP; Index++) {
    Status = UsbHcGetRootHubPortStatus (Bus, Port, &PortState);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (!USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_RESET)) {
      break;
    }

    gBS->Stall (USB_WAIT_PORT_STS_CHANGE_STALL);
  }

  if (Index == USB_WAIT_PORT_STS_CHANGE_LOOP) {
    DEBUG ((EFI_D_ERROR, "UsbRootHubResetPort: reset not finished in time on port %d\n", Port));
    return EFI_TIMEOUT;
  }

  if (!USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_ENABLE)) {
    //
    // OK, the port is reset. If root hub is of high speed and
    // the device is of low/full speed, release the ownership to
    // companion UHCI. If root hub is of full speed, it won't
    // automatically enable the port, we need to enable it manually.
    //
    if (RootIf->MaxSpeed == EFI_USB_SPEED_HIGH) {
      DEBUG (( EFI_D_ERROR, "UsbRootHubResetPort: release low/full speed device (%d) to UHCI\n", Port));

      UsbRootHubSetPortFeature (RootIf, Port, EfiUsbPortOwner);
      return EFI_NOT_FOUND;

    } else {

      Status = UsbRootHubSetPortFeature (RootIf, Port, EfiUsbPortEnable);

      if (EFI_ERROR (Status)) {
        DEBUG (( EFI_D_ERROR, "UsbRootHubResetPort: failed to enable port %d for UHCI\n", Port));
        return Status;
      }

      gBS->Stall (USB_SET_ROOT_PORT_ENABLE_STALL);
    }
  }

  return EFI_SUCCESS;
}


/**
  Release the root hub's control of the interface

  @param  HubIf                 The root hub interface

  @retval EFI_SUCCESS           The root hub's control of the interface is
                                released.

**/
STATIC
EFI_STATUS
UsbRootHubRelease (
  IN USB_INTERFACE        *HubIf
  )
{
  DEBUG (( EFI_D_INFO, "UsbRootHubRelease: root hub released for hub %x\n", HubIf));

  gBS->SetTimer (HubIf->HubNotify, TimerCancel, USB_ROOTHUB_POLL_INTERVAL);
  gBS->CloseEvent (HubIf->HubNotify);

  return EFI_SUCCESS;
}

USB_HUB_API mUsbHubApi = {
  UsbHubInit,
  UsbHubGetPortStatus,
  UsbHubClearPortChange,
  UsbHubSetPortFeature,
  UsbHubClearPortFeature,
  UsbHubResetPort,
  UsbHubRelease
};

USB_HUB_API mUsbRootHubApi = {
  UsbRootHubInit,
  UsbRootHubGetPortStatus,
  UsbRootHubClearPortChange,
  UsbRootHubSetPortFeature,
  UsbRootHubClearPortFeature,
  UsbRootHubResetPort,
  UsbRootHubRelease
};
