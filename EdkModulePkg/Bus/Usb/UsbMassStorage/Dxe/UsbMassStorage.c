/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UsbMassStorage.c

Abstract:

  USB Mass Storage Driver

Revision History

--*/

#include "UsbMassStorage.h"
#include "UsbMassStorageHelper.h"

//
// Block I/O Protocol Interface
//
STATIC
EFI_STATUS
EFIAPI
USBFloppyReset (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  BOOLEAN                 ExtendedVerification
  );

STATIC
EFI_STATUS
EFIAPI
USBFloppyReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  );

STATIC
EFI_STATUS
EFIAPI
USBFloppyWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  );

STATIC
EFI_STATUS
EFIAPI
USBFloppyFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  );

//
// USB Floppy Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL         gUSBFloppyDriverBinding = {
  USBFloppyDriverBindingSupported,
  USBFloppyDriverBindingStart,
  USBFloppyDriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
USBFloppyDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:
    Test to see if this driver supports ControllerHandle. Any ControllerHandle
    that has UsbHcProtocol installed will be supported.

  Arguments:
    This                - Protocol instance pointer.
    Controller         - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.

--*/
{
  EFI_STATUS              OpenStatus;
  EFI_USB_ATAPI_PROTOCOL  *AtapiProtocol;

  //
  // check whether EFI_USB_ATAPI_PROTOCOL exists, if it does,
  // then the controller must be a USB Mass Storage Controller
  //
  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiUsbAtapiProtocolGuid,
                      (VOID **) &AtapiProtocol,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );
  if (EFI_ERROR (OpenStatus)) {
    return OpenStatus;
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiUsbAtapiProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
USBFloppyDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:
    Starting the Usb Bus Driver

  Arguments:
    This                - Protocol instance pointer.
    Controller          - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.
    EFI_DEVICE_ERROR    - This driver cannot be started due to device
                          Error
    EFI_OUT_OF_RESOURCES- Can't allocate memory resources
    EFI_ALREADY_STARTED - Thios driver has been started
--*/
{
  EFI_STATUS                Status;
  EFI_USB_ATAPI_PROTOCOL    *AtapiProtocol;
  USB_FLOPPY_DEV            *UsbFloppyDevice;

  UsbFloppyDevice = NULL;
  //
  // Check whether Usb Atapi Protocol attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbAtapiProtocolGuid,
                  (VOID **) &AtapiProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (USB_FLOPPY_DEV),
                  (VOID **) &UsbFloppyDevice
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbAtapiProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return Status;
  }

  ZeroMem (UsbFloppyDevice, sizeof (USB_FLOPPY_DEV));

  UsbFloppyDevice->Handle             = Controller;
  UsbFloppyDevice->BlkIo.Media        = &UsbFloppyDevice->BlkMedia;
  UsbFloppyDevice->Signature          = USB_FLOPPY_DEV_SIGNATURE;
  UsbFloppyDevice->BlkIo.Reset        = USBFloppyReset;
  UsbFloppyDevice->BlkIo.ReadBlocks   = USBFloppyReadBlocks;
  UsbFloppyDevice->BlkIo.WriteBlocks  = USBFloppyWriteBlocks;
  UsbFloppyDevice->BlkIo.FlushBlocks  = USBFloppyFlushBlocks;
  UsbFloppyDevice->AtapiProtocol      = AtapiProtocol;

  //
  // Identify drive type and retrieve media information.
  //
  Status = USBFloppyIdentify (UsbFloppyDevice);
  if (EFI_ERROR (Status)) {
    if (UsbFloppyDevice->SenseData != NULL) {
      gBS->FreePool (UsbFloppyDevice->SenseData);
    }

    gBS->FreePool (UsbFloppyDevice);
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbAtapiProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return Status;
  }
  //
  // Install Block I/O protocol for the usb floppy device.
  //
  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiBlockIoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &UsbFloppyDevice->BlkIo
                  );
  if (EFI_ERROR (Status)) {
    if (UsbFloppyDevice->SenseData != NULL) {
      gBS->FreePool (UsbFloppyDevice->SenseData);
    }

    gBS->FreePool (UsbFloppyDevice);
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbAtapiProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return Status;
  }

  return EFI_SUCCESS;

}


EFI_STATUS
EFIAPI
USBFloppyDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
/*++

  Routine Description:
    Stop this driver on ControllerHandle. Support stoping any child handles
    created by this driver.

  Arguments:
    This              - Protocol instance pointer.
    Controller        - Handle of device to stop driver on
    NumberOfChildren  - Number of Children in the ChildHandleBuffer
    ChildHandleBuffer - List of handles for the children we need to stop.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    others

--*/
{
  EFI_STATUS            Status;
  USB_FLOPPY_DEV        *UsbFloppyDevice;
  EFI_BLOCK_IO_PROTOCOL *BlkIo;

  //
  // First find USB_FLOPPY_DEV
  //
  gBS->OpenProtocol (
        Controller,
        &gEfiBlockIoProtocolGuid,
        (VOID **) &BlkIo,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );

  UsbFloppyDevice = USB_FLOPPY_DEV_FROM_THIS (BlkIo);

  //
  // Uninstall Block I/O protocol from the device handle
  //
  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  &UsbFloppyDevice->BlkIo
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Stop using EFI_USB_ATAPI_PROTOCOL
  //
  gBS->CloseProtocol (
        Controller,
        &gEfiUsbAtapiProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  if (UsbFloppyDevice->SenseData != NULL) {
    gBS->FreePool (UsbFloppyDevice->SenseData);
  }

  gBS->FreePool (UsbFloppyDevice);

  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
EFIAPI
USBFloppyReset (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  BOOLEAN                 ExtendedVerification
  )
/*++

  Routine Description:
    Implements EFI_BLOCK_IO_PROTOCOL.Reset() function.

  Arguments:
    This     The EFI_BLOCK_IO_PROTOCOL instance.
    ExtendedVerification
              Indicates that the driver may perform a more exhaustive
              verification operation of the device during reset.
              (This parameter is ingored in this driver.)

  Returns:
    EFI_SUCCESS - Success
--*/
{
  USB_FLOPPY_DEV          *UsbFloppyDevice;
  EFI_USB_ATAPI_PROTOCOL  *UsbAtapiInterface;
  EFI_STATUS              Status;
  EFI_TPL                 OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  UsbFloppyDevice   = USB_FLOPPY_DEV_FROM_THIS (This);

  UsbAtapiInterface = UsbFloppyDevice->AtapiProtocol;

  //
  // directly calling EFI_USB_ATAPI_PROTOCOL.Reset() to implement reset.
  //
  Status = UsbAtapiInterface->UsbAtapiReset (UsbAtapiInterface, ExtendedVerification);

  gBS->RestoreTPL (OldTpl);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
USBFloppyReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  )
/*++

  Routine Description:
    Implements EFI_BLOCK_IO_PROTOCOL.ReadBlocks() function.

  Arguments:
    This     The EFI_BLOCK_IO_PROTOCOL instance.
    MediaId  The media id that the read request is for.
    LBA      The starting logical block address to read from on the device.
    BufferSize
              The size of the Buffer in bytes. This must be a multiple of
              the intrinsic block size of the device.
    Buffer    A pointer to the destination buffer for the data. The caller
              is responsible for either having implicit or explicit ownership
              of the buffer.

  Returns:
    EFI_INVALID_PARAMETER - Parameter is error
    EFI_SUCCESS           - Success
    EFI_DEVICE_ERROR      - Hardware Error
    EFI_NO_MEDIA          - No media
    EFI_MEDIA_CHANGED     - Media Change
    EFI_BAD_BUFFER_SIZE   - Buffer size is bad
 --*/
{
  USB_FLOPPY_DEV      *UsbFloppyDevice;
  EFI_STATUS          Status;
  EFI_BLOCK_IO_MEDIA  *Media;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  BOOLEAN             MediaChange;
  EFI_TPL             OldTpl;

  Status          = EFI_SUCCESS;
  MediaChange     = FALSE;
  UsbFloppyDevice = USB_FLOPPY_DEV_FROM_THIS (This);

  //
  // Check parameters
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  UsbFloppyTestUnitReady (UsbFloppyDevice);

  Status = UsbFloppyDetectMedia (UsbFloppyDevice, &MediaChange);
  if (EFI_ERROR (Status)) {

    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  if (MediaChange) {
    gBS->ReinstallProtocolInterface (
          UsbFloppyDevice->Handle,
          &gEfiBlockIoProtocolGuid,
          &UsbFloppyDevice->BlkIo,
          &UsbFloppyDevice->BlkIo
          );
  }

  Media           = UsbFloppyDevice->BlkIo.Media;
  BlockSize       = Media->BlockSize;
  NumberOfBlocks  = BufferSize / BlockSize;

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if (MediaId != Media->MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Done;
  }

  if (BufferSize % BlockSize != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Done;
  }

  if (LBA > Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if ((LBA + NumberOfBlocks - 1) > Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if ((Media->IoAlign > 1) && (((UINTN) Buffer & (Media->IoAlign - 1)) != 0)) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  while (NumberOfBlocks > 0) {

    if (NumberOfBlocks > BLOCK_UNIT) {
      Status = USBFloppyRead10 (UsbFloppyDevice, Buffer, LBA, BLOCK_UNIT);
    } else {
      Status = USBFloppyRead10 (UsbFloppyDevice, Buffer, LBA, NumberOfBlocks);
    }

    if (EFI_ERROR (Status)) {
      This->Reset (This, TRUE);
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    if (NumberOfBlocks > BLOCK_UNIT) {
       NumberOfBlocks -= BLOCK_UNIT;
       LBA += BLOCK_UNIT;
       Buffer = (UINT8 *) Buffer + This->Media->BlockSize * BLOCK_UNIT;
    } else {
       NumberOfBlocks -= NumberOfBlocks;
       LBA += NumberOfBlocks;
       Buffer = (UINT8 *) Buffer + This->Media->BlockSize * NumberOfBlocks;
    }
 }

 Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
USBFloppyWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )
/*++

  Routine Description:
    Implements EFI_BLOCK_IO_PROTOCOL.WriteBlocks() function.

  Arguments:
    This     The EFI_BLOCK_IO_PROTOCOL instance.
    MediaId  The media id that the write request is for.
    LBA      The starting logical block address to be written.
             The caller is responsible for writing to only
             legitimate locations.
    BufferSize
              The size of the Buffer in bytes. This must be a multiple of
              the intrinsic block size of the device.
    Buffer    A pointer to the source buffer for the data. The caller
              is responsible for either having implicit or explicit ownership
              of the buffer.

  Returns:
    EFI_INVALID_PARAMETER - Parameter is error
    EFI_SUCCESS           - Success
    EFI_DEVICE_ERROR      - Hardware Error
    EFI_NO_MEDIA          - No media
    EFI_MEDIA_CHANGED     - Media Change
    EFI_BAD_BUFFER_SIZE   - Buffer size is bad

--*/
{
  USB_FLOPPY_DEV      *UsbFloppyDevice;
  EFI_STATUS          Status;
  EFI_BLOCK_IO_MEDIA  *Media;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  BOOLEAN             MediaChange;
  EFI_TPL             OldTpl;

  Status          = EFI_SUCCESS;
  MediaChange     = FALSE;

  UsbFloppyDevice = USB_FLOPPY_DEV_FROM_THIS (This);

  //
  // Check parameters
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  UsbFloppyTestUnitReady (UsbFloppyDevice);

  Status = UsbFloppyDetectMedia (UsbFloppyDevice, &MediaChange);
  if (EFI_ERROR (Status)) {

    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  if (MediaChange) {
    gBS->ReinstallProtocolInterface (
          UsbFloppyDevice->Handle,
          &gEfiBlockIoProtocolGuid,
          &UsbFloppyDevice->BlkIo,
          &UsbFloppyDevice->BlkIo
          );
  }

  Media           = UsbFloppyDevice->BlkIo.Media;
  BlockSize       = Media->BlockSize;
  NumberOfBlocks  = BufferSize / BlockSize;

  if (!(Media->MediaPresent)) {
    Status = EFI_NO_MEDIA;
    goto Done;
  }

  if (MediaId != Media->MediaId) {
    Status = EFI_MEDIA_CHANGED;
    goto Done;
  }

  if (BufferSize % BlockSize != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Done;
  }

  if (LBA > Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if ((LBA + NumberOfBlocks - 1) > Media->LastBlock) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if ((Media->IoAlign > 1) && (((UINTN) Buffer & (Media->IoAlign - 1)) != 0)) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (UsbFloppyDevice->BlkMedia.ReadOnly) {
    Status = EFI_WRITE_PROTECTED;
    goto Done;
  }

  while (NumberOfBlocks > 0) {

    if (NumberOfBlocks > BLOCK_UNIT) {
      Status = USBFloppyWrite10 (UsbFloppyDevice, Buffer, LBA, BLOCK_UNIT);
    } else {
      Status = USBFloppyWrite10 (UsbFloppyDevice, Buffer, LBA, NumberOfBlocks);
    }

    if (EFI_ERROR (Status)) {
      This->Reset (This, TRUE);
      Status = EFI_DEVICE_ERROR;
      goto Done;
    }

    if (NumberOfBlocks > BLOCK_UNIT) {
       NumberOfBlocks -= BLOCK_UNIT;
       LBA += BLOCK_UNIT;
       Buffer = (UINT8 *) Buffer + This->Media->BlockSize * BLOCK_UNIT;
    } else {
       NumberOfBlocks -= NumberOfBlocks;
       LBA += NumberOfBlocks;
       Buffer = (UINT8 *) Buffer + This->Media->BlockSize * NumberOfBlocks;
    }
 }

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
USBFloppyFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  )
/*++

  Routine Description:
    Implements EFI_BLOCK_IO_PROTOCOL.FlushBlocks() function.
    (In this driver, this function just returns EFI_SUCCESS.)

  Arguments:
    This     The EFI_BLOCK_IO_PROTOCOL instance.

  Returns:
    EFI_SUCCESS - Success
--*/
{
  return EFI_SUCCESS;
}
