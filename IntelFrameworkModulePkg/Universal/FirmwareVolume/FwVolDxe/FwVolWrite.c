/** @file
  Implements write firmware file.

  Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>

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
  Calculate the checksum for the FFS header.

  @param FfsHeader   FFS File Header which needs to calculate the checksum

**/
VOID
SetHeaderChecksum (
  IN EFI_FFS_FILE_HEADER *FfsHeader
  )
{
  EFI_FFS_FILE_STATE  State;
  UINT8               FileChecksum;

  //
  // The state and the File checksum are not included
  //
  State = FfsHeader->State;
  FfsHeader->State = 0;

  FileChecksum = FfsHeader->IntegrityCheck.Checksum.File;
  FfsHeader->IntegrityCheck.Checksum.File = 0;

  FfsHeader->IntegrityCheck.Checksum.Header = 0;

  if (IS_FFS_FILE2 (FfsHeader)) {
    FfsHeader->IntegrityCheck.Checksum.Header = CalculateCheckSum8 (
      (UINT8 *) FfsHeader,
      sizeof (EFI_FFS_FILE_HEADER2)
      );
  } else {
    FfsHeader->IntegrityCheck.Checksum.Header = CalculateCheckSum8 (
      (UINT8 *) FfsHeader,
      sizeof (EFI_FFS_FILE_HEADER)
      );
  }

  FfsHeader->State                          = State;
  FfsHeader->IntegrityCheck.Checksum.File   = FileChecksum;

  return ;
}

/**
  Calculate the checksum for the FFS File.

  @param FfsHeader       FFS File Header which needs to calculate the checksum
  @param ActualFileSize  The whole Ffs File Length.

**/
VOID
SetFileChecksum (
  IN EFI_FFS_FILE_HEADER *FfsHeader,
  IN UINTN               ActualFileSize
  )
{
  if ((FfsHeader->Attributes & FFS_ATTRIB_CHECKSUM) != 0) {

    FfsHeader->IntegrityCheck.Checksum.File = 0;

    if (IS_FFS_FILE2 (FfsHeader)) {
      FfsHeader->IntegrityCheck.Checksum.File = CalculateCheckSum8 (
        (UINT8 *) FfsHeader + sizeof (EFI_FFS_FILE_HEADER2),
        ActualFileSize - sizeof (EFI_FFS_FILE_HEADER2)
        );
    } else {
      FfsHeader->IntegrityCheck.Checksum.File = CalculateCheckSum8 (
        (UINT8 *) FfsHeader + sizeof (EFI_FFS_FILE_HEADER),
        ActualFileSize - sizeof (EFI_FFS_FILE_HEADER)
        );
    }

  } else {

    FfsHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;

  }

  return ;
}

/**
  Get the alignment value from File Attributes.

  @param FfsAttributes  FFS attribute

  @return Alignment value.

**/
UINTN
GetRequiredAlignment (
  IN EFI_FV_FILE_ATTRIBUTES FfsAttributes
  )
{
  UINTN AlignmentValue;

  AlignmentValue = FfsAttributes & EFI_FV_FILE_ATTRIB_ALIGNMENT;

  if (AlignmentValue <= 3) {
    return 0x08;
  }

  if (AlignmentValue > 16) {
    //
    // Anyway, we won't reach this code
    //
    return 0x08;
  }

  return (UINTN)1 << AlignmentValue;

}

/**
  Calculate the leading Pad file size to meet the alignment requirement.

  @param FvDevice          Cached Firmware Volume.
  @param StartAddress      The starting address to write the FFS File.
  @param BufferSize        The FFS File Buffer Size.
  @param RequiredAlignment FFS File Data alignment requirement.

  @return The required Pad File Size.

**/
UINTN
CalculatePadFileSize (
  IN FV_DEVICE            *FvDevice,
  IN EFI_PHYSICAL_ADDRESS StartAddress,
  IN UINTN                BufferSize,
  IN UINTN                RequiredAlignment
  )
{
  UINTN DataStartPos;
  UINTN RelativePos;
  UINTN PadSize;

  if (BufferSize > 0x00FFFFFF) {
    DataStartPos  = (UINTN) StartAddress + sizeof (EFI_FFS_FILE_HEADER2);
  } else {
    DataStartPos  = (UINTN) StartAddress + sizeof (EFI_FFS_FILE_HEADER);
  }
  RelativePos   = DataStartPos - (UINTN) FvDevice->CachedFv;

  PadSize       = 0;

  while ((RelativePos & (RequiredAlignment - 1)) != 0) {
    RelativePos++;
    PadSize++;
  }
  //
  // If padsize is 0, no pad file needed;
  // If padsize is great than 24, then pad file can be created
  //
  if ((PadSize == 0) || (PadSize >= sizeof (EFI_FFS_FILE_HEADER))) {
    return PadSize;
  }

  //
  // Perhaps following method can save space
  //
  RelativePos = DataStartPos - (UINTN) FvDevice->CachedFv + sizeof (EFI_FFS_FILE_HEADER);
  PadSize     = sizeof (EFI_FFS_FILE_HEADER);

  while ((RelativePos & (RequiredAlignment - 1)) != 0) {
    RelativePos++;
    PadSize++;
  }

  return PadSize;
}

/**
  Convert EFI_FV_FILE_ATTRIBUTES to FFS_FILE_ATTRIBUTES.

  @param FvFileAttrib    The value of EFI_FV_FILE_ATTRIBUTES
  @param FfsFileAttrib   Pointer to the got FFS_FILE_ATTRIBUTES value.

**/
VOID
FvFileAttrib2FfsFileAttrib (
  IN     EFI_FV_FILE_ATTRIBUTES  FvFileAttrib,
  OUT UINT8                      *FfsFileAttrib
  )
{
  UINT8 FvFileAlignment;
  UINT8 FfsFileAlignment;

  FvFileAlignment   = (UINT8) (FvFileAttrib & EFI_FV_FILE_ATTRIB_ALIGNMENT);
  FfsFileAlignment  = 0;

  switch (FvFileAlignment) {
  case 0:
    //
    // fall through
    //
  case 1:
    //
    // fall through
    //
  case 2:
    //
    // fall through
    //
  case 3:
    //
    // fall through
    //
    FfsFileAlignment = 0;
    break;

  case 4:
    //
    // fall through
    //
  case 5:
    //
    // fall through
    //
  case 6:
    //
    // fall through
    //
    FfsFileAlignment = 1;
    break;

  case 7:
    //
    // fall through
    //
  case 8:
    //
    // fall through
    //
    FfsFileAlignment = 2;
    break;

  case 9:
    FfsFileAlignment = 3;
    break;

  case 10:
    //
    // fall through
    //
  case 11:
    //
    // fall through
    //
    FfsFileAlignment = 4;
    break;

  case 12:
    //
    // fall through
    //
  case 13:
    //
    // fall through
    //
  case 14:
    //
    // fall through
    //
    FfsFileAlignment = 5;
    break;

  case 15:
    FfsFileAlignment = 6;
    break;

  case 16:
    FfsFileAlignment = 7;
    break;
  }

  *FfsFileAttrib = (UINT8) (FfsFileAlignment << 3);

  return ;
}

/**
  Locate a free space entry that can hold this FFS file.

  @param FvDevice          Cached Firmware Volume.
  @param Size              The FFS file size.
  @param RequiredAlignment FFS File Data alignment requirement.
  @param PadSize           Pointer to the size of leading Pad File.
  @param FreeSpaceEntry    Pointer to the Free Space Entry that meets the requirement.

  @retval EFI_SUCCESS     The free space entry is found.
  @retval EFI_NOT_FOUND   The free space entry can't be found.

**/
EFI_STATUS
FvLocateFreeSpaceEntry (
  IN  FV_DEVICE             *FvDevice,
  IN  UINTN                 Size,
  IN  UINTN                 RequiredAlignment,
  OUT UINTN                 *PadSize,
  OUT FREE_SPACE_ENTRY      **FreeSpaceEntry
  )
{
  FREE_SPACE_ENTRY  *FreeSpaceListEntry;
  LIST_ENTRY        *Link;
  UINTN             PadFileSize;

  Link                = FvDevice->FreeSpaceHeader.ForwardLink;
  FreeSpaceListEntry  = (FREE_SPACE_ENTRY *) Link;

  //
  // Loop the free space entry list to find one that can hold the
  // required the file size
  //
  while ((LIST_ENTRY *) FreeSpaceListEntry != &FvDevice->FreeSpaceHeader) {
    PadFileSize = CalculatePadFileSize (
                    FvDevice,
                    (EFI_PHYSICAL_ADDRESS) (UINTN) FreeSpaceListEntry->StartingAddress,
                    Size,
                    RequiredAlignment
                    );
    if (FreeSpaceListEntry->Length >= Size + PadFileSize) {
      *FreeSpaceEntry = FreeSpaceListEntry;
      *PadSize        = PadFileSize;
      return EFI_SUCCESS;
    }

    FreeSpaceListEntry = (FREE_SPACE_ENTRY *) FreeSpaceListEntry->Link.ForwardLink;
  }

  return EFI_NOT_FOUND;

}

/**
  Locate Pad File for writing, this is got from FV Cache.

  @param FvDevice           Cached Firmware Volume.
  @param Size               The required FFS file size.
  @param RequiredAlignment  FFS File Data alignment requirement.
  @param PadSize            Pointer to the size of leading Pad File.
  @param PadFileEntry       Pointer to the Pad File Entry that meets the requirement.

  @retval EFI_SUCCESS     The required pad file is found.
  @retval EFI_NOT_FOUND   The required pad file can't be found.

**/
EFI_STATUS
FvLocatePadFile (
  IN  FV_DEVICE           *FvDevice,
  IN  UINTN               Size,
  IN  UINTN               RequiredAlignment,
  OUT UINTN               *PadSize,
  OUT FFS_FILE_LIST_ENTRY **PadFileEntry
  )
{
  FFS_FILE_LIST_ENTRY *FileEntry;
  EFI_FFS_FILE_STATE  FileState;
  EFI_FFS_FILE_HEADER *FileHeader;
  UINTN               PadAreaLength;
  UINTN               PadFileSize;
  UINTN               HeaderSize;

  FileEntry = (FFS_FILE_LIST_ENTRY *) FvDevice->FfsFileListHeader.ForwardLink;

  //
  // travel through the whole file list to get the pad file entry
  //
  while (FileEntry != (FFS_FILE_LIST_ENTRY *) &FvDevice->FfsFileListHeader) {

    FileHeader  = (EFI_FFS_FILE_HEADER *) FileEntry->FfsHeader;
    FileState   = GetFileState (FvDevice->ErasePolarity, FileHeader);

    if ((FileHeader->Type == EFI_FV_FILETYPE_FFS_PAD) && (FileState == EFI_FILE_DATA_VALID)) {
      //
      // we find one valid pad file, check its free area length
      //
      if (IS_FFS_FILE2 (FileHeader)) {
        HeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
        PadAreaLength = FFS_FILE2_SIZE (FileHeader) - HeaderSize;
      } else {
        HeaderSize = sizeof (EFI_FFS_FILE_HEADER);
        PadAreaLength = FFS_FILE_SIZE (FileHeader) - HeaderSize;
      }

      PadFileSize = CalculatePadFileSize (
                      FvDevice,
                      (EFI_PHYSICAL_ADDRESS) (UINTN) FileHeader + HeaderSize,
                      Size,
                      RequiredAlignment
                      );
      if (PadAreaLength >= (Size + PadFileSize)) {
        *PadSize      = PadFileSize;
        *PadFileEntry = FileEntry;
        return EFI_SUCCESS;
      }
    }

    FileEntry = (FFS_FILE_LIST_ENTRY *) (FileEntry->Link.ForwardLink);
  }

  return EFI_NOT_FOUND;
}

/**
  Locate a suitable pad file for multiple file writing.

  @param FvDevice          Cached Firmware Volume.
  @param NumOfFiles        The number of Files that needed updating
  @param BufferSize        The array of each file size.
  @param RequiredAlignment The array of of FFS File Data alignment requirement.
  @param PadSize           The array of size of each leading Pad File.
  @param TotalSizeNeeded   The totalsize that can hold these files.
  @param PadFileEntry      Pointer to the Pad File Entry that meets the requirement.

  @retval EFI_SUCCESS     The required pad file is found.
  @retval EFI_NOT_FOUND   The required pad file can't be found.

**/
EFI_STATUS
FvSearchSuitablePadFile (
  IN FV_DEVICE              *FvDevice,
  IN UINTN                  NumOfFiles,
  IN UINTN                  *BufferSize,
  IN UINTN                  *RequiredAlignment,
  OUT UINTN                 *PadSize,
  OUT UINTN                 *TotalSizeNeeded,
  OUT FFS_FILE_LIST_ENTRY   **PadFileEntry
  )
{
  FFS_FILE_LIST_ENTRY *FileEntry;
  EFI_FFS_FILE_STATE  FileState;
  EFI_FFS_FILE_HEADER *FileHeader;
  UINTN               PadAreaLength;
  UINTN               TotalSize;
  UINTN               Index;
  UINTN               HeaderSize;

  FileEntry = (FFS_FILE_LIST_ENTRY *) FvDevice->FfsFileListHeader.ForwardLink;

  //
  // travel through the whole file list to get the pad file entry
  //
  while (FileEntry != (FFS_FILE_LIST_ENTRY *) &FvDevice->FfsFileListHeader) {

    FileHeader  = (EFI_FFS_FILE_HEADER *) FileEntry->FfsHeader;
    FileState   = GetFileState (FvDevice->ErasePolarity, FileHeader);

    if ((FileHeader->Type == EFI_FV_FILETYPE_FFS_PAD) && (FileState == EFI_FILE_DATA_VALID)) {
      //
      // we find one valid pad file, check its length
      //
      if (IS_FFS_FILE2 (FileHeader)) {
        HeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
        PadAreaLength = FFS_FILE2_SIZE (FileHeader) - HeaderSize;
      } else {
        HeaderSize = sizeof (EFI_FFS_FILE_HEADER);
        PadAreaLength = FFS_FILE_SIZE (FileHeader) - HeaderSize;
      }
      TotalSize     = 0;

      for (Index = 0; Index < NumOfFiles; Index++) {
        PadSize[Index] = CalculatePadFileSize (
                      FvDevice,
                      (EFI_PHYSICAL_ADDRESS) (UINTN) FileHeader + HeaderSize + TotalSize,
                      BufferSize[Index],
                      RequiredAlignment[Index]
                      );
        TotalSize += PadSize[Index];
        TotalSize += BufferSize[Index];

        if (TotalSize > PadAreaLength) {
          break;
        }
      }

      if (PadAreaLength >= TotalSize) {
        *PadFileEntry     = FileEntry;
        *TotalSizeNeeded  = TotalSize;
        return EFI_SUCCESS;
      }
    }

    FileEntry = (FFS_FILE_LIST_ENTRY *) (FileEntry->Link.ForwardLink);
  }

  return EFI_NOT_FOUND;
}

/**
  Locate a Free Space entry which can hold these files, including
  meeting the alignment requirements.

  @param FvDevice          Cached Firmware Volume.
  @param NumOfFiles        The number of Files that needed updating
  @param BufferSize        The array of each file size.
  @param RequiredAlignment The array of of FFS File Data alignment requirement.
  @param PadSize           The array of size of each leading Pad File.
  @param TotalSizeNeeded   The got total size that can hold these files.
  @param FreeSpaceEntry    The Free Space Entry that can hold these files.

  @retval EFI_SUCCESS     The free space entry is found.
  @retval EFI_NOT_FOUND   The free space entry can't be found.

**/
EFI_STATUS
FvSearchSuitableFreeSpace (
  IN FV_DEVICE              *FvDevice,
  IN UINTN                  NumOfFiles,
  IN UINTN                  *BufferSize,
  IN UINTN                  *RequiredAlignment,
  OUT UINTN                 *PadSize,
  OUT UINTN                 *TotalSizeNeeded,
  OUT FREE_SPACE_ENTRY      **FreeSpaceEntry
  )
{
  FREE_SPACE_ENTRY  *FreeSpaceListEntry;
  LIST_ENTRY        *Link;
  UINTN             TotalSize;
  UINTN             Index;
  UINT8             *StartAddr;

  Link                = FvDevice->FreeSpaceHeader.ForwardLink;

  FreeSpaceListEntry  = (FREE_SPACE_ENTRY *) Link;

  while ((LIST_ENTRY *) FreeSpaceListEntry != &FvDevice->FreeSpaceHeader) {
    TotalSize = 0;
    StartAddr = FreeSpaceListEntry->StartingAddress;

    //
    // Calculate the totalsize we need
    //
    for (Index = 0; Index < NumOfFiles; Index++) {
      //
      // Perhaps we don't need an EFI_FFS_FILE_HEADER, the first file
      // have had its leading pad file.
      //
      PadSize[Index] = CalculatePadFileSize (
                    FvDevice,
                    (EFI_PHYSICAL_ADDRESS) (UINTN) StartAddr + TotalSize,
                    BufferSize[Index],
                    RequiredAlignment[Index]
                    );

      TotalSize += PadSize[Index];
      TotalSize += BufferSize[Index];

      if (TotalSize > FreeSpaceListEntry->Length) {
        break;
      }
    }

    if (FreeSpaceListEntry->Length >= TotalSize) {
      *FreeSpaceEntry   = FreeSpaceListEntry;
      *TotalSizeNeeded  = TotalSize;
      return EFI_SUCCESS;
    }

    FreeSpaceListEntry = (FREE_SPACE_ENTRY *) FreeSpaceListEntry->Link.ForwardLink;
  }

  return EFI_NOT_FOUND;
}

/**
  Calculate the length of the remaining space in FV.

  @param FvDevice        Cached Firmware Volume
  @param Offset          Current offset to FV base address.
  @param Lba             LBA number for the current offset.
  @param LOffset         Offset in block for the current offset.

  @return the length of remaining space.

**/
UINTN
CalculateRemainingLength (
  IN     FV_DEVICE                            *FvDevice,
  IN     UINTN                                Offset,
  OUT  EFI_LBA                                *Lba,
  OUT  UINTN                                  *LOffset
  )
{
  LIST_ENTRY      *Link;
  LBA_ENTRY       *LbaEntry;
  UINTN           Count;

  Count     = 0;
  *Lba      = 0;
  Link      = FvDevice->LbaHeader.ForwardLink;
  LbaEntry  = (LBA_ENTRY *) Link;

  while (&LbaEntry->Link != &FvDevice->LbaHeader) {
    if (Count > Offset) {
      break;
    }

    Count += LbaEntry->BlockLength;
    (*Lba)++;
    Link      = LbaEntry->Link.ForwardLink;
    LbaEntry  = (LBA_ENTRY *) Link;
  }

  if (Count <= Offset) {
    return 0;
  }

  Link      = LbaEntry->Link.BackLink;
  LbaEntry  = (LBA_ENTRY *) Link;

  (*Lba)--;
  *LOffset  = (UINTN) (LbaEntry->BlockLength - (Count - Offset));

  Count     = 0;
  while (&LbaEntry->Link != &FvDevice->LbaHeader) {

    Count += LbaEntry->BlockLength;

    Link      = LbaEntry->Link.ForwardLink;
    LbaEntry  = (LBA_ENTRY *) Link;
  }

  Count -= *LOffset;

  return Count;
}

/**
  Writes data beginning at Lba:Offset from FV. The write terminates either
  when *NumBytes of data have been written, or when the firmware end is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written.

  @param FvDevice        Cached Firmware Volume
  @param Offset          Offset in the block at which to begin write
  @param NumBytes        At input, indicates the requested write size.
                         At output, indicates the actual number of bytes written.
  @param Buffer          Buffer containing source data for the write.

  @retval EFI_SUCCESS  Data is successfully written into FV.
  @return error        Data is failed written.

**/
EFI_STATUS
FvcWrite (
  IN     FV_DEVICE                            *FvDevice,
  IN     UINTN                                Offset,
  IN OUT UINTN                                *NumBytes,
  IN     UINT8                                *Buffer
  )
{
  EFI_STATUS                          Status;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_LBA                             Lba;
  UINTN                               LOffset;
  EFI_FVB_ATTRIBUTES_2                FvbAttributes;
  UINTN                               RemainingLength;
  UINTN                               WriteLength;
  UINT8                               *TmpBuffer;
  
  LOffset = 0;
  RemainingLength = CalculateRemainingLength (FvDevice, Offset, &Lba, &LOffset);
  if ((UINTN) (*NumBytes) > RemainingLength) {
    *NumBytes = (UINTN) RemainingLength;
    return EFI_INVALID_PARAMETER;
  }

  Fvb = FvDevice->Fvb;

  Status = Fvb->GetAttributes (
                  Fvb,
                  &FvbAttributes
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((FvbAttributes & EFI_FV2_WRITE_STATUS) == 0) {
    return EFI_ACCESS_DENIED;
  }

  RemainingLength = *NumBytes;
  WriteLength     = RemainingLength;
  TmpBuffer       = Buffer;

  do {
    Status = Fvb->Write (
                    Fvb,
                    Lba,
                    LOffset,
                    &WriteLength,
                    TmpBuffer
                    );
    if (!EFI_ERROR (Status)) {
      goto Done;
    }

    if (Status == EFI_BAD_BUFFER_SIZE) {
      Lba++;
      LOffset = 0;
      TmpBuffer += WriteLength;
      RemainingLength -= WriteLength;
      WriteLength = (UINTN) RemainingLength;

      continue;
    } else {
      return Status;
    }
  } while (1);

Done:
  return EFI_SUCCESS;
}

/**
  Create a new FFS file into Firmware Volume device.

  @param FvDevice        Cached Firmware Volume.
  @param FfsFileBuffer   A buffer that holds an FFS file,(it contains
                         a File Header which is in init state).
  @param BufferSize      The size of FfsFileBuffer.
  @param ActualFileSize  The actual file length, it may not be multiples of 8.
  @param FileName        The FFS File Name.
  @param FileType        The FFS File Type.
  @param FileAttributes  The Attributes of the FFS File to be created.

  @retval EFI_SUCCESS           FFS fle is added into FV.
  @retval EFI_INVALID_PARAMETER File type is not valid.
  @retval EFI_DEVICE_ERROR      FV doesn't set writable attribute.
  @retval EFI_NOT_FOUND         FV has no enough space for the added file.

**/
EFI_STATUS
FvCreateNewFile (
  IN FV_DEVICE                *FvDevice,
  IN UINT8                    *FfsFileBuffer,
  IN UINTN                    BufferSize,
  IN UINTN                    ActualFileSize,
  IN EFI_GUID                 *FileName,
  IN EFI_FV_FILETYPE          FileType,
  IN EFI_FV_FILE_ATTRIBUTES   FileAttributes
  )
{
  EFI_STATUS                          Status;
  EFI_FFS_FILE_HEADER                 *FileHeader;
  EFI_PHYSICAL_ADDRESS                BufferPtr;
  UINTN                               Offset;
  UINTN                               NumBytesWritten;
  UINTN                               StateOffset;
  FREE_SPACE_ENTRY                    *FreeSpaceEntry;
  UINTN                               RequiredAlignment;
  UINTN                               PadFileSize;
  FFS_FILE_LIST_ENTRY                 *PadFileEntry;
  EFI_FFS_FILE_ATTRIBUTES             TmpFileAttribute;
  FFS_FILE_LIST_ENTRY                 *FfsFileEntry;
  UINTN                               HeaderSize;

  //
  // File Type: 0x0E~0xE0 are reserved
  //
  if ((FileType > EFI_FV_FILETYPE_SMM_CORE) && (FileType < 0xE0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // First find a free space that can hold this image.
  // Check alignment, FFS at least must be aligned at 8-byte boundry
  //
  RequiredAlignment = GetRequiredAlignment (FileAttributes);

  Status = FvLocateFreeSpaceEntry (
            FvDevice,
            BufferSize,
            RequiredAlignment,
            &PadFileSize,
            &FreeSpaceEntry
            );
  if (EFI_ERROR (Status)) {
    //
    // Maybe we need to find a PAD file that can hold this image
    //
    Status = FvCreateNewFileInsidePadFile (
              FvDevice,
              FfsFileBuffer,
              BufferSize,
              ActualFileSize,
              FileName,
              FileType,
              FileAttributes
              );

    return Status;
  }

  BufferPtr     = (EFI_PHYSICAL_ADDRESS) (UINTN) FreeSpaceEntry->StartingAddress;

  //
  // If we need a leading PAD File, create it first.
  //
  if (PadFileSize != 0) {
    Status = FvCreatePadFileInFreeSpace (
              FvDevice,
              FreeSpaceEntry,
              PadFileSize - sizeof (EFI_FFS_FILE_HEADER),
              &PadFileEntry
              );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  //
  // Maybe we create a pad file, so re-get the free space starting address
  // and length
  //
  BufferPtr     = (EFI_PHYSICAL_ADDRESS) (UINTN) FreeSpaceEntry->StartingAddress;

  //
  // File creation step 1: Allocate File Header,
  // Mark EFI_FILE_HEADER_CONSTRUCTION bit to TRUE,
  // Write Name, IntegrityCheck.Header, Type, Attributes, and Size
  //
  FileHeader = (EFI_FFS_FILE_HEADER *) FfsFileBuffer;
  if (ActualFileSize > 0x00FFFFFF) {
    HeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
  } else {
    HeaderSize = sizeof (EFI_FFS_FILE_HEADER);
  }
  SetFileState (EFI_FILE_HEADER_CONSTRUCTION, FileHeader);

  Offset          = (UINTN) (BufferPtr - FvDevice->CachedFv);
  StateOffset     = Offset + (UINT8 *) &FileHeader->State - (UINT8 *) FileHeader;

  NumBytesWritten = sizeof (EFI_FFS_FILE_STATE);
  Status = FvcWrite (
            FvDevice,
            StateOffset,
            &NumBytesWritten,
            &FileHeader->State
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // update header 2 cache
  //
  CopyMem (
    (UINT8 *) (UINTN) BufferPtr,
    FileHeader,
    HeaderSize
    );

  //
  // update Free Space Entry, now need to substract the file header length
  //
  FreeSpaceEntry->StartingAddress += HeaderSize;
  FreeSpaceEntry->Length -= HeaderSize;

  CopyGuid (&FileHeader->Name, FileName);
  FileHeader->Type = FileType;

  //
  // Convert FvFileAttribute to FfsFileAttributes
  //
  FvFileAttrib2FfsFileAttrib (FileAttributes, &TmpFileAttribute);

  FileHeader->Attributes = TmpFileAttribute;

  //
  // File size is including the FFS File Header.
  //
  if (ActualFileSize > 0x00FFFFFF) {
    ((EFI_FFS_FILE_HEADER2 *) FileHeader)->ExtendedSize = (UINT32) ActualFileSize;
    *(UINT32 *) FileHeader->Size &= 0xFF000000;
    FileHeader->Attributes |= FFS_ATTRIB_LARGE_FILE;
  } else {
    *(UINT32 *) FileHeader->Size &= 0xFF000000;
    *(UINT32 *) FileHeader->Size |= ActualFileSize;
  }

  SetHeaderChecksum (FileHeader);

  Offset          = (UINTN) (BufferPtr - FvDevice->CachedFv);

  NumBytesWritten = HeaderSize;
  Status = FvcWrite (
            FvDevice,
            Offset,
            &NumBytesWritten,
            (UINT8 *) FileHeader
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // update header 2 cache
  //
  CopyMem (
    (UINT8 *) (UINTN) BufferPtr,
    FileHeader,
    HeaderSize
    );

  //
  // end of step 1
  //
  // File creation step 2:
  // MARK EFI_FILE_HEADER_VALID bit to TRUE,
  // Write IntegrityCheck.File, File Data
  //
  SetFileState (EFI_FILE_HEADER_VALID, FileHeader);

  Offset          = (UINTN) (BufferPtr - FvDevice->CachedFv);
  StateOffset     = Offset + (UINT8 *) &FileHeader->State - (UINT8 *) FileHeader;

  NumBytesWritten = sizeof (EFI_FFS_FILE_STATE);
  Status = FvcWrite (
            FvDevice,
            StateOffset,
            &NumBytesWritten,
            &FileHeader->State
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // update header 2 cache
  //
  CopyMem (
    (UINT8 *) (UINTN) BufferPtr,
    FileHeader,
    HeaderSize
    );

  //
  // update Free Space Entry, now need to substract the file data length
  //
  FreeSpaceEntry->StartingAddress += (BufferSize - HeaderSize);
  FreeSpaceEntry->Length -= (BufferSize - HeaderSize);

  //
  // Calculate File Checksum
  //
  SetFileChecksum (FileHeader, ActualFileSize);

  Offset          = (UINTN) (BufferPtr - FvDevice->CachedFv);

  NumBytesWritten = BufferSize;
  Status = FvcWrite (
            FvDevice,
            Offset,
            &NumBytesWritten,
            FfsFileBuffer
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // each time write block successfully, write also to cache
  //
  CopyMem (
    (UINT8 *) (UINTN) BufferPtr,
    FfsFileBuffer,
    NumBytesWritten
    );

  //
  // Step 3: Mark EFI_FILE_DATA_VALID to TRUE
  //
  SetFileState (EFI_FILE_DATA_VALID, FileHeader);

  Offset          = (UINTN) (BufferPtr - FvDevice->CachedFv);
  StateOffset     = Offset + (UINT8 *) &FileHeader->State - (UINT8 *) FileHeader;

  NumBytesWritten = sizeof (EFI_FFS_FILE_STATE);
  Status = FvcWrite (
            FvDevice,
            StateOffset,
            &NumBytesWritten,
            &FileHeader->State
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // update header 2 cache
  //
  CopyMem (
    (UINT8 *) (UINTN) BufferPtr,
    FileHeader,
    HeaderSize
    );

  //
  // If successfully, insert an FfsFileEntry at the end of ffs file list
  //

  FfsFileEntry            = AllocateZeroPool (sizeof (FFS_FILE_LIST_ENTRY));
  ASSERT (FfsFileEntry   != NULL);
  FfsFileEntry->FfsHeader = (UINT8 *) (UINTN) BufferPtr;
  InsertTailList (&FvDevice->FfsFileListHeader, &FfsFileEntry->Link);

  //
  // Set cache file to this file
  //
  FvDevice->CurrentFfsFile = FfsFileEntry;

  return EFI_SUCCESS;
}

/**
  Update a File, so after successful update, there are 2 files existing
  in FV, one is marked for deleted, and another one is valid.

  @param FvDevice          Cached Firmware Volume.
  @param FfsFileBuffer     A buffer that holds an FFS file,(it contains
                           a File Header which is in init state).
  @param BufferSize        The size of FfsFileBuffer.
  @param ActualFileSize    The actual file length, it may not be multiples of 8.
  @param FileName          The FFS File Name.
  @param NewFileType       The FFS File Type.
  @param NewFileAttributes The Attributes of the FFS File to be created.

  @retval EFI_SUCCESS           FFS fle is updated into FV.
  @retval EFI_INVALID_PARAMETER File type is not valid.
  @retval EFI_DEVICE_ERROR      FV doesn't set writable attribute.
  @retval EFI_NOT_FOUND         FV has no enough space for the added file.
                                FFS with same file name is not found in FV.

**/
EFI_STATUS
FvUpdateFile (
  IN FV_DEVICE                *FvDevice,
  IN UINT8                    *FfsFileBuffer,
  IN UINTN                    BufferSize,
  IN UINTN                    ActualFileSize,
  IN EFI_GUID                 *FileName,
  IN EFI_FV_FILETYPE          NewFileType,
  IN EFI_FV_FILE_ATTRIBUTES   NewFileAttributes
  )
{
  EFI_STATUS                          Status;
  EFI_FIRMWARE_VOLUME2_PROTOCOL       *Fv;
  UINTN                               NumBytesWritten;
  EFI_FV_FILETYPE                     OldFileType;
  EFI_FV_FILE_ATTRIBUTES              OldFileAttributes;
  UINTN                               OldFileSize;
  EFI_FFS_FILE_HEADER                 *OldFileHeader;
  UINTN                               OldOffset;
  UINTN                               OldStateOffset;
  FFS_FILE_LIST_ENTRY                 *OldFfsFileEntry;
  UINTN                               Key;
  EFI_GUID                            FileNameGuid;

  Fv  = &FvDevice->Fv;

  //
  // Step 1, find old file,
  // Mark EFI_FILE_MARKED_FOR_UPDATE to TRUE in the older header
  //

  //
  // Check if the file was read last time.
  //
  OldFileHeader   = NULL;
  OldFfsFileEntry = FvDevice->CurrentFfsFile;

  if (OldFfsFileEntry != NULL) {
    OldFileHeader = (EFI_FFS_FILE_HEADER *) OldFfsFileEntry->FfsHeader;
  }

  if ((OldFfsFileEntry == NULL) || (!CompareGuid (&OldFileHeader->Name, FileName))) {
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
        return Status;
      }
    } while (!CompareGuid (&FileNameGuid, FileName));

    //
    // Get FfsFileEntry from the search key
    //
    OldFfsFileEntry = (FFS_FILE_LIST_ENTRY *) Key;

    //
    // Double check file state before being ready to be removed
    //
    OldFileHeader = (EFI_FFS_FILE_HEADER *) OldFfsFileEntry->FfsHeader;
  } else {
    //
    // Mark the cache file to invalid
    //
    FvDevice->CurrentFfsFile = NULL;
  }
  //
  // Update File: Mark EFI_FILE_MARKED_FOR_UPDATE to TRUE
  //
  SetFileState (EFI_FILE_MARKED_FOR_UPDATE, OldFileHeader);

  OldOffset       = (UINTN) ((EFI_PHYSICAL_ADDRESS) (UINTN) OldFileHeader - FvDevice->CachedFv);
  OldStateOffset  = OldOffset + (UINT8 *) &OldFileHeader->State - (UINT8 *) OldFileHeader;

  NumBytesWritten = sizeof (EFI_FFS_FILE_STATE);
  Status = FvcWrite (
            FvDevice,
            OldStateOffset,
            &NumBytesWritten,
            &OldFileHeader->State
            );
  if (EFI_ERROR (Status)) {
    //
    // if failed, write the bit back in the cache, its XOR operation.
    //
    SetFileState (EFI_FILE_MARKED_FOR_UPDATE, OldFileHeader);

    return Status;
  }

  //
  // Step 2, Create New Files
  //
  Status = FvCreateNewFile (
            FvDevice,
            FfsFileBuffer,
            BufferSize,
            ActualFileSize,
            FileName,
            NewFileType,
            NewFileAttributes
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If successfully, remove this file entry,
  // although delete file may fail.
  //
  (OldFfsFileEntry->Link.BackLink)->ForwardLink = OldFfsFileEntry->Link.ForwardLink;
  (OldFfsFileEntry->Link.ForwardLink)->BackLink = OldFfsFileEntry->Link.BackLink;
  FreePool (OldFfsFileEntry);

  //
  // Step 3: Delete old files,
  // by marking EFI_FILE_DELETED to TRUE
  //
  SetFileState (EFI_FILE_DELETED, OldFileHeader);

  OldOffset       = (UINTN) ((EFI_PHYSICAL_ADDRESS) (UINTN) OldFileHeader - FvDevice->CachedFv);
  OldStateOffset  = OldOffset + (UINT8 *) &OldFileHeader->State - (UINT8 *) OldFileHeader;

  NumBytesWritten = sizeof (EFI_FFS_FILE_STATE);
  Status = FvcWrite (
            FvDevice,
            OldStateOffset,
            &NumBytesWritten,
            &OldFileHeader->State
            );
  if (EFI_ERROR (Status)) {
    //
    // if failed, write the bit back in the cache, its XOR operation.
    //
    SetFileState (EFI_FILE_DELETED, OldFileHeader);

    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Deleted a given file from FV device.

  @param FvDevice        Cached Firmware Volume.
  @param NameGuid        The FFS File Name.

  @retval EFI_SUCCESS    FFS file with the specified FFS name is removed.
  @retval EFI_NOT_FOUND  FFS file with the specified FFS name is not found.

**/
EFI_STATUS
FvDeleteFile (
  IN FV_DEVICE  *FvDevice,
  IN EFI_GUID   *NameGuid
  )
{
  EFI_STATUS                          Status;
  UINTN                               Key;
  EFI_GUID                            FileNameGuid;
  EFI_FV_FILETYPE                     FileType;
  EFI_FV_FILE_ATTRIBUTES              FileAttributes;
  UINTN                               FileSize;
  EFI_FFS_FILE_HEADER                 *FileHeader;
  FFS_FILE_LIST_ENTRY                 *FfsFileEntry;
  EFI_FFS_FILE_STATE                  FileState;
  EFI_FIRMWARE_VOLUME2_PROTOCOL        *Fv;
  UINTN                               Offset;
  UINTN                               StateOffset;
  UINTN                               NumBytesWritten;

  Fv  = &FvDevice->Fv;

  //
  // Check if the file was read last time.
  //
  FileHeader    = NULL;
  FfsFileEntry  = FvDevice->CurrentFfsFile;

  if (FfsFileEntry != NULL) {
    FileHeader = (EFI_FFS_FILE_HEADER *) FfsFileEntry->FfsHeader;
  }

  if ((FfsFileEntry == NULL) || (!CompareGuid (&FileHeader->Name, NameGuid))) {
    //
    // Next search for the file using GetNextFile
    //
    Key = 0;
    do {
      FileType = 0;
      Status = Fv->GetNextFile (
                    Fv,
                    &Key,
                    &FileType,
                    &FileNameGuid,
                    &FileAttributes,
                    &FileSize
                    );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    } while (!CompareGuid (&FileNameGuid, NameGuid));

    //
    // Get FfsFileEntry from the search key
    //
    FfsFileEntry = (FFS_FILE_LIST_ENTRY *) Key;

    //
    // Double check file state before being ready to be removed
    //
    FileHeader = (EFI_FFS_FILE_HEADER *) FfsFileEntry->FfsHeader;
  } else {
    //
    // Mark the cache file to NULL
    //
    FvDevice->CurrentFfsFile = NULL;
  }

  FileState = GetFileState (FvDevice->ErasePolarity, FileHeader);

  if (FileState == EFI_FILE_HEADER_INVALID) {
    return EFI_NOT_FOUND;
  }

  if (FileState == EFI_FILE_DELETED) {
    return EFI_NOT_FOUND;
  }
  //
  // Delete File: Mark EFI_FILE_DELETED to TRUE
  //
  SetFileState (EFI_FILE_DELETED, FileHeader);

  Offset          = (UINTN) ((EFI_PHYSICAL_ADDRESS) (UINTN) FileHeader - FvDevice->CachedFv);
  StateOffset     = Offset + (UINT8 *) &FileHeader->State - (UINT8 *) FileHeader;

  NumBytesWritten = sizeof (EFI_FFS_FILE_STATE);
  Status = FvcWrite (
            FvDevice,
            StateOffset,
            &NumBytesWritten,
            &FileHeader->State
            );
  if (EFI_ERROR (Status)) {
    //
    // if failed, write the bit back in the cache, its XOR operation.
    //
    SetFileState (EFI_FILE_DELETED, FileHeader);

    return Status;
  }
  //
  // If successfully, remove this file entry
  //
  FvDevice->CurrentFfsFile                    = NULL;

  (FfsFileEntry->Link.BackLink)->ForwardLink  = FfsFileEntry->Link.ForwardLink;
  (FfsFileEntry->Link.ForwardLink)->BackLink  = FfsFileEntry->Link.BackLink;
  FreePool (FfsFileEntry);

  return EFI_SUCCESS;
}

/**
  Writes one or more files to the firmware volume.

  @param  This                   Indicates the calling context.
  @param  NumberOfFiles          Number of files.
  @param  WritePolicy            WritePolicy indicates the level of reliability
                                 for the write in the event of a power failure or
                                 other system failure during the write operation.
  @param  FileData               FileData is an pointer to an array of
                                 EFI_FV_WRITE_DATA. Each element of array
                                 FileData represents a file to be written.

  @retval EFI_SUCCESS            Files successfully written to firmware volume
  @retval EFI_OUT_OF_RESOURCES   Not enough buffer to be allocated.
  @retval EFI_DEVICE_ERROR       Device error.
  @retval EFI_WRITE_PROTECTED    Write protected.
  @retval EFI_NOT_FOUND          Not found.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_UNSUPPORTED        This function not supported.

**/
EFI_STATUS
EFIAPI
FvWriteFile (
  IN CONST EFI_FIRMWARE_VOLUME2_PROTOCOL   *This,
  IN UINT32                         NumberOfFiles,
  IN EFI_FV_WRITE_POLICY            WritePolicy,
  IN EFI_FV_WRITE_FILE_DATA         *FileData
  )
{
  EFI_STATUS                          Status;
  UINTN                               Index1;
  UINTN                               Index2;
  UINT8                               *FileBuffer;
  UINTN                               BufferSize;
  UINTN                               ActualSize;
  UINT8                               ErasePolarity;
  FV_DEVICE                           *FvDevice;
  EFI_FV_FILETYPE                     FileType;
  EFI_FV_FILE_ATTRIBUTES              FileAttributes;
  UINTN                               Size;
  BOOLEAN                             CreateNewFile[MAX_FILES];
  UINTN                               NumDelete;
  EFI_FV_ATTRIBUTES                   FvAttributes;
  UINT32                              AuthenticationStatus;
  UINTN                               HeaderSize;

  if (NumberOfFiles > MAX_FILES) {
    return EFI_UNSUPPORTED;
  }

  Status = EFI_SUCCESS;

  SetMem (CreateNewFile, NumberOfFiles, TRUE);

  FvDevice  = FV_DEVICE_FROM_THIS (This);

  //
  // First check the volume attributes.
  //
  Status = This->GetVolumeAttributes (
                  This,
                  &FvAttributes
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Can we have write right?
  //
  if ((FvAttributes & EFI_FV2_WRITE_STATUS) == 0) {
    return EFI_WRITE_PROTECTED;
  }

  ErasePolarity = FvDevice->ErasePolarity;

  //
  // Loop for all files
  //
  NumDelete = 0;
  for (Index1 = 0; Index1 < NumberOfFiles; Index1++) {

    if ((FileData[Index1].BufferSize + sizeof (EFI_FFS_FILE_HEADER) > 0x00FFFFFF) && !FvDevice->IsFfs3Fv) {
      //
      // Found a file needs a FFS3 formatted file to store it, but it is in a non-FFS3 formatted FV.
      //
      DEBUG ((EFI_D_ERROR, "FFS3 formatted file can't be written in a non-FFS3 formatted FV.\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (FileData[Index1].BufferSize == 0) {
      //
      // Here we will delete this file
      //
      Status = This->ReadFile (
                      This,
                      FileData[Index1].NameGuid,
                      NULL,
                      &Size,
                      &FileType,
                      &FileAttributes,
                      &AuthenticationStatus
                      );
      if (!EFI_ERROR (Status)) {
        NumDelete++;
      } else {
        return Status;
      }
    }

    if (FileData[Index1].Type == EFI_FV_FILETYPE_FFS_PAD) {
      //
      // According to PI spec, on EFI_FV_FILETYPE_FFS_PAD: 
      // "Standard firmware file system services will not return the handle of any pad files, 
      // nor will they permit explicit creation of such files."
      //
      return EFI_INVALID_PARAMETER;
    }
  }

  if ((NumDelete != NumberOfFiles) && (NumDelete != 0)) {
    //
    // A delete was request with a multiple file write
    //
    return EFI_INVALID_PARAMETER;
  }

  if (NumDelete == NumberOfFiles) {
    for (Index1 = 0; Index1 < NumberOfFiles; Index1++) {
      //
      // Delete Files
      //
      Status = FvDeleteFile (FvDevice, FileData[Index1].NameGuid);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    return EFI_SUCCESS;
  }

  for (Index1 = 0; Index1 < NumberOfFiles; Index1++) {
    Status = This->ReadFile (
                    This,
                    FileData[Index1].NameGuid,
                    NULL,
                    &Size,
                    &FileType,
                    &FileAttributes,
                    &AuthenticationStatus
                    );
    if (!EFI_ERROR (Status)) {
      CreateNewFile[Index1] = FALSE;
    } else if (Status == EFI_NOT_FOUND) {
      CreateNewFile[Index1] = TRUE;
    } else {
      return Status;
    }
    //
    // Checking alignment
    //
    if ((FileData[Index1].FileAttributes & EFI_FV_FILE_ATTRIB_ALIGNMENT) != 0) {
      UINT8 FFSAlignmentValue;
      UINT8 FvAlignmentValue;

      FFSAlignmentValue = (UINT8) (FileData[Index1].FileAttributes & EFI_FV_FILE_ATTRIB_ALIGNMENT);
      FvAlignmentValue = (UINT8) (((UINT32) (FvAttributes & EFI_FV2_ALIGNMENT)) >> 16);

      if (FFSAlignmentValue > FvAlignmentValue) {
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  if ((WritePolicy != EFI_FV_RELIABLE_WRITE) && (WritePolicy != EFI_FV_UNRELIABLE_WRITE)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Checking the reliable write is supported by FV
  //

  if ((WritePolicy == EFI_FV_RELIABLE_WRITE) && (NumberOfFiles > 1)) {
    //
    // Only for multiple files, reliable write is meaningful
    //
    Status = FvCreateMultipleFiles (
              FvDevice,
              NumberOfFiles,
              FileData,
              CreateNewFile
              );

    return Status;
  }

  for (Index1 = 0; Index1 < NumberOfFiles; Index1++) {
    //
    // Making Buffersize QWORD boundry, and add file tail.
    //
    HeaderSize = sizeof (EFI_FFS_FILE_HEADER);
    ActualSize = FileData[Index1].BufferSize + HeaderSize;
    if (ActualSize > 0x00FFFFFF) {
      HeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
      ActualSize = FileData[Index1].BufferSize + HeaderSize;
    }
    BufferSize  = ActualSize;

    while ((BufferSize & 0x07) != 0) {
      BufferSize++;
    }

    FileBuffer = AllocateZeroPool (BufferSize);
    if (FileBuffer == NULL) {
      return Status;
    }
    //
    // Copy File Data into FileBuffer
    //
    CopyMem (
      FileBuffer + HeaderSize,
      FileData[Index1].Buffer,
      FileData[Index1].BufferSize
      );

    if (ErasePolarity == 1) {
      //
      // Fill the file header and padding byte with Erase Byte
      //
      for (Index2 = 0; Index2 < HeaderSize; Index2++) {
        FileBuffer[Index2] = (UINT8)~FileBuffer[Index2];
      }

      for (Index2 = ActualSize; Index2 < BufferSize; Index2++) {
        FileBuffer[Index2] = (UINT8)~FileBuffer[Index2];
      }
    }

    if (CreateNewFile[Index1]) {
      Status = FvCreateNewFile (
                FvDevice,
                FileBuffer,
                BufferSize,
                ActualSize,
                FileData[Index1].NameGuid,
                FileData[Index1].Type,
                FileData[Index1].FileAttributes
                );
    } else {
      Status = FvUpdateFile (
                FvDevice,
                FileBuffer,
                BufferSize,
                ActualSize,
                FileData[Index1].NameGuid,
                FileData[Index1].Type,
                FileData[Index1].FileAttributes
                );
    }

    FreePool (FileBuffer);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}
