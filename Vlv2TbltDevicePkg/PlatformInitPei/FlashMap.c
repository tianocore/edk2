/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   



Module Name:

  FlashMap.c

Abstract:

  Build GUIDed HOBs for platform specific flash map.

--*/

#include "Efi.h"
#include "Pei.h"
#include "PeiLib.h"
#include "PeiLib.h"
#include "EfiFlashMap.h"
#include EFI_PROTOCOL_CONSUMER (FirmwareVolumeBlock)
#include EFI_GUID_DEFINITION (FlashMapHob)
#include EFI_GUID_DEFINITION (SystemNvDataGuid)
#include EFI_GUID_DEFINITION (FirmwareFileSystem)

EFI_GUID                            mFvBlockGuid      = EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL_GUID;
EFI_GUID                            mFfsGuid          = EFI_FIRMWARE_FILE_SYSTEM_GUID;
EFI_GUID                            mSystemDataGuid   = EFI_SYSTEM_NV_DATA_HOB_GUID;

static EFI_FLASH_AREA_DATA          mFlashAreaData[]  = {
	//
  // Variable area
  //
  { FixedPcdGet32 (PcdFlashNvStorageVariableBase),
    FixedPcdGet32 (PcdFlashNvStorageVariableSize),
    EFI_FLASH_AREA_SUBFV | EFI_FLASH_AREA_MEMMAPPED_FV,
    EFI_FLASH_AREA_EFI_VARIABLES },

  //
  // Boot block 2nd part
  //
  { FixedPcdGet32 (PcdFlashFvRecovery2Base),
    FixedPcdGet32 (PcdFlashFvRecovery2Size),
    EFI_FLASH_AREA_SUBFV | EFI_FLASH_AREA_MEMMAPPED_FV,
    EFI_FLASH_AREA_FTW_BACKUP },

  //
  // Recovery FV
  //
  { FixedPcdGet32 (PcdFlashFvRecoveryBase),
    FixedPcdGet32 (PcdFlashFvRecoverySize),
    EFI_FLASH_AREA_FV | EFI_FLASH_AREA_MEMMAPPED_FV,
    EFI_FLASH_AREA_RECOVERY_BIOS },

  //
  // Main FV
  //
  { FixedPcdGet32 (PcdFlashFvMainBase),
    FixedPcdGet32 (PcdFlashFvMainSize),
    EFI_FLASH_AREA_FV | EFI_FLASH_AREA_MEMMAPPED_FV,
    EFI_FLASH_AREA_MAIN_BIOS }

};

#define NUM_FLASH_AREA_DATA (sizeof (mFlashAreaData) / sizeof (mFlashAreaData[0]))

/**
  Build GUID HOBs for platform specific flash map.

  @param FfsHeader     Pointer this FFS file header.
  @param PeiServices   General purpose services available to every PEIM.

  @retval EFI_SUCCESS   Guid HOBs for platform flash map is built.
  @retval Otherwise     Failed to build the Guid HOB data.

**/
EFI_STATUS
PeimInitializeFlashMap (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
{
  UINTN                         Index;
  EFI_FLASH_AREA_HOB_DATA       FlashHobData;

  //
  // Build flash area entries as GUIDed HOBs.
  //
  for (Index = 0; Index < NUM_FLASH_AREA_DATA; Index++) {
    ZeroMem(&FlashHobData, sizeof (EFI_FLASH_AREA_HOB_DATA));

    FlashHobData.AreaType               = mFlashAreaData[Index].AreaType;
    FlashHobData.NumberOfEntries        = 1;
    FlashHobData.SubAreaData.Attributes = mFlashAreaData[Index].Attributes;
    FlashHobData.SubAreaData.Base       = (EFI_PHYSICAL_ADDRESS) (UINTN) mFlashAreaData[Index].Base;
    FlashHobData.SubAreaData.Length     = (EFI_PHYSICAL_ADDRESS) (UINTN) mFlashAreaData[Index].Length;

    switch (FlashHobData.AreaType) {
    case EFI_FLASH_AREA_RECOVERY_BIOS:
    case EFI_FLASH_AREA_MAIN_BIOS:
      CopyMem (
        &FlashHobData.AreaTypeGuid,
        &mFfsGuid,
        sizeof (EFI_GUID)
        );
      CopyMem (
        &FlashHobData.SubAreaData.FileSystem,
        &mFvBlockGuid,
        sizeof (EFI_GUID)
        );
      break;

    case EFI_FLASH_AREA_GUID_DEFINED:
      CopyMem (
        &FlashHobData.AreaTypeGuid,
        &mSystemDataGuid,
        sizeof (EFI_GUID)
        );
      CopyMem (
        &FlashHobData.SubAreaData.FileSystem,
        &mFvBlockGuid,
        sizeof (EFI_GUID)
        );
      break;

    default:
      break;
    }

    PeiBuildHobGuidData(PeiServices,
                        &gEfiFlashMapHobGuid,
                        &FlashHobData,
                        sizeof (EFI_FLASH_AREA_HOB_DATA)
                        );
  }
  return EFI_SUCCESS;
}

