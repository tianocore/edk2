/** @file
  BDS routines to handle capsules.

Copyright (c) 2004 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "Bds.h"

/**

  This routine is called to see if there are any capsules we need to process.
  If the boot mode is not UPDATE, then we do nothing. Otherwise find the
  capsule HOBS and produce firmware volumes for them via the DXE service.
  Then call the dispatcher to dispatch drivers from them. Finally, check
  the status of the updates.

  This function should be called by BDS in case we need to do some
  sort of processing even if there is no capsule to process. We
  need to do this if an earlier update went away and we need to
  clear the capsule variable so on the next reset PEI does not see it and
  think there is a capsule available.

  @param BootMode                 the current boot mode

  @retval EFI_INVALID_PARAMETER   boot mode is not correct for an update
  @retval EFI_SUCCESS             There is no error when processing capsule

**/
EFI_STATUS
EFIAPI
BdsProcessCapsules (
  EFI_BOOT_MODE BootMode
  )
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
  BOOLEAN                     NeedReset;

  CapsuleNumber      = 0;
  CapsuleTotalNumber = 0;
  CacheIndex         = 0;
  CacheNumber        = 0;
  CapsulePtr         = NULL;
  CapsulePtrCache    = NULL;
  CapsuleGuidCache   = NULL;
  NeedReset          = FALSE;

  //
  // We don't do anything else if the boot mode is not flash-update
  //
  if (BootMode != BOOT_ON_FLASH_UPDATE) {
    DEBUG ((EFI_D_ERROR, "Boot mode is not correct for capsule update.\n"));
    return EFI_INVALID_PARAMETER;
  }
  
  Status = EFI_SUCCESS;
  //
  // Find all capsule images from hob
  //
  HobPointer.Raw = GetHobList ();
  while ((HobPointer.Raw = GetNextHob (EFI_HOB_TYPE_UEFI_CAPSULE, HobPointer.Raw)) != NULL) {
    CapsuleTotalNumber ++;
    HobPointer.Raw = GET_NEXT_HOB (HobPointer);
  }
  
  if (CapsuleTotalNumber == 0) {
    //
    // We didn't find a hob, so had no errors.
    //
    DEBUG ((EFI_D_ERROR, "We can not find capsule data in capsule update boot mode.\n"));
    DEBUG ((EFI_D_ERROR, "Please check the followings are correct if unexpected capsule update error happens.\n"));
    DEBUG ((EFI_D_ERROR, "1. CapsuleX64 is built as X64 module when PEI is IA32 and DXE is X64\n"));
    DEBUG ((EFI_D_ERROR, "2. Capsule data should persist in memory across a system reset.\n"));
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
  while ((HobPointer.Raw = GetNextHob (EFI_HOB_TYPE_UEFI_CAPSULE, HobPointer.Raw)) != NULL) {
    CapsulePtr [CapsuleNumber++] = (VOID *) (UINTN) HobPointer.Capsule->BaseAddress;
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
      // Always reset system after all capsule processed if FMP capsule exist
      //
      if (CompareGuid (&gEfiFmpCapsuleGuid, &CapsuleHeader->CapsuleGuid)){
        NeedReset = TRUE;
      }

      //
      // Call capsule library to process capsule image.
      //
      ProcessCapsuleImage (CapsuleHeader);
    }
  }

  if (NeedReset) {
    Print(L"Capsule Request Cold Reboot.\n");

    for (Index = 5; Index > 0; Index--) {
      Print(L"\rResetting system in %d seconds ...", Index);
      gBS->Stall (1000000);
    }

    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

    CpuDeadLoop ();
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

