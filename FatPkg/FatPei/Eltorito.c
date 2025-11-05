/** @file
  Routines supporting partition discovery and
  logical device reading

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/ElTorito.h>
#include "FatLitePeim.h"

/**
  This function finds Eltorito partitions. Main algorithm
  is ported from DXE partition driver.

  @param[in]  PrivateData       The global memory map
  @param[in]  ParentBlockDevNo  The parent block device

  @retval TRUE              New partitions are detected and logical block devices
                            are added to block device array
  @retval FALSE             No new partitions are added

**/
BOOLEAN
FatFindEltoritoPartitions (
  IN  PEI_FAT_PRIVATE_DATA  *PrivateData,
  IN  UINTN                 ParentBlockDevNo
  )
{
  EFI_STATUS               Status;
  BOOLEAN                  Found;
  PEI_FAT_BLOCK_DEVICE     *BlockDev;
  PEI_FAT_BLOCK_DEVICE     *ParentBlockDev;
  UINT32                   VolDescriptorLba;
  UINT32                   Lba;
  CDROM_VOLUME_DESCRIPTOR  *VolDescriptor;
  ELTORITO_CATALOG         *Catalog;
  UINTN                    Check;
  UINTN                    Index;
  UINTN                    MaxIndex;
  UINT16                   *CheckBuffer;
  UINT32                   SubBlockSize;
  UINT32                   SectorCount;
  UINT32                   VolSpaceSize;

  if (ParentBlockDevNo > PEI_FAT_MAX_BLOCK_DEVICE - 1) {
    return FALSE;
  }

  Found          = FALSE;
  ParentBlockDev = &(PrivateData->BlockDevice[ParentBlockDevNo]);
  VolSpaceSize   = 0;

  //
  // CD_ROM has the fixed block size as 2048 bytes
  //
  if (ParentBlockDev->BlockSize != 2048) {
    return FALSE;
  }

  VolDescriptor = (CDROM_VOLUME_DESCRIPTOR *)PrivateData->BlockData;
  Catalog       = (ELTORITO_CATALOG *)VolDescriptor;

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
    if ((VolDescriptor->Unknown.Type == CDVOL_TYPE_END) ||
        (CompareMem (VolDescriptor->Unknown.Id, CDVOL_ID, sizeof (VolDescriptor->Unknown.Id)) != 0)
        )
    {
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
          ) != 0)
    {
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
    if ((Catalog->Catalog.Indicator != ELTORITO_ID_CATALOG) || (Catalog->Catalog.Id55AA != 0xAA55)) {
      continue;
    }

    Check       = 0;
    CheckBuffer = (UINT16 *)Catalog;
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
      if ((Catalog->Boot.Indicator != ELTORITO_ID_SECTION_BOOTABLE) || (Catalog->Boot.Lba == 0)) {
        continue;
      }

      SubBlockSize = 512;
      SectorCount  = Catalog->Boot.SectorCount;

      switch (Catalog->Boot.MediaType) {
        case ELTORITO_NO_EMULATION:
          SubBlockSize = ParentBlockDev->BlockSize;
          SectorCount  = Catalog->Boot.SectorCount;
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
          SectorCount  = 0;
          SubBlockSize = ParentBlockDev->BlockSize;
          break;
      }

      if (SectorCount < 2) {
        SectorCount = (VolSpaceSize > ParentBlockDev->LastBlock + 1) ? (UINT32)(ParentBlockDev->LastBlock - Catalog->Boot.Lba + 1) : (UINT32)(VolSpaceSize - Catalog->Boot.Lba);
      }

      //
      // Register this partition
      //
      if (PrivateData->BlockDeviceCount < PEI_FAT_MAX_BLOCK_DEVICE) {
        Found = TRUE;

        BlockDev = &(PrivateData->BlockDevice[PrivateData->BlockDeviceCount]);

        BlockDev->BlockSize        = SubBlockSize;
        BlockDev->LastBlock        = SectorCount - 1;
        BlockDev->IoAlign          = ParentBlockDev->IoAlign;
        BlockDev->Logical          = TRUE;
        BlockDev->PartitionChecked = FALSE;
        BlockDev->StartingPos      = MultU64x32 (Catalog->Boot.Lba, ParentBlockDev->BlockSize);
        BlockDev->ParentDevNo      = ParentBlockDevNo;

        PrivateData->BlockDeviceCount++;
      }
    }
  }

  ParentBlockDev->PartitionChecked = TRUE;

  return Found;
}
