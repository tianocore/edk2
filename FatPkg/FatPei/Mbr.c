/** @file
  Routines supporting partition discovery and
  logical device reading

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Mbr.h>
#include "FatLitePeim.h"

/**
  Test to see if the Mbr buffer is a valid MBR

  @param[in]  Mbr               Parent Handle
  @param[in]  LastLba           Last Lba address on the device.

  @retval     TRUE              Mbr is a Valid MBR
  @retval     FALSE             Mbr is not a Valid MBR

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
      // Compatibility Errata:
      //  Some systems try to hide drive space with their INT 13h driver
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

  @param[in]  PrivateData       The global memory map
  @param[in]  ParentBlockDevNo  The parent block device

  @retval TRUE              New partitions are detected and logical block devices
                            are added to block device array
  @retval FALSE             No new partitions are added

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

  if (ParentBlockDev->BlockSize > PEI_FAT_MAX_BLOCK_SIZE) {
    DEBUG((DEBUG_ERROR, "Device BlockSize %x exceeds FAT_MAX_BLOCK_SIZE\n", ParentBlockDev->BlockSize));
    return FALSE;
  }

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
