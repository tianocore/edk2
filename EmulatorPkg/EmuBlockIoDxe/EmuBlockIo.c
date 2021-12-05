/**@file

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
bbe
**/

#include "EmuBlockIo.h"

/**
  Reset the block device hardware.

  @param[in]  This                 Indicates a pointer to the calling context.
  @param[in]  ExtendedVerification Indicates that the driver may perform a more
                                   exhausive verfication operation of the device
                                   during reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
EmuBlockIo2Reset (
  IN EFI_BLOCK_IO2_PROTOCOL  *This,
  IN BOOLEAN                 ExtendedVerification
  )
{
  EFI_STATUS            Status;
  EMU_BLOCK_IO_PRIVATE  *Private;
  EFI_TPL               OldTpl;

  Private = EMU_BLOCK_IO2_PRIVATE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Status = Private->Io->Reset (Private->Io, ExtendedVerification);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Read BufferSize bytes from Lba into Buffer.

  This function reads the requested number of blocks from the device. All the
  blocks are read, or an error is returned.
  If EFI_DEVICE_ERROR, EFI_NO_MEDIA,_or EFI_MEDIA_CHANGED is returned and
  non-blocking I/O is being used, the Event associated with this request will
  not be signaled.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    Id of the media, changes every time the media is
                              replaced.
  @param[in]       Lba        The starting Logical Block Address to read from.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[out]      Buffer     A pointer to the destination buffer for the data. The
                              caller is responsible for either having implicit or
                              explicit ownership of the buffer.

  @retval EFI_SUCCESS           The read request was queued if Token->Event is
                                not NULL.The data was read correctly from the
                                device if the Token->Event is NULL.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing
                                the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of the
                                intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack
                                of resources.
**/
EFI_STATUS
EFIAPI
EmuBlockIo2ReadBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN     UINT32                  MediaId,
  IN     EFI_LBA                 LBA,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token,
  IN     UINTN                   BufferSize,
  OUT VOID                       *Buffer
  )
{
  EFI_STATUS            Status;
  EMU_BLOCK_IO_PRIVATE  *Private;
  EFI_TPL               OldTpl;

  Private = EMU_BLOCK_IO2_PRIVATE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Status = Private->Io->ReadBlocks (Private->Io, MediaId, LBA, Token, BufferSize, Buffer);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Write BufferSize bytes from Lba into Buffer.

  This function writes the requested number of blocks to the device. All blocks
  are written, or an error is returned.If EFI_DEVICE_ERROR, EFI_NO_MEDIA,
  EFI_WRITE_PROTECTED or EFI_MEDIA_CHANGED is returned and non-blocking I/O is
  being used, the Event associated with this request will not be signaled.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    The media ID that the write request is for.
  @param[in]       Lba        The starting logical block address to be written. The
                              caller is responsible for writing to only legitimate
                              locations.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[in]       Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The write request was queued if Event is not NULL.
                                The data was written correctly to the device if
                                the Event is NULL.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack
                                of resources.

**/
EFI_STATUS
EFIAPI
EmuBlockIo2WriteBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN     UINT32                  MediaId,
  IN     EFI_LBA                 LBA,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token,
  IN     UINTN                   BufferSize,
  IN     VOID                    *Buffer
  )
{
  EFI_STATUS            Status;
  EMU_BLOCK_IO_PRIVATE  *Private;
  EFI_TPL               OldTpl;

  Private = EMU_BLOCK_IO2_PRIVATE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Status = Private->Io->WriteBlocks (Private->Io, MediaId, LBA, Token, BufferSize, Buffer);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Flush the Block Device.

  If EFI_DEVICE_ERROR, EFI_NO_MEDIA,_EFI_WRITE_PROTECTED or EFI_MEDIA_CHANGED
  is returned and non-blocking I/O is being used, the Event associated with
  this request will not be signaled.

  @param[in]      This     Indicates a pointer to the calling context.
  @param[in,out]  Token    A pointer to the token associated with the transaction

  @retval EFI_SUCCESS          The flush request was queued if Event is not NULL.
                               All outstanding data was written correctly to the
                               device if the Event is NULL.
  @retval EFI_DEVICE_ERROR     The device reported an error while writting back
                               the data.
  @retval EFI_WRITE_PROTECTED  The device cannot be written to.
  @retval EFI_NO_MEDIA         There is no media in the device.
  @retval EFI_MEDIA_CHANGED    The MediaId is not for the current media.
  @retval EFI_OUT_OF_RESOURCES The request could not be completed due to a lack
                               of resources.

**/
EFI_STATUS
EFIAPI
EmuBlockIo2Flush (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token
  )
{
  EFI_STATUS            Status;
  EMU_BLOCK_IO_PRIVATE  *Private;
  EFI_TPL               OldTpl;

  Private = EMU_BLOCK_IO2_PRIVATE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Status = Private->Io->FlushBlocks (Private->Io, Token);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Reset the Block Device.

  @param  This                 Indicates a pointer to the calling context.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
EmuBlockIoReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  )
{
  EFI_STATUS            Status;
  EMU_BLOCK_IO_PRIVATE  *Private;
  EFI_TPL               OldTpl;

  Private = EMU_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Status = Private->Io->Reset (Private->Io, ExtendedVerification);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
EmuBlockIoReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  OUT VOID                  *Buffer
  )
{
  EFI_STATUS            Status;
  EMU_BLOCK_IO_PRIVATE  *Private;
  EFI_TPL               OldTpl;
  EFI_BLOCK_IO2_TOKEN   Token;

  Private = EMU_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Token.Event = NULL;
  Status      = Private->Io->ReadBlocks (Private->Io, MediaId, Lba, &Token, BufferSize, Buffer);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
EmuBlockIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  )
{
  EFI_STATUS            Status;
  EMU_BLOCK_IO_PRIVATE  *Private;
  EFI_TPL               OldTpl;
  EFI_BLOCK_IO2_TOKEN   Token;

  Private = EMU_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Token.Event = NULL;
  Status      = Private->Io->WriteBlocks (Private->Io, MediaId, Lba, &Token, BufferSize, Buffer);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Flush the Block Device.

  @param  This              Indicates a pointer to the calling context.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writting back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
EmuBlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  EFI_STATUS            Status;
  EMU_BLOCK_IO_PRIVATE  *Private;
  EFI_TPL               OldTpl;
  EFI_BLOCK_IO2_TOKEN   Token;

  Private = EMU_BLOCK_IO_PRIVATE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Token.Event = NULL;
  Status      = Private->Io->FlushBlocks (Private->Io, &Token);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers will typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Because ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
EmuBlockIoDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS             Status;
  EMU_IO_THUNK_PROTOCOL  *EmuIoThunk;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEmuIoThunkProtocolGuid,
                  (VOID **)&EmuIoThunk,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure GUID is for a File System handle.
  //
  Status = EFI_UNSUPPORTED;
  if (CompareGuid (EmuIoThunk->Protocol, &gEmuBlockIoProtocolGuid)) {
    Status = EFI_SUCCESS;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         Handle,
         &gEmuIoThunkProtocolGuid,
         This->DriverBindingHandle,
         Handle
         );
  return Status;
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
EmuBlockIoDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS             Status;
  EMU_IO_THUNK_PROTOCOL  *EmuIoThunk;
  EMU_BLOCK_IO_PRIVATE   *Private = NULL;

  //
  // Grab the protocols we need
  //

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEmuIoThunkProtocolGuid,
                  (void *)&EmuIoThunk,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!CompareGuid (EmuIoThunk->Protocol, &gEmuBlockIoProtocolGuid)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status = EmuIoThunk->Open (EmuIoThunk);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Private = AllocatePool (sizeof (EMU_BLOCK_IO_PRIVATE));
  if (Private == NULL) {
    goto Done;
  }

  Private->Signature = EMU_BLOCK_IO_PRIVATE_SIGNATURE;
  Private->IoThunk   = EmuIoThunk;
  Private->Io        = EmuIoThunk->Interface;
  Private->EfiHandle = Handle;

  Private->BlockIo.Revision    = EFI_BLOCK_IO_PROTOCOL_REVISION2;
  Private->BlockIo.Media       = &Private->Media;
  Private->BlockIo.Reset       = EmuBlockIoReset;
  Private->BlockIo.ReadBlocks  = EmuBlockIoReadBlocks;
  Private->BlockIo.WriteBlocks = EmuBlockIoWriteBlocks;
  Private->BlockIo.FlushBlocks = EmuBlockIoFlushBlocks;

  Private->BlockIo2.Media         = &Private->Media;
  Private->BlockIo2.Reset         = EmuBlockIo2Reset;
  Private->BlockIo2.ReadBlocksEx  = EmuBlockIo2ReadBlocksEx;
  Private->BlockIo2.WriteBlocksEx = EmuBlockIo2WriteBlocksEx;
  Private->BlockIo2.FlushBlocksEx = EmuBlockIo2Flush;

  Private->ControllerNameTable = NULL;

  Status = Private->Io->CreateMapping (Private->Io, &Private->Media);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  AddUnicodeString2 (
    "eng",
    gEmuBlockIoComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    EmuIoThunk->ConfigString,
    TRUE
    );

  AddUnicodeString2 (
    "en",
    gEmuBlockIoComponentName2.SupportedLanguages,
    &Private->ControllerNameTable,
    EmuIoThunk->ConfigString,
    FALSE
    );

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiBlockIoProtocolGuid,
                  &Private->BlockIo,
                  &gEfiBlockIo2ProtocolGuid,
                  &Private->BlockIo2,
                  NULL
                  );

Done:
  if (EFI_ERROR (Status)) {
    if (Private != NULL) {
      if (Private->ControllerNameTable != NULL) {
        FreeUnicodeStringTable (Private->ControllerNameTable);
      }

      gBS->FreePool (Private);
    }

    gBS->CloseProtocol (
           Handle,
           &gEmuIoThunkProtocolGuid,
           This->DriverBindingHandle,
           Handle
           );
  }

  return Status;
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
EmuBlockIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  EFI_STATUS             Status;
  EMU_BLOCK_IO_PRIVATE   *Private;

  //
  // Get our context back
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiBlockIoProtocolGuid,
                  (void *)&BlockIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Private = EMU_BLOCK_IO_PRIVATE_DATA_FROM_THIS (BlockIo);

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Private->EfiHandle,
                  &gEfiBlockIoProtocolGuid,
                  &Private->BlockIo,
                  &gEfiBlockIo2ProtocolGuid,
                  &Private->BlockIo2,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->CloseProtocol (
                    Handle,
                    &gEmuIoThunkProtocolGuid,
                    This->DriverBindingHandle,
                    Handle
                    );
    ASSERT_EFI_ERROR (Status);
    //
    // Destroy the IO interface.
    //
    Status = Private->IoThunk->Close (Private->IoThunk);
    ASSERT_EFI_ERROR (Status);
    //
    // Free our instance data
    //
    FreeUnicodeStringTable (Private->ControllerNameTable);
    FreePool (Private);
  }

  return Status;
}

EFI_DRIVER_BINDING_PROTOCOL  gEmuBlockIoDriverBinding = {
  EmuBlockIoDriverBindingSupported,
  EmuBlockIoDriverBindingStart,
  EmuBlockIoDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  The user Entry Point for module EmuBlockIo . The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeEmuBlockIo (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallAllDriverProtocols2 (
             ImageHandle,
             SystemTable,
             &gEmuBlockIoDriverBinding,
             ImageHandle,
             &gEmuBlockIoComponentName,
             &gEmuBlockIoComponentName2,
             NULL,
             NULL,
             &gEmuBlockIoDriverDiagnostics,
             &gEmuBlockIoDriverDiagnostics2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
