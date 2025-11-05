/** @file

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/NvVariableInfoGuid.h>
#include <Library/HobLib.h>

#define FVB_MEDIA_BLOCK_SIZE  0x1000

typedef struct {
  EFI_FIRMWARE_VOLUME_HEADER    FvInfo;
  EFI_FV_BLOCK_MAP_ENTRY        End[1];
} EFI_FVB2_MEDIA_INFO;

//
// This data structure contains a template of FV header which is used to restore
// Fv header if it's corrupted.
//
EFI_FVB2_MEDIA_INFO  mFvbMediaInfo = {
  {
    { 0, },         // ZeroVector[16]
    EFI_SYSTEM_NV_DATA_FV_GUID,
    0,
    EFI_FVH_SIGNATURE,
    0x0004feff,     // check PiFirmwareVolume.h for details on EFI_FVB_ATTRIBUTES_2
    sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
    0,              // CheckSum which will be calucated dynamically.
    0,              // ExtHeaderOffset
    { 0, },
    EFI_FVH_REVISION,
    {
      {
        0,
        FVB_MEDIA_BLOCK_SIZE,
      }
    }
  },
  {
    {
      0,
      0
    }
  }
};

/**
  Initialize the variable store

  @retval     EFI_SUCCESS if initialize the store success.

**/
EFI_STATUS
InitVariableStore (
  VOID
  )
{
  EFI_STATUS         Status;
  UINT32             NvStorageBase;
  UINT32             NvStorageSize;
  UINT32             NvVariableSize;
  UINT32             FtwWorkingSize;
  UINT32             FtwSpareSize;
  EFI_HOB_GUID_TYPE  *GuidHob;
  NV_VARIABLE_INFO   *NvVariableInfo;

  //
  // Find SPI flash variable hob
  //
  GuidHob = GetFirstGuidHob (&gNvVariableInfoGuid);
  if (GuidHob == NULL) {
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  NvVariableInfo = (NV_VARIABLE_INFO *)GET_GUID_HOB_DATA (GuidHob);

  //
  // Get variable region base and size.
  //
  NvStorageSize = NvVariableInfo->VariableStoreSize;
  NvStorageBase = NvVariableInfo->VariableStoreBase;

  //
  // NvStorageBase needs to be 4KB aligned, NvStorageSize needs to be 8KB * n
  //
  if (((NvStorageBase & (SIZE_4KB - 1)) != 0) || ((NvStorageSize & (SIZE_8KB - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  FtwSpareSize   = NvStorageSize / 2;
  FtwWorkingSize = 0x2000;
  NvVariableSize = NvStorageSize / 2 - FtwWorkingSize;
  DEBUG ((DEBUG_INFO, "NvStorageBase:0x%x, NvStorageSize:0x%x\n", NvStorageBase, NvStorageSize));

  if (NvVariableSize >= 0x80000000) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PcdSet32S (PcdFlashNvStorageVariableSize, NvVariableSize);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdFlashNvStorageVariableBase, NvStorageBase);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet64S (PcdFlashNvStorageVariableBase64, NvStorageBase);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet32S (PcdFlashNvStorageFtwWorkingSize, FtwWorkingSize);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdFlashNvStorageFtwWorkingBase, NvStorageBase + NvVariableSize);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet32S (PcdFlashNvStorageFtwSpareSize, FtwSpareSize);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdFlashNvStorageFtwSpareBase, NvStorageBase + FtwSpareSize);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Get a heathy FV header used for variable store recovery

  @retval     The FV header.

**/
EFI_FIRMWARE_VOLUME_HEADER *
GetFvHeaderTemplate (
  VOID
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  UINTN                       FvSize;

  FvSize                          = PcdGet32 (PcdFlashNvStorageFtwSpareSize) * 2;
  FvHeader                        = &mFvbMediaInfo.FvInfo;
  FvHeader->FvLength              = FvSize;
  FvHeader->BlockMap[0].NumBlocks = (UINT32)(FvSize / FvHeader->BlockMap[0].Length);
  FvHeader->Checksum              = 0;
  FvHeader->Checksum              = CalculateCheckSum16 ((UINT16 *)FvHeader, FvHeader->HeaderLength);

  return FvHeader;
}
