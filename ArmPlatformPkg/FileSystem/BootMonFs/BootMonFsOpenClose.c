/** @file
*
*  Copyright (c) 2012-2015, ARM Limited. All rights reserved.
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
  VOID                   *Buffer;
  EFI_STATUS              Status;

  DiskIo = File->Instance->DiskIo;
  BlockIo = File->Instance->BlockIo;
  MediaId = BlockIo->Media->MediaId;

  Buffer = AllocateZeroPool (sizeof (HW_IMAGE_DESCRIPTION));

  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = DiskIo->WriteDisk (DiskIo,
                    MediaId,
                    File->HwDescAddress,
                    sizeof (HW_IMAGE_DESCRIPTION),
                    Buffer
                    );

  FreePool(Buffer);

  return Status;
}

/**
  Write the description of a file to storage media.

  This function uses DiskIo to write to the media, so call BlockIo->FlushBlocks()
  after calling it to ensure the data are written on the media.

  @param[in]  File       Description of the file whose description on the
                         storage media has to be updated.
  @param[in]  FileName   Name of the file. Its length is assumed to be
                         lower than MAX_NAME_LENGTH.
  @param[in]  DataSize   Number of data bytes of the file.
  @param[in]  FileStart  File's starting position on media. FileStart must
                         be aligned to the media's block size.

  @retval  EFI_WRITE_PROTECTED  The device cannot be written to.
  @retval  EFI_DEVICE_ERROR     The device reported an error while performing
                                the write operation.

**/
STATIC
EFI_STATUS
WriteFileDescription (
  IN  BOOTMON_FS_FILE  *File,
  IN  CHAR8            *FileName,
  IN  UINT32            DataSize,
  IN  UINT64            FileStart
  )
{
  EFI_STATUS            Status;
  EFI_DISK_IO_PROTOCOL  *DiskIo;
  UINTN                 BlockSize;
  UINT32                FileSize;
  HW_IMAGE_DESCRIPTION  *Description;

  DiskIo    = File->Instance->DiskIo;
  BlockSize = File->Instance->BlockIo->Media->BlockSize;
  ASSERT (FileStart % BlockSize == 0);

  //
  // Construct the file description
  //

  FileSize = DataSize + sizeof (HW_IMAGE_DESCRIPTION);
  Description = &File->HwDescription;
  Description->Attributes = 1;
  Description->BlockStart = FileStart / BlockSize;
  Description->BlockEnd   = Description->BlockStart + (FileSize / BlockSize);
  AsciiStrCpy (Description->Footer.Filename, FileName);

#ifdef MDE_CPU_ARM
  Description->Footer.Offset  = HW_IMAGE_FOOTER_OFFSET;
  Description->Footer.Version = HW_IMAGE_FOOTER_VERSION;
#else
  Description->Footer.Offset  = HW_IMAGE_FOOTER_OFFSET2;
  Description->Footer.Version = HW_IMAGE_FOOTER_VERSION2;
#endif
  Description->Footer.FooterSignature1 = HW_IMAGE_FOOTER_SIGNATURE_1;
  Description->Footer.FooterSignature2 = HW_IMAGE_FOOTER_SIGNATURE_2;
  Description->RegionCount = 1;
  Description->Region[0].Checksum = 0;
  Description->Region[0].Offset = Description->BlockStart * BlockSize;
  Description->Region[0].Size = DataSize;

  Status = BootMonFsComputeFooterChecksum (Description);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  File->HwDescAddress = ((Description->BlockEnd + 1) * BlockSize) - sizeof (HW_IMAGE_DESCRIPTION);

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
  IN  UINT64              FileSize,
  OUT UINT64              *FileStart
  )
{
  LIST_ENTRY              *FileLink;
  BOOTMON_FS_FILE         *RootFile;
  BOOTMON_FS_FILE         *FileEntry;
  UINTN                    BlockSize;
  EFI_BLOCK_IO_MEDIA      *Media;

  Media = File->Instance->BlockIo->Media;
  BlockSize = Media->BlockSize;
  RootFile = File->Instance->RootFile;

  // This function must only be called for file which has not been flushed into
  // Flash yet
  ASSERT (File->HwDescription.RegionCount == 0);

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

/**
  Flush all modified data associated with a file to a device.

  @param[in]  This  A pointer to the EFI_FILE_PROTOCOL instance that is the
                    file handle to flush.

  @retval  EFI_SUCCESS            The data was flushed.
  @retval  EFI_ACCESS_DENIED      The file was opened read-only.
  @retval  EFI_DEVICE_ERROR       The device reported an error.
  @retval  EFI_VOLUME_FULL        The volume is full.
  @retval  EFI_OUT_OF_RESOURCES   Not enough resources were available to flush the data.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
EFIAPI
EFI_STATUS
BootMonFsFlushFile (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  EFI_STATUS               Status;
  BOOTMON_FS_INSTANCE     *Instance;
  EFI_FILE_INFO           *Info;
  EFI_BLOCK_IO_PROTOCOL   *BlockIo;
  EFI_BLOCK_IO_MEDIA      *Media;
  EFI_DISK_IO_PROTOCOL    *DiskIo;
  UINTN                    BlockSize;
  CHAR8                    AsciiFileName[MAX_NAME_LENGTH];
  LIST_ENTRY              *RegionToFlushLink;
  BOOTMON_FS_FILE         *File;
  BOOTMON_FS_FILE         *NextFile;
  BOOTMON_FS_FILE_REGION  *Region;
  LIST_ENTRY              *FileLink;
  UINTN                    CurrentPhysicalSize;
  UINT64                   FileStart;
  UINT64                   FileEnd;
  UINT64                   RegionStart;
  UINT64                   RegionEnd;
  UINT64                   NewDataSize;
  UINT64                   NewFileSize;
  UINT64                   EndOfAppendSpace;
  BOOLEAN                  HasSpace;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (File->Info == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (File->OpenMode == EFI_FILE_MODE_READ) {
    return EFI_ACCESS_DENIED;
  }

  Instance  = File->Instance;
  Info      = File->Info;
  BlockIo   = Instance->BlockIo;
  Media     = BlockIo->Media;
  DiskIo    = Instance->DiskIo;
  BlockSize = Media->BlockSize;

  UnicodeStrToAsciiStr (Info->FileName, AsciiFileName);

  // If the file doesn't exist then find a space for it
  if (File->HwDescription.RegionCount == 0) {
    Status = BootMonFsFindSpaceForNewFile (
               File,
               Info->FileSize + sizeof (HW_IMAGE_DESCRIPTION),
               &FileStart
               );
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
    if (Region->Size == 0) {
      continue;
    }

    // RegionStart and RegionEnd are the the intended NOR address of the
    // start and end of the region
    RegionStart = FileStart   + Region->Offset;
    RegionEnd   = RegionStart + Region->Size;

    if (RegionEnd < FileEnd) {
      // Handle regions representing edits to existing portions of the file
      // Write the region data straight into the file
      Status = DiskIo->WriteDisk (DiskIo,
                        Media->MediaId,
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
      NewDataSize = RegionEnd - FileStart;
      NewFileSize = NewDataSize + sizeof (HW_IMAGE_DESCRIPTION);
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
          EndOfAppendSpace = (Media->LastBlock + 1) * BlockSize;
        }
        if (EndOfAppendSpace - FileStart >= NewFileSize) {
          HasSpace = TRUE;
        }
      }

      if (HasSpace == TRUE) {
        // Invalidate the current image description of the file if any.
        if (File->HwDescAddress != 0) {
          Status = InvalidateImageDescription (File);
          if (EFI_ERROR (Status)) {
            return Status;
          }
        }

        // Write the new file data
        Status = DiskIo->WriteDisk (
                    DiskIo,
                    Media->MediaId,
                    RegionStart,
                    Region->Size,
                    Region->Buffer
                    );
        if (EFI_ERROR (Status)) {
          return Status;
        }

        Status = WriteFileDescription (File, AsciiFileName, NewDataSize, FileStart);
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
  Info->PhysicalSize = BootMonFsGetPhysicalSize (File);

  if ((AsciiStrCmp (AsciiFileName, File->HwDescription.Footer.Filename) != 0) ||
      (Info->FileSize != File->HwDescription.Region[0].Size)               ) {
    Status = WriteFileDescription (File, AsciiFileName, Info->FileSize, FileStart);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  // Flush DiskIo Buffers (see UEFI Spec 12.7 - DiskIo buffers are flushed by
  // calling FlushBlocks on the same device's BlockIo).
  BlockIo->FlushBlocks (BlockIo);

  return EFI_SUCCESS;
}

/**
  Close a specified file handle.

  @param[in]  This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                    handle to close.

  @retval  EFI_SUCCESS            The file was closed.
  @retval  EFI_INVALID_PARAMETER  The parameter "This" is NULL or is not an open
                                  file handle.

**/
EFIAPI
EFI_STATUS
BootMonFsCloseFile (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  BOOTMON_FS_FILE  *File;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (File->Info == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // In the case of a file and not the root directory
  if (This != &File->Instance->RootFile->File) {
    This->Flush (This);
    FreePool (File->Info);
    File->Info = NULL;
  }

  return EFI_SUCCESS;
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
  EFI_FILE_INFO        *Info;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Directory = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (Directory->Info == NULL) {
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
  Info = NULL;

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
    // Allocate a buffer to store the characteristics of the file while the
    // file is open. We allocate the maximum size to not have to reallocate
    // if the file name is changed.
    //
    Info = AllocateZeroPool (
             SIZE_OF_EFI_FILE_INFO + (sizeof (CHAR16) * MAX_NAME_LENGTH));
    if (Info == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
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

      Status = BootMonFsCreateFile (Instance, &File);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
      InsertHeadList (&Instance->RootFile->Link, &File->Link);
      Info->Attribute = Attributes;
    } else {
      //
      // File already open, not supported yet.
      //
      if (File->Info != NULL) {
        Status = EFI_UNSUPPORTED;
        goto Error;
      }
    }

    Info->FileSize     = BootMonFsGetImageLength (File);
    Info->PhysicalSize = BootMonFsGetPhysicalSize (File);
    AsciiStrToUnicodeStr (AsciiFileName, Info->FileName);

    File->Info = Info;
    Info = NULL;
    File->Position = 0;
    File->OpenMode = OpenMode;

    *NewHandle = &File->File;
  }

Error:

  FreePool (Buf);
  if (AsciiFileName != NULL) {
    FreePool (AsciiFileName);
  }
  if (Info != NULL) {
    FreePool (Info);
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

/**
  Close and delete a file from the boot monitor file system.

  @param[in]  This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                    handle to delete.

  @retval  EFI_SUCCESS              The file was closed and deleted.
  @retval  EFI_INVALID_PARAMETER    The parameter "This" is NULL or is not an open
                                    file handle.
  @retval  EFI_WARN_DELETE_FAILURE  The handle was closed, but the file was not deleted.

**/
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

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (File->Info == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsListEmpty (&File->RegionToFlushLink)) {
    // Free the entries from the Buffer List
    RegionToFlushLink = GetFirstNode (&File->RegionToFlushLink);
    do {
      Region = (BOOTMON_FS_FILE_REGION*)RegionToFlushLink;

      //
      // Get next element of the list before deleting the region description
      // that contain the LIST_ENTRY structure.
      //
      RegionToFlushLink = RemoveEntryList (RegionToFlushLink);

      // Free the buffers
      FreePool (Region->Buffer);
      FreePool (Region);
    } while (!IsListEmpty (&File->RegionToFlushLink));
  }

  // If (RegionCount is greater than 0) then the file already exists
  if (File->HwDescription.RegionCount > 0) {
    // Invalidate the last Block
    Status = InvalidateImageDescription (File);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return  EFI_WARN_DELETE_FAILURE;
    }
  }

  // Remove the entry from the list
  RemoveEntryList (&File->Link);
  FreePool (File->Info);
  FreePool (File);

  return EFI_SUCCESS;
}
