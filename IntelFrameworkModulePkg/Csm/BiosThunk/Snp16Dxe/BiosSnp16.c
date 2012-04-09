/** @file

Copyright (c) 1999 - 2012, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BiosSnp16.h"


///
/// EFI Driver Binding Protocol Instance
///
EFI_DRIVER_BINDING_PROTOCOL gBiosSnp16DriverBinding = {
  BiosSnp16DriverBindingSupported,
  BiosSnp16DriverBindingStart,
  BiosSnp16DriverBindingStop,
  0x3,
  NULL,
  NULL
};

///
///  This boolean is used to determine if we should release the cached vector during an error condition.
///
BOOLEAN     mCachedInt1A = FALSE;

//
// Private worker functions;
//

/**
  Start the UNDI interface.

  @param SimpleNetworkDevice A pointer to EFI_SIMPLE_NETWORK_DEV data structure.
  @param Ax                  PCI address of Undi device.
  
  @retval EFI_DEVICE_ERROR Fail to start 16 bit UNDI ROM. 
  @retval Others           Status of start 16 bit UNDI ROM. 
**/
EFI_STATUS
Undi16SimpleNetworkStartUndi (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  UINT16                  Ax
  );

/**
  Start the UNDI interface

  @param SimpleNetworkDevice A pointer to EFI_SIMPLE_NETWORK_DEV data structure.
  
  @retval EFI_DEVICE_ERROR Fail to start 16 bit UNDI ROM. 
  @retval Others           Status of start 16 bit UNDI ROM. 
**/
EFI_STATUS
Undi16SimpleNetworkStopUndi (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice
  );

/**
  Stop the UNDI interface

  @param SimpleNetworkDevice A pointer to EFI_SIMPLE_NETWORK_DEV data structure.
  
  @retval EFI_DEVICE_ERROR Fail to stop 16 bit UNDI ROM. 
  @retval Others           Status of stop 16 bit UNDI ROM. 
**/
EFI_STATUS
Undi16SimpleNetworkCleanupUndi (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice
  );

/**
  Get runtime information for Undi network interface

  @param This A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  
  @retval EFI_SUCCESS Sucess operation. 
  @retval Others      Fail to get runtime information for Undi network interface. 
**/
EFI_STATUS
Undi16SimpleNetworkGetInformation (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  );

/**
  Get NIC type

  @param This A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  
  @retval EFI_SUCCESS Sucess operation. 
  @retval Others      Fail to get NIC type.
**/
EFI_STATUS
Undi16SimpleNetworkGetNicType (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  );

/**
  Get NDIS information

  @param This A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  
  @retval EFI_SUCCESS Sucess operation. 
  @retval Others      Fail to get NDIS information.
**/
EFI_STATUS
Undi16SimpleNetworkGetNdisInfo (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  );

/**
  Signal handlers for ExitBootServices event.
  
  Clean up any Real-mode UNDI residue from the system 
   
  @param Event      ExitBootServices event
  @param Context 
**/
VOID
EFIAPI
Undi16SimpleNetworkEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Loads the undi driver.

  @param SimpleNetworkDevice A pointer to EFI_SIMPLE_NETWORK_DEV data structure.
  
  @retval   EFI_SUCCESS   - Successfully loads undi driver.
  @retval   EFI_NOT_FOUND - Doesn't find undi driver or undi driver load failure.
**/
EFI_STATUS
Undi16SimpleNetworkLoadUndi (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice
  );

/**
  Unload 16 bit UNDI Option ROM from memory

  @param SimpleNetworkDevice A pointer to EFI_SIMPLE_NETWORK_DEV data structure.
  
  @return EFI_STATUS 
**/
EFI_STATUS
Undi16SimpleNetworkUnloadUndi (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice
  );

/**
  Entry point for EFI drivers.

  @param ImageHandle Handle that identifies the loaded image.
  @param SystemTable System Table for this image.
  
  @return EFI_STATUS Return status from EfiLibInstallAllDriverProtocols. 
**/
EFI_STATUS
EFIAPI
BiosSnp16DriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gBiosSnp16DriverBinding,
           ImageHandle,
           &gBiosSnp16ComponentName,
           &gBiosSnp16ComponentName2
           );
}

//
// EFI Driver Binding Protocol Functions
//
/**
  Tests to see if this driver supports a given controller.

  @param This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param Controller           The handle of the controller to test.
  @param RemainingDevicePath  A pointer to the remaining portion of a device path.
  
  @retval EFI_SUCCESS    The driver supports given controller.
  @retval EFI_UNSUPPORT  The driver doesn't support given controller.
  @retval Other          Other errors prevent driver finishing to test
                         if the driver supports given controller.
**/
EFI_STATUS
EFIAPI
BiosSnp16DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;

  //
  // See if the Legacy BIOS Protocol is available
  //
  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // See if this is a PCI Network Controller by looking at the Command register and
  // Class Code Register
  //
  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0, sizeof (Pci) / sizeof (UINT32), &Pci);
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status = EFI_UNSUPPORTED;
  if (Pci.Hdr.ClassCode[2] == PCI_CLASS_NETWORK) {
    Status = EFI_SUCCESS;
  }

Done:
  gBS->CloseProtocol (
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}

/**
  Starts the Snp device controller

  @param This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param Controller           The handle of the controller to test.
  @param RemainingDevicePath  A pointer to the remaining portion of a device path.
  
  @retval  EFI_SUCCESS          - The device was started.   
  @retval  EFI_DEVICE_ERROR     - The device could not be started due to a device error.
  @retval  EFI_OUT_OF_RESOURCES - The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
BiosSnp16DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_SIMPLE_NETWORK_DEV    *SimpleNetworkDevice;
  EFI_DEV_PATH              Node;
  UINTN                     Index;
  UINTN                     Index2;
  UINTN                     Segment;
  UINTN                     Bus;
  UINTN                     Device;
  UINTN                     Function;
  UINTN                     Flags;
  UINT64                    Supports;

  SimpleNetworkDevice = NULL;
  PciIo               = NULL;

  //
  // See if the Legacy BIOS Protocol is available
  //
  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Open the IO Abstraction(s) needed
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );
  if (!EFI_ERROR (Status)) {
    Supports &= EFI_PCI_DEVICE_ENABLE;
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationEnable,
                      Supports,
                      NULL
                      );
  }

  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Check to see if there is a legacy option ROM image associated with this PCI device
  //
  Status = LegacyBios->CheckPciRom (
                         LegacyBios,
                         Controller,
                         NULL,
                         NULL,
                         &Flags
                         );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Post the legacy option ROM if it is available.
  //
  Status = LegacyBios->InstallPciRom (
                         LegacyBios,
                         Controller,
                         NULL,
                         &Flags,
                         NULL,
                         NULL,
                         NULL,
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Allocate memory for this SimpleNetwork device instance
  //
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_SIMPLE_NETWORK_DEV),
                  (VOID **) &SimpleNetworkDevice
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  ZeroMem (SimpleNetworkDevice, sizeof (EFI_SIMPLE_NETWORK_DEV));

  //
  // Initialize the SimpleNetwork device instance
  //
  SimpleNetworkDevice->Signature      = EFI_SIMPLE_NETWORK_DEV_SIGNATURE;
  SimpleNetworkDevice->LegacyBios     = LegacyBios;
  SimpleNetworkDevice->BaseDevicePath = DevicePath;
  SimpleNetworkDevice->PciIo          = PciIo;

  //
  // Initialize the Nii Protocol
  //
  SimpleNetworkDevice->Nii.Revision = EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_REVISION;
  SimpleNetworkDevice->Nii.Type     = EfiNetworkInterfaceUndi;

  CopyMem (&SimpleNetworkDevice->Nii.StringId, "UNDI", 4);

  //
  // Load 16 bit UNDI Option ROM into Memory
  //
  Status = Undi16SimpleNetworkLoadUndi (SimpleNetworkDevice);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_NET, "ERROR : Could not load UNDI.  Status = %r\n", Status));
    goto Done;
  }

  SimpleNetworkDevice->UndiLoaded = TRUE;

  //
  // Call PXENV_START_UNDI - Initilizes the UNID interface for use.
  //
  PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);
  Status = Undi16SimpleNetworkStartUndi (
             SimpleNetworkDevice,
             (UINT16) ((Bus << 0x8) | (Device << 0x3) | (Function))
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_NET, "ERROR : Could not StartUndi.  Status = %r\n", Status));
    goto Done;
  }
  //
  // Initialize the Simple Network Protocol
  //
  DEBUG ((DEBUG_NET, "Initialize SimpleNetworkDevice instance\n"));

  SimpleNetworkDevice->SimpleNetwork.Revision       = EFI_SIMPLE_NETWORK_PROTOCOL_REVISION;
  SimpleNetworkDevice->SimpleNetwork.Start          = Undi16SimpleNetworkStart;
  SimpleNetworkDevice->SimpleNetwork.Stop           = Undi16SimpleNetworkStop;
  SimpleNetworkDevice->SimpleNetwork.Initialize     = Undi16SimpleNetworkInitialize;
  SimpleNetworkDevice->SimpleNetwork.Reset          = Undi16SimpleNetworkReset;
  SimpleNetworkDevice->SimpleNetwork.Shutdown       = Undi16SimpleNetworkShutdown;
  SimpleNetworkDevice->SimpleNetwork.ReceiveFilters = Undi16SimpleNetworkReceiveFilters;
  SimpleNetworkDevice->SimpleNetwork.StationAddress = Undi16SimpleNetworkStationAddress;
  SimpleNetworkDevice->SimpleNetwork.Statistics     = Undi16SimpleNetworkStatistics;
  SimpleNetworkDevice->SimpleNetwork.MCastIpToMac   = Undi16SimpleNetworkMCastIpToMac;
  SimpleNetworkDevice->SimpleNetwork.NvData         = Undi16SimpleNetworkNvData;
  SimpleNetworkDevice->SimpleNetwork.GetStatus      = Undi16SimpleNetworkGetStatus;
  SimpleNetworkDevice->SimpleNetwork.Transmit       = Undi16SimpleNetworkTransmit;
  SimpleNetworkDevice->SimpleNetwork.Receive        = Undi16SimpleNetworkReceive;
  SimpleNetworkDevice->SimpleNetwork.Mode           = &(SimpleNetworkDevice->SimpleNetworkMode);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  Undi16SimpleNetworkWaitForPacket,
                  &SimpleNetworkDevice->SimpleNetwork,
                  &SimpleNetworkDevice->SimpleNetwork.WaitForPacket
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR : Could not create event.  Status = %r\n", Status));
    goto Done;
  }
  //
  // Create an event to be signalled when ExitBootServices occurs in order
  // to clean up nicely
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Undi16SimpleNetworkEvent,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &SimpleNetworkDevice->EfiBootEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR : Could not create event.  Status = %r\n", Status));
    goto Done;
  }

  //
  // Create an event to be signalled when Legacy Boot occurs to clean up the IVT
  //
  Status = EfiCreateEventLegacyBootEx(
             TPL_NOTIFY, 
             Undi16SimpleNetworkEvent, 
             NULL, 
             &SimpleNetworkDevice->LegacyBootEvent
             );
  
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR,"ERROR : Could not create event.  Status = %r\n",Status));
    goto Done;
  }

  //
  // Initialize the SimpleNetwork Mode Information
  //
  DEBUG ((DEBUG_NET, "Initialize Mode Information\n"));

  SimpleNetworkDevice->SimpleNetworkMode.State                = EfiSimpleNetworkStopped;
  SimpleNetworkDevice->SimpleNetworkMode.MediaHeaderSize      = 14;
  SimpleNetworkDevice->SimpleNetworkMode.MacAddressChangeable = TRUE;
  SimpleNetworkDevice->SimpleNetworkMode.MultipleTxSupported  = TRUE;
  SimpleNetworkDevice->SimpleNetworkMode.ReceiveFilterMask = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST |
    EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST |
    EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST |
    EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS |
    EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;
  SimpleNetworkDevice->SimpleNetworkMode.MaxMCastFilterCount = MAXNUM_MCADDR;

  //
  // Initialize the SimpleNetwork Private Information
  //
  DEBUG ((DEBUG_NET, "Initialize Private Information\n"));

  Status = BiosSnp16AllocatePagesBelowOneMb (
             sizeof (PXENV_UNDI_TBD_T) / EFI_PAGE_SIZE + 1,
             (VOID **) &SimpleNetworkDevice->Xmit
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = BiosSnp16AllocatePagesBelowOneMb (
             1,
             &SimpleNetworkDevice->TxRealModeMediaHeader
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = BiosSnp16AllocatePagesBelowOneMb (
             1,
             &SimpleNetworkDevice->TxRealModeDataBuffer
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = BiosSnp16AllocatePagesBelowOneMb (
             1,
             &SimpleNetworkDevice->TxDestAddr
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  SimpleNetworkDevice->Xmit->XmitOffset               = (UINT16) (((UINT32)(UINTN) SimpleNetworkDevice->TxRealModeMediaHeader) & 0x000f);

  SimpleNetworkDevice->Xmit->XmitSegment              = (UINT16) (((UINT32)(UINTN) SimpleNetworkDevice->TxRealModeMediaHeader) >> 4);

  SimpleNetworkDevice->Xmit->DataBlkCount             = 1;

  SimpleNetworkDevice->Xmit->DataBlock[0].TDPtrType   = 1;
  SimpleNetworkDevice->Xmit->DataBlock[0].TDRsvdByte  = 0;

  SimpleNetworkDevice->Xmit->DataBlock[0].TDDataPtrOffset = (UINT16) (((UINT32)(UINTN) SimpleNetworkDevice->TxRealModeDataBuffer) & 0x000f);

  SimpleNetworkDevice->Xmit->DataBlock[0].TDDataPtrSegment = (UINT16) (((UINT32)(UINTN) SimpleNetworkDevice->TxRealModeDataBuffer) >> 4);

  SimpleNetworkDevice->TxBufferFifo.First = 0;
  SimpleNetworkDevice->TxBufferFifo.Last  = 0;

  //
  // Start() the SimpleNetwork device
  //
  DEBUG ((DEBUG_NET, "Start()\n"));

  Status = Undi16SimpleNetworkStart (&SimpleNetworkDevice->SimpleNetwork);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // GetInformation() the SimpleNetwork device
  //
  DEBUG ((DEBUG_NET, "GetInformation()\n"));

  Status = Undi16SimpleNetworkGetInformation (&SimpleNetworkDevice->SimpleNetwork);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Build the device path for the child device
  //
  ZeroMem (&Node, sizeof (Node));
  Node.DevPath.Type     = MESSAGING_DEVICE_PATH;
  Node.DevPath.SubType  = MSG_MAC_ADDR_DP;
  SetDevicePathNodeLength (&Node.DevPath, sizeof (MAC_ADDR_DEVICE_PATH));
  CopyMem (
    &Node.MacAddr.MacAddress,
    &SimpleNetworkDevice->SimpleNetworkMode.CurrentAddress,
    sizeof (EFI_MAC_ADDRESS)
    );
  SimpleNetworkDevice->DevicePath = AppendDevicePathNode (
                                      SimpleNetworkDevice->BaseDevicePath,
                                      &Node.DevPath
                                      );

  //
  // GetNicType()  the SimpleNetwork device
  //
  DEBUG ((DEBUG_NET, "GetNicType()\n"));

  Status = Undi16SimpleNetworkGetNicType (&SimpleNetworkDevice->SimpleNetwork);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // GetNdisInfo() the SimpleNetwork device
  //
  DEBUG ((DEBUG_NET, "GetNdisInfo()\n"));

  Status = Undi16SimpleNetworkGetNdisInfo (&SimpleNetworkDevice->SimpleNetwork);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Stop() the SimpleNetwork device
  //
  DEBUG ((DEBUG_NET, "Stop()\n"));

  Status = SimpleNetworkDevice->SimpleNetwork.Stop (&SimpleNetworkDevice->SimpleNetwork);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Print Mode information
  //
  DEBUG ((DEBUG_NET, "Mode->State                = %d\n", SimpleNetworkDevice->SimpleNetworkMode.State));
  DEBUG ((DEBUG_NET, "Mode->HwAddressSize        = %d\n", SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize));
  DEBUG ((DEBUG_NET, "Mode->MacAddressChangeable = %d\n", SimpleNetworkDevice->SimpleNetworkMode.MacAddressChangeable));
  DEBUG ((DEBUG_NET, "Mode->MultiplTxSupported   = %d\n", SimpleNetworkDevice->SimpleNetworkMode.MultipleTxSupported));
  DEBUG ((DEBUG_NET, "Mode->NvRamSize            = %d\n", SimpleNetworkDevice->SimpleNetworkMode.NvRamSize));
  DEBUG ((DEBUG_NET, "Mode->NvRamAccessSize      = %d\n", SimpleNetworkDevice->SimpleNetworkMode.NvRamAccessSize));
  DEBUG ((DEBUG_NET, "Mode->ReceiveFilterSetting = %d\n", SimpleNetworkDevice->SimpleNetworkMode.ReceiveFilterSetting));
  DEBUG ((DEBUG_NET, "Mode->IfType               = %d\n", SimpleNetworkDevice->SimpleNetworkMode.IfType));
  DEBUG ((DEBUG_NET, "Mode->MCastFilterCount     = %d\n", SimpleNetworkDevice->SimpleNetworkMode.MCastFilterCount));
  for (Index = 0; Index < SimpleNetworkDevice->SimpleNetworkMode.MCastFilterCount; Index++) {
    DEBUG ((DEBUG_NET, "  Filter[%02d] = ", Index));
    for (Index2 = 0; Index2 < 16; Index2++) {
      DEBUG ((DEBUG_NET, "%02x ", SimpleNetworkDevice->SimpleNetworkMode.MCastFilter[Index].Addr[Index2]));
    }

    DEBUG ((DEBUG_NET, "\n"));
  }

  DEBUG ((DEBUG_NET, "CurrentAddress = "));
  for (Index2 = 0; Index2 < 16; Index2++) {
    DEBUG ((DEBUG_NET, "%02x ", SimpleNetworkDevice->SimpleNetworkMode.CurrentAddress.Addr[Index2]));
  }

  DEBUG ((DEBUG_NET, "\n"));

  DEBUG ((DEBUG_NET, "BroadcastAddress = "));
  for (Index2 = 0; Index2 < 16; Index2++) {
    DEBUG ((DEBUG_NET, "%02x ", SimpleNetworkDevice->SimpleNetworkMode.BroadcastAddress.Addr[Index2]));
  }

  DEBUG ((DEBUG_NET, "\n"));

  DEBUG ((DEBUG_NET, "PermanentAddress = "));
  for (Index2 = 0; Index2 < 16; Index2++) {
    DEBUG ((DEBUG_NET, "%02x ", SimpleNetworkDevice->SimpleNetworkMode.PermanentAddress.Addr[Index2]));
  }

  DEBUG ((DEBUG_NET, "\n"));

  //
  // The network device was started, information collected, and stopped.
  // Install protocol interfaces for the SimpleNetwork device.
  //
  DEBUG ((DEBUG_NET, "Install Protocol Interfaces on network interface\n"));

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &SimpleNetworkDevice->Handle,
                  &gEfiSimpleNetworkProtocolGuid,
                  &SimpleNetworkDevice->SimpleNetwork,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid,
                  &SimpleNetworkDevice->Nii,
                  &gEfiDevicePathProtocolGuid,
                  SimpleNetworkDevice->DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Open PCI I/O from the newly created child handle
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  SimpleNetworkDevice->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );

  DEBUG ((DEBUG_INIT, "UNDI16 Driver : EFI_SUCCESS\n"));

Done:
  if (EFI_ERROR (Status)) {
    if (SimpleNetworkDevice != NULL) {

      Undi16SimpleNetworkShutdown (&SimpleNetworkDevice->SimpleNetwork);
      //
      // CLOSE + SHUTDOWN
      //
      Undi16SimpleNetworkCleanupUndi (SimpleNetworkDevice);
      //
      // CLEANUP
      //
      Undi16SimpleNetworkStopUndi (SimpleNetworkDevice);
      //
      // STOP
      //
      if (SimpleNetworkDevice->UndiLoaded) {
        Undi16SimpleNetworkUnloadUndi (SimpleNetworkDevice);
      }

      if (SimpleNetworkDevice->SimpleNetwork.WaitForPacket != NULL) {
        gBS->CloseEvent (SimpleNetworkDevice->SimpleNetwork.WaitForPacket);
      }

      if (SimpleNetworkDevice->LegacyBootEvent != NULL) {
        gBS->CloseEvent (SimpleNetworkDevice->LegacyBootEvent);
      }
      
      if (SimpleNetworkDevice->EfiBootEvent != NULL) {
        gBS->CloseEvent (SimpleNetworkDevice->EfiBootEvent);
      }

      if (SimpleNetworkDevice->Xmit != NULL) {
        gBS->FreePages (
               (EFI_PHYSICAL_ADDRESS) (UINTN) SimpleNetworkDevice->Xmit,
               sizeof (PXENV_UNDI_TBD_T) / EFI_PAGE_SIZE + 1
               );
      }

      if (SimpleNetworkDevice->TxRealModeMediaHeader != NULL) {
        gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) SimpleNetworkDevice->TxRealModeMediaHeader, 1);
      }

      if (SimpleNetworkDevice->TxRealModeDataBuffer != NULL) {
        gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) SimpleNetworkDevice->TxRealModeDataBuffer, 1);
      }

      if (SimpleNetworkDevice->TxDestAddr != NULL) {
        gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) SimpleNetworkDevice->TxDestAddr, 1);
      }

      gBS->FreePool (SimpleNetworkDevice);
      
      //
      //  Only restore the vector if it was cached.
      //
      if (mCachedInt1A) {
        RestoreCachedVectorAddress (0x1A);
        mCachedInt1A = FALSE;
      }
    }

    if (PciIo != NULL) {
      Status = PciIo->Attributes (
                        PciIo,
                        EfiPciIoAttributeOperationSupported,
                        0,
                        &Supports
                        );
      if (!EFI_ERROR (Status)) {
        Supports &= EFI_PCI_DEVICE_ENABLE;
        Status = PciIo->Attributes (
                          PciIo,
                          EfiPciIoAttributeOperationDisable,
                          Supports,
                          NULL
                          );
      }
    }

    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    if (Status != EFI_OUT_OF_RESOURCES) {
      Status = EFI_DEVICE_ERROR;
    }
  }  
  return Status;
}

/**
  Stops the device by given device controller.

  @param This               A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param Controller         The handle of the controller to test.
  @param NumberOfChildren   The number of child device handles in ChildHandleBuffer.
  @param ChildHandleBuffer  An array of child handles to be freed. May be NULL if
                            NumberOfChildren is 0.
  
  @retval  EFI_SUCCESS      - The device was stopped.
  @retval  EFI_DEVICE_ERROR - The device could not be stopped due to a device error.
**/
EFI_STATUS
EFIAPI
BiosSnp16DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  BOOLEAN                     AllChildrenStopped;
  EFI_SIMPLE_NETWORK_PROTOCOL *SimpleNetwork;
  EFI_SIMPLE_NETWORK_DEV      *SimpleNetworkDevice;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  UINT64                      Supports;

  //
  // Complete all outstanding transactions to Controller.
  // Don't allow any new transaction to Controller to be started.
  //
  if (NumberOfChildren == 0) {
    //
    // Close the bus driver
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiPciIoProtocolGuid,
                    (VOID **) &PciIo,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      Status = PciIo->Attributes (
                        PciIo,
                        EfiPciIoAttributeOperationSupported,
                        0,
                        &Supports
                        );
      if (!EFI_ERROR (Status)) {
        Supports &= EFI_PCI_DEVICE_ENABLE;
        Status = PciIo->Attributes (
                          PciIo,
                          EfiPciIoAttributeOperationDisable,
                          Supports,
                          NULL
                          );
      }
    }

    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiPciIoProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );

    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiDevicePathProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
                    
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
    }
    return Status;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {

    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiSimpleNetworkProtocolGuid,
                    (VOID **) &SimpleNetwork,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {

      SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (SimpleNetwork);

      Status = gBS->CloseProtocol (
                      Controller,
                      &gEfiPciIoProtocolGuid,
                      This->DriverBindingHandle,
                      ChildHandleBuffer[Index]
                      );

      Status = gBS->UninstallMultipleProtocolInterfaces (
                      SimpleNetworkDevice->Handle,
                      &gEfiSimpleNetworkProtocolGuid,
                      &SimpleNetworkDevice->SimpleNetwork,
                      &gEfiNetworkInterfaceIdentifierProtocolGuid,
                      &SimpleNetworkDevice->Nii,
                      &gEfiDevicePathProtocolGuid,
                      SimpleNetworkDevice->DevicePath,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        gBS->OpenProtocol (
               Controller,
               &gEfiPciIoProtocolGuid,
               (VOID **) &PciIo,
               This->DriverBindingHandle,
               ChildHandleBuffer[Index],
               EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
               );
      } else {

        Undi16SimpleNetworkShutdown (&SimpleNetworkDevice->SimpleNetwork);
        //
        // CLOSE + SHUTDOWN
        //
        Undi16SimpleNetworkCleanupUndi (SimpleNetworkDevice);
        //
        // CLEANUP
        //
        Undi16SimpleNetworkStopUndi (SimpleNetworkDevice);
        //
        // STOP
        //
        if (SimpleNetworkDevice->UndiLoaded) {
          Undi16SimpleNetworkUnloadUndi (SimpleNetworkDevice);
        }

        if (SimpleNetworkDevice->SimpleNetwork.WaitForPacket != NULL) {
          gBS->CloseEvent (SimpleNetworkDevice->SimpleNetwork.WaitForPacket);
        }

        if (SimpleNetworkDevice->LegacyBootEvent != NULL) {
          gBS->CloseEvent (SimpleNetworkDevice->LegacyBootEvent);
        }
      
        if (SimpleNetworkDevice->EfiBootEvent != NULL) {
          gBS->CloseEvent (SimpleNetworkDevice->EfiBootEvent);
        }

        if (SimpleNetworkDevice->Xmit != NULL) {
          gBS->FreePages (
                 (EFI_PHYSICAL_ADDRESS) (UINTN) SimpleNetworkDevice->Xmit,
                 sizeof (PXENV_UNDI_TBD_T) / EFI_PAGE_SIZE + 1
                 );
        }

        if (SimpleNetworkDevice->TxRealModeMediaHeader != NULL) {
          gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) SimpleNetworkDevice->TxRealModeMediaHeader, 1);
        }

        if (SimpleNetworkDevice->TxRealModeDataBuffer != NULL) {
          gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) SimpleNetworkDevice->TxRealModeDataBuffer, 1);
        }

        if (SimpleNetworkDevice->TxDestAddr != NULL) {
          gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) SimpleNetworkDevice->TxDestAddr, 1);
        }

        gBS->FreePool (SimpleNetworkDevice);
      }

    }

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

//
// FIFO Support Functions
//
/**
  Judge whether transmit FIFO is full.

  @param Fifo Point to trasmit FIFO structure.
  
  @return BOOLEAN whether transmit FIFO is full.
**/
BOOLEAN
SimpleNetworkTransmitFifoFull (
  EFI_SIMPLE_NETWORK_DEV_FIFO *Fifo
  )
{
  if (((Fifo->Last + 1) % EFI_SIMPLE_NETWORK_MAX_TX_FIFO_SIZE) == Fifo->First) {
    return TRUE;
  }

  return FALSE;
}

/**
  Judge whether transmit FIFO is empty.

  @param Fifo Point to trasmit FIFO structure.
  
  @return BOOLEAN whether transmit FIFO is empty.
**/
BOOLEAN
SimpleNetworkTransmitFifoEmpty (
  EFI_SIMPLE_NETWORK_DEV_FIFO *Fifo
  )
{
  if (Fifo->Last == Fifo->First) {
    return TRUE;
  }

  return FALSE;
}


/**
  Add data into transmit buffer.

  @param Fifo Point to trasmit FIFO structure.
  @param Data The data point want to be added.
  
  @retval EFI_OUT_OF_RESOURCES  FIFO is full 
  @retval EFI_SUCCESS           Success operation. 
**/
EFI_STATUS
SimpleNetworkTransmitFifoAdd (
  EFI_SIMPLE_NETWORK_DEV_FIFO *Fifo,
  VOID                        *Data
  )
{
  if (SimpleNetworkTransmitFifoFull (Fifo)) {
    return EFI_OUT_OF_RESOURCES;
  }

  Fifo->Data[Fifo->Last]  = Data;
  Fifo->Last              = (Fifo->Last + 1) % EFI_SIMPLE_NETWORK_MAX_TX_FIFO_SIZE;
  return EFI_SUCCESS;
}

/**
  Get a data and remove it from network transmit FIFO.

  @param Fifo Point to trasmit FIFO structure.
  @param Data On return, point to the data point want to be got and removed.
  
  @retval EFI_OUT_OF_RESOURCES network transmit buffer is empty. 
  @retval EFI_SUCCESS           Success operation. 
**/
EFI_STATUS
SimpleNetworkTransmitFifoRemove (
  EFI_SIMPLE_NETWORK_DEV_FIFO *Fifo,
  VOID                        **Data
  )
{
  if (SimpleNetworkTransmitFifoEmpty (Fifo)) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Data       = Fifo->Data[Fifo->First];
  Fifo->First = (Fifo->First + 1) % EFI_SIMPLE_NETWORK_MAX_TX_FIFO_SIZE;
  return EFI_SUCCESS;
}

/**
  Get recive filter setting according to EFI mask value.

  @param ReceiveFilterSetting  filter setting EFI mask value.
  
  @return UINT16 Undi filter setting value.
**/
UINT16
Undi16GetPacketFilterSetting (
  UINTN ReceiveFilterSetting
  )
{
  UINT16  PktFilter;

  PktFilter = 0;
  if ((ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_UNICAST) != 0) {
    PktFilter |= FLTR_DIRECTED;
  }

  if ((ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) != 0) {
    PktFilter |= FLTR_DIRECTED;
  }

  if ((ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST) != 0) {
    PktFilter |= FLTR_BRDCST;
  }

  if ((ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) != 0) {
    PktFilter |= FLTR_PRMSCS;
  }

  if ((ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST) != 0) {
    PktFilter |= FLTR_PRMSCS;
    //
    // @bug : Do not know if this is right????
    //
  }
  //
  // @bug : What is FLTR_SRC_RTG?
  //
  return PktFilter;
}

/**
  Get filter setting from multi cast buffer .
   
  @param Mode           Point to mode structure.
  @param McastBuffer    The multi cast buffer 
  @param HwAddressSize  Size of filter value.
  
**/
VOID
Undi16GetMCastFilters (
  IN EFI_SIMPLE_NETWORK_MODE      *Mode,
  IN OUT PXENV_UNDI_MCAST_ADDR_T  *McastBuffer,
  IN UINTN                        HwAddressSize
  )
{
  UINTN Index;

  //
  // @bug : What if Mode->MCastFilterCount > MAXNUM_MCADDR?
  //
  McastBuffer->MCastAddrCount = (UINT16) Mode->MCastFilterCount;
  for (Index = 0; Index < MAXNUM_MCADDR; Index++) {
    if (Index < McastBuffer->MCastAddrCount) {
      CopyMem (&McastBuffer->MCastAddr[Index], &Mode->MCastFilter[Index], HwAddressSize);
    } else {
      ZeroMem (&McastBuffer->MCastAddr[Index], HwAddressSize);
    }
  }
}
//
// Load 16 bit UNDI Option ROM into memory
//
/**
  Loads the undi driver.

  @param SimpleNetworkDevice A pointer to EFI_SIMPLE_NETWORK_DEV data structure.
  
  @retval   EFI_SUCCESS   - Successfully loads undi driver.
  @retval   EFI_NOT_FOUND - Doesn't find undi driver or undi driver load failure.
**/
EFI_STATUS
Undi16SimpleNetworkLoadUndi (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice
  )
{
  EFI_STATUS                Status;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINTN                     RomAddress;
  PCI_EXPANSION_ROM_HEADER  *PciExpansionRomHeader;
  PCI_DATA_STRUCTURE        *PciDataStructure;
  PCI_TYPE00                Pci;
  
  if (!mCachedInt1A) {
    Status = CacheVectorAddress (0x1A);
    if (!EFI_ERROR (Status)) {
      mCachedInt1A = TRUE;    
    }
  }

  PciIo = SimpleNetworkDevice->PciIo;

  PciIo->Pci.Read (
               PciIo,
               EfiPciIoWidthUint32,
               0,
               sizeof (Pci) / sizeof (UINT32),
               &Pci
               );

  for (RomAddress = 0xc0000; RomAddress < 0xfffff; RomAddress += 0x800) {

    PciExpansionRomHeader = (PCI_EXPANSION_ROM_HEADER *) RomAddress;

    if (PciExpansionRomHeader->Signature != PCI_EXPANSION_ROM_HEADER_SIGNATURE) {
      continue;
    }

    DEBUG ((DEBUG_INIT, "Option ROM found at %X\n", RomAddress));

    //
    // If the pointer to the PCI Data Structure is invalid, no further images can be located. 
    // The PCI Data Structure must be DWORD aligned. 
    //
    if (PciExpansionRomHeader->PcirOffset == 0 ||
        (PciExpansionRomHeader->PcirOffset & 3) != 0 ||
        RomAddress + PciExpansionRomHeader->PcirOffset + sizeof (PCI_DATA_STRUCTURE) > 0x100000) {
      break;
    }

    PciDataStructure = (PCI_DATA_STRUCTURE *) (RomAddress + PciExpansionRomHeader->PcirOffset);

    if (PciDataStructure->Signature != PCI_DATA_STRUCTURE_SIGNATURE) {
      continue;
    }

    DEBUG ((DEBUG_INIT, "PCI Data Structure found at %X\n", PciDataStructure));

    if (PciDataStructure->VendorId != Pci.Hdr.VendorId || PciDataStructure->DeviceId != Pci.Hdr.DeviceId) {
      continue;
    }

    DEBUG (
        (DEBUG_INIT, 
         "PCI device with matchinng VendorId and DeviceId (%d,%d)\n",
         (UINTN) PciDataStructure->VendorId,
         (UINTN) PciDataStructure->DeviceId)
        );

    Status = LaunchBaseCode (SimpleNetworkDevice, RomAddress);

    if (!EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Unload 16 bit UNDI Option ROM from memory

  @param SimpleNetworkDevice A pointer to EFI_SIMPLE_NETWORK_DEV data structure.
  
  @return EFI_STATUS 
**/
EFI_STATUS
Undi16SimpleNetworkUnloadUndi (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice
  )
{
  if (SimpleNetworkDevice->UndiLoaderTable != NULL) {
    ZeroMem (SimpleNetworkDevice->UndiLoaderTable, SimpleNetworkDevice->UndiLoaderTablePages << EFI_PAGE_SHIFT);
    gBS->FreePages (
          (EFI_PHYSICAL_ADDRESS) (UINTN) SimpleNetworkDevice->UndiLoaderTable,
          SimpleNetworkDevice->UndiLoaderTablePages
          );
  }

  if (SimpleNetworkDevice->DestinationDataSegment != NULL) {
    ZeroMem (
      SimpleNetworkDevice->DestinationDataSegment,
      SimpleNetworkDevice->DestinationDataSegmentPages << EFI_PAGE_SHIFT
      );
    gBS->FreePages (
          (EFI_PHYSICAL_ADDRESS) (UINTN) SimpleNetworkDevice->DestinationDataSegment,
          SimpleNetworkDevice->DestinationDataSegmentPages
          );
  }

  if (SimpleNetworkDevice->DestinationStackSegment != NULL) {
    ZeroMem (
      SimpleNetworkDevice->DestinationStackSegment,
      SimpleNetworkDevice->DestinationStackSegmentPages << EFI_PAGE_SHIFT
      );
    gBS->FreePages (
          (EFI_PHYSICAL_ADDRESS) (UINTN) SimpleNetworkDevice->DestinationStackSegment,
          SimpleNetworkDevice->DestinationStackSegmentPages
          );
  }

  if (SimpleNetworkDevice->DestinationCodeSegment != NULL) {
    ZeroMem (
      SimpleNetworkDevice->DestinationCodeSegment,
      SimpleNetworkDevice->DestinationCodeSegmentPages << EFI_PAGE_SHIFT
      );
    gBS->FreePages (
          (EFI_PHYSICAL_ADDRESS) (UINTN) SimpleNetworkDevice->DestinationCodeSegment,
          SimpleNetworkDevice->DestinationCodeSegmentPages
          );
  }

  return EFI_SUCCESS;
}

/**
  Start the UNDI interface.

  @param SimpleNetworkDevice A pointer to EFI_SIMPLE_NETWORK_DEV data structure.
  @param Ax                  PCI address of Undi device.
  
  @retval EFI_DEVICE_ERROR Fail to start 16 bit UNDI ROM. 
  @retval Others           Status of start 16 bit UNDI ROM. 
**/
EFI_STATUS
Undi16SimpleNetworkStartUndi (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  UINT16                  Ax
  )
{
  EFI_STATUS          Status;
  PXENV_START_UNDI_T  Start;

  //
  // Call 16 bit UNDI ROM to start the network interface
  //
  //
  // @bug : What is this state supposed to be???
  //
  Start.Status  = INIT_PXE_STATUS;
  Start.Ax      = Ax;
  Start.Bx      = 0x0000;
  Start.Dx      = 0x0000;
  Start.Di      = 0x0000;
  Start.Es      = 0x0000;

  Status        = PxeStartUndi (SimpleNetworkDevice, &Start);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (Start.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  return Status;
}


/**
  Stop the UNDI interface

  @param SimpleNetworkDevice A pointer to EFI_SIMPLE_NETWORK_DEV data structure.
  
  @retval EFI_DEVICE_ERROR Fail to stop 16 bit UNDI ROM. 
  @retval Others           Status of stop 16 bit UNDI ROM. 
**/
EFI_STATUS
Undi16SimpleNetworkStopUndi (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice
  )
{
  EFI_STATUS        Status;
  PXENV_STOP_UNDI_T Stop;

  //
  // Call 16 bit UNDI ROM to start the network interface
  //
  Stop.Status = INIT_PXE_STATUS;

  Status      = PxeUndiStop (SimpleNetworkDevice, &Stop);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (Stop.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  Cleanup Unid network interface

  @param SimpleNetworkDevice A pointer to EFI_SIMPLE_NETWORK_DEV data structure.
  
  @retval EFI_DEVICE_ERROR Fail to cleanup 16 bit UNDI ROM. 
  @retval Others           Status of cleanup 16 bit UNDI ROM. 
**/
EFI_STATUS
Undi16SimpleNetworkCleanupUndi (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice
  )
{
  EFI_STATUS            Status;
  PXENV_UNDI_CLEANUP_T  Cleanup;

  //
  // Call 16 bit UNDI ROM to cleanup the network interface
  //
  Cleanup.Status  = INIT_PXE_STATUS;

  Status          = PxeUndiCleanup (SimpleNetworkDevice, &Cleanup);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (Cleanup.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  Get runtime information for Undi network interface

  @param This A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  
  @retval EFI_SUCCESS Sucess operation. 
  @retval Others      Fail to get runtime information for Undi network interface. 
**/
EFI_STATUS
Undi16SimpleNetworkGetInformation (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;
  UINTN                   Index;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkStarted:
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  default:
    return EFI_DEVICE_ERROR;
  }
  //
  // Call 16 bit UNDI ROM to start the network interface
  //
  ZeroMem (&SimpleNetworkDevice->GetInformation, sizeof (PXENV_UNDI_GET_INFORMATION_T));

  SimpleNetworkDevice->GetInformation.Status  = INIT_PXE_STATUS;

  Status = PxeUndiGetInformation (SimpleNetworkDevice, &SimpleNetworkDevice->GetInformation);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_NET, "  GetInformation.Status      = %d\n", SimpleNetworkDevice->GetInformation.Status));
  DEBUG ((DEBUG_NET, "  GetInformation.BaseIo      = %d\n", SimpleNetworkDevice->GetInformation.BaseIo));
  DEBUG ((DEBUG_NET, "  GetInformation.IntNumber   = %d\n", SimpleNetworkDevice->GetInformation.IntNumber));
  DEBUG ((DEBUG_NET, "  GetInformation.MaxTranUnit = %d\n", SimpleNetworkDevice->GetInformation.MaxTranUnit));
  DEBUG ((DEBUG_NET, "  GetInformation.HwType      = %d\n", SimpleNetworkDevice->GetInformation.HwType));
  DEBUG ((DEBUG_NET, "  GetInformation.HwAddrLen   = %d\n", SimpleNetworkDevice->GetInformation.HwAddrLen));
  DEBUG ((DEBUG_NET, "  GetInformation.ROMAddress  = %d\n", SimpleNetworkDevice->GetInformation.ROMAddress));
  DEBUG ((DEBUG_NET, "  GetInformation.RxBufCt     = %d\n", SimpleNetworkDevice->GetInformation.RxBufCt));
  DEBUG ((DEBUG_NET, "  GetInformation.TxBufCt     = %d\n", SimpleNetworkDevice->GetInformation.TxBufCt));

  DEBUG ((DEBUG_NET, "  GetInformation.CurNodeAddr ="));
  for (Index = 0; Index < 16; Index++) {
    DEBUG ((DEBUG_NET, "%02x ", SimpleNetworkDevice->GetInformation.CurrentNodeAddress[Index]));
  }

  DEBUG ((DEBUG_NET, "\n"));

  DEBUG ((DEBUG_NET, "  GetInformation.PermNodeAddr ="));
  for (Index = 0; Index < 16; Index++) {
    DEBUG ((DEBUG_NET, "%02x ", SimpleNetworkDevice->GetInformation.PermNodeAddress[Index]));
  }

  DEBUG ((DEBUG_NET, "\n"));

  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (SimpleNetworkDevice->GetInformation.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // The information has been retrieved.  Fill in Mode data.
  //
  SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize  = SimpleNetworkDevice->GetInformation.HwAddrLen;

  SimpleNetworkDevice->SimpleNetworkMode.MaxPacketSize  = SimpleNetworkDevice->GetInformation.MaxTranUnit;

  SimpleNetworkDevice->SimpleNetworkMode.IfType         = (UINT8) SimpleNetworkDevice->GetInformation.HwType;

  ZeroMem (
    &SimpleNetworkDevice->SimpleNetworkMode.CurrentAddress,
    sizeof SimpleNetworkDevice->SimpleNetworkMode.CurrentAddress
    );

  CopyMem (
    &SimpleNetworkDevice->SimpleNetworkMode.CurrentAddress,
    &SimpleNetworkDevice->GetInformation.CurrentNodeAddress,
    SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
    );

  ZeroMem (
    &SimpleNetworkDevice->SimpleNetworkMode.PermanentAddress,
    sizeof SimpleNetworkDevice->SimpleNetworkMode.PermanentAddress
    );

  CopyMem (
    &SimpleNetworkDevice->SimpleNetworkMode.PermanentAddress,
    &SimpleNetworkDevice->GetInformation.PermNodeAddress,
    SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
    );

  //
  // hard code broadcast address - not avail in PXE2.1
  //
  ZeroMem (
    &SimpleNetworkDevice->SimpleNetworkMode.BroadcastAddress,
    sizeof SimpleNetworkDevice->SimpleNetworkMode.BroadcastAddress
    );

  SetMem (
    &SimpleNetworkDevice->SimpleNetworkMode.BroadcastAddress,
    SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize,
    0xff
    );

  return Status;
}

/**
  Get NIC type

  @param This A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  
  @retval EFI_SUCCESS Sucess operation. 
  @retval Others      Fail to get NIC type.
**/
EFI_STATUS
Undi16SimpleNetworkGetNicType (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (&SimpleNetworkDevice->GetNicType, sizeof (PXENV_UNDI_GET_NIC_TYPE_T));

  SimpleNetworkDevice->GetNicType.Status  = INIT_PXE_STATUS;

  Status = PxeUndiGetNicType (SimpleNetworkDevice, &SimpleNetworkDevice->GetNicType);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_NET, "  GetNicType.Status      = %d\n", SimpleNetworkDevice->GetNicType.Status));
  DEBUG ((DEBUG_NET, "  GetNicType.NicType     = %d\n", SimpleNetworkDevice->GetNicType.NicType));
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (SimpleNetworkDevice->GetNicType.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // The information has been retrieved.  Fill in Mode data.
  //
  return Status;
}

/**
  Get NDIS information

  @param This A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  
  @retval EFI_SUCCESS Sucess operation. 
  @retval Others      Fail to get NDIS information.
**/
EFI_STATUS
Undi16SimpleNetworkGetNdisInfo (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (&SimpleNetworkDevice->GetNdisInfo, sizeof (PXENV_UNDI_GET_NDIS_INFO_T));

  SimpleNetworkDevice->GetNdisInfo.Status = INIT_PXE_STATUS;

  Status = PxeUndiGetNdisInfo (SimpleNetworkDevice, &SimpleNetworkDevice->GetNdisInfo);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_NET, "  GetNdisInfo.Status       = %d\n", SimpleNetworkDevice->GetNdisInfo.Status));
  DEBUG ((DEBUG_NET, "  GetNdisInfo.IfaceType    = %a\n", SimpleNetworkDevice->GetNdisInfo.IfaceType));
  DEBUG ((DEBUG_NET, "  GetNdisInfo.LinkSpeed    = %d\n", SimpleNetworkDevice->GetNdisInfo.LinkSpeed));
  DEBUG ((DEBUG_NET, "  GetNdisInfo.ServiceFlags = %08x\n", SimpleNetworkDevice->GetNdisInfo.ServiceFlags));

  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (SimpleNetworkDevice->GetNdisInfo.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // The information has been retrieved.  Fill in Mode data.
  //
  return Status;
}

/**
  Call Undi ROM 16bit ISR() to check interrupt cause.

  @param This               A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @param FrameLength        The length of frame buffer.
  @param FrameHeaderLength  The length of frame buffer's header if has.
  @param Frame              The frame buffer to process network interrupt.
  @param ProtType           The type network transmit protocol
  @param PktType            The type of package.
  
  @retval EFI_DEVICE_ERROR  Fail to execute 16 bit ROM's ISR, or status is invalid. 
  @retval EFI_SUCCESS       Success operation. 
**/
EFI_STATUS
Undi16SimpleNetworkIsr (
  IN EFI_SIMPLE_NETWORK_PROTOCOL * This,
  IN UINTN                       *FrameLength,
  IN UINTN                       *FrameHeaderLength, OPTIONAL
  IN UINT8                       *Frame, OPTIONAL
  IN UINT8                       *ProtType, OPTIONAL
  IN UINT8                       *PktType OPTIONAL
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;
  BOOLEAN                 FrameReceived;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }

  FrameReceived = FALSE;

  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkStarted:
  default:
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((DEBUG_NET, "Isr() IsrValid = %d\n", SimpleNetworkDevice->IsrValid));

  if (!SimpleNetworkDevice->IsrValid) {
    //
    // Call 16 bit UNDI ROM to open the network interface
    //
    ZeroMem (&SimpleNetworkDevice->Isr, sizeof (PXENV_UNDI_ISR_T));
    SimpleNetworkDevice->Isr.Status   = INIT_PXE_STATUS;
    SimpleNetworkDevice->Isr.FuncFlag = PXENV_UNDI_ISR_IN_START;

    DEBUG ((DEBUG_NET, "Isr() START\n"));

    Status = PxeUndiIsr (SimpleNetworkDevice, &SimpleNetworkDevice->Isr);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Check the status code from the 16 bit UNDI ROM
    //
    if (SimpleNetworkDevice->Isr.Status != PXENV_STATUS_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
    //
    // There have been no events on this UNDI interface, so return EFI_NOT_READY
    //
    if (SimpleNetworkDevice->Isr.FuncFlag == PXENV_UNDI_ISR_OUT_NOT_OURS) {
      return EFI_SUCCESS;
    }
    //
    // There is data to process, so call until all events processed.
    //
    ZeroMem (&SimpleNetworkDevice->Isr, sizeof (PXENV_UNDI_ISR_T));
    SimpleNetworkDevice->Isr.Status   = INIT_PXE_STATUS;
    SimpleNetworkDevice->Isr.FuncFlag = PXENV_UNDI_ISR_IN_PROCESS;

    DEBUG ((DEBUG_NET, "Isr() PROCESS\n"));

    Status = PxeUndiIsr (SimpleNetworkDevice, &SimpleNetworkDevice->Isr);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    SimpleNetworkDevice->IsrValid = TRUE;
  }
  //
  // Call UNDI GET_NEXT until DONE
  //
  while (SimpleNetworkDevice->Isr.FuncFlag != PXENV_UNDI_ISR_OUT_DONE) {
    //
    // Check the status code from the 16 bit UNDI ROM
    //
    if (SimpleNetworkDevice->Isr.Status != PXENV_STATUS_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
    //
    // UNDI is busy.  Caller will have to call again.
    // This should never happen with a polled mode driver.
    //
    if (SimpleNetworkDevice->Isr.FuncFlag == PXENV_UNDI_ISR_OUT_BUSY) {
      DEBUG ((DEBUG_NET, "  BUSY\n"));
      return EFI_SUCCESS;
    }
    //
    // Check for invalud UNDI FuncFlag
    //
    if (SimpleNetworkDevice->Isr.FuncFlag != PXENV_UNDI_ISR_OUT_RECEIVE &&
        SimpleNetworkDevice->Isr.FuncFlag != PXENV_UNDI_ISR_OUT_TRANSMIT
        ) {
      DEBUG ((DEBUG_NET, "  Invalid SimpleNetworkDevice->Isr.FuncFlag value %d\n", SimpleNetworkDevice->Isr.FuncFlag));
      return EFI_DEVICE_ERROR;
    }
    //
    // Check for Transmit Event
    //
    if (SimpleNetworkDevice->Isr.FuncFlag == PXENV_UNDI_ISR_OUT_TRANSMIT) {
      DEBUG ((DEBUG_NET, "  TRANSMIT\n"));
      SimpleNetworkDevice->InterruptStatus |= EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;
    }
    //
    // Check for Receive Event
    //
    else if (SimpleNetworkDevice->Isr.FuncFlag == PXENV_UNDI_ISR_OUT_RECEIVE) {
      //
      // note - this code will hang on a receive interrupt in a GetStatus loop
      //
      DEBUG ((DEBUG_NET, "  RECEIVE\n"));
      SimpleNetworkDevice->InterruptStatus |= EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT;

      DEBUG ((DEBUG_NET, "SimpleNetworkDevice->Isr.BufferLength      = %d\n", SimpleNetworkDevice->Isr.BufferLength));
      DEBUG ((DEBUG_NET, "SimpleNetworkDevice->Isr.FrameLength       = %d\n", SimpleNetworkDevice->Isr.FrameLength));
      DEBUG ((DEBUG_NET, "SimpleNetworkDevice->Isr.FrameHeaderLength = %d\n", SimpleNetworkDevice->Isr.FrameHeaderLength));
      DEBUG (
        (
        DEBUG_NET, "SimpleNetworkDevice->Isr.Frame             = %04x:%04x\n", SimpleNetworkDevice->Isr.FrameSegSel,
        SimpleNetworkDevice->Isr.FrameOffset
        )
        );
      DEBUG ((DEBUG_NET, "SimpleNetworkDevice->Isr.ProtType          = 0x%02x\n", SimpleNetworkDevice->Isr.BufferLength));
      DEBUG ((DEBUG_NET, "SimpleNetworkDevice->Isr.PktType           = 0x%02x\n", SimpleNetworkDevice->Isr.BufferLength));

      if (FrameReceived) {
        return EFI_SUCCESS;
      }

      if ((Frame == NULL) || (SimpleNetworkDevice->Isr.FrameLength > *FrameLength)) {
        DEBUG ((DEBUG_NET, "return EFI_BUFFER_TOO_SMALL   *FrameLength = %08x\n", *FrameLength));
        *FrameLength = SimpleNetworkDevice->Isr.FrameLength;
        return EFI_BUFFER_TOO_SMALL;
      }

      *FrameLength = SimpleNetworkDevice->Isr.FrameLength;
      if (FrameHeaderLength != NULL) {
        *FrameHeaderLength = SimpleNetworkDevice->Isr.FrameHeaderLength;
      }

      if (ProtType != NULL) {
        *ProtType = SimpleNetworkDevice->Isr.ProtType;
      }

      if (PktType != NULL) {
        *PktType = SimpleNetworkDevice->Isr.PktType;
      }

      CopyMem (
        Frame,
        (VOID *)(UINTN) ((SimpleNetworkDevice->Isr.FrameSegSel << 4) + SimpleNetworkDevice->Isr.FrameOffset),
        SimpleNetworkDevice->Isr.BufferLength
        );
      Frame = Frame + SimpleNetworkDevice->Isr.BufferLength;
      if (SimpleNetworkDevice->Isr.BufferLength == SimpleNetworkDevice->Isr.FrameLength) {
        FrameReceived = TRUE;
      }
    }
    //
    // There is data to process, so call until all events processed.
    //
    ZeroMem (&SimpleNetworkDevice->Isr, sizeof (PXENV_UNDI_ISR_T));
    SimpleNetworkDevice->Isr.Status   = INIT_PXE_STATUS;
    SimpleNetworkDevice->Isr.FuncFlag = PXENV_UNDI_ISR_IN_GET_NEXT;

    DEBUG ((DEBUG_NET, "Isr() GET NEXT\n"));

    Status = PxeUndiIsr (SimpleNetworkDevice, &SimpleNetworkDevice->Isr);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Check the status code from the 16 bit UNDI ROM
    //
    //        if (SimpleNetworkDevice->Isr.Status != PXENV_STATUS_SUCCESS) {
    //            return EFI_DEVICE_ERROR;
    //        }
    //
  }

  SimpleNetworkDevice->IsrValid = FALSE;
  return EFI_SUCCESS;
}
//
// ///////////////////////////////////////////////////////////////////////////////////////
// Simple Network Protocol Interface Functions using 16 bit UNDI Option ROMs
/////////////////////////////////////////////////////////////////////////////////////////
//
// Start()
//
/**
  Call 16 bit UNDI ROM to start the network interface

  @param This       A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  
  @retval EFI_DEVICE_ERROR Network interface has not be initialized.
  @retval EFI_DEVICE_ERROR Fail to execute 16 bit ROM call.
  @retval EFI_SUCESS       Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkStart (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;
  PXENV_UNDI_STARTUP_T    Startup;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkStopped:
    break;

  case EfiSimpleNetworkStarted:
  case EfiSimpleNetworkInitialized:
    return EFI_ALREADY_STARTED;

  default:
    return EFI_DEVICE_ERROR;
  }
  //
  // Call 16 bit UNDI ROM to start the network interface
  //
  Startup.Status  = INIT_PXE_STATUS;

  Status          = PxeUndiStartup (SimpleNetworkDevice, &Startup);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (Startup.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // The UNDI interface has been started, so update the State.
  //
  SimpleNetworkDevice->SimpleNetworkMode.State = EfiSimpleNetworkStarted;

  //
  //
  //
  SimpleNetworkDevice->SimpleNetworkMode.ReceiveFilterSetting = 0;
  SimpleNetworkDevice->SimpleNetworkMode.MCastFilterCount     = 0;

  return Status;
}
//
// Stop()
//
/**
  Call 16 bit UNDI ROM to stop the network interface

  @param This       A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  
  @retval EFI_DEVICE_ERROR Network interface has not be initialized.
  @retval EFI_DEVICE_ERROR Fail to execute 16 bit ROM call.
  @retval EFI_SUCESS       Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkStop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkStarted:
    break;

  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkInitialized:
  default:
    return EFI_DEVICE_ERROR;
  }

  SimpleNetworkDevice->SimpleNetworkMode.State = EfiSimpleNetworkStopped;

  return Status;
}

//
// Initialize()
//
/**
  Initialize network interface 

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @param ExtraRxBufferSize    The size of extra request receive buffer.
  @param ExtraTxBufferSize    The size of extra request transmit buffer.
 
  @retval EFI_DEVICE_ERROR Fail to execute 16 bit ROM call.
  @retval EFI_SUCESS       Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkInitialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL            *This,
  IN UINTN                                  ExtraRxBufferSize  OPTIONAL,
  IN UINTN                                  ExtraTxBufferSize  OPTIONAL
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;
  PXENV_UNDI_INITIALIZE_T Initialize;
  PXENV_UNDI_OPEN_T       Open;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkStarted:
    break;

  case EfiSimpleNetworkInitialized:
  default:
    return EFI_DEVICE_ERROR;
  }
  //
  // Call 16 bit UNDI ROM to start the network interface
  //
  Initialize.Status       = INIT_PXE_STATUS;
  Initialize.ProtocolIni  = 0;

  Status                  = PxeUndiInitialize (SimpleNetworkDevice, &Initialize);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR : PxeUndiInitialize() - Status = %r\n", Status));
    DEBUG ((DEBUG_ERROR, "Initialize.Status == %xh\n", Initialize.Status));

    if (Initialize.Status == PXENV_STATUS_UNDI_MEDIATEST_FAILED) {
      Status = EFI_NO_MEDIA;
    }

    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (Initialize.Status != PXENV_STATUS_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "ERROR : PxeUndiInitialize() - Initialize.Status = %04x\n", Initialize.Status));
    return EFI_DEVICE_ERROR;
  }
  //
  // Call 16 bit UNDI ROM to open the network interface
  //
  Open.Status     = INIT_PXE_STATUS;
  Open.OpenFlag   = 0;
  Open.PktFilter  = Undi16GetPacketFilterSetting (SimpleNetworkDevice->SimpleNetworkMode.ReceiveFilterSetting);
  Undi16GetMCastFilters (
    &SimpleNetworkDevice->SimpleNetworkMode,
    &Open.McastBuffer,
    SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
    );

  Status = PxeUndiOpen (SimpleNetworkDevice, &Open);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR : PxeUndiOpen() - Status = %r\n", Status));
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (Open.Status != PXENV_STATUS_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "ERROR : PxeUndiOpen() - Open.Status = %04x\n", Open.Status));
    return EFI_DEVICE_ERROR;
  }
  //
  // The UNDI interface has been initialized, so update the State.
  //
  SimpleNetworkDevice->SimpleNetworkMode.State = EfiSimpleNetworkInitialized;

  //
  // If initialize succeeds, then assume that media is present.
  //
  SimpleNetworkDevice->SimpleNetworkMode.MediaPresent = TRUE;

  //
  // Reset the recycled transmit buffer FIFO
  //
  SimpleNetworkDevice->TxBufferFifo.First = 0;
  SimpleNetworkDevice->TxBufferFifo.Last  = 0;
  SimpleNetworkDevice->IsrValid           = FALSE;

  return Status;
}
//
// Reset()
//
/**
  Reset network interface.

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @param ExtendedVerification Need extended verfication.
  
  @retval EFI_INVALID_PARAMETER Invalid This paramter.
  @retval EFI_DEVICE_ERROR      Network device has not been initialized.
  @retval EFI_NOT_STARTED       Network device has been stopped.
  @retval EFI_DEVICE_ERROR      Invalid status for network device
  @retval EFI_SUCCESS           Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkReset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL   *This,
  IN BOOLEAN                       ExtendedVerification
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;
  PXENV_UNDI_RESET_T      Reset;
  UINT16                  Rx_filter;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStarted:
  default:
    return EFI_DEVICE_ERROR;
  }

  Reset.Status  = INIT_PXE_STATUS;

  Rx_filter     = Undi16GetPacketFilterSetting (SimpleNetworkDevice->SimpleNetworkMode.ReceiveFilterSetting);

  Undi16GetMCastFilters (
    &SimpleNetworkDevice->SimpleNetworkMode,
    &Reset.R_Mcast_Buf,
    SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
    );

  Status = PxeUndiResetNic (SimpleNetworkDevice, &Reset, Rx_filter);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (Reset.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Reset the recycled transmit buffer FIFO
  //
  SimpleNetworkDevice->TxBufferFifo.First = 0;
  SimpleNetworkDevice->TxBufferFifo.Last  = 0;
  SimpleNetworkDevice->IsrValid           = FALSE;

  return Status;
}
//
// Shutdown()
//
/**
  Shutdown network interface.

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  
  @retval EFI_INVALID_PARAMETER Invalid This paramter.
  @retval EFI_DEVICE_ERROR      Network device has not been initialized.
  @retval EFI_NOT_STARTED       Network device has been stopped.
  @retval EFI_DEVICE_ERROR      Invalid status for network device
  @retval EFI_SUCCESS           Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkShutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;
  PXENV_UNDI_CLOSE_T      Close;
  PXENV_UNDI_SHUTDOWN_T   Shutdown;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStarted:
  default:
    return EFI_DEVICE_ERROR;
  }

  SimpleNetworkDevice->IsrValid = FALSE;

  //
  // Call 16 bit UNDI ROM to start the network interface
  //
  Close.Status  = INIT_PXE_STATUS;

  Status        = PxeUndiClose (SimpleNetworkDevice, &Close);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (Close.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Call 16 bit UNDI ROM to open the network interface
  //
  Shutdown.Status = INIT_PXE_STATUS;

  Status          = PxeUndiShutdown (SimpleNetworkDevice, &Shutdown);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (Shutdown.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // The UNDI interface has been initialized, so update the State.
  //
  SimpleNetworkDevice->SimpleNetworkMode.State = EfiSimpleNetworkStarted;

  //
  // If shutdown succeeds, then assume that media is not present.
  //
  SimpleNetworkDevice->SimpleNetworkMode.MediaPresent = FALSE;

  //
  // Reset the recycled transmit buffer FIFO
  //
  SimpleNetworkDevice->TxBufferFifo.First = 0;
  SimpleNetworkDevice->TxBufferFifo.Last  = 0;

  //
  // A short delay.  Without this an initialize immediately following
  // a shutdown will cause some versions of UNDI-16 to stop operating.
  //
  gBS->Stall (250000);

  return Status;
}
//
// ReceiveFilters()
//
/**
  Reset network interface.

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @param Enable               Enable mask value
  @param Disable              Disable mask value
  @param ResetMCastFilter     Whether reset multi cast filter or not
  @param MCastFilterCnt       Count of mutli cast filter for different MAC address
  @param MCastFilter          Buffer for mustli cast filter for different MAC address.
  
  @retval EFI_INVALID_PARAMETER Invalid This paramter.
  @retval EFI_DEVICE_ERROR      Network device has not been initialized.
  @retval EFI_NOT_STARTED       Network device has been stopped.
  @retval EFI_DEVICE_ERROR      Invalid status for network device
  @retval EFI_SUCCESS           Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL                     * This,
  IN UINT32                                          Enable,
  IN UINT32                                          Disable,
  IN BOOLEAN                                         ResetMCastFilter,
  IN UINTN                                           MCastFilterCnt     OPTIONAL,
  IN EFI_MAC_ADDRESS                                 * MCastFilter OPTIONAL
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  UINT32                  NewFilter;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;
  PXENV_UNDI_CLOSE_T      Close;
  PXENV_UNDI_OPEN_T       Open;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStarted:
  default:
    return EFI_DEVICE_ERROR;
  }
  //
  // First deal with possible filter setting changes
  //
  if ((Enable == 0) && (Disable == 0) && !ResetMCastFilter) {
    return EFI_SUCCESS;
  }

  NewFilter = (SimpleNetworkDevice->SimpleNetworkMode.ReceiveFilterSetting | Enable) &~Disable;

  if ((NewFilter & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) != 0) {
    if ((MCastFilterCnt == 0) || (MCastFilter == 0) || MCastFilterCnt > SimpleNetworkDevice->SimpleNetworkMode.MaxMCastFilterCount) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Call 16 bit UNDI ROM to close the network interface
  //
  Close.Status  = INIT_PXE_STATUS;

  Status        = PxeUndiClose (SimpleNetworkDevice, &Close);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (Close.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Call 16 bit UNDI ROM to open the network interface
  //
  //
  // Reset the recycled transmit buffer FIFO
  //
  SimpleNetworkDevice->TxBufferFifo.First = 0;
  SimpleNetworkDevice->TxBufferFifo.Last  = 0;

  //
  // Call 16 bit UNDI ROM to open the network interface
  //
  ZeroMem (&Open, sizeof Open);

  Open.Status     = INIT_PXE_STATUS;
  Open.PktFilter  = Undi16GetPacketFilterSetting (NewFilter);

  if ((NewFilter & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) != 0) {
    //
    // Copy the MAC addresses into the UNDI open parameter structure
    //
    Open.McastBuffer.MCastAddrCount = (UINT16) MCastFilterCnt;
    for (Index = 0; Index < MCastFilterCnt; ++Index) {
      CopyMem (
        Open.McastBuffer.MCastAddr[Index],
        &MCastFilter[Index],
        sizeof Open.McastBuffer.MCastAddr[Index]
        );
    }
  } else if (!ResetMCastFilter) {
    for (Index = 0; Index < SimpleNetworkDevice->SimpleNetworkMode.MCastFilterCount; ++Index) {
      CopyMem (
        Open.McastBuffer.MCastAddr[Index],
        &SimpleNetworkDevice->SimpleNetworkMode.MCastFilter[Index],
        sizeof Open.McastBuffer.MCastAddr[Index]
        );
    }
  }

  Status = PxeUndiOpen (SimpleNetworkDevice, &Open);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (Open.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  SimpleNetworkDevice->IsrValid = FALSE;
  SimpleNetworkDevice->SimpleNetworkMode.ReceiveFilterSetting = NewFilter;

  if ((NewFilter & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST) != 0) {
    SimpleNetworkDevice->SimpleNetworkMode.MCastFilterCount = (UINT32) MCastFilterCnt;
    for (Index = 0; Index < MCastFilterCnt; ++Index) {
      CopyMem (
        &SimpleNetworkDevice->SimpleNetworkMode.MCastFilter[Index],
        &MCastFilter[Index],
        sizeof (EFI_MAC_ADDRESS)
        );      
    }
  }
  //
  // Read back multicast addresses.
  //
  return EFI_SUCCESS;
}
//
// StationAddress()
//
/**
  Set new MAC address.

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @param Reset                Whether reset station MAC address to permenent address
  @param New                  A pointer to New address
  
  @retval EFI_INVALID_PARAMETER Invalid This paramter.
  @retval EFI_DEVICE_ERROR      Network device has not been initialized.
  @retval EFI_NOT_STARTED       Network device has been stopped.
  @retval EFI_DEVICE_ERROR      Invalid status for network device
  @retval EFI_SUCCESS           Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkStationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL           * This,
  IN BOOLEAN                               Reset,
  IN EFI_MAC_ADDRESS                       * New OPTIONAL
  )
{
  EFI_STATUS                    Status;
  EFI_SIMPLE_NETWORK_DEV        *SimpleNetworkDevice;
  PXENV_UNDI_SET_STATION_ADDR_T SetStationAddr;
  //
  // EFI_DEVICE_PATH_PROTOCOL     *OldDevicePath;
  //
  PXENV_UNDI_CLOSE_T            Close;
  PXENV_UNDI_OPEN_T             Open;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;

  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkStarted:
  default:
    return EFI_DEVICE_ERROR;
  }
  //
  // Call 16 bit UNDI ROM to open the network interface
  //
  SetStationAddr.Status = INIT_PXE_STATUS;

  if (Reset) {
    //
    // If we are reseting the Station Address to the permanent address, and the
    // Station Address is not programmable, then just return EFI_SUCCESS.
    //
    if (!SimpleNetworkDevice->SimpleNetworkMode.MacAddressChangeable) {
      return EFI_SUCCESS;
    }
    //
    // If the address is already the permanent address, then just return success.
    //
    if (CompareMem (
          &SimpleNetworkDevice->SimpleNetworkMode.CurrentAddress,
          &SimpleNetworkDevice->SimpleNetworkMode.PermanentAddress,
          SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
          ) == 0) {
      return EFI_SUCCESS;
    }
    //
    // Copy the adapters permanent address to the new station address
    //
    CopyMem (
      &SetStationAddr.StationAddress,
      &SimpleNetworkDevice->SimpleNetworkMode.PermanentAddress,
      SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
      );
  } else {
    //
    // If we are setting the Station Address, and the
    // Station Address is not programmable, return invalid parameter.
    //
    if (!SimpleNetworkDevice->SimpleNetworkMode.MacAddressChangeable) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // If the address is already the new address, then just return success.
    //
    if (CompareMem (
          &SimpleNetworkDevice->SimpleNetworkMode.CurrentAddress,
          New,
          SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
          ) == 0) {
      return EFI_SUCCESS;
    }
    //
    // Copy New to the new station address
    //
    CopyMem (
      &SetStationAddr.StationAddress,
      New,
      SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
      );

  }
  //
  // Call 16 bit UNDI ROM to stop the network interface
  //
  Close.Status = INIT_PXE_STATUS;

  PxeUndiClose (SimpleNetworkDevice, &Close);

  //
  // Call 16-bit UNDI ROM to set the station address
  //
  SetStationAddr.Status = PXENV_STATUS_SUCCESS;

  Status                = PxeUndiSetStationAddr (SimpleNetworkDevice, &SetStationAddr);

  //
  // Call 16-bit UNDI ROM to start the network interface
  //
  Open.Status     = PXENV_STATUS_SUCCESS;
  Open.OpenFlag   = 0;
  Open.PktFilter  = Undi16GetPacketFilterSetting (SimpleNetworkDevice->SimpleNetworkMode.ReceiveFilterSetting);
  Undi16GetMCastFilters (
    &SimpleNetworkDevice->SimpleNetworkMode,
    &Open.McastBuffer,
    SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
    );

  PxeUndiOpen (SimpleNetworkDevice, &Open);

  //
  // Check status from station address change
  //
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (SetStationAddr.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  CopyMem (
    &SimpleNetworkDevice->SimpleNetworkMode.CurrentAddress,
    &SetStationAddr.StationAddress,
    SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
    );

#if 0 /* The device path is based on the permanent address not the current address. */
  //
  // The station address was changed, so update the device path with the new MAC address.
  //
  OldDevicePath                   = SimpleNetworkDevice->DevicePath;
  SimpleNetworkDevice->DevicePath = DuplicateDevicePath (SimpleNetworkDevice->BaseDevicePath);
  SimpleNetworkAppendMacAddressDevicePath (
    &SimpleNetworkDevice->DevicePath,
    &SimpleNetworkDevice->SimpleNetworkMode.CurrentAddress
    );

  Status = LibReinstallProtocolInterfaces (
            SimpleNetworkDevice->Handle,
            &DevicePathProtocol,
            OldDevicePath,
            SimpleNetworkDevice->DevicePath,
            NULL
            );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to reinstall the DevicePath protocol for the Simple Network Device\n"));
    DEBUG ((DEBUG_ERROR, "  Status = %r\n", Status));
  }

  FreePool (OldDevicePath);
#endif /* 0 */

  return Status;
}
//
// Statistics()
//
/**
  Resets or collects the statistics on a network interface.

  @param  This            Protocol instance pointer.
  @param  Reset           Set to TRUE to reset the statistics for the network interface.
  @param  StatisticsSize  On input the size, in bytes, of StatisticsTable. On
                          output the size, in bytes, of the resulting table of
                          statistics.
  @param  StatisticsTable A pointer to the EFI_NETWORK_STATISTICS structure that
                          contains the statistics.

  @retval EFI_SUCCESS           The statistics were collected from the network interface.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_BUFFER_TOO_SMALL  The Statistics buffer was too small. The current buffer
                                size needed to hold the statistics is returned in
                                StatisticsSize.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkStatistics (
  IN EFI_SIMPLE_NETWORK_PROTOCOL       * This,
  IN BOOLEAN                           Reset,
  IN OUT UINTN                         *StatisticsSize OPTIONAL,
  OUT EFI_NETWORK_STATISTICS           * StatisticsTable OPTIONAL
  )
{
  EFI_STATUS                    Status;
  EFI_SIMPLE_NETWORK_DEV        *SimpleNetworkDevice;
  PXENV_UNDI_CLEAR_STATISTICS_T ClearStatistics;
  PXENV_UNDI_GET_STATISTICS_T   GetStatistics;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkStarted:
  default:
    return EFI_DEVICE_ERROR;
  }

  if ((StatisticsSize != NULL) && (*StatisticsSize != 0) && (StatisticsTable == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If Reset is TRUE, then clear all the statistics.
  //
  if (Reset) {

    DEBUG ((DEBUG_NET, "  RESET Statistics\n"));

    //
    // Call 16 bit UNDI ROM to open the network interface
    //
    ClearStatistics.Status  = INIT_PXE_STATUS;

    Status                  = PxeUndiClearStatistics (SimpleNetworkDevice, &ClearStatistics);

    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Check the status code from the 16 bit UNDI ROM
    //
    if (ClearStatistics.Status != PXENV_STATUS_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }

    DEBUG ((DEBUG_NET, "  RESET Statistics Complete"));
  }

  if (StatisticsSize != NULL) {
    EFI_NETWORK_STATISTICS  LocalStatisticsTable;

    DEBUG ((DEBUG_NET, "  GET Statistics\n"));

    //
    // If the size if valid, then see if the table is valid
    //
    if (StatisticsTable == NULL) {
      DEBUG ((DEBUG_NET, "  StatisticsTable is NULL\n"));
      return EFI_INVALID_PARAMETER;
    }
    //
    // Call 16 bit UNDI ROM to open the network interface
    //
    GetStatistics.Status            = INIT_PXE_STATUS;
    GetStatistics.XmtGoodFrames     = 0;
    GetStatistics.RcvGoodFrames     = 0;
    GetStatistics.RcvCRCErrors      = 0;
    GetStatistics.RcvResourceErrors = 0;

    Status                          = PxeUndiGetStatistics (SimpleNetworkDevice, &GetStatistics);

    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Check the status code from the 16 bit UNDI ROM
    //
    if (GetStatistics.Status != PXENV_STATUS_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
    //
    // Fill in the Statistics Table with the collected values.
    //
    SetMem (&LocalStatisticsTable, sizeof LocalStatisticsTable, 0xff);

    LocalStatisticsTable.TxGoodFrames     = GetStatistics.XmtGoodFrames;
    LocalStatisticsTable.RxGoodFrames     = GetStatistics.RcvGoodFrames;
    LocalStatisticsTable.RxCrcErrorFrames = GetStatistics.RcvCRCErrors;
    LocalStatisticsTable.RxDroppedFrames  = GetStatistics.RcvResourceErrors;

    CopyMem (StatisticsTable, &LocalStatisticsTable, *StatisticsSize);

    DEBUG (
      (DEBUG_NET,
      "  Statistics Collected : Size=%d  Buf=%08x\n",
      *StatisticsSize,
      StatisticsTable)
      );

    DEBUG ((DEBUG_NET, "  GET Statistics Complete"));

    if (*StatisticsSize < sizeof LocalStatisticsTable) {
      DEBUG ((DEBUG_NET, "  BUFFER TOO SMALL\n"));
      Status = EFI_BUFFER_TOO_SMALL;
    }

    *StatisticsSize = sizeof LocalStatisticsTable;

    return Status;

  }

  return EFI_SUCCESS;
}
//
// MCastIpToMac()
//
/**
  Translate IP address to MAC address.

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @param IPv6                 IPv6 or IPv4
  @param IP                   A pointer to given Ip address.
  @param MAC                  On return, translated MAC address.
  
  @retval EFI_INVALID_PARAMETER Invalid This paramter.
  @retval EFI_INVALID_PARAMETER Invalid IP address.
  @retval EFI_INVALID_PARAMETER Invalid return buffer for holding MAC address.
  @retval EFI_UNSUPPORTED       Do not support IPv6 
  @retval EFI_DEVICE_ERROR      Network device has not been initialized.
  @retval EFI_NOT_STARTED       Network device has been stopped.
  @retval EFI_DEVICE_ERROR      Invalid status for network device
  @retval EFI_SUCCESS           Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkMCastIpToMac (
  IN EFI_SIMPLE_NETWORK_PROTOCOL            *This,
  IN BOOLEAN                                IPv6,
  IN EFI_IP_ADDRESS                         *IP,
  OUT EFI_MAC_ADDRESS                       *MAC
  )
{
  EFI_STATUS                  Status;
  EFI_SIMPLE_NETWORK_DEV      *SimpleNetworkDevice;
  PXENV_UNDI_GET_MCAST_ADDR_T GetMcastAddr;

  if (This == NULL || IP == NULL || MAC == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStarted:
  default:
    return EFI_DEVICE_ERROR;
  }
  //
  // 16 bit UNDI Option ROMS do not support IPv6.  Check for IPv6 usage.
  //
  if (IPv6) {
    return EFI_UNSUPPORTED;
  }
  //
  // Call 16 bit UNDI ROM to open the network interface
  //
  GetMcastAddr.Status = INIT_PXE_STATUS;
  CopyMem (&GetMcastAddr.InetAddr, IP, 4);

  Status = PxeUndiGetMcastAddr (SimpleNetworkDevice, &GetMcastAddr);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  if (GetMcastAddr.Status != PXENV_STATUS_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Copy the MAC address from the returned data structure.
  //
  CopyMem (
    MAC,
    &GetMcastAddr.MediaAddr,
    SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
    );

  return Status;
}
//
// NvData()
//
/**
  Performs read and write operations on the NVRAM device attached to a 
  network interface.

  @param  This       The protocol instance pointer.
  @param  ReadWrite  TRUE for read operations, FALSE for write operations.
  @param  Offset     Byte offset in the NVRAM device at which to start the read or
                     write operation. This must be a multiple of NvRamAccessSize and
                     less than NvRamSize.
  @param  BufferSize The number of bytes to read or write from the NVRAM device.
                     This must also be a multiple of NvramAccessSize.
  @param  Buffer     A pointer to the data buffer.

  @retval EFI_SUCCESS           The NVRAM access was performed.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkNvData (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      ReadWrite,
  IN UINTN                        Offset,
  IN UINTN                        BufferSize,
  IN OUT VOID                     *Buffer
  )
{
  return EFI_UNSUPPORTED;
}
//
// GetStatus()
//
/**
  Reads the current interrupt status and recycled transmit buffer status from 
  a network interface.

  @param  This            The protocol instance pointer.
  @param  InterruptStatus A pointer to the bit mask of the currently active interrupts
                          If this is NULL, the interrupt status will not be read from
                          the device. If this is not NULL, the interrupt status will
                          be read from the device. When the  interrupt status is read,
                          it will also be cleared. Clearing the transmit  interrupt
                          does not empty the recycled transmit buffer array.
  @param  TxBuf           Recycled transmit buffer address. The network interface will
                          not transmit if its internal recycled transmit buffer array
                          is full. Reading the transmit buffer does not clear the
                          transmit interrupt. If this is NULL, then the transmit buffer
                          status will not be read. If there are no transmit buffers to
                          recycle and TxBuf is not NULL, * TxBuf will be set to NULL.

  @retval EFI_SUCCESS           The status of the network interface was retrieved.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkGetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  * This,
  OUT UINT32                      *InterruptStatus OPTIONAL,
  OUT VOID                        **TxBuf OPTIONAL
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;
  UINTN                   FrameLength;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkStarted:
  default:
    return EFI_DEVICE_ERROR;
  }

  if (InterruptStatus == NULL && TxBuf == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FrameLength = 0;
  Status      = Undi16SimpleNetworkIsr (This, &FrameLength, NULL, NULL, NULL, NULL);

  if (Status != EFI_BUFFER_TOO_SMALL) {
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  //
  // See if the caller wants interrupt info.
  //
  if (InterruptStatus != NULL) {
    *InterruptStatus                      = SimpleNetworkDevice->InterruptStatus;
    SimpleNetworkDevice->InterruptStatus  = 0;
  }
  //
  // See if the caller wants transmit buffer status info.
  //
  if (TxBuf != NULL) {
    *TxBuf = 0;
    SimpleNetworkTransmitFifoRemove (&(SimpleNetworkDevice->TxBufferFifo), TxBuf);
  }

  return EFI_SUCCESS;
}

/**
  Places a packet in the transmit queue of a network interface.

  @param  This       The protocol instance pointer.
  @param  HeaderSize The size, in bytes, of the media header to be filled in by
                     the Transmit() function. If HeaderSize is non-zero, then it
                     must be equal to This->Mode->MediaHeaderSize and the DestAddr
                     and Protocol parameters must not be NULL.
  @param  BufferSize The size, in bytes, of the entire packet (media header and
                     data) to be transmitted through the network interface.
  @param  Buffer     A pointer to the packet (media header followed by data) to be
                     transmitted. This parameter cannot be NULL. If HeaderSize is zero,
                     then the media header in Buffer must already be filled in by the
                     caller. If HeaderSize is non-zero, then the media header will be
                     filled in by the Transmit() function.
  @param  SrcAddr    The source HW MAC address. If HeaderSize is zero, then this parameter
                     is ignored. If HeaderSize is non-zero and SrcAddr is NULL, then
                     This->Mode->CurrentAddress is used for the source HW MAC address.
  @param  DestAddr   The destination HW MAC address. If HeaderSize is zero, then this
                     parameter is ignored.
  @param  Protocol   The type of header to build. If HeaderSize is zero, then this
                     parameter is ignored. See RFC 1700, section "Ether Types", for
                     examples.

  @retval EFI_SUCCESS           The packet was placed on the transmit queue.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_NOT_READY         The network interface is too busy to accept this transmit request.                      
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkTransmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL           *This,
  IN UINTN                                 HeaderSize,
  IN UINTN                                 BufferSize,
  IN VOID                                  *Buffer,
  IN EFI_MAC_ADDRESS                       *SrcAddr OPTIONAL,
  IN EFI_MAC_ADDRESS                       *DestAddr OPTIONAL,
  IN UINT16                                *Protocol OPTIONAL
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;
  PXENV_UNDI_TRANSMIT_T   XmitInfo;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkStarted:
  default:
    return EFI_DEVICE_ERROR;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize < SimpleNetworkDevice->SimpleNetworkMode.MediaHeaderSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  if (HeaderSize != 0) {
    if (HeaderSize != SimpleNetworkDevice->SimpleNetworkMode.MediaHeaderSize) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestAddr == NULL || Protocol == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestAddr != NULL) {
      CopyMem (
        Buffer,
        DestAddr,
        SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
        );
    }

    if (SrcAddr == NULL) {
      SrcAddr = &SimpleNetworkDevice->SimpleNetworkMode.CurrentAddress;
    }

    CopyMem (
      (UINT8 *) Buffer + SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize,
      SrcAddr,
      SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
      );

    if (Protocol != NULL) {
      *(UINT16 *) ((UINT8 *) Buffer + 2 * SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize) = (UINT16) (((*Protocol & 0xFF) << 8) | ((*Protocol >> 8) & 0xFF));
    }
  }
  //
  // See if the recycled transmit buffer FIFO is full.
  // If it is full, then we can not transmit until the caller calls GetStatus() to pull
  // off recycled transmit buffers.
  //
  if (SimpleNetworkTransmitFifoFull (&(SimpleNetworkDevice->TxBufferFifo))) {
    return EFI_NOT_READY;
  }
  //
  //  Output debug trace message.
  //
  DEBUG ((DEBUG_NET, "Undi16SimpleNetworkTransmit\n\r "));

  //
  // Initialize UNDI WRITE parameter structure.
  //
  XmitInfo.Status           = INIT_PXE_STATUS;
  XmitInfo.Protocol         = P_UNKNOWN;
  XmitInfo.XmitFlag         = XMT_DESTADDR;
  XmitInfo.DestAddrOffset   = (UINT16) ((UINT32)(UINTN) SimpleNetworkDevice->TxDestAddr & 0x000f);
  XmitInfo.DestAddrSegment  = (UINT16) ((UINT32)(UINTN) SimpleNetworkDevice->TxDestAddr >> 4);
  XmitInfo.TBDOffset        = (UINT16) ((UINT32)(UINTN) SimpleNetworkDevice->Xmit & 0x000f);
  XmitInfo.TBDSegment       = (UINT16) ((UINT32)(UINTN) SimpleNetworkDevice->Xmit >> 4);
  XmitInfo.Reserved[0]      = 0;
  XmitInfo.Reserved[1]      = 0;

  CopyMem (
    SimpleNetworkDevice->TxDestAddr,
    Buffer,
    SimpleNetworkDevice->SimpleNetworkMode.HwAddressSize
    );

  CopyMem (
    SimpleNetworkDevice->TxRealModeMediaHeader,
    Buffer,
    SimpleNetworkDevice->SimpleNetworkMode.MediaHeaderSize
    );

  SimpleNetworkDevice->Xmit->ImmedLength            = (UINT16) SimpleNetworkDevice->SimpleNetworkMode.MediaHeaderSize;

  SimpleNetworkDevice->Xmit->DataBlock[0].TDDataLen = (UINT16) (BufferSize - SimpleNetworkDevice->Xmit->ImmedLength);

  CopyMem (
    SimpleNetworkDevice->TxRealModeDataBuffer,
    (UINT8 *) Buffer + SimpleNetworkDevice->SimpleNetworkMode.MediaHeaderSize,
    SimpleNetworkDevice->Xmit->DataBlock[0].TDDataLen
    );

  //
  // Make API call to UNDI TRANSMIT
  //
  XmitInfo.Status = 0;

  Status          = PxeUndiTransmit (SimpleNetworkDevice, &XmitInfo);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the status code from the 16 bit UNDI ROM
  //
  switch (XmitInfo.Status) {
  case PXENV_STATUS_OUT_OF_RESOURCES:
    return EFI_NOT_READY;

  case PXENV_STATUS_SUCCESS:
    break;

  default:
    return EFI_DEVICE_ERROR;
  }
  //
  // Add address of Buffer to the recycled transmit buffer FIFO
  //
  SimpleNetworkTransmitFifoAdd (&(SimpleNetworkDevice->TxBufferFifo), Buffer);

  return EFI_SUCCESS;
}

/**
  Receives a packet from a network interface.
  
  @param  This       The protocol instance pointer.
  @param  HeaderSize The size, in bytes, of the media header received on the network
                     interface. If this parameter is NULL, then the media header size
                     will not be returned.
  @param  BufferSize On entry, the size, in bytes, of Buffer. On exit, the size, in
                     bytes, of the packet that was received on the network interface.
  @param  Buffer     A pointer to the data buffer to receive both the media header and
                     the data.
  @param  SrcAddr    The source HW MAC address. If this parameter is NULL, the
                     HW MAC source address will not be extracted from the media
                     header.
  @param  DestAddr   The destination HW MAC address. If this parameter is NULL,
                     the HW MAC destination address will not be extracted from the
                     media header.
  @param  Protocol   The media header type. If this parameter is NULL, then the
                     protocol will not be extracted from the media header. See
                     RFC 1700 section "Ether Types" for examples.

  @retval  EFI_SUCCESS           The received data was stored in Buffer, and BufferSize has
                                 been updated to the number of bytes received.
  @retval  EFI_NOT_STARTED       The network interface has not been started.
  @retval  EFI_NOT_READY         The network interface is too busy to accept this transmit
                                 request.
  @retval  EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval  EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval  EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval  EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkReceive (
  IN EFI_SIMPLE_NETWORK_PROTOCOL            *This,
  OUT UINTN                                 *HeaderSize OPTIONAL,
  IN OUT UINTN                              *BufferSize,
  OUT VOID                                  *Buffer,
  OUT EFI_MAC_ADDRESS                       *SrcAddr OPTIONAL,
  OUT EFI_MAC_ADDRESS                       *DestAddr OPTIONAL,
  OUT UINT16                                *Protocol OPTIONAL
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;
  UINTN                   MediaAddrSize;
  UINT8                   ProtType;

  if (This == NULL || BufferSize == NULL || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkStarted:
  default:
    return EFI_DEVICE_ERROR;
  }

  Status = Undi16SimpleNetworkIsr (
            This,
            BufferSize,
            HeaderSize,
            Buffer,
            &ProtType,
            NULL
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((SimpleNetworkDevice->InterruptStatus & EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT) == 0) {
    return EFI_NOT_READY;

  }

  SimpleNetworkDevice->InterruptStatus &= ~EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT;

  MediaAddrSize = This->Mode->HwAddressSize;

  if (SrcAddr != NULL) {
    CopyMem (SrcAddr, (UINT8 *) Buffer + MediaAddrSize, MediaAddrSize);
  }

  if (DestAddr != NULL) {
    CopyMem (DestAddr, Buffer, MediaAddrSize);
  }

  if (Protocol != NULL) {
    *((UINT8 *) Protocol)     = *((UINT8 *) Buffer + (2 * MediaAddrSize) + 1);
    *((UINT8 *) Protocol + 1) = *((UINT8 *) Buffer + (2 * MediaAddrSize));
  }

  DEBUG ((DEBUG_NET, "Packet Received: BufferSize=%d  HeaderSize = %d\n", *BufferSize, *HeaderSize));

  return Status;

}
//
// WaitForPacket()
//
/**
  wait for a packet to be received.

  @param Event      Event used with WaitForEvent() to wait for a packet to be received.
  @param Context    Event Context
  
**/
VOID
EFIAPI
Undi16SimpleNetworkWaitForPacket (
  IN EFI_EVENT               Event,
  IN VOID                    *Context
  )
{
  //
  // Someone is waiting on the receive packet event, if there's
  // a packet pending, signal the event
  //
  if (!EFI_ERROR (Undi16SimpleNetworkCheckForPacket (Context))) {
    gBS->SignalEvent (Event);
  }
}
//
// CheckForPacket()
//
/**
  Check whether packet is ready for receive.

  @param This The protocol instance pointer.
  
  @retval  EFI_SUCCESS           Receive data is ready.
  @retval  EFI_NOT_STARTED       The network interface has not been started.
  @retval  EFI_NOT_READY         The network interface is too busy to accept this transmit
                                 request.
  @retval  EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval  EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval  EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval  EFI_UNSUPPORTED       This function is not supported by the network interface.
**/
EFI_STATUS
Undi16SimpleNetworkCheckForPacket (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  )
{
  EFI_STATUS              Status;
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice;
  UINTN                   FrameLength;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status              = EFI_SUCCESS;
  SimpleNetworkDevice = EFI_SIMPLE_NETWORK_DEV_FROM_THIS (This);

  if (SimpleNetworkDevice == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Verify that the current state of the adapter is valid for this call.
  //
  switch (SimpleNetworkDevice->SimpleNetworkMode.State) {
  case EfiSimpleNetworkInitialized:
    break;

  case EfiSimpleNetworkStopped:
    return EFI_NOT_STARTED;

  case EfiSimpleNetworkStarted:
  default:
    return EFI_DEVICE_ERROR;
  }

  FrameLength = 0;
  Status = Undi16SimpleNetworkIsr (
            This,
            &FrameLength,
            NULL,
            NULL,
            NULL,
            NULL
            );

  if (Status != EFI_BUFFER_TOO_SMALL) {
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return ((SimpleNetworkDevice->InterruptStatus & EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT) != 0) ? EFI_SUCCESS : EFI_NOT_READY;
}

/**
  Signal handlers for ExitBootServices event.
  
  Clean up any Real-mode UNDI residue from the system 
   
  @param Event      ExitBootServices event
  @param Context 
**/
VOID
EFIAPI
Undi16SimpleNetworkEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  //
  // NOTE:  This is not the only way to effect this cleanup.  The prescribed mechanism
  //        would be to perform an UNDI STOP command.  This strategam has been attempted
  //        but results in problems making some of the EFI core services from TPL_CALLBACK.
  //        This issue needs to be resolved, but the other alternative has been to perform
  //        the unchain logic explicitly, as done below.
  //
  RestoreCachedVectorAddress (0x1A);
}

/**
  Allocate buffer below 1M for real mode.

  @param NumPages     The number pages want to be allocated.
  @param Buffer       On return, allocated buffer.
  
  @return Status of allocating pages.
**/
EFI_STATUS
BiosSnp16AllocatePagesBelowOneMb (
  UINTN  NumPages,
  VOID   **Buffer
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;

  PhysicalAddress = 0x000fffff;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiRuntimeServicesData,
                  NumPages,
                  &PhysicalAddress
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Buffer = (VOID *) (UINTN) PhysicalAddress;
  return EFI_SUCCESS;
}
