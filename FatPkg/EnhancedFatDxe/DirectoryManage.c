/** @file
  Functions for performing directory entry io.

Copyright (c) 2005 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Fat.h"

/**

  Get a directory entry from disk for the Ofile.

  @param  Parent                - The parent of the OFile which need to update.
  @param  IoMode                - Indicate whether to read directory entry or write directory entry.
  @param  EntryPos              - The position of the directory entry to be accessed.
  @param  Entry                 - The directory entry read or written.

  @retval EFI_SUCCESS           - Access the directory entry successfully.
  @return other                 - An error occurred when reading the directory entry.

**/
STATIC
EFI_STATUS
FatAccessEntry (
  IN     FAT_OFILE  *Parent,
  IN     IO_MODE    IoMode,
  IN     UINTN      EntryPos,
  IN OUT VOID       *Entry
  )
{
  UINTN  Position;
  UINTN  BufferSize;

  Position = EntryPos * sizeof (FAT_DIRECTORY_ENTRY);
  if (Position >= Parent->FileSize) {
    //
    // End of directory
    //
    ASSERT (IoMode == ReadData);
    ((FAT_DIRECTORY_ENTRY *)Entry)->FileName[0] = EMPTY_ENTRY_MARK;
    ((FAT_DIRECTORY_ENTRY *)Entry)->Attributes  = 0;
    return EFI_SUCCESS;
  }

  BufferSize = sizeof (FAT_DIRECTORY_ENTRY);
  return FatAccessOFile (Parent, IoMode, Position, &BufferSize, Entry, NULL);
}

/**

  Save the directory entry to disk.

  @param  OFile                 - The parent OFile which needs to update.
  @param  DirEnt                - The directory entry to be saved.

  @retval EFI_SUCCESS           - Store the directory entry successfully.
  @return other                 - An error occurred when writing the directory entry.

**/
EFI_STATUS
FatStoreDirEnt (
  IN FAT_OFILE   *OFile,
  IN FAT_DIRENT  *DirEnt
  )
{
  EFI_STATUS         Status;
  FAT_DIRECTORY_LFN  LfnEntry;
  UINTN              EntryPos;
  CHAR16             *LfnBufferPointer;
  CHAR16             LfnBuffer[MAX_LFN_ENTRIES * LFN_CHAR_TOTAL + 1];
  UINT8              EntryCount;
  UINT8              LfnOrdinal;

  EntryPos   = DirEnt->EntryPos;
  EntryCount = DirEnt->EntryCount;
  //
  // Write directory entry
  //
  Status = FatAccessEntry (OFile, WriteData, EntryPos, &DirEnt->Entry);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (--EntryCount > 0) {
    //
    // Write LFN directory entry
    //
    SetMem (LfnBuffer, sizeof (CHAR16) * LFN_CHAR_TOTAL * EntryCount, 0xff);
    Status = StrCpyS (
               LfnBuffer,
               ARRAY_SIZE (LfnBuffer),
               DirEnt->FileString
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    LfnBufferPointer    = LfnBuffer;
    LfnEntry.Attributes = FAT_ATTRIBUTE_LFN;
    LfnEntry.Type       = 0;
    LfnEntry.MustBeZero = 0;
    LfnEntry.Checksum   = FatCheckSum (DirEnt->Entry.FileName);
    for (LfnOrdinal = 1; LfnOrdinal <= EntryCount; LfnOrdinal++) {
      LfnEntry.Ordinal = LfnOrdinal;
      if (LfnOrdinal == EntryCount) {
        LfnEntry.Ordinal |= FAT_LFN_LAST;
      }

      CopyMem (LfnEntry.Name1, LfnBufferPointer, sizeof (CHAR16) * LFN_CHAR1_LEN);
      LfnBufferPointer += LFN_CHAR1_LEN;
      CopyMem (LfnEntry.Name2, LfnBufferPointer, sizeof (CHAR16) * LFN_CHAR2_LEN);
      LfnBufferPointer += LFN_CHAR2_LEN;
      CopyMem (LfnEntry.Name3, LfnBufferPointer, sizeof (CHAR16) * LFN_CHAR3_LEN);
      LfnBufferPointer += LFN_CHAR3_LEN;
      EntryPos--;
      if (DirEnt->Invalid) {
        LfnEntry.Ordinal = DELETE_ENTRY_MARK;
      }

      Status = FatAccessEntry (OFile, WriteData, EntryPos, &LfnEntry);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  return EFI_SUCCESS;
}

/**

  Determine whether the directory entry is "." or ".." entry.

  @param  DirEnt               - The corresponding directory entry.

  @retval TRUE                 - The directory entry is "." or ".." directory entry
  @retval FALSE                - The directory entry is not "." or ".." directory entry

**/
BOOLEAN
FatIsDotDirEnt (
  IN FAT_DIRENT  *DirEnt
  )
{
  CHAR16  *FileString;

  FileString = DirEnt->FileString;
  if ((StrCmp (FileString, L".") == 0) || (StrCmp (FileString, L"..") == 0)) {
    return TRUE;
  }

  return FALSE;
}

/**

  Set the OFile's cluster info in its directory entry.

  @param  OFile                 - The corresponding OFile.

**/
STATIC
VOID
FatSetDirEntCluster (
  IN FAT_OFILE  *OFile
  )
{
  UINTN       Cluster;
  FAT_DIRENT  *DirEnt;

  DirEnt                        = OFile->DirEnt;
  Cluster                       = OFile->FileCluster;
  DirEnt->Entry.FileClusterHigh = (UINT16)(Cluster >> 16);
  DirEnt->Entry.FileCluster     = (UINT16)Cluster;
}

/**

  Set the OFile's cluster and size info in its directory entry.

  @param  OFile                 - The corresponding OFile.

**/
VOID
FatUpdateDirEntClusterSizeInfo (
  IN FAT_OFILE  *OFile
  )
{
  ASSERT (OFile->ODir == NULL);
  OFile->DirEnt->Entry.FileSize = (UINT32)OFile->FileSize;
  FatSetDirEntCluster (OFile);
}

/**

  Copy all the information of DirEnt2 to DirEnt1 except for 8.3 name.

  @param  DirEnt1               - The destination directory entry.
  @param  DirEnt2               - The source directory entry.

**/
VOID
FatCloneDirEnt (
  IN  FAT_DIRENT  *DirEnt1,
  IN  FAT_DIRENT  *DirEnt2
  )
{
  UINT8  *Entry1;
  UINT8  *Entry2;

  Entry1 = (UINT8 *)&DirEnt1->Entry;
  Entry2 = (UINT8 *)&DirEnt2->Entry;
  CopyMem (
    Entry1 + FAT_ENTRY_INFO_OFFSET,
    Entry2 + FAT_ENTRY_INFO_OFFSET,
    sizeof (FAT_DIRECTORY_ENTRY) - FAT_ENTRY_INFO_OFFSET
    );
}

/**

  Get the LFN for the directory entry.

  @param  Parent                - The parent directory.
  @param  DirEnt                - The directory entry to get LFN.

**/
STATIC
VOID
FatLoadLongNameEntry (
  IN FAT_OFILE   *Parent,
  IN FAT_DIRENT  *DirEnt
  )
{
  CHAR16             LfnBuffer[MAX_LFN_ENTRIES * LFN_CHAR_TOTAL + 1];
  CHAR16             *LfnBufferPointer;
  CHAR8              *File8Dot3Name;
  UINTN              EntryPos;
  UINT8              LfnOrdinal;
  UINT8              LfnChecksum;
  FAT_DIRECTORY_LFN  LfnEntry;
  EFI_STATUS         Status;

  EntryPos         = DirEnt->EntryPos;
  File8Dot3Name    = DirEnt->Entry.FileName;
  LfnBufferPointer = LfnBuffer;
  //
  // Computes checksum for LFN
  //
  LfnChecksum = FatCheckSum (File8Dot3Name);
  LfnOrdinal  = 1;
  do {
    if (EntryPos == 0) {
      LfnBufferPointer = LfnBuffer;
      break;
    }

    EntryPos--;
    Status = FatAccessEntry (Parent, ReadData, EntryPos, &LfnEntry);
    if (EFI_ERROR (Status) ||
        (LfnEntry.Attributes != FAT_ATTRIBUTE_LFN) ||
        (LfnEntry.MustBeZero != 0) ||
        (LfnEntry.Checksum != LfnChecksum) ||
        ((LfnEntry.Ordinal & (~FAT_LFN_LAST)) != LfnOrdinal) ||
        (LfnOrdinal > MAX_LFN_ENTRIES)
        )
    {
      //
      // The directory entry does not have a long file name or
      // some error occurs when loading long file name for a directory entry,
      // and then we load the long name from short name
      //
      LfnBufferPointer = LfnBuffer;
      break;
    }

    CopyMem (LfnBufferPointer, LfnEntry.Name1, sizeof (CHAR16) * LFN_CHAR1_LEN);
    LfnBufferPointer += LFN_CHAR1_LEN;
    CopyMem (LfnBufferPointer, LfnEntry.Name2, sizeof (CHAR16) * LFN_CHAR2_LEN);
    LfnBufferPointer += LFN_CHAR2_LEN;
    CopyMem (LfnBufferPointer, LfnEntry.Name3, sizeof (CHAR16) * LFN_CHAR3_LEN);
    LfnBufferPointer += LFN_CHAR3_LEN;
    LfnOrdinal++;
  } while ((LfnEntry.Ordinal & FAT_LFN_LAST) == 0);

  DirEnt->EntryCount = LfnOrdinal;
  //
  // Terminate current Lfnbuffer
  //
  *LfnBufferPointer = 0;
  if (LfnBufferPointer == LfnBuffer) {
    //
    // Fail to get the long file name from long file name entry,
    // get the file name from short name
    //
    FatGetFileNameViaCaseFlag (
      DirEnt,
      LfnBuffer,
      ARRAY_SIZE (LfnBuffer)
      );
  }

  DirEnt->FileString = AllocateCopyPool (StrSize (LfnBuffer), LfnBuffer);
}

/**

  Add this directory entry node to the list of directory entries and hash table.

  @param  ODir                  - The parent OFile which needs to be updated.
  @param  DirEnt                - The directory entry to be added.

**/
STATIC
VOID
FatAddDirEnt (
  IN FAT_ODIR    *ODir,
  IN FAT_DIRENT  *DirEnt
  )
{
  if (DirEnt->Link.BackLink == NULL) {
    DirEnt->Link.BackLink = &ODir->ChildList;
  }

  InsertTailList (DirEnt->Link.BackLink, &DirEnt->Link);
  FatInsertToHashTable (ODir, DirEnt);
}

/**

  Load from disk the next directory entry at current end of directory position.

  @param  OFile                 - The parent OFile.
  @param  PtrDirEnt             - The directory entry that is loaded.

  @retval EFI_SUCCESS           - Load the directory entry successfully.
  @retval EFI_OUT_OF_RESOURCES  - Out of resource.
  @return other                 - An error occurred when reading the directory entries.

**/
STATIC
EFI_STATUS
FatLoadNextDirEnt (
  IN  FAT_OFILE   *OFile,
  OUT FAT_DIRENT  **PtrDirEnt
  )
{
  EFI_STATUS           Status;
  FAT_DIRENT           *DirEnt;
  FAT_ODIR             *ODir;
  FAT_DIRECTORY_ENTRY  Entry;

  ODir = OFile->ODir;
  //
  // Make sure the parent's directory has been opened
  //
  ASSERT (ODir != NULL);
  //
  // Assert we have not reached the end of directory
  //
  ASSERT (!ODir->EndOfDir);
  DirEnt = NULL;

  for ( ; ;) {
    //
    // Read the next directory entry until we find a valid directory entry (excluding lfn entry)
    //
    Status = FatAccessEntry (OFile, ReadData, ODir->CurrentEndPos, &Entry);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (((UINT8)Entry.FileName[0] != DELETE_ENTRY_MARK) && ((Entry.Attributes & FAT_ATTRIBUTE_VOLUME_ID) == 0)) {
      //
      // We get a valid directory entry, then handle it
      //
      break;
    }

    ODir->CurrentEndPos++;
  }

  if (Entry.FileName[0] != EMPTY_ENTRY_MARK) {
    //
    // Although FAT spec states this field is always 0 for FAT12 & FAT16, some applications
    // might use it for some special usage, it is safer to zero it in memory for FAT12 & FAT16.
    //
    if (OFile->Volume->FatType != Fat32) {
      Entry.FileClusterHigh = 0;
    }

    //
    // This is a valid directory entry
    //
    DirEnt = AllocateZeroPool (sizeof (FAT_DIRENT));
    if (DirEnt == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    DirEnt->Signature = FAT_DIRENT_SIGNATURE;
    //
    // Remember the directory's entry position on disk
    //
    DirEnt->EntryPos = (UINT16)ODir->CurrentEndPos;
    CopyMem (&DirEnt->Entry, &Entry, sizeof (FAT_DIRECTORY_ENTRY));
    FatLoadLongNameEntry (OFile, DirEnt);
    if (DirEnt->FileString == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    //
    // Add this directory entry to directory
    //
    FatAddDirEnt (ODir, DirEnt);
    //
    // Point to next directory entry
    //
    ODir->CurrentEndPos++;
  } else {
    ODir->EndOfDir = TRUE;
  }

  *PtrDirEnt = DirEnt;
  return EFI_SUCCESS;

Done:
  FatFreeDirEnt (DirEnt);
  return Status;
}

/**

  Get the directory entry's info into Buffer.

  @param  Volume                - FAT file system volume.
  @param  DirEnt                - The corresponding directory entry.
  @param  BufferSize            - Size of Buffer.
  @param  Buffer                - Buffer containing file info.

  @retval EFI_SUCCESS           - Get the file info successfully.
  @retval EFI_BUFFER_TOO_SMALL  - The buffer is too small.

**/
EFI_STATUS
FatGetDirEntInfo (
  IN     FAT_VOLUME  *Volume,
  IN     FAT_DIRENT  *DirEnt,
  IN OUT UINTN       *BufferSize,
  OUT VOID           *Buffer
  )
{
  UINTN                Size;
  UINTN                NameSize;
  UINTN                ResultSize;
  UINTN                Cluster;
  EFI_STATUS           Status;
  EFI_FILE_INFO        *Info;
  FAT_DIRECTORY_ENTRY  *Entry;
  FAT_DATE_TIME        FatLastAccess;

  ASSERT_VOLUME_LOCKED (Volume);

  Size       = SIZE_OF_EFI_FILE_INFO;
  NameSize   = StrSize (DirEnt->FileString);
  ResultSize = Size + NameSize;

  Status = EFI_BUFFER_TOO_SMALL;
  if (*BufferSize >= ResultSize) {
    Status     = EFI_SUCCESS;
    Entry      = &DirEnt->Entry;
    Info       = Buffer;
    Info->Size = ResultSize;
    if ((Entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0) {
      Cluster            = (Entry->FileClusterHigh << 16) | Entry->FileCluster;
      Info->PhysicalSize = FatPhysicalDirSize (Volume, Cluster);
      Info->FileSize     = Info->PhysicalSize;
    } else {
      Info->FileSize     = Entry->FileSize;
      Info->PhysicalSize = FatPhysicalFileSize (Volume, Entry->FileSize);
    }

    ZeroMem (&FatLastAccess.Time, sizeof (FatLastAccess.Time));
    CopyMem (&FatLastAccess.Date, &Entry->FileLastAccess, sizeof (FatLastAccess.Date));
    FatFatTimeToEfiTime (&FatLastAccess, &Info->LastAccessTime);
    FatFatTimeToEfiTime (&Entry->FileCreateTime, &Info->CreateTime);
    FatFatTimeToEfiTime (&Entry->FileModificationTime, &Info->ModificationTime);
    Info->Attribute = Entry->Attributes & EFI_FILE_VALID_ATTR;
    CopyMem ((CHAR8 *)Buffer + Size, DirEnt->FileString, NameSize);
  }

  *BufferSize = ResultSize;
  return Status;
}

/**

  Search the directory for the directory entry whose filename is FileNameString.

  @param  OFile                 - The parent OFile whose directory is to be searched.
  @param  FileNameString        - The filename to be searched.
  @param  PtrDirEnt             - pointer to the directory entry if found.

  @retval EFI_SUCCESS           - Find the directory entry or not found.
  @return other                 - An error occurred when reading the directory entries.

**/
STATIC
EFI_STATUS
FatSearchODir (
  IN  FAT_OFILE   *OFile,
  IN  CHAR16      *FileNameString,
  OUT FAT_DIRENT  **PtrDirEnt
  )
{
  BOOLEAN     PossibleShortName;
  CHAR8       File8Dot3Name[FAT_NAME_LEN];
  FAT_ODIR    *ODir;
  FAT_DIRENT  *DirEnt;
  EFI_STATUS  Status;

  ODir = OFile->ODir;
  ASSERT (ODir != NULL);
  //
  // Check if the file name is a valid short name
  //
  PossibleShortName = FatCheckIs8Dot3Name (FileNameString, File8Dot3Name);
  //
  // Search the hash table first
  //
  DirEnt = *FatLongNameHashSearch (ODir, FileNameString);
  if ((DirEnt == NULL) && PossibleShortName) {
    DirEnt = *FatShortNameHashSearch (ODir, File8Dot3Name);
  }

  if (DirEnt == NULL) {
    //
    // We fail to get the directory entry from hash table; we then
    // search the rest directory
    //
    while (!ODir->EndOfDir) {
      Status = FatLoadNextDirEnt (OFile, &DirEnt);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if (DirEnt != NULL) {
        if (FatStriCmp (FileNameString, DirEnt->FileString) == 0) {
          break;
        }

        if (PossibleShortName && (CompareMem (File8Dot3Name, DirEnt->Entry.FileName, FAT_NAME_LEN) == 0)) {
          break;
        }
      }
    }
  }

  *PtrDirEnt = DirEnt;
  return EFI_SUCCESS;
}

/**

  Set the OFile's current directory cursor to the list head.

  @param OFile                 - The directory OFile whose directory cursor is reset.

**/
VOID
FatResetODirCursor (
  IN FAT_OFILE  *OFile
  )
{
  FAT_ODIR  *ODir;

  ODir = OFile->ODir;
  ASSERT (ODir != NULL);
  ODir->CurrentCursor = &(ODir->ChildList);
  ODir->CurrentPos    = 0;
}

/**

  Set the directory's cursor to the next and get the next directory entry.

  @param  OFile                 - The parent OFile.
  @param PtrDirEnt             - The next directory entry.

  @retval EFI_SUCCESS           - We get the next directory entry successfully.
  @return other                 - An error occurred when get next directory entry.

**/
EFI_STATUS
FatGetNextDirEnt (
  IN  FAT_OFILE   *OFile,
  OUT FAT_DIRENT  **PtrDirEnt
  )
{
  EFI_STATUS  Status;
  FAT_DIRENT  *DirEnt;
  FAT_ODIR    *ODir;

  ODir = OFile->ODir;
  ASSERT (ODir != NULL);
  if (ODir->CurrentCursor->ForwardLink == &ODir->ChildList) {
    //
    // End of directory, we will try one more time
    //
    if (!ODir->EndOfDir) {
      //
      // Read directory from disk
      //
      Status = FatLoadNextDirEnt (OFile, &DirEnt);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  if (ODir->CurrentCursor->ForwardLink == &ODir->ChildList) {
    //
    // End of directory, return NULL
    //
    DirEnt           = NULL;
    ODir->CurrentPos = ODir->CurrentEndPos;
  } else {
    ODir->CurrentCursor = ODir->CurrentCursor->ForwardLink;
    DirEnt              = DIRENT_FROM_LINK (ODir->CurrentCursor);
    ODir->CurrentPos    = DirEnt->EntryPos + 1;
  }

  *PtrDirEnt = DirEnt;
  return EFI_SUCCESS;
}

/**

  Set the directory entry count according to the filename.

  @param  OFile                 - The corresponding OFile.
  @param  DirEnt                - The directory entry to be set.

**/
STATIC
VOID
FatSetEntryCount (
  IN FAT_OFILE   *OFile,
  IN FAT_DIRENT  *DirEnt
  )
{
  CHAR16  *FileString;
  CHAR8   *File8Dot3Name;

  //
  // Get new entry count and set the 8.3 name
  //
  DirEnt->EntryCount = 1;
  FileString         = DirEnt->FileString;
  File8Dot3Name      = DirEnt->Entry.FileName;
  SetMem (File8Dot3Name, FAT_NAME_LEN, ' ');
  if (StrCmp (FileString, L".") == 0) {
    //
    // "." entry
    //
    File8Dot3Name[0] = '.';
    FatCloneDirEnt (DirEnt, OFile->DirEnt);
  } else if (StrCmp (FileString, L"..") == 0) {
    //
    // ".." entry
    //
    File8Dot3Name[0] = '.';
    File8Dot3Name[1] = '.';
    FatCloneDirEnt (DirEnt, OFile->Parent->DirEnt);
  } else {
    //
    // Normal name
    //
    if (FatCheckIs8Dot3Name (FileString, File8Dot3Name)) {
      //
      // This file name is a valid 8.3 file name, we need to further check its case flag
      //
      FatSetCaseFlag (DirEnt);
    } else {
      //
      // The file name is not a valid 8.3 name we need to generate an 8.3 name for it
      //
      FatCreate8Dot3Name (OFile, DirEnt);
      DirEnt->EntryCount = (UINT8)(LFN_ENTRY_NUMBER (StrLen (FileString)) + DirEnt->EntryCount);
    }
  }
}

/**

  Append a zero cluster to the current OFile.

  @param  OFile        - The directory OFile which needs to be updated.

  @retval EFI_SUCCESS  - Append a zero cluster to the OFile successfully.
  @return other        - An error occurred when appending the zero cluster.

**/
STATIC
EFI_STATUS
FatExpandODir (
  IN FAT_OFILE  *OFile
  )
{
  return FatExpandOFile (OFile, OFile->FileSize + OFile->Volume->ClusterSize);
}

/**

  Search the Root OFile for the possible volume label.

  @param  Root                  - The Root OFile.
  @param  DirEnt                - The returned directory entry of volume label.

  @retval EFI_SUCCESS           - The search process is completed successfully.
  @return other                 - An error occurred when searching volume label.

**/
STATIC
EFI_STATUS
FatSeekVolumeId (
  IN  FAT_OFILE   *Root,
  OUT FAT_DIRENT  *DirEnt
  )
{
  EFI_STATUS           Status;
  UINTN                EntryPos;
  FAT_DIRECTORY_ENTRY  *Entry;

  EntryPos        = 0;
  Entry           = &DirEnt->Entry;
  DirEnt->Invalid = TRUE;
  do {
    Status = FatAccessEntry (Root, ReadData, EntryPos, Entry);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (((UINT8)Entry->FileName[0] != DELETE_ENTRY_MARK) && (((Entry->Attributes) & (~FAT_ATTRIBUTE_ARCHIVE)) == FAT_ATTRIBUTE_VOLUME_ID)) {
      DirEnt->EntryPos   = (UINT16)EntryPos;
      DirEnt->EntryCount = 1;
      DirEnt->Invalid    = FALSE;
      break;
    }

    EntryPos++;
  } while (Entry->FileName[0] != EMPTY_ENTRY_MARK);

  return EFI_SUCCESS;
}

/**

  Use First Fit Algorithm to insert directory entry.
  Only this function will erase "E5" entries in a directory.
  In view of safest recovery, this function will only be triggered
  when maximum directory entry number has reached.

  @param  OFile                 - The corresponding OFile.
  @param  DirEnt                - The directory entry to be inserted.

  @retval EFI_SUCCESS           - The directory entry has been successfully inserted.
  @retval EFI_VOLUME_FULL       - The directory can not hold more directory entries.
  @return Others                - Some error occurred when inserting new directory entries.

**/
STATIC
EFI_STATUS
FatFirstFitInsertDirEnt (
  IN FAT_OFILE   *OFile,
  IN FAT_DIRENT  *DirEnt
  )
{
  EFI_STATUS  Status;
  FAT_ODIR    *ODir;
  LIST_ENTRY  *CurrentEntry;
  FAT_DIRENT  *CurrentDirEnt;
  UINT32      CurrentPos;
  UINT32      LabelPos;
  UINT32      NewEntryPos;
  UINT16      EntryCount;
  FAT_DIRENT  LabelDirEnt;

  LabelPos = 0;
  if (OFile->Parent == NULL) {
    Status = FatSeekVolumeId (OFile, &LabelDirEnt);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (!LabelDirEnt.Invalid) {
      LabelPos = LabelDirEnt.EntryPos;
    }
  }

  EntryCount  = DirEnt->EntryCount;
  NewEntryPos = EntryCount;
  CurrentPos  = 0;
  ODir        = OFile->ODir;
  for (CurrentEntry = ODir->ChildList.ForwardLink;
       CurrentEntry != &ODir->ChildList;
       CurrentEntry = CurrentEntry->ForwardLink
       )
  {
    CurrentDirEnt = DIRENT_FROM_LINK (CurrentEntry);
    if (NewEntryPos + CurrentDirEnt->EntryCount <= CurrentDirEnt->EntryPos) {
      if ((LabelPos > NewEntryPos) || (LabelPos <= CurrentPos)) {
        //
        // first fit succeeded
        //
        goto Done;
      }
    }

    CurrentPos  = CurrentDirEnt->EntryPos;
    NewEntryPos = CurrentPos + EntryCount;
  }

  if (NewEntryPos >= ODir->CurrentEndPos) {
    return EFI_VOLUME_FULL;
  }

Done:
  DirEnt->EntryPos      = (UINT16)NewEntryPos;
  DirEnt->Link.BackLink = CurrentEntry;
  return EFI_SUCCESS;
}

/**

  Find the new directory entry position for the directory entry.

  @param  OFile                 - The corresponding OFile.
  @param  DirEnt                - The directory entry whose new position is to be set.

  @retval EFI_SUCCESS           - The new directory entry position is successfully found.
  @retval EFI_VOLUME_FULL       - The directory has reach its maximum capacity.
  @return other                 - An error occurred when reading the directory entry.

**/
STATIC
EFI_STATUS
FatNewEntryPos (
  IN FAT_OFILE   *OFile,
  IN FAT_DIRENT  *DirEnt
  )
{
  EFI_STATUS  Status;
  FAT_ODIR    *ODir;
  FAT_DIRENT  *TempDirEnt;
  UINT32      NewEndPos;

  ODir = OFile->ODir;
  ASSERT (ODir != NULL);
  //
  // Make sure the whole directory has been loaded
  //
  while (!ODir->EndOfDir) {
    Status = FatLoadNextDirEnt (OFile, &TempDirEnt);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // We will append this entry to the end of directory
  //
  FatGetCurrentFatTime (&DirEnt->Entry.FileCreateTime);
  CopyMem (&DirEnt->Entry.FileModificationTime, &DirEnt->Entry.FileCreateTime, sizeof (FAT_DATE_TIME));
  CopyMem (&DirEnt->Entry.FileLastAccess, &DirEnt->Entry.FileCreateTime.Date, sizeof (FAT_DATE));
  NewEndPos = ODir->CurrentEndPos + DirEnt->EntryCount;
  if (NewEndPos * sizeof (FAT_DIRECTORY_ENTRY) > OFile->FileSize) {
    if (NewEndPos >= (OFile->IsFixedRootDir ? OFile->Volume->RootEntries : FAT_MAX_DIRENTRY_COUNT)) {
      //
      // We try to use fist fit algorithm to insert this directory entry
      //
      return FatFirstFitInsertDirEnt (OFile, DirEnt);
    }

    //
    // We should allocate a new cluster for this directory
    //
    Status = FatExpandODir (OFile);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // We append our directory entry at the end of directory file
  //
  ODir->CurrentEndPos = NewEndPos;
  DirEnt->EntryPos    = (UINT16)(ODir->CurrentEndPos - 1);
  return EFI_SUCCESS;
}

/**

  Get the directory entry for the volume.

  @param  Volume                - FAT file system volume.
  @param  Name                  - The file name of the volume.

  @retval EFI_SUCCESS           - Update the volume with the directory entry successfully.
  @return others                - An error occurred when getting volume label.

**/
EFI_STATUS
FatGetVolumeEntry (
  IN FAT_VOLUME  *Volume,
  IN CHAR16      *Name
  )
{
  EFI_STATUS  Status;
  FAT_DIRENT  LabelDirEnt;

  *Name  = 0;
  Status = FatSeekVolumeId (Volume->Root, &LabelDirEnt);
  if (!EFI_ERROR (Status)) {
    if (!LabelDirEnt.Invalid) {
      FatNameToStr (LabelDirEnt.Entry.FileName, FAT_NAME_LEN, FALSE, Name);
    }
  }

  return Status;
}

/**

  Set the relevant directory entry into disk for the volume.

  @param  Volume              - FAT file system volume.
  @param  Name                - The new file name of the volume.

  @retval EFI_SUCCESS         - Update the Volume successfully.
  @retval EFI_UNSUPPORTED     - The input label is not a valid volume label.
  @return other               - An error occurred when setting volume label.

**/
EFI_STATUS
FatSetVolumeEntry (
  IN FAT_VOLUME  *Volume,
  IN CHAR16      *Name
  )
{
  EFI_STATUS  Status;
  FAT_DIRENT  LabelDirEnt;
  FAT_OFILE   *Root;

  Root   = Volume->Root;
  Status = FatSeekVolumeId (Volume->Root, &LabelDirEnt);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (LabelDirEnt.Invalid) {
    //
    // If there is not the relevant directory entry, create a new one
    //
    ZeroMem (&LabelDirEnt, sizeof (FAT_DIRENT));
    LabelDirEnt.EntryCount = 1;
    Status                 = FatNewEntryPos (Root, &LabelDirEnt);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    LabelDirEnt.Entry.Attributes = FAT_ATTRIBUTE_VOLUME_ID;
  }

  SetMem (LabelDirEnt.Entry.FileName, FAT_NAME_LEN, ' ');
  if (FatStrToFat (Name, FAT_NAME_LEN, LabelDirEnt.Entry.FileName)) {
    return EFI_UNSUPPORTED;
  }

  FatGetCurrentFatTime (&LabelDirEnt.Entry.FileModificationTime);
  return FatStoreDirEnt (Root, &LabelDirEnt);
}

/**

  Create "." and ".." directory entries in the newly-created parent OFile.

  @param  OFile                 - The parent OFile.

  @retval EFI_SUCCESS           - The dot directory entries are successfully created.
  @return other                 - An error occurred when creating the directory entry.

**/
EFI_STATUS
FatCreateDotDirEnts (
  IN FAT_OFILE  *OFile
  )
{
  EFI_STATUS  Status;
  FAT_DIRENT  *DirEnt;

  Status = FatExpandODir (OFile);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FatSetDirEntCluster (OFile);
  //
  // Create "."
  //
  Status = FatCreateDirEnt (OFile, L".", FAT_ATTRIBUTE_DIRECTORY, &DirEnt);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create ".."
  //
  Status = FatCreateDirEnt (OFile, L"..", FAT_ATTRIBUTE_DIRECTORY, &DirEnt);
  return Status;
}

/**

  Create a directory entry in the parent OFile.

  @param  OFile                 - The parent OFile.
  @param  FileName              - The filename of the newly-created directory entry.
  @param  Attributes            - The attribute of the newly-created directory entry.
  @param  PtrDirEnt             - The pointer to the newly-created directory entry.

  @retval EFI_SUCCESS           - The directory entry is successfully created.
  @retval EFI_OUT_OF_RESOURCES  - Not enough memory to create the directory entry.
  @return other                 - An error occurred when creating the directory entry.

**/
EFI_STATUS
FatCreateDirEnt (
  IN  FAT_OFILE   *OFile,
  IN  CHAR16      *FileName,
  IN  UINT8       Attributes,
  OUT FAT_DIRENT  **PtrDirEnt
  )
{
  FAT_DIRENT  *DirEnt;
  FAT_ODIR    *ODir;
  EFI_STATUS  Status;

  ASSERT (OFile != NULL);
  ODir = OFile->ODir;
  ASSERT (ODir != NULL);
  DirEnt = AllocateZeroPool (sizeof (FAT_DIRENT));
  if (DirEnt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DirEnt->Signature  = FAT_DIRENT_SIGNATURE;
  DirEnt->FileString = AllocateCopyPool (StrSize (FileName), FileName);
  if (DirEnt->FileString == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Determine how many directory entries we need
  //
  FatSetEntryCount (OFile, DirEnt);
  //
  // Determine the file's directory entry position
  //
  Status = FatNewEntryPos (OFile, DirEnt);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  FatAddDirEnt (ODir, DirEnt);
  DirEnt->Entry.Attributes = Attributes;
  *PtrDirEnt               = DirEnt;
  DEBUG ((DEBUG_INFO, "FSOpen: Created new directory entry '%S'\n", DirEnt->FileString));
  return FatStoreDirEnt (OFile, DirEnt);

Done:
  FatFreeDirEnt (DirEnt);
  return Status;
}

/**

  Remove this directory entry node from the list of directory entries and hash table.

  @param  OFile                - The parent OFile.
  @param  DirEnt               - The directory entry to be removed.

  @retval EFI_SUCCESS          - The directory entry is successfully removed.
  @return other                - An error occurred when removing the directory entry.

**/
EFI_STATUS
FatRemoveDirEnt (
  IN FAT_OFILE   *OFile,
  IN FAT_DIRENT  *DirEnt
  )
{
  FAT_ODIR  *ODir;

  ODir = OFile->ODir;
  if (ODir->CurrentCursor == &DirEnt->Link) {
    //
    // Move the directory cursor to its previous directory entry
    //
    ODir->CurrentCursor = ODir->CurrentCursor->BackLink;
  }

  //
  // Remove from directory entry list
  //
  RemoveEntryList (&DirEnt->Link);
  //
  // Remove from hash table
  //
  FatDeleteFromHashTable (ODir, DirEnt);
  DirEnt->Entry.FileName[0] = DELETE_ENTRY_MARK;
  DirEnt->Invalid           = TRUE;
  return FatStoreDirEnt (OFile, DirEnt);
}

/**

  Open the directory entry to get the OFile.

  @param  Parent                - The parent OFile.
  @param  DirEnt                - The directory entry to be opened.

  @retval EFI_SUCCESS           - The directory entry is successfully opened.
  @retval EFI_OUT_OF_RESOURCES  - not enough memory to allocate a new OFile.
  @return other                 - An error occurred when opening the directory entry.

**/
EFI_STATUS
FatOpenDirEnt (
  IN FAT_OFILE   *Parent,
  IN FAT_DIRENT  *DirEnt
  )
{
  FAT_OFILE   *OFile;
  FAT_VOLUME  *Volume;

  if (DirEnt->OFile == NULL) {
    //
    // Open the directory entry
    //
    OFile = AllocateZeroPool (sizeof (FAT_OFILE));
    if (OFile == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    OFile->Signature = FAT_OFILE_SIGNATURE;
    InitializeListHead (&OFile->Opens);
    InitializeListHead (&OFile->ChildHead);
    OFile->Parent = Parent;
    OFile->DirEnt = DirEnt;
    if (Parent != NULL) {
      //
      // The newly created OFile is not root
      //
      Volume             = Parent->Volume;
      OFile->FullPathLen = Parent->FullPathLen + 1 + StrLen (DirEnt->FileString);
      OFile->FileCluster = ((DirEnt->Entry.FileClusterHigh) << 16) | (DirEnt->Entry.FileCluster);
      InsertTailList (&Parent->ChildHead, &OFile->ChildLink);
    } else {
      //
      // The newly created OFile is root
      //
      Volume             = VOLUME_FROM_ROOT_DIRENT (DirEnt);
      Volume->Root       = OFile;
      OFile->FileCluster = Volume->RootCluster;
      if (Volume->FatType  != Fat32) {
        OFile->IsFixedRootDir = TRUE;
      }
    }

    OFile->FileCurrentCluster = OFile->FileCluster;
    OFile->Volume             = Volume;
    InsertHeadList (&Volume->CheckRef, &OFile->CheckLink);

    OFile->FileSize = DirEnt->Entry.FileSize;
    if ((DirEnt->Entry.Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0) {
      if (OFile->IsFixedRootDir) {
        OFile->FileSize = Volume->RootEntries * sizeof (FAT_DIRECTORY_ENTRY);
      } else {
        OFile->FileSize = FatPhysicalDirSize (Volume, OFile->FileCluster);
      }

      FatRequestODir (OFile);
      if (OFile->ODir == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    }

    DirEnt->OFile = OFile;
  }

  return EFI_SUCCESS;
}

/**

  Close the directory entry and free the OFile.

  @param  DirEnt               - The directory entry to be closed.

**/
VOID
FatCloseDirEnt (
  IN FAT_DIRENT  *DirEnt
  )
{
  FAT_OFILE   *OFile;
  FAT_VOLUME  *Volume;

  OFile = DirEnt->OFile;
  ASSERT (OFile != NULL);
  Volume = OFile->Volume;

  if (OFile->ODir != NULL) {
    FatDiscardODir (OFile);
  }

  if (OFile->Parent == NULL) {
    Volume->Root = NULL;
  } else {
    RemoveEntryList (&OFile->ChildLink);
  }

  FreePool (OFile);
  DirEnt->OFile = NULL;
  if (DirEnt->Invalid == TRUE) {
    //
    // Free directory entry itself
    //
    FatFreeDirEnt (DirEnt);
  }
}

/**

  Traverse filename and open all OFiles that can be opened.
  Update filename pointer to the component that can't be opened.
  If more than one name component remains, returns an error;
  otherwise, return the remaining name component so that the caller might choose to create it.

  @param  PtrOFile              - As input, the reference OFile; as output, the located OFile.
  @param  FileName              - The file name relevant to the OFile.
  @param  Attributes            - The attribute of the destination OFile.
  @param  NewFileName           - The remaining file name.

  @retval EFI_NOT_FOUND         - The file name can't be opened and there is more than one
                          components within the name left (this means the name can
                          not be created either).
  @retval EFI_INVALID_PARAMETER - The parameter is not valid.
  @retval EFI_SUCCESS           - Open the file successfully.
  @return other                 - An error occurred when locating the OFile.

**/
EFI_STATUS
FatLocateOFile (
  IN OUT FAT_OFILE  **PtrOFile,
  IN     CHAR16     *FileName,
  IN     UINT8      Attributes,
  OUT CHAR16        *NewFileName
  )
{
  EFI_STATUS  Status;
  FAT_VOLUME  *Volume;
  CHAR16      ComponentName[EFI_PATH_STRING_LENGTH];
  UINTN       FileNameLen;
  BOOLEAN     DirIntended;
  CHAR16      *Next;
  FAT_OFILE   *OFile;
  FAT_DIRENT  *DirEnt;

  DirEnt = NULL;

  FileNameLen = StrLen (FileName);
  if (FileNameLen == 0) {
    return EFI_INVALID_PARAMETER;
  }

  OFile  = *PtrOFile;
  Volume = OFile->Volume;

  DirIntended = FALSE;
  if (FileName[FileNameLen - 1] == PATH_NAME_SEPARATOR) {
    DirIntended = TRUE;
  }

  //
  // If name starts with path name separator, then move to root OFile
  //
  if (*FileName == PATH_NAME_SEPARATOR) {
    OFile = Volume->Root;
    FileName++;
    FileNameLen--;
  }

  //
  // Per FAT Spec the file name should meet the following criteria:
  //   C1. Length (FileLongName) <= 255
  //   C2. Length (X:FileFullPath<NUL>) <= 260
  // Here we check C2 first.
  //
  if (2 + OFile->FullPathLen + 1 + FileNameLen + 1 > EFI_PATH_STRING_LENGTH) {
    //
    // Full path length can not surpass 256
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Start at current location
  //
  Next = FileName;
  for ( ; ;) {
    //
    // Get the next component name
    //
    FileName = Next;
    Next     = FatGetNextNameComponent (FileName, ComponentName);

    //
    // If end of the file name, we're done
    //
    if (ComponentName[0] == 0) {
      if (DirIntended && (OFile->ODir == NULL)) {
        return EFI_NOT_FOUND;
      }

      NewFileName[0] = 0;
      break;
    }

    //
    // If "dot", then current
    //
    if (StrCmp (ComponentName, L".") == 0) {
      continue;
    }

    //
    // If "dot dot", then parent
    //
    if (StrCmp (ComponentName, L"..") == 0) {
      if (OFile->Parent == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      OFile = OFile->Parent;
      continue;
    }

    if (!FatFileNameIsValid (ComponentName, NewFileName)) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // We have a component name, try to open it
    //
    if (OFile->ODir == NULL) {
      //
      // This file isn't a directory, can't open it
      //
      return EFI_NOT_FOUND;
    }

    //
    // Search the compName in the directory
    //
    Status = FatSearchODir (OFile, NewFileName, &DirEnt);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (DirEnt == NULL) {
      //
      // component name is not found in the directory
      //
      if (*Next != 0) {
        return EFI_NOT_FOUND;
      }

      if (DirIntended && ((Attributes & FAT_ATTRIBUTE_DIRECTORY) == 0)) {
        return EFI_INVALID_PARAMETER;
      }

      //
      // It's the last component name - return with the open
      // path and the remaining name
      //
      break;
    }

    Status = FatOpenDirEnt (OFile, DirEnt);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    OFile = DirEnt->OFile;
  }

  *PtrOFile = OFile;
  return EFI_SUCCESS;
}
