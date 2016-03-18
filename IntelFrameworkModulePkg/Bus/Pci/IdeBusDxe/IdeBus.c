/** @file
  This file implement UEFI driver for IDE Bus which includes device identification, 
  Child device(Disk, CDROM, etc) enumeration and child handler installation, and 
  driver stop.
    
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This module is modified from DXE\IDE module for Ide Contriller Init support

**/

#include "IdeBus.h"

#define PCI_CLASS_MASS_STORAGE  0x01
#define PCI_SUB_CLASS_IDE       0x01


//
// IDE Bus Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL gIDEBusDriverBinding = {
  IDEBusDriverBindingSupported,
  IDEBusDriverBindingStart,
  IDEBusDriverBindingStop,
  0xa,
  NULL,
  NULL
};
/**
  Deregister an IDE device and free resources

  @param  This Protocol instance pointer.
  @param  Controller Ide device handle
  @param  Handle Handle of device to deregister driver on

  @retval EFI_SUCCESS  Deregiter a specific IDE device successfully


**/
EFI_STATUS
DeRegisterIdeDevice (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  EFI_HANDLE                     Handle
  )
{
  EFI_STATUS            Status;
  EFI_BLOCK_IO_PROTOCOL *BlkIo;
  IDE_BLK_IO_DEV        *IdeBlkIoDevice;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINTN                 Index;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &BlkIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IdeBlkIoDevice = IDE_BLOCK_IO_DEV_FROM_THIS (BlkIo);

  //
  // Report Status code: Device disabled
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_ATA_ATAPI | EFI_P_PC_DISABLE),
    IdeBlkIoDevice->DevicePath
    );

  //
  // Close the child handle
  //
  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  This->DriverBindingHandle,
                  Handle
                  );

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  IdeBlkIoDevice->DevicePath,
                  &gEfiBlockIoProtocolGuid,
                  &IdeBlkIoDevice->BlkIo,
                  &gEfiDiskInfoProtocolGuid,
                  &IdeBlkIoDevice->DiskInfo,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    gBS->OpenProtocol (
          Controller,
          &gEfiPciIoProtocolGuid,
          (VOID **) &PciIo,
          This->DriverBindingHandle,
          Handle,
          EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
          );
    return Status;
  }

  //
  // Release allocated resources
  //
  Index = IdeBlkIoDevice->Channel * 2 + IdeBlkIoDevice->Device;
  if (Index < MAX_IDE_DEVICE) {
    IdeBlkIoDevice->IdeBusDriverPrivateData->HaveScannedDevice[Index] = FALSE;
  }
  ReleaseIdeResources (IdeBlkIoDevice);

  return EFI_SUCCESS;
}
/**
  Supported function of Driver Binding protocol for this driver.

  @param This                A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param ControllerHandle    The handle of the controller to test.
  @param RemainingDevicePath A pointer to the remaining portion of a device path.

  @retval  EFI_SUCCESS Driver loaded.
  @retval  other       Driver not loaded.

**/
EFI_STATUS
EFIAPI
IDEBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_PROTOCOL          *ParentDevicePath;
  EFI_DEV_PATH                      *Node;
  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *IdeInit;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  PCI_TYPE00                        PciData;

  if (RemainingDevicePath != NULL) {
    Node = (EFI_DEV_PATH *) RemainingDevicePath;
    //
    // Check if RemainingDevicePath is the End of Device Path Node, 
    // if yes, go on checking other conditions
    //
    if (!IsDevicePathEnd (Node)) {
      //
      // If RemainingDevicePath isn't the End of Device Path Node,
      // check its validation
      //
      if (Node->DevPath.Type != MESSAGING_DEVICE_PATH ||
          Node->DevPath.SubType != MSG_ATAPI_DP ||
          DevicePathNodeLength(&Node->DevPath) != sizeof(ATAPI_DEVICE_PATH)) {
        return EFI_UNSUPPORTED;
      }
    }
  }

  //
  // Verify the Ide Controller Init Protocol, which installed by the
  // IdeController module.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIdeControllerInitProtocolGuid,
                  (VOID **) &IdeInit,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
        Controller,
        &gEfiIdeControllerInitProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  //
  // Close protocol, don't use device path protocol in the Support() function
  //
  gBS->CloseProtocol (
        Controller,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  //
  // Get the EfiPciIoProtocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Now further check the PCI header: Base class (offset 0x0B) and
  // Sub Class (offset 0x0A). This controller should be an IDE controller
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        0,
                        sizeof (PciData),
                        &PciData
                        );

  if (!EFI_ERROR (Status)) {
    //
    // Examine if it is IDE mode by class code
    //
    if ((PciData.Hdr.ClassCode[2] != PCI_CLASS_MASS_STORAGE) || (PciData.Hdr.ClassCode[1] != PCI_SUB_CLASS_IDE)) {     
      Status = EFI_UNSUPPORTED;
    } else {    
      Status = EFI_SUCCESS;
    } 
  }

  return Status;
}


/**
  Start function of Driver binding protocol which start this driver on Controller
  by detecting all disks and installing BlockIo protocol on them.

  @param  This                Protocol instance pointer.
  @param  Controller          Handle of device to bind driver to.
  @param  RemainingDevicePath produce all possible children.

  @retval  EFI_SUCCESS         This driver is added to ControllerHandle.
  @retval  EFI_ALREADY_STARTED This driver is already running on ControllerHandle.
  @retval  other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
IDEBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  EFI_STATUS                        SavedStatus;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_DEVICE_PATH_PROTOCOL          *ParentDevicePath;
  EFI_DEV_PATH                      *Node;
  UINT8                             IdeChannel;
  UINT8                             BeginningIdeChannel;
  UINT8                             EndIdeChannel;
  UINT8                             IdeDevice;
  UINT8                             BeginningIdeDevice;
  UINT8                             EndIdeDevice;
  IDE_BLK_IO_DEV                    *IdeBlkIoDevice[IdeMaxChannel][IdeMaxDevice];
  IDE_BLK_IO_DEV                    *IdeBlkIoDevicePtr;
  IDE_REGISTERS_BASE_ADDR           IdeRegsBaseAddr[IdeMaxChannel];
  ATA_TRANSFER_MODE                 TransferMode;
  ATA_DRIVE_PARMS                   DriveParameters;
  EFI_DEV_PATH                      NewNode;
  UINT8                             ConfigurationOptions;
  UINT16                            CommandBlockBaseAddr;
  UINT16                            ControlBlockBaseAddr;
  UINTN                             DataSize;
  IDE_BUS_DRIVER_PRIVATE_DATA       *IdeBusDriverPrivateData;
  UINT64                            Supports;

  //
  // Local variables declaration for IdeControllerInit support
  //
  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *IdeInit;
  BOOLEAN                           EnumAll;
  BOOLEAN                           ChannelEnabled;
  UINT8                             MaxDevices;
  EFI_IDENTIFY_DATA                 IdentifyData;
  EFI_ATA_COLLECTIVE_MODE           *SupportedModes;

  IdeBusDriverPrivateData = NULL;
  SupportedModes          = NULL;

  //
  // Perform IdeBus initialization
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  //
  // Now open the IDE_CONTROLLER_INIT protocol. Step7.1
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIdeControllerInitProtocolGuid,
                  (VOID **) &IdeInit,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  //
  // The following OpenProtocol function with _GET_PROTOCOL attribute and
  // will not return EFI_ALREADY_STARTED, so save it for now
  //
  SavedStatus = Status;

  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG ((EFI_D_ERROR, "Open Init, Status=%x", Status));
    //
    // open protocol is not SUCCESS or not ALREADY_STARTED, error exit
    //
    goto ErrorExit;
  }

  //
  // Save Enumall. Step7.2
  //
  EnumAll       = IdeInit->EnumAll;

  //
  // Consume PCI I/O protocol. Note that the OpenProtocol with _GET_PROTOCOL
  // attribute will not return EFI_ALREADY_STARTED
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Open PciIo, Status=%x", Status));
    goto ErrorExit;
  }

  //
  // We must check EFI_ALREADY_STARTED because many ATAPI devices are removable
  //
  if (SavedStatus != EFI_ALREADY_STARTED) {
    IdeBusDriverPrivateData = AllocatePool (sizeof (IDE_BUS_DRIVER_PRIVATE_DATA));
    if (IdeBusDriverPrivateData == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    ZeroMem (IdeBusDriverPrivateData, sizeof (IDE_BUS_DRIVER_PRIVATE_DATA));
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Controller,
                    &gEfiCallerIdGuid,
                    IdeBusDriverPrivateData,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

  } else {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiCallerIdGuid,
                    (VOID **) &IdeBusDriverPrivateData,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      IdeBusDriverPrivateData = NULL;
      goto ErrorExit;
    }
  }

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );
  if (!EFI_ERROR (Status)) {
    Supports &= (UINT64)EFI_PCI_DEVICE_ENABLE;
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationEnable,
                      Supports,
                      NULL
                      );
  }

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Read the environment variable that contains the IDEBus Driver's
  // Config options that were set by the Driver Configuration Protocol
  //
  DataSize = sizeof (ConfigurationOptions);
  Status = gRT->GetVariable (
                  (CHAR16 *) L"Configuration",
                  &gEfiCallerIdGuid,
                  NULL,
                  &DataSize,
                  &ConfigurationOptions
                  );
  if (EFI_ERROR (Status)) {
    ConfigurationOptions = 0x0f;
  }

   if (EnumAll || RemainingDevicePath == NULL) {
    //
    // If IdeInit->EnumAll is TRUE or RemainingDevicePath is NULL, 
    // must enumerate all IDE devices anyway
    //
    BeginningIdeChannel = IdePrimary;
    EndIdeChannel       = IdeSecondary;
    BeginningIdeDevice  = IdeMaster;
    EndIdeDevice        = IdeSlave;

  } else if (!IsDevicePathEnd (RemainingDevicePath)) {
    //
    // If RemainingDevicePath isn't the End of Device Path Node, 
    // only scan the specified device by RemainingDevicePath
    //
    Node                = (EFI_DEV_PATH *) RemainingDevicePath;
    BeginningIdeChannel = Node->Atapi.PrimarySecondary;
    EndIdeChannel       = BeginningIdeChannel;
    BeginningIdeDevice  = Node->Atapi.SlaveMaster;
    EndIdeDevice        = BeginningIdeDevice;
    if (BeginningIdeChannel >= IdeMaxChannel || EndIdeChannel >= IdeMaxChannel) {
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }
    if (BeginningIdeDevice >= IdeMaxDevice|| EndIdeDevice >= IdeMaxDevice) {
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }

  } else {
    //
    // If RemainingDevicePath is the End of Device Path Node,
    // skip enumerate any device and return EFI_SUCESSS
    // 
    BeginningIdeChannel = IdeMaxChannel;
    EndIdeChannel       = IdeMaxChannel - 1;
    BeginningIdeDevice  = IdeMaxDevice;
    EndIdeDevice        = IdeMaxDevice - 1;
  }

  //
  // Obtain IDE IO port registers' base addresses
  //
  Status = GetIdeRegistersBaseAddr (PciIo, IdeRegsBaseAddr);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Report status code: begin IdeBus initialization
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_PC_RESET),
    ParentDevicePath
    );

  //
  // Strictly follow the enumeration based on IDE_CONTROLLER_INIT protocol
  //
  for (IdeChannel = BeginningIdeChannel; IdeChannel <= EndIdeChannel; IdeChannel++) {

    IdeInit->NotifyPhase (IdeInit, EfiIdeBeforeChannelEnumeration, IdeChannel);

    //
    // now obtain channel information fron IdeControllerInit protocol. Step9
    //
    Status = IdeInit->GetChannelInfo (
                        IdeInit,
                        IdeChannel,
                        &ChannelEnabled,
                        &MaxDevices
                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[GetChannel, Status=%x]", Status));
      continue;
    }

    if (!ChannelEnabled) {
      continue;
    }

    EndIdeDevice = (UINT8) MIN ((MaxDevices - 1), EndIdeDevice);
    ASSERT (EndIdeDevice < IdeMaxDevice);
    //
    // Now inform the IDE Controller Init Module. Sept10
    //
    IdeInit->NotifyPhase (IdeInit, EfiIdeBeforeChannelReset, IdeChannel);

    //
    // No reset channel function implemented. Sept11
    //
    IdeInit->NotifyPhase (IdeInit, EfiIdeAfterChannelReset, IdeChannel);

    //
    // Step13
    //
    IdeInit->NotifyPhase (
              IdeInit,
              EfiIdeBusBeforeDevicePresenceDetection,
              IdeChannel
              );

    //
    // Prepare to detect IDE device of this channel
    //
    InitializeIDEChannelData ();

    //
    // -- 1st inner loop --- Master/Slave ------------  Step14
    //
    for (IdeDevice = BeginningIdeDevice; IdeDevice <= EndIdeDevice; IdeDevice++) {
      //
      // Check whether the configuration options allow this device
      //
      if ((ConfigurationOptions & (1 << (IdeChannel * 2 + IdeDevice))) == 0) {
        continue;
      }

      //
      // The device has been scanned in another Start(), No need to scan it again
      // for perf optimization.
      //
      if (IdeBusDriverPrivateData->HaveScannedDevice[IdeChannel * 2 + IdeDevice]) {
        continue;
      }

      //
      // create child handle for the detected device.
      //
      IdeBlkIoDevice[IdeChannel][IdeDevice] = AllocatePool (sizeof (IDE_BLK_IO_DEV));
      if (IdeBlkIoDevice[IdeChannel][IdeDevice] == NULL) {
        continue;
      }

      IdeBlkIoDevicePtr = IdeBlkIoDevice[IdeChannel][IdeDevice];

      ZeroMem (IdeBlkIoDevicePtr, sizeof (IDE_BLK_IO_DEV));

      IdeBlkIoDevicePtr->Signature  = IDE_BLK_IO_DEV_SIGNATURE;
      IdeBlkIoDevicePtr->Channel    = (EFI_IDE_CHANNEL) IdeChannel;
      IdeBlkIoDevicePtr->Device     = (EFI_IDE_DEVICE) IdeDevice;

      //
      // initialize Block IO interface's Media pointer
      //
      IdeBlkIoDevicePtr->BlkIo.Media = &IdeBlkIoDevicePtr->BlkMedia;

      //
      // Initialize IDE IO port addresses, including Command Block registers
      // and Control Block registers
      //
      IdeBlkIoDevicePtr->IoPort = AllocatePool (sizeof (IDE_BASE_REGISTERS));
      if (IdeBlkIoDevicePtr->IoPort == NULL) {
        continue;
      }

      ZeroMem (IdeBlkIoDevicePtr->IoPort, sizeof (IDE_BASE_REGISTERS));
      CommandBlockBaseAddr = IdeRegsBaseAddr[IdeChannel].CommandBlockBaseAddr;
      ControlBlockBaseAddr = IdeRegsBaseAddr[IdeChannel].ControlBlockBaseAddr;

      IdeBlkIoDevicePtr->IoPort->Data = CommandBlockBaseAddr;
      (*(UINT16 *) &IdeBlkIoDevicePtr->IoPort->Reg1) = (UINT16) (CommandBlockBaseAddr + 0x01);
      IdeBlkIoDevicePtr->IoPort->SectorCount = (UINT16) (CommandBlockBaseAddr + 0x02);
      IdeBlkIoDevicePtr->IoPort->SectorNumber = (UINT16) (CommandBlockBaseAddr + 0x03);
      IdeBlkIoDevicePtr->IoPort->CylinderLsb = (UINT16) (CommandBlockBaseAddr + 0x04);
      IdeBlkIoDevicePtr->IoPort->CylinderMsb = (UINT16) (CommandBlockBaseAddr + 0x05);
      IdeBlkIoDevicePtr->IoPort->Head = (UINT16) (CommandBlockBaseAddr + 0x06);
      (*(UINT16 *) &IdeBlkIoDevicePtr->IoPort->Reg) = (UINT16) (CommandBlockBaseAddr + 0x07);

      (*(UINT16 *) &IdeBlkIoDevicePtr->IoPort->Alt) = ControlBlockBaseAddr;
      IdeBlkIoDevicePtr->IoPort->DriveAddress = (UINT16) (ControlBlockBaseAddr + 0x01);

      IdeBlkIoDevicePtr->IoPort->MasterSlave = (UINT16) ((IdeDevice == IdeMaster) ? 1 : 0);

      IdeBlkIoDevicePtr->PciIo = PciIo;
      IdeBlkIoDevicePtr->IdeBusDriverPrivateData = IdeBusDriverPrivateData;
      IdeBlkIoDevicePtr->IoPort->BusMasterBaseAddr = IdeRegsBaseAddr[IdeChannel].BusMasterBaseAddr;

      //
      // Report Status code: is about to detect IDE drive
      //
      REPORT_STATUS_CODE_EX (
        EFI_PROGRESS_CODE,
        (EFI_IO_BUS_ATA_ATAPI | EFI_P_PC_PRESENCE_DETECT),
        0,
        &gEfiCallerIdGuid,
        NULL,
        NULL,
        0
      );

      //
      // Discover device, now!
      //
      PERF_START (NULL, "DiscoverIdeDevice", "IDE", 0);
      Status = DiscoverIdeDevice (IdeBlkIoDevicePtr);
      PERF_END (NULL, "DiscoverIdeDevice", "IDE", 0);

      IdeBusDriverPrivateData->HaveScannedDevice[IdeChannel * 2 + IdeDevice]  = TRUE;
      IdeBusDriverPrivateData->DeviceProcessed[IdeChannel * 2 + IdeDevice]    = FALSE;

      if (!EFI_ERROR (Status)) {
        //
        // Set Device Path
        //
        ZeroMem (&NewNode, sizeof (NewNode));
        NewNode.DevPath.Type    = MESSAGING_DEVICE_PATH;
        NewNode.DevPath.SubType = MSG_ATAPI_DP;
        SetDevicePathNodeLength (&NewNode.DevPath, sizeof (ATAPI_DEVICE_PATH));

        NewNode.Atapi.PrimarySecondary  = (UINT8) IdeBlkIoDevicePtr->Channel;
        NewNode.Atapi.SlaveMaster       = (UINT8) IdeBlkIoDevicePtr->Device;
        NewNode.Atapi.Lun               = IdeBlkIoDevicePtr->Lun;
        IdeBlkIoDevicePtr->DevicePath = AppendDevicePathNode (
                                          ParentDevicePath,
                                          &NewNode.DevPath
                                          );
        if (IdeBlkIoDevicePtr->DevicePath == NULL) {
          ReleaseIdeResources (IdeBlkIoDevicePtr);
          continue;
        }

        //
        // Submit identify data to IDE controller init driver
        //
        CopyMem (&IdentifyData, IdeBlkIoDevicePtr->IdData, sizeof (IdentifyData));
        IdeBusDriverPrivateData->DeviceFound[IdeChannel * 2 + IdeDevice] = TRUE;
        IdeInit->SubmitData (IdeInit, IdeChannel, IdeDevice, &IdentifyData);
      } else {
        //
        // Device detection failed
        //
        IdeBusDriverPrivateData->DeviceFound[IdeChannel * 2 + IdeDevice] = FALSE;
        IdeInit->SubmitData (IdeInit, IdeChannel, IdeDevice, NULL);
        ReleaseIdeResources (IdeBlkIoDevicePtr);
        IdeBlkIoDevicePtr = NULL;
      }
      //
      // end of 1st inner loop ---
      //
    }
    //
    // end of 1st outer loop =========
    //
  }

  //
  // = 2nd outer loop == Primary/Secondary =================
  //
  for (IdeChannel = BeginningIdeChannel; IdeChannel <= EndIdeChannel; IdeChannel++) {

    //
    // -- 2nd inner loop --- Master/Slave --------
    //
    for (IdeDevice = BeginningIdeDevice; IdeDevice <= EndIdeDevice; IdeDevice++) {

      ASSERT (IdeChannel * 2 + IdeDevice < MAX_IDE_DEVICE);
      if (IdeBusDriverPrivateData->DeviceProcessed[IdeChannel * 2 + IdeDevice]) {
        continue;
      }

      if (!IdeBusDriverPrivateData->DeviceFound[IdeChannel * 2 + IdeDevice]) {
        continue;
      }

      Status = IdeInit->CalculateMode (
                          IdeInit,
                          IdeChannel,
                          IdeDevice,
                          &SupportedModes
                          );
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "[bStStp20S=%x]", Status));
        continue;
      }

      ASSERT (IdeChannel < IdeMaxChannel && IdeDevice < IdeMaxDevice);
      IdeBlkIoDevicePtr = IdeBlkIoDevice[IdeChannel][IdeDevice];

      //
      // Set best supported PIO mode on this IDE device
      //
      if (SupportedModes->PioMode.Mode <= AtaPioMode2) {
        TransferMode.ModeCategory = ATA_MODE_CATEGORY_DEFAULT_PIO;
      } else {
        TransferMode.ModeCategory = ATA_MODE_CATEGORY_FLOW_PIO;
      }

      TransferMode.ModeNumber = (UINT8) (SupportedModes->PioMode.Mode);

      if (SupportedModes->ExtModeCount == 0){
        Status                  = SetDeviceTransferMode (IdeBlkIoDevicePtr, &TransferMode);

        if (EFI_ERROR (Status)) {
          IdeBusDriverPrivateData->DeviceFound[IdeChannel * 2 + IdeDevice] = FALSE;
          ReleaseIdeResources (IdeBlkIoDevicePtr);
          IdeBlkIoDevicePtr = NULL;
          continue;
        }
      }

      //
      // Set supported DMA mode on this IDE device. Note that UDMA & MDMA cann't
      // be set together. Only one DMA mode can be set to a device. If setting
      // DMA mode operation fails, we can continue moving on because we only use
      // PIO mode at boot time. DMA modes are used by certain kind of OS booting
      //
      if (SupportedModes->UdmaMode.Valid) {

        TransferMode.ModeCategory = ATA_MODE_CATEGORY_UDMA;
        TransferMode.ModeNumber   = (UINT8) (SupportedModes->UdmaMode.Mode);
        Status                    = SetDeviceTransferMode (IdeBlkIoDevicePtr, &TransferMode);

        if (EFI_ERROR (Status)) {
          IdeBusDriverPrivateData->DeviceFound[IdeChannel * 2 + IdeDevice] = FALSE;
          ReleaseIdeResources (IdeBlkIoDevicePtr);
          IdeBlkIoDevicePtr = NULL;
          continue;
        }
        //
        // Record Udma Mode
        //
        IdeBlkIoDevicePtr->UdmaMode.Valid = TRUE;
        IdeBlkIoDevicePtr->UdmaMode.Mode  = SupportedModes->UdmaMode.Mode;
        EnableInterrupt (IdeBlkIoDevicePtr);
      } else if (SupportedModes->MultiWordDmaMode.Valid) {

        TransferMode.ModeCategory = ATA_MODE_CATEGORY_MDMA;
        TransferMode.ModeNumber   = (UINT8) SupportedModes->MultiWordDmaMode.Mode;
        Status                    = SetDeviceTransferMode (IdeBlkIoDevicePtr, &TransferMode);

        if (EFI_ERROR (Status)) {
          IdeBusDriverPrivateData->DeviceFound[IdeChannel * 2 + IdeDevice] = FALSE;
          ReleaseIdeResources (IdeBlkIoDevicePtr);
          IdeBlkIoDevicePtr = NULL;
          continue;
        }

        EnableInterrupt (IdeBlkIoDevicePtr);
      }
      //
      // Init driver parameters
      //
      DriveParameters.Sector          = (UINT8) ((ATA5_IDENTIFY_DATA *) IdeBlkIoDevicePtr->IdData)->sectors_per_track;
      DriveParameters.Heads           = (UINT8) (((ATA5_IDENTIFY_DATA *) IdeBlkIoDevicePtr->IdData)->heads - 1);
      DriveParameters.MultipleSector  = (UINT8) IdeBlkIoDevicePtr->IdData->AtaData.multi_sector_cmd_max_sct_cnt;
      //
      // Set Parameters for the device:
      // 1) Init
      // 2) Establish the block count for READ/WRITE MULTIPLE (EXT) command
      //
      if ((IdeBlkIoDevicePtr->Type == IdeHardDisk) || (IdeBlkIoDevicePtr->Type == Ide48bitAddressingHardDisk)) {
        Status = SetDriveParameters (IdeBlkIoDevicePtr, &DriveParameters);
      }

      //
      // Record PIO mode used in private data
      //
      IdeBlkIoDevicePtr->PioMode = (ATA_PIO_MODE) SupportedModes->PioMode.Mode;

      //
      // Set IDE controller Timing Blocks in the PCI Configuration Space
      //
      IdeInit->SetTiming (IdeInit, IdeChannel, IdeDevice, SupportedModes);

      //
      // Add Component Name for the IDE/ATAPI device that was discovered.
      //
      IdeBlkIoDevicePtr->ControllerNameTable = NULL;
      ADD_IDE_ATAPI_NAME (IdeBlkIoDevicePtr);

      Status = gBS->InstallMultipleProtocolInterfaces (
                      &IdeBlkIoDevicePtr->Handle,
                      &gEfiDevicePathProtocolGuid,
                      IdeBlkIoDevicePtr->DevicePath,
                      &gEfiBlockIoProtocolGuid,
                      &IdeBlkIoDevicePtr->BlkIo,
                      &gEfiDiskInfoProtocolGuid,
                      &IdeBlkIoDevicePtr->DiskInfo,
                      NULL
                      );

      if (EFI_ERROR (Status)) {
        ReleaseIdeResources (IdeBlkIoDevicePtr);
      }

      gBS->OpenProtocol (
            Controller,
            &gEfiPciIoProtocolGuid,
            (VOID **) &PciIo,
            This->DriverBindingHandle,
            IdeBlkIoDevicePtr->Handle,
            EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
            );

      IdeBusDriverPrivateData->DeviceProcessed[IdeChannel * 2 + IdeDevice] = TRUE;

      //
      // Report status code: device eanbled!
      //
      REPORT_STATUS_CODE_WITH_DEVICE_PATH (
        EFI_PROGRESS_CODE,
        (EFI_IO_BUS_ATA_ATAPI | EFI_P_PC_ENABLE),
        IdeBlkIoDevicePtr->DevicePath
        );

      //
      // Create event to clear pending IDE interrupt
      //
      Status = gBS->CreateEventEx (
                      EVT_NOTIFY_SIGNAL,
                      TPL_NOTIFY,
                      ClearInterrupt,
                      IdeBlkIoDevicePtr,
                      &gEfiEventExitBootServicesGuid,
                      &IdeBlkIoDevicePtr->ExitBootServiceEvent
                      );

      //
      // end of 2nd inner loop ----
      //
    }
    //
    // end of 2nd outer loop ==========
    //
  }

  //
  // All configurations done! Notify IdeController to do post initialization
  // work such as saving IDE controller PCI settings for S3 resume
  //
  IdeInit->NotifyPhase (IdeInit, EfiIdeBusPhaseMaximum, 0);

  if (SupportedModes != NULL) {
    FreePool (SupportedModes);
  }

  PERF_START (NULL, "Finish IDE detection", "IDE", 1);
  PERF_END (NULL, "Finish IDE detection", "IDE", 0);

  return EFI_SUCCESS;

ErrorExit:

  //
  // Report error code: controller error
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_ERROR_CODE | EFI_ERROR_MINOR,
    (EFI_IO_BUS_ATA_ATAPI | EFI_IOB_EC_CONTROLLER_ERROR),
    ParentDevicePath
    );

  gBS->CloseProtocol (
        Controller,
        &gEfiIdeControllerInitProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  gBS->UninstallMultipleProtocolInterfaces (
        Controller,
        &gEfiCallerIdGuid,
        IdeBusDriverPrivateData,
        NULL
        );

  if (IdeBusDriverPrivateData != NULL) {
    gBS->FreePool (IdeBusDriverPrivateData);
  }

  if (SupportedModes != NULL) {
    gBS->FreePool (SupportedModes);
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

  return Status;

}
/**
  Stop function of Driver Binding Protocol which is to stop the driver on Controller Handle and all
  child handle attached to the controller handle if there are.

  @param  This Protocol instance pointer.
  @param  Controller Handle of device to stop driver on
  @param  NumberOfChildren Not used
  @param  ChildHandleBuffer Not used

  @retval  EFI_SUCCESS This driver is removed DeviceHandle
  @retval  other This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
IDEBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  BOOLEAN                     AllChildrenStopped;
  UINTN                       Index;
  IDE_BUS_DRIVER_PRIVATE_DATA *IdeBusDriverPrivateData;
  UINT64                      Supports;

  IdeBusDriverPrivateData = NULL;

  if (NumberOfChildren == 0) {

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
        Supports &= (UINT64)(EFI_PCI_IO_ATTRIBUTE_IDE_PRIMARY_IO | EFI_PCI_IO_ATTRIBUTE_IDE_SECONDARY_IO | EFI_PCI_DEVICE_ENABLE);
        PciIo->Attributes (
                PciIo,
                EfiPciIoAttributeOperationDisable,
                Supports,
                NULL
                );
      }
    }

    gBS->OpenProtocol (
          Controller,
          &gEfiCallerIdGuid,
          (VOID **) &IdeBusDriverPrivateData,
          This->DriverBindingHandle,
          Controller,
          EFI_OPEN_PROTOCOL_GET_PROTOCOL
          );

    gBS->UninstallMultipleProtocolInterfaces (
          Controller,
          &gEfiCallerIdGuid,
          IdeBusDriverPrivateData,
          NULL
          );

    if (IdeBusDriverPrivateData != NULL) {
      gBS->FreePool (IdeBusDriverPrivateData);
    }
    //
    // Close the bus driver
    //
    gBS->CloseProtocol (
          Controller,
          &gEfiIdeControllerInitProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
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

    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {

    Status = DeRegisterIdeDevice (This, Controller, ChildHandleBuffer[Index]);

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  issue ATA or ATAPI command to reset a block IO device.
  @param  This                  Block IO protocol instance pointer.
  @param  ExtendedVerification  If FALSE,for ATAPI device, driver will only invoke ATAPI reset method
                                If TRUE, for ATAPI device, driver need invoke ATA reset method after
                                invoke ATAPI reset method

  @retval EFI_DEVICE_ERROR      When the device is neighther ATA device or ATAPI device.
  @retval EFI_SUCCESS           The device reset successfully

**/
EFI_STATUS
EFIAPI
IDEBlkIoReset (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  BOOLEAN                 ExtendedVerification
  )
{
  IDE_BLK_IO_DEV  *IdeBlkIoDevice;
  EFI_STATUS      Status;
  EFI_TPL         OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  IdeBlkIoDevice = IDE_BLOCK_IO_DEV_FROM_THIS (This);
  //
  // Requery IDE IO resources in case of the switch of native and legacy modes
  //
  ReassignIdeResources (IdeBlkIoDevice);

  //
  // for ATA device, using ATA reset method
  //
  if (IdeBlkIoDevice->Type == IdeHardDisk ||
      IdeBlkIoDevice->Type == Ide48bitAddressingHardDisk) {
    Status = AtaSoftReset (IdeBlkIoDevice);
    goto Done;
  }

  if (IdeBlkIoDevice->Type == IdeUnknown) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  //
  // for ATAPI device, using ATAPI reset method
  //
  Status = AtapiSoftReset (IdeBlkIoDevice);
  if (ExtendedVerification) {
    Status = AtaSoftReset (IdeBlkIoDevice);
  }

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Read data from a block IO device

  @param  This       Block IO protocol instance pointer.
  @param  MediaId    The media ID of the device
  @param  Lba        Starting LBA address to read data
  @param  BufferSize The size of data to be read
  @param  Buffer     Caller supplied buffer to save data

  @retval EFI_DEVICE_ERROR  unknown device type
  @retval other             read data status.

**/
EFI_STATUS
EFIAPI
IDEBlkIoReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  )
{
  IDE_BLK_IO_DEV  *IdeBlkIoDevice;
  EFI_STATUS      Status;
  EFI_TPL         OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  IdeBlkIoDevice = IDE_BLOCK_IO_DEV_FROM_THIS (This);

  //
  // Requery IDE IO resources in case of the switch of native and legacy modes
  //
  ReassignIdeResources (IdeBlkIoDevice);

  //
  // For ATA compatible device, use ATA read block's mechanism
  //
  if (IdeBlkIoDevice->Type == IdeHardDisk ||
      IdeBlkIoDevice->Type == Ide48bitAddressingHardDisk) {
    Status = AtaBlkIoReadBlocks (
            IdeBlkIoDevice,
            MediaId,
            Lba,
            BufferSize,
            Buffer
            );
    goto Done;
  }

  if (IdeBlkIoDevice->Type == IdeUnknown) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  //
  // for ATAPI device, using ATAPI read block's mechanism
  //
  Status = AtapiBlkIoReadBlocks (
          IdeBlkIoDevice,
          MediaId,
          Lba,
          BufferSize,
          Buffer
          );

Done:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Write data to block io device.

  @param  This       Protocol instance pointer.
  @param  MediaId    The media ID of the device
  @param  Lba        Starting LBA address to write data
  @param  BufferSize The size of data to be written
  @param  Buffer     Caller supplied buffer to save data

  @retval EFI_DEVICE_ERROR  unknown device type
  @retval other             write data status

**/
EFI_STATUS
EFIAPI
IDEBlkIoWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )
{
  IDE_BLK_IO_DEV  *IdeBlkIoDevice;
  EFI_STATUS      Status;
  EFI_TPL         OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  IdeBlkIoDevice = IDE_BLOCK_IO_DEV_FROM_THIS (This);
  //
  // Requery IDE IO resources in case of the switch of native and legacy modes
  //
  ReassignIdeResources (IdeBlkIoDevice);

  //
  // for ATA device, using ATA write block's mechanism
  //
  if (IdeBlkIoDevice->Type == IdeHardDisk ||
      IdeBlkIoDevice->Type == Ide48bitAddressingHardDisk) {

    Status = AtaBlkIoWriteBlocks (
            IdeBlkIoDevice,
            MediaId,
            Lba,
            BufferSize,
            Buffer
            );
    goto Done;
  }

  if (IdeBlkIoDevice->Type == IdeUnknown) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  //
  // for ATAPI device, using ATAPI write block's mechanism
  //
  Status = AtapiBlkIoWriteBlocks (
          IdeBlkIoDevice,
          MediaId,
          Lba,
          BufferSize,
          Buffer
          );

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}
/**
  Flushes all modified data to a physical block devices

  @param  This  Indicates a pointer to the calling context which to sepcify a
                sepcific block device

  @retval EFI_SUCCESS   Always return success.
**/
EFI_STATUS
EFIAPI
IDEBlkIoFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  )
{
  //
  // return directly
  //
  return EFI_SUCCESS;
}

/**
  This function is used by the IDE bus driver to get inquiry data. 
  Data format of Identify data is defined by the Interface GUID.

  @param  This                  Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param  InquiryData           Pointer to a buffer for the inquiry data.
  @param  InquiryDataSize       Pointer to the value for the inquiry data size.

  @retval EFI_SUCCESS           The command was accepted without any errors.
  @retval EFI_NOT_FOUND         Device does not support this data class 
  @retval EFI_DEVICE_ERROR      Error reading InquiryData from device 
  @retval EFI_BUFFER_TOO_SMALL  IntquiryDataSize not big enough 

**/
EFI_STATUS
EFIAPI
IDEDiskInfoInquiry (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *InquiryData,
  IN OUT UINT32                   *InquiryDataSize
  )
{
  IDE_BLK_IO_DEV  *IdeBlkIoDevice;

  IdeBlkIoDevice = IDE_BLOCK_IO_DEV_FROM_DISK_INFO_THIS (This);

  if (*InquiryDataSize < sizeof (ATAPI_INQUIRY_DATA)) {
    *InquiryDataSize = sizeof (ATAPI_INQUIRY_DATA);
    return EFI_BUFFER_TOO_SMALL;
  }

  if (IdeBlkIoDevice->InquiryData == NULL) {
    return EFI_NOT_FOUND;
  }

  gBS->CopyMem (InquiryData, IdeBlkIoDevice->InquiryData, sizeof (ATAPI_INQUIRY_DATA));
  *InquiryDataSize = sizeof (ATAPI_INQUIRY_DATA);

  return EFI_SUCCESS;
}

/**
  This function is used by the IDE bus driver to get identify data. 
  Data format of Identify data is defined by the Interface GUID.

  @param  This                  Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param  IdentifyData          Pointer to a buffer for the identify data.
  @param  IdentifyDataSize      Pointer to the value for the identify data size.

  @retval EFI_SUCCESS           The command was accepted without any errors.
  @retval EFI_NOT_FOUND         Device does not support this data class 
  @retval EFI_DEVICE_ERROR      Error reading IdentifyData from device 
  @retval EFI_BUFFER_TOO_SMALL  IdentifyDataSize not big enough 

**/
EFI_STATUS
EFIAPI
IDEDiskInfoIdentify (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *IdentifyData,
  IN OUT UINT32                   *IdentifyDataSize
  )
{
  IDE_BLK_IO_DEV  *IdeBlkIoDevice;

  IdeBlkIoDevice = IDE_BLOCK_IO_DEV_FROM_DISK_INFO_THIS (This);

  if (*IdentifyDataSize < sizeof (EFI_IDENTIFY_DATA)) {
    *IdentifyDataSize = sizeof (EFI_IDENTIFY_DATA);
    return EFI_BUFFER_TOO_SMALL;
  }

  if (IdeBlkIoDevice->IdData == NULL) {
    return EFI_NOT_FOUND;
  }

  gBS->CopyMem (IdentifyData, IdeBlkIoDevice->IdData, sizeof (EFI_IDENTIFY_DATA));
  *IdentifyDataSize = sizeof (EFI_IDENTIFY_DATA);

  return EFI_SUCCESS;
}

/**
  This function is used by the IDE bus driver to get sense data. 
  Data format of Sense data is defined by the Interface GUID.

  @param  This                  Pointer to the EFI_DISK_INFO_PROTOCOL instance. 
  @param  SenseData             Pointer to the SenseData. 
  @param  SenseDataSize         Size of SenseData in bytes. 
  @param  SenseDataNumber       Pointer to the value for the identify data size.

  @retval EFI_SUCCESS           The command was accepted without any errors.
  @retval EFI_NOT_FOUND         Device does not support this data class 
  @retval EFI_DEVICE_ERROR      Error reading InquiryData from device 
  @retval EFI_BUFFER_TOO_SMALL  SenseDataSize not big enough 

**/
EFI_STATUS
EFIAPI
IDEDiskInfoSenseData (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *SenseData,
  IN OUT UINT32                   *SenseDataSize,
  OUT    UINT8                    *SenseDataNumber
  )
{
  return EFI_NOT_FOUND;
}

/**
  This function is used by the IDE bus driver to get controller information.

  @param  This                  Pointer to the EFI_DISK_INFO_PROTOCOL instance. 
  @param  IdeChannel            Pointer to the Ide Channel number. Primary or secondary.
  @param  IdeDevice             Pointer to the Ide Device number. Master or slave.

  @retval EFI_SUCCESS           IdeChannel and IdeDevice are valid 
  @retval EFI_UNSUPPORTED       This is not an IDE device 

**/
EFI_STATUS
EFIAPI
IDEDiskInfoWhichIde (
  IN  EFI_DISK_INFO_PROTOCOL   *This,
  OUT UINT32                   *IdeChannel,
  OUT UINT32                   *IdeDevice
  )
{
  IDE_BLK_IO_DEV  *IdeBlkIoDevice;

  IdeBlkIoDevice  = IDE_BLOCK_IO_DEV_FROM_DISK_INFO_THIS (This);
  *IdeChannel     = IdeBlkIoDevice->Channel;
  *IdeDevice      = IdeBlkIoDevice->Device;

  return EFI_SUCCESS;
}

/**
  The is an event(generally the event is exitBootService event) call back function.
  Clear pending IDE interrupt before OS loader/kernel take control of the IDE device.

  @param  Event   Pointer to this event
  @param  Context Event handler private data

**/
VOID
EFIAPI
ClearInterrupt (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS      Status;
  UINT64          IoPortForBmis;
  UINT8           RegisterValue;
  IDE_BLK_IO_DEV  *IdeDev;

  //
  // Get our context
  //
  IdeDev = (IDE_BLK_IO_DEV *) Context;

  //
  // Obtain IDE IO port registers' base addresses
  //
  Status = ReassignIdeResources (IdeDev);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Check whether interrupt is pending
  //

  //
  // Reset IDE device to force it de-assert interrupt pin
  // Note: this will reset all devices on this IDE channel
  //
  Status = AtaSoftReset (IdeDev);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Get base address of IDE Bus Master Status Regsiter
  //
  if (IdePrimary == IdeDev->Channel) {
    IoPortForBmis = IdeDev->IoPort->BusMasterBaseAddr + BMISP_OFFSET;
  } else {
    if (IdeSecondary == IdeDev->Channel) {
      IoPortForBmis = IdeDev->IoPort->BusMasterBaseAddr + BMISS_OFFSET;
    } else {
      return;
    }
  }
  //
  // Read BMIS register and clear ERROR and INTR bit
  //
  IdeDev->PciIo->Io.Read (
                      IdeDev->PciIo,
                      EfiPciIoWidthUint8,
                      EFI_PCI_IO_PASS_THROUGH_BAR,
                      IoPortForBmis,
                      1,
                      &RegisterValue
                      );

  RegisterValue |= (BMIS_INTERRUPT | BMIS_ERROR);

  IdeDev->PciIo->Io.Write (
                      IdeDev->PciIo,
                      EfiPciIoWidthUint8,
                      EFI_PCI_IO_PASS_THROUGH_BAR,
                      IoPortForBmis,
                      1,
                      &RegisterValue
                      );

  //
  // Select the other device on this channel to ensure this device to release the interrupt pin
  //
  if (IdeDev->Device == 0) {
    RegisterValue = (1 << 4) | 0xe0;
  } else {
    RegisterValue = (0 << 4) | 0xe0;
  }
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    RegisterValue
    );

}

/**
  The user Entry Point for module IdeBus. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeIdeBus(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallAllDriverProtocols2 (
             ImageHandle,
             SystemTable,
             &gIDEBusDriverBinding,
             ImageHandle,
             &gIDEBusComponentName,
             &gIDEBusComponentName2,
             NULL,
             NULL,
             &gIDEBusDriverDiagnostics,
             &gIDEBusDriverDiagnostics2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
