/** @file
  This file contains code for USB Remote Network Driver
  Interface Spec. Driver Binding

  Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UsbRndis.h"

EFI_DRIVER_BINDING_PROTOCOL  gUsbRndisDriverBinding = {
  UsbRndisDriverSupported,
  UsbRndisDriverStart,
  UsbRndisDriverStop,
  USB_RNDIS_DRIVER_VERSION,
  NULL,
  NULL
};

/**
  Check if this interface is USB Rndis SubType

  @param[in]  UsbIo  A pointer to the EFI_USB_IO_PROTOCOL instance.

  @retval TRUE   USB Rndis SubType.
  @retval FALSE  Not USB Rndis SubType.

**/
BOOLEAN
IsSupportedDevice (
  IN EFI_USB_IO_PROTOCOL  *UsbIo
  )
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &InterfaceDescriptor);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  // Check specific device/RNDIS and CDC-DATA
  if (((InterfaceDescriptor.InterfaceClass == USB_CDC_CLASS) &&
       (InterfaceDescriptor.InterfaceSubClass == USB_CDC_ACM_SUBCLASS) &&
       (InterfaceDescriptor.InterfaceProtocol == USB_VENDOR_PROTOCOL)) || \
      ((InterfaceDescriptor.InterfaceClass == USB_MISC_CLASS) &&
       (InterfaceDescriptor.InterfaceSubClass == USB_RNDIS_SUBCLASS) &&
       (InterfaceDescriptor.InterfaceProtocol == USB_RNDIS_ETHERNET_PROTOCOL)) || \
      ((InterfaceDescriptor.InterfaceClass == USB_CDC_DATA_CLASS) &&
       (InterfaceDescriptor.InterfaceSubClass == USB_CDC_DATA_SUBCLASS) &&
       (InterfaceDescriptor.InterfaceProtocol == USB_NO_CLASS_PROTOCOL))
      )
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Check if this interface is USB Rndis SubType but not CDC Data interface

  @param[in]  UsbIo  A pointer to the EFI_USB_IO_PROTOCOL instance.

  @retval TRUE   USB Rndis SubType.
  @retval FALSE  Not USB Rndis SubType.
**/
BOOLEAN
IsRndisInterface (
  IN EFI_USB_IO_PROTOCOL  *UsbIo
  )
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &InterfaceDescriptor);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  // Check for specific device/RNDIS and CDC-DATA
  if (((InterfaceDescriptor.InterfaceClass == USB_CDC_CLASS) &&
       (InterfaceDescriptor.InterfaceSubClass == USB_CDC_ACM_SUBCLASS) &&
       (InterfaceDescriptor.InterfaceProtocol == USB_VENDOR_PROTOCOL)) || \
      ((InterfaceDescriptor.InterfaceClass == USB_MISC_CLASS) &&
       (InterfaceDescriptor.InterfaceSubClass == USB_RNDIS_SUBCLASS) &&
       (InterfaceDescriptor.InterfaceProtocol == USB_RNDIS_ETHERNET_PROTOCOL))
      )
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Check if the USB RNDIS and USB CDC Data interfaces are from the same device.

  @param[in]  UsbRndisDataPath  A pointer to the EFI_DEVICE_PATH_PROTOCOL instance.
  @param[in]  UsbCdcDataPath    A pointer to the EFI_DEVICE_PATH_PROTOCOL instance.

  @retval EFI_SUCCESS           Is the same device.
  @retval EFI_UNSUPPORTED       Is not the same device.

**/
EFI_STATUS
IsSameDevice (
  IN  EFI_DEVICE_PATH_PROTOCOL  *UsbRndisDataPath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *UsbCdcDataPath
  )
{
  DEBUG ((DEBUG_VERBOSE, "IsSameDevice Entry \n"));
  while (1) {
    if (IsDevicePathEnd (NextDevicePathNode (UsbRndisDataPath))) {
      if (((USB_DEVICE_PATH *)UsbRndisDataPath)->ParentPortNumber ==
          ((USB_DEVICE_PATH *)UsbCdcDataPath)->ParentPortNumber)
      {
        return EFI_SUCCESS;
      } else {
        return EFI_UNSUPPORTED;
      }
    } else {
      if (CompareMem (UsbCdcDataPath, UsbRndisDataPath, sizeof (EFI_DEVICE_PATH_PROTOCOL)) != 0) {
        return EFI_UNSUPPORTED;
      }

      UsbRndisDataPath = NextDevicePathNode (UsbRndisDataPath);
      UsbCdcDataPath   = NextDevicePathNode (UsbCdcDataPath);
    }
  }

  DEBUG ((DEBUG_VERBOSE, "IsSameDevice Exit \n"));
}

/**
  Check if the USB CDC Data(UsbIo) installed and return USB CDC Data Handle.

  @param[in]  UsbIo  A pointer to the EFI_USB_IO_PROTOCOL instance.

  @retval TRUE              USB CDC Data(UsbIo) installed.
  @retval FALSE             USB CDC Data(UsbIo) did not installed.

**/
BOOLEAN
IsUsbCdcData (
  IN EFI_USB_IO_PROTOCOL  *UsbIo
  )
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &InterfaceDescriptor);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  // Check for CDC-DATA
  if ((InterfaceDescriptor.InterfaceClass == USB_CDC_DATA_CLASS) &&
      (InterfaceDescriptor.InterfaceSubClass == USB_CDC_DATA_SUBCLASS) &&
      (InterfaceDescriptor.InterfaceProtocol == USB_NO_CLASS_PROTOCOL))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Check if the USB Rndis(UsbIo) installed

  @param[in]  UsbIo        A pointer to the EFI_USB_IO_PROTOCOL instance.

  @retval TRUE              USB Rndis(UsbIo) installed.
  @retval FALSE             USB Rndis(UsbIo) did not installed.

**/
BOOLEAN
IsUsbRndis (
  IN EFI_USB_IO_PROTOCOL  *UsbIo
  )
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &InterfaceDescriptor);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  // Check for Rndis
  if ((InterfaceDescriptor.InterfaceClass == USB_CDC_CLASS) &&
      (InterfaceDescriptor.InterfaceSubClass == USB_CDC_ACM_SUBCLASS) &&
      (InterfaceDescriptor.InterfaceProtocol == USB_VENDOR_PROTOCOL))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Control comes here when a CDC device is found.Check if a RNDIS interface is already found for this device or not.
  For one device two USBIO will be installed each for CDC and RNDIS interface.

  @param[in]  UsbCdcDataPath    A pointer to the EFI_DEVICE_PATH_PROTOCOL instance.
  @param[out] UsbRndisDevice    A pointer to the USB_RNDIS_DEVICE Data.

  @retval EFI_SUCCESS             The USB_RNDIS_DEVICE matching this CDC Data is found.
  @retval EFI_NOT_FOUND           The USB_RNDIS_DEVICE matching this CDC Data is not found.

**/
EFI_STATUS
UpdateRndisDevice (
  IN  EFI_DEVICE_PATH_PROTOCOL  *UsbCdcDataPath,
  OUT USB_RNDIS_DEVICE          **UsbRndisDevice
  )
{
  EFI_STATUS                   Status;
  UINTN                        Index;
  UINTN                        HandleCount;
  EFI_HANDLE                   *HandleBuffer;
  EDKII_USB_ETHERNET_PROTOCOL  *UsbEthDevice;
  EFI_DEVICE_PATH_PROTOCOL     *UsbRndisDataPath;
  EFI_USB_IO_PROTOCOL          *UsbIo;
  BOOLEAN                      IsRndisInterfaceFlag;

  IsRndisInterfaceFlag = FALSE;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEdkIIUsbEthProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEdkIIUsbEthProtocolGuid,
                    (VOID **)&UsbEthDevice
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **)&UsbIo
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    IsRndisInterfaceFlag = IsRndisInterface (UsbIo);
    if (IsRndisInterfaceFlag == FALSE) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&UsbRndisDataPath
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = IsSameDevice (UsbRndisDataPath, UsbCdcDataPath);

    DEBUG ((DEBUG_VERBOSE, "Rndis IsSameDevice %r\n", Status));

    if (!EFI_ERROR (Status)) {
      *UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (UsbEthDevice);
      FreePool (HandleBuffer);
      return EFI_SUCCESS;
    }
  }   // End of For loop

  FreePool (HandleBuffer);
  return EFI_NOT_FOUND;
}

/**

  For the given Rndis Device, find a matching CDC device already exists or not. If found update the handle
  and UsbIO protocol.

  @param[in]  UsbRndisDevice        A pointer to the USB_RNDIS_DEVICE data.

**/
VOID
FindMatchingCdcData (
  IN USB_RNDIS_DEVICE  *UsbRndisDevice
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  EFI_DEVICE_PATH_PROTOCOL  *UsbRndisDataPath;
  EFI_DEVICE_PATH_PROTOCOL  *UsbCdcDataPath;

  // Find the parent RNDIS and update the UsbIo for the CDC device
  Status = gBS->HandleProtocol (
                  UsbRndisDevice->UsbRndisHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&UsbRndisDataPath
                  );

  if (EFI_ERROR (Status)) {
    return;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiUsbIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **)&UsbIo
                    );
    ASSERT_EFI_ERROR (Status);

    if (IsUsbCdcData (UsbIo)) {
      DEBUG ((DEBUG_VERBOSE, "Rndis FindMatchingCdcData CDCData interface found\n"));

      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiDevicePathProtocolGuid,
                      (VOID **)&UsbCdcDataPath
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_VERBOSE, "Rndis CDCData DevicePath not found\n"));
        FreePool (HandleBuffer);
        return;
      }

      Status = IsSameDevice (UsbRndisDataPath, UsbCdcDataPath);
      DEBUG ((DEBUG_VERBOSE, "Rndis IsSameDevice %r\n", Status));
      if (!EFI_ERROR (Status)) {
        UsbRndisDevice->UsbCdcDataHandle = HandleBuffer[Index];
        UsbRndisDevice->UsbIoCdcData     = UsbIo;
        GetEndpoint (UsbRndisDevice->UsbIoCdcData, UsbRndisDevice);
        FreePool (HandleBuffer);
        return;
      }
    }
  }   // End of For loop

  FreePool (HandleBuffer);
}

/**

  For the given UsbIo CdcData, find a matching RNDIS device already exists or not.

  @param[in]  CdcHandle       A pointer to the EFI_HANDLE for USB CDC Data.
  @param[out] CdcUsbIo        A pointer for retrieve the EFI_USB_IO_PROTOCOL instance.
  @param[out] RndisHandle     A pointer for retrieve the handle of RNDIS device.

  @retval EFI_SUCCESS             The USB_RNDIS_DEVICE matching this CDC Data is found.
  @retval EFI_NOT_FOUND           The USB_RNDIS_DEVICE matching this CDC Data is not found.

**/
EFI_STATUS
EFIAPI
FindMatchingRndisDev (
  IN  EFI_HANDLE           CdcHandle,
  OUT EFI_USB_IO_PROTOCOL  **CdcUsbIo,
  OUT EFI_HANDLE           *RndisHandle
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  EFI_DEVICE_PATH_PROTOCOL  *UsbRndisDataPath;
  EFI_DEVICE_PATH_PROTOCOL  *UsbCdcDataPath;

  // Find the parent RNDIS and update the UsbIo for the CDC device
  Status = gBS->HandleProtocol (
                  CdcHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&UsbCdcDataPath
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiUsbIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **)&UsbIo
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (IsUsbRndis (UsbIo)) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiDevicePathProtocolGuid,
                      (VOID **)&UsbRndisDataPath
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Usb Rndis DevicePath not found\n"));
        break;
      }

      Status = IsSameDevice (UsbRndisDataPath, UsbCdcDataPath);

      if (!EFI_ERROR (Status)) {
        *RndisHandle = HandleBuffer[Index];
        *CdcUsbIo    = UsbIo;
        FreePool (HandleBuffer);
        return Status;
      }
    }
  }   // End of For loop

  FreePool (HandleBuffer);

  return EFI_NOT_FOUND;
}

/**
  USB Rndis Driver Binding Support.

  @param[in]  This                    Protocol instance pointer.
  @param[in]  ControllerHandle        Handle of device to test.
  @param[in]  RemainingDevicePath     Optional parameter use to pick a specific child
                                      device to start.

  @retval EFI_SUCCESS                 This driver supports this device.
  @retval EFI_ALREADY_STARTED         This driver is already running on this device.
  @retval other                       This driver does not support this device.

**/
EFI_STATUS
EFIAPI
UsbRndisDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_USB_IO_PROTOCOL  *UsbIo;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IsSupportedDevice (UsbIo) ? EFI_SUCCESS : EFI_UNSUPPORTED;

  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );
  return Status;
}

/**
  USB RNDIS Driver Binding Start.

  @param[in]  This                    Protocol instance pointer.
  @param[in]  ControllerHandle        Handle of device to bind driver to.
  @param[in]  RemainingDevicePath     Optional parameter use to pick a specific child
                                      device to start.

  @retval EFI_SUCCESS                 This driver is added to ControllerHandle
  @retval EFI_DEVICE_ERROR            This driver could not be started due to a device error
  @retval EFI_OUT_OF_RESOURCES        The driver could not install successfully due to a lack of resources.
  @retval other                       This driver does not support this device

**/
EFI_STATUS
EFIAPI
UsbRndisDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                    Status;
  USB_RNDIS_DEVICE              *UsbRndisDevice;
  EFI_DEVICE_PATH_PROTOCOL      *UsbEthPath;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  Interface;
  EFI_HANDLE                    RndisHandle;

  RndisHandle = ControllerHandle;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&UsbEthPath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiUsbIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    return Status;
  }

  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  // Controls come here for RNDIS and CDC. If it is CDC, check whether RNDIS is present on the same controller or not.
  if (IsUsbCdcData (UsbIo)) {
    DEBUG ((DEBUG_INFO, "Rndis CDCData interface found\n"));

    // Find the parent RNDIS and update the UsbIo for the CDC device
    Status = UpdateRndisDevice (
               UsbEthPath,
               &UsbRndisDevice
               );

    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Rndis Matching interface found\n"));
      UsbRndisDevice->UsbCdcDataHandle = ControllerHandle;
      UsbRndisDevice->UsbIoCdcData     = UsbIo;
      GetEndpoint (UsbRndisDevice->UsbIoCdcData, UsbRndisDevice);
      return Status;
    } else {
      // Check if RnDis exist
      Status = FindMatchingRndisDev (
                 ControllerHandle,
                 &UsbIo,
                 &RndisHandle
                 );

      if (EFI_ERROR (Status)) {
        gBS->CloseProtocol (
               ControllerHandle,
               &gEfiUsbIoProtocolGuid,
               This->DriverBindingHandle,
               ControllerHandle
               );
        return Status;
      }
    }
  }

  UsbRndisDevice = AllocateZeroPool (sizeof (USB_RNDIS_DEVICE));

  if (!UsbRndisDevice) {
    DEBUG ((DEBUG_ERROR, "AllocateZeroPool Fail\n"));

    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiUsbIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    return EFI_OUT_OF_RESOURCES;
  }

  Status = LoadAllDescriptor (
             UsbIo,
             &UsbRndisDevice->Config
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:LoadAllDescriptor status = %r\n", __func__, Status));
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiUsbIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    FreePool (UsbRndisDevice);
    return Status;
  }

  Status = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &Interface
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:UsbGetInterfaceDescriptor status = %r\n", __func__, Status));
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiUsbIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    FreePool (UsbRndisDevice->Config);
    FreePool (UsbRndisDevice);
    return Status;
  }

  UsbRndisDevice->Signature                          = USB_RNDIS_SIGNATURE;
  UsbRndisDevice->NumOfInterface                     = Interface.InterfaceNumber;
  UsbRndisDevice->UsbRndisHandle                     = RndisHandle;
  UsbRndisDevice->UsbCdcDataHandle                   = 0;
  UsbRndisDevice->UsbIo                              = UsbIo;
  UsbRndisDevice->UsbEth.UsbEthReceive               = RndisUndiReceive;
  UsbRndisDevice->UsbEth.UsbEthTransmit              = RndisUndiTransmit;
  UsbRndisDevice->UsbEth.UsbEthInterrupt             = UsbRndisInterrupt;
  UsbRndisDevice->UsbEth.UsbEthMacAddress            = GetUsbEthMacAddress;
  UsbRndisDevice->UsbEth.UsbEthMaxBulkSize           = UsbEthBulkSize;
  UsbRndisDevice->UsbEth.UsbHeaderFunDescriptor      = GetUsbHeaderFunDescriptor;
  UsbRndisDevice->UsbEth.UsbUnionFunDescriptor       = GetUsbUnionFunDescriptor;
  UsbRndisDevice->UsbEth.UsbEthFunDescriptor         = GetUsbRndisFunDescriptor;
  UsbRndisDevice->UsbEth.SetUsbEthMcastFilter        = SetUsbRndisMcastFilter;
  UsbRndisDevice->UsbEth.SetUsbEthPowerPatternFilter = SetUsbRndisPowerFilter;
  UsbRndisDevice->UsbEth.GetUsbEthPowerPatternFilter = GetUsbRndisPowerFilter;
  UsbRndisDevice->UsbEth.SetUsbEthPacketFilter       = SetUsbRndisPacketFilter;
  UsbRndisDevice->UsbEth.GetUsbEthStatistic          = GetRndisStatistic;

  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiGetState        = RndisDummyReturn;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiStart           = RndisUndiStart;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiStop            = RndisUndiStop;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiGetInitInfo     = RndisUndiGetInitInfo;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiGetConfigInfo   = RndisUndiGetConfigInfo;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiInitialize      = RndisUndiInitialize;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiReset           = RndisUndiReset;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiShutdown        = RndisUndiShutdown;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiInterruptEnable = RndisDummyReturn;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiReceiveFilter   = RndisUndiReceiveFilter;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiStationAddress  = RndisDummyReturn;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiStatistics      = NULL;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiMcastIp2Mac     = RndisDummyReturn;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiNvData          = RndisDummyReturn;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiGetStatus       = RndisUndiGetStatus;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiFillHeader      = RndisDummyReturn;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiTransmit        = NULL;
  UsbRndisDevice->UsbEth.UsbEthUndi.UsbEthUndiReceive         = NULL;

  UsbRndisDevice->MaxTransferSize       = RNDIS_MAX_TRANSFER_SIZE;
  UsbRndisDevice->MaxPacketsPerTransfer = 1;
  UsbRndisDevice->PacketAlignmentFactor = 0;

  InitializeListHead (&UsbRndisDevice->ReceivePacketList);

  // This is a RNDIS interface. See whether CDC-DATA interface has already been connected or not
  FindMatchingCdcData (UsbRndisDevice);

  if (UsbRndisDevice->UsbIoCdcData) {
    Status = gBS->InstallProtocolInterface (
                    &ControllerHandle,
                    &gEdkIIUsbEthProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &(UsbRndisDevice->UsbEth)
                    );
    if (EFI_ERROR (Status)) {
      gBS->CloseProtocol (
             ControllerHandle,
             &gEfiUsbIoProtocolGuid,
             This->DriverBindingHandle,
             ControllerHandle
             );

      FreePool (UsbRndisDevice->Config);
      FreePool (UsbRndisDevice);
      return Status;
    }

    GetEndpoint (UsbRndisDevice->UsbIo, UsbRndisDevice);

    DEBUG ((DEBUG_INFO, "Rndis DeviceHandle %r\n", UsbRndisDevice->UsbRndisHandle));
    DEBUG ((DEBUG_INFO, "CDC DeviceHandle %r\n", UsbRndisDevice->UsbCdcDataHandle));
    return EFI_SUCCESS;
  }

  FreePool (UsbRndisDevice->Config);
  FreePool (UsbRndisDevice);

  return EFI_SUCCESS;
}

/**
  CheckandStopRndisDevice

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to bind driver to.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_DEVICE_ERROR     This driver could not be started due to a device error
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
CheckandStopRndisDevice (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle
  )
{
  EFI_STATUS           Status;
  EFI_USB_IO_PROTOCOL  *UsbIo;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (IsUsbRndis (UsbIo)) {
    Status = gBS->CloseProtocol (
                    ControllerHandle,
                    &gEfiUsbIoProtocolGuid,
                    This->DriverBindingHandle,
                    ControllerHandle
                    );
    DEBUG ((DEBUG_ERROR, "Rndis ControllerHandle Stop %r\n", Status));
    return Status;
  }

  return EFI_UNSUPPORTED;
}

/**
  USB Rndis Driver Binding Stop.

  @param[in]  This                  Protocol instance pointer.
  @param[in]  ControllerHandle      Handle of device to stop driver on
  @param[in]  NumberOfChildren      Number of Handles in ChildHandleBuffer. If number of
                                    children is zero stop the entire bus driver.
  @param[in]  ChildHandleBuffer     List of Child Handles to Stop.

  @retval EFI_SUCCESS               This driver is removed ControllerHandle
  @retval other                     This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
UsbRndisDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                   Status;
  EDKII_USB_ETHERNET_PROTOCOL  *UsbEthProtocol;
  USB_RNDIS_DEVICE             *UsbRndisDevice;

  DEBUG ((DEBUG_INFO, "UsbRndisDriverStop ControllerHandle %lx\n", ControllerHandle));

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEdkIIUsbEthProtocolGuid,
                  (VOID **)&UsbEthProtocol,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    Status = CheckandStopRndisDevice (This, ControllerHandle);
    return Status;
  }

  UsbRndisDevice = USB_RNDIS_DEVICE_FROM_THIS (UsbEthProtocol);

  Status = gBS->CloseProtocol (
                  UsbRndisDevice->UsbCdcDataHandle,
                  &gEfiUsbIoProtocolGuid,
                  This->DriverBindingHandle,
                  UsbRndisDevice->UsbCdcDataHandle
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:CloseProtocol status = %r\n", __func__, Status));
  }

  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEdkIIUsbEthProtocolGuid,
                  UsbEthProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->CloseProtocol (
                  ControllerHandle,
                  &gEfiUsbIoProtocolGuid,
                  This->DriverBindingHandle,
                  ControllerHandle
                  );

  FreePool (UsbRndisDevice->Config);
  FreePool (UsbRndisDevice);

  DEBUG ((DEBUG_INFO, "UsbRndisDriverStop %r\n", Status));
  return Status;
}

/**
  Entrypoint of RNDIS Driver.

  This function is the entrypoint of RNDIS Driver. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param[in]  ImageHandle       The firmware allocated handle for the EFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
UsbRndisEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  gUsbRndisDriverBinding.DriverBindingHandle = ImageHandle;
  gUsbRndisDriverBinding.ImageHandle         = ImageHandle;

  return gBS->InstallMultipleProtocolInterfaces (
                &gUsbRndisDriverBinding.DriverBindingHandle,
                &gEfiDriverBindingProtocolGuid,
                &gUsbRndisDriverBinding,
                &gEfiComponentName2ProtocolGuid,
                &gUsbRndisComponentName2,
                NULL
                );
}
