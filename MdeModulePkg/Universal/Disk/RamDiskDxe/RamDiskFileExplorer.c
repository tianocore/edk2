/** @file
  Internal file explorer helper functions for RamDiskDxe driver.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "RamDiskImpl.h"


/**
  Helper function called as part of the code needed to allocate the proper
  sized buffer for various EFI interfaces.

  @param[in, out] Status     Current status.
  @param[in, out] Buffer     Current allocated buffer, or NULL.
  @param[in]      BufferSize Current buffer size needed.

  @retval  TRUE         If the buffer was reallocated and the caller should
                        try the API again.
  @retval  FALSE        The caller should not call this function again.

**/
BOOLEAN
GrowBuffer (
  IN OUT EFI_STATUS   *Status,
  IN OUT VOID         **Buffer,
  IN UINTN            BufferSize
  )
{
  BOOLEAN TryAgain;

  //
  // If this is an initial request, buffer will be null with a new buffer size
  //
  if ((*Buffer == NULL) && (BufferSize != 0)) {
    *Status = EFI_BUFFER_TOO_SMALL;
  }
  //
  // If the status code is "buffer too small", resize the buffer
  //
  TryAgain = FALSE;
  if (*Status == EFI_BUFFER_TOO_SMALL) {

    if (*Buffer != NULL) {
      FreePool (*Buffer);
    }

    *Buffer = AllocateZeroPool (BufferSize);

    if (*Buffer != NULL) {
      TryAgain = TRUE;
    } else {
      *Status = EFI_OUT_OF_RESOURCES;
    }
  }
  //
  // If there's an error, free the buffer
  //
  if (!TryAgain && EFI_ERROR (*Status) && (*Buffer != NULL)) {
    FreePool (*Buffer);
    *Buffer = NULL;
  }

  return TryAgain;
}


/**
  This function gets the file information from an open file descriptor,
  and stores it in a buffer allocated from pool.

  @param[in] FHand           File Handle.

  @return    A pointer to a buffer with file information or NULL is returned.

**/
EFI_FILE_INFO *
FileInfo (
  IN EFI_FILE_HANDLE                        FHand
  )
{
  EFI_STATUS                           Status;
  EFI_FILE_INFO                        *Buffer;
  UINTN                                BufferSize;

  //
  // Initialize for GrowBuffer loop
  //
  Buffer      = NULL;
  BufferSize  = SIZE_OF_EFI_FILE_INFO + 200;

  //
  // Call the real function
  //
  while (GrowBuffer (&Status, (VOID **) &Buffer, BufferSize)) {
    Status = FHand->GetInfo (
                      FHand,
                      &gEfiFileInfoGuid,
                      &BufferSize,
                      Buffer
                      );
  }

  return Buffer;
}


/**
  This function will open a file or directory referenced by DevicePath.

  This function opens a file with the open mode according to the file path. The
  Attributes is valid only for EFI_FILE_MODE_CREATE.

  @param[in, out] FilePath   On input, the device path to the file.
                             On output, the remaining device path.
  @param[out]     FileHandle Pointer to the file handle.
  @param[in]      OpenMode   The mode to open the file with.
  @param[in]      Attributes The file's file attributes.

  @retval EFI_SUCCESS             The information was set.
  @retval EFI_INVALID_PARAMETER   One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED         Could not open the file path.
  @retval EFI_NOT_FOUND           The specified file could not be found on the
                                  device or the file system could not be found
                                  on the device.
  @retval EFI_NO_MEDIA            The device has no medium.
  @retval EFI_MEDIA_CHANGED       The device has a different medium in it or
                                  the medium is no longer supported.
  @retval EFI_DEVICE_ERROR        The device reported an error.
  @retval EFI_VOLUME_CORRUPTED    The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED     The file or medium is write protected.
  @retval EFI_ACCESS_DENIED       The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES    Not enough resources were available to open
                                  the file.
  @retval EFI_VOLUME_FULL         The volume is full.
**/
EFI_STATUS
EFIAPI
OpenFileByDevicePath(
  IN OUT EFI_DEVICE_PATH_PROTOCOL           **FilePath,
  OUT EFI_FILE_HANDLE                       *FileHandle,
  IN UINT64                                 OpenMode,
  IN UINT64                                 Attributes
  )
{
  EFI_STATUS                           Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL      *EfiSimpleFileSystemProtocol;
  EFI_FILE_PROTOCOL                    *Handle1;
  EFI_FILE_PROTOCOL                    *Handle2;
  EFI_HANDLE                           DeviceHandle;

  if ((FilePath == NULL || FileHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  FilePath,
                  &DeviceHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol(
                  DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID**)&EfiSimpleFileSystemProtocol,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EfiSimpleFileSystemProtocol->OpenVolume(EfiSimpleFileSystemProtocol, &Handle1);
  if (EFI_ERROR (Status)) {
    FileHandle = NULL;
    return Status;
  }

  //
  // go down directories one node at a time.
  //
  while (!IsDevicePathEnd (*FilePath)) {
    //
    // For file system access each node should be a file path component
    //
    if (DevicePathType    (*FilePath) != MEDIA_DEVICE_PATH ||
        DevicePathSubType (*FilePath) != MEDIA_FILEPATH_DP
       ) {
      FileHandle = NULL;
      return (EFI_INVALID_PARAMETER);
    }
    //
    // Open this file path node
    //
    Handle2  = Handle1;
    Handle1 = NULL;

    //
    // Try to test opening an existing file
    //
    Status = Handle2->Open (
                          Handle2,
                          &Handle1,
                          ((FILEPATH_DEVICE_PATH*)*FilePath)->PathName,
                          OpenMode &~EFI_FILE_MODE_CREATE,
                          0
                         );

    //
    // see if the error was that it needs to be created
    //
    if ((EFI_ERROR (Status)) && (OpenMode != (OpenMode &~EFI_FILE_MODE_CREATE))) {
      Status = Handle2->Open (
                            Handle2,
                            &Handle1,
                            ((FILEPATH_DEVICE_PATH*)*FilePath)->PathName,
                            OpenMode,
                            Attributes
                           );
    }
    //
    // Close the last node
    //
    Handle2->Close (Handle2);

    if (EFI_ERROR(Status)) {
      return (Status);
    }

    //
    // Get the next node
    //
    *FilePath = NextDevicePathNode (*FilePath);
  }

  //
  // This is a weak spot since if the undefined SHELL_FILE_HANDLE format changes this must change also!
  //
  *FileHandle = (VOID*)Handle1;
  return EFI_SUCCESS;
}
