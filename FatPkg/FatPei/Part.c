/** @file
  Routines supporting partition discovery and 
  logical device reading

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution. The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <IndustryStandard/Mbr.h>
#include <IndustryStandard/ElTorito.h>
#include "FatLitePeim.h"

/**
  This function finds Eltorito partitions. Main algorithm
  is ported from DXE partition driver.

  @param  PrivateData       The global memory map 
  @param  ParentBlockDevNo  The parent block device 

  @retval TRUE              New partitions are detected and logical block devices 
                            are  added to block device array 
  @retval FALSE             No New partitions are added;

**/
BOOLEAN
FatFindEltoritoPartitions (
  IN  PEI_FAT_PRIVATE_DATA *PrivateData,
  IN  UINTN                ParentBlockDevNo
  );

/**
  This function finds Mbr partitions. Main algorithm
  is ported from DXE partition driver.

  @param  PrivateData       The global memory map 
  @param  ParentBlockDevNo  The parent block device 

  @retval TRUE              New partitions are detected and logical block devices 
                            are  added to block device array 
  @retval FALSE             No New partitions are added;

**/
BOOLEAN
FatFindMbrPartitions (
  IN  PEI_FAT_PRIVATE_DATA *PrivateData,
  IN  UINTN                ParentBlockDevNo
  );


/**
  This function finds partitions (logical devices) in physical block devices.

  @param  PrivateData       Global memory map for accessing global variables.

**/
VOID
FatFindPartitions (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData
  )
{
  BOOLEAN Found;
  UINTN   Index;

  do {
    Found = FALSE;

    for (Index = 0; Index < PrivateData->BlockDeviceCount; Index++) {
      if (!PrivateData->BlockDevice[Index].PartitionChecked) {
        Found = FatFindMbrPartitions (PrivateData, Index);
        if (!Found) {
          Found = FatFindEltoritoPartitions (PrivateData, Index);
        }
      }
    }
  } while (Found && PrivateData->BlockDeviceCount <= PEI_FAT_MAX_BLOCK_DEVICE);
}


/**
  This function finds Eltorito partitions. Main algorithm
  is ported from DXE partition driver.

  @param  PrivateData       The global memory map 
  @param  ParentBlockDevNo  The parent block device 

  @retval TRUE              New partitions are detected and logical block devices 
                            are  added to block device array 
  @retval FALSE             No New partitions are added;

**/
BOOLEAN
FatFindEltoritoPartitions (
  IN  PEI_FAT_PRIVATE_DATA *PrivateData,
  IN  UINTN                ParentBlockDevNo
  )
{
  EFI_STATUS              Status;
  BOOLEAN                 Found;
  PEI_FAT_BLOCK_DEVICE    *BlockDev;
  PEI_FAT_BLOCK_DEVICE    *ParentBlockDev;
  UINT32                  VolDescriptorLba;
  UINT32                  Lba;
  CDROM_VOLUME_DESCRIPTOR *VolDescriptor;
  ELTORITO_CATALOG        *Catalog;
  UINTN                   Check;
  UINTN                   Index;
  UINTN                   MaxIndex;
  UINT16                  *CheckBuffer;
  UINT32                  SubBlockSize;
  UINT32                  SectorCount;
  UINT32                  VolSpaceSize;

  if (ParentBlockDevNo > PEI_FAT_MAX_BLOCK_DEVICE - 1) {
    return FALSE;
  }

  Found           = FALSE;
  ParentBlockDev  = &(PrivateData->BlockDevice[ParentBlockDevNo]);
  VolSpaceSize    = 0;

  //
  // CD_ROM has the fixed block size as 2048 bytes
  //
  if (ParentBlockDev->BlockSize != 2048) {
    return FALSE;
  }

  VolDescriptor = (CDROM_VOLUME_DESCRIPTOR *) PrivateData->BlockData;
  Catalog       = (ELTORITO_CATALOG *) VolDescriptor;

  //
  // the ISO-9660 volume descriptor starts at 32k on the media
  // and CD_ROM has the fixed block size as 2048 bytes, so...
  //
  VolDescriptorLba = 15;
  //
  // ((16*2048) / Media->BlockSize) - 1;
  //
  // Loop: handle one volume descriptor per time
  //
  while (TRUE) {

    VolDescriptorLba += 1;
    if (VolDescriptorLba > ParentBlockDev->LastBlock) {
      //
      // We are pointing past the end of the device so exit
      //
      break;
    }

    Status = FatReadBlock (
              PrivateData,
              ParentBlockDevNo,
              VolDescriptorLba,
              ParentBlockDev->BlockSize,
              VolDescriptor
              );
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Check for valid volume descriptor signature
    //
    if (VolDescriptor->Unknown.Type == CDVOL_TYPE_END ||
        CompareMem (VolDescriptor->Unknown.Id, CDVOL_ID, sizeof (VolDescriptor->Unknown.Id)) != 0
        ) {
      //
      // end of Volume descriptor list
      //
      break;
    }
    //
    // Read the Volume Space Size from Primary Volume Descriptor 81-88 byte
    //
    if (VolDescriptor->Unknown.Type == CDVOL_TYPE_CODED) {
      VolSpaceSize = VolDescriptor->PrimaryVolume.VolSpaceSize[1];
    }
    //
    // Is it an El Torito volume descriptor?
    //
    if (CompareMem (
          VolDescriptor->BootRecordVolume.SystemId,
          CDVOL_ELTORITO_ID,
          sizeof (CDVOL_ELTORITO_ID) - 1
          ) != 0) {
      continue;
    }
    //
    // Read in the boot El Torito boot catalog
    //
    Lba = UNPACK_INT32 (VolDescriptor->BootRecordVolume.EltCatalog);
    if (Lba > ParentBlockDev->LastBlock) {
      continue;
    }

    Status = FatReadBlock (
              PrivateData,
              ParentBlockDevNo,
              Lba,
              ParentBlockDev->BlockSize,
              Catalog
              );
    if (EFI_ERROR (Status)) {
      continue;
    }
    //
    // We don't care too much about the Catalog header's contents, but we do want
    // to make sure it looks like a Catalog header
    //
    if (Catalog->Catalog.Indicator != ELTORITO_ID_CATALOG || Catalog->Catalog.Id55AA != 0xAA55) {
      continue;
    }

    Check       = 0;
    CheckBuffer = (UINT16 *) Catalog;
    for (Index = 0; Index < sizeof (ELTORITO_CATALOG) / sizeof (UINT16); Index += 1) {
      Check += CheckBuffer[Index];
    }

    if ((Check & 0xFFFF) != 0) {
      continue;
    }

    MaxIndex = ParentBlockDev->BlockSize / sizeof (ELTORITO_CATALOG);
    for (Index = 1; Index < MaxIndex; Index += 1) {
      //
      // Next entry
      //
      Catalog += 1;

      //
      // Check this entry
      //
      if (Catalog->Boot.Indicator != ELTORITO_ID_SECTION_BOOTABLE || Catalog->Boot.Lba == 0) {
        continue;
      }

      SubBlockSize  = 512;
      SectorCount   = Catalog->Boot.SectorCount;

      switch (Catalog->Boot.MediaType) {

      case ELTORITO_NO_EMULATION:
        SubBlockSize  = ParentBlockDev->BlockSize;
        SectorCount   = Catalog->Boot.SectorCount;
        break;

      case ELTORITO_HARD_DISK:
        break;

      case ELTORITO_12_DISKETTE:
        SectorCount = 0x50 * 0x02 * 0x0F;
        break;

      case ELTORITO_14_DISKETTE:
        SectorCount = 0x50 * 0x02 * 0x12;
        break;

      case ELTORITO_28_DISKETTE:
        SectorCount = 0x50 * 0x02 * 0x24;
        break;

      default:
        SectorCount   = 0;
        SubBlockSize  = ParentBlockDev->BlockSize;
        break;
      }

      if (SectorCount < 2) {
        SectorCount = (VolSpaceSize > ParentBlockDev->LastBlock + 1) ? (UINT32) (ParentBlockDev->LastBlock - Catalog->Boot.Lba + 1) : (UINT32) (VolSpaceSize - Catalog->Boot.Lba);
      }
      //
      // Register this partition
      //
      if (PrivateData->BlockDeviceCount < PEI_FAT_MAX_BLOCK_DEVICE) {

        Found                       = TRUE;

        BlockDev                    = &(PrivateData->BlockDevice[PrivateData->BlockDeviceCount]);

        BlockDev->BlockSize         = SubBlockSize;
        BlockDev->LastBlock         = SectorCount - 1;
        BlockDev->IoAlign           = ParentBlockDev->IoAlign;
        BlockDev->Logical           = TRUE;
        BlockDev->PartitionChecked  = FALSE;
        BlockDev->StartingPos       = MultU64x32 (Catalog->Boot.Lba, ParentBlockDev->BlockSize);
        BlockDev->ParentDevNo       = ParentBlockDevNo;

        PrivateData->BlockDeviceCount++;
      }
    }
  }

  ParentBlockDev->PartitionChecked = TRUE;

  return Found;

}


/**
  Test to see if the Mbr buffer is a valid MBR

  @param  Mbr               Parent Handle 
  @param  LastLba           Last Lba address on the device. 

  @retval TRUE              Mbr is a Valid MBR 
  @retval FALSE             Mbr is not a Valid MBR

**/
BOOLEAN
PartitionValidMbr (
  IN  MASTER_BOOT_RECORD      *Mbr,
  IN  EFI_PEI_LBA             LastLba
  )
{
  UINT32  StartingLBA;
  UINT32  EndingLBA;
  UINT32  NewEndingLBA;
  INTN    Index1;
  INTN    Index2;
  BOOLEAN MbrValid;

  if (Mbr->Signature != MBR_SIGNATURE) {
    return FALSE;
  }
  //
  // The BPB also has this signature, so it can not be used alone.
  //
  MbrValid = FALSE;
  for (Index1 = 0; Index1 < MAX_MBR_PARTITIONS; Index1++) {
    if (Mbr->Partition[Index1].OSIndicator == 0x00 || UNPACK_UINT32 (Mbr->Partition[Index1].SizeInLBA) == 0) {
      continue;
    }

    MbrValid    = TRUE;
    StartingLBA = UNPACK_UINT32 (Mbr->Partition[Index1].StartingLBA);
    EndingLBA   = StartingLBA + UNPACK_UINT32 (Mbr->Partition[Index1].SizeInLBA) - 1;
    if (EndingLBA > LastLba) {
      //
      // Compatability Errata:
      //  Some systems try to hide drive space with thier INT 13h driver
      //  This does not hide space from the OS driver. This means the MBR
      //  that gets created from DOS is smaller than the MBR created from
      //  a real OS (NT & Win98). This leads to BlockIo->LastBlock being
      //  wrong on some systems FDISKed by the OS.
      //
      //  return FALSE Because no block devices on a system are implemented
      //  with INT 13h
      //
      return FALSE;
    }

    for (Index2 = Index1 + 1; Index2 < MAX_MBR_PARTITIONS; Index2++) {
      if (Mbr->Partition[Index2].OSIndicator == 0x00 || UNPACK_INT32 (Mbr->Partition[Index2].SizeInLBA) == 0) {
        continue;
      }

      NewEndingLBA = UNPACK_UINT32 (Mbr->Partition[Index2].StartingLBA) + UNPACK_UINT32 (Mbr->Partition[Index2].SizeInLBA) - 1;
      if (NewEndingLBA >= StartingLBA && UNPACK_UINT32 (Mbr->Partition[Index2].StartingLBA) <= EndingLBA) {
        //
        // This region overlaps with the Index1'th region
        //
        return FALSE;
      }
    }
  }
  //
  // Non of the regions overlapped so MBR is O.K.
  //
  return MbrValid;
}


/**
  This function finds Mbr partitions. Main algorithm
  is ported from DXE partition driver.

  @param  PrivateData       The global memory map 
  @param  ParentBlockDevNo  The parent block device 

  @retval TRUE              New partitions are detected and logical block devices 
                            are  added to block device array 
  @retval FALSE             No New partitions are added;

**/
BOOLEAN
FatFindMbrPartitions (
  IN  PEI_FAT_PRIVATE_DATA *PrivateData,
  IN  UINTN                ParentBlockDevNo
  )
{
  EFI_STATUS            Status;
  MASTER_BOOT_RECORD    *Mbr;
  UINTN                 Index;
  BOOLEAN               Found;
  PEI_FAT_BLOCK_DEVICE  *ParentBlockDev;
  PEI_FAT_BLOCK_DEVICE  *BlockDev;

  if (ParentBlockDevNo > PEI_FAT_MAX_BLOCK_DEVICE - 1) {
    return FALSE;
  }

  ParentBlockDev  = &(PrivateData->BlockDevice[ParentBlockDevNo]);

  Found           = FALSE;
  Mbr             = (MASTER_BOOT_RECORD *) PrivateData->BlockData;

  Status = FatReadBlock (
            PrivateData,
            ParentBlockDevNo,
            0,
            ParentBlockDev->BlockSize,
            Mbr
            );

  if (EFI_ERROR (Status) || !PartitionValidMbr (Mbr, ParentBlockDev->LastBlock)) {
    goto Done;
  }
  //
  // We have a valid mbr - add each partition
  //
  for (Index = 0; Index < MAX_MBR_PARTITIONS; Index++) {
    if (Mbr->Partition[Index].OSIndicator == 0x00 || UNPACK_INT32 (Mbr->Partition[Index].SizeInLBA) == 0) {
      //
      // Don't use null MBR entries
      //
      continue;
    }
    //
    // Register this partition
    //
    if (PrivateData->BlockDeviceCount < PEI_FAT_MAX_BLOCK_DEVICE) {

      Found                       = TRUE;

      BlockDev                    = &(PrivateData->BlockDevice[PrivateData->BlockDeviceCount]);

      BlockDev->BlockSize         = MBR_SIZE;
      BlockDev->LastBlock         = UNPACK_INT32 (Mbr->Partition[Index].SizeInLBA) - 1;
      BlockDev->IoAlign           = ParentBlockDev->IoAlign;
      BlockDev->Logical           = TRUE;
      BlockDev->PartitionChecked  = FALSE;
      BlockDev->StartingPos = MultU64x32 (
                                UNPACK_INT32 (Mbr->Partition[Index].StartingLBA),
                                ParentBlockDev->BlockSize
                                );
      BlockDev->ParentDevNo = ParentBlockDevNo;

      PrivateData->BlockDeviceCount++;
    }
  }

Done:

  ParentBlockDev->PartitionChecked = TRUE;
  return Found;
}
