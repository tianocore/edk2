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

EFIAPI
EFI_STATUS
OpenBootMonFsOpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
  OUT EFI_FILE_PROTOCOL              **Root
  )
{
  BOOTMON_FS_INSTANCE *Instance;

  Instance = BOOTMON_FS_FROM_FS_THIS (This);
  if (Instance == NULL) {
    return EFI_DEVICE_ERROR;
  }

  *Root = &Instance->RootFile->File;

  return EFI_SUCCESS;
}

UINT32
BootMonFsGetImageLength (
  IN BOOTMON_FS_FILE      *File
  )
{
  UINT32                   Index;
  UINT32                   FileSize;
  LIST_ENTRY              *RegionToFlushLink;
  BOOTMON_FS_FILE_REGION  *Region;

  FileSize = 0;

  // Look at all Flash areas to determine file size
  for (Index = 0; Index < HW_IMAGE_DESCRIPTION_REGION_MAX; Index++) {
    FileSize += File->HwDescription.Region[Index].Size;
  }

  // Add the regions that have not been flushed yet
  for (RegionToFlushLink = GetFirstNode (&File->RegionToFlushLink);
       !IsNull (&File->RegionToFlushLink, RegionToFlushLink);
       RegionToFlushLink = GetNextNode (&File->RegionToFlushLink, RegionToFlushLink)
       )
  {
    Region = (BOOTMON_FS_FILE_REGION*)RegionToFlushLink;
    if (Region->Offset + Region->Size > FileSize) {
      FileSize += Region->Offset + Region->Size;
    }
  }

  return FileSize;
}

UINTN
BootMonFsGetPhysicalSize (
  IN BOOTMON_FS_FILE* File
  )
{
  // Return 0 for files that haven't yet been flushed to media
  if (File->HwDescription.RegionCount == 0) {
    return 0;
  }

  return ((File->HwDescription.BlockEnd - File->HwDescription.BlockStart) + 1 )
          * File->Instance->Media->BlockSize;
}

EFIAPI
EFI_STATUS
BootMonFsSetDirPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  )
{
  BOOTMON_FS_FILE       *File;

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // UEFI Spec section 12.5:
  // "The seek request for nonzero is not valid on open directories."
  if (Position != 0) {
    return EFI_UNSUPPORTED;
  }
  File->Position = Position;

  return EFI_SUCCESS;
}

EFI_STATUS
BootMonFsOpenDirectory (
  OUT EFI_FILE_PROTOCOL **NewHandle,
  IN CHAR16             *FileName,
  IN BOOTMON_FS_INSTANCE *Volume
  )
{
  ASSERT(0);

  return EFI_UNSUPPORTED;
}
EFI_STATUS
GetFileSystemVolumeLabelInfo (
  IN BOOTMON_FS_INSTANCE *Instance,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  UINTN                         Size;
  EFI_FILE_SYSTEM_VOLUME_LABEL *Label;
  EFI_STATUS                    Status;

  Label = Buffer;

  // Value returned by StrSize includes null terminator.
  Size = SIZE_OF_EFI_FILE_SYSTEM_VOLUME_LABEL
         + StrSize (Instance->FsInfo.VolumeLabel);

  if (*BufferSize >= Size) {
    CopyMem (&Label->VolumeLabel, &Instance->FsInfo.VolumeLabel, Size);
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_BUFFER_TOO_SMALL;
  }
  *BufferSize = Size;
  return Status;
}

// Helper function that calculates a rough "free space" by:
// - Taking the media size
// - Subtracting the sum of all file sizes
// - Subtracting the block size times the number of files
//    (To account for the blocks containing the HW_IMAGE_INFO
STATIC
UINT64
ComputeFreeSpace (
  IN BOOTMON_FS_INSTANCE *Instance
  )
{
  LIST_ENTRY   *FileLink;
  UINT64        FileSizeSum;
  UINT64        MediaSize;
  UINTN         NumFiles;
  EFI_BLOCK_IO_MEDIA *Media;
  BOOTMON_FS_FILE *File;

  Media = Instance->BlockIo->Media;
  MediaSize = Media->BlockSize * (Media->LastBlock + 1);

  NumFiles = 0;
  FileSizeSum = 0;
  for (FileLink = GetFirstNode (&Instance->RootFile->Link);
         !IsNull (&Instance->RootFile->Link, FileLink);
         FileLink = GetNextNode (&Instance->RootFile->Link, FileLink)
         )
  {
    File = BOOTMON_FS_FILE_FROM_LINK_THIS (FileLink);
    FileSizeSum += BootMonFsGetImageLength (File);

    NumFiles++;
  }

  return MediaSize - (FileSizeSum + (Media->BlockSize + NumFiles));
}

EFI_STATUS
GetFilesystemInfo (
  IN BOOTMON_FS_INSTANCE *Instance,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  EFI_STATUS              Status;

  if (*BufferSize >= Instance->FsInfo.Size) {
    Instance->FsInfo.FreeSpace = ComputeFreeSpace (Instance);
    CopyMem (Buffer, &Instance->FsInfo, Instance->FsInfo.Size);
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_BUFFER_TOO_SMALL;
  }

  *BufferSize = Instance->FsInfo.Size;
  return Status;
}

EFI_STATUS
GetFileInfo (
  IN BOOTMON_FS_INSTANCE *Instance,
  IN BOOTMON_FS_FILE     *File,
  IN OUT UINTN           *BufferSize,
  OUT VOID               *Buffer
  )
{
  EFI_FILE_INFO   *Info;
  UINTN           ResultSize;
  UINTN           NameSize;
  UINTN           Index;

  if (File == Instance->RootFile) {
    NameSize = 0;
    ResultSize = SIZE_OF_EFI_FILE_INFO + sizeof (CHAR16);
  } else {
    NameSize   = AsciiStrLen (File->HwDescription.Footer.Filename) + 1;
    ResultSize = SIZE_OF_EFI_FILE_INFO + (NameSize * sizeof (CHAR16));
  }

  if (*BufferSize < ResultSize) {
    *BufferSize = ResultSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  Info = Buffer;

  // Zero out the structure
  ZeroMem (Info, ResultSize);

  // Fill in the structure
  Info->Size = ResultSize;

  if (File == Instance->RootFile) {
    Info->Attribute    = EFI_FILE_READ_ONLY | EFI_FILE_DIRECTORY;
    Info->FileName[0]  = L'\0';
  } else {
    Info->FileSize     = BootMonFsGetImageLength (File);
    Info->PhysicalSize = BootMonFsGetPhysicalSize (File);

    for (Index = 0; Index < NameSize; Index++) {
      Info->FileName[Index] = File->HwDescription.Footer.Filename[Index];
    }
  }

  *BufferSize = ResultSize;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SetFileName (
  IN  BOOTMON_FS_FILE *File,
  IN  CHAR16          *FileNameUnicode
  )
{
  CHAR8                 *FileNameAscii;
  UINT16                 SavedChar;
  UINTN                  FileNameSize;
  BOOTMON_FS_FILE       *SameFile;
  EFI_STATUS             Status;

  // EFI Shell inserts '\' in front of the filename that must be stripped
  if (FileNameUnicode[0] == L'\\') {
    FileNameUnicode++;
  }
  //
  // Convert Unicode into Ascii
  //
  SavedChar = L'\0';
  FileNameSize = StrLen (FileNameUnicode) + 1;
  FileNameAscii = AllocatePool (FileNameSize * sizeof (CHAR8));
  if (FileNameAscii == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  // If Unicode string is too long then truncate it.
  if (FileNameSize > MAX_NAME_LENGTH) {
    SavedChar = FileNameUnicode[MAX_NAME_LENGTH - 1];
    FileNameUnicode[MAX_NAME_LENGTH - 1] = L'\0';
  }
  UnicodeStrToAsciiStr (FileNameUnicode, FileNameAscii);
  // If the unicode string was truncated then restore its original content.
  if (SavedChar != L'\0') {
    FileNameUnicode[MAX_NAME_LENGTH - 1] = SavedChar;
  }

  // If we're changing the file name
  if (AsciiStrCmp (FileNameAscii, File->HwDescription.Footer.Filename)) {
    // Check a file with that filename doesn't already exist
    if (BootMonGetFileFromAsciiFileName (
          File->Instance,
          File->HwDescription.Footer.Filename,
          &SameFile) != EFI_NOT_FOUND) {
      Status = EFI_ACCESS_DENIED;
    } else {
      AsciiStrCpy (FileNameAscii, File->HwDescription.Footer.Filename);
      Status = EFI_SUCCESS;
    }
  } else {
    // No change to filename
    Status = EFI_SUCCESS;
  }

  FreePool (FileNameAscii);
  return Status;
}

// Set the file's size (NB "size", not "physical size"). If the change amounts
// to an increase, simply do a write followed by a flush.
// (This is a helper function for SetFileInfo.)
STATIC
EFI_STATUS
SetFileSize (
  IN BOOTMON_FS_INSTANCE *Instance,
  IN BOOTMON_FS_FILE     *BootMonFsFile,
  IN UINTN               Size
  )
{
  UINT64             StoredPosition;
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL *File;
  CHAR8              Buffer;
  UINTN              BufferSize;

  Buffer = 0;
  BufferSize = sizeof (Buffer);

  File = &BootMonFsFile->File;

  if (!(BootMonFsFile->OpenMode & EFI_FILE_MODE_WRITE)) {
    return EFI_ACCESS_DENIED;
  }

  if (Size <= BootMonFsFile->HwDescription.Region[0].Size) {
    BootMonFsFile->HwDescription.Region[0].Size = Size;
  } else {
    // Increasing a file's size is potentially complicated as it may require
    // moving the image description on media. The simplest way to do it is to
    // seek past the end of the file (which is valid in UEFI) and perform a
    // Write.

    // Save position
    Status = File->GetPosition (File, &StoredPosition);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = File->SetPosition (File, Size - 1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Status = File->Write (File, &BufferSize, &Buffer);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Restore saved position
    Status = File->SetPosition (File, Size - 1);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = File->Flush (File);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  return EFI_SUCCESS;
}

EFI_STATUS
SetFileInfo (
  IN BOOTMON_FS_INSTANCE *Instance,
  IN BOOTMON_FS_FILE     *File,
  IN UINTN                BufferSize,
  IN EFI_FILE_INFO       *Info
  )
{
  EFI_STATUS             Status;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;
  UINT8                 *DataBuffer;
  UINTN                  BlockSize;

  Status  = EFI_SUCCESS;
  BlockIo = Instance->BlockIo;

  // Note that a call to this function on a file opened read-only is only
  // invalid if it actually changes fields, so  we don't immediately fail if the
  // OpenMode is wrong.
  // Also note that the only fields supported are filename and size, others are
  // ignored.

  if (File != Instance->RootFile) {
    if (!(File->OpenMode & EFI_FILE_MODE_WRITE)) {
      return EFI_ACCESS_DENIED;
    }

    SetFileName (File, Info->FileName);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Update file size
    Status = SetFileSize (Instance, File, Info->FileSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Update the last block
    //
    BlockSize = BlockIo->Media->BlockSize;
    DataBuffer = AllocatePool (BlockSize);
    if (DataBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Status = BlockIo->ReadBlocks (BlockIo, Instance->Media->MediaId,
        File->HwDescription.BlockEnd, BlockSize, DataBuffer);
    if (EFI_ERROR (Status)) {
      FreePool (DataBuffer);
      return Status;
    }
    CopyMem (DataBuffer + BlockSize - sizeof (File->HwDescription), &File->HwDescription, sizeof (File->HwDescription));
    Status = BlockIo->WriteBlocks (BlockIo, Instance->Media->MediaId,
            File->HwDescription.BlockEnd, BlockSize, DataBuffer);
    FreePool (DataBuffer);
  }
  return Status;
}

EFIAPI
EFI_STATUS
BootMonFsGetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  EFI_STATUS           Status;
  BOOTMON_FS_FILE     *File;
  BOOTMON_FS_INSTANCE *Instance;

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (File == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Instance = File->Instance;

  // If the instance has not been initialized yet then do it ...
  if (!Instance->Initialized) {
    Status = BootMonFsInitialize (Instance);
  } else {
    Status = EFI_SUCCESS;
  }

  if (!EFI_ERROR (Status)) {
    if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)
        != 0) {
      Status = GetFileSystemVolumeLabelInfo (Instance, BufferSize, Buffer);
    } else if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid) != 0) {
      Status = GetFilesystemInfo (Instance, BufferSize, Buffer);
    } else if (CompareGuid (InformationType, &gEfiFileInfoGuid) != 0) {
      Status = GetFileInfo (Instance, File, BufferSize, Buffer);
    } else {
      Status = EFI_UNSUPPORTED;
    }
  }

  return Status;
}

EFIAPI
EFI_STATUS
BootMonFsSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  )
{
  EFI_STATUS           Status;
  BOOTMON_FS_FILE     *File;
  BOOTMON_FS_INSTANCE *Instance;

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (File == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Instance = File->Instance;

  if (CompareGuid (InformationType, &gEfiFileInfoGuid) != 0) {
    Status = SetFileInfo (Instance, File, BufferSize, (EFI_FILE_INFO *) Buffer);
  } else {
    // The only writable field in the other two information types
    // (i.e. EFI_FILE_SYSTEM_INFO and EFI_FILE_SYSTEM_VOLUME_LABEL) is the
    // filesystem volume label. This can be retrieved with GetInfo, but it is
    // hard-coded into this driver, not stored on media.
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

EFIAPI
EFI_STATUS
BootMonFsReadDirectory (
  IN EFI_FILE_PROTOCOL    *This,
  IN OUT UINTN            *BufferSize,
  OUT VOID                *Buffer
  )
{
  BOOTMON_FS_INSTANCE *Instance;
  BOOTMON_FS_FILE     *RootFile;
  BOOTMON_FS_FILE     *File;
  EFI_FILE_INFO       *Info;
  UINTN               NameSize;
  UINTN               ResultSize;
  EFI_STATUS          Status;
  UINTN               Index;

  RootFile = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (RootFile == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = RootFile->Instance;
  Status = BootMonGetFileFromPosition (Instance, RootFile->Position, &File);
  if (EFI_ERROR (Status)) {
    // No more file
    *BufferSize = 0;
    return EFI_SUCCESS;
  }

  NameSize   = AsciiStrLen (File->HwDescription.Footer.Filename) + 1;
  ResultSize = SIZE_OF_EFI_FILE_INFO + (NameSize * sizeof (CHAR16));
  if (*BufferSize < ResultSize) {
    *BufferSize = ResultSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  // Zero out the structure
  Info = Buffer;
  ZeroMem (Info, ResultSize);

  // Fill in the structure
  Info->Size         = ResultSize;
  Info->FileSize     = BootMonFsGetImageLength (File);
  Info->PhysicalSize = BootMonFsGetPhysicalSize (File);
  for (Index = 0; Index < NameSize; Index++) {
    Info->FileName[Index] = File->HwDescription.Footer.Filename[Index];
  }

  *BufferSize = ResultSize;
  RootFile->Position++;

  return EFI_SUCCESS;
}

EFIAPI
EFI_STATUS
BootMonFsFlushDirectory (
  IN EFI_FILE_PROTOCOL  *This
  )
{
  BOOTMON_FS_FILE *RootFile;
  LIST_ENTRY      *ListFiles;
  LIST_ENTRY      *Link;
  BOOTMON_FS_FILE *File;

  RootFile = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (RootFile == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ListFiles = &RootFile->Link;

  if (IsListEmpty (ListFiles)) {
    return EFI_SUCCESS;
  }

  //
  // Flush all the files that need to be flushed
  //

  // Go through all the list of files to flush them
  for (Link = GetFirstNode (ListFiles);
       !IsNull (ListFiles, Link);
       Link = GetNextNode (ListFiles, Link)
       )
  {
    File = BOOTMON_FS_FILE_FROM_LINK_THIS (Link);
    File->File.Flush (&File->File);
  }

  return EFI_SUCCESS;
}
