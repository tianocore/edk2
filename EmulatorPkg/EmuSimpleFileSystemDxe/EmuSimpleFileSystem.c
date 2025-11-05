/*++ @file
  Produce Simple File System abstractions for directories on your PC using Posix APIs.
  The configuration of what devices to mount or emulate comes from UNIX
  environment variables. The variables must be visible to the Microsoft*
  Developer Studio for them to work.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EmuSimpleFileSystem.h"

/**
  Opens a new file relative to the source file's location.

  @param  This       The protocol instance pointer.
  @param  NewHandle  Returns File Handle for FileName.
  @param  FileName   Null terminated string. "\", ".", and ".." are supported.
  @param  OpenMode   Open mode for file.
  @param  Attributes Only used for EFI_FILE_MODE_CREATE.

  @retval EFI_SUCCESS          The device was opened.
  @retval EFI_NOT_FOUND        The specified file could not be found on the device.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_MEDIA_CHANGED    The media has changed.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_ACCESS_DENIED    The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES The volume was not opened due to lack of resources.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
EFIAPI
EmuSimpleFileSystemOpen (
  IN  EFI_FILE_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN  CHAR16             *FileName,
  IN  UINT64             OpenMode,
  IN  UINT64             Attributes
  )
{
  EFI_STATUS            Status;
  EFI_TPL               OldTpl;
  EMU_EFI_FILE_PRIVATE  *PrivateFile;
  EMU_EFI_FILE_PRIVATE  *NewPrivateFile;

  //
  // Check for obvious invalid parameters.
  //
  if ((This == NULL) || (NewHandle == NULL) || (FileName == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (OpenMode) {
    case EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE:
      if (Attributes &~EFI_FILE_VALID_ATTR) {
        return EFI_INVALID_PARAMETER;
      }

      if (Attributes & EFI_FILE_READ_ONLY) {
        return EFI_INVALID_PARAMETER;
      }

    //
    // fall through
    //
    case EFI_FILE_MODE_READ:
    case EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE:
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  NewPrivateFile = AllocateCopyPool (sizeof (EMU_EFI_FILE_PRIVATE), PrivateFile);
  if (NewPrivateFile == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = PrivateFile->Io->Open (PrivateFile->Io, &NewPrivateFile->Io, FileName, OpenMode, Attributes);
  if (!EFI_ERROR (Status)) {
    *NewHandle = &NewPrivateFile->EfiFile;
  } else {
    *NewHandle = NULL;
    FreePool (NewPrivateFile);
  }

Done:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Close the file handle

  @param  This          Protocol instance pointer.

  @retval EFI_SUCCESS   The file was closed.

**/
EFI_STATUS
EFIAPI
EmuSimpleFileSystemClose (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS            Status;
  EMU_EFI_FILE_PRIVATE  *PrivateFile;
  EFI_TPL               OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Status = PrivateFile->Io->Close (PrivateFile->Io);
  if (!EFI_ERROR (Status)) {
    gBS->FreePool (PrivateFile);
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Close and delete the file handle.

  @param  This                     Protocol instance pointer.

  @retval EFI_SUCCESS              The file was closed and deleted.
  @retval EFI_WARN_DELETE_FAILURE  The handle was closed but the file was not deleted.

**/
EFI_STATUS
EFIAPI
EmuSimpleFileSystemDelete (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS            Status;
  EMU_EFI_FILE_PRIVATE  *PrivateFile;
  EFI_TPL               OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  Status = PrivateFile->Io->Delete (PrivateFile->Io);
  if (!EFI_ERROR (Status)) {
    gBS->FreePool (PrivateFile);
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Read data from the file.

  @param  This       Protocol instance pointer.
  @param  BufferSize On input size of buffer, on output amount of data in buffer.
  @param  Buffer     The buffer in which data is read.

  @retval EFI_SUCCESS          Data was read.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_BUFFER_TO_SMALL  BufferSize is too small. BufferSize contains required size.

**/
EFI_STATUS
EFIAPI
EmuSimpleFileSystemRead (
  IN     EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN              *BufferSize,
  OUT    VOID               *Buffer
  )
{
  EFI_STATUS            Status;
  EMU_EFI_FILE_PRIVATE  *PrivateFile;
  EFI_TPL               OldTpl;

  if ((This == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*BufferSize != 0) && (Buffer == NULL)) {
    // Buffer can be NULL  if *BufferSize is zero
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  Status = PrivateFile->Io->Read (PrivateFile->Io, BufferSize, Buffer);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Write data to a file.

  @param  This       Protocol instance pointer.
  @param  BufferSize On input size of buffer, on output amount of data in buffer.
  @param  Buffer     The buffer in which data to write.

  @retval EFI_SUCCESS          Data was written.
  @retval EFI_UNSUPPORTED      Writes to Open directory are not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_DEVICE_ERROR     An attempt was made to write to a deleted file.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
EFIAPI
EmuSimpleFileSystemWrite (
  IN     EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN              *BufferSize,
  IN     VOID               *Buffer
  )
{
  EFI_STATUS            Status;
  EMU_EFI_FILE_PRIVATE  *PrivateFile;
  EFI_TPL               OldTpl;

  if ((This == NULL) || (BufferSize == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  Status = PrivateFile->Io->Write (PrivateFile->Io, BufferSize, Buffer);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Get a file's current position

  @param  This            Protocol instance pointer.
  @param  Position        Byte position from the start of the file.

  @retval EFI_SUCCESS     Position was updated.
  @retval EFI_UNSUPPORTED Seek request for non-zero is not valid on open.

**/
EFI_STATUS
EFIAPI
EmuSimpleFileSystemGetPosition (
  IN  EFI_FILE_PROTOCOL  *This,
  OUT UINT64             *Position
  )
{
  EFI_STATUS            Status;
  EMU_EFI_FILE_PRIVATE  *PrivateFile;
  EFI_TPL               OldTpl;

  if ((This == NULL) || (Position == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  Status = PrivateFile->Io->GetPosition (PrivateFile->Io, Position);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Set file's current position

  @param  This            Protocol instance pointer.
  @param  Position        Byte position from the start of the file.

  @retval EFI_SUCCESS     Position was updated.
  @retval EFI_UNSUPPORTED Seek request for non-zero is not valid on open..

**/
EFI_STATUS
EFIAPI
EmuSimpleFileSystemSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  )
{
  EFI_STATUS            Status;
  EMU_EFI_FILE_PRIVATE  *PrivateFile;
  EFI_TPL               OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  Status = PrivateFile->Io->SetPosition (PrivateFile->Io, Position);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Get information about a file.

  @param  This            Protocol instance pointer.
  @param  InformationType Type of information to return in Buffer.
  @param  BufferSize      On input size of buffer, on output amount of data in buffer.
  @param  Buffer          The buffer to return data.

  @retval EFI_SUCCESS          Data was returned.
  @retval EFI_UNSUPPORTED      InformationType is not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.
  @retval EFI_BUFFER_TOO_SMALL Buffer was too small; required size returned in BufferSize.

**/
EFI_STATUS
EFIAPI
EmuSimpleFileSystemGetInfo (
  IN     EFI_FILE_PROTOCOL  *This,
  IN     EFI_GUID           *InformationType,
  IN OUT UINTN              *BufferSize,
  OUT    VOID               *Buffer
  )
{
  EFI_STATUS            Status;
  EMU_EFI_FILE_PRIVATE  *PrivateFile;
  EFI_TPL               OldTpl;

  if ((This == NULL) || (InformationType == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  Status = PrivateFile->Io->GetInfo (PrivateFile->Io, InformationType, BufferSize, Buffer);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Set information about a file

  @param  File            Protocol instance pointer.
  @param  InformationType Type of information in Buffer.
  @param  BufferSize      Size of buffer.
  @param  Buffer          The data to write.

  @retval EFI_SUCCESS          Data was set.
  @retval EFI_UNSUPPORTED      InformationType is not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.

**/
EFI_STATUS
EFIAPI
EmuSimpleFileSystemSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  )
{
  EFI_STATUS            Status;
  EMU_EFI_FILE_PRIVATE  *PrivateFile;
  EFI_TPL               OldTpl;

  //
  // Check for invalid parameters.
  //
  if ((This == NULL) || (InformationType == NULL) || (BufferSize == 0) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  Status = PrivateFile->Io->SetInfo (PrivateFile->Io, InformationType, BufferSize, Buffer);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Flush data back for the file handle.

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS          Data was flushed.
  @retval EFI_UNSUPPORTED      Writes to Open directory are not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED  The device is write protected.
  @retval EFI_ACCESS_DENIED    The file was open for read only.
  @retval EFI_VOLUME_FULL      The volume is full.

**/
EFI_STATUS
EFIAPI
EmuSimpleFileSystemFlush (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS            Status;
  EMU_EFI_FILE_PRIVATE  *PrivateFile;
  EFI_TPL               OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  PrivateFile = EMU_EFI_FILE_PRIVATE_DATA_FROM_THIS (This);

  Status = PrivateFile->Io->Flush (PrivateFile->Io);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Open the root directory on a volume.

  @param  This Protocol instance pointer.
  @param  Root Returns an Open file handle for the root directory

  @retval EFI_SUCCESS          The device was opened.
  @retval EFI_UNSUPPORTED      This volume does not support the file system.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_ACCESS_DENIED    The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES The volume was not opened due to lack of resources.

**/
EFI_STATUS
EFIAPI
EmuSimpleFileSystemOpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL               **Root
  )
{
  EFI_STATUS                      Status;
  EMU_SIMPLE_FILE_SYSTEM_PRIVATE  *Private;
  EMU_EFI_FILE_PRIVATE            *PrivateFile;
  EFI_TPL                         OldTpl;

  Status = EFI_UNSUPPORTED;

  if ((This == NULL) || (Root == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Private = EMU_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (This);

  PrivateFile = AllocatePool (sizeof (EMU_EFI_FILE_PRIVATE));
  if (PrivateFile == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  PrivateFile->Signature        = EMU_EFI_FILE_PRIVATE_SIGNATURE;
  PrivateFile->IoThunk          = Private->IoThunk;
  PrivateFile->SimpleFileSystem = This;

  ZeroMem (&PrivateFile->EfiFile, sizeof (PrivateFile->EfiFile));
  PrivateFile->EfiFile.Revision    = EFI_FILE_PROTOCOL_REVISION;
  PrivateFile->EfiFile.Open        = EmuSimpleFileSystemOpen;
  PrivateFile->EfiFile.Close       = EmuSimpleFileSystemClose;
  PrivateFile->EfiFile.Delete      = EmuSimpleFileSystemDelete;
  PrivateFile->EfiFile.Read        = EmuSimpleFileSystemRead;
  PrivateFile->EfiFile.Write       = EmuSimpleFileSystemWrite;
  PrivateFile->EfiFile.GetPosition = EmuSimpleFileSystemGetPosition;
  PrivateFile->EfiFile.SetPosition = EmuSimpleFileSystemSetPosition;
  PrivateFile->EfiFile.GetInfo     = EmuSimpleFileSystemGetInfo;
  PrivateFile->EfiFile.SetInfo     = EmuSimpleFileSystemSetInfo;
  PrivateFile->EfiFile.Flush       = EmuSimpleFileSystemFlush;

  *Root = &PrivateFile->EfiFile;

  Status = Private->Io->OpenVolume (Private->Io, &PrivateFile->Io);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  AddUnicodeString2 (
    "eng",
    gEmuSimpleFileSystemComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    Private->IoThunk->ConfigString,
    TRUE
    );

  AddUnicodeString2 (
    "en",
    gEmuSimpleFileSystemComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    Private->IoThunk->ConfigString,
    FALSE
    );

Done:
  if (EFI_ERROR (Status)) {
    if (PrivateFile) {
      gBS->FreePool (PrivateFile);
    }

    *Root = NULL;
  }

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
EmuSimpleFileSystemDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS             Status;
  EMU_IO_THUNK_PROTOCOL  *EmuIoThunk;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEmuIoThunkProtocolGuid,
                  (VOID **)&EmuIoThunk,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure GUID is for a File System handle.
  //
  Status = EFI_UNSUPPORTED;
  if (CompareGuid (EmuIoThunk->Protocol, &gEfiSimpleFileSystemProtocolGuid)) {
    Status = EFI_SUCCESS;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEmuIoThunkProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
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
EmuSimpleFileSystemDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                      Status;
  EMU_IO_THUNK_PROTOCOL           *EmuIoThunk;
  EMU_SIMPLE_FILE_SYSTEM_PRIVATE  *Private;

  Private = NULL;

  //
  // Open the IO Abstraction(s) needed
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEmuIoThunkProtocolGuid,
                  (VOID **)&EmuIoThunk,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Validate GUID
  //
  if (!CompareGuid (EmuIoThunk->Protocol, &gEfiSimpleFileSystemProtocolGuid)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Private = AllocateZeroPool (sizeof (EMU_SIMPLE_FILE_SYSTEM_PRIVATE));
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = EmuIoThunk->Open (EmuIoThunk);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Private->Signature = EMU_SIMPLE_FILE_SYSTEM_PRIVATE_SIGNATURE;
  Private->IoThunk   = EmuIoThunk;
  Private->Io        = EmuIoThunk->Interface;

  Private->SimpleFileSystem.Revision   = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  Private->SimpleFileSystem.OpenVolume = EmuSimpleFileSystemOpenVolume;

  Private->ControllerNameTable = NULL;

  AddUnicodeString2 (
    "eng",
    gEmuSimpleFileSystemComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    EmuIoThunk->ConfigString,
    TRUE
    );

  AddUnicodeString2 (
    "en",
    gEmuSimpleFileSystemComponentName2.SupportedLanguages,
    &Private->ControllerNameTable,
    EmuIoThunk->ConfigString,
    FALSE
    );

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  &Private->SimpleFileSystem,
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
           ControllerHandle,
           &gEmuIoThunkProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
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
EmuSimpleFileSystemDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFileSystem;
  EMU_SIMPLE_FILE_SYSTEM_PRIVATE   *Private;

  //
  // Get our context back
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&SimpleFileSystem,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Private = EMU_SIMPLE_FILE_SYSTEM_PRIVATE_DATA_FROM_THIS (SimpleFileSystem);

  //
  // Uninstall the Simple File System Protocol from ControllerHandle
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  &Private->SimpleFileSystem,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->CloseProtocol (
                    ControllerHandle,
                    &gEmuIoThunkProtocolGuid,
                    This->DriverBindingHandle,
                    ControllerHandle
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
    gBS->FreePool (Private);
  }

  return Status;
}

EFI_DRIVER_BINDING_PROTOCOL  gEmuSimpleFileSystemDriverBinding = {
  EmuSimpleFileSystemDriverBindingSupported,
  EmuSimpleFileSystemDriverBindingStart,
  EmuSimpleFileSystemDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  The user Entry Point for module EmuSimpleFileSystem. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeEmuSimpleFileSystem (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gEmuSimpleFileSystemDriverBinding,
             ImageHandle,
             &gEmuSimpleFileSystemComponentName,
             &gEmuSimpleFileSystemComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
