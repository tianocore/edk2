/** @file
  Implement ReadOnly Variable Services required by PEIM and install
  PEI ReadOnly Varaiable2 PPI. These services operates the non volatile storage space.

Copyright (c) 2006 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "VariableParsing.h"
#include "VariableStore.h"


/**
  Get variable store status.

  @param  VarStoreHeader  Pointer to the Variable Store Header.

  @retval  EfiRaw      Variable store is raw
  @retval  EfiValid    Variable store is valid
  @retval  EfiInvalid  Variable store is invalid

**/
VARIABLE_STORE_STATUS
GetVariableStoreStatus (
  IN VARIABLE_STORE_HEADER *VarStoreHeader
  )
{
  if ((CompareGuid (&VarStoreHeader->Signature, &gEfiAuthenticatedVariableGuid) ||
       CompareGuid (&VarStoreHeader->Signature, &gEfiVariableGuid)) &&
      VarStoreHeader->Format == VARIABLE_STORE_FORMATTED &&
      VarStoreHeader->State == VARIABLE_STORE_HEALTHY
      ) {

    return EfiValid;
  }

  if (((UINT32 *)(&VarStoreHeader->Signature))[0] == 0xffffffff &&
      ((UINT32 *)(&VarStoreHeader->Signature))[1] == 0xffffffff &&
      ((UINT32 *)(&VarStoreHeader->Signature))[2] == 0xffffffff &&
      ((UINT32 *)(&VarStoreHeader->Signature))[3] == 0xffffffff &&
      VarStoreHeader->Size == 0xffffffff &&
      VarStoreHeader->Format == 0xff &&
      VarStoreHeader->State == 0xff
      ) {

    return EfiRaw;
  } else {
    return EfiInvalid;
  }
}

/**
  Reports HOB variable store is available or not.

  @retval EFI_NOT_READY  HOB variable store info not available.
  @retval EFI_NOT_FOUND  HOB variable store is NOT available.
  @retval EFI_SUCCESS    HOB variable store is available.
**/
EFI_STATUS
EFIAPI
IsHobVariableStoreAvailable (
  VOID
  )
{
  EFI_HOB_GUID_TYPE      *GuidHob;
  VOID*                  VariableStoreInfoHob;

  //
  // Discover if Variable Store Info Hob has been published by platform driver.
  // It contains information regards to HOB or NV Variable Store availability
  //
  GuidHob = GetFirstGuidHob (&gEfiPeiVariableStoreDiscoveredPpiGuid);
  if(GuidHob == NULL) {
    return EFI_NOT_READY;
  }

  //
  // Check if HOB Variable Store is available
  //
  VariableStoreInfoHob = GET_GUID_HOB_DATA (GuidHob);
  if(*(BOOLEAN *) VariableStoreInfoHob == TRUE) {
    return EFI_SUCCESS;
  }

  //
  // This might be NV Variable Store
  //
  return EFI_NOT_FOUND;
}

/**
  Get HOB variable store.

  @param[out] StoreInfo             Return the store info.
  @param[out] VariableStoreHeader   Return variable store header.

**/
VOID
GetHobVariableStore (
  OUT VARIABLE_STORE_INFO        *StoreInfo
  )
{
  EFI_HOB_GUID_TYPE              *GuidHob;

  //
  // Make sure there is no more than one Variable HOB.
  //
  DEBUG_CODE (
    GuidHob = GetFirstGuidHob (&gEfiAuthenticatedVariableGuid);
    if (GuidHob != NULL) {
      if ((GetNextGuidHob (&gEfiAuthenticatedVariableGuid, GET_NEXT_HOB (GuidHob)) != NULL)) {
        DEBUG ((DEBUG_ERROR, "ERROR: Found two Auth Variable HOBs\n"));
        ASSERT (FALSE);
      } else if (GetFirstGuidHob (&gEfiVariableGuid) != NULL) {
        DEBUG ((DEBUG_ERROR, "ERROR: Found one Auth + one Normal Variable HOBs\n"));
        ASSERT (FALSE);
      }
    } else {
      GuidHob = GetFirstGuidHob (&gEfiVariableGuid);
      if (GuidHob != NULL) {
        if ((GetNextGuidHob (&gEfiVariableGuid, GET_NEXT_HOB (GuidHob)) != NULL)) {
          DEBUG ((DEBUG_ERROR, "ERROR: Found two Normal Variable HOBs\n"));
          ASSERT (FALSE);
        }
      }
    }
  );

  GuidHob = GetFirstGuidHob (&gEfiAuthenticatedVariableGuid);
  if (GuidHob != NULL) {
    StoreInfo->VariableStoreHeader = (VARIABLE_STORE_HEADER *)GET_GUID_HOB_DATA (GuidHob);
    StoreInfo->AuthFlag = TRUE;
  } else {
    GuidHob = GetFirstGuidHob (&gEfiVariableGuid);
    if (GuidHob != NULL) {
      StoreInfo->VariableStoreHeader = (VARIABLE_STORE_HEADER *)GET_GUID_HOB_DATA (GuidHob);
      StoreInfo->AuthFlag = FALSE;
    }
  }
}

/**
  Get NV variable store.

  @param[out] StoreInfo             Return the store info.
  @param[out] VariableStoreHeader   Return header of FV containing the store.

**/
VOID
GetNvVariableStore (
  OUT VARIABLE_STORE_INFO                 *StoreInfo,
  OUT EFI_FIRMWARE_VOLUME_HEADER          **VariableFvHeader
  )
{
  EFI_HOB_GUID_TYPE                     *GuidHob;
  EFI_FIRMWARE_VOLUME_HEADER            *FvHeader;
  VARIABLE_STORE_HEADER                 *StoreHeader;
  FAULT_TOLERANT_WRITE_LAST_WRITE_DATA  *HobData;
  FAULT_TOLERANT_WRITE_LAST_WRITE_DATA  *FtwLastWriteData;
  EFI_PHYSICAL_ADDRESS                  NvStorageBase;
  UINT32                                NvStorageSize;
  UINT32                                BackUpOffset;

  NvStorageSize = PcdGet32 (PcdFlashNvStorageVariableSize);
  NvStorageBase = (EFI_PHYSICAL_ADDRESS)
                  (PcdGet64 (PcdFlashNvStorageVariableBase64) != 0)
                  ? PcdGet64 (PcdFlashNvStorageVariableBase64)
                  : PcdGet32 (PcdFlashNvStorageVariableBase);
  ASSERT (NvStorageBase != 0);

  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)NvStorageBase;

  //
  // Check the FTW last write data hob.
  //
  BackUpOffset      = 0;
  FtwLastWriteData  = NULL;
  HobData           = NULL;
  GuidHob           = GetFirstGuidHob (&gEdkiiFaultTolerantWriteGuid);

  if (GuidHob != NULL) {
    HobData = (FAULT_TOLERANT_WRITE_LAST_WRITE_DATA *)GET_GUID_HOB_DATA (GuidHob);
    if (HobData->TargetAddress == NvStorageBase) {
      //
      // Let FvHeader point to spare block.
      //
      DEBUG ((
        EFI_D_INFO,
        "PeiVariable: NV storage is backed up in spare block: 0x%x\n",
        (UINTN) HobData->SpareAddress
        ));

      FvHeader  = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)HobData->SpareAddress;
      HobData   = NULL;
    } else if ((HobData->TargetAddress > NvStorageBase) &&
               (HobData->TargetAddress < (NvStorageBase + NvStorageSize))) {
      //
      // Flash NV storage from the offset is backed up in spare block.
      //
      BackUpOffset = (UINT32) (HobData->TargetAddress - NvStorageBase);
      DEBUG ((
        EFI_D_INFO,
        "PeiVariable: High partial NV storage from offset: %x is backed up in spare block: 0x%x\n",
        BackUpOffset,
        (UINTN)FtwLastWriteData->SpareAddress
        ));
      //
      // At least one block data in flash NV storage is still valid, so still
      // leave FvHeader point to NV storage base.
      //
    }
  }

  if (StoreInfo != NULL) {
    StoreInfo->FtwLastWriteData = HobData;
  }

  if (VariableFvHeader != NULL) {
    *VariableFvHeader = FvHeader;
  }

  //
  // Check if the Firmware Volume is not corrupted
  //
  if ((FvHeader->Signature == EFI_FVH_SIGNATURE) &&
      CompareGuid (&gEfiSystemNvDataFvGuid, &FvHeader->FileSystemGuid)) {
    StoreHeader = (VARIABLE_STORE_HEADER *)((UINTN)FvHeader + FvHeader->HeaderLength);
  } else {
    StoreHeader = NULL;
    DEBUG ((DEBUG_ERROR, "Firmware Volume for Variable Store is corrupted\n"));
  }

  if (StoreInfo != NULL) {
    StoreInfo->VariableStoreHeader = StoreHeader;
    if (StoreHeader != NULL) {
      StoreInfo->AuthFlag = CompareGuid (
                              &StoreHeader->Signature,
                              &gEfiAuthenticatedVariableGuid
                              );
    }
  }
}

/**
  Return the variable store header and the store info based on the Index.

  @param Type       The type of the variable store.
  @param StoreInfo  Return the store info.

  @return  Pointer to the variable store header.
**/
VARIABLE_STORE_HEADER *
GetVariableStore (
  IN VARIABLE_STORE_TYPE         Type,
  OUT VARIABLE_STORE_INFO        *StoreInfo
  )
{
  EFI_HOB_GUID_TYPE                     *GuidHob;

  StoreInfo->VariableStoreHeader  = NULL;
  StoreInfo->IndexTable           = NULL;
  StoreInfo->FtwLastWriteData     = NULL;
  StoreInfo->AuthFlag             = FALSE;
  switch (Type) {
    case VariableStoreTypeHob:
      GetHobVariableStore (StoreInfo);
      break;

    case VariableStoreTypeNv:
      if (!PcdGetBool (PcdEmuVariableNvModeEnable)) {
        //
        // Emulated non-volatile variable mode is not enabled.
        //
        GetNvVariableStore (StoreInfo, NULL);
        if (StoreInfo->VariableStoreHeader != NULL) {
          GuidHob = GetFirstGuidHob (&gEfiVariableIndexTableGuid);
          if (GuidHob != NULL) {
            StoreInfo->IndexTable = GET_GUID_HOB_DATA (GuidHob);
          } else {
            //
            // If it's the first time to access variable region in flash, create a guid hob to record
            // VAR_ADDED type variable info.
            // Note that as the resource of PEI phase is limited, only store the limited number of
            // VAR_ADDED type variables to reduce access time.
            //
            StoreInfo->IndexTable = (VARIABLE_INDEX_TABLE *) BuildGuidHob (&gEfiVariableIndexTableGuid, sizeof (VARIABLE_INDEX_TABLE));
            StoreInfo->IndexTable->Length      = 0;
            StoreInfo->IndexTable->StartPtr    = GetStartPointer (StoreInfo->VariableStoreHeader);
            StoreInfo->IndexTable->EndPtr      = GetEndPointer   (StoreInfo->VariableStoreHeader);
            StoreInfo->IndexTable->GoneThrough = 0;
          }
        }
      }
      break;

    default:
      ASSERT (FALSE);
      break;
  }

  return StoreInfo->VariableStoreHeader;
}

