/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "BootMonFsInternal.h"

// Clear a file's image description on storage media:
// UEFI allows you to seek past the end of a file, a subsequent write will grow
// the file. It does not specify how space between the former end of the file
// and the beginning of the write should be filled. It's therefore possible that
// BootMonFs metadata, that comes after the end of a file, could be left there
// and wrongly detected by BootMonFsImageInBlock.
STATIC
EFI_STATUS
InvalidateImageDescription (
  IN  BOOTMON_FS_FILE  *File
  )
{
  EFI_DISK_IO_PROTOCOL   *DiskIo;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  UINT32                  MediaId;
  UINT32                  BlockSize;
  VOID                   *Buffer;
  EFI_STATUS              Status;
  UINT64                  DescriptionAddress;

  DiskIo = File->Instance->DiskIo;
  BlockIo = File->Instance->BlockIo;
  MediaId = BlockIo->Media->MediaId;
  BlockSize = BlockIo->Media->BlockSize;

  DescriptionAddress = (File->HwDescription.BlockEnd * BlockSize)
                       - sizeof (HW_IMAGE_DESCRIPTION);

  Buffer = AllocateZeroPool (sizeof (HW_IMAGE_DESCRIPTION));

  Status = DiskIo->WriteDisk (DiskIo,
                    MediaId,
                    DescriptionAddress,
                    sizeof (HW_IMAGE_DESCRIPTION),
                    Buffer
                    );

  FreePool(Buffer);

  return Status;
}

// Flush file data that will extend the file's length. Update and, if necessary,
// move the image description.
// We need to pass the file's starting position on media (FileStart), because
// if the file hasn't been flushed before its Description->BlockStart won't
// have been initialised.
// FileStart must be aligned to the media's block size.
// Note that this function uses DiskIo to flush, so call BlockIo->FlushBlocks()
// after calling it.
STATIC
EFI_STATUS
FlushAppendRegion (
  IN BOOTMON_FS_FILE         *File,
  IN BOOTMON_FS_FILE_REGION  *Region,
  IN UINT64                   NewFileSize,
  IN UINT64                   FileStart
  )
{
  EFI_STATUS               Status;
  EFI_DISK_IO_PROTOCOL    *DiskIo;
  UINTN                    BlockSize;
  HW_IMAGE_DESCRIPTION    *Description;

  DiskIo = File->Instance->DiskIo;

  BlockSize = File->Instance->BlockIo->Media->BlockSize;

  ASSERT (FileStart % BlockSize == 0);

  // Only invalidate the Image Description of files that have already been
  // written in Flash
  if (File->HwDescription.RegionCount > 0) {
    Status = InvalidateImageDescription (File);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Update File Description
  //
  Description = &File->HwDescription;
  Description->Attributes = 1;
  Description->BlockStart = FileStart / BlockSize;
  Description->BlockEnd = Description->BlockStart + (NewFileSize / BlockSize);
  Description->Footer.FooterSignature1 = HW_IMAGE_FOOTER_SIGNATURE_1;
  Description->Footer.FooterSignature2 = HW_IMAGE_FOOTER_SIGNATURE_2;
  Description->Footer.Version = HW_IMAGE_FOOTER_VERSION;
  Description->Footer.Offset = HW_IMAGE_FOOTER_OFFSET;
  Description->RegionCount = 1;
  Description->Region[0].Checksum = 0;
  Description->Region[0].Offset = Description->BlockStart * BlockSize;
  Description->Region[0].Size = NewFileSize - sizeof (HW_IMAGE_DESCRIPTION);

  Status = BootMonFsComputeFooterChecksum (Description);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Write the new file data
  Status = DiskIo->WriteDisk (
                    DiskIo,
                    File->Instance->Media->MediaId,
                    FileStart + Region->Offset,
                    Region->Size,
                    Region->Buffer
                    );
  ASSERT_EFI_ERROR (Status);

  // Round the file size up to the nearest block size
  if ((NewFileSize % BlockSize) > 0) {
    NewFileSize += BlockSize - (NewFileSize % BlockSize);
  }
  // Update the file description on the media
  Status = DiskIo->WriteDisk (
                    DiskIo,
                    File->Instance->Media->MediaId,
                    (FileStart + NewFileSize) - sizeof (HW_IMAGE_DESCRIPTION),
                    sizeof (HW_IMAGE_DESCRIPTION),
                    Description
                    );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

BOOLEAN
BootMonFsFileNeedFlush (
  IN BOOTMON_FS_FILE         *File
  )
{
  return !IsListEmpty (&File->RegionToFlushLink);
}

// Find a space on media for a file that has not yet been flushed to disk.
// Just returns the first space that's big enough.
// This function could easily be adapted to:
// - Find space for moving an existing file that has outgrown its space
//   (We do not currently move files, just return EFI_VOLUME_FULL)
// - Find space for a fragment of a file that has outgrown its space
//   (We do not currently fragment files - it's not clear whether fragmentation
//    is actually part of BootMonFs as there is no spec)
// - Be more clever about finding space (choosing the largest or smallest
//   suitable space)
// Parameters:
// File - the new (not yet flushed) file for which we need to find space.
// FileStart - the position on media of the file (in bytes).
STATIC
EFI_STATUS
BootMonFsFindSpaceForNewFile (
  IN  BOOTMON_FS_FILE     *File,
  OUT UINT64              *FileStart
  )
{
  LIST_ENTRY              *FileLink;
  BOOTMON_FS_FILE         *RootFile;
  BOOTMON_FS_FILE         *FileEntry;
  UINTN                    BlockSize;
  UINT64                   FileSize;
  EFI_BLOCK_IO_MEDIA      *Media;

  Media = File->Instance->BlockIo->Media;
  BlockSize = Media->BlockSize;
  RootFile = File->Instance->RootFile;

  if (IsListEmpty (&RootFile->Link)) {
    return EFI_SUCCESS;
  }

  // This function must only be called for file which has not been flushed into
  // Flash yet
  ASSERT (File->HwDescription.RegionCount == 0);

  // Find out how big the file will be
  FileSize = BootMonFsGetImageLength (File);
  // Add the file header to the file
  FileSize += sizeof (HW_IMAGE_DESCRIPTION);

  *FileStart = 0;
  // Go through all the files in the list
  for (FileLink = GetFirstNode (&RootFile->Link);
         !IsNull (&RootFile->Link, FileLink);
         FileLink = GetNextNode (&RootFile->Link, FileLink)
         )
  {
    FileEntry = BOOTMON_FS_FILE_FROM_LINK_THIS (FileLink);
    // If the free space preceding the file is big enough to contain the new
    // file then use it!
    if (((FileEntry->HwDescription.BlockStart * BlockSize) - *FileStart)
        >= FileSize) {
      // The file list must be in disk-order
      RemoveEntryList (&File->Link);
      File->Link.BackLink = FileLink->BackLink;
      File->Link.ForwardLink = FileLink;
      FileLink->BackLink->ForwardLink = &File->Link;
      FileLink->BackLink = &File->Link;

      return EFI_SUCCESS;
    } else {
      *FileStart = (FileEntry->HwDescription.BlockEnd + 1) * BlockSize;
    }
  }
  // See if there's space after the last file
  if ((((Media->LastBlock + 1) * BlockSize) - *FileStart) >= FileSize) {
    return EFI_SUCCESS;
  } else {
    return EFI_VOLUME_FULL;
  }
}

// Free the resources in the file's Region list.
STATIC
VOID
FreeFileRegions (
  IN  BOOTMON_FS_FILE *File
  )
{
  LIST_ENTRY              *RegionToFlushLink;
  BOOTMON_FS_FILE_REGION  *Region;

  RegionToFlushLink = GetFirstNode (&File->RegionToFlushLink);
  while (!IsNull (&File->RegionToFlushLink, RegionToFlushLink)) {
    // Repeatedly remove the first node from the list and free its resources.
    Region = (BOOTMON_FS_FILE_REGION *) RegionToFlushLink;
    RemoveEntryList (RegionToFlushLink);
    FreePool (Region->Buffer);
    FreePool (Region);

    RegionToFlushLink = GetFirstNode (&File->RegionToFlushLink);
  }
}

EFIAPI
EFI_STATUS
BootMonFsFlushFile (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS               Status;
  BOOTMON_FS_INSTANCE     *Instance;
  LIST_ENTRY              *RegionToFlushLink;
  BOOTMON_FS_FILE         *File;
  BOOTMON_FS_FILE         *NextFile;
  BOOTMON_FS_FILE_REGION  *Region;
  LIST_ENTRY              *FileLink;
  UINTN                    CurrentPhysicalSize;
  UINTN                    BlockSize;
  UINT64                   FileStart;
  UINT64                   FileEnd;
  UINT64                   RegionStart;
  UINT64                   RegionEnd;
  UINT64                   NewFileSize;
  UINT64                   EndOfAppendSpace;
  BOOLEAN                  HasSpace;
  EFI_DISK_IO_PROTOCOL    *DiskIo;
  EFI_BLOCK_IO_PROTOCOL   *BlockIo;

  Status      = EFI_SUCCESS;
  FileStart   = 0;

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Check if the file needs to be flushed
  if (!BootMonFsFileNeedFlush (File)) {
    return Status;
  }

  Instance = File->Instance;
  BlockIo = Instance->BlockIo;
  DiskIo = Instance->DiskIo;
  BlockSize = BlockIo->Media->BlockSize;

  // If the file doesn't exist then find a space for it
  if (File->HwDescription.RegionCount == 0) {
    Status = BootMonFsFindSpaceForNewFile (File, &FileStart);
    // FileStart has changed so we need to recompute RegionEnd
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    FileStart = File->HwDescription.BlockStart * BlockSize;
  }

  // FileEnd is the NOR address of the end of the file's data
  FileEnd = FileStart + BootMonFsGetImageLength (File);

  for (RegionToFlushLink = GetFirstNode (&File->RegionToFlushLink);
       !IsNull (&File->RegionToFlushLink, RegionToFlushLink);
       RegionToFlushLink = GetNextNode (&File->RegionToFlushLink, RegionToFlushLink)
       )
  {
    Region = (BOOTMON_FS_FILE_REGION*)RegionToFlushLink;

    // RegionStart and RegionEnd are the the intended NOR address of the
    // start and end of the region
    RegionStart = FileStart + Region->Offset;
    RegionEnd = RegionStart + Region->Size;

    if (RegionEnd < FileEnd) {
      // Handle regions representing edits to existing portions of the file
      // Write the region data straight into the file
      Status = DiskIo->WriteDisk (DiskIo,
                        BlockIo->Media->MediaId,
                        RegionStart,
                        Region->Size,
                        Region->Buffer
                        );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    } else {
      // Handle regions representing appends to the file
      //
      // Note: Since seeking past the end of the file with SetPosition() is
      //  valid, it's possible there will be a gap between the current end of
      //  the file and the beginning of the new region. Since the UEFI spec
      //  says nothing about this case (except "a subsequent write would grow
      //  the file"), we just leave garbage in the gap.

      // Check if there is space to append the new region
      HasSpace = FALSE;
      NewFileSize = (RegionEnd - FileStart) + sizeof (HW_IMAGE_DESCRIPTION);
      CurrentPhysicalSize = BootMonFsGetPhysicalSize (File);
      if (NewFileSize <= CurrentPhysicalSize) {
        HasSpace = TRUE;
      } else {
        // Get the File Description for the next file
        FileLink = GetNextNode (&Instance->RootFile->Link, &File->Link);
        if (!IsNull (&Instance->RootFile->Link, FileLink)) {
          NextFile = BOOTMON_FS_FILE_FROM_LINK_THIS (FileLink);

          // If there is space between the beginning of the current file and the
          // beginning of the next file then use it
          EndOfAppendSpace = NextFile->HwDescription.BlockStart * BlockSize;
        } else {
          // We are flushing the last file.
          EndOfAppendSpace = (BlockIo->Media->LastBlock + 1) * BlockSize;
        }
        if (EndOfAppendSpace - FileStart >= NewFileSize) {
          HasSpace = TRUE;
        }
      }

      if (HasSpace == TRUE) {
        Status = FlushAppendRegion (File, Region, NewFileSize, FileStart);
        if (EFI_ERROR (Status)) {
          return Status;
        }
      } else {
        // There isn't a space for the file.
        // Options here are to move the file or fragment it. However as files
        // may represent boot images at fixed positions, these options will
        // break booting if the bootloader doesn't use BootMonFs to find the
        // image.

        return EFI_VOLUME_FULL;
      }
    }
  }

  FreeFileRegions (File);

  // Flush DiskIo Buffers (see UEFI Spec 12.7 - DiskIo buffers are flushed by
  // calling FlushBlocks on the same device's BlockIo).
  BlockIo->FlushBlocks (BlockIo);

  return Status;
}

/**
  Closes a file on the Nor Flash FS volume.

  @param  This  The EFI_FILE_PROTOCOL to close.

  @return Always returns EFI_SUCCESS.

**/
EFIAPI
EFI_STATUS
BootMonFsCloseFile (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  // Flush the file if needed
  This->Flush (This);
  return EFI_SUCCESS;
}

// Create a new instance of BOOTMON_FS_FILE.
// Uses BootMonFsCreateFile to
STATIC
EFI_STATUS
CreateNewFile (
  IN  BOOTMON_FS_INSTANCE  *Instance,
  IN  CHAR8*                AsciiFileName,
  OUT BOOTMON_FS_FILE     **NewHandle
  )
{
  EFI_STATUS       Status;
  BOOTMON_FS_FILE *File;

  Status = BootMonFsCreateFile (Instance, &File);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Remove the leading '\\'
  if (*AsciiFileName == '\\') {
    AsciiFileName++;
  }

  // Set the file name
  CopyMem (File->HwDescription.Footer.Filename, AsciiFileName, MAX_NAME_LENGTH);

  // Add the file to list of files of the File System
  InsertHeadList (&Instance->RootFile->Link, &File->Link);

  *NewHandle = File;
  return Status;
}

/**
  Opens a file on the Nor Flash FS volume

  Calls BootMonFsGetFileFromAsciiFilename to search the list of tracked files.

  @param  This  The EFI_FILE_PROTOCOL parent handle.
  @param  NewHandle Double-pointer to the newly created protocol.
  @param  FileName The name of the image/metadata on flash
  @param  OpenMode Read,write,append etc
  @param  Attributes ?

  @return EFI_STATUS
  OUT_OF_RESOURCES
    Run out of space to keep track of the allocated structures
  DEVICE_ERROR
    Unable to locate the volume associated with the parent file handle
  NOT_FOUND
    Filename wasn't found on flash
  SUCCESS

**/
EFIAPI
EFI_STATUS
BootMonFsOpenFile (
  IN EFI_FILE_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL **NewHandle,
  IN CHAR16             *FileName,
  IN UINT64             OpenMode,
  IN UINT64             Attributes
  )
{
  BOOTMON_FS_FILE     *Directory;
  BOOTMON_FS_FILE     *File;
  BOOTMON_FS_INSTANCE *Instance;
  CHAR8*               AsciiFileName;
  EFI_STATUS           Status;

  if ((FileName == NULL) || (NewHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // The only valid modes are read, read/write, and read/write/create
  if (!(OpenMode & EFI_FILE_MODE_READ) || ((OpenMode & EFI_FILE_MODE_CREATE)  && !(OpenMode & EFI_FILE_MODE_WRITE))) {
    return EFI_INVALID_PARAMETER;
  }

  Directory = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (Directory == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Instance = Directory->Instance;

  // If the instance has not been initialized it yet then do it ...
  if (!Instance->Initialized) {
    Status = BootMonFsInitialize (Instance);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  // BootMonFs interface requires ASCII filenames
  AsciiFileName = AllocatePool ((StrLen (FileName) + 1) * sizeof (CHAR8));
  if (AsciiFileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  UnicodeStrToAsciiStr (FileName, AsciiFileName);

  if ((AsciiStrCmp (AsciiFileName, "\\") == 0) ||
      (AsciiStrCmp (AsciiFileName, "/")  == 0) ||
      (AsciiStrCmp (AsciiFileName, "")   == 0) ||
      (AsciiStrCmp (AsciiFileName, ".")  == 0))
  {
    //
    // Opening '/', '\', '.', or the NULL pathname is trying to open the root directory
    //

    *NewHandle = &Instance->RootFile->File;
    Instance->RootFile->Position = 0;
    Status = EFI_SUCCESS;
  } else {
    //
    // Open or Create a regular file
    //

    // Check if the file already exists
    Status = BootMonGetFileFromAsciiFileName (Instance, AsciiFileName, &File);
    if (Status == EFI_NOT_FOUND) {
      // The file doesn't exist.
      if (OpenMode & EFI_FILE_MODE_CREATE) {
        // If the file does not exist but is required then create it.
        if (Attributes & EFI_FILE_DIRECTORY) {
          // BootMonFS doesn't support subdirectories
          Status = EFI_UNSUPPORTED;
        } else {
          // Create a new file
          Status = CreateNewFile (Instance, AsciiFileName, &File);
          if (!EFI_ERROR (Status)) {
            File->OpenMode = OpenMode;
            *NewHandle = &File->File;
            File->Position = 0;
          }
        }
      }
    } else if (Status == EFI_SUCCESS) {
      // The file exists
      File->OpenMode = OpenMode;
      *NewHandle = &File->File;
      File->Position = 0;
    }
  }

  FreePool (AsciiFileName);

  return Status;
}

// Delete() for the root directory's EFI_FILE_PROTOCOL instance
EFIAPI
EFI_STATUS
BootMonFsDeleteFail (
  IN EFI_FILE_PROTOCOL *This
  )
{
  This->Close(This);
  // You can't delete the root directory
  return EFI_WARN_DELETE_FAILURE;
}
EFIAPI
EFI_STATUS
BootMonFsDelete (
  IN EFI_FILE_PROTOCOL *This
  )
{
  EFI_STATUS               Status;
  BOOTMON_FS_FILE         *File;
  LIST_ENTRY              *RegionToFlushLink;
  BOOTMON_FS_FILE_REGION  *Region;
  HW_IMAGE_DESCRIPTION    *Description;
  EFI_BLOCK_IO_PROTOCOL   *BlockIo;
  UINT8                   *EmptyBuffer;

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (File == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Status = EFI_SUCCESS;

  if (BootMonFsFileNeedFlush (File)) {
    // Free the entries from the Buffer List
    RegionToFlushLink = GetFirstNode (&File->RegionToFlushLink);
    do {
      Region = (BOOTMON_FS_FILE_REGION*)RegionToFlushLink;

      // Get Next entry
      RegionToFlushLink = RemoveEntryList (RegionToFlushLink);

      // Free the buffers
      FreePool (Region->Buffer);
      FreePool (Region);
    } while (!IsListEmpty (&File->RegionToFlushLink));
  }

  // If (RegionCount is greater than 0) then the file already exists
  if (File->HwDescription.RegionCount > 0) {
    Description = &File->HwDescription;
    BlockIo = File->Instance->BlockIo;

    // Create an empty buffer
    EmptyBuffer = AllocateZeroPool (BlockIo->Media->BlockSize);
    if (EmptyBuffer == NULL) {
      FreePool (File);
      return EFI_OUT_OF_RESOURCES;
    }

    // Invalidate the last Block
    Status = BlockIo->WriteBlocks (BlockIo, BlockIo->Media->MediaId, Description->BlockEnd, BlockIo->Media->BlockSize, EmptyBuffer);
    ASSERT_EFI_ERROR (Status);

    FreePool (EmptyBuffer);
  }

  // Remove the entry from the list
  RemoveEntryList (&File->Link);
  FreePool (File);
  return Status;
}

