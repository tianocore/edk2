/** @file
  Implement ReadOnly Variable Services required by PEIM and install
  PEI ReadOnly Varaiable2 PPI. These services operates the non volatile storage space.

Copyright (c) 2006 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"

//
// Module globals
//
EFI_PEI_READ_ONLY_VARIABLE2_PPI  mVariablePpi = {
  PeiGetVariable,
  PeiGetNextVariableName
};

EFI_PEI_PPI_DESCRIPTOR  mPpiListVariable = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiReadOnlyVariable2PpiGuid,
  &mVariablePpi
};

/**
  Build gEdkiiVariableRuntimeCacheInfoHobGuid.

  @param[in] PeiServices          General purpose services available to every PEIM.
  @param[in] NotifyDescriptor     The notification structure this PEIM registered on install.
  @param[in] Ppi                  The memory discovered PPI.  Not used.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval others                  Failed to build VariableRuntimeCacheInfo Hob.

**/
EFI_STATUS
EFIAPI
BuildVariableRuntimeCacheInfoHob (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR  mPostMemNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMemoryDiscoveredPpiGuid,
  BuildVariableRuntimeCacheInfoHob
};

/**
  Provide the functionality of the variable services.

  @param  FileHandle   Handle of the file being invoked.
                       Type EFI_PEI_FILE_HANDLE is defined in FfsFindNextFile().
  @param  PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS  If the interface could be successfully installed
  @retval Others       Returned from PeiServicesInstallPpi()
**/
EFI_STATUS
EFIAPI
PeimInitializeVariableServices (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  if (FeaturePcdGet (PcdEnableVariableRuntimeCache)) {
    PeiServicesNotifyPpi (&mPostMemNotifyList);
  }

  return PeiServicesInstallPpi (&mPpiListVariable);
}

/**

  Gets the pointer to the first variable header in given variable store area.

  @param VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the first variable header.

**/
VARIABLE_HEADER *
GetStartPointer (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  //
  // The start of variable store
  //
  return (VARIABLE_HEADER *)HEADER_ALIGN (VarStoreHeader + 1);
}

/**

  Gets the pointer to the end of the variable storage area.

  This function gets pointer to the end of the variable storage
  area, according to the input variable store header.

  @param VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the end of the variable storage area.

**/
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *)HEADER_ALIGN ((UINTN)VarStoreHeader + VarStoreHeader->Size);
}

/**
  This code checks if variable header is valid or not.

  @param  Variable  Pointer to the Variable Header.

  @retval TRUE      Variable header is valid.
  @retval FALSE     Variable header is not valid.

**/
BOOLEAN
IsValidVariableHeader (
  IN  VARIABLE_HEADER  *Variable
  )
{
  if ((Variable == NULL) || (Variable->StartId != VARIABLE_DATA)) {
    return FALSE;
  }

  return TRUE;
}

/**
  This code gets the size of variable header.

  @param AuthFlag   Authenticated variable flag.

  @return Size of variable header in bytes in type UINTN.

**/
UINTN
GetVariableHeaderSize (
  IN  BOOLEAN  AuthFlag
  )
{
  UINTN  Value;

  if (AuthFlag) {
    Value = sizeof (AUTHENTICATED_VARIABLE_HEADER);
  } else {
    Value = sizeof (VARIABLE_HEADER);
  }

  return Value;
}

/**
  This code gets the size of name of variable.

  @param  Variable  Pointer to the Variable Header.
  @param  AuthFlag  Authenticated variable flag.

  @return Size of variable in bytes in type UINTN.

**/
UINTN
NameSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFlag
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (AuthFlag) {
    if ((AuthVariable->State == (UINT8)(-1)) ||
        (AuthVariable->DataSize == (UINT32)(-1)) ||
        (AuthVariable->NameSize == (UINT32)(-1)) ||
        (AuthVariable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)AuthVariable->NameSize;
  } else {
    if ((Variable->State == (UINT8)(-1)) ||
        (Variable->DataSize == (UINT32)(-1)) ||
        (Variable->NameSize == (UINT32)(-1)) ||
        (Variable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)Variable->NameSize;
  }
}

/**
  This code gets the size of data of variable.

  @param  Variable  Pointer to the Variable Header.
  @param  AuthFlag  Authenticated variable flag.

  @return Size of variable in bytes in type UINTN.

**/
UINTN
DataSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFlag
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (AuthFlag) {
    if ((AuthVariable->State == (UINT8)(-1)) ||
        (AuthVariable->DataSize == (UINT32)(-1)) ||
        (AuthVariable->NameSize == (UINT32)(-1)) ||
        (AuthVariable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)AuthVariable->DataSize;
  } else {
    if ((Variable->State == (UINT8)(-1)) ||
        (Variable->DataSize == (UINT32)(-1)) ||
        (Variable->NameSize == (UINT32)(-1)) ||
        (Variable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)Variable->DataSize;
  }
}

/**
  This code gets the pointer to the variable name.

  @param   Variable  Pointer to the Variable Header.
  @param   AuthFlag  Authenticated variable flag.

  @return  A CHAR16* pointer to Variable Name.

**/
CHAR16 *
GetVariableNamePtr (
  IN VARIABLE_HEADER  *Variable,
  IN BOOLEAN          AuthFlag
  )
{
  return (CHAR16 *)((UINTN)Variable + GetVariableHeaderSize (AuthFlag));
}

/**
  This code gets the pointer to the variable guid.

  @param Variable   Pointer to the Variable Header.
  @param AuthFlag   Authenticated variable flag.

  @return A EFI_GUID* pointer to Vendor Guid.

**/
EFI_GUID *
GetVendorGuidPtr (
  IN VARIABLE_HEADER  *Variable,
  IN BOOLEAN          AuthFlag
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (AuthFlag) {
    return &AuthVariable->VendorGuid;
  } else {
    return &Variable->VendorGuid;
  }
}

/**
  This code gets the pointer to the variable data.

  @param   Variable         Pointer to the Variable Header.
  @param   VariableHeader   Pointer to the Variable Header that has consecutive content.
  @param   AuthFlag         Authenticated variable flag.

  @return  A UINT8* pointer to Variable Data.

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER  *Variable,
  IN  VARIABLE_HEADER  *VariableHeader,
  IN  BOOLEAN          AuthFlag
  )
{
  UINTN  Value;

  //
  // Be careful about pad size for alignment
  //
  Value  =  (UINTN)GetVariableNamePtr (Variable, AuthFlag);
  Value += NameSizeOfVariable (VariableHeader, AuthFlag);
  Value += GET_PAD_SIZE (NameSizeOfVariable (VariableHeader, AuthFlag));

  return (UINT8 *)Value;
}

/**
  This code gets the pointer to the next variable header.

  @param  StoreInfo         Pointer to variable store info structure.
  @param  Variable          Pointer to the Variable Header.
  @param  VariableHeader    Pointer to the Variable Header that has consecutive content.

  @return  A VARIABLE_HEADER* pointer to next variable header.

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_STORE_INFO  *StoreInfo,
  IN  VARIABLE_HEADER      *Variable,
  IN  VARIABLE_HEADER      *VariableHeader
  )
{
  EFI_PHYSICAL_ADDRESS  TargetAddress;
  EFI_PHYSICAL_ADDRESS  SpareAddress;
  UINTN                 Value;

  Value  =  (UINTN)GetVariableDataPtr (Variable, VariableHeader, StoreInfo->AuthFlag);
  Value += DataSizeOfVariable (VariableHeader, StoreInfo->AuthFlag);
  Value += GET_PAD_SIZE (DataSizeOfVariable (VariableHeader, StoreInfo->AuthFlag));
  //
  // Be careful about pad size for alignment
  //
  Value = HEADER_ALIGN (Value);

  if (StoreInfo->FtwLastWriteData != NULL) {
    TargetAddress = StoreInfo->FtwLastWriteData->TargetAddress;
    SpareAddress  = StoreInfo->FtwLastWriteData->SpareAddress;
    if (((UINTN)Variable < (UINTN)TargetAddress) && (Value >= (UINTN)TargetAddress)) {
      //
      // Next variable is in spare block.
      //
      Value = (UINTN)SpareAddress + (Value - (UINTN)TargetAddress);
    }
  }

  return (VARIABLE_HEADER *)Value;
}

/**
  Get variable store status.

  @param  VarStoreHeader  Pointer to the Variable Store Header.

  @retval  EfiRaw      Variable store is raw
  @retval  EfiValid    Variable store is valid
  @retval  EfiInvalid  Variable store is invalid

**/
VARIABLE_STORE_STATUS
GetVariableStoreStatus (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  if ((CompareGuid (&VarStoreHeader->Signature, &gEfiAuthenticatedVariableGuid) ||
       CompareGuid (&VarStoreHeader->Signature, &gEfiVariableGuid)) &&
      (VarStoreHeader->Format == VARIABLE_STORE_FORMATTED) &&
      (VarStoreHeader->State == VARIABLE_STORE_HEALTHY)
      )
  {
    return EfiValid;
  }

  if ((((UINT32 *)(&VarStoreHeader->Signature))[0] == 0xffffffff) &&
      (((UINT32 *)(&VarStoreHeader->Signature))[1] == 0xffffffff) &&
      (((UINT32 *)(&VarStoreHeader->Signature))[2] == 0xffffffff) &&
      (((UINT32 *)(&VarStoreHeader->Signature))[3] == 0xffffffff) &&
      (VarStoreHeader->Size == 0xffffffff) &&
      (VarStoreHeader->Format == 0xff) &&
      (VarStoreHeader->State == 0xff)
      )
  {
    return EfiRaw;
  } else {
    return EfiInvalid;
  }
}

/**
  Compare two variable names, one of them may be inconsecutive.

  @param StoreInfo      Pointer to variable store info structure.
  @param Name1          Pointer to one variable name.
  @param Name2          Pointer to another variable name.
  @param NameSize       Variable name size.

  @retval TRUE          Name1 and Name2 are identical.
  @retval FALSE         Name1 and Name2 are not identical.

**/
BOOLEAN
CompareVariableName (
  IN VARIABLE_STORE_INFO  *StoreInfo,
  IN CONST CHAR16         *Name1,
  IN CONST CHAR16         *Name2,
  IN UINTN                NameSize
  )
{
  EFI_PHYSICAL_ADDRESS  TargetAddress;
  EFI_PHYSICAL_ADDRESS  SpareAddress;
  UINTN                 PartialNameSize;

  if (StoreInfo->FtwLastWriteData != NULL) {
    TargetAddress = StoreInfo->FtwLastWriteData->TargetAddress;
    SpareAddress  = StoreInfo->FtwLastWriteData->SpareAddress;
    if (((UINTN)Name1 < (UINTN)TargetAddress) && (((UINTN)Name1 + NameSize) > (UINTN)TargetAddress)) {
      //
      // Name1 is inconsecutive.
      //
      PartialNameSize = (UINTN)TargetAddress - (UINTN)Name1;
      //
      // Partial content is in NV storage.
      //
      if (CompareMem ((UINT8 *)Name1, (UINT8 *)Name2, PartialNameSize) == 0) {
        //
        // Another partial content is in spare block.
        //
        if (CompareMem ((UINT8 *)(UINTN)SpareAddress, (UINT8 *)Name2 + PartialNameSize, NameSize - PartialNameSize) == 0) {
          return TRUE;
        }
      }

      return FALSE;
    } else if (((UINTN)Name2 < (UINTN)TargetAddress) && (((UINTN)Name2 + NameSize) > (UINTN)TargetAddress)) {
      //
      // Name2 is inconsecutive.
      //
      PartialNameSize = (UINTN)TargetAddress - (UINTN)Name2;
      //
      // Partial content is in NV storage.
      //
      if (CompareMem ((UINT8 *)Name2, (UINT8 *)Name1, PartialNameSize) == 0) {
        //
        // Another partial content is in spare block.
        //
        if (CompareMem ((UINT8 *)(UINTN)SpareAddress, (UINT8 *)Name1 + PartialNameSize, NameSize - PartialNameSize) == 0) {
          return TRUE;
        }
      }

      return FALSE;
    }
  }

  //
  // Both Name1 and Name2 are consecutive.
  //
  if (CompareMem ((UINT8 *)Name1, (UINT8 *)Name2, NameSize) == 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  This function compares a variable with variable entries in database.

  @param  StoreInfo     Pointer to variable store info structure.
  @param  Variable      Pointer to the variable in our database
  @param  VariableHeader Pointer to the Variable Header that has consecutive content.
  @param  VariableName  Name of the variable to compare to 'Variable'
  @param  VendorGuid    GUID of the variable to compare to 'Variable'
  @param  PtrTrack      Variable Track Pointer structure that contains Variable Information.

  @retval EFI_SUCCESS    Found match variable
  @retval EFI_NOT_FOUND  Variable not found

**/
EFI_STATUS
CompareWithValidVariable (
  IN  VARIABLE_STORE_INFO     *StoreInfo,
  IN  VARIABLE_HEADER         *Variable,
  IN  VARIABLE_HEADER         *VariableHeader,
  IN  CONST CHAR16            *VariableName,
  IN  CONST EFI_GUID          *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack
  )
{
  VOID      *Point;
  EFI_GUID  *TempVendorGuid;

  TempVendorGuid = GetVendorGuidPtr (VariableHeader, StoreInfo->AuthFlag);

  if (VariableName[0] == 0) {
    PtrTrack->CurrPtr = Variable;
    return EFI_SUCCESS;
  } else {
    //
    // Don't use CompareGuid function here for performance reasons.
    // Instead we compare the GUID a UINT32 at a time and branch
    // on the first failed comparison.
    //
    if ((((INT32 *)VendorGuid)[0] == ((INT32 *)TempVendorGuid)[0]) &&
        (((INT32 *)VendorGuid)[1] == ((INT32 *)TempVendorGuid)[1]) &&
        (((INT32 *)VendorGuid)[2] == ((INT32 *)TempVendorGuid)[2]) &&
        (((INT32 *)VendorGuid)[3] == ((INT32 *)TempVendorGuid)[3])
        )
    {
      ASSERT (NameSizeOfVariable (VariableHeader, StoreInfo->AuthFlag) != 0);
      Point = (VOID *)GetVariableNamePtr (Variable, StoreInfo->AuthFlag);
      if (CompareVariableName (StoreInfo, VariableName, Point, NameSizeOfVariable (VariableHeader, StoreInfo->AuthFlag))) {
        PtrTrack->CurrPtr = Variable;
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Get HOB variable store.

  @param[out] StoreInfo             Return the store info.
  @param[out] VariableStoreHeader   Return variable store header.

**/
VOID
GetHobVariableStore (
  OUT VARIABLE_STORE_INFO    *StoreInfo,
  OUT VARIABLE_STORE_HEADER  **VariableStoreHeader
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  //
  // Make sure there is no more than one Variable HOB.
  //
  DEBUG_CODE_BEGIN ();
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

  DEBUG_CODE_END ();

  GuidHob = GetFirstGuidHob (&gEfiAuthenticatedVariableGuid);
  if (GuidHob != NULL) {
    *VariableStoreHeader = (VARIABLE_STORE_HEADER *)GET_GUID_HOB_DATA (GuidHob);
    StoreInfo->AuthFlag  = TRUE;
  } else {
    GuidHob = GetFirstGuidHob (&gEfiVariableGuid);
    if (GuidHob != NULL) {
      *VariableStoreHeader = (VARIABLE_STORE_HEADER *)GET_GUID_HOB_DATA (GuidHob);
      StoreInfo->AuthFlag  = FALSE;
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
  IN VARIABLE_STORE_TYPE   Type,
  OUT VARIABLE_STORE_INFO  *StoreInfo
  )
{
  EFI_STATUS                            Status;
  EFI_HOB_GUID_TYPE                     *GuidHob;
  EFI_FIRMWARE_VOLUME_HEADER            *FvHeader;
  VARIABLE_STORE_HEADER                 *VariableStoreHeader;
  EFI_PHYSICAL_ADDRESS                  NvStorageBase;
  UINT32                                NvStorageSize;
  UINT64                                NvStorageSize64;
  FAULT_TOLERANT_WRITE_LAST_WRITE_DATA  *FtwLastWriteData;
  UINT32                                BackUpOffset;

  StoreInfo->IndexTable       = NULL;
  StoreInfo->FtwLastWriteData = NULL;
  StoreInfo->AuthFlag         = FALSE;
  VariableStoreHeader         = NULL;
  switch (Type) {
    case VariableStoreTypeHob:
      GetHobVariableStore (StoreInfo, &VariableStoreHeader);

      break;

    case VariableStoreTypeNv:
      if (!PcdGetBool (PcdEmuVariableNvModeEnable)) {
        //
        // Emulated non-volatile variable mode is not enabled.
        //

        Status = GetVariableFlashNvStorageInfo (&NvStorageBase, &NvStorageSize64);
        ASSERT_EFI_ERROR (Status);

        Status = SafeUint64ToUint32 (NvStorageSize64, &NvStorageSize);
        // This driver currently assumes the size will be UINT32 so assert the value is safe for now.
        ASSERT_EFI_ERROR (Status);

        ASSERT (NvStorageBase != 0);

        //
        // First let FvHeader point to NV storage base.
        //
        FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)NvStorageBase;

        //
        // Check the FTW last write data hob.
        //
        BackUpOffset = 0;
        GuidHob      = GetFirstGuidHob (&gEdkiiFaultTolerantWriteGuid);
        if (GuidHob != NULL) {
          FtwLastWriteData = (FAULT_TOLERANT_WRITE_LAST_WRITE_DATA *)GET_GUID_HOB_DATA (GuidHob);
          if (FtwLastWriteData->TargetAddress == NvStorageBase) {
            //
            // Let FvHeader point to spare block.
            //
            FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FtwLastWriteData->SpareAddress;
            DEBUG ((DEBUG_INFO, "PeiVariable: NV storage is backed up in spare block: 0x%x\n", (UINTN)FtwLastWriteData->SpareAddress));
          } else if ((FtwLastWriteData->TargetAddress > NvStorageBase) && (FtwLastWriteData->TargetAddress < (NvStorageBase + NvStorageSize))) {
            StoreInfo->FtwLastWriteData = FtwLastWriteData;
            //
            // Flash NV storage from the offset is backed up in spare block.
            //
            BackUpOffset = (UINT32)(FtwLastWriteData->TargetAddress - NvStorageBase);
            DEBUG ((DEBUG_INFO, "PeiVariable: High partial NV storage from offset: %x is backed up in spare block: 0x%x\n", BackUpOffset, (UINTN)FtwLastWriteData->SpareAddress));
            //
            // At least one block data in flash NV storage is still valid, so still leave FvHeader point to NV storage base.
            //
          }
        }

        //
        // Check if the Firmware Volume is not corrupted
        //
        if ((FvHeader->Signature != EFI_FVH_SIGNATURE) || (!CompareGuid (&gEfiSystemNvDataFvGuid, &FvHeader->FileSystemGuid))) {
          DEBUG ((DEBUG_ERROR, "Firmware Volume for Variable Store is corrupted\n"));
          break;
        }

        VariableStoreHeader = (VARIABLE_STORE_HEADER *)((UINT8 *)FvHeader + FvHeader->HeaderLength);

        StoreInfo->AuthFlag = (BOOLEAN)(CompareGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid));

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
          StoreInfo->IndexTable              = (VARIABLE_INDEX_TABLE *)BuildGuidHob (&gEfiVariableIndexTableGuid, sizeof (VARIABLE_INDEX_TABLE));
          StoreInfo->IndexTable->Length      = 0;
          StoreInfo->IndexTable->StartPtr    = GetStartPointer (VariableStoreHeader);
          StoreInfo->IndexTable->EndPtr      = GetEndPointer (VariableStoreHeader);
          StoreInfo->IndexTable->GoneThrough = 0;
        }
      }

      break;

    default:
      ASSERT (FALSE);
      break;
  }

  StoreInfo->VariableStoreHeader = VariableStoreHeader;
  return VariableStoreHeader;
}

/**
  Get variable header that has consecutive content.

  @param StoreInfo      Pointer to variable store info structure.
  @param Variable       Pointer to the Variable Header.
  @param VariableHeader Pointer to Pointer to the Variable Header that has consecutive content.

  @retval TRUE          Variable header is valid.
  @retval FALSE         Variable header is not valid.

**/
BOOLEAN
GetVariableHeader (
  IN VARIABLE_STORE_INFO  *StoreInfo,
  IN VARIABLE_HEADER      *Variable,
  OUT VARIABLE_HEADER     **VariableHeader
  )
{
  EFI_PHYSICAL_ADDRESS  TargetAddress;
  EFI_PHYSICAL_ADDRESS  SpareAddress;
  EFI_HOB_GUID_TYPE     *GuidHob;
  UINTN                 PartialHeaderSize;

  if (Variable == NULL) {
    return FALSE;
  }

  //
  // First assume variable header pointed by Variable is consecutive.
  //
  *VariableHeader = Variable;

  if (StoreInfo->FtwLastWriteData != NULL) {
    TargetAddress = StoreInfo->FtwLastWriteData->TargetAddress;
    SpareAddress  = StoreInfo->FtwLastWriteData->SpareAddress;
    if (((UINTN)Variable > (UINTN)SpareAddress) &&
        (((UINTN)Variable - (UINTN)SpareAddress + (UINTN)TargetAddress) >= (UINTN)GetEndPointer (StoreInfo->VariableStoreHeader)))
    {
      //
      // Reach the end of variable store.
      //
      return FALSE;
    }

    if (((UINTN)Variable < (UINTN)TargetAddress) && (((UINTN)Variable + GetVariableHeaderSize (StoreInfo->AuthFlag)) > (UINTN)TargetAddress)) {
      //
      // Variable header pointed by Variable is inconsecutive,
      // create a guid hob to combine the two partial variable header content together.
      //
      GuidHob = GetFirstGuidHob (&gEfiCallerIdGuid);
      if (GuidHob != NULL) {
        *VariableHeader = (VARIABLE_HEADER *)GET_GUID_HOB_DATA (GuidHob);
      } else {
        *VariableHeader   = (VARIABLE_HEADER *)BuildGuidHob (&gEfiCallerIdGuid, GetVariableHeaderSize (StoreInfo->AuthFlag));
        PartialHeaderSize = (UINTN)TargetAddress - (UINTN)Variable;
        //
        // Partial content is in NV storage.
        //
        CopyMem ((UINT8 *)*VariableHeader, (UINT8 *)Variable, PartialHeaderSize);
        //
        // Another partial content is in spare block.
        //
        CopyMem ((UINT8 *)*VariableHeader + PartialHeaderSize, (UINT8 *)(UINTN)SpareAddress, GetVariableHeaderSize (StoreInfo->AuthFlag) - PartialHeaderSize);
      }
    }
  } else {
    if (Variable >= GetEndPointer (StoreInfo->VariableStoreHeader)) {
      //
      // Reach the end of variable store.
      //
      return FALSE;
    }
  }

  return IsValidVariableHeader (*VariableHeader);
}

/**
  Get variable name or data to output buffer.

  @param  StoreInfo     Pointer to variable store info structure.
  @param  NameOrData    Pointer to the variable name/data that may be inconsecutive.
  @param  Size          Variable name/data size.
  @param  Buffer        Pointer to output buffer to hold the variable name/data.

**/
VOID
GetVariableNameOrData (
  IN VARIABLE_STORE_INFO  *StoreInfo,
  IN UINT8                *NameOrData,
  IN UINTN                Size,
  OUT UINT8               *Buffer
  )
{
  EFI_PHYSICAL_ADDRESS  TargetAddress;
  EFI_PHYSICAL_ADDRESS  SpareAddress;
  UINTN                 PartialSize;

  if (StoreInfo->FtwLastWriteData != NULL) {
    TargetAddress = StoreInfo->FtwLastWriteData->TargetAddress;
    SpareAddress  = StoreInfo->FtwLastWriteData->SpareAddress;
    if (((UINTN)NameOrData < (UINTN)TargetAddress) && (((UINTN)NameOrData + Size) > (UINTN)TargetAddress)) {
      //
      // Variable name/data is inconsecutive.
      //
      PartialSize = (UINTN)TargetAddress - (UINTN)NameOrData;
      //
      // Partial content is in NV storage.
      //
      CopyMem (Buffer, NameOrData, PartialSize);
      //
      // Another partial content is in spare block.
      //
      CopyMem (Buffer + PartialSize, (UINT8 *)(UINTN)SpareAddress, Size - PartialSize);
      return;
    }
  }

  //
  // Variable name/data is consecutive.
  //
  CopyMem (Buffer, NameOrData, Size);
}

/**
  Find the variable in the specified variable store.

  @param  StoreInfo           Pointer to the store info structure.
  @param  VariableName        Name of the variable to be found
  @param  VendorGuid          Vendor GUID to be found.
  @param  PtrTrack            Variable Track Pointer structure that contains Variable Information.

  @retval  EFI_SUCCESS            Variable found successfully
  @retval  EFI_NOT_FOUND          Variable not found
  @retval  EFI_INVALID_PARAMETER  Invalid variable name

**/
EFI_STATUS
FindVariableEx (
  IN VARIABLE_STORE_INFO      *StoreInfo,
  IN CONST CHAR16             *VariableName,
  IN CONST EFI_GUID           *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack
  )
{
  VARIABLE_HEADER        *Variable;
  VARIABLE_HEADER        *LastVariable;
  VARIABLE_HEADER        *MaxIndex;
  UINTN                  Index;
  UINTN                  Offset;
  BOOLEAN                StopRecord;
  VARIABLE_HEADER        *InDeletedVariable;
  VARIABLE_STORE_HEADER  *VariableStoreHeader;
  VARIABLE_INDEX_TABLE   *IndexTable;
  VARIABLE_HEADER        *VariableHeader;
  BOOLEAN                Valid;

  VariableStoreHeader = StoreInfo->VariableStoreHeader;

  if (VariableStoreHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (GetVariableStoreStatus (VariableStoreHeader) != EfiValid) {
    return EFI_UNSUPPORTED;
  }

  if (~VariableStoreHeader->Size == 0) {
    return EFI_NOT_FOUND;
  }

  IndexTable         = StoreInfo->IndexTable;
  PtrTrack->StartPtr = GetStartPointer (VariableStoreHeader);
  PtrTrack->EndPtr   = GetEndPointer (VariableStoreHeader);

  InDeletedVariable = NULL;

  //
  // No Variable Address equals zero, so 0 as initial value is safe.
  //
  MaxIndex       = NULL;
  VariableHeader = NULL;

  if (IndexTable != NULL) {
    //
    // traverse the variable index table to look for varible.
    // The IndexTable->Index[Index] records the distance of two neighbouring VAR_ADDED type variables.
    //
    for (Offset = 0, Index = 0; Index < IndexTable->Length; Index++) {
      ASSERT (Index < sizeof (IndexTable->Index) / sizeof (IndexTable->Index[0]));
      Offset  += IndexTable->Index[Index];
      MaxIndex = (VARIABLE_HEADER *)((UINT8 *)IndexTable->StartPtr + Offset);
      Valid    = GetVariableHeader (StoreInfo, MaxIndex, &VariableHeader);
      if (Valid && (CompareWithValidVariable (StoreInfo, MaxIndex, VariableHeader, VariableName, VendorGuid, PtrTrack) == EFI_SUCCESS)) {
        if (VariableHeader->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
          InDeletedVariable = PtrTrack->CurrPtr;
        } else {
          return EFI_SUCCESS;
        }
      }
    }

    if (IndexTable->GoneThrough != 0) {
      //
      // If the table has all the existing variables indexed, return.
      //
      PtrTrack->CurrPtr = InDeletedVariable;
      return (PtrTrack->CurrPtr == NULL) ? EFI_NOT_FOUND : EFI_SUCCESS;
    }
  }

  if ((MaxIndex != NULL) && (VariableHeader != NULL)) {
    //
    // HOB exists but the variable cannot be found in HOB
    // If not found in HOB, then let's start from the MaxIndex we've found.
    //
    Variable     = GetNextVariablePtr (StoreInfo, MaxIndex, VariableHeader);
    LastVariable = MaxIndex;
  } else {
    //
    // Start Pointers for the variable.
    // Actual Data Pointer where data can be written.
    //
    Variable     = PtrTrack->StartPtr;
    LastVariable = PtrTrack->StartPtr;
  }

  //
  // Find the variable by walk through variable store
  //
  StopRecord = FALSE;
  while (GetVariableHeader (StoreInfo, Variable, &VariableHeader)) {
    if ((VariableHeader->State == VAR_ADDED) || (VariableHeader->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED))) {
      //
      // Record Variable in VariableIndex HOB
      //
      if ((IndexTable != NULL) && !StopRecord) {
        Offset = (UINTN)Variable - (UINTN)LastVariable;
        if ((Offset > 0x0FFFF) || (IndexTable->Length >= sizeof (IndexTable->Index) / sizeof (IndexTable->Index[0]))) {
          //
          // Stop to record if the distance of two neighbouring VAR_ADDED variable is larger than the allowable scope(UINT16),
          // or the record buffer is full.
          //
          StopRecord = TRUE;
        } else {
          IndexTable->Index[IndexTable->Length++] = (UINT16)Offset;
          LastVariable                            = Variable;
        }
      }

      if (CompareWithValidVariable (StoreInfo, Variable, VariableHeader, VariableName, VendorGuid, PtrTrack) == EFI_SUCCESS) {
        if (VariableHeader->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
          InDeletedVariable = PtrTrack->CurrPtr;
        } else {
          return EFI_SUCCESS;
        }
      }
    }

    Variable = GetNextVariablePtr (StoreInfo, Variable, VariableHeader);
  }

  //
  // If gone through the VariableStore, that means we never find in Firmware any more.
  //
  if ((IndexTable != NULL) && !StopRecord) {
    IndexTable->GoneThrough = 1;
  }

  PtrTrack->CurrPtr = InDeletedVariable;

  return (PtrTrack->CurrPtr == NULL) ? EFI_NOT_FOUND : EFI_SUCCESS;
}

/**
  Find the variable in HOB and Non-Volatile variable storages.

  @param  VariableName  Name of the variable to be found
  @param  VendorGuid    Vendor GUID to be found.
  @param  PtrTrack      Variable Track Pointer structure that contains Variable Information.
  @param  StoreInfo     Return the store info.

  @retval  EFI_SUCCESS            Variable found successfully
  @retval  EFI_NOT_FOUND          Variable not found
  @retval  EFI_INVALID_PARAMETER  Invalid variable name
**/
EFI_STATUS
FindVariable (
  IN CONST  CHAR16            *VariableName,
  IN CONST  EFI_GUID          *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  OUT VARIABLE_STORE_INFO     *StoreInfo
  )
{
  EFI_STATUS           Status;
  VARIABLE_STORE_TYPE  Type;

  if ((VariableName[0] != 0) && (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Type = (VARIABLE_STORE_TYPE)0; Type < VariableStoreTypeMax; Type++) {
    GetVariableStore (Type, StoreInfo);
    Status = FindVariableEx (
               StoreInfo,
               VariableName,
               VendorGuid,
               PtrTrack
               );
    if (!EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  This service retrieves a variable's value using its name and GUID.

  Read the specified variable from the UEFI variable store. If the Data
  buffer is too small to hold the contents of the variable, the error
  EFI_BUFFER_TOO_SMALL is returned and DataSize is set to the required buffer
  size to obtain the data.

  @param  This                  A pointer to this instance of the EFI_PEI_READ_ONLY_VARIABLE2_PPI.
  @param  VariableName          A pointer to a null-terminated string that is the variable's name.
  @param  VariableGuid          A pointer to an EFI_GUID that is the variable's GUID. The combination of
                                VariableGuid and VariableName must be unique.
  @param  Attributes            If non-NULL, on return, points to the variable's attributes.
  @param  DataSize              On entry, points to the size in bytes of the Data buffer.
                                On return, points to the size of the data returned in Data.
  @param  Data                  Points to the buffer which will hold the returned variable value.
                                May be NULL with a zero DataSize in order to determine the size of the buffer needed.

  @retval EFI_SUCCESS           The variable was read successfully.
  @retval EFI_NOT_FOUND         The variable was be found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the resulting data.
                                DataSize is updated with the size required for
                                the specified variable.
  @retval EFI_INVALID_PARAMETER VariableName, VariableGuid, DataSize or Data is NULL.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved because of a device error.

**/
EFI_STATUS
EFIAPI
PeiGetVariable (
  IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *This,
  IN CONST  CHAR16                           *VariableName,
  IN CONST  EFI_GUID                         *VariableGuid,
  OUT       UINT32                           *Attributes,
  IN OUT    UINTN                            *DataSize,
  OUT       VOID                             *Data OPTIONAL
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarDataSize;
  EFI_STATUS              Status;
  VARIABLE_STORE_INFO     StoreInfo;
  VARIABLE_HEADER         *VariableHeader;

  if ((VariableName == NULL) || (VariableGuid == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableName[0] == 0) {
    return EFI_NOT_FOUND;
  }

  VariableHeader = NULL;

  //
  // Find existing variable
  //
  Status = FindVariable (VariableName, VariableGuid, &Variable, &StoreInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  GetVariableHeader (&StoreInfo, Variable.CurrPtr, &VariableHeader);

  //
  // Get data size
  //
  VarDataSize = DataSizeOfVariable (VariableHeader, StoreInfo.AuthFlag);
  if (*DataSize >= VarDataSize) {
    if (Data == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    GetVariableNameOrData (&StoreInfo, GetVariableDataPtr (Variable.CurrPtr, VariableHeader, StoreInfo.AuthFlag), VarDataSize, Data);
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_BUFFER_TOO_SMALL;
  }

  if (Attributes != NULL) {
    *Attributes = VariableHeader->Attributes;
  }

  *DataSize = VarDataSize;

  return Status;
}

/**
  Return the next variable name and GUID.

  This function is called multiple times to retrieve the VariableName
  and VariableGuid of all variables currently available in the system.
  On each call, the previous results are passed into the interface,
  and, on return, the interface returns the data for the next
  interface. When the entire variable list has been returned,
  EFI_NOT_FOUND is returned.

  @param  This              A pointer to this instance of the EFI_PEI_READ_ONLY_VARIABLE2_PPI.

  @param  VariableNameSize  On entry, points to the size of the buffer pointed to by VariableName.
                            On return, the size of the variable name buffer.
  @param  VariableName      On entry, a pointer to a null-terminated string that is the variable's name.
                            On return, points to the next variable's null-terminated name string.
  @param  VariableGuid      On entry, a pointer to an EFI_GUID that is the variable's GUID.
                            On return, a pointer to the next variable's GUID.

  @retval EFI_SUCCESS           The variable was read successfully.
  @retval EFI_NOT_FOUND         The variable could not be found.
  @retval EFI_BUFFER_TOO_SMALL  The VariableNameSize is too small for the resulting
                                data. VariableNameSize is updated with the size
                                required for the specified variable.
  @retval EFI_INVALID_PARAMETER VariableName, VariableGuid or
                                VariableNameSize is NULL.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved because of a device error.

**/
EFI_STATUS
EFIAPI
PeiGetNextVariableName (
  IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *This,
  IN OUT UINTN                               *VariableNameSize,
  IN OUT CHAR16                              *VariableName,
  IN OUT EFI_GUID                            *VariableGuid
  )
{
  VARIABLE_STORE_TYPE     Type;
  VARIABLE_POINTER_TRACK  Variable;
  VARIABLE_POINTER_TRACK  VariableInHob;
  VARIABLE_POINTER_TRACK  VariablePtrTrack;
  UINTN                   VarNameSize;
  EFI_STATUS              Status;
  VARIABLE_STORE_HEADER   *VariableStoreHeader[VariableStoreTypeMax];
  VARIABLE_HEADER         *VariableHeader;
  VARIABLE_STORE_INFO     StoreInfo;
  VARIABLE_STORE_INFO     StoreInfoForNv;
  VARIABLE_STORE_INFO     StoreInfoForHob;

  if ((VariableName == NULL) || (VariableGuid == NULL) || (VariableNameSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  VariableHeader = NULL;

  Status = FindVariable (VariableName, VariableGuid, &Variable, &StoreInfo);
  if ((Status != EFI_SUCCESS) || (Variable.CurrPtr == NULL)) {
    return Status;
  }

  if (VariableName[0] != 0) {
    //
    // If variable name is not NULL, get next variable
    //
    GetVariableHeader (&StoreInfo, Variable.CurrPtr, &VariableHeader);
    Variable.CurrPtr = GetNextVariablePtr (&StoreInfo, Variable.CurrPtr, VariableHeader);
  }

  VariableStoreHeader[VariableStoreTypeHob] = GetVariableStore (VariableStoreTypeHob, &StoreInfoForHob);
  VariableStoreHeader[VariableStoreTypeNv]  = GetVariableStore (VariableStoreTypeNv, &StoreInfoForNv);

  while (TRUE) {
    //
    // Switch from HOB to Non-Volatile.
    //
    while (!GetVariableHeader (&StoreInfo, Variable.CurrPtr, &VariableHeader)) {
      //
      // Find current storage index
      //
      for (Type = (VARIABLE_STORE_TYPE)0; Type < VariableStoreTypeMax; Type++) {
        if ((VariableStoreHeader[Type] != NULL) && (Variable.StartPtr == GetStartPointer (VariableStoreHeader[Type]))) {
          break;
        }
      }

      ASSERT (Type < VariableStoreTypeMax);
      //
      // Switch to next storage
      //
      for (Type++; Type < VariableStoreTypeMax; Type++) {
        if (VariableStoreHeader[Type] != NULL) {
          break;
        }
      }

      //
      // Capture the case that
      // 1. current storage is the last one, or
      // 2. no further storage
      //
      if (Type == VariableStoreTypeMax) {
        return EFI_NOT_FOUND;
      }

      Variable.StartPtr = GetStartPointer (VariableStoreHeader[Type]);
      Variable.EndPtr   = GetEndPointer (VariableStoreHeader[Type]);
      Variable.CurrPtr  = Variable.StartPtr;
      GetVariableStore (Type, &StoreInfo);
    }

    if ((VariableHeader->State == VAR_ADDED) || (VariableHeader->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED))) {
      if (VariableHeader->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
        //
        // If it is a IN_DELETED_TRANSITION variable,
        // and there is also a same ADDED one at the same time,
        // don't return it.
        //
        Status = FindVariableEx (
                   &StoreInfo,
                   GetVariableNamePtr (Variable.CurrPtr, StoreInfo.AuthFlag),
                   GetVendorGuidPtr (VariableHeader, StoreInfo.AuthFlag),
                   &VariablePtrTrack
                   );
        if (!EFI_ERROR (Status) && (VariablePtrTrack.CurrPtr != Variable.CurrPtr)) {
          Variable.CurrPtr = GetNextVariablePtr (&StoreInfo, Variable.CurrPtr, VariableHeader);
          continue;
        }
      }

      //
      // Don't return NV variable when HOB overrides it
      //
      if ((VariableStoreHeader[VariableStoreTypeHob] != NULL) && (VariableStoreHeader[VariableStoreTypeNv] != NULL) &&
          (Variable.StartPtr == GetStartPointer (VariableStoreHeader[VariableStoreTypeNv]))
          )
      {
        Status = FindVariableEx (
                   &StoreInfoForHob,
                   GetVariableNamePtr (Variable.CurrPtr, StoreInfo.AuthFlag),
                   GetVendorGuidPtr (VariableHeader, StoreInfo.AuthFlag),
                   &VariableInHob
                   );
        if (!EFI_ERROR (Status)) {
          Variable.CurrPtr = GetNextVariablePtr (&StoreInfo, Variable.CurrPtr, VariableHeader);
          continue;
        }
      }

      VarNameSize = NameSizeOfVariable (VariableHeader, StoreInfo.AuthFlag);
      ASSERT (VarNameSize != 0);

      if (VarNameSize <= *VariableNameSize) {
        GetVariableNameOrData (&StoreInfo, (UINT8 *)GetVariableNamePtr (Variable.CurrPtr, StoreInfo.AuthFlag), VarNameSize, (UINT8 *)VariableName);

        CopyMem (VariableGuid, GetVendorGuidPtr (VariableHeader, StoreInfo.AuthFlag), sizeof (EFI_GUID));

        Status = EFI_SUCCESS;
      } else {
        Status = EFI_BUFFER_TOO_SMALL;
      }

      *VariableNameSize = VarNameSize;
      //
      // Variable is found
      //
      return Status;
    } else {
      Variable.CurrPtr = GetNextVariablePtr (&StoreInfo, Variable.CurrPtr, VariableHeader);
    }
  }
}

/**
  Calculate the auth variable storage size converted from normal variable storage.

  @param[in]  StoreInfo         Pointer to the store info
  @param[in]  NormalHobVarStorage  Pointer to the normal variable storage header

  @retval the auth variable storage size
**/
UINTN
CalculateAuthVarStorageSize (
  IN  VARIABLE_STORE_INFO    *StoreInfo,
  IN  VARIABLE_STORE_HEADER  *NormalHobVarStorage
  )
{
  VARIABLE_HEADER  *StartPtr;
  VARIABLE_HEADER  *EndPtr;
  UINTN            AuthVarStroageSize;

  AuthVarStroageSize = sizeof (VARIABLE_STORE_HEADER);

  //
  // Calculate Auth Variable Storage Size
  //
  StartPtr = GetStartPointer (NormalHobVarStorage);
  EndPtr   = GetEndPointer (NormalHobVarStorage);
  while (StartPtr < EndPtr) {
    if (StartPtr->State == VAR_ADDED) {
      AuthVarStroageSize  = HEADER_ALIGN (AuthVarStroageSize);
      AuthVarStroageSize += sizeof (AUTHENTICATED_VARIABLE_HEADER);
      AuthVarStroageSize += StartPtr->NameSize + GET_PAD_SIZE (StartPtr->NameSize);
      AuthVarStroageSize += StartPtr->DataSize + GET_PAD_SIZE (StartPtr->DataSize);
    }

    StartPtr = GetNextVariablePtr (StoreInfo, StartPtr, StartPtr);
  }

  return AuthVarStroageSize;
}

/**
  Calculate Hob variable cache size.

  @param[in]  NvAuthFlag   If the NV variable store is Auth.

  @retval Maximum of Nv variable cache size.

**/
UINTN
CalculateHobVariableCacheSize (
  IN BOOLEAN  NvAuthFlag
  )
{
  VARIABLE_STORE_INFO    StoreInfo;
  VARIABLE_STORE_HEADER  *VariableStoreHeader;

  VariableStoreHeader = NULL;
  ZeroMem (&StoreInfo, sizeof (VARIABLE_STORE_INFO));
  GetHobVariableStore (&StoreInfo, &VariableStoreHeader);

  if (VariableStoreHeader == NULL) {
    return 0;
  }

  if (NvAuthFlag == StoreInfo.AuthFlag) {
    return VariableStoreHeader->Size;
  } else {
    //
    // Normal NV variable store + Auth HOB variable store is not supported
    //
    ASSERT (NvAuthFlag && (!StoreInfo.AuthFlag));

    //
    // Need to calculate auth variable storage size converted from normal variable storage
    //
    return CalculateAuthVarStorageSize (&StoreInfo, VariableStoreHeader);
  }
}

/**
  Calculate Nv variable cache size.

  @param[out]  NvAuthFlag   If the NV variable store is Auth.

  @retval Maximum of Nv variable cache size.

**/
UINTN
CalculateNvVariableCacheSize (
  OUT BOOLEAN  *NvAuthFlag
  )
{
  EFI_STATUS                            Status;
  EFI_HOB_GUID_TYPE                     *GuidHob;
  EFI_FIRMWARE_VOLUME_HEADER            *FvHeader;
  VARIABLE_STORE_HEADER                 *VariableStoreHeader;
  EFI_PHYSICAL_ADDRESS                  NvStorageBase;
  UINT32                                NvStorageSize;
  UINT64                                NvStorageSize64;
  FAULT_TOLERANT_WRITE_LAST_WRITE_DATA  *FtwLastWriteData;

  if (PcdGetBool (PcdEmuVariableNvModeEnable)) {
    return PcdGet32 (PcdVariableStoreSize);
  }

  Status = GetVariableFlashNvStorageInfo (&NvStorageBase, &NvStorageSize64);
  ASSERT_EFI_ERROR (Status);

  Status = SafeUint64ToUint32 (NvStorageSize64, &NvStorageSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (NvStorageBase != 0);
  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)NvStorageBase;

  //
  // Check the FTW last write data hob.
  //
  GuidHob = GetFirstGuidHob (&gEdkiiFaultTolerantWriteGuid);
  if (GuidHob != NULL) {
    FtwLastWriteData = (FAULT_TOLERANT_WRITE_LAST_WRITE_DATA *)GET_GUID_HOB_DATA (GuidHob);
    if (FtwLastWriteData->TargetAddress == NvStorageBase) {
      //
      // Let FvHeader point to spare block.
      //
      FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FtwLastWriteData->SpareAddress;
    }
  }

  VariableStoreHeader = (VARIABLE_STORE_HEADER *)((UINT8 *)FvHeader + FvHeader->HeaderLength);
  *NvAuthFlag         = (BOOLEAN)(CompareGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid));

  return NvStorageSize - FvHeader->HeaderLength;
}

/**
  Build gEdkiiVariableRuntimeCacheInfoHobGuid.

  @param[in] PeiServices          General purpose services available to every PEIM.
  @param[in] NotifyDescriptor     The notification structure this PEIM registered on install.
  @param[in] Ppi                  The memory discovered PPI.  Not used.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval others                  Failed to build VariableRuntimeCacheInfo Hob.

**/
EFI_STATUS
EFIAPI
BuildVariableRuntimeCacheInfoHob (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  VARIABLE_RUNTIME_CACHE_INFO  TempHobBuffer;
  VARIABLE_RUNTIME_CACHE_INFO  *VariableRuntimeCacheInfo;
  EFI_STATUS                   Status;
  VOID                         *Buffer;
  UINTN                        BufferSize;
  BOOLEAN                      NvAuthFlag;
  UINTN                        Pages;

  ZeroMem (&TempHobBuffer, sizeof (VARIABLE_RUNTIME_CACHE_INFO));

  //
  // AllocateRuntimePages for CACHE_INFO_FLAG and unblock it.
  //
  Pages  = EFI_SIZE_TO_PAGES (sizeof (CACHE_INFO_FLAG));
  Buffer = AllocateRuntimePages (Pages);
  ASSERT (Buffer != NULL);
  Status = MmUnblockMemoryRequest (
             (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer,
             Pages
             );
  if ((Status != EFI_UNSUPPORTED) && EFI_ERROR (Status)) {
    return Status;
  }

  TempHobBuffer.CacheInfoFlagBuffer = (UINTN)Buffer;
  DEBUG ((
    DEBUG_INFO,
    "PeiVariable: CACHE_INFO_FLAG Buffer is: 0x%lx, number of pages is: 0x%x\n",
    TempHobBuffer.CacheInfoFlagBuffer,
    Pages
    ));

  //
  // AllocateRuntimePages for VolatileCache and unblock it.
  //
  BufferSize = PcdGet32 (PcdVariableStoreSize);
  if (BufferSize > 0) {
    Pages  = EFI_SIZE_TO_PAGES (BufferSize);
    Buffer = AllocateRuntimePages (Pages);
    ASSERT (Buffer != NULL);
    Status = MmUnblockMemoryRequest (
               (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer,
               Pages
               );
    if ((Status != EFI_UNSUPPORTED) && EFI_ERROR (Status)) {
      return Status;
    }

    TempHobBuffer.RuntimeVolatileCacheBuffer = (UINTN)Buffer;
    TempHobBuffer.RuntimeVolatileCachePages  = Pages;
  }

  DEBUG ((
    DEBUG_INFO,
    "PeiVariable: Volatile cache Buffer is: 0x%lx, number of pages is: 0x%lx\n",
    TempHobBuffer.RuntimeVolatileCacheBuffer,
    TempHobBuffer.RuntimeVolatileCachePages
    ));

  //
  // AllocateRuntimePages for NVCache and unblock it.
  //
  BufferSize = CalculateNvVariableCacheSize (&NvAuthFlag);
  if (BufferSize > 0) {
    Pages  = EFI_SIZE_TO_PAGES (BufferSize);
    Buffer = AllocateRuntimePages (Pages);
    ASSERT (Buffer != NULL);
    Status = MmUnblockMemoryRequest (
               (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer,
               Pages
               );
    if ((Status != EFI_UNSUPPORTED) && EFI_ERROR (Status)) {
      return Status;
    }

    TempHobBuffer.RuntimeNvCacheBuffer = (UINTN)Buffer;
    TempHobBuffer.RuntimeNvCachePages  = Pages;
  }

  DEBUG ((
    DEBUG_INFO,
    "PeiVariable: NV cache Buffer is: 0x%lx, number of pages is: 0x%lx\n",
    TempHobBuffer.RuntimeNvCacheBuffer,
    TempHobBuffer.RuntimeNvCachePages
    ));

  //
  // AllocateRuntimePages for HobCache and unblock it.
  //
  BufferSize = CalculateHobVariableCacheSize (NvAuthFlag);
  if (BufferSize > 0) {
    Pages  = EFI_SIZE_TO_PAGES (BufferSize);
    Buffer = AllocateRuntimePages (Pages);
    ASSERT (Buffer != NULL);
    Status = MmUnblockMemoryRequest (
               (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer,
               Pages
               );
    if ((Status != EFI_UNSUPPORTED) && EFI_ERROR (Status)) {
      return Status;
    }

    TempHobBuffer.RuntimeHobCacheBuffer = (UINTN)Buffer;
    TempHobBuffer.RuntimeHobCachePages  = Pages;
  }

  DEBUG ((
    DEBUG_INFO,
    "PeiVariable: HOB cache Buffer is: 0x%lx, number of pages is: 0x%lx\n",
    TempHobBuffer.RuntimeHobCacheBuffer,
    TempHobBuffer.RuntimeHobCachePages
    ));

  VariableRuntimeCacheInfo = BuildGuidHob (&gEdkiiVariableRuntimeCacheInfoHobGuid, sizeof (VARIABLE_RUNTIME_CACHE_INFO));
  ASSERT (VariableRuntimeCacheInfo != NULL);
  CopyMem (VariableRuntimeCacheInfo, &TempHobBuffer, sizeof (VARIABLE_RUNTIME_CACHE_INFO));

  return EFI_SUCCESS;
}
