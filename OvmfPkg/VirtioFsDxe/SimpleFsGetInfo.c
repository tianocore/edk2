/** @file
  EFI_FILE_PROTOCOL.GetInfo() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/FileSystemInfo.h>            // gEfiFileSystemInfoGuid
#include <Guid/FileSystemVolumeLabelInfo.h> // gEfiFileSystemVolumeLabelInfo...
#include <Library/BaseLib.h>                // StrSize()
#include <Library/BaseMemoryLib.h>          // CompareGuid()

#include "VirtioFsDxe.h"

/**
  Provide EFI_FILE_INFO about this particular file.
**/
STATIC
EFI_STATUS
GetFileInfo (
  IN     EFI_FILE_PROTOCOL *This,
  IN OUT UINTN             *BufferSize,
     OUT VOID              *Buffer
  )
{
  VIRTIO_FS_FILE                     *VirtioFsFile;
  VIRTIO_FS                          *VirtioFs;
  UINTN                              AllocSize;
  UINTN                              BasenameSize;
  EFI_STATUS                         Status;
  EFI_FILE_INFO                      *FileInfo;
  VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE FuseAttr;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  VirtioFs     = VirtioFsFile->OwnerFs;

  AllocSize = *BufferSize;

  //
  // Calculate the needed size.
  //
  BasenameSize = 0;
  Status = VirtioFsGetBasename (VirtioFsFile->CanonicalPathname, NULL,
             &BasenameSize);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  *BufferSize = OFFSET_OF (EFI_FILE_INFO, FileName) + BasenameSize;

  if (*BufferSize > AllocSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Set the structure size, and store the basename.
  //
  FileInfo = Buffer;
  FileInfo->Size = *BufferSize;
  Status = VirtioFsGetBasename (VirtioFsFile->CanonicalPathname,
             FileInfo->FileName, &BasenameSize);
  ASSERT_EFI_ERROR (Status);

  //
  // Fetch the file attributes, and convert them into the caller's buffer.
  //
  Status = VirtioFsFuseGetAttr (VirtioFs, VirtioFsFile->NodeId, &FuseAttr);
  if (!EFI_ERROR (Status)) {
    Status = VirtioFsFuseAttrToEfiFileInfo (&FuseAttr, FileInfo);
  }
  return (Status == EFI_BUFFER_TOO_SMALL) ? EFI_DEVICE_ERROR : Status;
}

/**
  Provide EFI_FILE_SYSTEM_INFO about the filesystem this file lives on.
**/
STATIC
EFI_STATUS
GetFileSystemInfo (
  IN     EFI_FILE_PROTOCOL *This,
  IN OUT UINTN             *BufferSize,
     OUT VOID              *Buffer
  )
{
  VIRTIO_FS_FILE                 *VirtioFsFile;
  VIRTIO_FS                      *VirtioFs;
  UINTN                          AllocSize;
  UINTN                          LabelSize;
  EFI_STATUS                     Status;
  VIRTIO_FS_FUSE_STATFS_RESPONSE FilesysAttr;
  UINT64                         MaxBlocks;
  EFI_FILE_SYSTEM_INFO           *FilesysInfo;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  VirtioFs     = VirtioFsFile->OwnerFs;

  AllocSize = *BufferSize;

  //
  // Calculate the needed size.
  //
  LabelSize = StrSize (VirtioFs->Label);
  *BufferSize = OFFSET_OF (EFI_FILE_SYSTEM_INFO, VolumeLabel) + LabelSize;

  if (*BufferSize > AllocSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Fetch the filesystem attributes.
  //
  Status = VirtioFsFuseStatFs (VirtioFs, VirtioFsFile->NodeId, &FilesysAttr);
  if (EFI_ERROR (Status)) {
    return (Status == EFI_BUFFER_TOO_SMALL) ? EFI_DEVICE_ERROR : Status;
  }
  //
  // Sanity checks...
  //
  if (FilesysAttr.Frsize != FilesysAttr.Bsize) {
    return EFI_UNSUPPORTED;
  }
  if (FilesysAttr.Frsize == 0 || FilesysAttr.Blocks == 0 ||
      FilesysAttr.Bavail > FilesysAttr.Blocks) {
    return EFI_DEVICE_ERROR;
  }
  MaxBlocks = DivU64x32 (MAX_UINT64, FilesysAttr.Frsize);
  if (FilesysAttr.Blocks > MaxBlocks || FilesysAttr.Bavail > MaxBlocks) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Fill in EFI_FILE_SYSTEM_INFO.
  //
  FilesysInfo             = Buffer;
  FilesysInfo->Size       = *BufferSize;
  FilesysInfo->ReadOnly   = FALSE;
  FilesysInfo->VolumeSize = MultU64x32 (FilesysAttr.Blocks,
                              FilesysAttr.Frsize);
  FilesysInfo->FreeSpace  = MultU64x32 (FilesysAttr.Bavail,
                              FilesysAttr.Frsize);
  FilesysInfo->BlockSize  = FilesysAttr.Frsize;
  CopyMem (FilesysInfo->VolumeLabel, VirtioFs->Label, LabelSize);

  return EFI_SUCCESS;
}

/**
  Return the filesystem label as EFI_FILE_SYSTEM_VOLUME_LABEL.
**/
STATIC
EFI_STATUS
GetFileSystemVolumeLabelInfo (
  IN     EFI_FILE_PROTOCOL *This,
  IN OUT UINTN             *BufferSize,
     OUT VOID              *Buffer
  )
{
  VIRTIO_FS_FILE               *VirtioFsFile;
  VIRTIO_FS                    *VirtioFs;
  UINTN                        AllocSize;
  UINTN                        LabelSize;
  EFI_FILE_SYSTEM_VOLUME_LABEL *FilesysVolumeLabel;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  VirtioFs     = VirtioFsFile->OwnerFs;

  AllocSize = *BufferSize;

  //
  // Calculate the needed size.
  //
  LabelSize = StrSize (VirtioFs->Label);
  *BufferSize = (OFFSET_OF (EFI_FILE_SYSTEM_VOLUME_LABEL, VolumeLabel) +
                 LabelSize);

  if (*BufferSize > AllocSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Store the label.
  //
  FilesysVolumeLabel = Buffer;
  CopyMem (FilesysVolumeLabel->VolumeLabel, VirtioFs->Label, LabelSize);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioFsSimpleFileGetInfo (
  IN     EFI_FILE_PROTOCOL *This,
  IN     EFI_GUID          *InformationType,
  IN OUT UINTN             *BufferSize,
     OUT VOID              *Buffer
  )
{
  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    return GetFileInfo (This, BufferSize, Buffer);
  }

  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    return GetFileSystemInfo (This, BufferSize, Buffer);
  }

  if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
    return GetFileSystemVolumeLabelInfo (This, BufferSize, Buffer);
  }

  return EFI_UNSUPPORTED;
}
