/** @file
  EFI_FILE_PROTOCOL.Write() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "VirtioFsDxe.h"

EFI_STATUS
EFIAPI
VirtioFsSimpleFileWrite (
  IN     EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN              *BufferSize,
  IN     VOID               *Buffer
  )
{
  VIRTIO_FS_FILE  *VirtioFsFile;
  VIRTIO_FS       *VirtioFs;
  EFI_STATUS      Status;
  UINTN           Transferred;
  UINTN           Left;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);
  VirtioFs     = VirtioFsFile->OwnerFs;

  if (VirtioFsFile->IsDirectory) {
    return EFI_UNSUPPORTED;
  }

  if (!VirtioFsFile->IsOpenForWriting) {
    return EFI_ACCESS_DENIED;
  }

  Status      = EFI_SUCCESS;
  Transferred = 0;
  Left        = *BufferSize;
  while (Left > 0) {
    UINT32  WriteSize;

    //
    // Honor the write buffer size limit.
    //
    WriteSize = (UINT32)MIN ((UINTN)VirtioFs->MaxWrite, Left);
    Status    = VirtioFsFuseWrite (
                  VirtioFs,
                  VirtioFsFile->NodeId,
                  VirtioFsFile->FuseHandle,
                  VirtioFsFile->FilePosition + Transferred,
                  &WriteSize,
                  (UINT8 *)Buffer + Transferred
                  );
    if (!EFI_ERROR (Status) && (WriteSize == 0)) {
      //
      // Progress should have been made.
      //
      Status = EFI_DEVICE_ERROR;
    }

    if (EFI_ERROR (Status)) {
      break;
    }

    Transferred += WriteSize;
    Left        -= WriteSize;
  }

  *BufferSize                 = Transferred;
  VirtioFsFile->FilePosition += Transferred;
  //
  // According to the UEFI spec,
  //
  // - 'Partial writes only occur when there has been a data error during the
  //    write attempt (such as "file space full")', and
  //
  // - (as an example) EFI_VOLUME_FULL is returned when 'The volume is full'.
  //
  // These together imply that after a partial write, we have to return an
  // error. In other words, (Transferred > 0) is inconsequential for the return
  // value.
  //
  return Status;
}
