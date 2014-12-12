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

  Instance->RootFile->Info->Attribute = EFI_FILE_READ_ONLY | EFI_FILE_DIRECTORY;

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

STATIC
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

STATIC
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

STATIC
EFI_STATUS
GetFileInfo (
  IN BOOTMON_FS_INSTANCE  *Instance,
  IN BOOTMON_FS_FILE      *File,
  IN OUT UINTN            *BufferSize,
  OUT VOID                *Buffer
  )
{
  EFI_FILE_INFO  *Info;
  UINTN          ResultSize;

  ResultSize = SIZE_OF_EFI_FILE_INFO + StrSize (File->Info->FileName);

  if (*BufferSize < ResultSize) {
    *BufferSize = ResultSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  Info = Buffer;

  CopyMem (Info, File->Info, ResultSize);
  // Size of the information
  Info->Size = ResultSize;

  *BufferSize = ResultSize;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetBootMonFsFileInfo (
  IN BOOTMON_FS_INSTANCE *Instance,
  IN BOOTMON_FS_FILE     *File,
  IN OUT UINTN           *BufferSize,
  OUT VOID               *Buffer
  )
{
  EFI_STATUS             Status;
  BOOTMON_FS_FILE_INFO   *Info;
  UINTN                  ResultSize;
  UINTN                  Index;

  if (File == Instance->RootFile) {
    Status = EFI_UNSUPPORTED;
  } else {
    ResultSize = SIZE_OF_BOOTMON_FS_FILE_INFO;

    if (*BufferSize < ResultSize) {
      *BufferSize = ResultSize;
      Status = EFI_BUFFER_TOO_SMALL;
    } else {
      Info = Buffer;

      // Zero out the structure
      ZeroMem (Info, ResultSize);

      // Fill in the structure
      Info->Size = ResultSize;

      Info->EntryPoint  = File->HwDescription.EntryPoint;
      Info->RegionCount = File->HwDescription.RegionCount;
      for (Index = 0; Index < File->HwDescription.RegionCount; Index++) {
        Info->Region[Index].LoadAddress = File->HwDescription.Region[Index].LoadAddress;
        Info->Region[Index].Size        = File->HwDescription.Region[Index].Size;
        Info->Region[Index].Offset      = File->HwDescription.Region[Index].Offset;
        Info->Region[Index].Checksum    = File->HwDescription.Region[Index].Checksum;
      }
      *BufferSize = ResultSize;
      Status = EFI_SUCCESS;
    }
  }

  return Status;
}

/**
  Set the name of a file.

  This is a helper function for SetFileInfo().

  @param[in]  Instance  A pointer to the description of the volume
                        the file belongs to.
  @param[in]  File      A pointer to the description of the file.
  @param[in]  FileName  A pointer to the new name of the file.

  @retval  EFI_SUCCESS        The name was set.
  @retval  EFI_ACCESS_DENIED  An attempt is made to change the name of a file
                              to a file that is already present.

**/
STATIC
EFI_STATUS
SetFileName (
  IN  BOOTMON_FS_INSTANCE  *Instance,
  IN  BOOTMON_FS_FILE      *File,
  IN  CONST CHAR16         *FileName
  )
{
  CHAR16           TruncFileName[MAX_NAME_LENGTH];
  CHAR8            AsciiFileName[MAX_NAME_LENGTH];
  BOOTMON_FS_FILE  *SameFile;

  // If the file path start with a \ strip it. The EFI Shell may
  // insert a \ in front of the file name.
  if (FileName[0] == L'\\') {
    FileName++;
  }

  StrnCpy (TruncFileName, FileName, MAX_NAME_LENGTH - 1);
  TruncFileName[MAX_NAME_LENGTH - 1] = 0;
  UnicodeStrToAsciiStr (TruncFileName, AsciiFileName);

  if (BootMonGetFileFromAsciiFileName (
        File->Instance,
        AsciiFileName,
        &SameFile
        ) != EFI_NOT_FOUND) {
    // A file with that name already exists.
    return EFI_ACCESS_DENIED;
  } else {
    // OK, change the filename.
    AsciiStrToUnicodeStr (AsciiFileName, File->Info->FileName);
    return EFI_SUCCESS;
  }
}

/**
  Set the size of a file.

  This is a helper function for SetFileInfo().

  @param[in]  Instance  A pointer to the description of the volume
                        the file belongs to.
  @param[in]  File      A pointer to the description of the file.
  @param[in]  NewSize   The requested new size for the file.

  @retval  EFI_SUCCESS           The size was set.
  @retval  EFI_OUT_OF_RESOURCES  An allocation needed to process the request failed.

**/
STATIC
EFI_STATUS
SetFileSize (
  IN BOOTMON_FS_INSTANCE  *Instance,
  IN BOOTMON_FS_FILE      *BootMonFsFile,
  IN UINTN                 NewSize
  )
{
  EFI_STATUS              Status;
  UINT32                  OldSize;
  LIST_ENTRY              *RegionToFlushLink;
  LIST_ENTRY              *NextRegionToFlushLink;
  BOOTMON_FS_FILE_REGION  *Region;
  EFI_FILE_PROTOCOL       *File;
  CHAR8                   *Buffer;
  UINTN                   BufferSize;
  UINT64                  StoredPosition;

  OldSize = BootMonFsFile->Info->FileSize;

  //
  // In case of file truncation, force the regions waiting for writing to
  // not overflow the new size of the file.
  //
  if (NewSize < OldSize) {
    for (RegionToFlushLink = GetFirstNode (&BootMonFsFile->RegionToFlushLink);
         !IsNull (&BootMonFsFile->RegionToFlushLink, RegionToFlushLink);
         )
    {
      NextRegionToFlushLink = GetNextNode (&BootMonFsFile->RegionToFlushLink, RegionToFlushLink);
      Region = (BOOTMON_FS_FILE_REGION*)RegionToFlushLink;
      if (Region->Offset > NewSize) {
        RemoveEntryList (RegionToFlushLink);
        FreePool (Region->Buffer);
        FreePool (Region);
      } else {
        Region->Size = MIN (Region->Size, NewSize - Region->Offset);
      }
      RegionToFlushLink = NextRegionToFlushLink;
    }

  } else if (NewSize > OldSize) {
    // Increasing a file's size is potentially complicated as it may require
    // moving the image description on media. The simplest way to do it is to
    // seek past the end of the file (which is valid in UEFI) and perform a
    // Write.
    File = &BootMonFsFile->File;

    // Save position
    Status = File->GetPosition (File, &StoredPosition);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    // Set position at the end of the file
    Status = File->SetPosition (File, OldSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    BufferSize = NewSize - OldSize;
    Buffer = AllocateZeroPool (BufferSize);
    if (Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = File->Write (File, &BufferSize, Buffer);
    FreePool (Buffer);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Restore saved position
    Status = File->SetPosition (File, StoredPosition);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  BootMonFsFile->Info->FileSize = NewSize;

  return EFI_SUCCESS;
}

/**
  Set information about a file.

  @param[in]  Instance  A pointer to the description of the volume
                        the file belongs to.
  @param[in]  File      A pointer to the description of the file.
  @param[in]  Info      A pointer to the file information to write.

  @retval  EFI_SUCCESS           The information was set.
  @retval  EFI_ACCESS_DENIED     An attempt is being made to change the
                                 EFI_FILE_DIRECTORY Attribute.
  @retval  EFI_ACCESS_DENIED     The file was opened in read-only mode and an
                                 attempt is being made to modify a field other
                                 than Attribute.
  @retval  EFI_ACCESS_DENIED     An attempt is made to change the name of a file
                                 to a file that is already present.
  @retval  EFI_WRITE_PROTECTED   An attempt is being made to modify a read-only
                                 attribute.
  @retval  EFI_OUT_OF_RESOURCES  An allocation needed to process the request
                                 failed.

**/
STATIC
EFI_STATUS
SetFileInfo (
  IN BOOTMON_FS_INSTANCE  *Instance,
  IN BOOTMON_FS_FILE      *File,
  IN EFI_FILE_INFO        *Info
  )
{
  EFI_STATUS  Status;
  BOOLEAN     FileSizeIsDifferent;
  BOOLEAN     FileNameIsDifferent;
  BOOLEAN     TimeIsDifferent;

  //
  // A directory can not be changed to a file and a file can
  // not be changed to a directory.
  //
  if ((Info->Attribute & EFI_FILE_DIRECTORY)      !=
      (File->Info->Attribute & EFI_FILE_DIRECTORY)  ) {
    return EFI_ACCESS_DENIED;
  }

  FileSizeIsDifferent = (Info->FileSize != File->Info->FileSize);
  FileNameIsDifferent = (StrnCmp (
                           Info->FileName,
                           File->Info->FileName,
                           MAX_NAME_LENGTH - 1
                           ) != 0);
  //
  // Check if the CreateTime, LastAccess or ModificationTime
  // have been changed. The file system does not support file
  // timestamps thus the three times in "File->Info" are
  // always equal to zero. The following comparison actually
  // checks if all three times are still equal to 0 or not.
  //
  TimeIsDifferent = CompareMem (
                      &Info->CreateTime,
                      &File->Info->CreateTime,
                      3 * sizeof (EFI_TIME)
                      ) != 0;

  //
  // For a file opened in read-only mode, only the Attribute field can be
  // modified. The root directory open mode is forced to read-only at opening
  // thus the following test protects the root directory to be somehow modified.
  //
  if (File->OpenMode == EFI_FILE_MODE_READ) {
    if (FileSizeIsDifferent || FileNameIsDifferent || TimeIsDifferent) {
      return EFI_ACCESS_DENIED;
    }
  }

  if (TimeIsDifferent) {
    return EFI_WRITE_PROTECTED;
  }

  if (FileSizeIsDifferent) {
    Status = SetFileSize (Instance, File, Info->FileSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Note down in RAM the Attribute field but we can not
  // ask to store it in flash for the time being.
  //
  File->Info->Attribute = Info->Attribute;

  if (FileNameIsDifferent) {
    Status = SetFileName (Instance, File, Info->FileName);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
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

  if ((This == NULL)                         ||
      (InformationType == NULL)              ||
      (BufferSize == NULL)                   ||
      ((Buffer == NULL) && (*BufferSize > 0))  ) {
    return EFI_INVALID_PARAMETER;
  }

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (File->Info == NULL) {
    return EFI_INVALID_PARAMETER;
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
    } else if (CompareGuid (InformationType, &gArmBootMonFsFileInfoGuid) != 0) {
      Status = GetBootMonFsFileInfo (Instance, File, BufferSize, Buffer);
    } else {
      Status = EFI_UNSUPPORTED;
    }
  }

  return Status;
}

/**
  Set information about a file or a volume.

  @param[in]  This             A pointer to the EFI_FILE_PROTOCOL instance that
                               is the file handle the information is for.
  @param[in]  InformationType  The type identifier for the information being set :
                               EFI_FILE_INFO_ID or EFI_FILE_SYSTEM_INFO_ID or
                               EFI_FILE_SYSTEM_VOLUME_LABEL_ID
  @param[in]  BufferSize       The size, in bytes, of Buffer.
  @param[in]  Buffer           A pointer to the data buffer to write. The type of the
                               data inside the buffer is indicated by InformationType.

  @retval  EFI_SUCCESS            The information was set.
  @retval  EFI_UNSUPPORTED        The InformationType is not known.
  @retval  EFI_DEVICE_ERROR       The last issued semi-hosting operation failed.
  @retval  EFI_ACCESS_DENIED      An attempt is made to change the name of a file
                                  to a file that is already present.
  @retval  EFI_ACCESS_DENIED      An attempt is being made to change the
                                  EFI_FILE_DIRECTORY Attribute.
  @retval  EFI_ACCESS_DENIED      InformationType is EFI_FILE_INFO_ID and
                                  the file was opened in read-only mode and an
                                  attempt is being made to modify a field other
                                  than Attribute.
  @retval  EFI_WRITE_PROTECTED    An attempt is being made to modify a read-only
                                  attribute.
  @retval  EFI_BAD_BUFFER_SIZE    The size of the buffer is lower than that indicated by
                                  the data inside the buffer.
  @retval  EFI_OUT_OF_RESOURCES   A allocation needed to process the request failed.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
EFIAPI
EFI_STATUS
BootMonFsSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  )
{
  BOOTMON_FS_FILE       *File;
  EFI_FILE_INFO         *Info;
  EFI_FILE_SYSTEM_INFO  *SystemInfo;

  if ((This == NULL)            ||
      (InformationType == NULL) ||
      (Buffer == NULL)             ) {
    return EFI_INVALID_PARAMETER;
  }

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (File->Info == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    Info = Buffer;
    if (Info->Size < (SIZE_OF_EFI_FILE_INFO + StrSize (Info->FileName))) {
      return EFI_INVALID_PARAMETER;
    }
    if (BufferSize < Info->Size) {
      return EFI_BAD_BUFFER_SIZE;
    }
    return (SetFileInfo (File->Instance, File, Info));
  }

  //
  // The only writable field in the other two information types
  // (i.e. EFI_FILE_SYSTEM_INFO and EFI_FILE_SYSTEM_VOLUME_LABEL) is the
  // filesystem volume label. This can be retrieved with GetInfo, but it is
  // hard-coded into this driver, not stored on media.
  //

  if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    SystemInfo = Buffer;
    if (SystemInfo->Size <
        (SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize (SystemInfo->VolumeLabel))) {
      return EFI_INVALID_PARAMETER;
    }
    if (BufferSize < SystemInfo->Size) {
      return EFI_BAD_BUFFER_SIZE;
    }
    return EFI_WRITE_PROTECTED;
  }

  if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
    return EFI_WRITE_PROTECTED;
  }

  return EFI_UNSUPPORTED;
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
