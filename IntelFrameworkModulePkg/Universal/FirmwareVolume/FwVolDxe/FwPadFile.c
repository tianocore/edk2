/** @file
  Implements functions to pad firmware file.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FwVolDriver.h"

/**
  Calculate the checksum for a PAD file.

  @param PadFileHeader   The Pad File to be caculeted the checksum.

**/
VOID
SetPadFileChecksum (
  IN EFI_FFS_FILE_HEADER *PadFileHeader
  )
{
  if ((PadFileHeader->Attributes & FFS_ATTRIB_CHECKSUM) != 0) {

    if (IS_FFS_FILE2 (PadFileHeader)) {
      //
      // Calculate checksum of Pad File Data
      //
      PadFileHeader->IntegrityCheck.Checksum.File =
        CalculateCheckSum8 ((UINT8 *) PadFileHeader + sizeof (EFI_FFS_FILE_HEADER2), FFS_FILE2_SIZE (PadFileHeader) - sizeof (EFI_FFS_FILE_HEADER2));

      } else {
      //
      // Calculate checksum of Pad File Data
      //
      PadFileHeader->IntegrityCheck.Checksum.File =
        CalculateCheckSum8 ((UINT8 *) PadFileHeader + sizeof (EFI_FFS_FILE_HEADER), FFS_FILE_SIZE (PadFileHeader) - sizeof (EFI_FFS_FILE_HEADER));
    }

  } else {

    PadFileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;

  }

  return ;
}

/**
  Create a PAD File in the Free Space.

  @param FvDevice        Firmware Volume Device.
  @param FreeSpaceEntry  Indicating in which Free Space(Cache) the Pad file will be inserted.
  @param Size            Pad file Size, not include the header.
  @param PadFileEntry    The Ffs File Entry that points to this Pad File.

  @retval EFI_SUCCESS            Successfully create a PAD file.
  @retval EFI_OUT_OF_RESOURCES   No enough free space to create a PAD file.
  @retval EFI_INVALID_PARAMETER  Size is not 8 byte alignment.
  @retval EFI_DEVICE_ERROR       Free space is not erased.
**/
EFI_STATUS
FvCreatePadFileInFreeSpace (
  IN  FV_DEVICE           *FvDevice,
  IN  FREE_SPACE_ENTRY    *FreeSpaceEntry,
  IN  UINTN               Size,
  OUT FFS_FILE_LIST_ENTRY **PadFileEntry
  )
{
  EFI_STATUS                          Status;
  EFI_FFS_FILE_HEADER                 *PadFileHeader;
  UINTN                               Offset;
  UINTN                               NumBytesWritten;
  UINTN                               StateOffset;
  UINT8                               *StartPos;
  FFS_FILE_LIST_ENTRY                 *FfsFileEntry;
  UINTN                               HeaderSize;
  UINTN                               FileSize;

  HeaderSize = sizeof (EFI_FFS_FILE_HEADER);
  FileSize = Size + HeaderSize;
  if (FileSize > 0x00FFFFFF) {
    HeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
    FileSize = Size + HeaderSize;
  }

  if (FreeSpaceEntry->Length < FileSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  if ((Size & 0x07) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  StartPos = FreeSpaceEntry->StartingAddress;

  //
  // First double check the space
  //
  if (!IsBufferErased (
        FvDevice->ErasePolarity,
        StartPos,
        FileSize
        )) {
    return EFI_DEVICE_ERROR;
  }

  PadFileHeader = (EFI_FFS_FILE_HEADER *) StartPos;

  //
  // Create File Step 1
  //
  SetFileState (EFI_FILE_HEADER_CONSTRUCTION, PadFileHeader);

  Offset          = (UINTN) (StartPos - FvDevice->CachedFv);
  StateOffset     = Offset + (UINT8 *) &PadFileHeader->State - (UINT8 *) PadFileHeader;

  NumBytesWritten = sizeof (EFI_FFS_FILE_STATE);
  Status = FvcWrite (
            FvDevice,
            StateOffset,
            &NumBytesWritten,
            &PadFileHeader->State
            );
  if (EFI_ERROR (Status)) {
    SetFileState (EFI_FILE_HEADER_CONSTRUCTION, PadFileHeader);
    return Status;
  }
  //
  // Update Free Space Entry, since header is allocated
  //
  FreeSpaceEntry->Length -= HeaderSize;
  FreeSpaceEntry->StartingAddress += HeaderSize;

  //
  // Fill File Name Guid, here we assign a NULL-GUID to Pad files
  //
  ZeroMem (&PadFileHeader->Name, sizeof (EFI_GUID));

  //
  // Fill File Type, checksum(0), Attributes(0), Size
  //
  PadFileHeader->Type       = EFI_FV_FILETYPE_FFS_PAD;
  PadFileHeader->Attributes = 0;
  if ((FileSize) > 0x00FFFFFF) {
    ((EFI_FFS_FILE_HEADER2 *) PadFileHeader)->ExtendedSize = (UINT32) FileSize;
    *(UINT32 *) PadFileHeader->Size &= 0xFF000000;
    PadFileHeader->Attributes |= FFS_ATTRIB_LARGE_FILE;
  } else {
    *(UINT32 *) PadFileHeader->Size &= 0xFF000000;
    *(UINT32 *) PadFileHeader->Size |= FileSize;
  }

  SetHeaderChecksum (PadFileHeader);
  SetPadFileChecksum (PadFileHeader);

  Offset          = (UINTN) (StartPos - FvDevice->CachedFv);

  NumBytesWritten = HeaderSize;
  Status = FvcWrite (
            FvDevice,
            Offset,
            &NumBytesWritten,
            (UINT8 *) PadFileHeader
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Step 2, then Mark header valid, since no data write,
  // mark the data valid at the same time.
  //
  SetFileState (EFI_FILE_HEADER_VALID, PadFileHeader);
  SetFileState (EFI_FILE_DATA_VALID, PadFileHeader);

  Offset          = (UINTN) (StartPos - FvDevice->CachedFv);
  StateOffset     = Offset + (UINT8 *) &PadFileHeader->State - (UINT8 *) PadFileHeader;

  NumBytesWritten = sizeof (EFI_FFS_FILE_STATE);
  Status = FvcWrite (
            FvDevice,
            StateOffset,
            &NumBytesWritten,
            &PadFileHeader->State
            );
  if (EFI_ERROR (Status)) {
    SetFileState (EFI_FILE_HEADER_VALID, PadFileHeader);
    SetFileState (EFI_FILE_DATA_VALID, PadFileHeader);
    return Status;
  }
  //
  // Update Free Space Entry, since header is allocated
  //
  FreeSpaceEntry->Length -= Size;
  FreeSpaceEntry->StartingAddress += Size;

  //
  // If successfully, insert an FfsFileEntry at the end of ffs file list
  //
  FfsFileEntry = AllocateZeroPool (sizeof (FFS_FILE_LIST_ENTRY));
  ASSERT (FfsFileEntry != NULL);

  FfsFileEntry->FfsHeader = (UINT8 *) (UINTN) StartPos;
  InsertTailList (&FvDevice->FfsFileListHeader, &FfsFileEntry->Link);

  *PadFileEntry             = FfsFileEntry;
  FvDevice->CurrentFfsFile  = FfsFileEntry;

  return EFI_SUCCESS;
}

/**
  Fill pad file header within firmware cache.
  
  @param PadFileHeader    The start of the Pad File Buffer.
  @param PadFileLength    The length of the pad file including the header.

**/
VOID
FvFillPadFile (
  IN EFI_FFS_FILE_HEADER  *PadFileHeader,
  IN UINTN                PadFileLength
  )
{
  //
  // Fill File Name Guid, here we assign a NULL-GUID to Pad files
  //
  ZeroMem (&PadFileHeader->Name, sizeof (EFI_GUID));

  //
  // Fill File Type, checksum(0), Attributes(0), Size
  //
  PadFileHeader->Type       = EFI_FV_FILETYPE_FFS_PAD;
  PadFileHeader->Attributes = 0;
  if (PadFileLength > 0x00FFFFFF) {
    ((EFI_FFS_FILE_HEADER2 *) PadFileHeader)->ExtendedSize = (UINT32) PadFileLength;
    *(UINT32 *) PadFileHeader->Size &= 0xFF000000;
    PadFileHeader->Attributes |= FFS_ATTRIB_LARGE_FILE;
  } else {
    *(UINT32 *) PadFileHeader->Size &= 0xFF000000;
    *(UINT32 *) PadFileHeader->Size |= PadFileLength;
  }

  SetHeaderChecksum (PadFileHeader);
  SetPadFileChecksum (PadFileHeader);

  //
  // Set File State to 0x00000111
  //
  SetFileState (EFI_FILE_HEADER_CONSTRUCTION, PadFileHeader);
  SetFileState (EFI_FILE_HEADER_VALID, PadFileHeader);
  SetFileState (EFI_FILE_DATA_VALID, PadFileHeader);

  return ;
}

/**
  Create entire FFS file.
  
  @param FileHeader      Starting Address of a Buffer that hold the FFS File image.
  @param FfsFileBuffer   The source buffer that contains the File Data.
  @param BufferSize      The length of FfsFileBuffer.
  @param ActualFileSize  Size of FFS file.
  @param FileName        The Guid of Ffs File.
  @param FileType        The type of the written Ffs File.
  @param FileAttributes  The attributes of the written Ffs File.

  @retval EFI_INVALID_PARAMETER  File type is not valid.
  @retval EFI_SUCCESS            FFS file is successfully created.

**/
EFI_STATUS
FvFillFfsFile (
  OUT EFI_FFS_FILE_HEADER   *FileHeader,
  IN UINT8                  *FfsFileBuffer,
  IN UINTN                  BufferSize,
  IN UINTN                  ActualFileSize,
  IN EFI_GUID               *FileName,
  IN EFI_FV_FILETYPE        FileType,
  IN EFI_FV_FILE_ATTRIBUTES FileAttributes
  )
{
  EFI_FFS_FILE_ATTRIBUTES TmpFileAttribute;
  EFI_FFS_FILE_HEADER     *TmpFileHeader;

  //
  // File Type value 0x0E~0xE0 are reserved
  //
  if ((FileType > EFI_FV_FILETYPE_SMM_CORE) && (FileType < 0xE0)) {
    return EFI_INVALID_PARAMETER;
  }

  TmpFileHeader = (EFI_FFS_FILE_HEADER *) FfsFileBuffer;
  //
  // First fill all fields ready in FfsFileBuffer
  //
  CopyGuid (&TmpFileHeader->Name, FileName);
  TmpFileHeader->Type = FileType;

  //
  // Convert the FileAttributes to FFSFileAttributes
  //
  FvFileAttrib2FfsFileAttrib (FileAttributes, &TmpFileAttribute);

  TmpFileHeader->Attributes = TmpFileAttribute;

  if (ActualFileSize > 0x00FFFFFF) {
    ((EFI_FFS_FILE_HEADER2 *) FileHeader)->ExtendedSize = (UINT32) ActualFileSize;
    *(UINT32 *) FileHeader->Size &= 0xFF000000;
    FileHeader->Attributes |= FFS_ATTRIB_LARGE_FILE;
  } else {
    *(UINT32 *) FileHeader->Size &= 0xFF000000;
    *(UINT32 *) FileHeader->Size |= ActualFileSize;
  }

  SetHeaderChecksum (TmpFileHeader);
  SetFileChecksum (TmpFileHeader, ActualFileSize);

  SetFileState (EFI_FILE_HEADER_CONSTRUCTION, TmpFileHeader);
  SetFileState (EFI_FILE_HEADER_VALID, TmpFileHeader);
  SetFileState (EFI_FILE_DATA_VALID, TmpFileHeader);

  //
  // Copy data from FfsFileBuffer to FileHeader(cache)
  //
  CopyMem (FileHeader, FfsFileBuffer, BufferSize);

  return EFI_SUCCESS;
}

/**
  Fill some other extra space using 0xFF(Erase Value).

  @param  ErasePolarity  Fv erase value.
  @param  FileHeader     Point to the start of FFS File.
  @param  ExtraLength    The pading length.

**/
VOID
FvAdjustFfsFile (
  IN  UINT8                 ErasePolarity,
  IN  EFI_FFS_FILE_HEADER   *FileHeader,
  IN  UINTN                 ExtraLength
  )
{
  UINT8 *Ptr;
  UINT8 PadingByte;

  if (IS_FFS_FILE2 (FileHeader)) {
    Ptr         = (UINT8 *) FileHeader + FFS_FILE2_SIZE (FileHeader);
  } else {
    Ptr         = (UINT8 *) FileHeader + FFS_FILE_SIZE (FileHeader);
  }

  if (ErasePolarity == 0) {
    PadingByte = 0;
  } else {
    PadingByte = 0xFF;
  }
  //
  // Fill the non-used space with Padding Byte
  //
  SetMem (Ptr, ExtraLength, PadingByte);

  return ;
}

/**
  Free File List entry pointed by FileListHead.

  @param FileListHeader   FileListEntry Header.

**/
VOID
FreeFileList (
  IN  LIST_ENTRY  *FileListHead
  )
{
  FFS_FILE_LIST_ENTRY *FfsFileEntry;
  LIST_ENTRY      *NextEntry;

  FfsFileEntry = (FFS_FILE_LIST_ENTRY *) (FileListHead->ForwardLink);

  //
  // Loop the whole list entry to free resources
  //
  while (&FfsFileEntry->Link != FileListHead) {
    NextEntry = (&FfsFileEntry->Link)->ForwardLink;
    FreePool (FfsFileEntry);
    FfsFileEntry = (FFS_FILE_LIST_ENTRY *) NextEntry;
  }

  return ;
}

/**
  Create a new file within a PAD file area.

  @param FvDevice        Firmware Volume Device.
  @param FfsFileBuffer   A buffer that holds an FFS file,(it contains a File Header which is in init state).
  @param BufferSize      The size of FfsFileBuffer.
  @param ActualFileSize  The actual file length, it may not be multiples of 8.
  @param FileName        The FFS File Name.
  @param FileType        The FFS File Type.
  @param FileAttributes  The Attributes of the FFS File to be created.

  @retval EFI_SUCCESS           Successfully create a new file within the found PAD file area.
  @retval EFI_OUT_OF_RESOURCES  No suitable PAD file is found.
  @retval other errors          New file is created failed.

**/
EFI_STATUS
FvCreateNewFileInsidePadFile (
  IN  FV_DEVICE               *FvDevice,
  IN  UINT8                   *FfsFileBuffer,
  IN  UINTN                   BufferSize,
  IN  UINTN                   ActualFileSize,
  IN  EFI_GUID                *FileName,
  IN  EFI_FV_FILETYPE         FileType,
  IN  EFI_FV_FILE_ATTRIBUTES  FileAttributes
  )
{
  UINTN                               RequiredAlignment;
  FFS_FILE_LIST_ENTRY                 *PadFileEntry;
  EFI_STATUS                          Status;
  UINTN                               PadAreaLength;
  UINTN                               PadSize;
  EFI_FFS_FILE_HEADER                 *FileHeader;
  EFI_FFS_FILE_HEADER                 *OldPadFileHeader;
  EFI_FFS_FILE_HEADER                 *PadFileHeader;
  EFI_FFS_FILE_HEADER                 *TailPadFileHeader;
  UINTN                               StateOffset;
  UINTN                               Offset;
  UINTN                               NumBytesWritten;
  UINT8                               *StartPos;
  LIST_ENTRY                          NewFileList;
  FFS_FILE_LIST_ENTRY                 *NewFileListEntry;
  FFS_FILE_LIST_ENTRY                 *FfsEntry;
  FFS_FILE_LIST_ENTRY                 *NextFfsEntry;

  //
  // First get the required alignment from the File Attributes
  //
  RequiredAlignment = GetRequiredAlignment (FileAttributes);

  //
  // Find a suitable PAD File
  //
  Status = FvLocatePadFile (
            FvDevice,
            BufferSize,
            RequiredAlignment,
            &PadSize,
            &PadFileEntry
            );

  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  OldPadFileHeader = (EFI_FFS_FILE_HEADER *) PadFileEntry->FfsHeader;

  //
  // Step 1: Update Pad File Header
  //
  SetFileState (EFI_FILE_MARKED_FOR_UPDATE, OldPadFileHeader);

  StartPos = PadFileEntry->FfsHeader;

  Offset          = (UINTN) (StartPos - FvDevice->CachedFv);
  StateOffset     = Offset + (UINT8 *) &OldPadFileHeader->State - (UINT8 *) OldPadFileHeader;

  NumBytesWritten = sizeof (EFI_FFS_FILE_STATE);
  Status = FvcWrite (
            FvDevice,
            StateOffset,
            &NumBytesWritten,
            &OldPadFileHeader->State
            );
  if (EFI_ERROR (Status)) {
    SetFileState (EFI_FILE_HEADER_CONSTRUCTION, OldPadFileHeader);
    return Status;
  }

  //
  // Step 2: Update Pad area
  //
  InitializeListHead (&NewFileList);

  if (IS_FFS_FILE2 (OldPadFileHeader)) {
    PadAreaLength = FFS_FILE2_SIZE (OldPadFileHeader) - sizeof (EFI_FFS_FILE_HEADER);
    PadFileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) OldPadFileHeader + sizeof (EFI_FFS_FILE_HEADER2));
  } else {
    PadAreaLength = FFS_FILE_SIZE (OldPadFileHeader) - sizeof (EFI_FFS_FILE_HEADER);
    PadFileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) OldPadFileHeader + sizeof (EFI_FFS_FILE_HEADER));
  }

  if (PadSize != 0) {
    //
    // Insert a PAD file before to achieve required alignment
    //
    FvFillPadFile (PadFileHeader, PadSize);
    NewFileListEntry            = AllocatePool (sizeof (FFS_FILE_LIST_ENTRY));
    ASSERT (NewFileListEntry   != NULL);
    NewFileListEntry->FfsHeader = (UINT8 *) PadFileHeader;
    InsertTailList (&NewFileList, &NewFileListEntry->Link);
  }

  FileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) PadFileHeader + PadSize);

  Status = FvFillFfsFile (
            FileHeader,
            FfsFileBuffer,
            BufferSize,
            ActualFileSize,
            FileName,
            FileType,
            FileAttributes
            );
  if (EFI_ERROR (Status)) {
    FreeFileList (&NewFileList);
    return Status;
  }

  NewFileListEntry            = AllocatePool (sizeof (FFS_FILE_LIST_ENTRY));
  ASSERT (NewFileListEntry   != NULL);

  NewFileListEntry->FfsHeader = (UINT8 *) FileHeader;
  InsertTailList (&NewFileList, &NewFileListEntry->Link);

  FvDevice->CurrentFfsFile = NewFileListEntry;

  if (PadAreaLength > (BufferSize + PadSize)) {
    if ((PadAreaLength - BufferSize - PadSize) >= sizeof (EFI_FFS_FILE_HEADER)) {
      //
      // we can insert another PAD file
      //
      TailPadFileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) FileHeader + BufferSize);
      FvFillPadFile (TailPadFileHeader, PadAreaLength - BufferSize - PadSize);

      NewFileListEntry            = AllocatePool (sizeof (FFS_FILE_LIST_ENTRY));
      ASSERT (NewFileListEntry   != NULL);

      NewFileListEntry->FfsHeader = (UINT8 *) TailPadFileHeader;
      InsertTailList (&NewFileList, &NewFileListEntry->Link);
    } else {
      //
      // because left size cannot hold another PAD file header,
      // adjust the writing file size (just in cache)
      //
      FvAdjustFfsFile (
        FvDevice->ErasePolarity,
        FileHeader,
        PadAreaLength - BufferSize - PadSize
        );
    }
  }
  //
  // Start writing to FV
  //
  if (IS_FFS_FILE2 (OldPadFileHeader)) {
    StartPos = (UINT8 *) OldPadFileHeader + sizeof (EFI_FFS_FILE_HEADER2);
  } else {
    StartPos = (UINT8 *) OldPadFileHeader + sizeof (EFI_FFS_FILE_HEADER);
  }

  Offset          = (UINTN) (StartPos - FvDevice->CachedFv);

  NumBytesWritten = PadAreaLength;
  Status = FvcWrite (
            FvDevice,
            Offset,
            &NumBytesWritten,
            StartPos
            );
  if (EFI_ERROR (Status)) {
    FreeFileList (&NewFileList);
    FvDevice->CurrentFfsFile = NULL;
    return Status;
  }

  //
  // Step 3: Mark Pad file header as EFI_FILE_HEADER_INVALID
  //
  SetFileState (EFI_FILE_HEADER_INVALID, OldPadFileHeader);

  StartPos = PadFileEntry->FfsHeader;

  Offset          = (UINTN) (StartPos - FvDevice->CachedFv);
  StateOffset     = Offset + (UINT8 *) &OldPadFileHeader->State - (UINT8 *) OldPadFileHeader;

  NumBytesWritten = sizeof (EFI_FFS_FILE_STATE);
  Status = FvcWrite (
            FvDevice,
            StateOffset,
            &NumBytesWritten,
            &OldPadFileHeader->State
            );
  if (EFI_ERROR (Status)) {
    SetFileState (EFI_FILE_HEADER_INVALID, OldPadFileHeader);
    FreeFileList (&NewFileList);
    FvDevice->CurrentFfsFile = NULL;
    return Status;
  }

  //
  // If all successfully, update FFS_FILE_LIST
  //

  //
  // Delete old pad file entry
  //
  FfsEntry      = (FFS_FILE_LIST_ENTRY *) PadFileEntry->Link.BackLink;
  NextFfsEntry  = (FFS_FILE_LIST_ENTRY *) PadFileEntry->Link.ForwardLink;

  FreePool (PadFileEntry);

  FfsEntry->Link.ForwardLink          = NewFileList.ForwardLink;
  (NewFileList.ForwardLink)->BackLink = &FfsEntry->Link;
  NextFfsEntry->Link.BackLink         = NewFileList.BackLink;
  (NewFileList.BackLink)->ForwardLink = &NextFfsEntry->Link;

  return EFI_SUCCESS;
}

/**
  Free all FfsBuffer.

  @param NumOfFiles      Number of FfsBuffer.
  @param FfsBuffer       An array of pointer to an FFS File Buffer

**/
VOID
FreeFfsBuffer (
  IN UINTN    NumOfFiles,
  IN UINT8    **FfsBuffer
  )
{
  UINTN Index;
  for (Index = 0; Index < NumOfFiles; Index++) {
    if (FfsBuffer[Index] != NULL) {
      FreePool (FfsBuffer[Index]);
    }
  }
}

/**
  Create multiple files within a PAD File area.

  @param FvDevice        Firmware Volume Device.
  @param PadFileEntry    The pad file entry to be written in.
  @param NumOfFiles      Total File number to be written.
  @param BufferSize      The array of buffer size of each FfsBuffer.
  @param ActualFileSize  The array of actual file size.
  @param PadSize         The array of leading pad file size for each FFS File
  @param FfsBuffer       The array of Ffs Buffer pointer.
  @param FileData        The array of EFI_FV_WRITE_FILE_DATA structure, 
                         used to get name, attributes, type, etc.

  @retval EFI_SUCCESS           Add the input multiple files into PAD file area.
  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval other error           Files can't be added into PAD file area.

**/
EFI_STATUS
FvCreateMultipleFilesInsidePadFile (
  IN FV_DEVICE              *FvDevice,
  IN FFS_FILE_LIST_ENTRY    *PadFileEntry,
  IN UINTN                  NumOfFiles,
  IN UINTN                  *BufferSize,
  IN UINTN                  *ActualFileSize,
  IN UINTN                  *PadSize,
  IN UINT8                  **FfsBuffer,
  IN EFI_FV_WRITE_FILE_DATA *FileData
  )
{
  EFI_STATUS                          Status;
  EFI_FFS_FILE_HEADER                 *OldPadFileHeader;
  UINTN                               Index;
  EFI_FFS_FILE_HEADER                 *PadFileHeader;
  EFI_FFS_FILE_HEADER                 *FileHeader;
  EFI_FFS_FILE_HEADER                 *TailPadFileHeader;
  UINTN                               TotalSize;
  UINTN                               PadAreaLength;
  LIST_ENTRY                          NewFileList;
  FFS_FILE_LIST_ENTRY                 *NewFileListEntry;
  UINTN                               Offset;
  UINTN                               NumBytesWritten;
  UINT8                               *StartPos;
  FFS_FILE_LIST_ENTRY                 *FfsEntry;
  FFS_FILE_LIST_ENTRY                 *NextFfsEntry;

  InitializeListHead (&NewFileList);

  NewFileListEntry  = NULL;

  OldPadFileHeader  = (EFI_FFS_FILE_HEADER *) PadFileEntry->FfsHeader;
  if (IS_FFS_FILE2 (OldPadFileHeader)) {
    PadAreaLength = FFS_FILE2_SIZE (OldPadFileHeader) - sizeof (EFI_FFS_FILE_HEADER2);
  } else {
    PadAreaLength = FFS_FILE_SIZE (OldPadFileHeader) - sizeof (EFI_FFS_FILE_HEADER);
  }

  Status = UpdateHeaderBit (
            FvDevice,
            OldPadFileHeader,
            EFI_FILE_MARKED_FOR_UPDATE
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Update PAD area
  //
  TotalSize     = 0;
  if (IS_FFS_FILE2 (OldPadFileHeader)) {
    PadFileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) OldPadFileHeader + sizeof (EFI_FFS_FILE_HEADER2));
  } else {
    PadFileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) OldPadFileHeader + sizeof (EFI_FFS_FILE_HEADER));
  }
  FileHeader    = PadFileHeader;

  for (Index = 0; Index < NumOfFiles; Index++) {
    if (PadSize[Index] != 0) {
      FvFillPadFile (PadFileHeader, PadSize[Index]);
      NewFileListEntry = AllocatePool (sizeof (FFS_FILE_LIST_ENTRY));
      if (NewFileListEntry == NULL) {
        FreeFileList (&NewFileList);
        return EFI_OUT_OF_RESOURCES;
      }

      NewFileListEntry->FfsHeader = (UINT8 *) PadFileHeader;
      InsertTailList (&NewFileList, &NewFileListEntry->Link);
    }

    FileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) PadFileHeader + PadSize[Index]);
    Status = FvFillFfsFile (
              FileHeader,
              FfsBuffer[Index],
              BufferSize[Index],
              ActualFileSize[Index],
              FileData[Index].NameGuid,
              FileData[Index].Type,
              FileData[Index].FileAttributes
              );
    if (EFI_ERROR (Status)) {
      FreeFileList (&NewFileList);
      return Status;
    }

    NewFileListEntry = AllocatePool (sizeof (FFS_FILE_LIST_ENTRY));
    if (NewFileListEntry == NULL) {
      FreeFileList (&NewFileList);
      return EFI_OUT_OF_RESOURCES;
    }

    NewFileListEntry->FfsHeader = (UINT8 *) FileHeader;
    InsertTailList (&NewFileList, &NewFileListEntry->Link);

    PadFileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) FileHeader + BufferSize[Index]);
    TotalSize += PadSize[Index];
    TotalSize += BufferSize[Index];
  }

  FvDevice->CurrentFfsFile = NewFileListEntry;
  //
  // Maybe we need a tail pad file
  //
  if (PadAreaLength > TotalSize) {
    if ((PadAreaLength - TotalSize) >= sizeof (EFI_FFS_FILE_HEADER)) {
      //
      // we can insert another PAD file
      //
      TailPadFileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) FileHeader + BufferSize[NumOfFiles - 1]);
      FvFillPadFile (TailPadFileHeader, PadAreaLength - TotalSize);

      NewFileListEntry = AllocatePool (sizeof (FFS_FILE_LIST_ENTRY));
      if (NewFileListEntry == NULL) {
        FreeFileList (&NewFileList);
        FvDevice->CurrentFfsFile = NULL;
        return EFI_OUT_OF_RESOURCES;
      }

      NewFileListEntry->FfsHeader = (UINT8 *) TailPadFileHeader;
      InsertTailList (&NewFileList, &NewFileListEntry->Link);
    } else {
      //
      // because left size cannot hold another PAD file header,
      // adjust the writing file size (just in cache)
      //
      FvAdjustFfsFile (
        FvDevice->ErasePolarity,
        FileHeader,
        PadAreaLength - TotalSize
        );
    }
  }
  //
  // Start writing to FV
  //
  if (IS_FFS_FILE2 (OldPadFileHeader)) {
    StartPos = (UINT8 *) OldPadFileHeader + sizeof (EFI_FFS_FILE_HEADER2);
  } else {
    StartPos = (UINT8 *) OldPadFileHeader + sizeof (EFI_FFS_FILE_HEADER);
  }

  Offset          = (UINTN) (StartPos - FvDevice->CachedFv);

  NumBytesWritten = PadAreaLength;
  Status = FvcWrite (
            FvDevice,
            Offset,
            &NumBytesWritten,
            StartPos
            );
  if (EFI_ERROR (Status)) {
    FreeFileList (&NewFileList);
    FvDevice->CurrentFfsFile = NULL;
    return Status;
  }

  Status = UpdateHeaderBit (
            FvDevice,
            OldPadFileHeader,
            EFI_FILE_HEADER_INVALID
            );
  if (EFI_ERROR (Status)) {
    FreeFileList (&NewFileList);
    FvDevice->CurrentFfsFile = NULL;
    return Status;
  }

  //
  // Update File List Link
  //

  //
  // First delete old pad file entry
  //
  FfsEntry      = (FFS_FILE_LIST_ENTRY *) PadFileEntry->Link.BackLink;
  NextFfsEntry  = (FFS_FILE_LIST_ENTRY *) PadFileEntry->Link.ForwardLink;

  FreePool (PadFileEntry);

  FfsEntry->Link.ForwardLink          = NewFileList.ForwardLink;
  (NewFileList.ForwardLink)->BackLink = &FfsEntry->Link;
  NextFfsEntry->Link.BackLink         = NewFileList.BackLink;
  (NewFileList.BackLink)->ForwardLink = &NextFfsEntry->Link;

  return EFI_SUCCESS;
}

/**
  Create multiple files within the Free Space.

  @param FvDevice        Firmware Volume Device.
  @param FreeSpaceEntry  Indicating in which Free Space(Cache) the multiple files will be inserted.
  @param NumOfFiles      Total File number to be written.
  @param BufferSize      The array of buffer size of each FfsBuffer.
  @param ActualFileSize  The array of actual file size.
  @param PadSize         The array of leading pad file size for each FFS File
  @param FfsBuffer       The array of Ffs Buffer pointer.
  @param FileData        The array of EFI_FV_WRITE_FILE_DATA structure, 
                         used to get name, attributes, type, etc.

  @retval EFI_SUCCESS           Add the input multiple files into PAD file area.
  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval other error           Files can't be added into PAD file area.

**/
EFI_STATUS
FvCreateMultipleFilesInsideFreeSpace (
  IN FV_DEVICE              *FvDevice,
  IN FREE_SPACE_ENTRY       *FreeSpaceEntry,
  IN UINTN                  NumOfFiles,
  IN UINTN                  *BufferSize,
  IN UINTN                  *ActualFileSize,
  IN UINTN                  *PadSize,
  IN UINT8                  **FfsBuffer,
  IN EFI_FV_WRITE_FILE_DATA *FileData
  )
{
  EFI_STATUS                          Status;
  UINTN                               Index;
  EFI_FFS_FILE_HEADER                 *PadFileHeader;
  EFI_FFS_FILE_HEADER                 *FileHeader;
  UINTN                               TotalSize;
  LIST_ENTRY                          NewFileList;
  FFS_FILE_LIST_ENTRY                 *NewFileListEntry;
  UINTN                               Offset;
  UINTN                               NumBytesWritten;
  UINT8                               *StartPos;

  InitializeListHead (&NewFileList);

  NewFileListEntry  = NULL;

  TotalSize     = 0;
  StartPos      = FreeSpaceEntry->StartingAddress;
  PadFileHeader = (EFI_FFS_FILE_HEADER *) StartPos;
  FileHeader    = PadFileHeader;

  for (Index = 0; Index < NumOfFiles; Index++) {
    if (PadSize[Index] != 0) {
      FvFillPadFile (PadFileHeader, PadSize[Index]);
      NewFileListEntry = AllocatePool (sizeof (FFS_FILE_LIST_ENTRY));
      if (NewFileListEntry == NULL) {
        FreeFileList (&NewFileList);
        return EFI_OUT_OF_RESOURCES;
      }

      NewFileListEntry->FfsHeader = (UINT8 *) PadFileHeader;
      InsertTailList (&NewFileList, &NewFileListEntry->Link);
    }

    FileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) PadFileHeader + PadSize[Index]);
    Status = FvFillFfsFile (
              FileHeader,
              FfsBuffer[Index],
              BufferSize[Index],
              ActualFileSize[Index],
              FileData[Index].NameGuid,
              FileData[Index].Type,
              FileData[Index].FileAttributes
              );
    if (EFI_ERROR (Status)) {
      FreeFileList (&NewFileList);
      return Status;
    }

    NewFileListEntry = AllocatePool (sizeof (FFS_FILE_LIST_ENTRY));
    if (NewFileListEntry == NULL) {
      FreeFileList (&NewFileList);
      return EFI_OUT_OF_RESOURCES;
    }

    NewFileListEntry->FfsHeader = (UINT8 *) FileHeader;
    InsertTailList (&NewFileList, &NewFileListEntry->Link);

    PadFileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) FileHeader + BufferSize[Index]);
    TotalSize += PadSize[Index];
    TotalSize += BufferSize[Index];
  }

  if (FreeSpaceEntry->Length < TotalSize) {
    FreeFileList (&NewFileList);
    return EFI_OUT_OF_RESOURCES;
  }

  FvDevice->CurrentFfsFile = NewFileListEntry;

  //
  // Start writing to FV
  //
  Offset          = (UINTN) (StartPos - FvDevice->CachedFv);

  NumBytesWritten = TotalSize;
  Status = FvcWrite (
            FvDevice,
            Offset,
            &NumBytesWritten,
            StartPos
            );
  if (EFI_ERROR (Status)) {
    FreeFileList (&NewFileList);
    FvDevice->CurrentFfsFile = NULL;
    return Status;
  }

  FreeSpaceEntry->Length -= TotalSize;
  FreeSpaceEntry->StartingAddress += TotalSize;

  NewFileListEntry = (FFS_FILE_LIST_ENTRY *) (NewFileList.ForwardLink);

  while (NewFileListEntry != (FFS_FILE_LIST_ENTRY *) &NewFileList) {
    InsertTailList (&FvDevice->FfsFileListHeader, &NewFileListEntry->Link);
    NewFileListEntry = (FFS_FILE_LIST_ENTRY *) (NewFileListEntry->Link.ForwardLink);
  }

  return EFI_SUCCESS;
}

/**
  Write multiple files into FV in reliable method.

  @param FvDevice        Firmware Volume Device.
  @param NumOfFiles      Total File number to be written.
  @param FileData        The array of EFI_FV_WRITE_FILE_DATA structure, 
                         used to get name, attributes, type, etc
  @param FileOperation   The array of operation for each file.

  @retval EFI_SUCCESS            Files are added into FV.
  @retval EFI_OUT_OF_RESOURCES   No enough free PAD files to add the input files.
  @retval EFI_INVALID_PARAMETER  File number is less than or equal to 1.
  @retval EFI_UNSUPPORTED        File number exceeds the supported max numbers of files.

**/
EFI_STATUS
FvCreateMultipleFiles (
  IN  FV_DEVICE               *FvDevice,
  IN  UINTN                   NumOfFiles,
  IN  EFI_FV_WRITE_FILE_DATA  *FileData,
  IN  BOOLEAN                 *FileOperation
  )
{
  EFI_STATUS                    Status;
  UINT8                         *FfsBuffer[MAX_FILES];
  UINTN                         Index1;
  UINTN                         Index2;
  UINTN                         BufferSize[MAX_FILES];
  UINTN                         ActualFileSize[MAX_FILES];
  UINTN                         RequiredAlignment[MAX_FILES];
  UINTN                         PadSize[MAX_FILES];
  FFS_FILE_LIST_ENTRY           *PadFileEntry;
  UINTN                         TotalSizeNeeded;
  FREE_SPACE_ENTRY              *FreeSpaceEntry;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  UINTN                         Key;
  EFI_GUID                      FileNameGuid;
  EFI_FV_FILETYPE               OldFileType;
  EFI_FV_FILE_ATTRIBUTES        OldFileAttributes;
  UINTN                         OldFileSize;
  FFS_FILE_LIST_ENTRY           *OldFfsFileEntry[MAX_FILES];
  EFI_FFS_FILE_HEADER           *OldFileHeader[MAX_FILES];
  BOOLEAN                       IsCreateFile;
  UINTN                         HeaderSize;

  //
  // To use this function, we must ensure that the NumOfFiles is great
  // than 1
  //
  if (NumOfFiles <= 1) {
    return EFI_INVALID_PARAMETER;
  }

  if (NumOfFiles > MAX_FILES) {
    return EFI_UNSUPPORTED;
  }

  Fv = &FvDevice->Fv;

  SetMem (FfsBuffer, NumOfFiles, 0);
  SetMem (RequiredAlignment, NumOfFiles, 8);
  SetMem (PadSize, NumOfFiles, 0);
  ZeroMem (OldFfsFileEntry, sizeof (OldFfsFileEntry));
  ZeroMem (OldFileHeader, sizeof (OldFileHeader));

  //
  // Adjust file size
  //
  for (Index1 = 0; Index1 < NumOfFiles; Index1++) {
    HeaderSize = sizeof (EFI_FFS_FILE_HEADER);
    ActualFileSize[Index1] = FileData[Index1].BufferSize + HeaderSize;
    if (ActualFileSize[Index1] > 0x00FFFFFF) {
      HeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
      ActualFileSize[Index1] = FileData[Index1].BufferSize + HeaderSize;
    }
    BufferSize[Index1]     = ActualFileSize[Index1];

    if (BufferSize[Index1] == HeaderSize) {
      //
      // clear file attributes, zero-length file does not have any attributes
      //
      FileData[Index1].FileAttributes = 0;
    }

    while ((BufferSize[Index1] & 0x07) != 0) {
      BufferSize[Index1]++;
    }

    FfsBuffer[Index1] = AllocateZeroPool (BufferSize[Index1]);

    //
    // Copy File Data into FileBuffer
    //
    CopyMem (
      FfsBuffer[Index1] + HeaderSize,
      FileData[Index1].Buffer,
      FileData[Index1].BufferSize
      );

    if (FvDevice->ErasePolarity == 1) {
      for (Index2 = 0; Index2 < HeaderSize; Index2++) {
        FfsBuffer[Index1][Index2] = (UINT8)~FfsBuffer[Index1][Index2];
      }
    }

    if ((FileData[Index1].FileAttributes & EFI_FV_FILE_ATTRIB_ALIGNMENT) != 0) {
      RequiredAlignment[Index1] = GetRequiredAlignment (FileData[Index1].FileAttributes);
    }
    //
    // If update file, mark the original file header to
    // EFI_FILE_MARKED_FOR_UPDATE
    //
    IsCreateFile = FileOperation[Index1];
    if (!IsCreateFile) {

      Key = 0;
      do {
        OldFileType = 0;
        Status = Fv->GetNextFile (
                      Fv,
                      &Key,
                      &OldFileType,
                      &FileNameGuid,
                      &OldFileAttributes,
                      &OldFileSize
                      );
        if (EFI_ERROR (Status)) {
          FreeFfsBuffer (NumOfFiles, FfsBuffer);
          return Status;
        }
      } while (!CompareGuid (&FileNameGuid, FileData[Index1].NameGuid));

      //
      // Get FfsFileEntry from the search key
      //
      OldFfsFileEntry[Index1]  = (FFS_FILE_LIST_ENTRY *) Key;
      OldFileHeader[Index1]    = (EFI_FFS_FILE_HEADER *) OldFfsFileEntry[Index1]->FfsHeader;
      Status = UpdateHeaderBit (
                FvDevice,
                OldFileHeader[Index1],
                EFI_FILE_MARKED_FOR_UPDATE
                );
      if (EFI_ERROR (Status)) {
        FreeFfsBuffer (NumOfFiles, FfsBuffer);
        return Status;
      }
    }
  }
  //
  // First to search a suitable pad file that can hold so
  // many files
  //
  Status = FvSearchSuitablePadFile (
            FvDevice,
            NumOfFiles,
            BufferSize,
            RequiredAlignment,
            PadSize,
            &TotalSizeNeeded,
            &PadFileEntry
            );

  if (Status == EFI_NOT_FOUND) {
    //
    // Try to find a free space that can hold these files
    //
    Status = FvSearchSuitableFreeSpace (
              FvDevice,
              NumOfFiles,
              BufferSize,
              RequiredAlignment,
              PadSize,
              &TotalSizeNeeded,
              &FreeSpaceEntry
              );
    if (EFI_ERROR (Status)) {
      FreeFfsBuffer (NumOfFiles, FfsBuffer);
      return EFI_OUT_OF_RESOURCES;
    }
    Status = FvCreateMultipleFilesInsideFreeSpace (
              FvDevice,
              FreeSpaceEntry,
              NumOfFiles,
              BufferSize,
              ActualFileSize,
              PadSize,
              FfsBuffer,
              FileData
              );

  } else {
    //
    // Create multiple files inside such a pad file
    // to achieve lock-step update
    //
    Status = FvCreateMultipleFilesInsidePadFile (
              FvDevice,
              PadFileEntry,
              NumOfFiles,
              BufferSize,
              ActualFileSize,
              PadSize,
              FfsBuffer,
              FileData
              );
  }

  FreeFfsBuffer (NumOfFiles, FfsBuffer);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Delete those updated files
  //
  for (Index1 = 0; Index1 < NumOfFiles; Index1++) {
    IsCreateFile = FileOperation[Index1];
    if (!IsCreateFile && OldFfsFileEntry[Index1] != NULL) {
      (OldFfsFileEntry[Index1]->Link.BackLink)->ForwardLink  = OldFfsFileEntry[Index1]->Link.ForwardLink;
      (OldFfsFileEntry[Index1]->Link.ForwardLink)->BackLink  = OldFfsFileEntry[Index1]->Link.BackLink;
      FreePool (OldFfsFileEntry[Index1]);
    }
  }
  //
  // Set those files' state to EFI_FILE_DELETED
  //
  for (Index1 = 0; Index1 < NumOfFiles; Index1++) {
    IsCreateFile = FileOperation[Index1];
    if (!IsCreateFile && OldFileHeader[Index1] != NULL) {
      Status = UpdateHeaderBit (FvDevice, OldFileHeader[Index1], EFI_FILE_DELETED);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  return EFI_SUCCESS;
}
