/** @file
  Functions to deal with Disk buffer.

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HexEditor.h"
#include <Protocol/BlockIo.h>

extern EFI_HANDLE                 HImageHandleBackup;
extern HEFI_EDITOR_BUFFER_IMAGE   HBufferImage;

extern BOOLEAN                    HBufferImageNeedRefresh;
extern BOOLEAN                    HBufferImageOnlyLineNeedRefresh;
extern BOOLEAN                    HBufferImageMouseNeedRefresh;

extern HEFI_EDITOR_GLOBAL_EDITOR  HMainEditor;

HEFI_EDITOR_DISK_IMAGE            HDiskImage;
HEFI_EDITOR_DISK_IMAGE            HDiskImageBackupVar;

//
// for basic initialization of HDiskImage
//
HEFI_EDITOR_DISK_IMAGE            HDiskImageConst = {
  NULL,
  0,
  0,
  0
};

/**
  Initialization function for HDiskImage.

  @retval EFI_SUCCESS     The operation was successful.
  @retval EFI_LOAD_ERROR  A load error occurred.
**/
EFI_STATUS
HDiskImageInit (
  VOID
  )
{
  //
  // basically initialize the HDiskImage
  //
  CopyMem (&HDiskImage, &HDiskImageConst, sizeof (HDiskImage));

  CopyMem (&HDiskImageBackupVar, &HDiskImageConst, sizeof (HDiskImageBackupVar));

  return EFI_SUCCESS;
}

/**
  Backup function for HDiskImage. Only a few fields need to be backup.
  This is for making the Disk buffer refresh as few as possible.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  gST->ConOut of resources.
**/
EFI_STATUS
HDiskImageBackup (
  VOID
  )
{
  //
  // backup the disk name, offset and size
  //
  //
  SHELL_FREE_NON_NULL (HDiskImageBackupVar.Name);

  HDiskImageBackupVar.Name = CatSPrint(NULL, L"%s", HDiskImage.Name);
  if (HDiskImageBackupVar.Name == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HDiskImageBackupVar.Offset  = HDiskImage.Offset;
  HDiskImageBackupVar.Size    = HDiskImage.Size;

  return EFI_SUCCESS;
}

/**
  Cleanup function for HDiskImage.

  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
HDiskImageCleanup (
  VOID
  )
{
  SHELL_FREE_NON_NULL (HDiskImage.Name);
  SHELL_FREE_NON_NULL (HDiskImageBackupVar.Name);

  return EFI_SUCCESS;
}

/**
  Set FileName field in HFileImage.

  @param[in] Str      File name to set.
  @param[in] Offset   The offset.
  @param[in] Size     The size.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
HDiskImageSetDiskNameOffsetSize (
  IN CONST CHAR16   *Str,
  IN UINTN    Offset,
  IN UINTN    Size
  )
{
  if (Str == HDiskImage.Name) {
    //
    // This function might be called using HDiskImage.FileName as Str.
    // Directly return without updating HDiskImage.FileName.
    //
    return EFI_SUCCESS;
  }

  //
  // free the old file name
  //
  SHELL_FREE_NON_NULL (HDiskImage.Name);
  HDiskImage.Name = AllocateCopyPool (StrSize (Str), Str);
  if (HDiskImage.Name == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HDiskImage.Offset     = Offset;
  HDiskImage.Size       = Size;

  return EFI_SUCCESS;
}

/**
  Read a disk from disk into HBufferImage.

  @param[in] DeviceName   filename to read.
  @param[in] Offset       The offset.
  @param[in] Size         The size.
  @param[in] Recover      if is for recover, no information print.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_LOAD_ERROR        A load error occurred.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.
**/
EFI_STATUS
HDiskImageRead (
  IN CONST CHAR16   *DeviceName,
  IN UINTN    Offset,
  IN UINTN    Size,
  IN BOOLEAN  Recover
  )
{
  CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *DupDevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *DupDevicePathForFree;
  EFI_HANDLE                      Handle;
  EFI_BLOCK_IO_PROTOCOL           *BlkIo;
  EFI_STATUS                      Status;

  VOID                            *Buffer;
  CHAR16                          *Str;
  UINTN                           Bytes;

  HEFI_EDITOR_LINE                *Line;

  HBufferImage.BufferType = FileTypeDiskBuffer;

  DevicePath              = gEfiShellProtocol->GetDevicePathFromMap(DeviceName);
  if (DevicePath == NULL) {
    StatusBarSetStatusString (L"Cannot Find Device");
    return EFI_INVALID_PARAMETER;
  }
  DupDevicePath = DuplicateDevicePath(DevicePath);
  DupDevicePathForFree = DupDevicePath;
  //
  // get blkio interface
  //
  Status = gBS->LocateDevicePath(&gEfiBlockIoProtocolGuid,&DupDevicePath,&Handle);
  FreePool(DupDevicePathForFree);
  if (EFI_ERROR (Status)) {
    StatusBarSetStatusString (L"Read Disk Failed");
    return Status;
  }
  Status = gBS->OpenProtocol(Handle, &gEfiBlockIoProtocolGuid, (VOID**)&BlkIo, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (EFI_ERROR (Status)) {
    StatusBarSetStatusString (L"Read Disk Failed");
    return Status;
  }
  //
  // if Offset exceeds LastBlock,
  //   return error
  //
  if (Offset > BlkIo->Media->LastBlock || Offset + Size > BlkIo->Media->LastBlock) {
    StatusBarSetStatusString (L"Invalid Offset + Size");
    return EFI_LOAD_ERROR;
  }

  Bytes   = BlkIo->Media->BlockSize * Size;
  Buffer  = AllocateZeroPool (Bytes);

  if (Buffer == NULL) {
    StatusBarSetStatusString (L"Read Disk Failed");
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // read from disk
  //
  Status = BlkIo->ReadBlocks (
                    BlkIo,
                    BlkIo->Media->MediaId,
                    Offset,
                    Bytes,
                    Buffer
                    );

  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    StatusBarSetStatusString (L"Read Disk Failed");
    return EFI_LOAD_ERROR;
  }

  HBufferImageFree ();

  //
  // convert buffer to line list
  //
  Status = HBufferImageBufferToList (Buffer, Bytes);
  FreePool (Buffer);

  if (EFI_ERROR (Status)) {
    StatusBarSetStatusString (L"Read Disk Failed");
    return Status;
  }

  Status = HDiskImageSetDiskNameOffsetSize (DeviceName, Offset, Size);
  if (EFI_ERROR (Status)) {
    StatusBarSetStatusString (L"Read Disk Failed");
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // initialize some variables
  //
  HDiskImage.BlockSize                = BlkIo->Media->BlockSize;

  HBufferImage.DisplayPosition.Row    = 2;
  HBufferImage.DisplayPosition.Column = 10;

  HBufferImage.MousePosition.Row      = 2;
  HBufferImage.MousePosition.Column   = 10;

  HBufferImage.LowVisibleRow          = 1;
  HBufferImage.HighBits               = TRUE;

  HBufferImage.BufferPosition.Row     = 1;
  HBufferImage.BufferPosition.Column  = 1;

  if (!Recover) {
    Str = CatSPrint(NULL, L"%d Lines Read", HBufferImage.NumLines);
    if (Str == NULL) {
      StatusBarSetStatusString (L"Read Disk Failed");
      return EFI_OUT_OF_RESOURCES;
    }

    StatusBarSetStatusString (Str);
    SHELL_FREE_NON_NULL (Str);

    HMainEditor.SelectStart = 0;
    HMainEditor.SelectEnd   = 0;

  }

  //
  // has line
  //
  if (HBufferImage.Lines != NULL) {
    HBufferImage.CurrentLine = CR (
                                HBufferImage.ListHead->ForwardLink,
                                HEFI_EDITOR_LINE,
                                Link,
                                EFI_EDITOR_LINE_LIST
                                );
  } else {
    //
    // create a dummy line
    //
    Line = HBufferImageCreateLine ();
    if (Line == NULL) {
      StatusBarSetStatusString (L"Read Disk Failed");
      return EFI_OUT_OF_RESOURCES;
    }

    HBufferImage.CurrentLine = Line;
  }

  HBufferImage.Modified           = FALSE;
  HBufferImageNeedRefresh         = TRUE;
  HBufferImageOnlyLineNeedRefresh = FALSE;
  HBufferImageMouseNeedRefresh    = TRUE;

  return EFI_SUCCESS;
}

/**
  Save lines in HBufferImage to disk.
  NOT ALLOW TO WRITE TO ANOTHER DISK!!!!!!!!!

  @param[in] DeviceName   The device name.
  @param[in] Offset       The offset.
  @param[in] Size         The size.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_LOAD_ERROR        A load error occurred.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.
**/
EFI_STATUS
HDiskImageSave (
  IN CHAR16 *DeviceName,
  IN UINTN  Offset,
  IN UINTN  Size
  )
{

  CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *DupDevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *DupDevicePathForFree;
  EFI_BLOCK_IO_PROTOCOL           *BlkIo;
  EFI_STATUS                      Status;
  EFI_HANDLE                      Handle;
  VOID                            *Buffer;
  UINTN                           Bytes;

  //
  // if not modified, directly return
  //
  if (HBufferImage.Modified == FALSE) {
    return EFI_SUCCESS;
  }

  HBufferImage.BufferType = FileTypeDiskBuffer;

  DevicePath              = gEfiShellProtocol->GetDevicePathFromMap(DeviceName);
  if (DevicePath == NULL) {
//    StatusBarSetStatusString (L"Cannot Find Device");
    return EFI_INVALID_PARAMETER;
  }
  DupDevicePath = DuplicateDevicePath(DevicePath);
  DupDevicePathForFree = DupDevicePath;

  //
  // get blkio interface
  //
  Status = gBS->LocateDevicePath(&gEfiBlockIoProtocolGuid,&DupDevicePath,&Handle);
  FreePool(DupDevicePathForFree);
  if (EFI_ERROR (Status)) {
//    StatusBarSetStatusString (L"Read Disk Failed");
    return Status;
  }
  Status = gBS->OpenProtocol(Handle, &gEfiBlockIoProtocolGuid, (VOID**)&BlkIo, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (EFI_ERROR (Status)) {
//    StatusBarSetStatusString (L"Read Disk Failed");
    return Status;
  }

  Bytes   = BlkIo->Media->BlockSize * Size;
  Buffer  = AllocateZeroPool (Bytes);

  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // concatenate the line list to a buffer
  //
  Status = HBufferImageListToBuffer (Buffer, Bytes);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return Status;
  }

  //
  // write the buffer to disk
  //
  Status = BlkIo->WriteBlocks (
                    BlkIo,
                    BlkIo->Media->MediaId,
                    Offset,
                    Bytes,
                    Buffer
                    );

  FreePool (Buffer);

  if (EFI_ERROR (Status)) {
    return EFI_LOAD_ERROR;
  }
  //
  // now not modified
  //
  HBufferImage.Modified = FALSE;

  return EFI_SUCCESS;
}
