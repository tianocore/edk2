/** @file
  BDS routines to handle capsules.

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "Bds.h"

VOID
BdsLockFv (
  IN EFI_CPU_IO_PROTOCOL          *CpuIo,
  IN EFI_PHYSICAL_ADDRESS         Base
  )
{
  EFI_FV_BLOCK_MAP_ENTRY      *BlockMap;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  EFI_PHYSICAL_ADDRESS        BaseAddress;
  UINT8                       Data;
  UINT32                      BlockLength;
  UINTN                       Index;

  BaseAddress = Base - 0x400000 + 2;
  FvHeader    = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) (Base));
  BlockMap    = &(FvHeader->BlockMap[0]);

  while ((BlockMap->NumBlocks != 0) && (BlockMap->Length != 0)) {
    BlockLength = BlockMap->Length;
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
  EFI_PEI_HOB_POINTERS        HobPointer;
  EFI_CAPSULE_HEADER          *CapsuleHeader;
  UINT32                      Size;
  UINT32                      CapsuleNumber;
  UINT32                      CapsuleTotalNumber;
  EFI_CAPSULE_TABLE           *CapsuleTable;
  UINT32                      Index;
  UINT32                      CacheIndex;
  UINT32                      CacheNumber;
  VOID                        **CapsulePtr;
  VOID                        **CapsulePtrCache;
  EFI_GUID                    *CapsuleGuidCache; 
  CAPSULE_HOB_INFO            *CapsuleHobInfo;

  CapsuleNumber = 0;
  CapsuleTotalNumber = 0;
  CacheIndex   = 0;
  CacheNumber  = 0;
  CapsulePtr        = NULL;
  CapsulePtrCache   = NULL;
  CapsuleGuidCache  = NULL;

  //
  // We don't do anything else if the boot mode is not flash-update
  //
  if (BootMode != BOOT_ON_FLASH_UPDATE) {
    return EFI_INVALID_PARAMETER;
  }
  
  Status = EFI_SUCCESS;
  //
  // Find all capsule images from hob
  //
  HobPointer.Raw = GetHobList ();
  while ((HobPointer.Raw = GetNextGuidHob (&gEfiCapsuleVendorGuid, HobPointer.Raw)) != NULL) {
    CapsuleTotalNumber ++;

    HobPointer.Raw = GET_NEXT_HOB (HobPointer);
  }
  
  if (CapsuleTotalNumber == 0) {
    //
    // We didn't find a hob, so had no errors.
    //
    PlatformBdsLockNonUpdatableFlash ();
    return EFI_SUCCESS;
  }
  
  //
  // Init temp Capsule Data table.
  //
  CapsulePtr       = (VOID **) AllocateZeroPool (sizeof (VOID *) * CapsuleTotalNumber);
  ASSERT (CapsulePtr != NULL);
  CapsulePtrCache  = (VOID **) AllocateZeroPool (sizeof (VOID *) * CapsuleTotalNumber);
  ASSERT (CapsulePtrCache != NULL);
  CapsuleGuidCache = (EFI_GUID *) AllocateZeroPool (sizeof (EFI_GUID) * CapsuleTotalNumber);
  ASSERT (CapsuleGuidCache != NULL);
  
  //
  // Find all capsule images from hob
  //
  HobPointer.Raw = GetHobList ();
  while ((HobPointer.Raw = GetNextGuidHob (&gEfiCapsuleVendorGuid, HobPointer.Raw)) != NULL) {
    CapsuleHobInfo = GET_GUID_HOB_DATA (HobPointer.Guid);
    CapsulePtr [CapsuleNumber++] = (VOID *)(UINTN)(CapsuleHobInfo->BaseAddress);

    HobPointer.Raw = GET_NEXT_HOB (HobPointer);
  }

  //
  //Check the capsule flags,if contains CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE, install
  //capsuleTable to configure table with EFI_CAPSULE_GUID
  //

  //
  // Capsules who have CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE always are used for operating
  // System to have information persist across a system reset. EFI System Table must 
  // point to an array of capsules that contains the same CapsuleGuid value. And agents
  // searching for this type capsule will look in EFI System Table and search for the 
  // capsule's Guid and associated pointer to retrieve the data. Two steps below describes
  // how to sorting the capsules by the unique guid and install the array to EFI System Table. 
  // Firstly, Loop for all coalesced capsules, record unique CapsuleGuids and cache them in an 
  // array for later sorting capsules by CapsuleGuid.
  //
  for (Index = 0; Index < CapsuleTotalNumber; Index++) {
    CapsuleHeader = (EFI_CAPSULE_HEADER*) CapsulePtr [Index];
    if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) != 0) {
      //
      // For each capsule, we compare it with known CapsuleGuid in the CacheArray.
      // If already has the Guid, skip it. Whereas, record it in the CacheArray as 
      // an additional one.
      //
      CacheIndex = 0;
      while (CacheIndex < CacheNumber) {
        if (CompareGuid(&CapsuleGuidCache[CacheIndex],&CapsuleHeader->CapsuleGuid)) {
          break;
        }
        CacheIndex++;
      }
      if (CacheIndex == CacheNumber) {
        CopyMem(&CapsuleGuidCache[CacheNumber++],&CapsuleHeader->CapsuleGuid,sizeof(EFI_GUID));
      }
    }
  }

  //
  // Secondly, for each unique CapsuleGuid in CacheArray, gather all coalesced capsules
  // whose guid is the same as it, and malloc memory for an array which preceding
  // with UINT32. The array fills with entry point of capsules that have the same
  // CapsuleGuid, and UINT32 represents the size of the array of capsules. Then install
  // this array into EFI System Table, so that agents searching for this type capsule
  // will look in EFI System Table and search for the capsule's Guid and associated
  // pointer to retrieve the data.
  //
  CacheIndex = 0;
  while (CacheIndex < CacheNumber) {
    CapsuleNumber = 0;  
    for (Index = 0; Index < CapsuleTotalNumber; Index++) {
      CapsuleHeader = (EFI_CAPSULE_HEADER*) CapsulePtr [Index];
      if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) != 0) {
        if (CompareGuid (&CapsuleGuidCache[CacheIndex], &CapsuleHeader->CapsuleGuid)) {
          //
          // Cache Caspuleheader to the array, this array is uniqued with certain CapsuleGuid.
          //
          CapsulePtrCache[CapsuleNumber++] = (VOID*)CapsuleHeader;
        }
      }
    }
    if (CapsuleNumber != 0) {
      Size = sizeof(EFI_CAPSULE_TABLE) + (CapsuleNumber - 1) * sizeof(VOID*);  
      CapsuleTable = AllocateRuntimePool (Size);
      ASSERT (CapsuleTable != NULL);
      CapsuleTable->CapsuleArrayNumber =  CapsuleNumber;
      CopyMem(&CapsuleTable->CapsulePtr[0], CapsulePtrCache, CapsuleNumber * sizeof(VOID*));
      Status = gBS->InstallConfigurationTable (&CapsuleGuidCache[CacheIndex], (VOID*)CapsuleTable);
      ASSERT_EFI_ERROR (Status);
    }
    CacheIndex++;
  }

  //
  // Besides ones with CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE flag, all capsules left are
  // recognized by platform with CapsuleGuid. For general platform driver, UpdateFlash 
  // type is commonly supported, so here only deal with encapsuled FVs capsule. Additional
  // type capsule transaction could be extended. It depends on platform policy.
  //
  for (Index = 0; Index < CapsuleTotalNumber; Index++) {
    CapsuleHeader = (EFI_CAPSULE_HEADER*) CapsulePtr [Index];
    if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) == 0) {
      //
      // Call capsule library to process capsule image.
      //
      ProcessCapsuleImage (CapsuleHeader);
    }
  }

  PlatformBdsLockNonUpdatableFlash ();
  
  //
  // Free the allocated temp memory space.
  //
  FreePool (CapsuleGuidCache);
  FreePool (CapsulePtrCache);
  FreePool (CapsulePtr);

  return Status;
}
