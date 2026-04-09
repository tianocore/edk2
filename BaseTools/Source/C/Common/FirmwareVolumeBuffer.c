/** @file
EFI Firmware Volume routines which work on a Fv image in buffers.

Copyright (c) 1999 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FirmwareVolumeBufferLib.h"
#include "BinderFuncs.h"

//
// Local macros
//
#define EFI_TEST_FFS_ATTRIBUTES_BIT(FvbAttributes, TestAttributes, Bit) \
    ( \
      (BOOLEAN) ( \
          (FvbAttributes & EFI_FVB2_ERASE_POLARITY) ? (((~TestAttributes) & Bit) == Bit) : ((TestAttributes & Bit) == Bit) \
        ) \
    )


//
// Local prototypes
//

STATIC
UINT32
FvBufGetSecHdrLen(
   IN EFI_COMMON_SECTION_HEADER *SectionHeader
   )
{
  if (SectionHeader == NULL) {
    return 0;
  }
  if (FvBufExpand3ByteSize(SectionHeader->Size) == 0xffffff) {
    return sizeof(EFI_COMMON_SECTION_HEADER2);
  }
  return sizeof(EFI_COMMON_SECTION_HEADER);
}

STATIC
UINT32
FvBufGetSecFileLen (
  IN EFI_COMMON_SECTION_HEADER *SectionHeader
  )
{
  UINT32 Length;
  if (SectionHeader == NULL) {
    return 0;
  }
  Length = FvBufExpand3ByteSize(SectionHeader->Size);
  if (Length == 0xffffff) {
    Length = ((EFI_COMMON_SECTION_HEADER2 *)SectionHeader)->ExtendedSize;
  }
  return Length;
}

//
// Local prototypes
//

STATIC
UINT16
FvBufCalculateChecksum16 (
  IN UINT16       *Buffer,
  IN UINTN        Size
  );

STATIC
UINT8
FvBufCalculateChecksum8 (
  IN UINT8        *Buffer,
  IN UINTN        Size
  );

//
// Procedures start
//

/**
  Clears out all files from the Fv buffer in memory

  @param SourceFv Address of the Fv in memory, this firmware volume will
                  be modified, if SourceFfsFile exists
  @param SourceFfsFile Input FFS file to replace

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
**/
EFI_STATUS
FvBufRemoveFileNew (
  IN OUT VOID *Fv,
  IN EFI_GUID *Name
  )
{
  EFI_STATUS                  Status;
  EFI_FFS_FILE_HEADER*        FileToRm;
  UINTN                       FileToRmLength;

  Status = FvBufFindFileByName(
    Fv,
    Name,
    (VOID **)&FileToRm
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FileToRmLength = FvBufGetFfsFileSize (FileToRm);

  CommonLibBinderSetMem (
    FileToRm,
    FileToRmLength,
    (((EFI_FIRMWARE_VOLUME_HEADER*)Fv)->Attributes & EFI_FVB2_ERASE_POLARITY)
      ? 0xFF : 0
    );

  return EFI_SUCCESS;
}

/**
  Clears out all files from the Fv buffer in memory

  @param SourceFv Address of the Fv in memory, this firmware volume will
                  be modified, if SourceFfsFile exists
  @param SourceFfsFile Input FFS file to replace

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
**/
EFI_STATUS
FvBufRemoveFile (
  IN OUT VOID *Fv,
  IN EFI_GUID *Name
  )
{
  EFI_STATUS                  Status;
  EFI_FFS_FILE_HEADER        *NextFile;
  EFI_FIRMWARE_VOLUME_HEADER *TempFv;
  UINTN                       FileKey;
  UINTN                       FvLength;

  Status = FvBufFindFileByName(
    Fv,
    Name,
    NULL
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FvBufGetSize (Fv, &FvLength);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TempFv = NULL;
  Status = FvBufDuplicate (Fv, (VOID **)&TempFv);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FvBufClearAllFiles (TempFv);
  if (EFI_ERROR (Status)) {
    CommonLibBinderFree (TempFv);
    return Status;
  }

  // TempFv has been allocated.  It must now be freed
  // before returning.

  FileKey = 0;
  while (TRUE) {

    Status = FvBufFindNextFile (Fv, &FileKey, (VOID **)&NextFile);
    if (Status == EFI_NOT_FOUND) {
      break;
    } else if (EFI_ERROR (Status)) {
      CommonLibBinderFree (TempFv);
      return Status;
    }

    if (CommonLibBinderCompareGuid (Name, &NextFile->Name)) {
      continue;
    }
    else {
      Status = FvBufAddFile (TempFv, NextFile);
      if (EFI_ERROR (Status)) {
        CommonLibBinderFree (TempFv);
        return Status;
      }
    }
  }

  CommonLibBinderCopyMem (Fv, TempFv, FvLength);
  CommonLibBinderFree (TempFv);

  return EFI_SUCCESS;
}

/**
  Clears out all files from the Fv buffer in memory

  @param SourceFfsFile Input FFS file to update the checksum for

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
**/
EFI_STATUS
FvBufChecksumFile (
  IN OUT VOID *FfsFile
  )
{
  EFI_FFS_FILE_HEADER* File = (EFI_FFS_FILE_HEADER*)FfsFile;
  EFI_FFS_FILE_STATE StateBackup;
  UINT32 FileSize;

  FileSize = FvBufGetFfsFileSize (File);

  //
  // Fill in checksums and state, they must be 0 for checksumming.
  //
  File->IntegrityCheck.Checksum.Header = 0;
  File->IntegrityCheck.Checksum.File = 0;
  StateBackup = File->State;
  File->State = 0;

  File->IntegrityCheck.Checksum.Header =
    FvBufCalculateChecksum8 (
      (UINT8 *) File,
      FvBufGetFfsHeaderSize (File)
      );

  if (File->Attributes & FFS_ATTRIB_CHECKSUM) {
    File->IntegrityCheck.Checksum.File = FvBufCalculateChecksum8 (
                                                (VOID*)((UINT8 *)File + FvBufGetFfsHeaderSize (File)),
                                                FileSize - FvBufGetFfsHeaderSize (File)
                                                );
  } else {
    File->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
  }

  File->State = StateBackup;

  return EFI_SUCCESS;
}

/**
  Clears out all files from the Fv buffer in memory

  @param SourceFv Address of the Fv in memory, this firmware volume will
                  be modified, if SourceFfsFile exists
  @param SourceFfsFile Input FFS file to replace

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
**/
EFI_STATUS
FvBufChecksumHeader (
  IN OUT VOID *Fv
  )
{
  EFI_FIRMWARE_VOLUME_HEADER* FvHeader = (EFI_FIRMWARE_VOLUME_HEADER*)Fv;

  FvHeader->Checksum = 0;
  FvHeader->Checksum =
    FvBufCalculateChecksum16 (
      (UINT16*) FvHeader,
      FvHeader->HeaderLength / sizeof (UINT16)
      );

  return EFI_SUCCESS;
}

/**
  Clears out all files from the Fv buffer in memory

  @param SourceFv - Address of the Fv in memory
  @param DestinationFv - Output for destination Fv
    DestinationFv == NULL - invalid parameter
    *DestinationFv == NULL - memory will be allocated
    *DestinationFv != NULL - this address will be the destination

  @retval EFI_SUCCESS
**/
EFI_STATUS
FvBufDuplicate (
  IN VOID *SourceFv,
  IN OUT VOID **DestinationFv
  )
{
  EFI_STATUS Status;
  UINTN size;

  if (DestinationFv == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FvBufGetSize (SourceFv, &size);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (*DestinationFv == NULL) {
    *DestinationFv = CommonLibBinderAllocate (size);
    if (*DestinationFv == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  CommonLibBinderCopyMem (*DestinationFv, SourceFv, size);

  return EFI_SUCCESS;
}

/**
  Extends a firmware volume by the given number of bytes.

  BUGBUG: Does not handle the case where the firmware volume has a
          VTF (Volume Top File).  The VTF will not be moved to the
          end of the extended FV.

  @param Fv Source and destination firmware volume.
            Note: The original firmware volume buffer is freed!

  @param Size The minimum size that the firmware volume is to be extended by.
              The FV may be extended more than this size.

  @retval EFI_SUCCESS
**/
EFI_STATUS
FvBufExtend (
  IN VOID **Fv,
  IN UINTN Size
  )
{
  EFI_STATUS Status;
  UINTN OldSize;
  UINTN NewSize;
  UINTN BlockCount;
  VOID* NewFv;

  EFI_FIRMWARE_VOLUME_HEADER* hdr;
  EFI_FV_BLOCK_MAP_ENTRY*     blk;

  Status = FvBufGetSize (*Fv, &OldSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Locate the block map in the fv header
  //
  hdr = (EFI_FIRMWARE_VOLUME_HEADER*)*Fv;
  blk = hdr->BlockMap;

  //
  // Calculate the number of blocks needed to achieve the requested
  // size extension
  //
  BlockCount = ((Size + (blk->Length - 1)) / blk->Length);

  //
  // Calculate the new size from the number of blocks that will be added
  //
  NewSize = OldSize + (BlockCount * blk->Length);

  NewFv = CommonLibBinderAllocate (NewSize);
  if (NewFv == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy the old data
  //
  CommonLibBinderCopyMem (NewFv, *Fv, OldSize);

  //
  // Free the old fv buffer
  //
  CommonLibBinderFree (*Fv);

  //
  // Locate the block map in the new fv header
  //
  hdr = (EFI_FIRMWARE_VOLUME_HEADER*)NewFv;
  hdr->FvLength = NewSize;
  blk = hdr->BlockMap;

  //
  // Update the block map for the new fv
  //
  blk->NumBlocks += (UINT32)BlockCount;

  //
  // Update the FV header checksum
  //
  FvBufChecksumHeader (NewFv);

  //
  // Clear out the new area of the FV
  //
  CommonLibBinderSetMem (
    (UINT8*)NewFv + OldSize,
    (NewSize - OldSize),
    (hdr->Attributes & EFI_FVB2_ERASE_POLARITY) ? 0xFF : 0
    );

  //
  // Set output with new fv that was created
  //
  *Fv = NewFv;

  return EFI_SUCCESS;

}

/**
  Clears out all files from the Fv buffer in memory

  @param Fv Address of the Fv in memory

  @retval EFI_SUCCESS
**/
EFI_STATUS
FvBufClearAllFiles (
  IN OUT VOID *Fv
  )
{
  EFI_FIRMWARE_VOLUME_HEADER *hdr = (EFI_FIRMWARE_VOLUME_HEADER*)Fv;
  EFI_STATUS Status;
  UINTN size = 0;

  Status = FvBufGetSize (Fv, &size);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CommonLibBinderSetMem(
    (UINT8*)hdr + hdr->HeaderLength,
    size - hdr->HeaderLength,
    (hdr->Attributes & EFI_FVB2_ERASE_POLARITY) ? 0xFF : 0
    );

  return EFI_SUCCESS;
}

/**
  Clears out all files from the Fv buffer in memory

  @param Fv Address of the Fv in memory

  @retval EFI_SUCCESS
**/
EFI_STATUS
FvBufGetSize (
  IN VOID *Fv,
  OUT UINTN *Size
  )
{
  EFI_FIRMWARE_VOLUME_HEADER *hdr = (EFI_FIRMWARE_VOLUME_HEADER*)Fv;
  EFI_FV_BLOCK_MAP_ENTRY *blk = hdr->BlockMap;

  *Size = 0;

  while (blk->Length != 0 || blk->NumBlocks != 0) {
    *Size = *Size + (blk->Length * blk->NumBlocks);
    if (*Size >= 0x40000000) {
      // If size is greater than 1GB, then assume it is corrupted
      return EFI_VOLUME_CORRUPTED;
    }
    blk++;
  }

  if (*Size == 0) {
    // If size is 0, then assume the volume is corrupted
    return EFI_VOLUME_CORRUPTED;
  }

  return EFI_SUCCESS;
}

/**
  Adds a new FFS file

  @param Fv Address of the Fv in memory
  @param File FFS file to add to Fv

  @retval EFI_SUCCESS
**/
EFI_STATUS
FvBufAddFile (
  IN OUT VOID *Fv,
  IN VOID *File
  )
{
  EFI_FIRMWARE_VOLUME_HEADER *hdr = (EFI_FIRMWARE_VOLUME_HEADER*)Fv;

  EFI_FFS_FILE_HEADER *fhdr = NULL;
  EFI_FVB_ATTRIBUTES_2 FvbAttributes;
  UINTN offset;
  UINTN fsize;
  UINTN newSize;
  UINTN clearLoop;

  EFI_STATUS Status;
  UINTN fvSize;

  Status = FvBufGetSize (Fv, &fvSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FvbAttributes = hdr->Attributes;
  newSize = FvBufGetFfsFileSize ((EFI_FFS_FILE_HEADER*)File);

  for(
      offset = (UINTN)ALIGN_POINTER (hdr->HeaderLength, 8);
      offset + newSize <= fvSize;
      offset = (UINTN)ALIGN_POINTER (offset, 8)
    ) {

    fhdr = (EFI_FFS_FILE_HEADER*) ((UINT8*)hdr + offset);

    if (EFI_TEST_FFS_ATTRIBUTES_BIT(
          FvbAttributes,
          fhdr->State,
          EFI_FILE_HEADER_VALID
        )
      ) {
      // BUGBUG: Need to make sure that the new file does not already
      // exist.

      fsize = FvBufGetFfsFileSize (fhdr);
      if (fsize == 0 || (offset + fsize > fvSize)) {
        return EFI_VOLUME_CORRUPTED;
      }

      offset = offset + fsize;
      continue;
    }

    clearLoop = 0;
    while ((clearLoop < newSize) &&
           (((UINT8*)fhdr)[clearLoop] ==
             (UINT8)((hdr->Attributes & EFI_FVB2_ERASE_POLARITY) ? 0xFF : 0)
           )
          ) {
      clearLoop++;
    }

    //
    // We found a place in the FV which is empty and big enough for
    // the new file
    //
    if (clearLoop >= newSize) {
      break;
    }

    offset = offset + 1; // Make some forward progress
  }

  if (offset + newSize > fvSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  CommonLibBinderCopyMem (fhdr, File, newSize);

  return EFI_SUCCESS;
}

/**
  Adds a new FFS file.  Extends the firmware volume if needed.

  @param Fv Source and destination firmware volume.
            Note: If the FV is extended, then the original firmware volume
             buffer is freed!

  @param Size The minimum size that the firmware volume is to be extended by.
              The FV may be extended more than this size.

  @retval EFI_SUCCESS
**/
EFI_STATUS
FvBufAddFileWithExtend (
  IN OUT VOID **Fv,
  IN VOID *File
  )
{
  EFI_STATUS Status;
  EFI_FFS_FILE_HEADER* NewFile;

  NewFile = (EFI_FFS_FILE_HEADER*)File;

  //
  // Try to add to the capsule volume
  //
  Status = FvBufAddFile (*Fv, NewFile);
  if (Status == EFI_OUT_OF_RESOURCES) {
    //
    // Try to extend the capsule volume by the size of the file
    //
    Status = FvBufExtend (Fv, FvBufExpand3ByteSize (NewFile->Size));
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Now, try to add the file again
    //
    Status = FvBufAddFile (*Fv, NewFile);
  }

  return Status;
}

/**
  Adds a new FFS VFT (Volume Top File) file.  In other words, adds the
  file to the end of the firmware volume.

  @param Fv Address of the Fv in memory
  @param File FFS file to add to Fv

  @retval EFI_SUCCESS
**/
EFI_STATUS
FvBufAddVtfFile (
  IN OUT VOID *Fv,
  IN VOID *File
  )
{
  EFI_STATUS Status;

  EFI_FIRMWARE_VOLUME_HEADER *hdr = (EFI_FIRMWARE_VOLUME_HEADER*)Fv;

  EFI_FFS_FILE_HEADER* NewFile;
  UINTN                NewFileSize;

  UINT8 erasedUint8;
  UINTN clearLoop;

  EFI_FFS_FILE_HEADER *LastFile;
  UINTN LastFileSize;

  UINTN fvSize;
  UINTN Key;

  Status = FvBufGetSize (Fv, &fvSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  erasedUint8 = (UINT8)((hdr->Attributes & EFI_FVB2_ERASE_POLARITY) ? 0xFF : 0);
  NewFileSize = FvBufGetFfsFileSize ((EFI_FFS_FILE_HEADER*)File);

  if (NewFileSize != (UINTN)ALIGN_POINTER (NewFileSize, 8)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the last file in the FV
  //
  Key = 0;
  LastFile = NULL;
  LastFileSize = 0;
  do {
    Status = FvBufFindNextFile (Fv, &Key, (VOID **)&LastFile);
    LastFileSize = FvBufGetFfsFileSize ((EFI_FFS_FILE_HEADER*)File);
  } while (!EFI_ERROR (Status));

  //
  // If no files were found, then we start at the beginning of the FV
  //
  if (LastFile == NULL) {
    LastFile = (EFI_FFS_FILE_HEADER*)((UINT8*)hdr + hdr->HeaderLength);
  }

  //
  // We want to put the new file (VTF) at the end of the FV
  //
  NewFile = (EFI_FFS_FILE_HEADER*)((UINT8*)hdr + (fvSize - NewFileSize));

  //
  // Check to see if there is enough room for the VTF after the last file
  // found in the FV
  //
  if ((UINT8*)NewFile < ((UINT8*)LastFile + LastFileSize)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Loop to determine if the end of the FV is empty
  //
  clearLoop = 0;
  while ((clearLoop < NewFileSize) &&
         (((UINT8*)NewFile)[clearLoop] == erasedUint8)
        ) {
    clearLoop++;
  }

  //
  // Check to see if there was not enough room for the file
  //
  if (clearLoop < NewFileSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  CommonLibBinderCopyMem (NewFile, File, NewFileSize);

  return EFI_SUCCESS;
}

/**
  Expands the 3 byte size commonly used in Firmware Volume data structures

  @param Size Address of the 3 byte array representing the size

  @return UINT32
**/
VOID
FvBufCompact3ByteSize (
  OUT VOID* SizeDest,
  IN UINT32 Size
  )
{
  ((UINT8*)SizeDest)[0] = (UINT8)Size;
  ((UINT8*)SizeDest)[1] = (UINT8)(Size >> 8);
  ((UINT8*)SizeDest)[2] = (UINT8)(Size >> 16);
}

/**
  Get the FFS file size.

  @param Ffs Pointer to FFS header

  @return UINT32
**/
UINT32
FvBufGetFfsFileSize (
  IN EFI_FFS_FILE_HEADER *Ffs
  )
{
  if (Ffs == NULL) {
    return 0;
  }
  if (Ffs->Attributes & FFS_ATTRIB_LARGE_FILE) {
    return (UINT32) ((EFI_FFS_FILE_HEADER2 *)Ffs)->ExtendedSize;
  }
  return FvBufExpand3ByteSize(Ffs->Size);
}

/**
  Get the FFS header size.

  @param Ffs Pointer to FFS header

  @return UINT32
**/
UINT32
FvBufGetFfsHeaderSize (
  IN EFI_FFS_FILE_HEADER *Ffs
  )
{
  if (Ffs == NULL) {
    return 0;
  }
  if (Ffs->Attributes & FFS_ATTRIB_LARGE_FILE) {
    return sizeof(EFI_FFS_FILE_HEADER2);
  }
  return sizeof(EFI_FFS_FILE_HEADER);
}

/**
  Expands the 3 byte size commonly used in Firmware Volume data structures

  @param Size Address of the 3 byte array representing the size

  @return UINT32
**/
UINT32
FvBufExpand3ByteSize (
  IN VOID* Size
  )
{
  return (((UINT8*)Size)[2] << 16) +
         (((UINT8*)Size)[1] << 8) +
         ((UINT8*)Size)[0];
}

/**
  Iterates through the files contained within the firmware volume

  @param Fv Address of the Fv in memory
  @param Key Should be 0 to get the first file.  After that, it should be
             passed back in without modifying its contents to retrieve
             subsequent files.
  @param File Output file pointer
    File == NULL - invalid parameter
    otherwise - *File will be update to the location of the file

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
  @retval EFI_VOLUME_CORRUPTED
**/
EFI_STATUS
FvBufFindNextFile (
  IN VOID *Fv,
  IN OUT UINTN *Key,
  OUT VOID **File
  )
{
  EFI_FIRMWARE_VOLUME_HEADER *hdr = (EFI_FIRMWARE_VOLUME_HEADER*)Fv;

  EFI_FFS_FILE_HEADER *fhdr = NULL;
  EFI_FVB_ATTRIBUTES_2 FvbAttributes;
  UINTN fsize;

  EFI_STATUS Status;
  UINTN fvSize;

  if (Fv == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FvBufGetSize (Fv, &fvSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (*Key == 0) {
    *Key = hdr->HeaderLength;
  }

  FvbAttributes = hdr->Attributes;

  for(
      *Key = (UINTN)ALIGN_POINTER (*Key, 8);
      (*Key + sizeof (*fhdr)) < fvSize;
      *Key = (UINTN)ALIGN_POINTER (*Key, 8)
    ) {

    fhdr = (EFI_FFS_FILE_HEADER*) ((UINT8*)hdr + *Key);
    fsize = FvBufGetFfsFileSize (fhdr);

    if (!EFI_TEST_FFS_ATTRIBUTES_BIT(
          FvbAttributes,
          fhdr->State,
          EFI_FILE_HEADER_VALID
        ) ||
        EFI_TEST_FFS_ATTRIBUTES_BIT(
          FvbAttributes,
          fhdr->State,
          EFI_FILE_HEADER_INVALID
        )
      ) {
      *Key = *Key + 1; // Make some forward progress
      continue;
    } else if(
        EFI_TEST_FFS_ATTRIBUTES_BIT(
          FvbAttributes,
          fhdr->State,
          EFI_FILE_MARKED_FOR_UPDATE
        ) ||
        EFI_TEST_FFS_ATTRIBUTES_BIT(
          FvbAttributes,
          fhdr->State,
          EFI_FILE_DELETED
        )
      ) {
      *Key = *Key + fsize;
      continue;
    } else if (EFI_TEST_FFS_ATTRIBUTES_BIT(
          FvbAttributes,
          fhdr->State,
          EFI_FILE_DATA_VALID
        )
      ) {
      *File = (UINT8*)hdr + *Key;
      *Key = *Key + fsize;
      return EFI_SUCCESS;
    }

    *Key = *Key + 1; // Make some forward progress
  }

  return EFI_NOT_FOUND;
}

/**
  Searches the Fv for a file by its name

  @param Fv Address of the Fv in memory
  @param Name Guid filename to search for in the firmware volume
  @param File Output file pointer
    File == NULL - Only determine if the file exists, based on return
                   value from the function call.
    otherwise - *File will be update to the location of the file

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
  @retval EFI_VOLUME_CORRUPTED
**/
EFI_STATUS
FvBufFindFileByName (
  IN VOID *Fv,
  IN EFI_GUID *Name,
  OUT VOID **File
  )
{
  EFI_STATUS Status;
  UINTN Key;
  EFI_FFS_FILE_HEADER *NextFile;

  Key = 0;
  while (TRUE) {
    Status = FvBufFindNextFile (Fv, &Key, (VOID **)&NextFile);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (CommonLibBinderCompareGuid (Name, &NextFile->Name)) {
      if (File != NULL) {
        *File = NextFile;
      }
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Searches the Fv for a file by its type

  @param Fv Address of the Fv in memory
  @param Type FFS FILE type to search for
  @param File Output file pointer
    (File == NULL) -> Only determine if the file exists, based on return
                      value from the function call.
    otherwise -> *File will be update to the location of the file

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
  @retval EFI_VOLUME_CORRUPTED
**/
EFI_STATUS
FvBufFindFileByType (
  IN VOID *Fv,
  IN EFI_FV_FILETYPE Type,
  OUT VOID **File
  )
{
  EFI_STATUS Status;
  UINTN Key;
  EFI_FFS_FILE_HEADER *NextFile;

  Key = 0;
  while (TRUE) {
    Status = FvBufFindNextFile (Fv, &Key, (VOID **)&NextFile);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Type == NextFile->Type) {
      if (File != NULL) {
        *File = NextFile;
      }
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Searches the requested file for raw data.

  This routine either returns all the payload of a EFI_FV_FILETYPE_RAW file,
  or finds the EFI_SECTION_RAW section within the file and returns its data.

  @param FfsFile Address of the FFS file in memory
  @param RawData Pointer to the raw data within the file
                 (This is NOT allocated.  It is within the file.)
  @param RawDataSize Size of the raw data within the file

  @return EFI_STATUS
**/
EFI_STATUS
FvBufGetFileRawData (
  IN  VOID*     FfsFile,
  OUT VOID**    RawData,
  OUT UINTN*    RawDataSize
  )
{
  EFI_STATUS Status;
  EFI_FFS_FILE_HEADER* File;
  EFI_RAW_SECTION* Section;

  File = (EFI_FFS_FILE_HEADER*)FfsFile;

  //
  // Is the file type == EFI_FV_FILETYPE_RAW?
  //
  if (File->Type == EFI_FV_FILETYPE_RAW) {
    //
    // Raw filetypes don't have sections, so we just return the raw data
    //
    *RawData = (VOID*)((UINT8 *)File + FvBufGetFfsHeaderSize (File));
    *RawDataSize = FvBufGetFfsFileSize (File) - FvBufGetFfsHeaderSize (File);
    return EFI_SUCCESS;
  }

  //
  // Within the file, we now need to find the EFI_SECTION_RAW section.
  //
  Status = FvBufFindSectionByType (File, EFI_SECTION_RAW, (VOID **)&Section);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *RawData = (VOID*)((UINT8 *)Section + FvBufGetSecHdrLen(Section));
  *RawDataSize =
    FvBufGetSecFileLen (Section) - FvBufGetSecHdrLen(Section);

  return EFI_SUCCESS;

}

/**
  Packages up a FFS file containing the input raw data.

  The file created will have a type of EFI_FV_FILETYPE_FREEFORM, and will
  contain one EFI_FV_FILETYPE_RAW section.

  @param RawData Pointer to the raw data to be packed
  @param RawDataSize Size of the raw data to be packed
  @param FfsFile Address of the packaged FFS file.
                 Note: The called must deallocate this memory!

  @return EFI_STATUS
**/
EFI_STATUS
FvBufPackageFreeformRawFile (
  IN EFI_GUID*  Filename,
  IN VOID*      RawData,
  IN UINTN      RawDataSize,
  OUT VOID**    FfsFile
  )
{
  EFI_FFS_FILE_HEADER* NewFile;
  UINT32 NewFileSize;
  EFI_RAW_SECTION* NewSection;
  UINT32 NewSectionSize;
  UINT32 FfsHdrLen;
  UINT32 SecHdrLen;

  //
  // The section size is the DataSize + the size of the section header
  //
  NewSectionSize = (UINT32)sizeof (EFI_RAW_SECTION) + (UINT32)RawDataSize;
  SecHdrLen = sizeof (EFI_RAW_SECTION);
  if (NewSectionSize >= MAX_SECTION_SIZE) {
    NewSectionSize = (UINT32)sizeof (EFI_RAW_SECTION2) + (UINT32)RawDataSize;
    SecHdrLen = sizeof (EFI_RAW_SECTION2);
  }

  //
  // The file size is the size of the file header + the section size
  //
  NewFileSize = sizeof (EFI_FFS_FILE_HEADER) + NewSectionSize;
  FfsHdrLen = sizeof (EFI_FFS_FILE_HEADER);
  if (NewFileSize >= MAX_FFS_SIZE) {
    NewFileSize = sizeof (EFI_FFS_FILE_HEADER2) + NewSectionSize;
    FfsHdrLen = sizeof (EFI_FFS_FILE_HEADER2);
  }

  //
  // Try to allocate a buffer to build the new FFS file in
  //
  NewFile = CommonLibBinderAllocate (NewFileSize);
  if (NewFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CommonLibBinderSetMem (NewFile, NewFileSize, 0);

  //
  // The NewSection follow right after the FFS file header
  //
  NewSection = (EFI_RAW_SECTION*)((UINT8*)NewFile + FfsHdrLen);
  if (NewSectionSize >= MAX_SECTION_SIZE) {
    FvBufCompact3ByteSize (NewSection->Size, 0xffffff);
    ((EFI_RAW_SECTION2 *)NewSection)->ExtendedSize = NewSectionSize;
  } else {
    FvBufCompact3ByteSize (NewSection->Size, NewSectionSize);
  }
  NewSection->Type = EFI_SECTION_RAW;

  //
  // Copy the actual file data into the buffer
  //
  CommonLibBinderCopyMem ((UINT8 *)NewSection + SecHdrLen, RawData, RawDataSize);

  //
  // Initialize the FFS file header
  //
  CommonLibBinderCopyMem (&NewFile->Name, Filename, sizeof (EFI_GUID));
  NewFile->Attributes = 0;
  if (NewFileSize >= MAX_FFS_SIZE) {
    FvBufCompact3ByteSize (NewFile->Size, 0x0);
    ((EFI_FFS_FILE_HEADER2 *)NewFile)->ExtendedSize = NewFileSize;
    NewFile->Attributes |= FFS_ATTRIB_LARGE_FILE;
  } else {
    FvBufCompact3ByteSize (NewFile->Size, NewFileSize);
  }
  NewFile->Type = EFI_FV_FILETYPE_FREEFORM;
  NewFile->IntegrityCheck.Checksum.Header =
    FvBufCalculateChecksum8 ((UINT8*)NewFile, FfsHdrLen);
  NewFile->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
  NewFile->State = (UINT8)~( EFI_FILE_HEADER_CONSTRUCTION |
                             EFI_FILE_HEADER_VALID |
                             EFI_FILE_DATA_VALID
                           );

  *FfsFile = NewFile;

  return EFI_SUCCESS;
}

/**
  Iterates through the sections contained within a given array of sections

  @param SectionsStart Address of the start of the FFS sections array
  @param TotalSectionsSize Total size of all the sections
  @param Key Should be 0 to get the first section.  After that, it should be
             passed back in without modifying its contents to retrieve
             subsequent files.
  @param Section Output section pointer
    (Section == NULL) -> invalid parameter
    otherwise -> *Section will be update to the location of the file

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
  @retval EFI_VOLUME_CORRUPTED
**/
EFI_STATUS
FvBufFindNextSection (
  IN VOID *SectionsStart,
  IN UINTN TotalSectionsSize,
  IN OUT UINTN *Key,
  OUT VOID **Section
  )
{
  EFI_COMMON_SECTION_HEADER *sectionHdr;
  UINTN sectionSize;

  *Key = (UINTN)ALIGN_POINTER (*Key, 4); // Sections are DWORD aligned

  if ((*Key + sizeof (*sectionHdr)) > TotalSectionsSize) {
    return EFI_NOT_FOUND;
  }

  sectionHdr = (EFI_COMMON_SECTION_HEADER*)((UINT8*)SectionsStart + *Key);
  sectionSize = FvBufGetSecFileLen (sectionHdr);

  if (sectionSize < sizeof (EFI_COMMON_SECTION_HEADER)) {
    return EFI_NOT_FOUND;
  }

  if ((*Key + sectionSize) > TotalSectionsSize) {
    return EFI_NOT_FOUND;
  }

  *Section = (UINT8*)sectionHdr;
  *Key = *Key + sectionSize;
  return EFI_SUCCESS;

}

/**
  Searches the FFS file and counts the number of sections found.
  The sections are NOT recursed.

  @param FfsFile Address of the FFS file in memory
  @param Count The location to store the section count in

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
  @retval EFI_VOLUME_CORRUPTED
**/
EFI_STATUS
FvBufCountSections (
  IN VOID* FfsFile,
  IN UINTN* Count
  )
{
  EFI_STATUS                 Status;
  UINTN                      Key;
  VOID*                      SectionStart;
  UINTN                      TotalSectionsSize;
  EFI_COMMON_SECTION_HEADER* NextSection;

  SectionStart = (VOID*)((UINTN)FfsFile + FvBufGetFfsHeaderSize(FfsFile));
  TotalSectionsSize =
    FvBufGetFfsFileSize ((EFI_FFS_FILE_HEADER*)FfsFile) -
    FvBufGetFfsHeaderSize(FfsFile);
  Key = 0;
  *Count = 0;
  while (TRUE) {
    Status = FvBufFindNextSection (
               SectionStart,
               TotalSectionsSize,
               &Key,
               (VOID **)&NextSection
               );
    if (Status == EFI_NOT_FOUND) {
      return EFI_SUCCESS;
    } else if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Increment the section counter
    //
    *Count += 1;

  }

  return EFI_NOT_FOUND;
}

/**
  Searches the FFS file for a section by its type

  @param FfsFile Address of the FFS file in memory
  @param Type FFS FILE section type to search for
  @param Section Output section pointer
    (Section == NULL) -> Only determine if the section exists, based on return
                         value from the function call.
    otherwise -> *Section will be update to the location of the file

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
  @retval EFI_VOLUME_CORRUPTED
**/
EFI_STATUS
FvBufFindSectionByType (
  IN VOID *FfsFile,
  IN UINT8 Type,
  OUT VOID **Section
  )
{
  EFI_STATUS Status;
  UINTN Key;
  VOID*                      SectionStart;
  UINTN                      TotalSectionsSize;
  EFI_COMMON_SECTION_HEADER* NextSection;

  SectionStart = (VOID*)((UINTN)FfsFile + FvBufGetFfsHeaderSize(FfsFile));
  TotalSectionsSize =
    FvBufGetFfsFileSize ((EFI_FFS_FILE_HEADER*)FfsFile) -
    FvBufGetFfsHeaderSize(FfsFile);
  Key = 0;
  while (TRUE) {
    Status = FvBufFindNextSection (
               SectionStart,
               TotalSectionsSize,
               &Key,
               (VOID **)&NextSection
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Type == NextSection->Type) {
      if (Section != NULL) {
        *Section = NextSection;
      }
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Shrinks a firmware volume (in place) to provide a minimal FV.

  BUGBUG: Does not handle the case where the firmware volume has a
          VTF (Volume Top File).  The VTF will not be moved to the
          end of the extended FV.

  @param Fv Firmware volume.

 @retval EFI_SUCCESS
**/
EFI_STATUS
FvBufShrinkWrap (
  IN VOID *Fv
  )
{
  EFI_STATUS Status;
  UINTN OldSize;
  UINT32 BlockCount;
  UINT32 NewBlockSize = 128;
  UINTN Key;
  EFI_FFS_FILE_HEADER* FileIt;
  VOID* EndOfLastFile;

  EFI_FIRMWARE_VOLUME_HEADER* FvHdr;

  Status = FvBufGetSize (Fv, &OldSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FvBufUnifyBlockSizes (Fv, NewBlockSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Locate the block map in the fv header
  //
  FvHdr = (EFI_FIRMWARE_VOLUME_HEADER*)Fv;

  //
  // Find the end of the last file
  //
  Key = 0;
  EndOfLastFile = (UINT8*)FvHdr + FvHdr->FvLength;
  while (!EFI_ERROR (FvBufFindNextFile (Fv, &Key, (VOID **)&FileIt))) {
    EndOfLastFile =
      (VOID*)((UINT8*)FileIt + FvBufGetFfsFileSize (FileIt));
  }

  //
  // Set the BlockCount to have the minimal number of blocks for the Fv.
  //
  BlockCount = (UINT32)((UINTN)EndOfLastFile - (UINTN)Fv);
  BlockCount = BlockCount + NewBlockSize - 1;
  BlockCount = BlockCount / NewBlockSize;

  //
  // Adjust the block count to shrink the Fv in place.
  //
  FvHdr->BlockMap[0].NumBlocks = BlockCount;
  FvHdr->FvLength = BlockCount * NewBlockSize;

  //
  // Update the FV header checksum
  //
  FvBufChecksumHeader (Fv);

  return EFI_SUCCESS;

}

/**
  Searches the FFS file for a section by its type

  @param Fv Address of the Fv in memory
  @param BlockSize The size of the blocks to convert the Fv to.  If the total size
                   of the Fv is not evenly divisible by this size, then
                   EFI_INVALID_PARAMETER will be returned.

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
  @retval EFI_VOLUME_CORRUPTED
**/
EFI_STATUS
FvBufUnifyBlockSizes (
  IN OUT VOID *Fv,
  IN UINTN BlockSize
  )
{
  EFI_FIRMWARE_VOLUME_HEADER *hdr = (EFI_FIRMWARE_VOLUME_HEADER*)Fv;
  EFI_FV_BLOCK_MAP_ENTRY *blk = hdr->BlockMap;
  UINT32 Size;

  Size = 0;

  //
  // Scan through the block map list, performing error checking, and adding
  // up the total Fv size.
  //
  while( blk->Length != 0 ||
         blk->NumBlocks != 0
       ) {
    Size = Size + (blk->Length * blk->NumBlocks);
    blk++;
    if ((UINT8*)blk > ((UINT8*)hdr + hdr->HeaderLength)) {
      return EFI_VOLUME_CORRUPTED;
    }
  }

  //
  // Make sure that the Fv size is a multiple of the new block size.
  //
  if ((Size % BlockSize) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Zero out the entire block map.
  //
  CommonLibBinderSetMem (
    &hdr->BlockMap,
    (UINTN)blk - (UINTN)&hdr->BlockMap,
    0
    );

  //
  // Write out the single block map entry.
  //
  hdr->BlockMap[0].Length = (UINT32)BlockSize;
  hdr->BlockMap[0].NumBlocks = Size / (UINT32)BlockSize;

  return EFI_SUCCESS;
}

/**
  This function calculates the UINT16 sum for the requested region.

  @param Buffer      Pointer to buffer containing byte data of component.
  @param Size        Size of the buffer

  @return The 16 bit checksum
**/
STATIC
UINT16
FvBufCalculateSum16 (
  IN UINT16       *Buffer,
  IN UINTN        Size
  )
{
  UINTN   Index;
  UINT16  Sum;

  Sum = 0;

  //
  // Perform the word sum for buffer
  //
  for (Index = 0; Index < Size; Index++) {
    Sum = (UINT16) (Sum + Buffer[Index]);
  }

  return (UINT16) Sum;
}

/**
  This function calculates the value needed for a valid UINT16 checksum

  @param Buffer      Pointer to buffer containing byte data of component.
  @param Size        Size of the buffer

  @return The 16 bit checksum value needed.
**/
STATIC
UINT16
FvBufCalculateChecksum16 (
  IN UINT16       *Buffer,
  IN UINTN        Size
  )
{
  return (UINT16)(0x10000 - FvBufCalculateSum16 (Buffer, Size));
}

/**
  This function calculates the UINT8 sum for the requested region.

  @param Buffer      Pointer to buffer containing byte data of component.
  @param Size        Size of the buffer

  @return The 8 bit checksum value needed.
**/
STATIC
UINT8
FvBufCalculateSum8 (
  IN UINT8  *Buffer,
  IN UINTN  Size
  )
{
  UINTN   Index;
  UINT8   Sum;

  Sum = 0;

  //
  // Perform the byte sum for buffer
  //
  for (Index = 0; Index < Size; Index++) {
    Sum = (UINT8) (Sum + Buffer[Index]);
  }

  return Sum;
}

/**
  This function calculates the value needed for a valid UINT8 checksum

  @param Buffer      Pointer to buffer containing byte data of component.
  @param Size        Size of the buffer

  @return The 8 bit checksum value needed.
**/
STATIC
UINT8
FvBufCalculateChecksum8 (
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
{
  return (UINT8)(0x100 - FvBufCalculateSum8 (Buffer, Size));
}


