/** @file

  The implementation of USB mass storage class device driver.
  The command set supported is "USB Mass Storage Specification
  for Bootability".

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UsbMassImpl.h"

USB_MASS_TRANSPORT *mUsbMassTransport[] = {
  &mUsbCbi0Transport,
  &mUsbCbi1Transport,
  &mUsbBotTransport,
  NULL
};

/**
  Reset the block device. ExtendedVerification is ignored for this.

  @param  This                   The BLOCK IO protocol
  @param  ExtendedVerification   Whether to execute extended verification.

  @retval EFI_SUCCESS            The device is successfully reseted.
  @retval Others                 Failed to reset the device.

**/
EFI_STATUS
EFIAPI
UsbMassReset (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ExtendedVerification
  )
{
  USB_MASS_DEVICE *UsbMass;
  EFI_TPL         OldTpl;
  EFI_STATUS      Status;

  OldTpl  = gBS->RaiseTPL (USB_MASS_TPL);

  UsbMass = USB_MASS_DEVICE_FROM_BLOCKIO (This);
  Status  = UsbMass->Transport->Reset (UsbMass->Context, ExtendedVerification);

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Read some blocks of data from the block device.

  @param  This                   The Block IO protocol
  @param  MediaId                The media's ID of the device for current request
  @param  Lba                    The start block number
  @param  BufferSize             The size of buffer to read data in
  @param  Buffer                 The buffer to read data to

  @retval EFI_SUCCESS            The data is successfully read
  @retval EFI_NO_MEDIA           Media isn't present
  @retval EFI_MEDIA_CHANGED      The device media has been changed, that is,
                                 MediaId changed
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid, such as Buffer is
                                 NULL.
  @retval EFI_BAD_BUFFER_SIZE    The buffer size isn't a multiple of media's block
                                 size,  or overflow the last block number.

**/
EFI_STATUS
EFIAPI
UsbMassReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  OUT VOID                    *Buffer
  )
{
  USB_MASS_DEVICE     *UsbMass;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  EFI_TPL             OldTpl;
  UINTN               TotalBlock;

  OldTpl  = gBS->RaiseTPL (USB_MASS_TPL);
  UsbMass = USB_MASS_DEVICE_FROM_BLOCKIO (This);
  Media   = &UsbMass->BlockIoMedia;

  //
  // First, validate the parameters
  //
  if ((Buffer == NULL) || (BufferSize == 0)) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  //
  // If it is a removable media, such as CD-Rom or Usb-Floppy,
  // need to detect the media before each rw. While some of
  // Usb-Flash is marked as removable media.
  //
  //
  if (Media->RemovableMedia == TRUE) {
    Status = UsbBootDetectMedia (UsbMass);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "UsbMassReadBlocks: UsbBootDetectMedia (%r)\n", Status));
      goto ON_EXIT;
    }
  }

  //
  // Make sure BlockSize and LBA is consistent with BufferSize
  //
  if ((BufferSize % Media->BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto ON_EXIT;
  }

  TotalBlock = BufferSize / Media->BlockSize;

  if (Lba + TotalBlock - 1 > Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto ON_EXIT;
  }

  if (MediaId != Media->MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto ON_EXIT;
  }

  Status = UsbBootReadBlocks (UsbMass, (UINT32) Lba, TotalBlock, Buffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbMassReadBlocks: UsbBootReadBlocks (%r) -> Reset\n", Status));
    UsbMassReset (This, TRUE);
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Write some blocks of data to the block device.

  @param  This                   The Block IO protocol
  @param  MediaId                The media's ID of the device for current request
  @param  Lba                    The start block number
  @param  BufferSize             The size of buffer to write data to
  @param  Buffer                 The buffer to write data to

  @retval EFI_SUCCESS            The data is successfully written
  @retval EFI_NO_MEDIA           Media isn't present
  @retval EFI_MEDIA_CHANGED      The device media has been changed, that is,
                                 MediaId changed
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid, such as Buffer is
                                 NULL.
  @retval EFI_BAD_BUFFER_SIZE    The buffer size isn't a multiple of media's block
                                 size,

**/
EFI_STATUS
EFIAPI
UsbMassWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer
  )
{
  USB_MASS_DEVICE     *UsbMass;
  EFI_BLOCK_IO_MEDIA  *Media;
  EFI_STATUS          Status;
  EFI_TPL             OldTpl;
  UINTN               TotalBlock;

  OldTpl  = gBS->RaiseTPL (USB_MASS_TPL);
  UsbMass = USB_MASS_DEVICE_FROM_BLOCKIO (This);
  Media   = &UsbMass->BlockIoMedia;

  //
  // First, validate the parameters
  //
  if ((Buffer == NULL) || (BufferSize == 0)) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  //
  // If it is a removable media, such as CD-Rom or Usb-Floppy,
  // need to detect the media before each rw. While some of
  // Usb-Flash is marked as removable media.
  //
  //
  if (Media->RemovableMedia == TRUE) {
    Status = UsbBootDetectMedia (UsbMass);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "UsbMassWriteBlocks: UsbBootDetectMedia (%r)\n", Status));
      goto ON_EXIT;
    }
  }

  //
  // Make sure BlockSize and LBA is consistent with BufferSize
  //
  if ((BufferSize % Media->BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto ON_EXIT;
  }

  TotalBlock = BufferSize / Media->BlockSize;

  if (Lba + TotalBlock - 1 > Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto ON_EXIT;
  }

  if (MediaId != Media->MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto ON_EXIT;
  }

  //
  // Try to write the data even the device is marked as ReadOnly,
  // and clear the status should the write succeed.
  //
  Status = UsbBootWriteBlocks (UsbMass, (UINT32) Lba, TotalBlock, Buffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbMassWriteBlocks: UsbBootWriteBlocks (%r) -> Reset\n", Status));
    UsbMassReset (This, TRUE);
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Flush the cached writes to disks. USB mass storage device doesn't
  support write cache, so return EFI_SUCCESS directly.

  @param  This                   The BLOCK IO protocol

  @retval EFI_SUCCESS            Always returns success

**/
EFI_STATUS
EFIAPI
UsbMassFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}

/**
  Retrieve the media parameters such as disk geometric for the
  device's BLOCK IO protocol.

  @param  UsbMass                The USB mass storage device

  @retval EFI_SUCCESS            The media parameters is updated successfully.
  @retval Others                 Failed to get the media parameters.

**/
EFI_STATUS
UsbMassInitMedia (
  IN USB_MASS_DEVICE          *UsbMass
  )
{
  EFI_BLOCK_IO_MEDIA          *Media;
  EFI_STATUS                  Status;
  UINTN                       Index;

  Media = &UsbMass->BlockIoMedia;

  //
  // Initialize the MediaPrsent/ReadOnly and others to the default.
  // We are not forced to get it right at this time, check UEFI2.0
  // spec for more information:
  //
  // MediaPresent: This field shows the media present status as
  //               of the most recent ReadBlocks or WriteBlocks call.
  //
  // ReadOnly    : This field shows the read-only status as of the
  //               recent WriteBlocks call.
  //
  // but remember to update MediaId/MediaPresent/ReadOnly status
  // after ReadBlocks and WriteBlocks
  //
  Media->MediaPresent     = FALSE;
  Media->LogicalPartition = FALSE;
  Media->ReadOnly         = FALSE;
  Media->WriteCaching     = FALSE;
  Media->IoAlign          = 0;
  Media->MediaId          = 1;

  //
  // Some device may spend several seconds before it is ready.
  // Try several times before giving up. Wait 5s at most.
  //
  Status = EFI_SUCCESS;

  for (Index = 0; Index < USB_BOOT_INIT_MEDIA_RETRY; Index++) {

    Status = UsbBootGetParams (UsbMass);
    if ((Status != EFI_MEDIA_CHANGED)
        && (Status != EFI_NOT_READY)
        && (Status != EFI_TIMEOUT)) {
      break;
    }

    Status = UsbBootIsUnitReady (UsbMass);
    if (EFI_ERROR (Status)) {
      gBS->Stall (USB_BOOT_RETRY_UNIT_READY_STALL * (Index + 1));
    }

  }

  return Status;
}

/**
  Initilize the transport.

  @param  This            The USB mass driver's driver binding.
  @param  Controller      The device to test.
  @param  Transport       The pointer to pointer to USB_MASS_TRANSPORT.
  @param  Context         The passing parameter.
  @param  MaxLun          Get the MaxLun if is BOT dev.

  @retval EFI_SUCCESS     The initialization is successful.
  @retval Others          Failed to initialize dev.

**/
EFI_STATUS
UsbMassInitTransport (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  OUT USB_MASS_TRANSPORT           **Transport,
  OUT VOID                         **Context,
  OUT UINT8                        *MaxLun
  )
{
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  Interface;
  UINT8                         Index;
  EFI_STATUS                    Status;
 
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbMassInitTransport: OpenUsbIoProtocol By Driver (%r)\n", Status));
    return Status;
  }
  
  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbMassInitTransport: UsbIo->UsbGetInterfaceDescriptor (%r)\n", Status));
    goto ON_EXIT;
  }
  
  Status = EFI_UNSUPPORTED;

  for (Index = 0; mUsbMassTransport[Index] != NULL; Index++) {
    *Transport = mUsbMassTransport[Index];

    if (Interface.InterfaceProtocol == (*Transport)->Protocol) {
      Status  = (*Transport)->Init (UsbIo, Context);
      break;
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbMassInitTransport: Transport->Init (%r)\n", Status));
    goto ON_EXIT;
  }

  //
  // For bot device, try to get max lun. 
  // If maxlun=0, then non-lun device, else multi-lun device.
  //
  if ((*Transport)->Protocol == USB_MASS_STORE_BOT) {
    (*Transport)->GetMaxLun (*Context, MaxLun);
    DEBUG ((EFI_D_INFO, "UsbMassInitTransport: GetMaxLun = %d\n", *MaxLun));
  }

ON_EXIT:
  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  return Status;  
}

/**
  Usb mass storage driver initializes multi lun.

  @param  This            The USB mass driver's driver binding.
  @param  Controller      The device to test.
  @param  Transport       The pointer to USB_MASS_TRANSPORT.
  @param  Context         The passing parameter.
  @param  DevicePath      The remaining device path
  @param  MaxLun          The MaxLun number passed.

  @retval EFI_SUCCESS     Initialization is success.
  @retval Other           Initialization fails.

**/
EFI_STATUS
UsbMassInitMultiLun (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    Controller,
  IN USB_MASS_TRANSPORT            *Transport,
  IN VOID                          *Context,
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath,
  IN UINT8                         MaxLun
  )
{
  USB_MASS_DEVICE                  *UsbMass;
  EFI_USB_IO_PROTOCOL              *UsbIo;
  DEVICE_LOGICAL_UNIT_DEVICE_PATH  LunNode;
  UINT8                            Index;
  EFI_STATUS                       Status;

  ASSERT (MaxLun > 0);

  for (Index = 0; Index <= MaxLun; Index++) { 

    DEBUG ((EFI_D_INFO, "UsbMassInitMultiLun: Start to initialize No.%d logic unit\n", Index));
    
    UsbIo   = NULL;
    UsbMass = AllocateZeroPool (sizeof (USB_MASS_DEVICE));
    if (UsbMass == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }
      
    UsbMass->Signature            = USB_MASS_SIGNATURE;
    UsbMass->UsbIo                = UsbIo;
    UsbMass->BlockIo.Media        = &UsbMass->BlockIoMedia;
    UsbMass->BlockIo.Reset        = UsbMassReset;
    UsbMass->BlockIo.ReadBlocks   = UsbMassReadBlocks;
    UsbMass->BlockIo.WriteBlocks  = UsbMassWriteBlocks;
    UsbMass->BlockIo.FlushBlocks  = UsbMassFlushBlocks;
    UsbMass->OpticalStorage       = FALSE;
    UsbMass->Transport            = Transport;
    UsbMass->Context              = Context;
    UsbMass->Lun                  = Index;
    
    //
    // Get the storage's parameters, such as last block number.
    // then install the BLOCK_IO
    //
    Status = UsbMassInitMedia (UsbMass);
    if (!EFI_ERROR (Status)) {
      if ((UsbMass->Pdt != USB_PDT_DIRECT_ACCESS) && 
           (UsbMass->Pdt != USB_PDT_CDROM) &&
           (UsbMass->Pdt != USB_PDT_OPTICAL) && 
           (UsbMass->Pdt != USB_PDT_SIMPLE_DIRECT)) {
        DEBUG ((EFI_D_ERROR, "UsbMassInitMultiLun: Found an unsupported peripheral type[%d]\n", UsbMass->Pdt));
        goto ON_ERROR;
      }
    } else if (Status != EFI_NO_MEDIA){
      DEBUG ((EFI_D_ERROR, "UsbMassInitMultiLun: UsbMassInitMedia (%r)\n", Status));
      goto ON_ERROR;
    }

    //
    // Create a device path node of device logic unit, and append it 
    //
    LunNode.Header.Type    = MESSAGING_DEVICE_PATH;
    LunNode.Header.SubType = MSG_DEVICE_LOGICAL_UNIT_DP;
    LunNode.Lun            = UsbMass->Lun;
  
    SetDevicePathNodeLength (&LunNode.Header, sizeof (LunNode));
  
    UsbMass->DevicePath = AppendDevicePathNode (DevicePath, &LunNode.Header);
  
    if (UsbMass->DevicePath == NULL) {
      DEBUG ((EFI_D_ERROR, "UsbMassInitMultiLun: failed to create device logic unit device path\n"));
  
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }

    //
    // Create a UsbMass handle for each lun, and install blockio and devicepath protocols.
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &UsbMass->Controller,
                    &gEfiDevicePathProtocolGuid,
                    UsbMass->DevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &UsbMass->BlockIo,
                    NULL
                    );
    
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "UsbMassInitMultiLun: InstallMultipleProtocolInterfaces (%r)\n", Status));
      goto ON_ERROR;
    }

    //
    // Open UsbIo protocol by child to setup a parent-child relationship.
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiUsbIoProtocolGuid,
                    (VOID **) &UsbIo,
                    This->DriverBindingHandle,
                    UsbMass->Controller,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "UsbMassInitMultiLun: OpenUsbIoProtocol By Child (%r)\n", Status));
      gBS->UninstallMultipleProtocolInterfaces (
             &UsbMass->Controller,
             &gEfiDevicePathProtocolGuid,
             UsbMass->DevicePath,
             &gEfiBlockIoProtocolGuid,
             &UsbMass->BlockIo,
             NULL
             );
      goto ON_ERROR;
    }
    
    DEBUG ((EFI_D_INFO, "UsbMassInitMultiLun: Success to initialize No.%d logic unit\n", Index));
  }
  
  return EFI_SUCCESS;

ON_ERROR:
  if (UsbMass->DevicePath != NULL) {
    gBS->FreePool (UsbMass->DevicePath);
  }
  if (UsbMass != NULL) {
    gBS->FreePool (UsbMass);
  }
  if (UsbIo != NULL) {
    gBS->CloseProtocol (
           Controller,
           &gEfiUsbIoProtocolGuid,
           This->DriverBindingHandle,
           UsbMass->Controller
           );
  }

  //
  // If only success to initialize one lun, return success, or else return error
  //
  if (Index > 0) {
    return EFI_SUCCESS; 
  } else {
    return Status;
  } 
}

/**
  Initialize No/Unsupported LUN device.

  @param This             The USB mass driver's driver binding.
  @param Controller       The device to test.
  @param Transport        The pointer to USB_MASS_TRANSPORT.
  @param Context          The passing parameter.

  @retval EFI_SUCCESS     Initialization is success.
  @retval Other           Initialization fails.

**/
EFI_STATUS
UsbMassInitNonLun (
  IN EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN EFI_HANDLE                    Controller,
  IN USB_MASS_TRANSPORT            *Transport,
  IN VOID                          *Context
  )
{
  USB_MASS_DEVICE             *UsbMass;
  EFI_USB_IO_PROTOCOL         *UsbIo;
  EFI_STATUS                  Status;

  UsbIo   = NULL;
  UsbMass = AllocateZeroPool (sizeof (USB_MASS_DEVICE));
  if (UsbMass == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbMassInitNonLun: OpenUsbIoProtocol By Driver (%r)\n", Status));
    goto ON_ERROR;
  }
  
  UsbMass->Signature            = USB_MASS_SIGNATURE;
  UsbMass->Controller           = Controller;
  UsbMass->UsbIo                = UsbIo;
  UsbMass->BlockIo.Media        = &UsbMass->BlockIoMedia;
  UsbMass->BlockIo.Reset        = UsbMassReset;
  UsbMass->BlockIo.ReadBlocks   = UsbMassReadBlocks;
  UsbMass->BlockIo.WriteBlocks  = UsbMassWriteBlocks;
  UsbMass->BlockIo.FlushBlocks  = UsbMassFlushBlocks;
  UsbMass->OpticalStorage       = FALSE;
  UsbMass->Transport            = Transport;
  UsbMass->Context              = Context;
  
  //
  // Get the storage's parameters, such as last block number.
  // then install the BLOCK_IO
  //
  Status = UsbMassInitMedia (UsbMass);
  if (!EFI_ERROR (Status)) {
    if ((UsbMass->Pdt != USB_PDT_DIRECT_ACCESS) && 
         (UsbMass->Pdt != USB_PDT_CDROM) &&
         (UsbMass->Pdt != USB_PDT_OPTICAL) && 
         (UsbMass->Pdt != USB_PDT_SIMPLE_DIRECT)) {
      DEBUG ((EFI_D_ERROR, "UsbMassInitNonLun: Found an unsupported peripheral type[%d]\n", UsbMass->Pdt));
      goto ON_ERROR;
    }
  } else if (Status != EFI_NO_MEDIA){
    DEBUG ((EFI_D_ERROR, "UsbMassInitNonLun: UsbMassInitMedia (%r)\n", Status));
    goto ON_ERROR;
  }
    
  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiBlockIoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &UsbMass->BlockIo
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  if (UsbMass != NULL) {
    gBS->FreePool (UsbMass);
  }
  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  return Status;  
}


/**
  Check whether the controller is a supported USB mass storage.

  @param  This                   The USB mass driver's driver binding.
  @param  Controller             The device to test against.
  @param  RemainingDevicePath    The remaining device path

  @retval EFI_SUCCESS            This device is a supported USB mass storage.
  @retval EFI_UNSUPPORTED        The device isn't supported
  @retval Others                 Some error happened.

**/
EFI_STATUS
EFIAPI
USBMassDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  Interface;
  USB_MASS_TRANSPORT            *Transport;
  EFI_STATUS                    Status;
  INTN                          Index;

  //
  // Check whether the controller support USB_IO
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the interface to check the USB class and find a transport
  // protocol handler.
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = EFI_UNSUPPORTED;

  if (Interface.InterfaceClass != USB_MASS_STORE_CLASS) {
    goto ON_EXIT;
  }

  for (Index = 0; mUsbMassTransport[Index] != NULL; Index++) {
    Transport = mUsbMassTransport[Index];
    if (Interface.InterfaceProtocol == Transport->Protocol) {
      Status = Transport->Init (UsbIo, NULL);
      break;
    }
  }

  DEBUG ((EFI_D_INFO, "Found a USB mass store device %r\n", Status));

ON_EXIT:
  gBS->CloseProtocol (
        Controller,
        &gEfiUsbIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}


/**
  Start the USB mass storage device on the controller. It will
  install a BLOCK_IO protocol on the device if everything is OK.

  @param  This                   The USB mass storage driver binding.
  @param  Controller             The USB mass storage device to start on
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver has started on the device.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory
  @retval Others                 Failed to start the driver on the device.

**/
EFI_STATUS
EFIAPI
USBMassDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  USB_MASS_TRANSPORT            *Transport;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  VOID                          *Context;
  UINT8                         MaxLun;
  EFI_STATUS                    Status;
  
  Transport = NULL;
  Context   = NULL;
  MaxLun    = 0;

  //
  // Get interface and protocols, initialize transport
  //
  Status = UsbMassInitTransport (This, Controller, &Transport, &Context, &MaxLun);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "USBMassDriverBindingStart: UsbMassInitTransport (%r)\n", Status));
    return Status;
  }
  if (MaxLun == 0) {
    //
    // Initialize No/Unsupported LUN device
    //
    Status = UsbMassInitNonLun(This, Controller, Transport, Context);
    if (EFI_ERROR (Status)) { 
      DEBUG ((EFI_D_ERROR, "USBMassDriverBindingStart: UsbMassInitNonLun (%r)\n", Status));
    }
  } else {
    //
    // Open device path to prepare append Device Logic Unit node.
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
      DEBUG ((EFI_D_ERROR, "USBMassDriverBindingStart: OpenDevicePathProtocol By Driver (%r)\n", Status));
      return Status;
    }

    //
    // Try best to initialize all LUNs, and return success only if one of LUNs successed to initialized.
    //
    Status = UsbMassInitMultiLun(This, Controller, Transport, Context, DevicePath, MaxLun);
    if (EFI_ERROR (Status)) {
     gBS->CloseProtocol (
            Controller,
            &gEfiDevicePathProtocolGuid,
            This->DriverBindingHandle,
            Controller
            );
      DEBUG ((EFI_D_ERROR, "USBMassDriverBindingStart: UsbMassInitMultiLun (%r) with Maxlun=%d\n", Status, MaxLun));
    }
  }
  return Status;
}


/**
  Stop controlling the device.

  @param  This                   The USB mass storage driver binding
  @param  Controller             The device controller controlled by the driver.
  @param  NumberOfChildren       The number of children of this device
  @param  ChildHandleBuffer      The buffer of children handle.

  @retval EFI_SUCCESS            The driver stopped from controlling the device.
  @retval Others                 Failed to stop the driver

**/
EFI_STATUS
EFIAPI
USBMassDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL *This,
  IN  EFI_HANDLE                  Controller,
  IN  UINTN                       NumberOfChildren,
  IN  EFI_HANDLE                  *ChildHandleBuffer
  )
{
  EFI_STATUS            Status;
  USB_MASS_DEVICE       *UsbMass;
  EFI_USB_IO_PROTOCOL     *UsbIo;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;
  UINTN                   Index;
  BOOLEAN                 AllChildrenStopped;

  //
  // This a bus driver stop function since multi-lun supported. There are three 
  // kinds of device handle might be passed, 1st is a handle with devicepath/
  // usbio/blockio installed(non-multi-lun), 2nd is a handle with devicepath/
  // usbio installed(multi-lun root), 3rd is a handle with devicepath/blockio
  // installed(multi-lun).
  //
  if (NumberOfChildren == 0) {
    //
    // A handle without any children, might be 1st and 2nd type.
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlockIo,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
  
    if (EFI_ERROR(Status)) {
      //
      // This is a 2nd type handle(multi-lun root), which only needs close 
      // devicepath protocol.
      //
      gBS->CloseProtocol (
            Controller,
            &gEfiDevicePathProtocolGuid,
            This->DriverBindingHandle,
            Controller
            );
      DEBUG ((EFI_D_INFO, "Success to stop multi-lun root handle\n"));
      return EFI_SUCCESS;
    }
    
    //
    // This is a 1st type handle(non-multi-lun), which only needs uninstall
    // blockio protocol, close usbio protocol and free mass device.
    //
    UsbMass = USB_MASS_DEVICE_FROM_BLOCKIO (BlockIo);
  
    //
    // Uninstall Block I/O protocol from the device handle,
    // then call the transport protocol to stop itself.
    //
    Status = gBS->UninstallProtocolInterface (
                    Controller,
                    &gEfiBlockIoProtocolGuid,
                    &UsbMass->BlockIo
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
  
    UsbMass->Transport->Fini (UsbMass->Context);
    gBS->FreePool (UsbMass);
    
    DEBUG ((EFI_D_INFO, "Success to stop non-multi-lun root handle\n"));
    return EFI_SUCCESS;
  } 

  //
  // This is a 3rd type handle(multi-lun), which needs uninstall
  // blockio and devicepath protocol, close usbio protocol and 
  // free mass device.
  //
  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {

    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlockIo,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
      DEBUG ((EFI_D_ERROR, "Fail to stop No.%d multi-lun child handle when opening blockio\n", Index));
      continue;
    }

    UsbMass = USB_MASS_DEVICE_FROM_BLOCKIO (BlockIo);

    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          ChildHandleBuffer[Index]
          );
  
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    ChildHandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    UsbMass->DevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &UsbMass->BlockIo,
                    NULL
                    );
    
    if (EFI_ERROR (Status)) {
      //
      // Fail to uninstall blockio and devicepath protocol, so re-open usbio by child.
      //
      AllChildrenStopped = FALSE;
      DEBUG ((EFI_D_ERROR, "Fail to stop No.%d multi-lun child handle when uninstalling blockio and devicepath\n", Index));
      
      gBS->OpenProtocol (
             Controller,
             &gEfiUsbIoProtocolGuid,
             (VOID **) &UsbIo,
             This->DriverBindingHandle,
             ChildHandleBuffer[Index],
             EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
             );
    } else {
      //
      // Success to stop this multi-lun handle, so go on next child.
      //
      if (((Index + 1) == NumberOfChildren) && AllChildrenStopped) {
        UsbMass->Transport->Fini (UsbMass->Context);
      }
      gBS->FreePool (UsbMass);
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }
  
  DEBUG ((EFI_D_INFO, "Success to stop all %d multi-lun children handles\n", NumberOfChildren));
  return EFI_SUCCESS;
}

EFI_DRIVER_BINDING_PROTOCOL gUSBMassDriverBinding = {
  USBMassDriverBindingSupported,
  USBMassDriverBindingStart,
  USBMassDriverBindingStop,
  0x11,
  NULL,
  NULL
};

/**
  The entry point for the driver, which will install the driver binding and
  component name protocol.

  @param  ImageHandle       The image handle of this driver.
  @param  SystemTable       The system table.

  @retval EFI_SUCCESS       The protocols are installed OK.
  @retval Others            Failed to install protocols.

**/
EFI_STATUS
EFIAPI
USBMassStorageEntryPoint (
  IN EFI_HANDLE               ImageHandle,
  IN EFI_SYSTEM_TABLE         *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install driver binding protocol
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUSBMassDriverBinding,
             ImageHandle,
             &gUsbMassStorageComponentName,
             &gUsbMassStorageComponentName2
             );

  return Status;
}
