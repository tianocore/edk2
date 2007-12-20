/*++

Copyright (c) 2005 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Info.c

Abstract:

  Routines dealing with setting/getting file/volume info

Revision History

--*/

#include "Fat.h"

EFI_STATUS
FatGetVolumeInfo (
  IN FAT_VOLUME       *Volume,
  IN OUT UINTN        *BufferSize,
  OUT VOID            *Buffer
  );

EFI_STATUS
FatSetVolumeInfo (
  IN FAT_VOLUME       *Volume,
  IN OUT UINTN        BufferSize,
  OUT VOID            *Buffer
  );

EFI_STATUS
FatSetOrGetInfo (
  IN BOOLEAN    IsSet,
  IN EFI_FILE   *FHand,
  IN EFI_GUID   *Type,
  IN OUT UINTN  *BufferSize,
  IN OUT VOID   *Buffer
  );

EFI_STATUS
FatGetFileInfo (
  IN FAT_OFILE        *OFile,
  IN OUT UINTN        *BufferSize,
  OUT VOID            *Buffer
  )
/*++

Routine Description:

  Get the open file's info into Buffer.

Arguments:

  OFile                 - The open file.
  BufferSize            - Size of Buffer.
  Buffer                - Buffer containing file info.

Returns:

  EFI_SUCCESS           - Get the file info successfully.
  EFI_BUFFER_TOO_SMALL  - The buffer is too small.

--*/
{
  return FatGetDirEntInfo (OFile->Volume, OFile->DirEnt, BufferSize, Buffer);
}

EFI_STATUS
FatGetVolumeInfo (
  IN     FAT_VOLUME     *Volume,
  IN OUT UINTN          *BufferSize,
     OUT VOID           *Buffer
  )
/*++

Routine Description:

  Get the volume's info into Buffer.

Arguments:

  Volume                - FAT file system volume.
  BufferSize            - Size of Buffer.
  Buffer                - Buffer containing volume info.

Returns:

  EFI_SUCCESS           - Get the volume info successfully.
  EFI_BUFFER_TOO_SMALL  - The buffer is too small.

--*/
{
  UINTN                 Size;
  UINTN                 NameSize;
  UINTN                 ResultSize;
  CHAR16                Name[FAT_NAME_LEN + 1];
  EFI_STATUS            Status;
  EFI_FILE_SYSTEM_INFO  *Info;
  UINT8                 ClusterAlignment;

  Size              = SIZE_OF_EFI_FILE_SYSTEM_INFO;
  Status            = FatGetVolumeEntry (Volume, Name);
  NameSize          = StrSize (Name);
  ResultSize        = Size + NameSize;
  ClusterAlignment  = Volume->ClusterAlignment;

  //
  // If we don't have valid info, compute it now
  //
  FatComputeFreeInfo (Volume);

  Status = EFI_BUFFER_TOO_SMALL;
  if (*BufferSize >= ResultSize) {
    Status  = EFI_SUCCESS;

    Info    = Buffer;
    ZeroMem (Info, SIZE_OF_EFI_FILE_SYSTEM_INFO);

    Info->Size        = ResultSize;
    Info->ReadOnly    = Volume->ReadOnly;
    Info->BlockSize   = (UINT32) Volume->ClusterSize;
    Info->VolumeSize  = LShiftU64 (Volume->MaxCluster, ClusterAlignment);
    Info->FreeSpace   = LShiftU64 (
                          Volume->FatInfoSector.FreeInfo.ClusterCount,
                          ClusterAlignment
                          );
    CopyMem ((CHAR8 *) Buffer + Size, Name, NameSize);
  }

  *BufferSize = ResultSize;
  return Status;
}

EFI_STATUS
FatGetVolumeLabelInfo (
  IN FAT_VOLUME       *Volume,
  IN OUT UINTN        *BufferSize,
  OUT VOID            *Buffer
  )
/*++

Routine Description:

  Get the volume's label info into Buffer.

Arguments:

  Volume                - FAT file system volume.
  BufferSize            - Size of Buffer.
  Buffer                - Buffer containing volume's label info.

Returns:

  EFI_SUCCESS           - Get the volume's label info successfully.
  EFI_BUFFER_TOO_SMALL  - The buffer is too small.

--*/
{
  UINTN                             Size;
  UINTN                             NameSize;
  UINTN                             ResultSize;
  CHAR16                            Name[FAT_NAME_LEN + 1];
  EFI_STATUS                        Status;

  Size        = SIZE_OF_EFI_FILE_SYSTEM_VOLUME_LABEL_INFO;
  Status      = FatGetVolumeEntry (Volume, Name);
  NameSize    = StrSize (Name);
  ResultSize  = Size + NameSize;

  Status      = EFI_BUFFER_TOO_SMALL;
  if (*BufferSize >= ResultSize) {
    Status  = EFI_SUCCESS;
    CopyMem ((CHAR8 *) Buffer + Size, Name, NameSize);
  }

  *BufferSize = ResultSize;
  return Status;
}

EFI_STATUS
FatSetVolumeInfo (
  IN FAT_VOLUME       *Volume,
  IN UINTN            BufferSize,
  IN VOID             *Buffer
  )
/*++

Routine Description:

  Set the volume's info.

Arguments:

  Volume                - FAT file system volume.
  BufferSize            - Size of Buffer.
  Buffer                - Buffer containing the new volume info.

Returns:

  EFI_SUCCESS           - Set the volume info successfully.
  EFI_BAD_BUFFER_SIZE   - The buffer size is error.
  EFI_WRITE_PROTECTED   - The volume is read only.
  other                 - An error occurred when operation the disk.

--*/
{
  EFI_FILE_SYSTEM_INFO  *Info;

  Info = (EFI_FILE_SYSTEM_INFO *) Buffer;

  if (BufferSize < SIZE_OF_EFI_FILE_SYSTEM_INFO + 2 || Info->Size > BufferSize) {
    return EFI_BAD_BUFFER_SIZE;
  }

  return FatSetVolumeEntry (Volume, Info->VolumeLabel);
}

EFI_STATUS
FatSetVolumeLabelInfo (
  IN FAT_VOLUME       *Volume,
  IN UINTN            BufferSize,
  IN VOID             *Buffer
  )
/*++

Routine Description:

  Set the volume's label info

Arguments:

  Volume                - FAT file system volume.
  BufferSize            - Size of Buffer.
  Buffer                - Buffer containing the new volume label info.

Returns:

  EFI_SUCCESS           - Set the volume label info successfully.
  EFI_WRITE_PROTECTED   - The disk is write protected.
  EFI_BAD_BUFFER_SIZE   - The buffer size is error.
  other                 - An error occurred when operation the disk.

--*/
{
  EFI_FILE_SYSTEM_VOLUME_LABEL_INFO *Info;

  Info = (EFI_FILE_SYSTEM_VOLUME_LABEL_INFO *) Buffer;

  if (BufferSize < SIZE_OF_EFI_FILE_SYSTEM_VOLUME_LABEL_INFO + 2) {
    return EFI_BAD_BUFFER_SIZE;
  }

  return FatSetVolumeEntry (Volume, Info->VolumeLabel);
}

EFI_STATUS
FatSetFileInfo (
  IN FAT_VOLUME       *Volume,
  IN FAT_IFILE        *IFile,
  IN FAT_OFILE        *OFile,
  IN UINTN            BufferSize,
  IN VOID             *Buffer
  )
/*++

Routine Description:

  Set the file info.

Arguments:

  Volume                - FAT file system volume.
  IFile                 - The instance of the open file.
  OFile                 - The open file.
  BufferSize            - Size of Buffer.
  Buffer                - Buffer containing the new file info.

Returns:

  EFI_SUCCESS           - Set the file info successfully.
  EFI_ACCESS_DENIED     - It is the root directory
                          or the directory attribute bit can not change
                          or try to change a directory size
                          or something else.
  EFI_UNSUPPORTED       - The new file size is larger than 4GB.
  EFI_WRITE_PROTECTED   - The disk is write protected.
  EFI_BAD_BUFFER_SIZE   - The buffer size is error.
  EFI_INVALID_PARAMETER - The time info or attributes info is error.
  EFI_OUT_OF_RESOURCES  - Can not allocate new memory.
  EFI_VOLUME_CORRUPTED  - The volume is corrupted.
  other                 - An error occurred when operation the disk.

--*/
{
  EFI_STATUS    Status;
  EFI_FILE_INFO *NewInfo;
  FAT_OFILE     *DotOFile;
  FAT_OFILE     *Parent;
  CHAR16        NewFileName[EFI_PATH_STRING_LENGTH];
  EFI_TIME      ZeroTime;
  FAT_DIRENT    *DirEnt;
  FAT_DIRENT    *TempDirEnt;
  UINT8         NewAttribute;
  BOOLEAN       ReadOnly;

  ZeroMem (&ZeroTime, sizeof (EFI_TIME));
  Parent  = OFile->Parent;
  DirEnt  = OFile->DirEnt;
  //
  // If this is the root directory, we can't make any updates
  //
  if (Parent == NULL) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Make sure there's a valid input buffer
  //
  NewInfo = Buffer;
  if (BufferSize < SIZE_OF_EFI_FILE_INFO + 2 || NewInfo->Size > BufferSize) {
    return EFI_BAD_BUFFER_SIZE;
  }

  ReadOnly = (BOOLEAN)(IFile->ReadOnly || (DirEnt->Entry.Attributes & EFI_FILE_READ_ONLY));
  //
  // if a zero time is specified, then the original time is preserved
  //
  if (CompareMem (&ZeroTime, &NewInfo->CreateTime, sizeof (EFI_TIME)) != 0) {
    if (!FatIsValidTime (&NewInfo->CreateTime)) {
      return EFI_INVALID_PARAMETER;
    }

    if (!ReadOnly) {
      FatEfiTimeToFatTime (&NewInfo->CreateTime, &DirEnt->Entry.FileCreateTime);
    }
  }

  if (CompareMem (&ZeroTime, &NewInfo->ModificationTime, sizeof (EFI_TIME)) != 0) {
    if (!FatIsValidTime (&NewInfo->ModificationTime)) {
      return EFI_INVALID_PARAMETER;
    }

    if (!ReadOnly) {
      FatEfiTimeToFatTime (&NewInfo->ModificationTime, &DirEnt->Entry.FileModificationTime);
    }

    OFile->PreserveLastModification = TRUE;
  }

  if (NewInfo->Attribute & (~EFI_FILE_VALID_ATTR)) {
    return EFI_INVALID_PARAMETER;
  }

  NewAttribute = (UINT8) NewInfo->Attribute;
  //
  // Can not change the directory attribute bit
  //
  if ((NewAttribute ^ DirEnt->Entry.Attributes) & EFI_FILE_DIRECTORY) {
    return EFI_ACCESS_DENIED;
  }
  //
  // Set the current attributes even if the IFile->ReadOnly is TRUE
  //
  DirEnt->Entry.Attributes = (UINT8) ((DirEnt->Entry.Attributes &~EFI_FILE_VALID_ATTR) | NewAttribute);
  //
  // Open the filename and see if it refers to an existing file
  //
  Status = FatLocateOFile (&Parent, NewInfo->FileName, DirEnt->Entry.Attributes, NewFileName);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (*NewFileName != 0) {
    //
    // File was not found.  We do not allow rename of the current directory if
    // there are open files below the current directory
    //
    if (!IsListEmpty (&OFile->ChildHead) || Parent == OFile) {
      return EFI_ACCESS_DENIED;
    }

    if (ReadOnly) {
      return EFI_ACCESS_DENIED;
    }

    Status = FatRemoveDirEnt (OFile->Parent, DirEnt);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Create new dirent
    //
    Status = FatCreateDirEnt (Parent, NewFileName, DirEnt->Entry.Attributes, &TempDirEnt);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    FatCloneDirEnt (TempDirEnt, DirEnt);
    FatFreeDirEnt (DirEnt);
    DirEnt        = TempDirEnt;
    DirEnt->OFile = OFile;
    OFile->DirEnt = DirEnt;
    OFile->Parent = Parent;
    RemoveEntryList (&OFile->ChildLink);
    InsertHeadList (&Parent->ChildHead, &OFile->ChildLink);
    //
    // If this is a directory, synchronize its dot directory entry
    //
    if (OFile->ODir != NULL) {
      //
      // Syncronize its dot entry
      //
      FatResetODirCursor (OFile);
      ASSERT (OFile->Parent != NULL);
      for (DotOFile = OFile; DotOFile != OFile->Parent->Parent; DotOFile = DotOFile->Parent) {
        Status = FatGetNextDirEnt (OFile, &DirEnt);
        if (EFI_ERROR (Status) || DirEnt == NULL || !FatIsDotDirEnt (DirEnt)) {
          return EFI_VOLUME_CORRUPTED;
        }

        FatCloneDirEnt (DirEnt, DotOFile->DirEnt);
        Status = FatStoreDirEnt (OFile, DirEnt);
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }
    }
    //
    // If the file is renamed, we should append the ARCHIVE attribute
    //
    OFile->Archive = TRUE;
  } else if (Parent != OFile) {
    //
    // filename is to a different filename that already exists
    //
    return EFI_ACCESS_DENIED;
  }
  //
  // If the file size has changed, apply it
  //
  if (NewInfo->FileSize != OFile->FileSize) {
    if (OFile->ODir != NULL || ReadOnly) {
      //
      // If this is a directory or the file is read only, we can't change the file size
      //
      return EFI_ACCESS_DENIED;
    }

    if (NewInfo->FileSize > OFile->FileSize) {
      Status = FatExpandOFile (OFile, NewInfo->FileSize);
    } else {
      Status = FatTruncateOFile (OFile, (UINTN) NewInfo->FileSize);
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    FatUpdateDirEntClusterSizeInfo (OFile);
  }

  OFile->Dirty = TRUE;
  return FatOFileFlush (OFile);
}

EFI_STATUS
FatSetOrGetInfo (
  IN     BOOLEAN        IsSet,
  IN     EFI_FILE       *FHand,
  IN     EFI_GUID       *Type,
  IN OUT UINTN          *BufferSize,
  IN OUT VOID           *Buffer
  )
/*++

Routine Description:

  Set or Get the some types info of the file into Buffer

Arguments:

  IsSet      - TRUE:The access is set, else is get
  FHand      - The handle of file
  Type       - The type of the info
  BufferSize - Size of Buffer
  Buffer     - Buffer containing volume info

Returns:

  EFI_SUCCESS       - Get the info successfully
  EFI_DEVICE_ERROR  - Can not find the OFile for the file

--*/
{
  FAT_IFILE   *IFile;
  FAT_OFILE   *OFile;
  FAT_VOLUME  *Volume;
  EFI_STATUS  Status;

  IFile   = IFILE_FROM_FHAND (FHand);
  OFile   = IFile->OFile;
  Volume  = OFile->Volume;

  Status  = OFile->Error;
  if (Status == EFI_NOT_FOUND) {
    return EFI_DEVICE_ERROR;
  }

  FatAcquireLock ();

  //
  // Verify the file handle isn't in an error state
  //
  if (!EFI_ERROR (Status)) {
    //
    // Get the proper information based on the request
    //
    Status = EFI_UNSUPPORTED;
    if (IsSet) {
      if (Volume->ReadOnly) {
        Status = EFI_WRITE_PROTECTED;
      } else {
        if (CompareGuid (Type, &gEfiFileInfoGuid)) {
          Status = FatSetFileInfo (Volume, IFile, OFile, *BufferSize, Buffer);
        }

        if (CompareGuid (Type, &gEfiFileSystemInfoGuid)) {
          Status = FatSetVolumeInfo (Volume, *BufferSize, Buffer);
        }

        if (CompareGuid (Type, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
          Status = FatSetVolumeLabelInfo (Volume, *BufferSize, Buffer);
        }
      }
    } else {
      if (CompareGuid (Type, &gEfiFileInfoGuid)) {
        Status = FatGetFileInfo (OFile, BufferSize, Buffer);
      }

      if (CompareGuid (Type, &gEfiFileSystemInfoGuid)) {
        Status = FatGetVolumeInfo (Volume, BufferSize, Buffer);
      }

      if (CompareGuid (Type, &gEfiFileSystemVolumeLabelInfoIdGuid)) {
        Status = FatGetVolumeLabelInfo (Volume, BufferSize, Buffer);
      }
    }
  }

  Status = FatCleanupVolume (Volume, NULL, Status);

  FatReleaseLock ();
  return Status;
}

EFI_STATUS
EFIAPI
FatGetInfo (
  IN     EFI_FILE       *FHand,
  IN     EFI_GUID       *Type,
  IN OUT UINTN          *BufferSize,
     OUT VOID           *Buffer
  )
/*++

Routine Description:

  Get the some types info of the file into Buffer.

Arguments:

  FHand                 - The handle of file.
  Type                  - The type of the info.
  BufferSize            - Size of Buffer.
  Buffer                - Buffer containing volume info.

Returns:

  EFI_SUCCESS           - Get the info successfully.
  EFI_DEVICE_ERROR      - Can not find the OFile for the file.

--*/
{
  return FatSetOrGetInfo (FALSE, FHand, Type, BufferSize, Buffer);
}

EFI_STATUS
EFIAPI
FatSetInfo (
  IN EFI_FILE  *FHand,
  IN EFI_GUID  *Type,
  IN UINTN     BufferSize,
  IN VOID      *Buffer
  )
/*++

Routine Description:

  Set the some types info of the file into Buffer.

Arguments:

  FHand                 - The handle of file.
  Type                  - The type of the info.
  BufferSize            - Size of Buffer
  Buffer                - Buffer containing volume info.

Returns:

  EFI_SUCCESS           - Set the info successfully.
  EFI_DEVICE_ERROR      - Can not find the OFile for the file.

--*/
{
  return FatSetOrGetInfo (TRUE, FHand, Type, &BufferSize, Buffer);
}
