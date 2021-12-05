/** @file
  EFI_FILE_PROTOCOL.Read() member function for the Virtio Filesystem driver.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseMemoryLib.h>       // CopyMem()
#include <Library/MemoryAllocationLib.h> // AllocatePool()

#include "VirtioFsDxe.h"

/**
  Populate a caller-allocated EFI_FILE_INFO object from
  VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE.

  @param[in] Dirent              The entry read from the directory stream. The
                                 caller is responsible for ensuring that
                                 Dirent->Namelen describe valid storage.

  @param[in] SingleFileInfoSize  The allocated size of FileInfo.

  @param[out] FileInfo           The EFI_FILE_INFO object to populate. On
                                 success, all fields in FileInfo will be
                                 updated, setting FileInfo->Size to the
                                 actually used size (which will not exceed
                                 SingleFileInfoSize).

  @retval EFI_SUCCESS  FileInfo has been filled in.

  @return              Error codes propagated from
                       VirtioFsFuseDirentPlusToEfiFileInfo() and
                       VirtioFsFuseAttrToEfiFileInfo(). The contents of
                       FileInfo are indeterminate.
**/
STATIC
EFI_STATUS
PopulateFileInfo (
  IN     VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE  *Dirent,
  IN     UINTN                               SingleFileInfoSize,
  OUT EFI_FILE_INFO                          *FileInfo
  )
{
  EFI_STATUS  Status;

  //
  // Convert the name, set the actual size.
  //
  FileInfo->Size = SingleFileInfoSize;
  Status         = VirtioFsFuseDirentPlusToEfiFileInfo (Dirent, FileInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the scalar fields.
  //
  Status = VirtioFsFuseAttrToEfiFileInfo (&Dirent->AttrResp, FileInfo);
  return Status;
}

/**
  Refill the EFI_FILE_INFO cache from the directory stream.
**/
STATIC
EFI_STATUS
RefillFileInfoCache (
  IN OUT VIRTIO_FS_FILE  *VirtioFsFile
  )
{
  VIRTIO_FS                       *VirtioFs;
  EFI_STATUS                      Status;
  VIRTIO_FS_FUSE_STATFS_RESPONSE  FilesysAttr;
  UINT32                          DirentBufSize;
  UINT8                           *DirentBuf;
  UINTN                           SingleFileInfoSize;
  UINT8                           *FileInfoArray;
  UINT64                          DirStreamCookie;
  UINT64                          CacheEndsAtCookie;
  UINTN                           NumFileInfo;

  //
  // Allocate a DirentBuf that can receive at least
  // VIRTIO_FS_FILE_MAX_FILE_INFO directory entries, based on the maximum
  // filename length supported by the filesystem. Note that the multiplication
  // is safe from overflow due to the VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE_SIZE()
  // check.
  //
  VirtioFs = VirtioFsFile->OwnerFs;
  Status   = VirtioFsFuseStatFs (VirtioFs, VirtioFsFile->NodeId, &FilesysAttr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DirentBufSize = (UINT32)VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE_SIZE (
                            FilesysAttr.Namelen
                            );
  if (DirentBufSize == 0) {
    return EFI_UNSUPPORTED;
  }

  DirentBufSize *= VIRTIO_FS_FILE_MAX_FILE_INFO;
  DirentBuf      = AllocatePool (DirentBufSize);
  if (DirentBuf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Allocate the EFI_FILE_INFO cache. A single EFI_FILE_INFO element is sized
  // accordingly to the maximum filename length supported by the filesystem.
  //
  // Note that the calculation below cannot overflow, due to the filename limit
  // imposed by the VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE_SIZE() check above. The
  // calculation takes the L'\0' character that we'll need to append into
  // account.
  //
  SingleFileInfoSize = (OFFSET_OF (EFI_FILE_INFO, FileName) +
                        ((UINTN)FilesysAttr.Namelen + 1) * sizeof (CHAR16));
  FileInfoArray = AllocatePool (
                    VIRTIO_FS_FILE_MAX_FILE_INFO * SingleFileInfoSize
                    );
  if (FileInfoArray == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeDirentBuf;
  }

  //
  // Pick up reading the directory stream where the previous cache ended.
  //
  DirStreamCookie   = VirtioFsFile->FilePosition;
  CacheEndsAtCookie = VirtioFsFile->FilePosition;
  NumFileInfo       = 0;
  do {
    UINT32  Remaining;
    UINT32  Consumed;

    //
    // Fetch a chunk of the directory stream. The chunk may hold more entries
    // than what we can fit in the cache. The chunk may also not entirely fill
    // the cache, especially after filtering out entries that cannot be
    // supported under UEFI (sockets, FIFOs, filenames with backslashes, etc).
    //
    Remaining = DirentBufSize;
    Status    = VirtioFsFuseReadFileOrDir (
                  VirtioFs,
                  VirtioFsFile->NodeId,
                  VirtioFsFile->FuseHandle,
                  TRUE,                  // IsDir
                  DirStreamCookie,       // Offset
                  &Remaining,            // Size
                  DirentBuf              // Data
                  );
    if (EFI_ERROR (Status)) {
      goto FreeFileInfoArray;
    }

    if (Remaining == 0) {
      //
      // The directory stream ends.
      //
      break;
    }

    //
    // Iterate over all records in DirentBuf. Primarily, forget them all.
    // Secondarily, if a record proves transformable to EFI_FILE_INFO, add it
    // to the EFI_FILE_INFO cache (unless the cache is full).
    //
    Consumed = 0;
    while (Remaining >= sizeof (VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE)) {
      VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE  *Dirent;
      UINT32                              DirentSize;

      Dirent     = (VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE *)(DirentBuf + Consumed);
      DirentSize = (UINT32)VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE_SIZE (
                             Dirent->Namelen
                             );
      if (DirentSize == 0) {
        //
        // This means one of two things: (a) Dirent->Namelen is zero, or (b)
        // (b) Dirent->Namelen is unsupportably large. (a) is just invalid for
        // the Virtio Filesystem device to send, while (b) shouldn't happen
        // because "FilesysAttr.Namelen" -- the maximum filename length
        // supported by the filesystem -- proved acceptable above.
        //
        Status = EFI_PROTOCOL_ERROR;
        goto FreeFileInfoArray;
      }

      if (DirentSize > Remaining) {
        //
        // Dirent->Namelen suggests that the filename byte array (plus any
        // padding) are truncated. This should never happen; the Virtio
        // Filesystem device is supposed to send complete entries only.
        //
        Status = EFI_PROTOCOL_ERROR;
        goto FreeFileInfoArray;
      }

      if (Dirent->Namelen > FilesysAttr.Namelen) {
        //
        // This is possible without tripping the truncation check above, due to
        // how entries are padded. The condition means that Dirent->Namelen is
        // reportedly larger than the filesystem limit, without spilling into
        // the next alignment bucket. Should never happen.
        //
        Status = EFI_PROTOCOL_ERROR;
        goto FreeFileInfoArray;
      }

      //
      // If we haven't filled the EFI_FILE_INFO cache yet, attempt transforming
      // Dirent to EFI_FILE_INFO.
      //
      if (NumFileInfo < VIRTIO_FS_FILE_MAX_FILE_INFO) {
        EFI_FILE_INFO  *FileInfo;

        FileInfo = (EFI_FILE_INFO *)(FileInfoArray +
                                     (NumFileInfo * SingleFileInfoSize));
        Status = PopulateFileInfo (Dirent, SingleFileInfoSize, FileInfo);
        if (!EFI_ERROR (Status)) {
          //
          // Dirent has been transformed and cached successfully.
          //
          NumFileInfo++;
          //
          // The next time we refill the cache, restart reading the directory
          // stream right after the entry that we've just transformed and
          // cached.
          //
          CacheEndsAtCookie = Dirent->CookieForNextEntry;
        }

        //
        // If Dirent wasn't transformable to an EFI_FILE_INFO, we'll just skip
        // it.
        //
      }

      //
      // Make the Virtio Filesystem device forget the NodeId in this directory
      // entry, as we'll need it no more. (The "." and ".." entries need no
      // FUSE_FORGET requests, when returned by FUSE_READDIRPLUS -- and so the
      // Virtio Filesystem device reports their NodeId fields as zero.)
      //
      if (Dirent->NodeResp.NodeId != 0) {
        VirtioFsFuseForget (VirtioFs, Dirent->NodeResp.NodeId);
      }

      //
      // Advance to the next entry in DirentBuf.
      //
      DirStreamCookie = Dirent->CookieForNextEntry;
      Consumed       += DirentSize;
      Remaining      -= DirentSize;
    }

    if (Remaining > 0) {
      //
      // This suggests that a VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE header was
      // truncated. This should never happen; the Virtio Filesystem device is
      // supposed to send complete entries only.
      //
      Status = EFI_PROTOCOL_ERROR;
      goto FreeFileInfoArray;
    }

    //
    // Fetch another DirentBuf from the directory stream, unless we've filled
    // the EFI_FILE_INFO cache.
    //
  } while (NumFileInfo < VIRTIO_FS_FILE_MAX_FILE_INFO);

  //
  // Commit the results. (Note that the result may be an empty cache.)
  //
  if (VirtioFsFile->FileInfoArray != NULL) {
    FreePool (VirtioFsFile->FileInfoArray);
  }

  VirtioFsFile->FileInfoArray      = FileInfoArray;
  VirtioFsFile->SingleFileInfoSize = SingleFileInfoSize;
  VirtioFsFile->NumFileInfo        = NumFileInfo;
  VirtioFsFile->NextFileInfo       = 0;
  VirtioFsFile->FilePosition       = CacheEndsAtCookie;

  FreePool (DirentBuf);
  return EFI_SUCCESS;

FreeFileInfoArray:
  FreePool (FileInfoArray);

FreeDirentBuf:
  FreePool (DirentBuf);

  return Status;
}

/**
  Read an entry from the EFI_FILE_INFO cache.
**/
STATIC
EFI_STATUS
ReadFileInfoCache (
  IN OUT VIRTIO_FS_FILE  *VirtioFsFile,
  IN OUT UINTN           *BufferSize,
  OUT VOID               *Buffer
  )
{
  EFI_FILE_INFO  *FileInfo;
  UINTN          CallerAllocated;

  //
  // Refill the cache if needed. If the refill doesn't produce any new
  // EFI_FILE_INFO, report End of Directory, by setting (*BufferSize) to 0.
  //
  if (VirtioFsFile->NextFileInfo == VirtioFsFile->NumFileInfo) {
    EFI_STATUS  Status;

    Status = RefillFileInfoCache (VirtioFsFile);
    if (EFI_ERROR (Status)) {
      return (Status == EFI_BUFFER_TOO_SMALL) ? EFI_DEVICE_ERROR : Status;
    }

    if (VirtioFsFile->NumFileInfo == 0) {
      *BufferSize = 0;
      return EFI_SUCCESS;
    }
  }

  FileInfo = (EFI_FILE_INFO *)(VirtioFsFile->FileInfoArray +
                               (VirtioFsFile->NextFileInfo *
                                VirtioFsFile->SingleFileInfoSize));

  //
  // Check if the caller is ready to accept FileInfo. If not, we'll just
  // present the required size for now.
  //
  // (The (UINTN) cast below is safe because FileInfo->Size has been reduced
  // from VirtioFsFile->SingleFileInfoSize, in
  //
  //   RefillFileInfoCache()
  //     PopulateFileInfo()
  //       VirtioFsFuseDirentPlusToEfiFileInfo()
  //
  // and VirtioFsFile->SingleFileInfoSize was computed from
  // FilesysAttr.Namelen, which had been accepted by
  // VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE_SIZE().)
  //
  CallerAllocated = *BufferSize;
  *BufferSize     = (UINTN)FileInfo->Size;
  if (CallerAllocated < *BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Output FileInfo, and remove it from the cache.
  //
  CopyMem (Buffer, FileInfo, *BufferSize);
  VirtioFsFile->NextFileInfo++;
  return EFI_SUCCESS;
}

/**
  Read from a regular file.
**/
STATIC
EFI_STATUS
ReadRegularFile (
  IN OUT VIRTIO_FS_FILE  *VirtioFsFile,
  IN OUT UINTN           *BufferSize,
  OUT VOID               *Buffer
  )
{
  VIRTIO_FS                           *VirtioFs;
  EFI_STATUS                          Status;
  VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE  FuseAttr;
  UINTN                               Transferred;
  UINTN                               Left;

  VirtioFs = VirtioFsFile->OwnerFs;
  //
  // The UEFI spec forbids reads that start beyond the end of the file.
  //
  Status = VirtioFsFuseGetAttr (VirtioFs, VirtioFsFile->NodeId, &FuseAttr);
  if (EFI_ERROR (Status) || (VirtioFsFile->FilePosition > FuseAttr.Size)) {
    return EFI_DEVICE_ERROR;
  }

  Status      = EFI_SUCCESS;
  Transferred = 0;
  Left        = *BufferSize;
  while (Left > 0) {
    UINT32  ReadSize;

    //
    // FUSE_READ cannot express a >=4GB buffer size.
    //
    ReadSize = (UINT32)MIN ((UINTN)MAX_UINT32, Left);
    Status   = VirtioFsFuseReadFileOrDir (
                 VirtioFs,
                 VirtioFsFile->NodeId,
                 VirtioFsFile->FuseHandle,
                 FALSE,                                  // IsDir
                 VirtioFsFile->FilePosition + Transferred,
                 &ReadSize,
                 (UINT8 *)Buffer + Transferred
                 );
    if (EFI_ERROR (Status) || (ReadSize == 0)) {
      break;
    }

    Transferred += ReadSize;
    Left        -= ReadSize;
  }

  *BufferSize                 = Transferred;
  VirtioFsFile->FilePosition += Transferred;
  //
  // If we managed to read some data, return success. If zero bytes were
  // transferred due to zero-sized buffer on input or due to EOF on first read,
  // return SUCCESS. Otherwise, return the error due to which zero bytes were
  // transferred.
  //
  return (Transferred > 0) ? EFI_SUCCESS : Status;
}

EFI_STATUS
EFIAPI
VirtioFsSimpleFileRead (
  IN     EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN              *BufferSize,
  OUT VOID                  *Buffer
  )
{
  VIRTIO_FS_FILE  *VirtioFsFile;
  EFI_STATUS      Status;

  VirtioFsFile = VIRTIO_FS_FILE_FROM_SIMPLE_FILE (This);

  if (VirtioFsFile->IsDirectory) {
    Status = ReadFileInfoCache (VirtioFsFile, BufferSize, Buffer);
  } else {
    Status = ReadRegularFile (VirtioFsFile, BufferSize, Buffer);
  }

  return Status;
}
