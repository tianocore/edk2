/** @file
  Handle on-disk format and volume structures in UDF/ECMA-167 file systems.

  Copyright (C) 2014-2017 Paulo Alcantara <pcacjr@zytor.com>
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Udf.h"

//
// Vendor-Defined Device Path GUID for UDF file system
//
EFI_GUID gUdfDevPathGuid = EFI_UDF_DEVICE_PATH_GUID;

/**
  Find the anchor volume descriptor pointer.

  @param[in]  BlockIo             BlockIo interface.
  @param[in]  DiskIo              DiskIo interface.
  @param[out] AnchorPoint         Anchor volume descriptor pointer.

  @retval EFI_SUCCESS             Anchor volume descriptor pointer found.
  @retval EFI_VOLUME_CORRUPTED    The file system structures are corrupted.
  @retval other                   Anchor volume descriptor pointer not found.

**/
EFI_STATUS
FindAnchorVolumeDescriptorPointer (
  IN   EFI_BLOCK_IO_PROTOCOL                 *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL                  *DiskIo,
  OUT  UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  *AnchorPoint
  )
{
  EFI_STATUS          Status;
  UINT32              BlockSize;
  EFI_LBA             EndLBA;
  EFI_LBA             DescriptorLBAs[4];
  UINTN               Index;
  UDF_DESCRIPTOR_TAG  *DescriptorTag;

  BlockSize = BlockIo->Media->BlockSize;
  EndLBA = BlockIo->Media->LastBlock;
  DescriptorLBAs[0] = 256;
  DescriptorLBAs[1] = EndLBA - 256;
  DescriptorLBAs[2] = EndLBA;
  DescriptorLBAs[3] = 512;

  for (Index = 0; Index < ARRAY_SIZE (DescriptorLBAs); Index++) {
    Status = DiskIo->ReadDisk (
      DiskIo,
      BlockIo->Media->MediaId,
      MultU64x32 (DescriptorLBAs[Index], BlockSize),
      sizeof (UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER),
      (VOID *)AnchorPoint
      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DescriptorTag = &AnchorPoint->DescriptorTag;

    //
    // Check if read LBA has a valid AVDP descriptor.
    //
    if (DescriptorTag->TagIdentifier == UdfAnchorVolumeDescriptorPointer) {
      return EFI_SUCCESS;
    }
  }
  //
  // No AVDP found.
  //
  return EFI_VOLUME_CORRUPTED;
}

/**
  Save the content of Logical Volume Descriptors and Partitions Descriptors in
  memory.

  @param[in]  BlockIo             BlockIo interface.
  @param[in]  DiskIo              DiskIo interface.
  @param[in]  AnchorPoint         Anchor volume descriptor pointer.
  @param[out] Volume              UDF volume information structure.

  @retval EFI_SUCCESS             The descriptors were saved.
  @retval EFI_OUT_OF_RESOURCES    The descriptors were not saved due to lack of
                                  resources.
  @retval other                   The descriptors were not saved due to
                                  ReadDisk error.

**/
EFI_STATUS
StartMainVolumeDescriptorSequence (
  IN   EFI_BLOCK_IO_PROTOCOL                 *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL                  *DiskIo,
  IN   UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  *AnchorPoint,
  OUT  UDF_VOLUME_INFO                       *Volume
  )
{
  EFI_STATUS            Status;
  UINT32                BlockSize;
  UDF_EXTENT_AD         *ExtentAd;
  EFI_LBA               SeqStartBlock;
  EFI_LBA               SeqEndBlock;
  BOOLEAN               StopSequence;
  VOID                  *Buffer;
  UDF_DESCRIPTOR_TAG    *DescriptorTag;
  UINT32                LogicalBlockSize;

  BlockSize = BlockIo->Media->BlockSize;
  ExtentAd = &AnchorPoint->MainVolumeDescriptorSequenceExtent;

  //
  // Allocate buffer for reading disk blocks
  //
  Buffer = AllocateZeroPool ((UINTN)BlockSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // The logical partition created by Partition driver is relative to the main
  // VDS extent location, so we start the Main Volume Descriptor Sequence at
  // LBA 0.
  //
  // We don't need to check again if we have valid Volume Descriptors here since
  // Partition driver already did.
  //
  SeqStartBlock = 0;
  SeqEndBlock = SeqStartBlock + DivU64x32 ((UINT64)ExtentAd->ExtentLength,
                                           BlockSize);
  StopSequence = FALSE;
  for (; SeqStartBlock < SeqEndBlock && !StopSequence; SeqStartBlock++) {
    //
    // Read disk block
    //
    Status = BlockIo->ReadBlocks (
      BlockIo,
      BlockIo->Media->MediaId,
      SeqStartBlock,
      BlockSize,
      Buffer
      );
    if (EFI_ERROR (Status)) {
      goto Out_Free;
    }

    DescriptorTag = Buffer;

    switch (DescriptorTag->TagIdentifier) {
    case UdfPartitionDescriptor:
      //
      // Save Partition Descriptor
      //
      CopyMem (&Volume->PartitionDesc, Buffer, sizeof (Volume->PartitionDesc));
      break;

    case UdfLogicalVolumeDescriptor:
      //
      // Save Logical Volume Descriptor
      //
      CopyMem (&Volume->LogicalVolDesc, Buffer, sizeof (Volume->LogicalVolDesc));
      break;

    case UdfTerminatingDescriptor:
      StopSequence = TRUE;
      break;

    default:
      ;
    }
  }

  //
  // Determine FE (File Entry) size
  //
  LogicalBlockSize = Volume->LogicalVolDesc.LogicalBlockSize;
  if (LogicalBlockSize >= UDF_LOGICAL_SECTOR_SIZE) {
    Volume->FileEntrySize = (UINTN)LogicalBlockSize;
  } else {
    Volume->FileEntrySize = UDF_LOGICAL_SECTOR_SIZE;
  }

  Status = EFI_SUCCESS;

Out_Free:
  //
  // Free block read buffer
  //
  FreePool (Buffer);

  return Status;
}

/**
  Return a Partition Descriptor given a Long Allocation Descriptor. This is
  necessary to calculate the right extent (LongAd) offset which is added up
  with partition's starting location.

  @param[in]  Volume              Volume information pointer.
  @param[in]  LongAd              Long Allocation Descriptor pointer.

  @return A pointer to a Partition Descriptor.

**/
UDF_PARTITION_DESCRIPTOR *
GetPdFromLongAd (
  IN UDF_VOLUME_INFO                 *Volume,
  IN UDF_LONG_ALLOCATION_DESCRIPTOR  *LongAd
  )
{
  UDF_LOGICAL_VOLUME_DESCRIPTOR  *LogicalVolDesc;
  UINT16                         PartitionNum;

  LogicalVolDesc = &Volume->LogicalVolDesc;

  switch (LogicalVolDesc->DomainIdentifier.Suffix.Domain.UdfRevision) {
  case 0x0102:
  case 0x0150:
  case 0x0200:
  case 0x0201:
  case 0x0250:
  case 0x0260:
    //
    // UDF 1.02 specification:
    //
    // There shall be exactly one prevailing Logical Volume Descriptor recorded
    // per Volume Set. The Partition Maps field shall contain only Type 1
    // Partition Maps.
    //
    // UDF 1.50 through 2.60 specs say:
    //
    // For the purpose of interchange partition maps shall be limited to
    // Partition Map type 1, except type 2 maps as described in the document.
    //
    // NOTE: Only one Type 1 (Physical) Partition is supported. It has been
    // checked already in Partition driver for existence of a single Type 1
    // Partition map. Hence, the 'PartitionReferenceNumber' field (the index
    // used to access Partition Maps data within the Logical Volume Descriptor)
    // in the Long Allocation Descriptor should be 0 to indicate there is only
    // one partition.
    //
    if (LongAd->ExtentLocation.PartitionReferenceNumber != 0) {
      return NULL;
    }
    //
    // Since only one partition, get the first one directly.
    //
    PartitionNum = *(UINT16 *)((UINTN)&LogicalVolDesc->PartitionMaps[4]);
    break;

  default:
    //
    // Unsupported UDF revision
    //
    return NULL;
  }

  //
  // Check if partition number matches Partition Descriptor found in Main Volume
  // Descriptor Sequence.
  //
  if (Volume->PartitionDesc.PartitionNumber == PartitionNum) {
    return &Volume->PartitionDesc;
  }

  return NULL;
}

/**
  Return logical sector number of a given Long Allocation Descriptor.

  @param[in]   Volume             Volume information pointer.
  @param[in]   LongAd             Long Allocation Descriptor pointer.
  @param[out]  Lsn                Logical sector number pointer.

  @retval EFI_SUCCESS             Logical sector number successfully returned.
  @retval EFI_UNSUPPORTED         Logical sector number is not returned due to
                                  unrecognized format.

**/
EFI_STATUS
GetLongAdLsn (
  IN  UDF_VOLUME_INFO                 *Volume,
  IN  UDF_LONG_ALLOCATION_DESCRIPTOR  *LongAd,
  OUT UINT64                          *Lsn
  )
{
  UDF_PARTITION_DESCRIPTOR *PartitionDesc;

  PartitionDesc = GetPdFromLongAd (Volume, LongAd);
  if (PartitionDesc == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Fail to get the Partition Descriptor from the given Long Allocation Descriptor.\n",
      __FUNCTION__
      ));
    return EFI_UNSUPPORTED;
  }

  *Lsn = (UINT64)PartitionDesc->PartitionStartingLocation -
         Volume->MainVdsStartLocation +
         LongAd->ExtentLocation.LogicalBlockNumber;

  return EFI_SUCCESS;
}

/**
  Return logical sector number of a given Short Allocation Descriptor.

  @param[in]  Volume              Volume pointer.
  @param[in]  PartitionDesc       Partition Descriptor pointer.
  @param[in]  ShortAd             Short Allocation Descriptor pointer.

  @return The logical sector number of a given Short Allocation Descriptor.

**/
UINT64
GetShortAdLsn (
  IN UDF_VOLUME_INFO                  *Volume,
  IN UDF_PARTITION_DESCRIPTOR         *PartitionDesc,
  IN UDF_SHORT_ALLOCATION_DESCRIPTOR  *ShortAd
  )
{
  return (UINT64)PartitionDesc->PartitionStartingLocation -
    Volume->MainVdsStartLocation + ShortAd->ExtentPosition;
}

/**
  Find File Set Descriptor of a given Logical Volume Descriptor.

  The found FSD will contain the extent (LogicalVolumeContentsUse) where our
  root directory is.

  @param[in]  BlockIo             BlockIo interface.
  @param[in]  DiskIo              DiskIo interface.
  @param[in]  Volume              Volume information pointer.

  @retval EFI_SUCCESS             File Set Descriptor pointer found.
  @retval EFI_VOLUME_CORRUPTED    The file system structures are corrupted.
  @retval other                   File Set Descriptor pointer not found.

**/
EFI_STATUS
FindFileSetDescriptor (
  IN   EFI_BLOCK_IO_PROTOCOL    *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL     *DiskIo,
  IN   UDF_VOLUME_INFO          *Volume
  )
{
  EFI_STATUS                     Status;
  UINT64                         Lsn;
  UDF_LOGICAL_VOLUME_DESCRIPTOR  *LogicalVolDesc;
  UDF_DESCRIPTOR_TAG             *DescriptorTag;

  LogicalVolDesc = &Volume->LogicalVolDesc;
  Status = GetLongAdLsn (Volume, &LogicalVolDesc->LogicalVolumeContentsUse, &Lsn);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // As per UDF 2.60 specification:
  //
  // There shall be exactly one File Set Descriptor recorded per Logical
  // Volume.
  //
  // Read disk block
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 (Lsn, LogicalVolDesc->LogicalBlockSize),
    sizeof (Volume->FileSetDesc),
    &Volume->FileSetDesc
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DescriptorTag = &Volume->FileSetDesc.DescriptorTag;

  //
  // Check if read block is a File Set Descriptor
  //
  if (DescriptorTag->TagIdentifier != UdfFileSetDescriptor) {
    return EFI_VOLUME_CORRUPTED;
  }

  return EFI_SUCCESS;
}

/**
  Read Volume and File Structure on an UDF file system.

  @param[in]   BlockIo            BlockIo interface.
  @param[in]   DiskIo             DiskIo interface.
  @param[out]  Volume             Volume information pointer.

  @retval EFI_SUCCESS             Volume and File Structure were read.
  @retval other                   Volume and File Structure were not read.

**/
EFI_STATUS
ReadVolumeFileStructure (
  IN   EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL   *DiskIo,
  OUT  UDF_VOLUME_INFO        *Volume
  )
{
  EFI_STATUS                            Status;
  UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  AnchorPoint;
  UDF_EXTENT_AD                         *ExtentAd;

  //
  // Find Anchor Volume Descriptor Pointer
  //
  Status = FindAnchorVolumeDescriptorPointer (
    BlockIo,
    DiskIo,
    &AnchorPoint
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Save Main VDS start block number
  //
  ExtentAd = &AnchorPoint.MainVolumeDescriptorSequenceExtent;

  Volume->MainVdsStartLocation = (UINT64)ExtentAd->ExtentLocation;

  //
  // Start Main Volume Descriptor Sequence.
  //
  Status = StartMainVolumeDescriptorSequence (
    BlockIo,
    DiskIo,
    &AnchorPoint,
    Volume
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}

/**
  Calculate length of a given File Identifier Descriptor.

  @param[in]  FileIdentifierDesc  File Identifier Descriptor pointer.

  @return The length of a given File Identifier Descriptor.

**/
UINT64
GetFidDescriptorLength (
  IN UDF_FILE_IDENTIFIER_DESCRIPTOR  *FileIdentifierDesc
  )
{
  return (UINT64)(
             (INTN)((OFFSET_OF (UDF_FILE_IDENTIFIER_DESCRIPTOR, Data[0]) + 3 +
             FileIdentifierDesc->LengthOfFileIdentifier +
             FileIdentifierDesc->LengthOfImplementationUse) >> 2) << 2
             );
}

/**
  Duplicate a given File Identifier Descriptor.

  @param[in]  FileIdentifierDesc     File Identifier Descriptor pointer.
  @param[out] NewFileIdentifierDesc  The duplicated File Identifier Descriptor.

**/
VOID
DuplicateFid (
  IN   UDF_FILE_IDENTIFIER_DESCRIPTOR  *FileIdentifierDesc,
  OUT  UDF_FILE_IDENTIFIER_DESCRIPTOR  **NewFileIdentifierDesc
  )
{
  *NewFileIdentifierDesc =
    (UDF_FILE_IDENTIFIER_DESCRIPTOR *)AllocateCopyPool (
      (UINTN) GetFidDescriptorLength (FileIdentifierDesc), FileIdentifierDesc);
}

/**
  Duplicate either a given File Entry or a given Extended File Entry.

  @param[in]  BlockIo             BlockIo interface.
  @param[in]  Volume              Volume information pointer.
  @param[in]  FileEntry           (Extended) File Entry pointer.
  @param[out] NewFileEntry        The duplicated (Extended) File Entry.

**/
VOID
DuplicateFe (
  IN   EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN   UDF_VOLUME_INFO        *Volume,
  IN   VOID                   *FileEntry,
  OUT  VOID                   **NewFileEntry
  )
{
  *NewFileEntry = AllocateCopyPool (Volume->FileEntrySize, FileEntry);
}

/**
  Get raw data + length of a given File Entry or Extended File Entry.

  The file's recorded data can contain either real file content (inline) or
  a sequence of extents (or Allocation Descriptors) which tells where file's
  content is stored in.

  NOTE: The FE/EFE can be thought it was an inode.

  @attention This is boundary function that may receive untrusted input.
  @attention The input is from FileSystem.

  The (Extended) File Entry is external input, so this routine will do basic
  validation for (Extended) File Entry and report status.

  @param[in]  FileEntryData       (Extended) File Entry pointer.
  @param[in]  FileEntrySize       Size of the (Extended) File Entry specified
                                  by FileEntryData.
  @param[out] Data                Buffer contains the raw data of a given
                                  (Extended) File Entry.
  @param[out] Length              Length of the data in Buffer.

  @retval EFI_SUCCESS             Raw data and size of the FE/EFE was read.
  @retval EFI_VOLUME_CORRUPTED    The file system structures are corrupted.

**/
EFI_STATUS
GetFileEntryData (
  IN   VOID    *FileEntryData,
  IN   UINTN   FileEntrySize,
  OUT  VOID    **Data,
  OUT  UINT64  *Length
  )
{
  UDF_DESCRIPTOR_TAG       *DescriptorTag;
  UDF_EXTENDED_FILE_ENTRY  *ExtendedFileEntry;
  UDF_FILE_ENTRY           *FileEntry;

  DescriptorTag = FileEntryData;

  if (DescriptorTag->TagIdentifier == UdfExtendedFileEntry) {
    ExtendedFileEntry = (UDF_EXTENDED_FILE_ENTRY *)FileEntryData;

    *Length  = ExtendedFileEntry->InformationLength;
    *Data    = (VOID *)((UINT8 *)ExtendedFileEntry->Data +
                        ExtendedFileEntry->LengthOfExtendedAttributes);
  } else if (DescriptorTag->TagIdentifier == UdfFileEntry) {
    FileEntry = (UDF_FILE_ENTRY *)FileEntryData;

    *Length  = FileEntry->InformationLength;
    *Data    = (VOID *)((UINT8 *)FileEntry->Data +
                        FileEntry->LengthOfExtendedAttributes);
  }

  if ((*Length > FileEntrySize) ||
      ((UINTN)FileEntryData > (UINTN)(*Data)) ||
      ((UINTN)(*Data) - (UINTN)FileEntryData > FileEntrySize - *Length)) {
    return EFI_VOLUME_CORRUPTED;
  }
  return EFI_SUCCESS;
}

/**
  Get Allocation Descriptors' data information from a given FE/EFE.

  @attention This is boundary function that may receive untrusted input.
  @attention The input is from FileSystem.

  The (Extended) File Entry is external input, so this routine will do basic
  validation for (Extended) File Entry and report status.

  @param[in]  FileEntryData       (Extended) File Entry pointer.
  @param[in]  FileEntrySize       Size of the (Extended) File Entry specified
                                  by FileEntryData.
  @param[out] AdsData             Buffer contains the Allocation Descriptors'
                                  data from a given FE/EFE.
  @param[out] Length              Length of the data in AdsData.

  @retval EFI_SUCCESS             The data and size of Allocation Descriptors
                                  were read from the FE/EFE.
  @retval EFI_VOLUME_CORRUPTED    The file system structures are corrupted.

**/
EFI_STATUS
GetAdsInformation (
  IN   VOID    *FileEntryData,
  IN   UINTN   FileEntrySize,
  OUT  VOID    **AdsData,
  OUT  UINT64  *Length
  )
{
  UDF_DESCRIPTOR_TAG       *DescriptorTag;
  UDF_EXTENDED_FILE_ENTRY  *ExtendedFileEntry;
  UDF_FILE_ENTRY           *FileEntry;

  DescriptorTag = FileEntryData;

  if (DescriptorTag->TagIdentifier == UdfExtendedFileEntry) {
    ExtendedFileEntry = (UDF_EXTENDED_FILE_ENTRY *)FileEntryData;

    *Length = ExtendedFileEntry->LengthOfAllocationDescriptors;
    *AdsData = (VOID *)((UINT8 *)ExtendedFileEntry->Data +
                        ExtendedFileEntry->LengthOfExtendedAttributes);
  } else if (DescriptorTag->TagIdentifier == UdfFileEntry) {
    FileEntry = (UDF_FILE_ENTRY *)FileEntryData;

    *Length = FileEntry->LengthOfAllocationDescriptors;
    *AdsData = (VOID *)((UINT8 *)FileEntry->Data +
                        FileEntry->LengthOfExtendedAttributes);
  }

  if ((*Length > FileEntrySize) ||
      ((UINTN)FileEntryData > (UINTN)(*AdsData)) ||
      ((UINTN)(*AdsData) - (UINTN)FileEntryData > FileEntrySize - *Length)) {
    return EFI_VOLUME_CORRUPTED;
  }
  return EFI_SUCCESS;
}

/**
  Read next Long Allocation Descriptor from a given file's data.

  @param[in]     Data             File's data pointer.
  @param[in,out] Offset           Starting offset of the File's data to read.
  @param[in]     Length           Length of the data to read.
  @param[out]    FoundLongAd      Long Allocation Descriptor pointer.

  @retval EFI_SUCCESS             A Long Allocation Descriptor was found.
  @retval EFI_DEVICE_ERROR        No more Long Allocation Descriptors.

**/
EFI_STATUS
GetLongAdFromAds (
  IN      VOID                            *Data,
  IN OUT  UINT64                          *Offset,
  IN      UINT64                          Length,
  OUT     UDF_LONG_ALLOCATION_DESCRIPTOR  **FoundLongAd
  )
{
  UDF_LONG_ALLOCATION_DESCRIPTOR  *LongAd;
  UDF_EXTENT_FLAGS                ExtentFlags;

  for (;;) {
    if (*Offset >= Length) {
      //
      // No more Long Allocation Descriptors.
      //
      return EFI_DEVICE_ERROR;
    }

    LongAd =
      (UDF_LONG_ALLOCATION_DESCRIPTOR *)((UINT8 *)Data + *Offset);

    //
    // If it's either an indirect AD (Extended Alllocation Descriptor) or an
    // allocated AD, then return it.
    //
    ExtentFlags = GET_EXTENT_FLAGS (LongAdsSequence, LongAd);
    if (ExtentFlags == ExtentIsNextExtent ||
        ExtentFlags == ExtentRecordedAndAllocated) {
      break;
    }

    //
    // This AD is either not recorded but allocated, or not recorded and not
    // allocated. Skip it.
    //
    *Offset += AD_LENGTH (LongAdsSequence);
  }

  *FoundLongAd = LongAd;

  return EFI_SUCCESS;
}

/**
  Read next Short Allocation Descriptor from a given file's data.

  @param[in]     Data             File's data pointer.
  @param[in,out] Offset           Starting offset of the File's data to read.
  @param[in]     Length           Length of the data to read.
  @param[out]    FoundShortAd     Short Allocation Descriptor pointer.

  @retval EFI_SUCCESS             A Short Allocation Descriptor was found.
  @retval EFI_DEVICE_ERROR        No more Short Allocation Descriptors.

**/
EFI_STATUS
GetShortAdFromAds (
  IN      VOID                             *Data,
  IN OUT  UINT64                           *Offset,
  IN      UINT64                           Length,
  OUT     UDF_SHORT_ALLOCATION_DESCRIPTOR  **FoundShortAd
  )
{
  UDF_SHORT_ALLOCATION_DESCRIPTOR *ShortAd;
  UDF_EXTENT_FLAGS                ExtentFlags;

  for (;;) {
    if (*Offset >= Length) {
      //
      // No more Short Allocation Descriptors.
      //
      return EFI_DEVICE_ERROR;
    }

    ShortAd =
      (UDF_SHORT_ALLOCATION_DESCRIPTOR *)((UINT8 *)Data + *Offset);

    //
    // If it's either an indirect AD (Extended Alllocation Descriptor) or an
    // allocated AD, then return it.
    //
    ExtentFlags = GET_EXTENT_FLAGS (ShortAdsSequence, ShortAd);
    if (ExtentFlags == ExtentIsNextExtent ||
        ExtentFlags == ExtentRecordedAndAllocated) {
      break;
    }

    //
    // This AD is either not recorded but allocated, or not recorded and not
    // allocated. Skip it.
    //
    *Offset += AD_LENGTH (ShortAdsSequence);
  }

  *FoundShortAd = ShortAd;

  return EFI_SUCCESS;
}

/**
  Get either a Short Allocation Descriptor or a Long Allocation Descriptor from
  file's data.

  @param[in]     RecordingFlags   Flag to indicate the type of descriptor.
  @param[in]     Data             File's data pointer.
  @param[in,out] Offset           Starting offset of the File's data to read.
  @param[in]     Length           Length of the data to read.
  @param[out]    FoundAd          Allocation Descriptor pointer.

  @retval EFI_SUCCESS             A Short Allocation Descriptor was found.
  @retval EFI_DEVICE_ERROR        No more Allocation Descriptors.
                                  Invalid type of descriptor was given.

**/
EFI_STATUS
GetAllocationDescriptor (
  IN      UDF_FE_RECORDING_FLAGS  RecordingFlags,
  IN      VOID                    *Data,
  IN OUT  UINT64                  *Offset,
  IN      UINT64                  Length,
  OUT     VOID                    **FoundAd
  )
{
  if (RecordingFlags == LongAdsSequence) {
    return GetLongAdFromAds (
      Data,
      Offset,
      Length,
      (UDF_LONG_ALLOCATION_DESCRIPTOR **)FoundAd
      );
  } else if (RecordingFlags == ShortAdsSequence) {
    return GetShortAdFromAds (
      Data,
      Offset,
      Length,
      (UDF_SHORT_ALLOCATION_DESCRIPTOR **)FoundAd
      );
  }

  //
  // Code should never reach here.
  //
  ASSERT (FALSE);
  return EFI_DEVICE_ERROR;
}

/**
  Return logical sector number of either Short or Long Allocation Descriptor.

  @param[in]  RecordingFlags      Flag to indicate the type of descriptor.
  @param[in]  Volume              Volume information pointer.
  @param[in]  ParentIcb           Long Allocation Descriptor pointer.
  @param[in]  Ad                  Allocation Descriptor pointer.
  @param[out] Lsn                 Logical sector number pointer.

  @retval EFI_SUCCESS             Logical sector number of the given Allocation
                                  Descriptor successfully returned.
  @retval EFI_UNSUPPORTED         Logical sector number of the given Allocation
                                  Descriptor is not returned due to unrecognized
                                  format.

**/
EFI_STATUS
GetAllocationDescriptorLsn (
  IN  UDF_FE_RECORDING_FLAGS          RecordingFlags,
  IN  UDF_VOLUME_INFO                 *Volume,
  IN  UDF_LONG_ALLOCATION_DESCRIPTOR  *ParentIcb,
  IN  VOID                            *Ad,
  OUT UINT64                          *Lsn
  )
{
  UDF_PARTITION_DESCRIPTOR *PartitionDesc;

  if (RecordingFlags == LongAdsSequence) {
    return GetLongAdLsn (Volume, (UDF_LONG_ALLOCATION_DESCRIPTOR *)Ad, Lsn);
  } else if (RecordingFlags == ShortAdsSequence) {
    PartitionDesc = GetPdFromLongAd (Volume, ParentIcb);
    if (PartitionDesc == NULL) {
      return EFI_UNSUPPORTED;
    }

    *Lsn = GetShortAdLsn (
             Volume,
             PartitionDesc,
             (UDF_SHORT_ALLOCATION_DESCRIPTOR *)Ad
             );
    return EFI_SUCCESS;
  }

  //
  // Code should never reach here.
  //
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**
  Return offset + length of a given indirect Allocation Descriptor (AED).

  @param[in]  BlockIo             BlockIo interface.
  @param[in]  DiskIo              DiskIo interface.
  @param[in]  Volume              Volume information pointer.
  @param[in]  ParentIcb           Long Allocation Descriptor pointer.
  @param[in]  RecordingFlags      Flag to indicate the type of descriptor.
  @param[in]  Ad                  Allocation Descriptor pointer.
  @param[out] Offset              Offset of a given indirect Allocation
                                  Descriptor.
  @param[out] Length              Length of a given indirect Allocation
                                  Descriptor.

  @retval EFI_SUCCESS             The offset and length were returned.
  @retval EFI_OUT_OF_RESOURCES    The offset and length were not returned due
                                  to lack of resources.
  @retval EFI_VOLUME_CORRUPTED    The file system structures are corrupted.
  @retval other                   The offset and length were not returned.

**/
EFI_STATUS
GetAedAdsOffset (
  IN   EFI_BLOCK_IO_PROTOCOL           *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL            *DiskIo,
  IN   UDF_VOLUME_INFO                 *Volume,
  IN   UDF_LONG_ALLOCATION_DESCRIPTOR  *ParentIcb,
  IN   UDF_FE_RECORDING_FLAGS          RecordingFlags,
  IN   VOID                            *Ad,
  OUT  UINT64                          *Offset,
  OUT  UINT64                          *Length
  )
{
  EFI_STATUS                        Status;
  UINT32                            ExtentLength;
  UINT64                            Lsn;
  VOID                              *Data;
  UINT32                            LogicalBlockSize;
  UDF_ALLOCATION_EXTENT_DESCRIPTOR  *AllocExtDesc;
  UDF_DESCRIPTOR_TAG                *DescriptorTag;

  ExtentLength  = GET_EXTENT_LENGTH (RecordingFlags, Ad);
  Status        = GetAllocationDescriptorLsn (RecordingFlags,
                                              Volume,
                                              ParentIcb,
                                              Ad,
                                              &Lsn);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Data = AllocatePool (ExtentLength);
  if (Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  LogicalBlockSize = Volume->LogicalVolDesc.LogicalBlockSize;

  //
  // Read extent.
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 (Lsn, LogicalBlockSize),
    ExtentLength,
    Data
    );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  AllocExtDesc = (UDF_ALLOCATION_EXTENT_DESCRIPTOR *)Data;

  DescriptorTag = &AllocExtDesc->DescriptorTag;

  //
  // Check if read extent contains a valid tag identifier for AED.
  //
  if (DescriptorTag->TagIdentifier != UdfAllocationExtentDescriptor) {
    Status = EFI_VOLUME_CORRUPTED;
    goto Exit;
  }

  //
  // Get AED's block offset and its length.
  //
  *Offset = MultU64x32 (Lsn, LogicalBlockSize) +
    sizeof (UDF_ALLOCATION_EXTENT_DESCRIPTOR);
  *Length = AllocExtDesc->LengthOfAllocationDescriptors;

Exit:
  FreePool (Data);

  return Status;
}

/**
  Read Allocation Extent Descriptor into memory.

  @param[in]  BlockIo             BlockIo interface.
  @param[in]  DiskIo              DiskIo interface.
  @param[in]  Volume              Volume information pointer.
  @param[in]  ParentIcb           Long Allocation Descriptor pointer.
  @param[in]  RecordingFlags      Flag to indicate the type of descriptor.
  @param[in]  Ad                  Allocation Descriptor pointer.
  @param[out] Data                Buffer that contains the Allocation Extent
                                  Descriptor.
  @param[out] Length              Length of Data.

  @retval EFI_SUCCESS             The Allocation Extent Descriptor was read.
  @retval EFI_OUT_OF_RESOURCES    The Allocation Extent Descriptor was not read
                                  due to lack of resources.
  @retval other                   Fail to read the disk.

**/
EFI_STATUS
GetAedAdsData (
  IN   EFI_BLOCK_IO_PROTOCOL           *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL            *DiskIo,
  IN   UDF_VOLUME_INFO                 *Volume,
  IN   UDF_LONG_ALLOCATION_DESCRIPTOR  *ParentIcb,
  IN   UDF_FE_RECORDING_FLAGS          RecordingFlags,
  IN   VOID                            *Ad,
  OUT  VOID                            **Data,
  OUT  UINT64                          *Length
  )
{
  EFI_STATUS  Status;
  UINT64      Offset;

  //
  // Get AED's offset + length.
  //
  Status = GetAedAdsOffset (
    BlockIo,
    DiskIo,
    Volume,
    ParentIcb,
    RecordingFlags,
    Ad,
    &Offset,
    Length
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate buffer to read in AED's data.
  //
  *Data = AllocatePool ((UINTN) (*Length));
  if (*Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    Offset,
    (UINTN) (*Length),
    *Data
    );
}

/**
  Function used to serialise reads of Allocation Descriptors.

  @param[in]      RecordingFlags  Flag to indicate the type of descriptor.
  @param[in]      Ad              Allocation Descriptor pointer.
  @param[in, out] Buffer          Buffer to hold the next Allocation Descriptor.
  @param[in]      Length          Length of Buffer.

  @retval EFI_SUCCESS             Buffer was grown to hold the next Allocation
                                  Descriptor.
  @retval EFI_OUT_OF_RESOURCES    Buffer was not grown due to lack of resources.

**/
EFI_STATUS
GrowUpBufferToNextAd (
  IN      UDF_FE_RECORDING_FLAGS  RecordingFlags,
  IN      VOID                    *Ad,
  IN OUT  VOID                    **Buffer,
  IN      UINT64                  Length
  )
{
  UINT32 ExtentLength;

  ExtentLength = GET_EXTENT_LENGTH (RecordingFlags, Ad);

  if (*Buffer == NULL) {
    *Buffer = AllocatePool (ExtentLength);
    if (*Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    *Buffer = ReallocatePool ((UINTN) Length, (UINTN) (Length + ExtentLength), *Buffer);
    if (*Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
}

/**
  Read data or size of either a File Entry or an Extended File Entry.

  @param[in]      BlockIo         BlockIo interface.
  @param[in]      DiskIo          DiskIo interface.
  @param[in]      Volume          Volume information pointer.
  @param[in]      ParentIcb       Long Allocation Descriptor pointer.
  @param[in]      FileEntryData   FE/EFE structure pointer.
  @param[in, out] ReadFileInfo    Read file information pointer.

  @retval EFI_SUCCESS             Data or size of a FE/EFE was read.
  @retval EFI_OUT_OF_RESOURCES    Data or size of a FE/EFE was not read due to
                                  lack of resources.
  @retval EFI_INVALID_PARAMETER   The read file flag given in ReadFileInfo is
                                  invalid.
  @retval EFI_UNSUPPORTED         The FE recording flag given in FileEntryData
                                  is not supported.
  @retval other                   Data or size of a FE/EFE was not read.

**/
EFI_STATUS
ReadFile (
  IN      EFI_BLOCK_IO_PROTOCOL           *BlockIo,
  IN      EFI_DISK_IO_PROTOCOL            *DiskIo,
  IN      UDF_VOLUME_INFO                 *Volume,
  IN      UDF_LONG_ALLOCATION_DESCRIPTOR  *ParentIcb,
  IN      VOID                            *FileEntryData,
  IN OUT  UDF_READ_FILE_INFO              *ReadFileInfo
  )
{
  EFI_STATUS              Status;
  UINT32                  LogicalBlockSize;
  VOID                    *Data;
  VOID                    *DataBak;
  UINT64                  Length;
  VOID                    *Ad;
  UINT64                  AdOffset;
  UINT64                  Lsn;
  BOOLEAN                 DoFreeAed;
  UINT64                  FilePosition;
  UINT64                  Offset;
  UINT64                  DataOffset;
  UINT64                  BytesLeft;
  UINT64                  DataLength;
  BOOLEAN                 FinishedSeeking;
  UINT32                  ExtentLength;
  UDF_FE_RECORDING_FLAGS  RecordingFlags;

  LogicalBlockSize  = Volume->LogicalVolDesc.LogicalBlockSize;
  DoFreeAed         = FALSE;

  //
  // set BytesLeft to suppress incorrect compiler/analyzer warnings
  //
  BytesLeft = 0;
  DataOffset = 0;
  FilePosition = 0;
  FinishedSeeking = FALSE;
  Data = NULL;

  switch (ReadFileInfo->Flags) {
  case ReadFileGetFileSize:
  case ReadFileAllocateAndRead:
    //
    // Initialise ReadFileInfo structure for either getting file size, or
    // reading file's recorded data.
    //
    ReadFileInfo->ReadLength = 0;
    ReadFileInfo->FileData = NULL;
    break;
  case ReadFileSeekAndRead:
    //
    // About to seek a file and/or read its data.
    //
    Length = ReadFileInfo->FileSize - ReadFileInfo->FilePosition;
    if (ReadFileInfo->FileDataSize > Length) {
      //
      // About to read beyond the EOF -- truncate it.
      //
      ReadFileInfo->FileDataSize = Length;
    }

    //
    // Initialise data to start seeking and/or reading a file.
    //
    BytesLeft = ReadFileInfo->FileDataSize;
    DataOffset = 0;
    FilePosition = 0;
    FinishedSeeking = FALSE;

    break;
  }

  RecordingFlags = GET_FE_RECORDING_FLAGS (FileEntryData);
  switch (RecordingFlags) {
  case InlineData:
    //
    // There are no extents for this FE/EFE. All data is inline.
    //
    Status = GetFileEntryData (FileEntryData, Volume->FileEntrySize, &Data, &Length);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (ReadFileInfo->Flags == ReadFileGetFileSize) {
      ReadFileInfo->ReadLength = Length;
    } else if (ReadFileInfo->Flags == ReadFileAllocateAndRead) {
      //
      // Allocate buffer for starting read data.
      //
      ReadFileInfo->FileData = AllocatePool ((UINTN) Length);
      if (ReadFileInfo->FileData == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Read all inline data into ReadFileInfo->FileData
      //
      CopyMem (ReadFileInfo->FileData, Data, (UINTN) Length);
      ReadFileInfo->ReadLength = Length;
    } else if (ReadFileInfo->Flags == ReadFileSeekAndRead) {
      //
      // If FilePosition is non-zero, seek file to FilePosition, read
      // FileDataSize bytes and then updates FilePosition.
      //
      CopyMem (
        ReadFileInfo->FileData,
        (VOID *)((UINT8 *)Data + ReadFileInfo->FilePosition),
        (UINTN) ReadFileInfo->FileDataSize
        );

      ReadFileInfo->FilePosition += ReadFileInfo->FileDataSize;
    } else {
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    }

    Status = EFI_SUCCESS;
    break;

  case LongAdsSequence:
  case ShortAdsSequence:
    //
    // This FE/EFE contains a run of Allocation Descriptors. Get data + size
    // for start reading them out.
    //
    Status = GetAdsInformation (FileEntryData, Volume->FileEntrySize, &Data, &Length);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    AdOffset = 0;

    for (;;) {
      //
      // Read AD.
      //
      Status = GetAllocationDescriptor (
        RecordingFlags,
        Data,
        &AdOffset,
        Length,
        &Ad
        );
      if (Status == EFI_DEVICE_ERROR) {
        Status = EFI_SUCCESS;
        goto Done;
      }

      //
      // Check if AD is an indirect AD. If so, read Allocation Extent
      // Descriptor and its extents (ADs).
      //
      if (GET_EXTENT_FLAGS (RecordingFlags, Ad) == ExtentIsNextExtent) {
        DataBak = Data;
        Status = GetAedAdsData (
          BlockIo,
          DiskIo,
          Volume,
          ParentIcb,
          RecordingFlags,
          Ad,
          &Data,
          &Length
          );

        if (!DoFreeAed) {
          DoFreeAed = TRUE;
        } else {
          FreePool (DataBak);
        }

        if (EFI_ERROR (Status)) {
          goto Error_Get_Aed;
        }
        ASSERT (Data != NULL);

        AdOffset = 0;
        continue;
      }

      ExtentLength = GET_EXTENT_LENGTH (RecordingFlags, Ad);

      Status = GetAllocationDescriptorLsn (RecordingFlags,
                                           Volume,
                                           ParentIcb,
                                           Ad,
                                           &Lsn);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      switch (ReadFileInfo->Flags) {
      case ReadFileGetFileSize:
        ReadFileInfo->ReadLength += ExtentLength;
        break;
      case ReadFileAllocateAndRead:
        //
        // Increase FileData (if necessary) to read next extent.
        //
        Status = GrowUpBufferToNextAd (
          RecordingFlags,
          Ad,
          &ReadFileInfo->FileData,
          ReadFileInfo->ReadLength
          );
        if (EFI_ERROR (Status)) {
          goto Error_Alloc_Buffer_To_Next_Ad;
        }

        //
        // Read extent's data into FileData.
        //
        Status = DiskIo->ReadDisk (
          DiskIo,
          BlockIo->Media->MediaId,
          MultU64x32 (Lsn, LogicalBlockSize),
          ExtentLength,
          (VOID *)((UINT8 *)ReadFileInfo->FileData +
                   ReadFileInfo->ReadLength)
          );
        if (EFI_ERROR (Status)) {
          goto Error_Read_Disk_Blk;
        }

        ReadFileInfo->ReadLength += ExtentLength;
        break;
      case ReadFileSeekAndRead:
        //
        // Seek file first before reading in its data.
        //
        if (FinishedSeeking) {
          Offset = 0;
          goto Skip_File_Seek;
        }

        if (FilePosition + ExtentLength < ReadFileInfo->FilePosition) {
          FilePosition += ExtentLength;
          goto Skip_Ad;
        }

        if (FilePosition + ExtentLength > ReadFileInfo->FilePosition) {
          Offset = ReadFileInfo->FilePosition - FilePosition;
        } else {
          Offset = 0;
        }

        //
        // Done with seeking file. Start reading its data.
        //
        FinishedSeeking = TRUE;

      Skip_File_Seek:
        //
        // Make sure we don't read more data than really wanted.
        //
        if (ExtentLength - Offset > BytesLeft) {
          DataLength = BytesLeft;
        } else {
          DataLength = ExtentLength - Offset;
        }

        //
        // Read extent's data into FileData.
        //
        Status = DiskIo->ReadDisk (
          DiskIo,
          BlockIo->Media->MediaId,
          Offset + MultU64x32 (Lsn, LogicalBlockSize),
          (UINTN) DataLength,
          (VOID *)((UINT8 *)ReadFileInfo->FileData +
                   DataOffset)
          );
        if (EFI_ERROR (Status)) {
          goto Error_Read_Disk_Blk;
        }

        //
        // Update current file's position.
        //
        DataOffset += DataLength;
        ReadFileInfo->FilePosition += DataLength;

        BytesLeft -= DataLength;
        if (BytesLeft == 0) {
          //
          // There is no more file data to read.
          //
          Status = EFI_SUCCESS;
          goto Done;
        }

        break;
      }

    Skip_Ad:
      //
      // Point to the next AD (extent).
      //
      AdOffset += AD_LENGTH (RecordingFlags);
    }

    break;
  case ExtendedAdsSequence:
     // FIXME: Not supported. Got no volume with it, yet.
    ASSERT (FALSE);
    Status = EFI_UNSUPPORTED;
    break;

  default:
    //
    // A flag value reserved by the ECMA-167 standard (3rd Edition - June
    // 1997); 14.6 ICB Tag; 14.6.8 Flags (RBP 18); was found.
    //
    Status = EFI_UNSUPPORTED;
    break;
  }

Done:
  if (DoFreeAed) {
    FreePool (Data);
  }

  return Status;

Error_Read_Disk_Blk:
Error_Alloc_Buffer_To_Next_Ad:
  if (ReadFileInfo->Flags != ReadFileSeekAndRead) {
    FreePool (ReadFileInfo->FileData);
  }

  if (DoFreeAed) {
    FreePool (Data);
  }

Error_Get_Aed:
  return Status;
}

/**
  Find a file by its filename from a given Parent file.

  @param[in]  BlockIo             BlockIo interface.
  @param[in]  DiskIo              DiskIo interface.
  @param[in]  Volume              Volume information pointer.
  @param[in]  FileName            File name string.
  @param[in]  Parent              Parent directory file.
  @param[in]  Icb                 Long Allocation Descriptor pointer.
  @param[out] File                Found file.

  @retval EFI_SUCCESS             The file was found.
  @retval EFI_INVALID_PARAMETER   One or more input parameters are invalid.
  @retval EFI_NOT_FOUND           The file was not found.

**/
EFI_STATUS
InternalFindFile (
  IN   EFI_BLOCK_IO_PROTOCOL           *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL            *DiskIo,
  IN   UDF_VOLUME_INFO                 *Volume,
  IN   CHAR16                          *FileName,
  IN   UDF_FILE_INFO                   *Parent,
  IN   UDF_LONG_ALLOCATION_DESCRIPTOR  *Icb,
  OUT  UDF_FILE_INFO                   *File
  )
{
  EFI_STATUS                      Status;
  UDF_FILE_IDENTIFIER_DESCRIPTOR  *FileIdentifierDesc;
  UDF_READ_DIRECTORY_INFO         ReadDirInfo;
  BOOLEAN                         Found;
  CHAR16                          FoundFileName[UDF_FILENAME_LENGTH];
  VOID                            *CompareFileEntry;

  //
  // Check if both Parent->FileIdentifierDesc and Icb are NULL.
  //
  if ((Parent->FileIdentifierDesc == NULL) && (Icb == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check if parent file is really directory.
  //
  if (FE_ICB_FILE_TYPE (Parent->FileEntry) != UdfFileEntryDirectory) {
    return EFI_NOT_FOUND;
  }

  //
  // If FileName is current file or working directory, just duplicate Parent's
  // FE/EFE and FID descriptors.
  //
  if (StrCmp (FileName, L".") == 0) {
    if (Parent->FileIdentifierDesc == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    DuplicateFe (BlockIo, Volume, Parent->FileEntry, &File->FileEntry);
    if (File->FileEntry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    DuplicateFid (Parent->FileIdentifierDesc, &File->FileIdentifierDesc);
    if (File->FileIdentifierDesc == NULL) {
      FreePool (File->FileEntry);
      return EFI_OUT_OF_RESOURCES;
    }

    return EFI_SUCCESS;
  }

  //
  // Start directory listing.
  //
  ZeroMem ((VOID *)&ReadDirInfo, sizeof (UDF_READ_DIRECTORY_INFO));
  Found = FALSE;

  for (;;) {
    Status = ReadDirectoryEntry (
      BlockIo,
      DiskIo,
      Volume,
      (Parent->FileIdentifierDesc != NULL) ?
      &Parent->FileIdentifierDesc->Icb :
      Icb,
      Parent->FileEntry,
      &ReadDirInfo,
      &FileIdentifierDesc
      );
    if (EFI_ERROR (Status)) {
      if (Status == EFI_DEVICE_ERROR) {
        Status = EFI_NOT_FOUND;
      }

      break;
    }
    //
    // After calling function ReadDirectoryEntry(), if 'FileIdentifierDesc' is
    // NULL, then the 'Status' must be EFI_OUT_OF_RESOURCES. Hence, if the code
    // reaches here, 'FileIdentifierDesc' must be not NULL.
    //
    // The ASSERT here is for addressing a false positive NULL pointer
    // dereference issue raised from static analysis.
    //
    ASSERT (FileIdentifierDesc != NULL);

    if (FileIdentifierDesc->FileCharacteristics & PARENT_FILE) {
      //
      // This FID contains the location (FE/EFE) of the parent directory of this
      // directory (Parent), and if FileName is either ".." or "\\", then it's
      // the expected FID.
      //
      if (StrCmp (FileName, L"..") == 0 || StrCmp (FileName, L"\\") == 0) {
        Found = TRUE;
        break;
      }
    } else {
      Status = GetFileNameFromFid (FileIdentifierDesc, ARRAY_SIZE (FoundFileName), FoundFileName);
      if (EFI_ERROR (Status)) {
        break;
      }

      if (StrCmp (FileName, FoundFileName) == 0) {
        //
        // FID has been found. Prepare to find its respective FE/EFE.
        //
        Found = TRUE;
        break;
      }
    }

    FreePool ((VOID *)FileIdentifierDesc);
  }

  if (ReadDirInfo.DirectoryData != NULL) {
    //
    // Free all allocated resources for the directory listing.
    //
    FreePool (ReadDirInfo.DirectoryData);
  }

  if (Found) {
    Status = EFI_SUCCESS;

    File->FileIdentifierDesc = FileIdentifierDesc;

    //
    // If the requested file is root directory, then the FE/EFE was already
    // retrieved in UdfOpenVolume() function, thus no need to find it again.
    //
    // Otherwise, find FE/EFE from the respective FID.
    //
    if (StrCmp (FileName, L"\\") != 0) {
      Status = FindFileEntry (
        BlockIo,
        DiskIo,
        Volume,
        &FileIdentifierDesc->Icb,
        &CompareFileEntry
        );
      if (EFI_ERROR (Status)) {
        goto Error_Find_Fe;
      }

      //
      // Make sure that both Parent's FE/EFE and found FE/EFE are not equal.
      //
      if (CompareMem ((VOID *)Parent->FileEntry, (VOID *)CompareFileEntry,
                      Volume->FileEntrySize) != 0) {
        File->FileEntry = CompareFileEntry;
      } else {
        FreePool ((VOID *)FileIdentifierDesc);
        FreePool ((VOID *)CompareFileEntry);
        Status = EFI_NOT_FOUND;
      }
    }
  }

  return Status;

Error_Find_Fe:
  FreePool ((VOID *)FileIdentifierDesc);

  return Status;
}

/**
  Read volume information on a medium which contains a valid UDF file system.

  @param[in]   BlockIo  BlockIo interface.
  @param[in]   DiskIo   DiskIo interface.
  @param[out]  Volume   UDF volume information structure.

  @retval EFI_SUCCESS          Volume information read.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_OUT_OF_RESOURCES The volume was not read due to lack of resources.

**/
EFI_STATUS
ReadUdfVolumeInformation (
  IN   EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL   *DiskIo,
  OUT  UDF_VOLUME_INFO        *Volume
  )
{
  EFI_STATUS Status;

  //
  // Read all necessary UDF volume information and keep it private to the driver
  //
  Status = ReadVolumeFileStructure (
    BlockIo,
    DiskIo,
    Volume
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find File Set Descriptor
  //
  Status = FindFileSetDescriptor (BlockIo, DiskIo, Volume);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}

/**
  Find the root directory on an UDF volume.

  @param[in]   BlockIo  BlockIo interface.
  @param[in]   DiskIo   DiskIo interface.
  @param[in]   Volume   UDF volume information structure.
  @param[out]  File     Root directory file.

  @retval EFI_SUCCESS          Root directory found.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_OUT_OF_RESOURCES The root directory was not found due to lack of
                               resources.

**/
EFI_STATUS
FindRootDirectory (
  IN   EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL   *DiskIo,
  IN   UDF_VOLUME_INFO        *Volume,
  OUT  UDF_FILE_INFO          *File
  )
{
  EFI_STATUS     Status;
  UDF_FILE_INFO  Parent;

  Status = FindFileEntry (
    BlockIo,
    DiskIo,
    Volume,
    &Volume->FileSetDesc.RootDirectoryIcb,
    &File->FileEntry
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Parent.FileEntry = File->FileEntry;
  Parent.FileIdentifierDesc = NULL;

  Status = FindFile (
    BlockIo,
    DiskIo,
    Volume,
    L"\\",
    NULL,
    &Parent,
    &Volume->FileSetDesc.RootDirectoryIcb,
    File
    );
  if (EFI_ERROR (Status)) {
    FreePool (File->FileEntry);
  }

  return Status;
}

/**
  Find either a File Entry or a Extended File Entry from a given ICB.

  @param[in]   BlockIo    BlockIo interface.
  @param[in]   DiskIo     DiskIo interface.
  @param[in]   Volume     UDF volume information structure.
  @param[in]   Icb        ICB of the FID.
  @param[out]  FileEntry  File Entry or Extended File Entry.

  @retval EFI_SUCCESS          File Entry or Extended File Entry found.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_OUT_OF_RESOURCES The FE/EFE entry was not found due to lack of
                               resources.

**/
EFI_STATUS
FindFileEntry (
  IN   EFI_BLOCK_IO_PROTOCOL           *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL            *DiskIo,
  IN   UDF_VOLUME_INFO                 *Volume,
  IN   UDF_LONG_ALLOCATION_DESCRIPTOR  *Icb,
  OUT  VOID                            **FileEntry
  )
{
  EFI_STATUS          Status;
  UINT64              Lsn;
  UINT32              LogicalBlockSize;
  UDF_DESCRIPTOR_TAG  *DescriptorTag;
  VOID                *ReadBuffer;

  Status = GetLongAdLsn (Volume, Icb, &Lsn);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LogicalBlockSize  = Volume->LogicalVolDesc.LogicalBlockSize;

  ReadBuffer = AllocateZeroPool (Volume->FileEntrySize);
  if (ReadBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read extent.
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 (Lsn, LogicalBlockSize),
    Volume->FileEntrySize,
    ReadBuffer
    );
  if (EFI_ERROR (Status)) {
    goto Error_Read_Disk_Blk;
  }

  DescriptorTag = ReadBuffer;

  //
  // Check if the read extent contains a valid Tag Identifier for the expected
  // FE/EFE.
  //
  if (DescriptorTag->TagIdentifier != UdfFileEntry &&
      DescriptorTag->TagIdentifier != UdfExtendedFileEntry) {
    Status = EFI_VOLUME_CORRUPTED;
    goto Error_Invalid_Fe;
  }

  *FileEntry = ReadBuffer;
  return EFI_SUCCESS;

Error_Invalid_Fe:
Error_Read_Disk_Blk:
  FreePool (ReadBuffer);

  return Status;
}

/**
  Find a file given its absolute path on an UDF volume.

  @param[in]   BlockIo   BlockIo interface.
  @param[in]   DiskIo    DiskIo interface.
  @param[in]   Volume    UDF volume information structure.
  @param[in]   FilePath  File's absolute path.
  @param[in]   Root      Root directory file.
  @param[in]   Parent    Parent directory file.
  @param[in]   Icb       ICB of Parent.
  @param[out]  File      Found file.

  @retval EFI_SUCCESS          FilePath was found.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_OUT_OF_RESOURCES The FilePath file was not found due to lack of
                               resources.

**/
EFI_STATUS
FindFile (
  IN   EFI_BLOCK_IO_PROTOCOL           *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL            *DiskIo,
  IN   UDF_VOLUME_INFO                 *Volume,
  IN   CHAR16                          *FilePath,
  IN   UDF_FILE_INFO                   *Root,
  IN   UDF_FILE_INFO                   *Parent,
  IN   UDF_LONG_ALLOCATION_DESCRIPTOR  *Icb,
  OUT  UDF_FILE_INFO                   *File
  )
{
  EFI_STATUS     Status;
  CHAR16         FileName[UDF_FILENAME_LENGTH];
  CHAR16         *FileNamePointer;
  UDF_FILE_INFO  PreviousFile;
  VOID           *FileEntry;

  Status = EFI_NOT_FOUND;

  CopyMem ((VOID *)&PreviousFile, (VOID *)Parent, sizeof (UDF_FILE_INFO));
  while (*FilePath != L'\0') {
    FileNamePointer = FileName;
    while (*FilePath != L'\0' && *FilePath != L'\\') {
      if ((((UINTN)FileNamePointer - (UINTN)FileName) / sizeof (CHAR16)) >=
          (ARRAY_SIZE (FileName) - 1)) {
        return EFI_NOT_FOUND;
      }

      *FileNamePointer++ = *FilePath++;
    }

    *FileNamePointer = L'\0';
    if (FileName[0] == L'\0') {
      //
      // Open root directory.
      //
      if (Root == NULL) {
        //
        // There is no file found for the root directory yet. So, find only its
        // FID by now.
        //
        // See UdfOpenVolume() function.
        //
        Status = InternalFindFile (BlockIo,
                                   DiskIo,
                                   Volume,
                                   L"\\",
                                   &PreviousFile,
                                   Icb,
                                   File);
      } else {
        //
        // We've already a file pointer (Root) for the root directory. Duplicate
        // its FE/EFE and FID descriptors.
        //
        Status = EFI_SUCCESS;
        DuplicateFe (BlockIo, Volume, Root->FileEntry, &File->FileEntry);
        if (File->FileEntry == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
        } else {
          //
          // File->FileEntry is not NULL.
          //
          DuplicateFid (Root->FileIdentifierDesc, &File->FileIdentifierDesc);
          if (File->FileIdentifierDesc == NULL) {
            FreePool (File->FileEntry);
            Status = EFI_OUT_OF_RESOURCES;
          }
        }
      }
    } else {
      //
      // No root directory. Find filename from the current directory.
      //
      Status = InternalFindFile (BlockIo,
                                 DiskIo,
                                 Volume,
                                 FileName,
                                 &PreviousFile,
                                 Icb,
                                 File);
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // If the found file is a symlink, then find its respective FE/EFE and
    // FID descriptors.
    //
    if (FE_ICB_FILE_TYPE (File->FileEntry) == UdfFileEntrySymlink) {
      FreePool ((VOID *)File->FileIdentifierDesc);

      FileEntry = File->FileEntry;

      Status = ResolveSymlink (BlockIo,
                               DiskIo,
                               Volume,
                               &PreviousFile,
                               FileEntry,
                               File);

      FreePool (FileEntry);

      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    if (CompareMem ((VOID *)&PreviousFile, (VOID *)Parent,
                    sizeof (UDF_FILE_INFO)) != 0) {
      CleanupFileInformation (&PreviousFile);
    }

    CopyMem ((VOID *)&PreviousFile, (VOID *)File, sizeof (UDF_FILE_INFO));
    if (*FilePath != L'\0' && *FilePath == L'\\') {
      FilePath++;
    }
  }

  return Status;
}

/**
  Read a directory entry at a time on an UDF volume.

  @param[in]      BlockIo        BlockIo interface.
  @param[in]      DiskIo         DiskIo interface.
  @param[in]      Volume         UDF volume information structure.
  @param[in]      ParentIcb      ICB of the parent file.
  @param[in]      FileEntryData  FE/EFE of the parent file.
  @param[in, out] ReadDirInfo    Next read directory listing structure
                                 information.
  @param[out]     FoundFid       File Identifier Descriptor pointer.

  @retval EFI_SUCCESS          Directory entry read.
  @retval EFI_UNSUPPORTED      Extended Allocation Descriptors not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_OUT_OF_RESOURCES The directory entry was not read due to lack of
                               resources.

**/
EFI_STATUS
ReadDirectoryEntry (
  IN      EFI_BLOCK_IO_PROTOCOL           *BlockIo,
  IN      EFI_DISK_IO_PROTOCOL            *DiskIo,
  IN      UDF_VOLUME_INFO                 *Volume,
  IN      UDF_LONG_ALLOCATION_DESCRIPTOR  *ParentIcb,
  IN      VOID                            *FileEntryData,
  IN OUT  UDF_READ_DIRECTORY_INFO         *ReadDirInfo,
  OUT     UDF_FILE_IDENTIFIER_DESCRIPTOR  **FoundFid
  )
{
  EFI_STATUS                      Status;
  UDF_READ_FILE_INFO              ReadFileInfo;
  UDF_FILE_IDENTIFIER_DESCRIPTOR  *FileIdentifierDesc;

  if (ReadDirInfo->DirectoryData == NULL) {
    //
    // The directory's recorded data has not been read yet. So let's cache it
    // into memory and the next calls won't need to read it again.
    //
    ReadFileInfo.Flags = ReadFileAllocateAndRead;

    Status = ReadFile (
      BlockIo,
      DiskIo,
      Volume,
      ParentIcb,
      FileEntryData,
      &ReadFileInfo
      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Fill in ReadDirInfo structure with the read directory's data information.
    //
    ReadDirInfo->DirectoryData = ReadFileInfo.FileData;
    ReadDirInfo->DirectoryLength = ReadFileInfo.ReadLength;
  }

  do {
    if (ReadDirInfo->FidOffset >= ReadDirInfo->DirectoryLength) {
      //
      // There are no longer FIDs for this directory. By returning
      // EFI_DEVICE_ERROR to the callee will indicate end of directory
      // listening.
      //
      return EFI_DEVICE_ERROR;
    }

    //
    // Get FID for this entry.
    //
    FileIdentifierDesc = GET_FID_FROM_ADS (ReadDirInfo->DirectoryData,
                                           ReadDirInfo->FidOffset);
    //
    // Update FidOffset to point to next FID.
    //
    ReadDirInfo->FidOffset += GetFidDescriptorLength (FileIdentifierDesc);
  } while (FileIdentifierDesc->FileCharacteristics & DELETED_FILE);

  DuplicateFid (FileIdentifierDesc, FoundFid);
  if (*FoundFid == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Get a filename (encoded in OSTA-compressed format) from a File Identifier
  Descriptor on an UDF volume.

  @attention This is boundary function that may receive untrusted input.
  @attention The input is from FileSystem.

  The File Identifier Descriptor is external input, so this routine will do
  basic validation for File Identifier Descriptor and report status.

  @param[in]   FileIdentifierDesc  File Identifier Descriptor pointer.
  @param[in]   CharMax             The maximum number of FileName Unicode char,
                                   including terminating null char.
  @param[out]  FileName            Decoded filename.

  @retval EFI_SUCCESS           Filename decoded and read.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_BUFFER_TOO_SMALL  The string buffer FileName cannot hold the
                                decoded filename.
**/
EFI_STATUS
GetFileNameFromFid (
  IN   UDF_FILE_IDENTIFIER_DESCRIPTOR  *FileIdentifierDesc,
  IN   UINTN                           CharMax,
  OUT  CHAR16                          *FileName
  )
{
  UINT8  *OstaCompressed;
  UINT8  CompressionId;
  UINT8  Length;
  UINTN  Index;
  CHAR16 *FileNameBak;

  if (CharMax == 0) {
    return EFI_BUFFER_TOO_SMALL;
  }

  OstaCompressed =
    (UINT8 *)(
      (UINT8 *)FileIdentifierDesc->Data +
      FileIdentifierDesc->LengthOfImplementationUse
      );

  CompressionId = OstaCompressed[0];
  if (!IS_VALID_COMPRESSION_ID (CompressionId)) {
    return EFI_VOLUME_CORRUPTED;
  }

  FileNameBak = FileName;

  //
  // Decode filename.
  //
  Length = FileIdentifierDesc->LengthOfFileIdentifier;
  if (CompressionId == 16) {
    if (((UINTN)Length >> 1) > CharMax) {
      return EFI_BUFFER_TOO_SMALL;
    }
  } else {
    if ((Length != 0) && ((UINTN)Length - 1 > CharMax)) {
      return EFI_BUFFER_TOO_SMALL;
    }
  }

  for (Index = 1; Index < Length; Index++) {
    if (CompressionId == 16) {
      *FileName = OstaCompressed[Index++] << 8;
    } else {
      *FileName = 0;
    }

    if (Index < Length) {
      *FileName |= (CHAR16)(OstaCompressed[Index]);
    }

    FileName++;
  }

  Index = ((UINTN)FileName - (UINTN)FileNameBak) / sizeof (CHAR16);
  if (Index > CharMax - 1) {
    Index = CharMax - 1;
  }
  FileNameBak[Index] = L'\0';

  return EFI_SUCCESS;
}

/**
  Resolve a symlink file on an UDF volume.

  @attention This is boundary function that may receive untrusted input.
  @attention The input is from FileSystem.

  The Path Component is external input, so this routine will do basic
  validation for Path Component and report status.

  @param[in]   BlockIo        BlockIo interface.
  @param[in]   DiskIo         DiskIo interface.
  @param[in]   Volume         UDF volume information structure.
  @param[in]   Parent         Parent file.
  @param[in]   FileEntryData  FE/EFE structure pointer.
  @param[out]  File           Resolved file.

  @retval EFI_SUCCESS          Symlink file resolved.
  @retval EFI_UNSUPPORTED      Extended Allocation Descriptors not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_OUT_OF_RESOURCES The symlink file was not resolved due to lack of
                               resources.

**/
EFI_STATUS
ResolveSymlink (
  IN   EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL   *DiskIo,
  IN   UDF_VOLUME_INFO        *Volume,
  IN   UDF_FILE_INFO          *Parent,
  IN   VOID                   *FileEntryData,
  OUT  UDF_FILE_INFO          *File
  )
{
  EFI_STATUS          Status;
  UDF_READ_FILE_INFO  ReadFileInfo;
  UINT8               *Data;
  UINT64              Length;
  UINT8               *EndData;
  UDF_PATH_COMPONENT  *PathComp;
  UINT8               PathCompLength;
  CHAR16              FileName[UDF_FILENAME_LENGTH];
  CHAR16              *Char;
  UINTN               Index;
  UINT8               CompressionId;
  UDF_FILE_INFO       PreviousFile;
  BOOLEAN             NotParent;
  BOOLEAN             NotFile;

  ZeroMem ((VOID *)File, sizeof (UDF_FILE_INFO));

  //
  // Symlink files on UDF volumes do not contain so much data other than
  // Path Components which resolves to real filenames, so it's OK to read in
  // all its data here -- usually the data will be inline with the FE/EFE for
  // lower filenames.
  //
  ReadFileInfo.Flags = ReadFileAllocateAndRead;

  Status = ReadFile (
    BlockIo,
    DiskIo,
    Volume,
    &Parent->FileIdentifierDesc->Icb,
    FileEntryData,
    &ReadFileInfo
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Length = ReadFileInfo.ReadLength;

  Data = (UINT8 *)ReadFileInfo.FileData;
  EndData = Data + Length;

  CopyMem ((VOID *)&PreviousFile, (VOID *)Parent, sizeof (UDF_FILE_INFO));

  for (;;) {
    PathComp = (UDF_PATH_COMPONENT *)Data;

    PathCompLength = PathComp->LengthOfComponentIdentifier;

    switch (PathComp->ComponentType) {
    case 1:
      //
      // This Path Component specifies the root directory hierarchy subject to
      // agreement between the originator and recipient of the medium. Skip it.
      //
      // Fall through.
      //
    case 2:
      //
      // "\\." of the current directory. Read next Path Component.
      //
      goto Next_Path_Component;
    case 3:
      //
      // ".." (parent directory). Go to it.
      //
      CopyMem ((VOID *)FileName, L"..", 6);
      break;
    case 4:
      //
      // "." (current file). Duplicate both FE/EFE and FID of this file.
      //
      DuplicateFe (BlockIo, Volume, PreviousFile.FileEntry, &File->FileEntry);
      if (File->FileEntry == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error_Find_File;
      }

      DuplicateFid (PreviousFile.FileIdentifierDesc,
                    &File->FileIdentifierDesc);
      if (File->FileIdentifierDesc == NULL) {
        FreePool (File->FileEntry);
        Status = EFI_OUT_OF_RESOURCES;
        goto Error_Find_File;
      }
      goto Next_Path_Component;
    case 5:
      //
      // This Path Component identifies an object, either a file or a
      // directory or an alias.
      //
      // Decode it from the compressed data in ComponentIdentifier and find
      // respective path.
      //
      CompressionId = PathComp->ComponentIdentifier[0];
      if (!IS_VALID_COMPRESSION_ID (CompressionId)) {
        return EFI_VOLUME_CORRUPTED;
      }

      if ((UINTN)PathComp->ComponentIdentifier + PathCompLength > (UINTN)EndData) {
        return EFI_VOLUME_CORRUPTED;
      }

      Char = FileName;
      for (Index = 1; Index < PathCompLength; Index++) {
        if (CompressionId == 16) {
          *Char = *(UINT8 *)((UINT8 *)PathComp->ComponentIdentifier +
                          Index) << 8;
          Index++;
        } else {
          if (Index > ARRAY_SIZE (FileName)) {
            return EFI_UNSUPPORTED;
          }
          *Char = 0;
        }

        if (Index < Length) {
          *Char |= (CHAR16)(*(UINT8 *)((UINT8 *)PathComp->ComponentIdentifier + Index));
        }

        Char++;
      }

      Index = ((UINTN)Char - (UINTN)FileName) / sizeof (CHAR16);
      if (Index > ARRAY_SIZE (FileName) - 1) {
        Index = ARRAY_SIZE (FileName) - 1;
      }
      FileName[Index] = L'\0';
      break;
    default:
      //
      // According to the ECMA-167 standard (3rd Edition - June 1997), Section
      // 14.16.1.1, all other values are reserved.
      //
      Status = EFI_VOLUME_CORRUPTED;
      goto Error_Find_File;
    }

    //
    // Find file from the read filename in symlink's file data.
    //
    Status = InternalFindFile (
      BlockIo,
      DiskIo,
      Volume,
      FileName,
      &PreviousFile,
      NULL,
      File
      );
    if (EFI_ERROR (Status)) {
      goto Error_Find_File;
    }

  Next_Path_Component:
    Data += sizeof (UDF_PATH_COMPONENT) + PathCompLength;
    if (Data >= EndData) {
      break;
    }

    //
    // Check the content in the file info pointed by File.
    //
    if ((File->FileEntry == NULL) || (File->FileIdentifierDesc == NULL)) {
      Status = EFI_VOLUME_CORRUPTED;
      goto Error_Find_File;
    }

    NotParent = (CompareMem ((VOID *)&PreviousFile, (VOID *)Parent,
                 sizeof (UDF_FILE_INFO)) != 0);
    NotFile   = (CompareMem ((VOID *)&PreviousFile, (VOID *)File,
                 sizeof (UDF_FILE_INFO)) != 0);

    if (NotParent && NotFile) {
      CleanupFileInformation (&PreviousFile);
    }

    if (NotFile) {
      CopyMem ((VOID *)&PreviousFile, (VOID *)File, sizeof (UDF_FILE_INFO));
    }
  }

  //
  // Unmap the symlink file.
  //
  FreePool (ReadFileInfo.FileData);

  //
  // Check the content in the resolved file info.
  //
  if ((File->FileEntry == NULL) || (File->FileIdentifierDesc == NULL)) {
    return EFI_VOLUME_CORRUPTED;
  }

  return EFI_SUCCESS;

Error_Find_File:
  if (CompareMem ((VOID *)&PreviousFile, (VOID *)Parent,
                  sizeof (UDF_FILE_INFO)) != 0) {
    CleanupFileInformation (&PreviousFile);
  }

  FreePool (ReadFileInfo.FileData);

  return Status;
}

/**
  Clean up in-memory UDF file information.

  @param[in] File File information pointer.

**/
VOID
CleanupFileInformation (
  IN UDF_FILE_INFO *File
  )
{
  if (File->FileEntry != NULL) {
    FreePool (File->FileEntry);
  }
  if (File->FileIdentifierDesc != NULL) {
    FreePool ((VOID *)File->FileIdentifierDesc);
  }

  ZeroMem ((VOID *)File, sizeof (UDF_FILE_INFO));
}

/**
  Find a file from its absolute path on an UDF volume.

  @param[in]   BlockIo  BlockIo interface.
  @param[in]   DiskIo   DiskIo interface.
  @param[in]   Volume   UDF volume information structure.
  @param[in]   File     File information structure.
  @param[out]  Size     Size of the file.

  @retval EFI_SUCCESS          File size calculated and set in Size.
  @retval EFI_UNSUPPORTED      Extended Allocation Descriptors not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_OUT_OF_RESOURCES The file size was not calculated due to lack of
                               resources.

**/
EFI_STATUS
GetFileSize (
  IN   EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL   *DiskIo,
  IN   UDF_VOLUME_INFO        *Volume,
  IN   UDF_FILE_INFO          *File,
  OUT  UINT64                 *Size
  )
{
  EFI_STATUS          Status;
  UDF_READ_FILE_INFO  ReadFileInfo;

  ReadFileInfo.Flags = ReadFileGetFileSize;

  Status = ReadFile (
    BlockIo,
    DiskIo,
    Volume,
    &File->FileIdentifierDesc->Icb,
    File->FileEntry,
    &ReadFileInfo
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Size = ReadFileInfo.ReadLength;

  return EFI_SUCCESS;
}

/**
  Set information about a file on an UDF volume.

  @param[in]      File        File pointer.
  @param[in]      FileSize    Size of the file.
  @param[in]      FileName    Filename of the file.
  @param[in, out] BufferSize  Size of the returned file infomation.
  @param[out]     Buffer      Data of the returned file information.

  @retval EFI_SUCCESS          File information set.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_OUT_OF_RESOURCES The file information was not set due to lack of
                               resources.

**/
EFI_STATUS
SetFileInfo (
  IN      UDF_FILE_INFO  *File,
  IN      UINT64         FileSize,
  IN      CHAR16         *FileName,
  IN OUT  UINTN          *BufferSize,
  OUT     VOID           *Buffer
  )
{
  UINTN                    FileInfoLength;
  EFI_FILE_INFO            *FileInfo;
  UDF_FILE_ENTRY           *FileEntry;
  UDF_EXTENDED_FILE_ENTRY  *ExtendedFileEntry;
  UDF_DESCRIPTOR_TAG       *DescriptorTag;

  //
  // Calculate the needed size for the EFI_FILE_INFO structure.
  //
  FileInfoLength = sizeof (EFI_FILE_INFO) + ((FileName != NULL) ?
                                             StrSize (FileName) :
                                             sizeof (CHAR16));
  if (*BufferSize < FileInfoLength) {
    //
    // The given Buffer has no size enough for EFI_FILE_INFO structure.
    //
    *BufferSize = FileInfoLength;
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Buffer now contains room enough to store EFI_FILE_INFO structure.
  // Now, fill it in with all necessary information about the file.
  //
  FileInfo = (EFI_FILE_INFO *)Buffer;
  FileInfo->Size         = FileInfoLength;
  FileInfo->Attribute    &= ~EFI_FILE_VALID_ATTR;
  FileInfo->Attribute    |= EFI_FILE_READ_ONLY;

  if (IS_FID_DIRECTORY_FILE (File->FileIdentifierDesc)) {
    FileInfo->Attribute |= EFI_FILE_DIRECTORY;
  } else if (IS_FID_NORMAL_FILE (File->FileIdentifierDesc)) {
    FileInfo->Attribute |= EFI_FILE_ARCHIVE;
  }

  if (IS_FID_HIDDEN_FILE (File->FileIdentifierDesc)) {
    FileInfo->Attribute |= EFI_FILE_HIDDEN;
  }

  DescriptorTag = File->FileEntry;

  if (DescriptorTag->TagIdentifier == UdfFileEntry) {
    FileEntry = (UDF_FILE_ENTRY *)File->FileEntry;

    //
    // Check if FE has the system attribute set.
    //
    if (FileEntry->IcbTag.Flags & (1 << 10)) {
      FileInfo->Attribute |= EFI_FILE_SYSTEM;
    }

    FileInfo->FileSize      = FileSize;
    FileInfo->PhysicalSize  = FileSize;

    FileInfo->CreateTime.Year        = FileEntry->AccessTime.Year;
    FileInfo->CreateTime.Month       = FileEntry->AccessTime.Month;
    FileInfo->CreateTime.Day         = FileEntry->AccessTime.Day;
    FileInfo->CreateTime.Hour        = FileEntry->AccessTime.Hour;
    FileInfo->CreateTime.Minute      = FileEntry->AccessTime.Minute;
    FileInfo->CreateTime.Second      = FileEntry->AccessTime.Second;
    FileInfo->CreateTime.Nanosecond  =
                                   FileEntry->AccessTime.HundredsOfMicroseconds;

    FileInfo->LastAccessTime.Year        =
                                   FileEntry->AccessTime.Year;
    FileInfo->LastAccessTime.Month       =
                                   FileEntry->AccessTime.Month;
    FileInfo->LastAccessTime.Day         =
                                   FileEntry->AccessTime.Day;
    FileInfo->LastAccessTime.Hour        =
                                   FileEntry->AccessTime.Hour;
    FileInfo->LastAccessTime.Minute      =
                                   FileEntry->AccessTime.Minute;
    FileInfo->LastAccessTime.Second      =
                                   FileEntry->AccessTime.Second;
    FileInfo->LastAccessTime.Nanosecond  =
                                   FileEntry->AccessTime.HundredsOfMicroseconds;
  } else if (DescriptorTag->TagIdentifier == UdfExtendedFileEntry) {
    ExtendedFileEntry = (UDF_EXTENDED_FILE_ENTRY *)File->FileEntry;

    //
    // Check if EFE has the system attribute set.
    //
    if (ExtendedFileEntry->IcbTag.Flags & (1 << 10)) {
      FileInfo->Attribute |= EFI_FILE_SYSTEM;
    }

    FileInfo->FileSize      = FileSize;
    FileInfo->PhysicalSize  = FileSize;

    FileInfo->CreateTime.Year        = ExtendedFileEntry->CreationTime.Year;
    FileInfo->CreateTime.Month       = ExtendedFileEntry->CreationTime.Month;
    FileInfo->CreateTime.Day         = ExtendedFileEntry->CreationTime.Day;
    FileInfo->CreateTime.Hour        = ExtendedFileEntry->CreationTime.Hour;
    FileInfo->CreateTime.Minute      = ExtendedFileEntry->CreationTime.Second;
    FileInfo->CreateTime.Second      = ExtendedFileEntry->CreationTime.Second;
    FileInfo->CreateTime.Nanosecond  =
                           ExtendedFileEntry->AccessTime.HundredsOfMicroseconds;

    FileInfo->LastAccessTime.Year        =
                           ExtendedFileEntry->AccessTime.Year;
    FileInfo->LastAccessTime.Month       =
                           ExtendedFileEntry->AccessTime.Month;
    FileInfo->LastAccessTime.Day         =
                           ExtendedFileEntry->AccessTime.Day;
    FileInfo->LastAccessTime.Hour        =
                           ExtendedFileEntry->AccessTime.Hour;
    FileInfo->LastAccessTime.Minute      =
                           ExtendedFileEntry->AccessTime.Minute;
    FileInfo->LastAccessTime.Second      =
                           ExtendedFileEntry->AccessTime.Second;
    FileInfo->LastAccessTime.Nanosecond  =
                           ExtendedFileEntry->AccessTime.HundredsOfMicroseconds;
  }

  FileInfo->CreateTime.TimeZone      = EFI_UNSPECIFIED_TIMEZONE;
  FileInfo->CreateTime.Daylight      = EFI_TIME_ADJUST_DAYLIGHT;
  FileInfo->LastAccessTime.TimeZone  = EFI_UNSPECIFIED_TIMEZONE;
  FileInfo->LastAccessTime.Daylight  = EFI_TIME_ADJUST_DAYLIGHT;

  CopyMem ((VOID *)&FileInfo->ModificationTime,
           (VOID *)&FileInfo->LastAccessTime,
           sizeof (EFI_TIME));

  if (FileName != NULL) {
    StrCpyS (FileInfo->FileName, StrLen (FileName) + 1, FileName);
  } else {
    FileInfo->FileName[0] = '\0';
  }

  *BufferSize = FileInfoLength;

  return EFI_SUCCESS;
}

/**
  Get volume label of an UDF volume.

  @attention This is boundary function that may receive untrusted input.
  @attention The input is from FileSystem.

  The File Set Descriptor is external input, so this routine will do basic
  validation for File Set Descriptor and report status.

  @param[in]   Volume   Volume information pointer.
  @param[in]   CharMax  The maximum number of Unicode char in String,
                        including terminating null char.
  @param[out]  String   String buffer pointer to store the volume label.

  @retval EFI_SUCCESS           Volume label is returned.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_BUFFER_TOO_SMALL  The string buffer String cannot hold the
                                volume label.

**/
EFI_STATUS
GetVolumeLabel (
  IN   UDF_VOLUME_INFO  *Volume,
  IN   UINTN            CharMax,
  OUT  CHAR16           *String
  )
{
  UDF_FILE_SET_DESCRIPTOR  *FileSetDesc;
  UINTN                    Index;
  UINT8                    *OstaCompressed;
  UINT8                    CompressionId;
  CHAR16                   *StringBak;

  FileSetDesc = &Volume->FileSetDesc;

  OstaCompressed = &FileSetDesc->LogicalVolumeIdentifier[0];

  CompressionId = OstaCompressed[0];
  if (!IS_VALID_COMPRESSION_ID (CompressionId)) {
    return EFI_VOLUME_CORRUPTED;
  }

  StringBak = String;
  for (Index = 1; Index < 128; Index++) {
    if (CompressionId == 16) {
      if ((Index >> 1) > CharMax) {
        return EFI_BUFFER_TOO_SMALL;
      }

      *String = *(UINT8 *)(OstaCompressed + Index) << 8;
      Index++;
    } else {
      if (Index > CharMax) {
        return EFI_BUFFER_TOO_SMALL;
      }

      *String = 0;
    }

    if (Index < 128) {
      *String |= (CHAR16)(*(UINT8 *)(OstaCompressed + Index));
    }

    //
    // Unlike FID Identifiers, Logical Volume Identifier is stored in a
    // NULL-terminated OSTA compressed format, so we must check for the NULL
    // character.
    //
    if (*String == L'\0') {
      break;
    }

    String++;
  }

  Index = ((UINTN)String - (UINTN)StringBak) / sizeof (CHAR16);
  if (Index > CharMax - 1) {
    Index = CharMax - 1;
  }
  StringBak[Index] = L'\0';

  return EFI_SUCCESS;
}

/**
  Get volume and free space size information of an UDF volume.

  @attention This is boundary function that may receive untrusted input.
  @attention The input is from FileSystem.

  The Logical Volume Descriptor and the Logical Volume Integrity Descriptor are
  external inputs, so this routine will do basic validation for both descriptors
  and report status.

  @param[in]   BlockIo        BlockIo interface.
  @param[in]   DiskIo         DiskIo interface.
  @param[in]   Volume         UDF volume information structure.
  @param[out]  VolumeSize     Volume size.
  @param[out]  FreeSpaceSize  Free space size.

  @retval EFI_SUCCESS          Volume and free space size calculated.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_OUT_OF_RESOURCES The volume and free space size were not
                               calculated due to lack of resources.

**/
EFI_STATUS
GetVolumeSize (
  IN   EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL   *DiskIo,
  IN   UDF_VOLUME_INFO        *Volume,
  OUT  UINT64                 *VolumeSize,
  OUT  UINT64                 *FreeSpaceSize
  )
{
  EFI_STATUS                     Status;
  UDF_LOGICAL_VOLUME_DESCRIPTOR  *LogicalVolDesc;
  UDF_EXTENT_AD                  *ExtentAd;
  UINT64                         Lsn;
  UINT32                         LogicalBlockSize;
  UDF_LOGICAL_VOLUME_INTEGRITY   *LogicalVolInt;
  UDF_DESCRIPTOR_TAG             *DescriptorTag;
  UINTN                          Index;
  UINTN                          Length;
  UINT32                         LsnsNo;

  LogicalVolDesc = &Volume->LogicalVolDesc;

  ExtentAd = &LogicalVolDesc->IntegritySequenceExtent;

  if ((ExtentAd->ExtentLength == 0) ||
      (ExtentAd->ExtentLength < sizeof (UDF_LOGICAL_VOLUME_INTEGRITY))) {
    return EFI_VOLUME_CORRUPTED;
  }

  LogicalVolInt = AllocatePool (ExtentAd->ExtentLength);
  if (LogicalVolInt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get location of Logical Volume Integrity Descriptor
  //
  Lsn = (UINT64)ExtentAd->ExtentLocation - Volume->MainVdsStartLocation;

  LogicalBlockSize = LogicalVolDesc->LogicalBlockSize;

  //
  // Read disk block
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 (Lsn, LogicalBlockSize),
    ExtentAd->ExtentLength,
    LogicalVolInt
    );
  if (EFI_ERROR (Status)) {
    goto Out_Free;
  }

  DescriptorTag = &LogicalVolInt->DescriptorTag;

  //
  // Check if read block is a Logical Volume Integrity Descriptor
  //
  if (DescriptorTag->TagIdentifier != UdfLogicalVolumeIntegrityDescriptor) {
    Status = EFI_VOLUME_CORRUPTED;
    goto Out_Free;
  }

  if ((LogicalVolInt->NumberOfPartitions > MAX_UINT32 / sizeof (UINT32) / 2) ||
      (LogicalVolInt->NumberOfPartitions * sizeof (UINT32) * 2 >
       ExtentAd->ExtentLength - sizeof (UDF_LOGICAL_VOLUME_INTEGRITY))) {
    Status = EFI_VOLUME_CORRUPTED;
    goto Out_Free;
  }

  *VolumeSize = 0;
  *FreeSpaceSize = 0;

  Length = LogicalVolInt->NumberOfPartitions;
  for (Index = 0; Index < Length; Index += sizeof (UINT32)) {
    LsnsNo = *(UINT32 *)((UINT8 *)LogicalVolInt->Data + Index);
    //
    // Check if size is not specified
    //
    if (LsnsNo == 0xFFFFFFFFUL) {
      continue;
    }
    //
    // Accumulate free space size
    //
    *FreeSpaceSize += MultU64x32 ((UINT64)LsnsNo, LogicalBlockSize);
  }

  Length = LogicalVolInt->NumberOfPartitions * sizeof (UINT32) * 2;
  for (; Index < Length; Index += sizeof (UINT32)) {
    LsnsNo = *(UINT32 *)((UINT8 *)LogicalVolInt->Data + Index);
    //
    // Check if size is not specified
    //
    if (LsnsNo == 0xFFFFFFFFUL) {
      continue;
    }
    //
    // Accumulate used volume space
    //
    *VolumeSize += MultU64x32 ((UINT64)LsnsNo, LogicalBlockSize);
  }

  Status = EFI_SUCCESS;

Out_Free:
  //
  // Free Logical Volume Integrity Descriptor
  //
  FreePool (LogicalVolInt);

  return Status;
}

/**
  Seek a file and read its data into memory on an UDF volume.

  @param[in]      BlockIo       BlockIo interface.
  @param[in]      DiskIo        DiskIo interface.
  @param[in]      Volume        UDF volume information structure.
  @param[in]      File          File information structure.
  @param[in]      FileSize      Size of the file.
  @param[in, out] FilePosition  File position.
  @param[in, out] Buffer        File data.
  @param[in, out] BufferSize    Read size.

  @retval EFI_SUCCESS          File seeked and read.
  @retval EFI_UNSUPPORTED      Extended Allocation Descriptors not supported.
  @retval EFI_NO_MEDIA         The device has no media.
  @retval EFI_DEVICE_ERROR     The device reported an error.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_OUT_OF_RESOURCES The file's recorded data was not read due to lack
                               of resources.

**/
EFI_STATUS
ReadFileData (
  IN      EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN      EFI_DISK_IO_PROTOCOL   *DiskIo,
  IN      UDF_VOLUME_INFO        *Volume,
  IN      UDF_FILE_INFO          *File,
  IN      UINT64                 FileSize,
  IN OUT  UINT64                 *FilePosition,
  IN OUT  VOID                   *Buffer,
  IN OUT  UINT64                 *BufferSize
  )
{
  EFI_STATUS          Status;
  UDF_READ_FILE_INFO  ReadFileInfo;

  ReadFileInfo.Flags         = ReadFileSeekAndRead;
  ReadFileInfo.FilePosition  = *FilePosition;
  ReadFileInfo.FileData      = Buffer;
  ReadFileInfo.FileDataSize  = *BufferSize;
  ReadFileInfo.FileSize      = FileSize;

  Status = ReadFile (
                 BlockIo,
                 DiskIo,
                 Volume,
                 &File->FileIdentifierDesc->Icb,
                 File->FileEntry,
                 &ReadFileInfo
                 );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *BufferSize    = ReadFileInfo.FileDataSize;
  *FilePosition  = ReadFileInfo.FilePosition;

  return EFI_SUCCESS;
}

/**
  Check if ControllerHandle supports an UDF file system.

  @param[in]  This                Protocol instance pointer.
  @param[in]  ControllerHandle    Handle of device to test.

  @retval EFI_SUCCESS             UDF file system found.
  @retval EFI_UNSUPPORTED         UDF file system not found.

**/
EFI_STATUS
SupportUdfFileSystem (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL  *LastDevicePathNode;
  EFI_GUID                  *VendorDefinedGuid;

  //
  // Open Device Path protocol on ControllerHandle
  //
  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gEfiDevicePathProtocolGuid,
    (VOID **)&DevicePath,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = EFI_UNSUPPORTED;

  //
  // Get last Device Path node
  //
  LastDevicePathNode = NULL;
  DevicePathNode = DevicePath;
  while (!IsDevicePathEnd (DevicePathNode)) {
    LastDevicePathNode = DevicePathNode;
    DevicePathNode = NextDevicePathNode (DevicePathNode);
  }
  //
  // Check if last Device Path node contains a Vendor-Defined Media Device Path
  // of an UDF file system.
  //
  if (LastDevicePathNode != NULL &&
      DevicePathType (LastDevicePathNode) == MEDIA_DEVICE_PATH &&
      DevicePathSubType (LastDevicePathNode) == MEDIA_VENDOR_DP) {
    VendorDefinedGuid = (EFI_GUID *)((UINTN)LastDevicePathNode +
                                     OFFSET_OF (VENDOR_DEVICE_PATH, Guid));
    if (CompareGuid (VendorDefinedGuid, &gUdfDevPathGuid)) {
      Status = EFI_SUCCESS;
    }
  }

  //
  // Close Device Path protocol on ControllerHandle
  //
  gBS->CloseProtocol (
    ControllerHandle,
    &gEfiDevicePathProtocolGuid,
    This->DriverBindingHandle,
    ControllerHandle
    );

  return Status;
}
