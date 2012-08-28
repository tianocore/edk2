/** @file
  The driver wrappers BlockMmio protocol instances to produce
  Block I/O Protocol instances.

  Copyright (c) 2007 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BlockIo.h"

EFI_DRIVER_BINDING_PROTOCOL gBlockIoDriverBinding = {
  BlockIoDriverBindingSupported,
  BlockIoDriverBindingStart,
  BlockIoDriverBindingStop,
  0x11,
  NULL,
  NULL
};

/**
  Reset the block device.

  This function implements EFI_BLOCK_IO_PROTOCOL.Reset(). 
  It resets the block device hardware.
  ExtendedVerification is ignored in this implementation.

  @param  This                   Indicates a pointer to the calling context.
  @param  ExtendedVerification   Indicates that the driver may perform a more exhaustive
                                 verification operation of the device during reset.

  @retval EFI_SUCCESS            The block device was reset.
  @retval EFI_DEVICE_ERROR       The block device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
BlockIoReset (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ExtendedVerification
  )
{
  return EFI_SUCCESS;
}

/**
  Reads the requested number of blocks from the device.

  This function implements EFI_BLOCK_IO_PROTOCOL.ReadBlocks(). 
  It reads the requested number of blocks from the device.
  All the blocks are read, or an error is returned.

  @param  This                   Indicates a pointer to the calling context.
  @param  ReadData               If TRUE then read data.  If FALSE then write data.
  @param  MediaId                The media ID that the read request is for.
  @param  Lba                    The starting logical block address to read from on the device.
  @param  BufferSize             The size of the Buffer in bytes.
                                 This must be a multiple of the intrinsic block size of the device.
  @param  Buffer                 A pointer to the destination buffer for the data. The caller is
                                 responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS            The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to perform the read operation.
  @retval EFI_NO_MEDIA           There is no media in the device.
  @retval EFI_MEDIA_CHANGED      The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE    The BufferSize parameter is not a multiple of the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER  The read request contains LBAs that are not valid,
                                 or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
ReadOrWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ReadData,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  OUT VOID                    *Buffer
  )
{
  EFI_STATUS                    Status;
  BLOCK_MMIO_TO_BLOCK_IO_DEVICE *Private;
  UINTN                         TotalBlock;
  EFI_BLOCK_IO_MEDIA            *Media;
  UINT64                        Address;
  UINTN                         Count;
  EFI_CPU_IO_PROTOCOL_IO_MEM    CpuAccessFunction;

  //
  // First, validate the parameters
  //
  if ((Buffer == NULL) || (BufferSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get private data structure
  //
  Private = PRIVATE_FROM_BLOCK_IO (This);
  Media   = Private->BlockMmio->Media;

  //
  // BufferSize must be a multiple of the intrinsic block size of the device.
  //
  if (ModU64x32 (BufferSize, Media->BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  TotalBlock = (UINTN) DivU64x32 (BufferSize, Media->BlockSize);

  //
  // Make sure the range to read is valid.
  //
  if (Lba + TotalBlock - 1 > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if (!(Media->MediaPresent)) {
    return EFI_NO_MEDIA;
  }

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  Address = Private->BlockMmio->BaseAddress;
  Address += MultU64x32 (Lba, Media->BlockSize);

  Count = BufferSize >> 3;

  if (ReadData) {
    CpuAccessFunction = Private->CpuIo->Mem.Read;
  } else {
    CpuAccessFunction = Private->CpuIo->Mem.Write;
  }

  Status = (CpuAccessFunction) (
             Private->CpuIo,
             EfiCpuIoWidthUint64,
             Address,
             Count,
             Buffer
             );

  return Status;
}


/**
  Reads the requested number of blocks from the device.

  This function implements EFI_BLOCK_IO_PROTOCOL.ReadBlocks(). 
  It reads the requested number of blocks from the device.
  All the blocks are read, or an error is returned.

  @param  This                   Indicates a pointer to the calling context.
  @param  MediaId                The media ID that the read request is for.
  @param  Lba                    The starting logical block address to read from on the device.
  @param  BufferSize             The size of the Buffer in bytes.
                                 This must be a multiple of the intrinsic block size of the device.
  @param  Buffer                 A pointer to the destination buffer for the data. The caller is
                                 responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS            The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to perform the read operation.
  @retval EFI_NO_MEDIA           There is no media in the device.
  @retval EFI_MEDIA_CHANGED      The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE    The BufferSize parameter is not a multiple of the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER  The read request contains LBAs that are not valid,
                                 or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
BlockIoReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  OUT VOID                    *Buffer
  )
{
  DEBUG ((EFI_D_INFO, "BlockIo (MMIO) ReadBlocks: lba=0x%lx, size=0x%x\n", Lba, BufferSize));
  return ReadOrWriteBlocks (
    This,
    TRUE,
    MediaId,
    Lba,
    BufferSize,
    Buffer
    );
}


/**
  Writes a specified number of blocks to the device.

  This function implements EFI_BLOCK_IO_PROTOCOL.WriteBlocks(). 
  It writes a specified number of blocks to the device.
  All blocks are written, or an error is returned.

  @param  This                   Indicates a pointer to the calling context.
  @param  MediaId                The media ID that the write request is for.
  @param  Lba                    The starting logical block address to be written.
  @param  BufferSize             The size of the Buffer in bytes.
                                 This must be a multiple of the intrinsic block size of the device.
  @param  Buffer                 Pointer to the source buffer for the data.

  @retval EFI_SUCCESS            The data were written correctly to the device.
  @retval EFI_WRITE_PROTECTED    The device cannot be written to.
  @retval EFI_NO_MEDIA           There is no media in the device.
  @retval EFI_MEDIA_CHANGED      The MediaId is not for the current media.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to perform the write operation.
  @retval EFI_BAD_BUFFER_SIZE    The BufferSize parameter is not a multiple of the intrinsic
                                 block size of the device.
  @retval EFI_INVALID_PARAMETER  The write request contains LBAs that are not valid,
                                 or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
BlockIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINT32                   MediaId,
  IN EFI_LBA                  Lba,
  IN UINTN                    BufferSize,
  IN VOID                     *Buffer
  )
{
  DEBUG ((EFI_D_INFO, "BlockIo (MMIO) WriteBlocks: lba=0x%lx, size=0x%x\n", Lba, BufferSize));
  return ReadOrWriteBlocks (
    This,
    FALSE,
    MediaId,
    Lba,
    BufferSize,
    Buffer
    );
}

/**
  Flushes all modified data to a physical block device.

  @param  This                   Indicates a pointer to the calling context.

  @retval EFI_SUCCESS            All outstanding data were written correctly to the device.
  @retval EFI_DEVICE_ERROR       The device reported an error while attempting to write data.
  @retval EFI_NO_MEDIA           There is no media in the device.

**/
EFI_STATUS
EFIAPI
BlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}


/**
  Initialize data for device that does not support multiple LUNSs.

  @param  This            The Driver Binding Protocol instance.
  @param  Controller      The device to initialize.
  @param  BlockMmio       Pointer to USB_MASS_TRANSPORT.
  @param  Context         Parameter for USB_MASS_DEVICE.Context.

  @retval EFI_SUCCESS     Initialization succeeds.
  @retval Other           Initialization fails.

**/
EFI_STATUS
BlockIoInit (
  IN EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN EFI_HANDLE                    Controller
  )
{
  EFI_STATUS                     Status;
  BLOCK_MMIO_TO_BLOCK_IO_DEVICE  *Private;
  BLOCK_MMIO_PROTOCOL            *BlockMmio;

  Private = (BLOCK_MMIO_TO_BLOCK_IO_DEVICE*) AllocateZeroPool (sizeof (*Private));
  ASSERT (Private != NULL);

  Status = gBS->LocateProtocol (
                  &gEfiCpuIo2ProtocolGuid,
                  NULL,
                  (VOID **) &(Private->CpuIo)
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->OpenProtocol (
                  Controller,
                  &gBlockMmioProtocolGuid,
                  (VOID **) &BlockMmio,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "BlockIoInit: OpenBlockMmioProtocol By Driver (%r)\n", Status));
    goto ON_ERROR;
  }
  DEBUG ((EFI_D_INFO, "BlockMmio: 0x%x\n", BlockMmio));
  DEBUG ((EFI_D_INFO, "BlockMmio->Media->LastBlock: 0x%lx\n", BlockMmio->Media->LastBlock));
  
  Private->Signature            = BLOCK_MMIO_TO_BLOCK_IO_SIGNATURE;
  Private->Controller           = Controller;
  Private->BlockMmio            = BlockMmio;
  Private->BlockIo.Media        = BlockMmio->Media;
  Private->BlockIo.Reset        = BlockIoReset;
  Private->BlockIo.ReadBlocks   = BlockIoReadBlocks;
  Private->BlockIo.WriteBlocks  = BlockIoWriteBlocks;
  Private->BlockIo.FlushBlocks  = BlockIoFlushBlocks;

  DEBUG ((EFI_D_INFO, "Private->BlockIo.Media->LastBlock: 0x%lx\n", Private->BlockIo.Media->LastBlock));

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiBlockIoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->BlockIo
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  if (Private != NULL) {
    FreePool (Private);
  }
  if (BlockMmio != NULL) {
    gBS->CloseProtocol (
           Controller,
           &gBlockMmioProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }
  return Status;  
}


/**
  Check whether the controller is a supported USB mass storage.

  @param  This                   The USB mass storage driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
BlockIoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                    Status;
  BLOCK_MMIO_PROTOCOL           *BlockMmio;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gBlockMmioProtocolGuid,
                  (VOID **) &BlockMmio,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gBlockMmioProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Starts the USB mass storage device with this driver.

  This function consumes USB I/O Portocol, intializes USB mass storage device,
  installs Block I/O Protocol, and submits Asynchronous Interrupt
  Transfer to manage the USB mass storage device.

  @param  This                  The USB mass storage driver binding protocol.
  @param  Controller            The USB mass storage device to start on
  @param  RemainingDevicePath   The remaining device path.

  @retval EFI_SUCCESS           This driver supports this device.
  @retval EFI_UNSUPPORTED       This driver does not support this device.
  @retval EFI_DEVICE_ERROR      This driver cannot be started due to device Error.
  @retval EFI_OUT_OF_RESOURCES  Can't allocate memory resources.
  @retval EFI_ALREADY_STARTED   This driver has been started.

**/
EFI_STATUS
EFIAPI
BlockIoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                    Status;
  
  Status = BlockIoInit (This, Controller);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "BlockIoDriverBindingStart: BlockIoInit (%r)\n", Status));
    return Status;
  }

  DEBUG ((EFI_D_INIT, "BlockIoDriverBindingStart: Successfully started\n"));
  return Status;
}


/**
  Stop controlling the device.

  @param  This                   The USB mass storage driver binding
  @param  Controller             The device controller controlled by the driver.
  @param  NumberOfChildren       The number of children of this device
  @param  ChildHandleBuffer      The buffer of children handle.

  @retval EFI_SUCCESS            The driver stopped from controlling the device.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a device error.
  @retval EFI_UNSUPPORTED        Block I/O Protocol is not installed on Controller.
  @retval Others                 Failed to stop the driver

**/
EFI_STATUS
EFIAPI
BlockIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL *This,
  IN  EFI_HANDLE                  Controller,
  IN  UINTN                       NumberOfChildren,
  IN  EFI_HANDLE                  *ChildHandleBuffer
  )
{
  EFI_STATUS                    Status;
  BLOCK_MMIO_TO_BLOCK_IO_DEVICE *Private;

  Private = PRIVATE_FROM_BLOCK_IO (This);

  //
  // Uninstall Block I/O protocol from the device handle,
  // then call the transport protocol to stop itself.
  //
  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  &Private->BlockIo
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
        Controller,
        &gBlockMmioProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  FreePool (Private);
  
  DEBUG ((EFI_D_INFO, "Successfully stopped BlockIo on BlockMmio\n"));
  return EFI_SUCCESS;
}

/**
  Entrypoint of Block MMIO to Block IO Driver.

  This function is the entrypoint of USB Mass Storage Driver. It installs Driver Binding
  Protocol together with Component Name Protocols.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
BlockMmioToBlockIoEntryPoint (
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
             &gBlockIoDriverBinding,
             ImageHandle,
             &gBlockMmioToBlockIoComponentName,
             &gBlockMmioToBlockIoComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
