/** @file
  FFS file access utilities.

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

#define PHYSICAL_ADDRESS_TO_POINTER(Address)  ((VOID *) ((UINTN) Address))

/**
  Set File State in the FfsHeader.

  @param  State          File state to be set into FFS header.
  @param  FfsHeader      Points to the FFS file header

**/
VOID
SetFileState (
  IN UINT8                State,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  //
  // Set File State in the FfsHeader
  //
  FfsHeader->State = (EFI_FFS_FILE_STATE) (FfsHeader->State ^ State);
  return ;
}

/**
  Get the FFS file state by checking the highest bit set in the header's state field.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  FfsHeader      Points to the FFS file header

  @return FFS File state

**/
EFI_FFS_FILE_STATE
GetFileState (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  EFI_FFS_FILE_STATE  FileState;
  UINT8               HighestBit;

  FileState = FfsHeader->State;

  if (ErasePolarity != 0) {
    FileState = (EFI_FFS_FILE_STATE)~FileState;
  }

  HighestBit = 0x80;
  while (HighestBit != 0 && ((HighestBit & FileState) == 0)) {
    HighestBit >>= 1;
  }

  return (EFI_FFS_FILE_STATE) HighestBit;
}

/**
  Convert the Buffer Address to LBA Entry Address.

  @param FvDevice        Cached FvDevice
  @param BufferAddress   Address of Buffer
  @param LbaListEntry    Pointer to the got LBA entry that contains the address.

  @retval EFI_NOT_FOUND  Buffer address is out of FvDevice.
  @retval EFI_SUCCESS    LBA entry is found for Buffer address.

**/
EFI_STATUS
Buffer2LbaEntry (
  IN     FV_DEVICE              *FvDevice,
  IN     EFI_PHYSICAL_ADDRESS   BufferAddress,
  OUT LBA_ENTRY                 **LbaListEntry
  )
{
  LBA_ENTRY   *LbaEntry;
  LIST_ENTRY  *Link;

  Link      = FvDevice->LbaHeader.ForwardLink;
  LbaEntry  = (LBA_ENTRY *) Link;

  //
  // Locate LBA which contains the address
  //
  while (&LbaEntry->Link != &FvDevice->LbaHeader) {
    if ((EFI_PHYSICAL_ADDRESS) (UINTN) (LbaEntry->StartingAddress) > BufferAddress) {
      break;
    }

    Link      = LbaEntry->Link.ForwardLink;
    LbaEntry  = (LBA_ENTRY *) Link;
  }

  if (&LbaEntry->Link == &FvDevice->LbaHeader) {
    return EFI_NOT_FOUND;
  }

  Link      = LbaEntry->Link.BackLink;
  LbaEntry  = (LBA_ENTRY *) Link;

  if (&LbaEntry->Link == &FvDevice->LbaHeader) {
    return EFI_NOT_FOUND;
  }

  *LbaListEntry = LbaEntry;

  return EFI_SUCCESS;
}

/**
  Convert the Buffer Address to LBA Address & Offset.

  @param FvDevice        Cached FvDevice
  @param BufferAddress   Address of Buffer
  @param Lba             Pointer to the gob Lba value
  @param Offset          Pointer to the got Offset

  @retval EFI_NOT_FOUND  Buffer address is out of FvDevice.
  @retval EFI_SUCCESS    LBA and Offset is found for Buffer address.

**/
EFI_STATUS
Buffer2Lba (
  IN     FV_DEVICE              *FvDevice,
  IN     EFI_PHYSICAL_ADDRESS   BufferAddress,
  OUT EFI_LBA                   *Lba,
  OUT UINTN                     *Offset
  )
{
  LBA_ENTRY   *LbaEntry;
  EFI_STATUS  Status;

  LbaEntry = NULL;

  Status = Buffer2LbaEntry (
            FvDevice,
            BufferAddress,
            &LbaEntry
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Lba    = LbaEntry->LbaIndex;
  *Offset = (UINTN) BufferAddress - (UINTN) LbaEntry->StartingAddress;

  return EFI_SUCCESS;
}

/**
  Check if a block of buffer is erased.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  Buffer         The buffer to be checked
  @param  BufferSize     Size of the buffer in bytes

  @retval TRUE           The block of buffer is erased
  @retval FALSE          The block of buffer is not erased

**/
BOOLEAN
IsBufferErased (
  IN UINT8    ErasePolarity,
  IN UINT8    *Buffer,
  IN UINTN    BufferSize
  )
{
  UINTN Count;
  UINT8 EraseByte;

  if (ErasePolarity == 1) {
    EraseByte = 0xFF;
  } else {
    EraseByte = 0;
  }

  for (Count = 0; Count < BufferSize; Count++) {
    if (Buffer[Count] != EraseByte) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Verify checksum of the firmware volume header.

  @param  FvHeader       Points to the firmware volume header to be checked

  @retval TRUE           Checksum verification passed
  @retval FALSE          Checksum verification failed

**/
BOOLEAN
VerifyFvHeaderChecksum (
  IN EFI_FIRMWARE_VOLUME_HEADER *FvHeader
  )
{
  UINT16  Checksum;

  Checksum = CalculateSum16 ((UINT16 *) FvHeader, FvHeader->HeaderLength);

  if (Checksum == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Verify checksum of the FFS file header.

  @param  FfsHeader      Points to the FFS file header to be checked

  @retval TRUE           Checksum verification passed
  @retval FALSE          Checksum verification failed

**/
BOOLEAN
VerifyHeaderChecksum (
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  UINT8 HeaderChecksum;

  if (IS_FFS_FILE2 (FfsHeader)) {
    HeaderChecksum = CalculateSum8 ((UINT8 *) FfsHeader, sizeof (EFI_FFS_FILE_HEADER2));
  } else {
    HeaderChecksum = CalculateSum8 ((UINT8 *) FfsHeader, sizeof (EFI_FFS_FILE_HEADER));
  }
  HeaderChecksum = (UINT8) (HeaderChecksum - FfsHeader->State - FfsHeader->IntegrityCheck.Checksum.File);

  if (HeaderChecksum == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Verify checksum of the FFS file data.

  @param  FfsHeader      Points to the FFS file header to be checked

  @retval TRUE           Checksum verification passed
  @retval FALSE          Checksum verification failed

**/
BOOLEAN
VerifyFileChecksum (
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  UINT8                   FileChecksum;
  EFI_FV_FILE_ATTRIBUTES  Attributes;

  Attributes = FfsHeader->Attributes;

  if ((Attributes & FFS_ATTRIB_CHECKSUM) != 0) {

    //
    // Check checksum of FFS data
    //
    if (IS_FFS_FILE2 (FfsHeader)) {
      FileChecksum = CalculateSum8 ((UINT8 *) FfsHeader + sizeof (EFI_FFS_FILE_HEADER2), FFS_FILE2_SIZE (FfsHeader) - sizeof (EFI_FFS_FILE_HEADER2));
    } else {
      FileChecksum = CalculateSum8 ((UINT8 *) FfsHeader + sizeof (EFI_FFS_FILE_HEADER), FFS_FILE_SIZE (FfsHeader) - sizeof (EFI_FFS_FILE_HEADER));
    }
    FileChecksum = (UINT8) (FileChecksum + FfsHeader->IntegrityCheck.Checksum.File);

    if (FileChecksum == 0) {
      return TRUE;
    } else {
      return FALSE;
    }

  } else {

    if (FfsHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM) {
      return FALSE;
    } else {
      return TRUE;
    }
  }

}

/**
  Check if it's a valid FFS file header.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  FfsHeader      Points to the FFS file header to be checked

  @retval TRUE           Valid FFS file header
  @retval FALSE          Invalid FFS file header

**/
BOOLEAN
IsValidFFSHeader (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  EFI_FFS_FILE_STATE  FileState;

  //
  // Check if it is a free space
  //
  if (IsBufferErased (
        ErasePolarity,
        (UINT8 *) FfsHeader,
        sizeof (EFI_FFS_FILE_HEADER)
        )) {
    return FALSE;
  }

  FileState = GetFileState (ErasePolarity, FfsHeader);

  switch (FileState) {
  case EFI_FILE_HEADER_CONSTRUCTION:
    //
    // fall through
    //
  case EFI_FILE_HEADER_INVALID:
    return FALSE;

  case EFI_FILE_HEADER_VALID:
    //
    // fall through
    //
  case EFI_FILE_DATA_VALID:
    //
    // fall through
    //
  case EFI_FILE_MARKED_FOR_UPDATE:
    //
    // fall through
    //
  case EFI_FILE_DELETED:
    //
    // Here we need to verify header checksum
    //
    if (!VerifyHeaderChecksum (FfsHeader)) {
      return FALSE;
    }
    break;

  default:
    //
    // return
    //
    return FALSE;
  }

  return TRUE;
}

/**
  Get next possible of Firmware File System Header.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  FfsHeader      Points to the FFS file header to be skipped.

  @return  Pointer to next FFS header.

**/
EFI_PHYSICAL_ADDRESS
GetNextPossibleFileHeader (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  UINT32  FileLength;
  UINT32  SkipLength;

  if (!IsValidFFSHeader (ErasePolarity, FfsHeader)) {
    //
    // Skip this header
    //
    if (IS_FFS_FILE2 (FfsHeader)) {
      return (EFI_PHYSICAL_ADDRESS) (UINTN) FfsHeader + sizeof (EFI_FFS_FILE_HEADER2);
    } else {
      return (EFI_PHYSICAL_ADDRESS) (UINTN) FfsHeader + sizeof (EFI_FFS_FILE_HEADER);
    }
  }

  if (IS_FFS_FILE2 (FfsHeader)) {
    FileLength = FFS_FILE2_SIZE (FfsHeader);
  } else {
    FileLength = FFS_FILE_SIZE (FfsHeader);
  }

  //
  // Since FileLength is not multiple of 8, we need skip some bytes
  // to get next possible header
  //
  SkipLength = FileLength;
  while ((SkipLength & 0x07) != 0) {
    SkipLength++;
  }

  return (EFI_PHYSICAL_ADDRESS) (UINTN) FfsHeader + SkipLength;
}

/**
  Search FFS file with the same FFS name in FV Cache.

  @param  FvDevice     Cached FV image.
  @param  FfsHeader    Points to the FFS file header to be skipped.
  @param  StateBit     FFS file state bit to be checked.

  @return  Pointer to next found FFS header. NULL will return if no found.

**/
EFI_FFS_FILE_HEADER *
DuplicateFileExist (
  IN FV_DEVICE            *FvDevice,
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  IN EFI_FFS_FILE_STATE   StateBit
  )
{
  UINT8               *Ptr;
  EFI_FFS_FILE_HEADER *NextFfsFile;

  //
  // Search duplicate file, not from the beginning of FV,
  // just search the next ocurrence of this file
  //
  NextFfsFile = FfsHeader;

  do {
    Ptr = (UINT8 *) PHYSICAL_ADDRESS_TO_POINTER (
                      GetNextPossibleFileHeader (FvDevice->ErasePolarity,
                      NextFfsFile)
                      );
    NextFfsFile = (EFI_FFS_FILE_HEADER *) Ptr;

    if ((UINT8 *) PHYSICAL_ADDRESS_TO_POINTER (FvDevice->CachedFv) + FvDevice->FwVolHeader->FvLength - Ptr <
        sizeof (EFI_FFS_FILE_HEADER)
          ) {
      break;
    }

    if (!IsValidFFSHeader (FvDevice->ErasePolarity, NextFfsFile)) {
      continue;
    }

    if (!VerifyFileChecksum (NextFfsFile)) {
      continue;
    }

    if (CompareGuid (&NextFfsFile->Name, &FfsHeader->Name)) {
      if (GetFileState (FvDevice->ErasePolarity, NextFfsFile) == StateBit) {
        return NextFfsFile;
      }
    }
  } while (Ptr < (UINT8 *) PHYSICAL_ADDRESS_TO_POINTER (FvDevice->CachedFv) + FvDevice->FwVolHeader->FvLength);

  return NULL;
}

/**
  Change FFS file header state and write to FV.

  @param  FvDevice     Cached FV image.
  @param  FfsHeader    Points to the FFS file header to be updated.
  @param  State        FFS file state to be set.

  @retval EFI_SUCCESS  File state is writen into FV.
  @retval others       File state can't be writen into FV.

**/
EFI_STATUS
UpdateHeaderBit (
  IN FV_DEVICE            *FvDevice,
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  IN EFI_FFS_FILE_STATE   State
  )
{
  EFI_STATUS  Status;
  EFI_LBA     Lba;
  UINTN       Offset;
  UINTN       NumBytesWritten;

  Lba    = 0;
  Offset = 0;

  SetFileState (State, FfsHeader);

  Buffer2Lba (
    FvDevice,
    (EFI_PHYSICAL_ADDRESS) (UINTN) (&FfsHeader->State),
    &Lba,
    &Offset
    );
  //
  // Write the state byte into FV
  //
  NumBytesWritten = sizeof (EFI_FFS_FILE_STATE);
  Status = FvDevice->Fvb->Write (
                            FvDevice->Fvb,
                            Lba,
                            Offset,
                            &NumBytesWritten,
                            &FfsHeader->State
                            );
  return Status;
}

/**
  Check if it's a valid FFS file.
  Here we are sure that it has a valid FFS file header since we must call IsValidFfsHeader() first.

  @param  FvDevice       Cached FV image.
  @param  FfsHeader      Points to the FFS file to be checked

  @retval TRUE           Valid FFS file
  @retval FALSE          Invalid FFS file

**/
BOOLEAN
IsValidFFSFile (
  IN FV_DEVICE            *FvDevice,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  EFI_FFS_FILE_STATE  FileState;
  UINT8               ErasePolarity;

  ErasePolarity = FvDevice->ErasePolarity;

  FileState     = GetFileState (ErasePolarity, FfsHeader);

  switch (FileState) {
  case EFI_FILE_DATA_VALID:
    if (!VerifyFileChecksum (FfsHeader)) {
      return FALSE;
    }

    if (FfsHeader->Type == EFI_FV_FILETYPE_FFS_PAD) {
      break;
    }
    //
    // Check if there is another duplicated file with the EFI_FILE_DATA_VALID
    //
    if (DuplicateFileExist (FvDevice, FfsHeader, EFI_FILE_DATA_VALID) != NULL) {
      return FALSE;
    }

    break;

  case EFI_FILE_MARKED_FOR_UPDATE:
    if (!VerifyFileChecksum (FfsHeader)) {
      return FALSE;
    }

    if (FfsHeader->Type == EFI_FV_FILETYPE_FFS_PAD) {
      //
      // since its data area is not unperturbed, it cannot be reclaimed,
      // marked it as deleted
      //
      UpdateHeaderBit (FvDevice, FfsHeader, EFI_FILE_DELETED);
      return TRUE;

    } else if (DuplicateFileExist (FvDevice, FfsHeader, EFI_FILE_DATA_VALID) != NULL) {
      //
      // Here the found file is more recent than this file,
      // mark it as deleted
      //
      UpdateHeaderBit (FvDevice, FfsHeader, EFI_FILE_DELETED);
      return TRUE;

    } else {
      return TRUE;
    }

    break;

  case EFI_FILE_DELETED:
    if (!VerifyFileChecksum (FfsHeader)) {
      return FALSE;
    }

    break;

  default:
    return FALSE;
  }

  return TRUE;
}

