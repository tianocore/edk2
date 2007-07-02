/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Capsules.c

Abstract:

  BDS routines to handle capsules.

--*/


//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include <Common/FlashMap.H>

VOID
BdsLockFv (
  IN EFI_CPU_IO_PROTOCOL          *CpuIo,
  IN EFI_FLASH_SUBAREA_ENTRY      *FlashEntry
  );

VOID
BdsLockFv (
  IN EFI_CPU_IO_PROTOCOL          *CpuIo,
  IN EFI_FLASH_SUBAREA_ENTRY      *FlashEntry
  )
{
  EFI_FV_BLOCK_MAP_ENTRY      *BlockMap;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  UINT64                      BaseAddress;
  UINT8                       Data;
  UINT32                      BlockLength;
  UINTN                       Index;

  BaseAddress = FlashEntry->Base - 0x400000 + 2;
  FvHeader    = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) (FlashEntry->Base));
  BlockMap    = &(FvHeader->FvBlockMap[0]);

  while ((BlockMap->NumBlocks != 0) && (BlockMap->BlockLength != 0)) {
    BlockLength = BlockMap->BlockLength;
    for (Index = 0; Index < BlockMap->NumBlocks; Index++) {
      CpuIo->Mem.Read (
                  CpuIo,
                  EfiCpuIoWidthUint8,
                  BaseAddress,
                  1,
                  &Data
                  );
      Data = (UINT8) (Data | 0x3);
      CpuIo->Mem.Write (
                  CpuIo,
                  EfiCpuIoWidthUint8,
                  BaseAddress,
                  1,
                  &Data
                  );
      BaseAddress += BlockLength;
    }

    BlockMap++;
  }
}

VOID
BdsLockNonUpdatableFlash (
  VOID
  )
{
  EFI_FLASH_MAP_ENTRY_DATA  *FlashMapEntryData;
  EFI_PEI_HOB_POINTERS      GuidHob;
  EFI_STATUS                Status;
  EFI_CPU_IO_PROTOCOL       *CpuIo;

  Status = gBS->LocateProtocol (&gEfiCpuIoProtocolGuid, NULL, &CpuIo);
  ASSERT_EFI_ERROR (Status);
  
  GuidHob.Raw = GetHobList ();
  while ((GuidHob.Raw = GetNextGuidHob (&gEfiFlashMapHobGuid, GuidHob.Raw)) != NULL) {
    FlashMapEntryData = (EFI_FLASH_MAP_ENTRY_DATA *) GET_GUID_HOB_DATA (GuidHob.Guid);

    //
    // Get the variable store area
    //
    if ((FlashMapEntryData->AreaType == EFI_FLASH_AREA_RECOVERY_BIOS) ||
        (FlashMapEntryData->AreaType == EFI_FLASH_AREA_MAIN_BIOS)
        ) {
      BdsLockFv (CpuIo, &(FlashMapEntryData->Entries[0]));
    }
    GuidHob.Raw = GET_NEXT_HOB (GuidHob);
  }

  return ;
}

EFI_STATUS
ProcessCapsules (
  EFI_BOOT_MODE BootMode
  )
/*++

Routine Description:

  This routine is called to see if there are any capsules we need to process.
  If the boot mode is not UPDATE, then we do nothing. Otherwise find the
  capsule HOBS and produce firmware volumes for them via the DXE service.
  Then call the dispatcher to dispatch drivers from them. Finally, check
  the status of the updates.

Arguments:

  BootMode - the current boot mode

Returns:
  
  EFI_INVALID_PARAMETER - boot mode is not correct for an update

Note:
 
 This function should be called by BDS in case we need to do some
 sort of processing even if there is no capsule to process. We
 need to do this if an earlier update went awry and we need to
 clear the capsule variable so on the next reset PEI does not see it and 
 think there is a capsule available.

--*/
{
  EFI_STATUS                  Status;
  EFI_HOB_CAPSULE_VOLUME      *CvHob;
  EFI_PHYSICAL_ADDRESS        BaseAddress;
  UINT64                      Length;
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  EFI_HANDLE                  FvProtocolHandle;

  //
  // We don't do anything else if the boot mode is not flash-update
  //
  if (BootMode != BOOT_ON_FLASH_UPDATE) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Only one capsule HOB allowed.
  //
  CvHob = GetFirstHob (EFI_HOB_TYPE_CV);
  if (CvHob == NULL) {
    //
    // We didn't find a hob, so had no errors.
    //
    BdsLockNonUpdatableFlash ();
    return EFI_SUCCESS;
  }
  
  BaseAddress = CvHob->BaseAddress;
  Length      = CvHob->Length;

  Status      = EFI_SUCCESS;
  //
  // Now walk the capsule and call the core to process each
  // firmware volume in it.
  //
  while (Length != 0) {
    //
    // Point to the next firmware volume header, and then
    // call the DXE service to process it.
    //
    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) BaseAddress;
    if (FwVolHeader->FvLength > Length) {
      //
      // Notes: need to stuff this status somewhere so that the
      // error can be detected at OS runtime
      //
      Status = EFI_VOLUME_CORRUPTED;
      break;
    }

    Status = gDS->ProcessFirmwareVolume (
                    (VOID *) (UINTN) BaseAddress,
                    (UINTN) FwVolHeader->FvLength,
                    &FvProtocolHandle
                    );
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Call the dispatcher to dispatch any drivers from the produced firmware volume
    //
    gDS->Dispatch ();
    //
    // On to the next FV in the capsule
    //
    Length -= FwVolHeader->FvLength;
    BaseAddress = (EFI_PHYSICAL_ADDRESS) ((UINTN) BaseAddress + FwVolHeader->FvLength);
    //
    // Notes: when capsule spec is finalized, if the requirement is made to
    // have each FV in a capsule aligned, then we will need to align the
    // BaseAddress and Length here.
    //
  }
   

  BdsLockNonUpdatableFlash ();

  return Status;
}

