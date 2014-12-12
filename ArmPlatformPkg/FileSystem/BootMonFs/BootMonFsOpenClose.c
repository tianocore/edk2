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

#include <Library/PathLib.h>
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
  VOID                   *Buffer;
  EFI_STATUS              Status;

  DiskIo = File->Instance->DiskIo;
  BlockIo = File->Instance->BlockIo;
  MediaId = BlockIo->Media->MediaId;

  Buffer = AllocateZeroPool (sizeof (HW_IMAGE_DESCRIPTION));

  Status = DiskIo->WriteDisk (DiskIo,
                    MediaId,
                    File->HwDescAddress,
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
  if (File->HwDescAddress != 0) {
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
#ifdef MDE_CPU_ARM
  Description->Footer.Version = HW_IMAGE_FOOTER_VERSION;
  Description->Footer.Offset = HW_IMAGE_FOOTER_OFFSET;
#else
  Description->Footer.Version = HW_IMAGE_FOOTER_VERSION2;
  Description->Footer.Offset = HW_IMAGE_FOOTER_OFFSET2;
#endif
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

  File->HwDescAddress = (FileStart + NewFileSize) - sizeof (HW_IMAGE_DESCRIPTION);

  // Update the file description on the media
  Status = DiskIo->WriteDisk (
                    DiskIo,
                    File->Instance->Media->MediaId,
                    File->HwDescAddress,
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
    // Skip files that aren't on disk yet
    if (FileEntry->HwDescription.RegionCount == 0) {
      continue;
    }

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

  // FileEnd is the current NOR address of the end of the file's data
  FileEnd = FileStart + File->HwDescription.Region[0].Size;

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
  Open a file on the boot monitor file system.

  The boot monitor file system does not allow for sub-directories. There is only
  one directory, the root one. On any attempt to create a directory, the function
  returns in error with the EFI_WRITE_PROTECTED error code.

  @param[in]   This        A pointer to the EFI_FILE_PROTOCOL instance that is
                           the file handle to source location.
  @param[out]  NewHandle   A pointer to the location to return the opened
                           handle for the new file.
  @param[in]   FileName    The Null-terminated string of the name of the file
                           to be opened.
  @param[in]   OpenMode    The mode to open the file : Read or Read/Write or
                           Read/Write/Create
  @param[in]   Attributes  Attributes of the file in case of a file creation

  @retval  EFI_SUCCESS            The file was open.
  @retval  EFI_NOT_FOUND          The specified file could not be found or the specified
                                  directory in which to create a file could not be found.
  @retval  EFI_DEVICE_ERROR       The device reported an error.
  @retval  EFI_WRITE_PROTECTED    Attempt to create a directory. This is not possible
                                  with the Boot Monitor file system.
  @retval  EFI_OUT_OF_RESOURCES   Not enough resources were available to open the file.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
EFIAPI
EFI_STATUS
BootMonFsOpenFile (
  IN EFI_FILE_PROTOCOL   *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN CHAR16              *FileName,
  IN UINT64              OpenMode,
  IN UINT64              Attributes
  )
{
  EFI_STATUS           Status;
  BOOTMON_FS_FILE      *Directory;
  BOOTMON_FS_FILE      *File;
  BOOTMON_FS_INSTANCE  *Instance;
  CHAR8                *Buf;
  CHAR16               *Path;
  CHAR16               *Separator;
  CHAR8                *AsciiFileName;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((FileName == NULL) || (NewHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The only valid modes are read, read/write, and read/write/create
  //
  if ( (OpenMode != EFI_FILE_MODE_READ) &&
       (OpenMode != (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE)) &&
       (OpenMode != (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE)) ) {
    return EFI_INVALID_PARAMETER;
  }

  Directory = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (Directory == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = Directory->Instance;

  //
  // If the instance has not been initialized yet then do it ...
  //
  if (!Instance->Initialized) {
    Status = BootMonFsInitialize (Instance);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Copy the file path to be able to work on it. We do not want to
  // modify the input file name string "FileName".
  //
  Buf = AllocateCopyPool (StrSize (FileName), FileName);
  if (Buf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Path = (CHAR16*)Buf;
  AsciiFileName = NULL;

  //
  // Handle single periods, double periods and convert forward slashes '/'
  // to backward '\' ones. Does not handle a '.' at the beginning of the
  // path for the time being.
  //
  if (PathCleanUpDirectories (Path) == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Error;
  }

  //
  // Detect if the first component of the path refers to a directory.
  // This is done to return the correct error code when trying to
  // access or create a directory other than the root directory.
  //

  //
  // Search for the '\\' sequence and if found return in error
  // with the EFI_INVALID_PARAMETER error code. ere in the path.
  //
  if (StrStr (Path, L"\\\\") != NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Error;
  }
  //
  // Get rid of the leading '\' if any.
  //
  Path += (Path[0] == L'\\');

  //
  // Look for a '\' in the file path. If one is found then
  // the first component of the path refers to a directory
  // that is not the root directory.
  //
  Separator = StrStr (Path, L"\\");
  if (Separator != NULL) {
    //
    // In the case '<dir name>\' and a creation, return
    // EFI_WRITE_PROTECTED if this is for a directory
    // creation, EFI_INVALID_PARAMETER otherwise.
    //
    if ((*(Separator + 1) == '\0') && ((OpenMode & EFI_FILE_MODE_CREATE) != 0)) {
      if (Attributes & EFI_FILE_DIRECTORY) {
        Status = EFI_WRITE_PROTECTED;
      } else {
        Status = EFI_INVALID_PARAMETER;
      }
    } else {
      //
      // Attempt to open a file or a directory that is not in the
      // root directory or to open without creation a directory
      // located in the root directory, returns EFI_NOT_FOUND.
      //
      Status = EFI_NOT_FOUND;
    }
    goto Error;
  }

  //
  // BootMonFs interface requires ASCII filenames
  //
  AsciiFileName = AllocatePool (StrLen (Path) + 1);
  if (AsciiFileName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }
  UnicodeStrToAsciiStr (Path, AsciiFileName);
  if (AsciiStrSize (AsciiFileName) > MAX_NAME_LENGTH) {
   AsciiFileName[MAX_NAME_LENGTH - 1] = '\0';
  }

  if ((AsciiFileName[0] == '\0') ||
      (AsciiFileName[0] == '.' )    ) {
    //
    // Opening the root directory
    //

    *NewHandle = &Instance->RootFile->File;
    Instance->RootFile->Position = 0;
    Status = EFI_SUCCESS;
  } else {

    if ((OpenMode & EFI_FILE_MODE_CREATE) &&
        (Attributes & EFI_FILE_DIRECTORY)    ) {
      Status = EFI_WRITE_PROTECTED;
      goto Error;
    }

    //
    // Open or create a file in the root directory.
    //

    Status = BootMonGetFileFromAsciiFileName (Instance, AsciiFileName, &File);
    if (Status == EFI_NOT_FOUND) {
      if ((OpenMode & EFI_FILE_MODE_CREATE) == 0) {
        goto Error;
      }

      Status = CreateNewFile (Instance, AsciiFileName, &File);
      if (!EFI_ERROR (Status)) {
        File->OpenMode = OpenMode;
        *NewHandle = &File->File;
        File->Position = 0;
      }
    } else {
      //
      // The file already exists.
      //
      File->OpenMode = OpenMode;
      *NewHandle = &File->File;
      File->Position = 0;
    }
  }

Error:

  FreePool (Buf);
  if (AsciiFileName != NULL) {
    FreePool (AsciiFileName);
  }

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
    BlockIo = File->Instance->BlockIo;

    // Create an empty buffer
    EmptyBuffer = AllocateZeroPool (BlockIo->Media->BlockSize);
    if (EmptyBuffer == NULL) {
      FreePool (File);
      return EFI_OUT_OF_RESOURCES;
    }

    // Invalidate the last Block
    Status = InvalidateImageDescription (File);
    ASSERT_EFI_ERROR (Status);

    FreePool (EmptyBuffer);
  }

  // Remove the entry from the list
  RemoveEntryList (&File->Link);
  FreePool (File);
  return Status;
}

