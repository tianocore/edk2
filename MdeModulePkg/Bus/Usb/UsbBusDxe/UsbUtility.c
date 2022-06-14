/** @file

    Wrapper function for usb host controller interface.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "UsbBus.h"

//
// if RemainingDevicePath== NULL, then all Usb child devices in this bus are wanted.
// Use a shor form Usb class Device Path, which could match any usb device, in WantedUsbIoDPList to indicate all Usb devices
// are wanted Usb devices
//
USB_CLASS_FORMAT_DEVICE_PATH mAllUsbClassDevicePath = {
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_USB_CLASS_DP,
      {
        (UINT8) (sizeof (USB_CLASS_DEVICE_PATH)),
        (UINT8) ((sizeof (USB_CLASS_DEVICE_PATH)) >> 8)
      }
    },
    0xffff, // VendorId
    0xffff, // ProductId
    0xff,   // DeviceClass
    0xff,   // DeviceSubClass
    0xff    // DeviceProtocol
  },

  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};


/**
  Get the capability of the host controller.

  @param  UsbBus           The usb driver.
  @param  MaxSpeed         The maximum speed this host controller supports.
  @param  NumOfPort        The number of the root hub port.
  @param  Is64BitCapable   Whether this controller support 64 bit addressing.

  @retval EFI_SUCCESS      The host controller capability is returned.
  @retval Others           Failed to retrieve the host controller capability.

**/
EFI_STATUS
UsbHcGetCapability (
  IN  USB_BUS             *UsbBus,
  OUT UINT8               *MaxSpeed,
  OUT UINT8               *NumOfPort,
  OUT UINT8               *Is64BitCapable
  )
{
  EFI_STATUS              Status;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->GetCapability (
                              UsbBus->Usb2Hc,
                              MaxSpeed,
                              NumOfPort,
                              Is64BitCapable
                              );

  } else {
    Status = UsbBus->UsbHc->GetRootHubPortNumber (UsbBus->UsbHc, NumOfPort);

    *MaxSpeed       = EFI_USB_SPEED_FULL;
    *Is64BitCapable = (UINT8) FALSE;
  }

  return Status;
}










/**
  Get the root hub port state.

  @param  UsbBus           The USB bus driver.
  @param  PortIndex        The index of port.
  @param  PortStatus       The variable to save port state.

  @retval EFI_SUCCESS      The root port state is returned in.
  @retval Others           Failed to get the root hub port state.

**/
EFI_STATUS
UsbHcGetRootHubPortStatus (
  IN  USB_BUS             *UsbBus,
  IN  UINT8               PortIndex,
  OUT EFI_USB_PORT_STATUS *PortStatus
  )
{
  EFI_STATUS              Status;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->GetRootHubPortStatus (UsbBus->Usb2Hc, PortIndex, PortStatus);
  } else {
    Status = UsbBus->UsbHc->GetRootHubPortStatus (UsbBus->UsbHc, PortIndex, PortStatus);
  }

  return Status;
}


/**
  Set the root hub port feature.

  @param  UsbBus           The USB bus driver.
  @param  PortIndex        The port index.
  @param  Feature          The port feature to set.

  @retval EFI_SUCCESS      The port feature is set.
  @retval Others           Failed to set port feature.

**/
EFI_STATUS
UsbHcSetRootHubPortFeature (
  IN USB_BUS              *UsbBus,
  IN UINT8                PortIndex,
  IN EFI_USB_PORT_FEATURE Feature
  )
{
  EFI_STATUS              Status;


  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->SetRootHubPortFeature (UsbBus->Usb2Hc, PortIndex, Feature);
  } else {
    Status = UsbBus->UsbHc->SetRootHubPortFeature (UsbBus->UsbHc, PortIndex, Feature);
  }

  return Status;
}


/**
  Clear the root hub port feature.

  @param  UsbBus           The USB bus driver.
  @param  PortIndex        The port index.
  @param  Feature          The port feature to clear.

  @retval EFI_SUCCESS      The port feature is clear.
  @retval Others           Failed to clear port feature.

**/
EFI_STATUS
UsbHcClearRootHubPortFeature (
  IN USB_BUS              *UsbBus,
  IN UINT8                PortIndex,
  IN EFI_USB_PORT_FEATURE Feature
  )
{
  EFI_STATUS              Status;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->ClearRootHubPortFeature (UsbBus->Usb2Hc, PortIndex, Feature);
  } else {
    Status = UsbBus->UsbHc->ClearRootHubPortFeature (UsbBus->UsbHc, PortIndex, Feature);
  }

  return Status;
}


/**
  Execute a control transfer to the device.

  @param  UsbBus           The USB bus driver.
  @param  DevAddr          The device address.
  @param  DevSpeed         The device speed.
  @param  MaxPacket        Maximum packet size of endpoint 0.
  @param  Request          The control transfer request.
  @param  Direction        The direction of data stage.
  @param  Data             The buffer holding data.
  @param  DataLength       The length of the data.
  @param  TimeOut          Timeout (in ms) to wait until timeout.
  @param  Translator       The transaction translator for low/full speed device.
  @param  UsbResult        The result of transfer.

  @retval EFI_SUCCESS      The control transfer finished without error.
  @retval Others           The control transfer failed, reason returned in UsbReslt.

**/
EFI_STATUS
UsbHcControlTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN  EFI_USB_DEVICE_REQUEST              *Request,
  IN  EFI_USB_DATA_DIRECTION              Direction,
  IN  OUT VOID                            *Data,
  IN  OUT UINTN                           *DataLength,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *UsbResult
  )
{
  EFI_STATUS              Status;
  BOOLEAN                 IsSlowDevice;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->ControlTransfer (
                               UsbBus->Usb2Hc,
                               DevAddr,
                               DevSpeed,
                               MaxPacket,
                               Request,
                               Direction,
                               Data,
                               DataLength,
                               TimeOut,
                               Translator,
                               UsbResult
                               );

  } else {
    IsSlowDevice = (BOOLEAN)(EFI_USB_SPEED_LOW == DevSpeed);
    Status = UsbBus->UsbHc->ControlTransfer (
                              UsbBus->UsbHc,
                              DevAddr,
                              IsSlowDevice,
                              (UINT8) MaxPacket,
                              Request,
                              Direction,
                              Data,
                              DataLength,
                              TimeOut,
                              UsbResult
                              );
  }

  return Status;
}


/**
  Execute a bulk transfer to the device's endpoint.

  @param  UsbBus           The USB bus driver.
  @param  DevAddr          The target device address.
  @param  EpAddr           The target endpoint address, with direction encoded in
                           bit 7.
  @param  DevSpeed         The device's speed.
  @param  MaxPacket        The endpoint's max packet size.
  @param  BufferNum        The number of data buffer.
  @param  Data             Array of pointers to data buffer.
  @param  DataLength       The length of data buffer.
  @param  DataToggle       On input, the initial data toggle to use, also  return
                           the next toggle on output.
  @param  TimeOut          The time to wait until timeout.
  @param  Translator       The transaction translator for low/full speed device.
  @param  UsbResult        The result of USB execution.

  @retval EFI_SUCCESS      The bulk transfer is finished without error.
  @retval Others           Failed to execute bulk transfer, result in UsbResult.

**/
EFI_STATUS
UsbHcBulkTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               EpAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN  UINT8                               BufferNum,
  IN  OUT VOID                            *Data[],
  IN  OUT UINTN                           *DataLength,
  IN  OUT UINT8                           *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *UsbResult
  )
{
  EFI_STATUS              Status;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->BulkTransfer (
                               UsbBus->Usb2Hc,
                               DevAddr,
                               EpAddr,
                               DevSpeed,
                               MaxPacket,
                               BufferNum,
                               Data,
                               DataLength,
                               DataToggle,
                               TimeOut,
                               Translator,
                               UsbResult
                               );
  } else {
    Status = UsbBus->UsbHc->BulkTransfer (
                              UsbBus->UsbHc,
                              DevAddr,
                              EpAddr,
                              (UINT8) MaxPacket,
                              *Data,
                              DataLength,
                              DataToggle,
                              TimeOut,
                              UsbResult
                              );
  }

  return Status;
}


/**
  Queue or cancel an asynchronous interrupt transfer.

  @param  UsbBus           The USB bus driver.
  @param  DevAddr          The target device address.
  @param  EpAddr           The target endpoint address, with direction encoded in
                           bit 7.
  @param  DevSpeed         The device's speed.
  @param  MaxPacket        The endpoint's max packet size.
  @param  IsNewTransfer    Whether this is a new request. If not, cancel the old
                           request.
  @param  DataToggle       Data toggle to use on input, next toggle on output.
  @param  PollingInterval  The interval to poll the interrupt transfer (in ms).
  @param  DataLength       The length of periodical data receive.
  @param  Translator       The transaction translator for low/full speed device.
  @param  Callback         Function to call when data is received.
  @param  Context          The context to the callback.

  @retval EFI_SUCCESS      The asynchronous transfer is queued.
  @retval Others           Failed to queue the transfer.

**/
EFI_STATUS
UsbHcAsyncInterruptTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               EpAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN  BOOLEAN                             IsNewTransfer,
  IN OUT UINT8                            *DataToggle,
  IN  UINTN                               PollingInterval,
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     Callback,
  IN  VOID                                *Context OPTIONAL
  )
{
  EFI_STATUS              Status;
  BOOLEAN                 IsSlowDevice;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->AsyncInterruptTransfer (
                               UsbBus->Usb2Hc,
                               DevAddr,
                               EpAddr,
                               DevSpeed,
                               MaxPacket,
                               IsNewTransfer,
                               DataToggle,
                               PollingInterval,
                               DataLength,
                               Translator,
                               Callback,
                               Context
                               );
  } else {
    IsSlowDevice = (BOOLEAN)(EFI_USB_SPEED_LOW == DevSpeed);

    Status = UsbBus->UsbHc->AsyncInterruptTransfer (
                              UsbBus->UsbHc,
                              DevAddr,
                              EpAddr,
                              IsSlowDevice,
                              (UINT8) MaxPacket,
                              IsNewTransfer,
                              DataToggle,
                              PollingInterval,
                              DataLength,
                              Callback,
                              Context
                              );
  }

  return Status;
}


/**
  Execute a synchronous interrupt transfer to the target endpoint.

  @param  UsbBus           The USB bus driver.
  @param  DevAddr          The target device address.
  @param  EpAddr           The target endpoint address, with direction encoded in
                           bit 7.
  @param  DevSpeed         The device's speed.
  @param  MaxPacket        The endpoint's max packet size.
  @param  Data             Pointer to data buffer.
  @param  DataLength       The length of data buffer.
  @param  DataToggle       On input, the initial data toggle to use, also  return
                           the next toggle on output.
  @param  TimeOut          The time to wait until timeout.
  @param  Translator       The transaction translator for low/full speed device.
  @param  UsbResult        The result of USB execution.

  @retval EFI_SUCCESS      The synchronous interrupt transfer is OK.
  @retval Others           Failed to execute the synchronous interrupt transfer.

**/
EFI_STATUS
UsbHcSyncInterruptTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               EpAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN OUT VOID                             *Data,
  IN OUT UINTN                            *DataLength,
  IN OUT UINT8                            *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *UsbResult
  )
{
  EFI_STATUS              Status;
  BOOLEAN                 IsSlowDevice;

  if (UsbBus->Usb2Hc != NULL) {
    Status = UsbBus->Usb2Hc->SyncInterruptTransfer (
                               UsbBus->Usb2Hc,
                               DevAddr,
                               EpAddr,
                               DevSpeed,
                               MaxPacket,
                               Data,
                               DataLength,
                               DataToggle,
                               TimeOut,
                               Translator,
                               UsbResult
                               );
  } else {
    IsSlowDevice = (BOOLEAN) ((EFI_USB_SPEED_LOW == DevSpeed) ? TRUE : FALSE);
    Status = UsbBus->UsbHc->SyncInterruptTransfer (
                              UsbBus->UsbHc,
                              DevAddr,
                              EpAddr,
                              IsSlowDevice,
                              (UINT8) MaxPacket,
                              Data,
                              DataLength,
                              DataToggle,
                              TimeOut,
                              UsbResult
                              );
  }

  return Status;
}








/**
  Open the USB host controller protocol BY_CHILD.

  @param  Bus              The USB bus driver.
  @param  Child            The child handle.

  @return The open protocol return.

**/
EFI_STATUS
UsbOpenHostProtoByChild (
  IN USB_BUS              *Bus,
  IN EFI_HANDLE           Child
  )
{
  EFI_USB_HC_PROTOCOL     *UsbHc;
  EFI_USB2_HC_PROTOCOL    *Usb2Hc;
  EFI_STATUS              Status;

  if (Bus->Usb2Hc != NULL) {
    Status = gBS->OpenProtocol (
                    Bus->HostHandle,
                    &gEfiUsb2HcProtocolGuid,
                    (VOID **) &Usb2Hc,
                    mUsbBusDriverBinding.DriverBindingHandle,
                    Child,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );

  } else {
    Status = gBS->OpenProtocol (
                    Bus->HostHandle,
                    &gEfiUsbHcProtocolGuid,
                    (VOID **) &UsbHc,
                    mUsbBusDriverBinding.DriverBindingHandle,
                    Child,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
  }

  return Status;
}


/**
  Close the USB host controller protocol BY_CHILD.

  @param  Bus              The USB bus driver.
  @param  Child            The child handle.

**/
VOID
UsbCloseHostProtoByChild (
  IN USB_BUS              *Bus,
  IN EFI_HANDLE           Child
  )
{
  if (Bus->Usb2Hc != NULL) {
    gBS->CloseProtocol (
           Bus->HostHandle,
           &gEfiUsb2HcProtocolGuid,
           mUsbBusDriverBinding.DriverBindingHandle,
           Child
           );

  } else {
    gBS->CloseProtocol (
           Bus->HostHandle,
           &gEfiUsbHcProtocolGuid,
           mUsbBusDriverBinding.DriverBindingHandle,
           Child
           );
  }
}


/**
  return the current TPL, copied from the EDKII glue lib.

  @param  VOID.

  @return Current TPL.

**/
EFI_TPL
UsbGetCurrentTpl (
  VOID
  )
{
  EFI_TPL                 Tpl;

  Tpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  gBS->RestoreTPL (Tpl);

  return Tpl;
}

/**
  Create a new device path which only contain the first Usb part of the DevicePath.

  @param DevicePath  A full device path which contain the usb nodes.

  @return            A new device path which only contain the Usb part of the DevicePath.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
GetUsbDPFromFullDP (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL    *UsbDevicePathPtr;
  EFI_DEVICE_PATH_PROTOCOL    *UsbDevicePathBeginPtr;
  EFI_DEVICE_PATH_PROTOCOL    *UsbDevicePathEndPtr;
  UINTN                       Size;

  //
  // Get the Usb part first Begin node in full device path
  //
  UsbDevicePathBeginPtr = DevicePath;
  while ( (!IsDevicePathEnd (UsbDevicePathBeginPtr))&&
         ((UsbDevicePathBeginPtr->Type != MESSAGING_DEVICE_PATH) ||
         (UsbDevicePathBeginPtr->SubType != MSG_USB_DP &&
          UsbDevicePathBeginPtr->SubType != MSG_USB_CLASS_DP
          && UsbDevicePathBeginPtr->SubType != MSG_USB_WWID_DP
          ))) {

    UsbDevicePathBeginPtr = NextDevicePathNode(UsbDevicePathBeginPtr);
  }

  //
  // Get the Usb part first End node in full device path
  //
  UsbDevicePathEndPtr = UsbDevicePathBeginPtr;
  while ((!IsDevicePathEnd (UsbDevicePathEndPtr))&&
         (UsbDevicePathEndPtr->Type == MESSAGING_DEVICE_PATH) &&
         (UsbDevicePathEndPtr->SubType == MSG_USB_DP ||
          UsbDevicePathEndPtr->SubType == MSG_USB_CLASS_DP
          || UsbDevicePathEndPtr->SubType == MSG_USB_WWID_DP
          )) {

    UsbDevicePathEndPtr = NextDevicePathNode(UsbDevicePathEndPtr);
  }

  Size  = GetDevicePathSize (UsbDevicePathBeginPtr);
  Size -= GetDevicePathSize (UsbDevicePathEndPtr);
  if (Size ==0){
    //
    // The passed in DevicePath does not contain the usb nodes
    //
    return NULL;
  }

  //
  // Create a new device path which only contain the above Usb part
  //
  UsbDevicePathPtr = AllocateZeroPool (Size + sizeof (EFI_DEVICE_PATH_PROTOCOL));
  ASSERT (UsbDevicePathPtr != NULL);
  CopyMem (UsbDevicePathPtr, UsbDevicePathBeginPtr, Size);
  //
  // Append end device path node
  //
  UsbDevicePathEndPtr = (EFI_DEVICE_PATH_PROTOCOL *) ((UINTN) UsbDevicePathPtr + Size);
  SetDevicePathEndNode (UsbDevicePathEndPtr);
  return UsbDevicePathPtr;
}

/**
  Check whether a usb device path is in a DEVICE_PATH_LIST_ITEM list.

  @param UsbDP       a usb device path of DEVICE_PATH_LIST_ITEM.
  @param UsbIoDPList a DEVICE_PATH_LIST_ITEM list.

  @retval TRUE       there is a DEVICE_PATH_LIST_ITEM in UsbIoDPList which contains the passed in UsbDP.
  @retval FALSE      there is no DEVICE_PATH_LIST_ITEM in UsbIoDPList which contains the passed in UsbDP.

**/
BOOLEAN
EFIAPI
SearchUsbDPInList (
  IN EFI_DEVICE_PATH_PROTOCOL     *UsbDP,
  IN LIST_ENTRY                   *UsbIoDPList
  )
{
  LIST_ENTRY                  *ListIndex;
  DEVICE_PATH_LIST_ITEM       *ListItem;
  BOOLEAN                     Found;
  UINTN                       UsbDpDevicePathSize;

  //
  // Check that UsbDP and UsbIoDPList are valid
  //
  if ((UsbIoDPList == NULL) || (UsbDP == NULL)) {
    return FALSE;
  }

  Found = FALSE;
  ListIndex = UsbIoDPList->ForwardLink;
  while (ListIndex != UsbIoDPList){
    ListItem = CR(ListIndex, DEVICE_PATH_LIST_ITEM, Link, DEVICE_PATH_LIST_ITEM_SIGNATURE);
    //
    // Compare DEVICE_PATH_LIST_ITEM.DevicePath[]
    //
    ASSERT (ListItem->DevicePath != NULL);

    UsbDpDevicePathSize  = GetDevicePathSize (UsbDP);
    if (UsbDpDevicePathSize == GetDevicePathSize (ListItem->DevicePath)) {
      if ((CompareMem (UsbDP, ListItem->DevicePath, UsbDpDevicePathSize)) == 0) {
        Found = TRUE;
        break;
      }
    }
    ListIndex =  ListIndex->ForwardLink;
  }

  return Found;
}

/**
  Add a usb device path into the DEVICE_PATH_LIST_ITEM list.

  @param UsbDP                   a usb device path of DEVICE_PATH_LIST_ITEM.
  @param UsbIoDPList             a DEVICE_PATH_LIST_ITEM list.

  @retval EFI_INVALID_PARAMETER  If parameters are invalid, return this value.
  @retval EFI_SUCCESS            If Add operation is successful, return this value.

**/
EFI_STATUS
EFIAPI
AddUsbDPToList (
  IN EFI_DEVICE_PATH_PROTOCOL     *UsbDP,
  IN LIST_ENTRY                   *UsbIoDPList
  )
{
  DEVICE_PATH_LIST_ITEM       *ListItem;

  //
  // Check that UsbDP and UsbIoDPList are valid
  //
  if ((UsbIoDPList == NULL) || (UsbDP == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (SearchUsbDPInList (UsbDP, UsbIoDPList)){
    return EFI_SUCCESS;
  }

  //
  // Prepare the usbio device path DEVICE_PATH_LIST_ITEM structure.
  //
  ListItem = AllocateZeroPool (sizeof (DEVICE_PATH_LIST_ITEM));
  ASSERT (ListItem != NULL);
  ListItem->Signature = DEVICE_PATH_LIST_ITEM_SIGNATURE;
  ListItem->DevicePath = DuplicateDevicePath (UsbDP);

  InsertTailList (UsbIoDPList, &ListItem->Link);

  return EFI_SUCCESS;
}

/**
  Check whether usb device, whose interface is UsbIf, matches the usb class which indicated by
  UsbClassDevicePathPtr whose is a short form usb class device path.

  @param UsbClassDevicePathPtr    a short form usb class device path.
  @param UsbIf                    a usb device interface.

  @retval TRUE                    the usb device match the usb class.
  @retval FALSE                   the usb device does not match the usb class.

**/
BOOLEAN
EFIAPI
MatchUsbClass (
  IN USB_CLASS_DEVICE_PATH      *UsbClassDevicePathPtr,
  IN USB_INTERFACE              *UsbIf
  )
{
  USB_INTERFACE_DESC            *IfDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  *ActIfDesc;
  EFI_USB_DEVICE_DESCRIPTOR     *DevDesc;


  if ((UsbClassDevicePathPtr->Header.Type != MESSAGING_DEVICE_PATH) ||
      (UsbClassDevicePathPtr->Header.SubType != MSG_USB_CLASS_DP)){
    ASSERT (0);
    return FALSE;
  }

  IfDesc       = UsbIf->IfDesc;
  ASSERT (IfDesc->ActiveIndex < USB_MAX_INTERFACE_SETTING);
  ActIfDesc    = &(IfDesc->Settings[IfDesc->ActiveIndex]->Desc);
  DevDesc      = &(UsbIf->Device->DevDesc->Desc);

  //
  // If connect class policy, determine whether to create device handle by the five fields
  // in class device path node.
  //
  // In addition, hub interface is always matched for this policy.
  //
  if ((ActIfDesc->InterfaceClass == USB_HUB_CLASS_CODE) &&
      (ActIfDesc->InterfaceSubClass == USB_HUB_SUBCLASS_CODE)) {
    return TRUE;
  }

  //
  // If vendor id or product id is 0xffff, they will be ignored.
  //
  if ((UsbClassDevicePathPtr->VendorId == 0xffff || UsbClassDevicePathPtr->VendorId == DevDesc->IdVendor) &&
      (UsbClassDevicePathPtr->ProductId == 0xffff || UsbClassDevicePathPtr->ProductId == DevDesc->IdProduct)) {

    //
    // If Class in Device Descriptor is set to 0, the counterparts in interface should be checked.
    //
    if (DevDesc->DeviceClass == 0) {
      if ((UsbClassDevicePathPtr->DeviceClass == ActIfDesc->InterfaceClass ||
                                          UsbClassDevicePathPtr->DeviceClass == 0xff) &&
          (UsbClassDevicePathPtr->DeviceSubClass == ActIfDesc->InterfaceSubClass ||
                                       UsbClassDevicePathPtr->DeviceSubClass == 0xff) &&
          (UsbClassDevicePathPtr->DeviceProtocol == ActIfDesc->InterfaceProtocol ||
                                       UsbClassDevicePathPtr->DeviceProtocol == 0xff)) {
        return TRUE;
      }

    } else if ((UsbClassDevicePathPtr->DeviceClass == DevDesc->DeviceClass ||
                                         UsbClassDevicePathPtr->DeviceClass == 0xff) &&
               (UsbClassDevicePathPtr->DeviceSubClass == DevDesc->DeviceSubClass ||
                                      UsbClassDevicePathPtr->DeviceSubClass == 0xff) &&
               (UsbClassDevicePathPtr->DeviceProtocol == DevDesc->DeviceProtocol ||
                                      UsbClassDevicePathPtr->DeviceProtocol == 0xff)) {

      return TRUE;
    }
  }

  return FALSE;
}

/**
  Check whether usb device, whose interface is UsbIf, matches the usb WWID requirement which indicated by
  UsbWWIDDevicePathPtr whose is a short form usb WWID device path.

  @param UsbWWIDDevicePathPtr    a short form usb WWID device path.
  @param UsbIf                   a usb device interface.

  @retval TRUE                   the usb device match the usb WWID requirement.
  @retval FALSE                  the usb device does not match the usb WWID requirement.

**/
BOOLEAN
MatchUsbWwid (
  IN USB_WWID_DEVICE_PATH       *UsbWWIDDevicePathPtr,
  IN USB_INTERFACE              *UsbIf
  )
{
  USB_INTERFACE_DESC            *IfDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  *ActIfDesc;
  EFI_USB_DEVICE_DESCRIPTOR     *DevDesc;
  EFI_USB_STRING_DESCRIPTOR     *StrDesc;
  UINT16                        Index;
  CHAR16                        *CompareStr;
  UINTN                         CompareLen;
  UINTN                         Length;

  if ((UsbWWIDDevicePathPtr->Header.Type != MESSAGING_DEVICE_PATH) ||
     (UsbWWIDDevicePathPtr->Header.SubType != MSG_USB_WWID_DP )){
    ASSERT (0);
    return FALSE;
  }

  IfDesc       = UsbIf->IfDesc;
  ASSERT (IfDesc->ActiveIndex < USB_MAX_INTERFACE_SETTING);
  ActIfDesc    = &(IfDesc->Settings[IfDesc->ActiveIndex]->Desc);
  DevDesc      = &(UsbIf->Device->DevDesc->Desc);

  //
  // In addition, Hub interface is always matched for this policy.
  //
  if ((ActIfDesc->InterfaceClass == USB_HUB_CLASS_CODE) &&
      (ActIfDesc->InterfaceSubClass == USB_HUB_SUBCLASS_CODE)) {
    return TRUE;
  }

  //
  // Check Vendor Id, Product Id and Interface Number.
  //
  if ((DevDesc->IdVendor != UsbWWIDDevicePathPtr->VendorId) ||
      (DevDesc->IdProduct != UsbWWIDDevicePathPtr->ProductId) ||
      (ActIfDesc->InterfaceNumber != UsbWWIDDevicePathPtr->InterfaceNumber)) {
    return FALSE;
  }

  //
  // Check SerialNumber.
  //
  if (DevDesc->StrSerialNumber == 0) {
    return FALSE;
  }

  //
  // Serial number in USB WWID device path is the last 64-or-less UTF-16 characters.
  //
  CompareStr = (CHAR16 *) (UINTN) (UsbWWIDDevicePathPtr + 1);
  CompareLen = (DevicePathNodeLength (UsbWWIDDevicePathPtr) - sizeof (USB_WWID_DEVICE_PATH)) / sizeof (CHAR16);
  if (CompareStr[CompareLen - 1] == L'\0') {
    CompareLen--;
  }

  //
  // Compare serial number in each supported language.
  //
  for (Index = 0; Index < UsbIf->Device->TotalLangId; Index++) {
    StrDesc = UsbGetOneString (UsbIf->Device, DevDesc->StrSerialNumber, UsbIf->Device->LangId[Index]);
    if (StrDesc == NULL) {
      continue;
    }

    Length = (StrDesc->Length - 2) / sizeof (CHAR16);
    if ((Length >= CompareLen) &&
        (CompareMem (StrDesc->String + Length - CompareLen, CompareStr, CompareLen * sizeof (CHAR16)) == 0)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Free a DEVICE_PATH_LIST_ITEM list.

  @param  UsbIoDPList            a DEVICE_PATH_LIST_ITEM list pointer.

  @retval EFI_INVALID_PARAMETER  If parameters are invalid, return this value.
  @retval EFI_SUCCESS            If free operation is successful, return this value.

**/
EFI_STATUS
EFIAPI
UsbBusFreeUsbDPList (
  IN LIST_ENTRY          *UsbIoDPList
  )
{
  LIST_ENTRY                  *ListIndex;
  DEVICE_PATH_LIST_ITEM       *ListItem;

  //
  // Check that ControllerHandle is a valid handle
  //
  if (UsbIoDPList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ListIndex = UsbIoDPList->ForwardLink;
  while (ListIndex != UsbIoDPList){
    ListItem = CR(ListIndex, DEVICE_PATH_LIST_ITEM, Link, DEVICE_PATH_LIST_ITEM_SIGNATURE);
    //
    // Free DEVICE_PATH_LIST_ITEM.DevicePath[]
    //
    if (ListItem->DevicePath != NULL){
      FreePool(ListItem->DevicePath);
    }
    //
    // Free DEVICE_PATH_LIST_ITEM itself
    //
    ListIndex =  ListIndex->ForwardLink;
    RemoveEntryList (&ListItem->Link);
    FreePool (ListItem);
  }

  InitializeListHead (UsbIoDPList);
  return EFI_SUCCESS;
}

/**
  Store a wanted usb child device info (its Usb part of device path) which is indicated by
  RemainingDevicePath in a Usb bus which  is indicated by UsbBusId.

  @param  UsbBusId               Point to EFI_USB_BUS_PROTOCOL interface.
  @param  RemainingDevicePath    The remaining device patch.

  @retval EFI_SUCCESS            Add operation is successful.
  @retval EFI_INVALID_PARAMETER  The parameters are invalid.

**/
EFI_STATUS
EFIAPI
UsbBusAddWantedUsbIoDP (
  IN EFI_USB_BUS_PROTOCOL         *UsbBusId,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  USB_BUS                       *Bus;
  EFI_STATUS                    Status;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePathPtr;

  //
  // Check whether remaining device path is valid
  //
  if (RemainingDevicePath != NULL && !IsDevicePathEnd (RemainingDevicePath)) {
    if ((RemainingDevicePath->Type    != MESSAGING_DEVICE_PATH) ||
        (RemainingDevicePath->SubType != MSG_USB_DP &&
         RemainingDevicePath->SubType != MSG_USB_CLASS_DP
         && RemainingDevicePath->SubType != MSG_USB_WWID_DP
         )) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (UsbBusId == NULL){
    return EFI_INVALID_PARAMETER;
  }

  Bus = USB_BUS_FROM_THIS (UsbBusId);

  if (RemainingDevicePath == NULL) {
    //
    // RemainingDevicePath == NULL means all Usb devices in this bus are wanted.
    // Here use a Usb class Device Path in WantedUsbIoDPList to indicate all Usb devices
    // are wanted Usb devices
    //
    Status = UsbBusFreeUsbDPList (&Bus->WantedUsbIoDPList);
    ASSERT (!EFI_ERROR (Status));
    DevicePathPtr = DuplicateDevicePath ((EFI_DEVICE_PATH_PROTOCOL *) &mAllUsbClassDevicePath);
  } else if (!IsDevicePathEnd (RemainingDevicePath)) {
    //
    // If RemainingDevicePath isn't the End of Device Path Node,
    // Create new Usb device path according to the usb part in remaining device path
    //
    DevicePathPtr = GetUsbDPFromFullDP (RemainingDevicePath);
  } else {
    //
    // If RemainingDevicePath is the End of Device Path Node,
    // skip enumerate any device and return EFI_SUCCESS
    //
    return EFI_SUCCESS;
  }

  ASSERT (DevicePathPtr != NULL);
  Status = AddUsbDPToList (DevicePathPtr, &Bus->WantedUsbIoDPList);
  ASSERT (!EFI_ERROR (Status));
  FreePool (DevicePathPtr);
  return EFI_SUCCESS;
}

/**
  Check whether a usb child device is the wanted device in a bus.

  @param  Bus     The Usb bus's private data pointer.
  @param  UsbIf   The usb child device interface.

  @retval True    If a usb child device is the wanted device in a bus.
  @retval False   If a usb child device is *NOT* the wanted device in a bus.

**/
BOOLEAN
EFIAPI
UsbBusIsWantedUsbIO (
  IN USB_BUS                 *Bus,
  IN USB_INTERFACE           *UsbIf
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *DevicePathPtr;
  LIST_ENTRY                    *WantedUsbIoDPListPtr;
  LIST_ENTRY                    *WantedListIndex;
  DEVICE_PATH_LIST_ITEM         *WantedListItem;
  BOOLEAN                       DoConvert;
  UINTN                         FirstDevicePathSize;

  //
  // Check whether passed in parameters are valid
  //
  if ((UsbIf == NULL) || (Bus == NULL)) {
    return FALSE;
  }
  //
  // Check whether UsbIf is Hub
  //
  if (UsbIf->IsHub) {
    return TRUE;
  }

  //
  // Check whether all Usb devices in this bus are wanted
  //
  if (SearchUsbDPInList ((EFI_DEVICE_PATH_PROTOCOL *)&mAllUsbClassDevicePath, &Bus->WantedUsbIoDPList)){
    return TRUE;
  }

  //
  // Check whether the Usb device match any item in WantedUsbIoDPList
  //
  WantedUsbIoDPListPtr = &Bus->WantedUsbIoDPList;
  //
  // Create new Usb device path according to the usb part in UsbIo full device path
  //
  DevicePathPtr = GetUsbDPFromFullDP (UsbIf->DevicePath);
  ASSERT (DevicePathPtr != NULL);

  DoConvert = FALSE;
  WantedListIndex = WantedUsbIoDPListPtr->ForwardLink;
  while (WantedListIndex != WantedUsbIoDPListPtr){
    WantedListItem = CR(WantedListIndex, DEVICE_PATH_LIST_ITEM, Link, DEVICE_PATH_LIST_ITEM_SIGNATURE);
    ASSERT (WantedListItem->DevicePath->Type == MESSAGING_DEVICE_PATH);
    switch (WantedListItem->DevicePath->SubType) {
    case MSG_USB_DP:
      FirstDevicePathSize = GetDevicePathSize (WantedListItem->DevicePath);
      if (FirstDevicePathSize == GetDevicePathSize (DevicePathPtr)) {
        if (CompareMem (
              WantedListItem->DevicePath,
              DevicePathPtr,
              GetDevicePathSize (DevicePathPtr)) == 0
            ) {
          DoConvert = TRUE;
        }
      }
      break;
    case MSG_USB_CLASS_DP:
      if (MatchUsbClass((USB_CLASS_DEVICE_PATH *)WantedListItem->DevicePath, UsbIf)) {
        DoConvert = TRUE;
      }
      break;
   case MSG_USB_WWID_DP:
      if (MatchUsbWwid((USB_WWID_DEVICE_PATH *)WantedListItem->DevicePath, UsbIf)) {
        DoConvert = TRUE;
      }
      break;
    default:
      ASSERT (0);
      break;
    }

    if (DoConvert) {
      break;
    }

    WantedListIndex =  WantedListIndex->ForwardLink;
  }
  gBS->FreePool (DevicePathPtr);

  //
  // Check whether the new Usb device path is wanted
  //
  if (DoConvert){
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Recursively connect every wanted usb child device to ensure they all fully connected.
  Check all the child Usb IO handles in this bus, recursively connecte if it is wanted usb child device.

  @param  UsbBusId                  Point to EFI_USB_BUS_PROTOCOL interface.

  @retval EFI_SUCCESS               Connect is done successfully.
  @retval EFI_INVALID_PARAMETER     The parameter is invalid.

**/
EFI_STATUS
EFIAPI
UsbBusRecursivelyConnectWantedUsbIo (
  IN EFI_USB_BUS_PROTOCOL         *UsbBusId
  )
{
  USB_BUS                       *Bus;
  EFI_STATUS                    Status;
  UINTN                         Index;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  USB_INTERFACE                 *UsbIf;
  UINTN                         UsbIoHandleCount;
  EFI_HANDLE                    *UsbIoBuffer;
  EFI_DEVICE_PATH_PROTOCOL      *UsbIoDevicePath;

  if (UsbBusId == NULL){
    return EFI_INVALID_PARAMETER;
  }

  Bus = USB_BUS_FROM_THIS (UsbBusId);

  //
  // Get all Usb IO handles in system
  //
  UsbIoHandleCount = 0;
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiUsbIoProtocolGuid, NULL, &UsbIoHandleCount, &UsbIoBuffer);
  if (Status == EFI_NOT_FOUND || UsbIoHandleCount == 0) {
    return EFI_SUCCESS;
  }
  ASSERT (!EFI_ERROR (Status));

  for (Index = 0; Index < UsbIoHandleCount; Index++) {
    //
    // Check whether the USB IO handle is a child of this bus
    // Note: The usb child handle maybe invalid because of hot plugged out during the loop
    //
    UsbIoDevicePath = NULL;
    Status = gBS->HandleProtocol (UsbIoBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID *) &UsbIoDevicePath);
    if (EFI_ERROR (Status) || UsbIoDevicePath == NULL) {
      continue;
    }
    if (CompareMem (
            UsbIoDevicePath,
            Bus->DevicePath,
            (GetDevicePathSize (Bus->DevicePath) - sizeof (EFI_DEVICE_PATH_PROTOCOL))
            ) != 0) {
      continue;
    }

    //
    // Get the child Usb IO interface
    //
    Status = gBS->HandleProtocol(
                     UsbIoBuffer[Index],
                     &gEfiUsbIoProtocolGuid,
                     (VOID **) &UsbIo
                     );
    if (EFI_ERROR (Status)) {
      continue;
    }
    UsbIf   = USB_INTERFACE_FROM_USBIO (UsbIo);

    if (UsbBusIsWantedUsbIO (Bus, UsbIf)) {
      if (!UsbIf->IsManaged) {
        //
        // Recursively connect the wanted Usb Io handle
        //
        DEBUG ((EFI_D_INFO, "UsbBusRecursivelyConnectWantedUsbIo: TPL before connect is %d\n", (UINT32)UsbGetCurrentTpl ()));
        Status            = gBS->ConnectController (UsbIf->Handle, NULL, NULL, TRUE);
        UsbIf->IsManaged  = (BOOLEAN)!EFI_ERROR (Status);
        DEBUG ((EFI_D_INFO, "UsbBusRecursivelyConnectWantedUsbIo: TPL after connect is %d\n", (UINT32)UsbGetCurrentTpl()));
      }
    }
  }

  FreePool (UsbIoBuffer);

  return EFI_SUCCESS;
}

