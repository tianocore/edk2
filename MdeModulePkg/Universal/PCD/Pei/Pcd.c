/** @file
  All Pcd Ppi services are implemented here.

Copyright (c) 2006 - 2021, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Service.h"

///
/// Instance of PCD_PPI protocol is EDKII native implementation.
/// This protocol instance support dynamic and dynamicEx type PCDs.
///
PCD_PPI  mPcdPpiInstance = {
  PeiPcdSetSku,

  PeiPcdGet8,
  PeiPcdGet16,
  PeiPcdGet32,
  PeiPcdGet64,
  PeiPcdGetPtr,
  PeiPcdGetBool,
  PeiPcdGetSize,

  PeiPcdGet8Ex,
  PeiPcdGet16Ex,
  PeiPcdGet32Ex,
  PeiPcdGet64Ex,
  PeiPcdGetPtrEx,
  PeiPcdGetBoolEx,
  PeiPcdGetSizeEx,

  PeiPcdSet8,
  PeiPcdSet16,
  PeiPcdSet32,
  PeiPcdSet64,
  PeiPcdSetPtr,
  PeiPcdSetBool,

  PeiPcdSet8Ex,
  PeiPcdSet16Ex,
  PeiPcdSet32Ex,
  PeiPcdSet64Ex,
  PeiPcdSetPtrEx,
  PeiPcdSetBoolEx,

  PeiRegisterCallBackOnSet,
  PcdUnRegisterCallBackOnSet,
  PeiPcdGetNextToken,
  PeiPcdGetNextTokenSpace
};

///
/// Instance of EFI_PEI_PCD_PPI which is defined in PI 1.2 Vol 3.
/// This PPI instance only support dyanmicEx type PCD.
///
EFI_PEI_PCD_PPI  mEfiPcdPpiInstance = {
  PeiPcdSetSku,

  PeiPcdGet8Ex,
  PeiPcdGet16Ex,
  PeiPcdGet32Ex,
  PeiPcdGet64Ex,
  PeiPcdGetPtrEx,
  PeiPcdGetBoolEx,
  PeiPcdGetSizeEx,
  PeiPcdSet8Ex,
  PeiPcdSet16Ex,
  PeiPcdSet32Ex,
  PeiPcdSet64Ex,
  PeiPcdSetPtrEx,
  PeiPcdSetBoolEx,
  (EFI_PEI_PCD_PPI_CALLBACK_ON_SET)PeiRegisterCallBackOnSet,
  (EFI_PEI_PCD_PPI_CANCEL_CALLBACK)PcdUnRegisterCallBackOnSet,
  PeiPcdGetNextToken,
  PeiPcdGetNextTokenSpace
};

///
/// Instance of GET_PCD_INFO_PPI protocol is EDKII native implementation.
/// This protocol instance support dynamic and dynamicEx type PCDs.
///
GET_PCD_INFO_PPI  mGetPcdInfoInstance = {
  PeiGetPcdInfoGetInfo,
  PeiGetPcdInfoGetInfoEx,
  PeiGetPcdInfoGetSku
};

///
/// Instance of EFI_GET_PCD_INFO_PPI which is defined in PI 1.2.1 Vol 3.
/// This PPI instance only support dyanmicEx type PCD.
///
EFI_GET_PCD_INFO_PPI  mEfiGetPcdInfoInstance = {
  PeiGetPcdInfoGetInfoEx,
  PeiGetPcdInfoGetSku
};

EFI_PEI_PPI_DESCRIPTOR  mPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gPcdPpiGuid,
    &mPcdPpiInstance
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiPcdPpiGuid,
    &mEfiPcdPpiInstance
  }
};

EFI_PEI_PPI_DESCRIPTOR  mPpiList2[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gGetPcdInfoPpiGuid,
    &mGetPcdInfoInstance
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiGetPcdInfoPpiGuid,
    &mEfiGetPcdInfoInstance
  }
};

/**
  Callback on SET PcdSetNvStoreDefaultId

  Once PcdSetNvStoreDefaultId is set, the default NV storage will be found from
  PcdNvStoreDefaultValueBuffer, and built into VariableHob.

  @param[in]      CallBackGuid  The PCD token GUID being set.
  @param[in]      CallBackToken The PCD token number being set.
  @param[in, out] TokenData     A pointer to the token data being set.
  @param[in]      TokenDataSize The size, in bytes, of the data being set.

**/
VOID
EFIAPI
PcdSetNvStoreDefaultIdCallBack (
  IN CONST EFI_GUID  *CallBackGuid  OPTIONAL,
  IN       UINTN     CallBackToken,
  IN OUT   VOID      *TokenData,
  IN       UINTN     TokenDataSize
  )
{
  EFI_STATUS             Status;
  UINT16                 DefaultId;
  SKU_ID                 SkuId;
  UINTN                  FullSize;
  UINTN                  Index;
  UINT8                  *DataBuffer;
  UINT8                  *VarStoreHobData;
  UINT8                  *BufferEnd;
  BOOLEAN                IsFound;
  VARIABLE_STORE_HEADER  *NvStoreBuffer;
  PCD_DEFAULT_DATA       *DataHeader;
  PCD_DEFAULT_INFO       *DefaultInfo;
  PCD_DATA_DELTA         *DeltaData;

  DefaultId = *(UINT16 *)TokenData;
  SkuId     = GetPcdDatabase ()->SystemSkuId;
  IsFound   = FALSE;

  if (PeiPcdGetSizeEx (&gEfiMdeModulePkgTokenSpaceGuid, PcdToken (PcdNvStoreDefaultValueBuffer)) > sizeof (PCD_NV_STORE_DEFAULT_BUFFER_HEADER)) {
    DataBuffer = (UINT8 *)PeiPcdGetPtrEx (&gEfiMdeModulePkgTokenSpaceGuid, PcdToken (PcdNvStoreDefaultValueBuffer));
    FullSize   = ((PCD_NV_STORE_DEFAULT_BUFFER_HEADER *)DataBuffer)->Length;
    DataHeader = (PCD_DEFAULT_DATA *)(DataBuffer + sizeof (PCD_NV_STORE_DEFAULT_BUFFER_HEADER));
    //
    // The first section data includes NV storage default setting.
    //
    NvStoreBuffer   = (VARIABLE_STORE_HEADER *)((UINT8 *)DataHeader + sizeof (DataHeader->DataSize) + DataHeader->HeaderSize);
    VarStoreHobData = (UINT8 *)BuildGuidHob (&NvStoreBuffer->Signature, NvStoreBuffer->Size);
    ASSERT (VarStoreHobData != NULL);
    CopyMem (VarStoreHobData, NvStoreBuffer, NvStoreBuffer->Size);
    //
    // Find the matched SkuId and DefaultId in the first section
    //
    DefaultInfo = &(DataHeader->DefaultInfo[0]);
    BufferEnd   = (UINT8 *)DataHeader + sizeof (DataHeader->DataSize) + DataHeader->HeaderSize;
    while ((UINT8 *)DefaultInfo < BufferEnd) {
      if ((DefaultInfo->DefaultId == DefaultId) && (DefaultInfo->SkuId == SkuId)) {
        IsFound = TRUE;
        break;
      }

      DefaultInfo++;
    }

    //
    // Find the matched SkuId and DefaultId in the remaining section
    //
    Index      = sizeof (PCD_NV_STORE_DEFAULT_BUFFER_HEADER) + ((DataHeader->DataSize + 7) & (~7));
    DataHeader = (PCD_DEFAULT_DATA *)(DataBuffer + Index);
    while (!IsFound && Index < FullSize && DataHeader->DataSize != 0xFFFFFFFF) {
      DefaultInfo = &(DataHeader->DefaultInfo[0]);
      BufferEnd   = (UINT8 *)DataHeader + sizeof (DataHeader->DataSize) + DataHeader->HeaderSize;
      while ((UINT8 *)DefaultInfo < BufferEnd) {
        if ((DefaultInfo->DefaultId == DefaultId) && (DefaultInfo->SkuId == SkuId)) {
          IsFound = TRUE;
          break;
        }

        DefaultInfo++;
      }

      if (IsFound) {
        DeltaData = (PCD_DATA_DELTA *)BufferEnd;
        BufferEnd = (UINT8 *)DataHeader + DataHeader->DataSize;
        while ((UINT8 *)DeltaData < BufferEnd) {
          *(VarStoreHobData + DeltaData->Offset) = (UINT8)DeltaData->Value;
          DeltaData++;
        }

        break;
      }

      Index      = (Index + DataHeader->DataSize + 7) & (~7);
      DataHeader = (PCD_DEFAULT_DATA *)(DataBuffer + Index);
    }
  }

  Status = PcdUnRegisterCallBackOnSet (
             &gEfiMdeModulePkgTokenSpaceGuid,
             PcdToken (PcdSetNvStoreDefaultId),
             PcdSetNvStoreDefaultIdCallBack
             );
  ASSERT_EFI_ERROR (Status);
}

/**
  Report Pei PCD database of all SKUs as Guid HOB so that DxePcd can access it.

  @param PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param NotifyDescriptor  Address of the notification descriptor data structure.
  @param Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS      Successfully update the Boot records.
**/
EFI_STATUS
EFIAPI
EndOfPeiSignalPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  PEI_PCD_DATABASE     *Database;
  EFI_BOOT_MODE        BootMode;
  EFI_STATUS           Status;
  UINTN                Instance;
  EFI_PEI_FV_HANDLE    VolumeHandle;
  EFI_PEI_FILE_HANDLE  FileHandle;
  VOID                 *PcdDb;
  UINT32               Length;
  PEI_PCD_DATABASE     *PeiPcdDb;

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // Don't need to report it on S3 boot.
  //
  if (BootMode == BOOT_ON_S3_RESUME) {
    return EFI_SUCCESS;
  }

  PeiPcdDb = GetPcdDatabase ();
  if (PeiPcdDb->SystemSkuId != (SKU_ID)0) {
    //
    // SkuId has been set. Don't need to report it to DXE phase.
    //
    return EFI_SUCCESS;
  }

  //
  // Get full PCD database from PcdPeim FileHandle
  //
  Instance   = 0;
  FileHandle = NULL;
  while (TRUE) {
    //
    // Traverse all firmware volume instances
    //
    Status = PeiServicesFfsFindNextVolume (Instance, &VolumeHandle);
    //
    // Error should not happen
    //
    ASSERT_EFI_ERROR (Status);

    //
    // Find PcdDb file from the beginning in this firmware volume.
    //
    FileHandle = NULL;
    Status     = PeiServicesFfsFindFileByName (&gEfiCallerIdGuid, VolumeHandle, &FileHandle);
    if (!EFI_ERROR (Status)) {
      //
      // Find PcdPeim FileHandle in this volume
      //
      break;
    }

    //
    // We cannot find PcdPeim in this firmware volume, then search the next volume.
    //
    Instance++;
  }

  //
  // Find PEI PcdDb and Build second PcdDB GuidHob
  //
  Status = PeiServicesFfsFindSectionData (EFI_SECTION_RAW, FileHandle, &PcdDb);
  ASSERT_EFI_ERROR (Status);
  Length   = PeiPcdDb->LengthForAllSkus;
  Database = BuildGuidHob (&gPcdDataBaseHobGuid, Length);
  CopyMem (Database, PcdDb, Length);

  return EFI_SUCCESS;
}

EFI_PEI_NOTIFY_DESCRIPTOR  mEndOfPeiSignalPpiNotifyList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiEndOfPeiSignalPpiGuid,
    EndOfPeiSignalPpiNotifyCallback
  }
};

/**
  Main entry for PCD PEIM driver.

  This routine initialize the PCD database for PEI phase and install PCD_PPI/EFI_PEI_PCD_PPI.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @return Status of install PCD_PPI

**/
EFI_STATUS
EFIAPI
PcdPeimInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = PeiServicesRegisterForShadow (FileHandle);
  if (Status == EFI_ALREADY_STARTED) {
    //
    // This is now starting in memory, the second time starting.
    //
    EFI_PEI_PPI_DESCRIPTOR  *OldPpiList;
    EFI_PEI_PPI_DESCRIPTOR  *OldPpiList2;
    VOID                    *Ppi;
    VOID                    *Ppi2;

    OldPpiList = NULL;
    Status     = PeiServicesLocatePpi (
                   &gPcdPpiGuid,
                   0,
                   &OldPpiList,
                   &Ppi
                   );
    ASSERT_EFI_ERROR (Status);

    if (OldPpiList != NULL) {
      Status = PeiServicesReInstallPpi (OldPpiList, &mPpiList[0]);
      ASSERT_EFI_ERROR (Status);
    }

    OldPpiList2 = NULL;
    Status      = PeiServicesLocatePpi (
                    &gGetPcdInfoPpiGuid,
                    0,
                    &OldPpiList2,
                    &Ppi2
                    );
    ASSERT_EFI_ERROR (Status);

    if (OldPpiList2 != NULL) {
      Status = PeiServicesReInstallPpi (OldPpiList2, &mPpiList2[0]);
      ASSERT_EFI_ERROR (Status);
    }

    OldPpiList = NULL;
    Status     = PeiServicesLocatePpi (
                   &gEfiPeiPcdPpiGuid,
                   0,
                   &OldPpiList,
                   &Ppi
                   );
    ASSERT_EFI_ERROR (Status);

    if (OldPpiList != NULL) {
      Status = PeiServicesReInstallPpi (OldPpiList, &mPpiList[1]);
      ASSERT_EFI_ERROR (Status);
    }

    OldPpiList2 = NULL;
    Status      = PeiServicesLocatePpi (
                    &gEfiGetPcdInfoPpiGuid,
                    0,
                    &OldPpiList2,
                    &Ppi2
                    );
    ASSERT_EFI_ERROR (Status);

    if (OldPpiList2 != NULL) {
      Status = PeiServicesReInstallPpi (OldPpiList2, &mPpiList2[1]);
      ASSERT_EFI_ERROR (Status);
    }

    return Status;
  }

  BuildPcdDatabase (FileHandle);

  //
  // Install PCD_PPI and EFI_PEI_PCD_PPI.
  //
  Status = PeiServicesInstallPpi (&mPpiList[0]);
  ASSERT_EFI_ERROR (Status);

  //
  // Install GET_PCD_INFO_PPI and EFI_GET_PCD_INFO_PPI.
  //
  Status = PeiServicesInstallPpi (&mPpiList2[0]);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesNotifyPpi (&mEndOfPeiSignalPpiNotifyList[0]);
  ASSERT_EFI_ERROR (Status);

  Status = PeiRegisterCallBackOnSet (
             &gEfiMdeModulePkgTokenSpaceGuid,
             PcdToken (PcdSetNvStoreDefaultId),
             PcdSetNvStoreDefaultIdCallBack
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Retrieve additional information associated with a PCD token in the default token space.

  This includes information such as the type of value the TokenNumber is associated with as well as possible
  human readable name that is associated with the token.

  @param[in]    TokenNumber The PCD token number.
  @param[out]   PcdInfo     The returned information associated with the requested TokenNumber.
                            The caller is responsible for freeing the buffer that is allocated by callee for PcdInfo->PcdName.

  @retval  EFI_SUCCESS      The PCD information was returned successfully.
  @retval  EFI_NOT_FOUND    The PCD service could not find the requested token number.
**/
EFI_STATUS
EFIAPI
PeiGetPcdInfoGetInfo (
  IN        UINTN         TokenNumber,
  OUT       EFI_PCD_INFO  *PcdInfo
  )
{
  return PeiGetPcdInfo (NULL, TokenNumber, PcdInfo);
}

/**
  Retrieve additional information associated with a PCD token.

  This includes information such as the type of value the TokenNumber is associated with as well as possible
  human readable name that is associated with the token.

  @param[in]    Guid        The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]    TokenNumber The PCD token number.
  @param[out]   PcdInfo     The returned information associated with the requested TokenNumber.
                            The caller is responsible for freeing the buffer that is allocated by callee for PcdInfo->PcdName.

  @retval  EFI_SUCCESS      The PCD information was returned successfully.
  @retval  EFI_NOT_FOUND    The PCD service could not find the requested token number.
**/
EFI_STATUS
EFIAPI
PeiGetPcdInfoGetInfoEx (
  IN CONST  EFI_GUID      *Guid,
  IN        UINTN         TokenNumber,
  OUT       EFI_PCD_INFO  *PcdInfo
  )
{
  return PeiGetPcdInfo (Guid, TokenNumber, PcdInfo);
}

/**
  Retrieve the currently set SKU Id.

  @return   The currently set SKU Id. If the platform has not set at a SKU Id, then the
            default SKU Id value of 0 is returned. If the platform has set a SKU Id, then the currently set SKU
            Id is returned.
**/
UINTN
EFIAPI
PeiGetPcdInfoGetSku (
  VOID
  )
{
  return (UINTN)GetPcdDatabase ()->SystemSkuId;
}

/**
  Sets the SKU value for subsequent calls to set or get PCD token values.

  SetSku() sets the SKU Id to be used for subsequent calls to set or get PCD values.
  SetSku() is normally called only once by the system.

  For each item (token), the database can hold a single value that applies to all SKUs,
  or multiple values, where each value is associated with a specific SKU Id. Items with multiple,
  SKU-specific values are called SKU enabled.

  The SKU Id of zero is reserved as a default.
  For tokens that are not SKU enabled, the system ignores any set SKU Id and works with the
  single value for that token. For SKU-enabled tokens, the system will use the SKU Id set by the
  last call to SetSku(). If no SKU Id is set or the currently set SKU Id isn't valid for the specified token,
  the system uses the default SKU Id. If the system attempts to use the default SKU Id and no value has been
  set for that Id, the results are unpredictable.

  @param[in]  SkuId The SKU value that will be used when the PCD service will retrieve and
              set values associated with a PCD token.

**/
VOID
EFIAPI
PeiPcdSetSku (
  IN  UINTN  SkuId
  )
{
  PEI_PCD_DATABASE        *PeiPcdDb;
  SKU_ID                  *SkuIdTable;
  UINTN                   Index;
  EFI_STATUS              Status;
  UINTN                   Instance;
  EFI_PEI_FV_HANDLE       VolumeHandle;
  EFI_PEI_FILE_HANDLE     FileHandle;
  VOID                    *PcdDb;
  UINT32                  Length;
  PCD_DATABASE_SKU_DELTA  *SkuDelta;
  PCD_DATA_DELTA          *SkuDeltaData;

  DEBUG ((DEBUG_INFO, "PcdPei - SkuId 0x%lx is to be set.\n", (SKU_ID)SkuId));

  PeiPcdDb = GetPcdDatabase ();

  if (SkuId == PeiPcdDb->SystemSkuId) {
    //
    // The input SKU Id is equal to current SKU Id, return directly.
    //
    DEBUG ((DEBUG_INFO, "PcdPei - SkuId is same to current system Sku.\n"));
    return;
  }

  if (PeiPcdDb->SystemSkuId != (SKU_ID)0) {
    DEBUG ((DEBUG_ERROR, "PcdPei - The SKU Id could be changed only once."));
    DEBUG ((
      DEBUG_ERROR,
      "PcdPei - The SKU Id was set to 0x%lx already, it could not be set to 0x%lx any more.",
      PeiPcdDb->SystemSkuId,
      (SKU_ID)SkuId
      ));
    ASSERT (FALSE);
    return;
  }

  SkuIdTable = (SKU_ID *)((UINT8 *)PeiPcdDb + PeiPcdDb->SkuIdTableOffset);
  for (Index = 0; Index < SkuIdTable[0]; Index++) {
    if (SkuId == SkuIdTable[Index + 1]) {
      DEBUG ((DEBUG_INFO, "PcdPei - SkuId is found in SkuId table.\n"));
      break;
    }
  }

  if (Index < SkuIdTable[0]) {
    //
    // Get full PCD database from PcdPeim FileHandle
    //
    Instance   = 0;
    FileHandle = NULL;
    while (TRUE) {
      //
      // Traverse all firmware volume instances
      //
      Status = PeiServicesFfsFindNextVolume (Instance, &VolumeHandle);
      //
      // Error should not happen
      //
      ASSERT_EFI_ERROR (Status);

      //
      // Find PcdDb file from the beginning in this firmware volume.
      //
      FileHandle = NULL;
      Status     = PeiServicesFfsFindFileByName (&gEfiCallerIdGuid, VolumeHandle, &FileHandle);
      if (!EFI_ERROR (Status)) {
        //
        // Find PcdPeim FileHandle in this volume
        //
        break;
      }

      //
      // We cannot find PcdPeim in this firmware volume, then search the next volume.
      //
      Instance++;
    }

    //
    // Find the delta data between the different Skus
    //
    Status = PeiServicesFfsFindSectionData (EFI_SECTION_RAW, FileHandle, &PcdDb);
    ASSERT_EFI_ERROR (Status);
    Length   = PeiPcdDb->LengthForAllSkus;
    Index    = (PeiPcdDb->Length + 7) & (~7);
    SkuDelta = NULL;
    while (Index < Length) {
      SkuDelta = (PCD_DATABASE_SKU_DELTA *)((UINT8 *)PcdDb + Index);
      if ((SkuDelta->SkuId == SkuId) && (SkuDelta->SkuIdCompared == 0)) {
        break;
      }

      Index = (Index + SkuDelta->Length + 7) & (~7);
    }

    //
    // Patch the delta data into current PCD database
    //
    if ((Index < Length) && (SkuDelta != NULL)) {
      SkuDeltaData = (PCD_DATA_DELTA *)(SkuDelta + 1);
      while ((UINT8 *)SkuDeltaData < (UINT8 *)SkuDelta + SkuDelta->Length) {
        *((UINT8 *)PeiPcdDb + SkuDeltaData->Offset) = (UINT8)SkuDeltaData->Value;
        SkuDeltaData++;
      }

      PeiPcdDb->SystemSkuId = (SKU_ID)SkuId;
      DEBUG ((DEBUG_INFO, "PcdPei - Set current SKU Id to 0x%lx.\n", (SKU_ID)SkuId));
      return;
    }
  }

  //
  // Invalid input SkuId, the default SKU Id will be still used for the system.
  //
  DEBUG ((DEBUG_ERROR, "PcdPei - Invalid input SkuId, the default SKU Id will be still used.\n"));

  return;
}

/**
  Retrieves an 8-bit value for a given PCD token.

  Retrieves the current byte-sized value for a PCD token number.
  If the TokenNumber is invalid, the results are unpredictable.

  @param[in]  TokenNumber The PCD token number.

  @return The UINT8 value.

**/
UINT8
EFIAPI
PeiPcdGet8 (
  IN UINTN  TokenNumber
  )
{
  return *((UINT8 *)GetWorker (TokenNumber, sizeof (UINT8)));
}

/**
  Retrieves an 16-bit value for a given PCD token.

  Retrieves the current 16-bits value for a PCD token number.
  If the TokenNumber is invalid, the results are unpredictable.

  @param[in]  TokenNumber The PCD token number.

  @return The UINT16 value.

**/
UINT16
EFIAPI
PeiPcdGet16 (
  IN UINTN  TokenNumber
  )
{
  return ReadUnaligned16 (GetWorker (TokenNumber, sizeof (UINT16)));
}

/**
  Retrieves an 32-bit value for a given PCD token.

  Retrieves the current 32-bits value for a PCD token number.
  If the TokenNumber is invalid, the results are unpredictable.

  @param[in]  TokenNumber The PCD token number.

  @return The UINT32 value.

**/
UINT32
EFIAPI
PeiPcdGet32 (
  IN UINTN  TokenNumber
  )
{
  return ReadUnaligned32 (GetWorker (TokenNumber, sizeof (UINT32)));
}

/**
  Retrieves an 64-bit value for a given PCD token.

  Retrieves the current 64-bits value for a PCD token number.
  If the TokenNumber is invalid, the results are unpredictable.

  @param[in]  TokenNumber The PCD token number.

  @return The UINT64 value.

**/
UINT64
EFIAPI
PeiPcdGet64 (
  IN UINTN  TokenNumber
  )
{
  return ReadUnaligned64 (GetWorker (TokenNumber, sizeof (UINT64)));
}

/**
  Retrieves a pointer to a value for a given PCD token.

  Retrieves the current pointer to the buffer for a PCD token number.
  Do not make any assumptions about the alignment of the pointer that
  is returned by this function call.  If the TokenNumber is invalid,
  the results are unpredictable.

  @param[in]  TokenNumber The PCD token number.

  @return The pointer to the buffer to be retrieved.

**/
VOID *
EFIAPI
PeiPcdGetPtr (
  IN UINTN  TokenNumber
  )
{
  return GetWorker (TokenNumber, 0);
}

/**
  Retrieves a Boolean value for a given PCD token.

  Retrieves the current boolean value for a PCD token number.
  Do not make any assumptions about the alignment of the pointer that
  is returned by this function call.  If the TokenNumber is invalid,
  the results are unpredictable.

  @param[in]  TokenNumber The PCD token number.

  @return The Boolean value.

**/
BOOLEAN
EFIAPI
PeiPcdGetBool (
  IN UINTN  TokenNumber
  )
{
  return *((BOOLEAN *)GetWorker (TokenNumber, sizeof (BOOLEAN)));
}

/**
  Retrieves the size of the value for a given PCD token.

  Retrieves the current size of a particular PCD token.
  If the TokenNumber is invalid, the results are unpredictable.

  @param[in]  TokenNumber The PCD token number.

  @return The size of the value for the PCD token.

**/
UINTN
EFIAPI
PeiPcdGetSize (
  IN UINTN  TokenNumber
  )
{
  PEI_PCD_DATABASE  *PeiPcdDb;
  UINTN             Size;
  UINTN             MaxSize;
  UINT32            LocalTokenCount;

  PeiPcdDb        = GetPcdDatabase ();
  LocalTokenCount = PeiPcdDb->LocalTokenCount;
  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the
  // comparison.
  ASSERT (TokenNumber + 1 < (LocalTokenCount + 1));

  Size = (*((UINT32 *)((UINT8 *)PeiPcdDb + PeiPcdDb->LocalTokenNumberTableOffset) + TokenNumber) & PCD_DATUM_TYPE_ALL_SET) >> PCD_DATUM_TYPE_SHIFT;

  if (Size == 0) {
    //
    // For pointer type, we need to scan the SIZE_TABLE to get the current size.
    //
    return GetPtrTypeSize (TokenNumber, &MaxSize, PeiPcdDb);
  } else {
    return Size;
  }
}

/**
  Retrieves an 8-bit value for a given PCD token.

  Retrieves the 8-bit value of a particular PCD token.
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are
  unpredictable.

  @param[in]  Guid              The token space for the token number.
  @param[in]  ExTokenNumber     The PCD token number.

  @return The size 8-bit value for the PCD token.

**/
UINT8
EFIAPI
PeiPcdGet8Ex (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           ExTokenNumber
  )
{
  return *((UINT8 *)ExGetWorker (Guid, ExTokenNumber, sizeof (UINT8)));
}

/**
  Retrieves an 16-bit value for a given PCD token.

  Retrieves the 16-bit value of a particular PCD token.
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are
  unpredictable.

  @param[in]  Guid          The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number.

  @return The size 16-bit value for the PCD token.

**/
UINT16
EFIAPI
PeiPcdGet16Ex (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           ExTokenNumber
  )
{
  return ReadUnaligned16 (ExGetWorker (Guid, ExTokenNumber, sizeof (UINT16)));
}

/**
  Retrieves an 32-bit value for a given PCD token.

  Retrieves the 32-bit value of a particular PCD token.
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are
  unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number.

  @return The size 32-bit value for the PCD token.

**/
UINT32
EFIAPI
PeiPcdGet32Ex (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           ExTokenNumber
  )
{
  return ReadUnaligned32 (ExGetWorker (Guid, ExTokenNumber, sizeof (UINT32)));
}

/**
  Retrieves an 64-bit value for a given PCD token.

  Retrieves the 64-bit value of a particular PCD token.
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are
  unpredictable.

  @param[in]  Guid The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number.

  @return The size 64-bit value for the PCD token.

**/
UINT64
EFIAPI
PeiPcdGet64Ex (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           ExTokenNumber
  )
{
  return ReadUnaligned64 (ExGetWorker (Guid, ExTokenNumber, sizeof (UINT64)));
}

/**
  Retrieves a pointer to a value for a given PCD token.

  Retrieves the current pointer to the buffer for a PCD token number.
  Do not make any assumptions about the alignment of the pointer that
  is returned by this function call.  If the TokenNumber is invalid,
  the results are unpredictable.

  @param[in]  Guid          The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number.

  @return The pointer to the buffer to be retrieved.

**/
VOID *
EFIAPI
PeiPcdGetPtrEx (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           ExTokenNumber
  )
{
  return ExGetWorker (Guid, ExTokenNumber, 0);
}

/**
  Retrieves an Boolean value for a given PCD token.

  Retrieves the Boolean value of a particular PCD token.
  If the TokenNumber is invalid or the token space
  specified by Guid does not exist, the results are
  unpredictable.

  @param[in]  Guid          The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number.

  @return The size Boolean value for the PCD token.

**/
BOOLEAN
EFIAPI
PeiPcdGetBoolEx (
  IN CONST  EFI_GUID  *Guid,
  IN UINTN            ExTokenNumber
  )
{
  return *((BOOLEAN *)ExGetWorker (Guid, ExTokenNumber, sizeof (BOOLEAN)));
}

/**
  Retrieves the size of the value for a given PCD token.

  Retrieves the current size of a particular PCD token.
  If the TokenNumber is invalid, the results are unpredictable.

  @param[in]  Guid          The token space for the token number.
  @param[in]  ExTokenNumber The PCD token number.

  @return The size of the value for the PCD token.

**/
UINTN
EFIAPI
PeiPcdGetSizeEx (
  IN CONST  EFI_GUID  *Guid,
  IN UINTN            ExTokenNumber
  )
{
  return PeiPcdGetSize (GetExPcdTokenNumber (Guid, ExTokenNumber));
}

/**
  Sets an 8-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the
  size of the value being set is compatible with the Token's existing definition.
  If it is not, an error will be returned.

  @param[in]  TokenNumber The PCD token number.
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data
                                  being set was incompatible with a call to this function.
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdSet8 (
  IN UINTN  TokenNumber,
  IN UINT8  Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}

/**
  Sets an 16-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the
  size of the value being set is compatible with the Token's existing definition.
  If it is not, an error will be returned.

  @param[in]  TokenNumber The PCD token number.
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data
                                  being set was incompatible with a call to this function.
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdSet16 (
  IN UINTN   TokenNumber,
  IN UINT16  Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}

/**
  Sets an 32-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the
  size of the value being set is compatible with the Token's existing definition.
  If it is not, an error will be returned.

  @param[in]  TokenNumber The PCD token number.
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data
                                  being set was incompatible with a call to this function.
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdSet32 (
  IN UINTN   TokenNumber,
  IN UINT32  Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}

/**
  Sets an 64-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the
  size of the value being set is compatible with the Token's existing definition.
  If it is not, an error will be returned.

  @param[in]  TokenNumber The PCD token number.
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data
                                  being set was incompatible with a call to this function.
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdSet64 (
  IN UINTN   TokenNumber,
  IN UINT64  Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}

/**
  Sets a value of a specified size for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the
  size of the value being set is compatible with the Token's existing definition.
  If it is not, an error will be returned.

  @param[in]  TokenNumber The PCD token number.
  @param[in, out] SizeOfBuffer A pointer to the length of the value being set for the PCD token.
                              On input, if the SizeOfValue is greater than the maximum size supported
                              for this TokenNumber then the output value of SizeOfValue will reflect
                              the maximum size supported for this TokenNumber.
  @param[in]  Buffer The buffer to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data
                                  being set was incompatible with a call to this function.
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdSetPtr (
  IN      UINTN  TokenNumber,
  IN OUT  UINTN  *SizeOfBuffer,
  IN      VOID   *Buffer
  )
{
  return SetWorker (TokenNumber, Buffer, SizeOfBuffer, TRUE);
}

/**
  Sets an Boolean value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the
  size of the value being set is compatible with the Token's existing definition.
  If it is not, an error will be returned.

  @param[in]  TokenNumber The PCD token number.
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data
                                  being set was incompatible with a call to this function.
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdSetBool (
  IN UINTN    TokenNumber,
  IN BOOLEAN  Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}

/**
  Sets an 8-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the
  size of the value being set is compatible with the Token's existing definition.
  If it is not, an error will be returned.

  @param[in]  Guid          The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  ExTokenNumber The PCD token number.
  @param[in]  Value         The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data
                                  being set was incompatible with a call to this function.
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdSet8Ex (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           ExTokenNumber,
  IN UINT8           Value
  )
{
  return ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}

/**
  Sets an 16-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the
  size of the value being set is compatible with the Token's existing definition.
  If it is not, an error will be returned.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  ExTokenNumber The PCD token number.
  @param[in]  Value The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data
                                  being set was incompatible with a call to this function.
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdSet16Ex (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           ExTokenNumber,
  IN UINT16          Value
  )
{
  return ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}

/**
  Sets an 32-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the
  size of the value being set is compatible with the Token's existing definition.
  If it is not, an error will be returned.

  @param[in]  Guid          The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  ExTokenNumber The PCD token number.
  @param[in]  Value         The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data
                                  being set was incompatible with a call to this function.
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdSet32Ex (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           ExTokenNumber,
  IN UINT32          Value
  )
{
  return ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}

/**
  Sets an 64-bit value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the
  size of the value being set is compatible with the Token's existing definition.
  If it is not, an error will be returned.

  @param[in]  Guid          The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  ExTokenNumber The PCD token number.
  @param[in]  Value         The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data
                                  being set was incompatible with a call to this function.
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdSet64Ex (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           ExTokenNumber,
  IN UINT64          Value
  )
{
  return ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}

/**
  Sets a value of a specified size for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the
  size of the value being set is compatible with the Token's existing definition.
  If it is not, an error will be returned.

  @param[in]        Guid            The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]        ExTokenNumber   The PCD token number.
  @param[in, out]   SizeOfBuffer    A pointer to the length of the value being set for the PCD token.
                                    On input, if the SizeOfValue is greater than the maximum size supported
                                    for this TokenNumber then the output value of SizeOfValue will reflect
                                    the maximum size supported for this TokenNumber.
  @param[in]        Value           The buffer to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data
                                  being set was incompatible with a call to this function.
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdSetPtrEx (
  IN     CONST EFI_GUID  *Guid,
  IN     UINTN           ExTokenNumber,
  IN OUT UINTN           *SizeOfBuffer,
  IN     VOID            *Value
  )
{
  return ExSetWorker (ExTokenNumber, Guid, Value, SizeOfBuffer, TRUE);
}

/**
  Sets an Boolean value for a given PCD token.

  When the PCD service sets a value, it will check to ensure that the
  size of the value being set is compatible with the Token's existing definition.
  If it is not, an error will be returned.

  @param [in]  Guid          The 128-bit unique value that designates the namespace from which to extract the value.
  @param [in]  ExTokenNumber The PCD token number.
  @param [in]  Value         The value to set for the PCD token.

  @retval EFI_SUCCESS  Procedure returned successfully.
  @retval EFI_INVALID_PARAMETER The PCD service determined that the size of the data
                                  being set was incompatible with a call to this function.
                                  Use GetSize() to retrieve the size of the target data.
  @retval EFI_NOT_FOUND The PCD service could not find the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdSetBoolEx (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           ExTokenNumber,
  IN BOOLEAN         Value
  )
{
  return ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}

/**
  Specifies a function to be called anytime the value of a designated token is changed.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  ExTokenNumber The PCD token number.
  @param[in]  CallBackFunction The function prototype called when the value associated with the CallBackToken is set.

  @retval EFI_SUCCESS  The PCD service has successfully established a call event
                        for the CallBackToken requested.
  @retval EFI_NOT_FOUND The PCD service could not find the referenced token number.

**/
EFI_STATUS
EFIAPI
PeiRegisterCallBackOnSet (
  IN  CONST EFI_GUID    *Guid  OPTIONAL,
  IN  UINTN             ExTokenNumber,
  IN  PCD_PPI_CALLBACK  CallBackFunction
  )
{
  if (!FeaturePcdGet (PcdPeiFullPcdDatabaseEnable)) {
    return EFI_UNSUPPORTED;
  }

  if (CallBackFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return PeiRegisterCallBackWorker (ExTokenNumber, Guid, CallBackFunction, TRUE);
}

/**
  Cancels a previously set callback function for a particular PCD token number.

  @param[in]  Guid The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]  ExTokenNumber The PCD token number.
  @param[in]  CallBackFunction The function prototype called when the value associated with the CallBackToken is set.

  @retval EFI_SUCCESS  The PCD service has successfully established a call event
                        for the CallBackToken requested.
  @retval EFI_NOT_FOUND The PCD service could not find the referenced token number.

**/
EFI_STATUS
EFIAPI
PcdUnRegisterCallBackOnSet (
  IN  CONST EFI_GUID    *Guid  OPTIONAL,
  IN  UINTN             ExTokenNumber,
  IN  PCD_PPI_CALLBACK  CallBackFunction
  )
{
  if (!FeaturePcdGet (PcdPeiFullPcdDatabaseEnable)) {
    return EFI_UNSUPPORTED;
  }

  if (CallBackFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return PeiRegisterCallBackWorker (ExTokenNumber, Guid, CallBackFunction, FALSE);
}

/**
  Retrieves the next valid token number in a given namespace.

  This is useful since the PCD infrastructure contains a sparse list of token numbers,
  and one cannot a priori know what token numbers are valid in the database.

  If TokenNumber is 0 and Guid is not NULL, then the first token from the token space specified by Guid is returned.
  If TokenNumber is not 0 and Guid is not NULL, then the next token in the token space specified by Guid is returned.
  If TokenNumber is 0 and Guid is NULL, then the first token in the default token space is returned.
  If TokenNumber is not 0 and Guid is NULL, then the next token in the default token space is returned.
  The token numbers in the default token space may not be related to token numbers in token spaces that are named by Guid.
  If the next token number can be retrieved, then it is returned in TokenNumber, and EFI_SUCCESS is returned.
  If TokenNumber represents the last token number in the token space specified by Guid, then EFI_NOT_FOUND is returned.
  If TokenNumber is not present in the token space specified by Guid, then EFI_NOT_FOUND is returned.


  @param[in]       Guid        The 128-bit unique value that designates the namespace from which to extract the value.
                               This is an optional parameter that may be NULL.  If this parameter is NULL, then a request
                               is being made to retrieve tokens from the default token space.
  @param[in, out]  TokenNumber A pointer to the PCD token number to use to find the subsequent token number.

  @retval EFI_SUCCESS   The PCD service has retrieved the next valid token number.
  @retval EFI_NOT_FOUND The PCD service could not find data from the requested token number.

**/
EFI_STATUS
EFIAPI
PeiPcdGetNextToken (
  IN CONST EFI_GUID  *Guid  OPTIONAL,
  IN OUT  UINTN      *TokenNumber
  )
{
  UINTN              GuidTableIdx;
  PEI_PCD_DATABASE   *PeiPcdDb;
  EFI_GUID           *MatchGuid;
  EFI_GUID           *GuidTable;
  DYNAMICEX_MAPPING  *ExMapTable;
  UINTN              Index;
  BOOLEAN            Found;
  BOOLEAN            PeiExMapTableEmpty;
  UINTN              PeiNexTokenNumber;

  if (!FeaturePcdGet (PcdPeiFullPcdDatabaseEnable)) {
    return EFI_UNSUPPORTED;
  }

  PeiPcdDb          = GetPcdDatabase ();
  PeiNexTokenNumber = PeiPcdDb->LocalTokenCount - PeiPcdDb->ExTokenCount;
  GuidTable         = (EFI_GUID *)((UINT8 *)PeiPcdDb + PeiPcdDb->GuidTableOffset);

  if (PeiPcdDb->ExTokenCount == 0) {
    PeiExMapTableEmpty = TRUE;
  } else {
    PeiExMapTableEmpty = FALSE;
  }

  if (Guid == NULL) {
    if (*TokenNumber > PeiNexTokenNumber) {
      return EFI_NOT_FOUND;
    }

    (*TokenNumber)++;
    if (*TokenNumber > PeiNexTokenNumber) {
      *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
      return EFI_NOT_FOUND;
    }

    return EFI_SUCCESS;
  } else {
    if (PeiExMapTableEmpty) {
      return EFI_NOT_FOUND;
    }

    MatchGuid = ScanGuid (GuidTable, PeiPcdDb->GuidTableCount * sizeof (EFI_GUID), Guid);

    if (MatchGuid == NULL) {
      return EFI_NOT_FOUND;
    }

    GuidTableIdx = MatchGuid - GuidTable;

    ExMapTable = (DYNAMICEX_MAPPING *)((UINT8 *)PeiPcdDb + PeiPcdDb->ExMapTableOffset);

    Found = FALSE;
    //
    // Locate the GUID in ExMapTable first.
    //
    for (Index = 0; Index < PeiPcdDb->ExTokenCount; Index++) {
      if (ExMapTable[Index].ExGuidIndex == GuidTableIdx) {
        Found = TRUE;
        break;
      }
    }

    if (Found) {
      //
      // If given token number is PCD_INVALID_TOKEN_NUMBER, then return the first
      // token number in found token space.
      //
      if (*TokenNumber == PCD_INVALID_TOKEN_NUMBER) {
        *TokenNumber = ExMapTable[Index].ExTokenNumber;
        return EFI_SUCCESS;
      }

      for ( ; Index < PeiPcdDb->ExTokenCount; Index++) {
        if ((ExMapTable[Index].ExTokenNumber == *TokenNumber) && (ExMapTable[Index].ExGuidIndex == GuidTableIdx)) {
          break;
        }
      }

      while (Index < PeiPcdDb->ExTokenCount) {
        Index++;
        if (Index == PeiPcdDb->ExTokenCount) {
          //
          // Exceed the length of ExMap Table
          //
          *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
          return EFI_NOT_FOUND;
        } else if (ExMapTable[Index].ExGuidIndex == GuidTableIdx) {
          //
          // Found the next match
          //
          *TokenNumber = ExMapTable[Index].ExTokenNumber;
          return EFI_SUCCESS;
        }
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Retrieves the next valid PCD token namespace for a given namespace.

  Gets the next valid token namespace for a given namespace. This is useful to traverse the valid
  token namespaces on a platform.

  @param[in, out]   Guid    An indirect pointer to EFI_GUID. On input it designates a known token
                            namespace from which the search will start. On output, it designates the next valid
                            token namespace on the platform. If *Guid is NULL, then the GUID of the first token
                            space of the current platform is returned. If the search cannot locate the next valid
                            token namespace, an error is returned and the value of *Guid is undefined.

  @retval  EFI_SUCCESS      The PCD service retrieved the value requested.
  @retval  EFI_NOT_FOUND    The PCD service could not find the next valid token namespace.

**/
EFI_STATUS
EFIAPI
PeiPcdGetNextTokenSpace (
  IN OUT CONST EFI_GUID  **Guid
  )
{
  UINTN              GuidTableIdx;
  EFI_GUID           *MatchGuid;
  PEI_PCD_DATABASE   *PeiPcdDb;
  DYNAMICEX_MAPPING  *ExMapTable;
  UINTN              Index;
  UINTN              Index2;
  BOOLEAN            Found;
  BOOLEAN            PeiExMapTableEmpty;
  EFI_GUID           *GuidTable;

  if (!FeaturePcdGet (PcdPeiFullPcdDatabaseEnable)) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (Guid != NULL);

  PeiPcdDb = GetPcdDatabase ();

  if (PeiPcdDb->ExTokenCount == 0) {
    PeiExMapTableEmpty = TRUE;
  } else {
    PeiExMapTableEmpty = FALSE;
  }

  if (PeiExMapTableEmpty) {
    return EFI_NOT_FOUND;
  }

  ExMapTable = (DYNAMICEX_MAPPING *)((UINT8 *)PeiPcdDb + PeiPcdDb->ExMapTableOffset);
  GuidTable  = (EFI_GUID *)((UINT8 *)PeiPcdDb + PeiPcdDb->GuidTableOffset);

  if (*Guid == NULL) {
    //
    // return the first Token Space Guid.
    //
    *Guid = GuidTable + ExMapTable[0].ExGuidIndex;
    return EFI_SUCCESS;
  }

  MatchGuid = ScanGuid (GuidTable, PeiPcdDb->GuidTableCount * sizeof (GuidTable[0]), *Guid);

  if (MatchGuid == NULL) {
    return EFI_NOT_FOUND;
  }

  GuidTableIdx = MatchGuid - GuidTable;

  Found = FALSE;
  for (Index = 0; Index < PeiPcdDb->ExTokenCount; Index++) {
    if (ExMapTable[Index].ExGuidIndex == GuidTableIdx) {
      Found = TRUE;
      break;
    }
  }

  if (Found) {
    Index++;
    for ( ; Index < PeiPcdDb->ExTokenCount; Index++ ) {
      if (ExMapTable[Index].ExGuidIndex != GuidTableIdx) {
        Found = FALSE;
        for (Index2 = 0; Index2 < Index; Index2++) {
          if (ExMapTable[Index2].ExGuidIndex == ExMapTable[Index].ExGuidIndex) {
            //
            // This token namespace should have been found and output at preceding getting.
            //
            Found = TRUE;
            break;
          }
        }

        if (!Found) {
          *Guid = (EFI_GUID *)((UINT8 *)PeiPcdDb + PeiPcdDb->GuidTableOffset) + ExMapTable[Index].ExGuidIndex;
          return EFI_SUCCESS;
        }
      }
    }

    *Guid = NULL;
  }

  return EFI_NOT_FOUND;
}

/**
  Get PCD value's size for POINTER type PCD.

  The POINTER type PCD's value will be stored into a buffer in specified size.
  The max size of this PCD's value is described in PCD's definition in DEC file.

  @param LocalTokenNumberTableIdx Index of PCD token number in PCD token table
  @param MaxSize                  Maximum size of PCD's value
  @param Database                 Pcd database in PEI phase.

  @return PCD value's size for POINTER type PCD.

**/
UINTN
GetPtrTypeSize (
  IN    UINTN             LocalTokenNumberTableIdx,
  OUT   UINTN             *MaxSize,
  IN    PEI_PCD_DATABASE  *Database
  )
{
  INTN       SizeTableIdx;
  UINTN      LocalTokenNumber;
  SIZE_INFO  *SizeTable;

  SizeTableIdx = GetSizeTableIndex (LocalTokenNumberTableIdx, Database);

  LocalTokenNumber = *((UINT32 *)((UINT8 *)Database + Database->LocalTokenNumberTableOffset) + LocalTokenNumberTableIdx);

  ASSERT ((LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) == PCD_DATUM_TYPE_POINTER);

  SizeTable = (SIZE_INFO *)((UINT8 *)Database + Database->SizeTableOffset);

  *MaxSize = SizeTable[SizeTableIdx];
  //
  // SizeTable only contain record for PCD_DATUM_TYPE_POINTER type
  // PCD entry.
  //
  if ((LocalTokenNumber & PCD_TYPE_VPD) != 0) {
    //
    // We have only two entry for VPD enabled PCD entry:
    // 1) MAX Size.
    // 2) Current Size
    // We consider current size is equal to MAX size.
    //
    return *MaxSize;
  } else {
    //
    // We have only two entry for Non-Sku enabled PCD entry:
    // 1) MAX SIZE
    // 2) Current Size
    //
    return SizeTable[SizeTableIdx + 1];
  }
}

/**
  Set PCD value's size for POINTER type PCD.

  The POINTER type PCD's value will be stored into a buffer in specified size.
  The max size of this PCD's value is described in PCD's definition in DEC file.

  @param LocalTokenNumberTableIdx Index of PCD token number in PCD token table
  @param CurrentSize              Maximum size of PCD's value
  @param Database                 Pcd database in PEI phase.

  @retval TRUE  Success to set PCD's value size, which is not exceed maximum size
  @retval FALSE Fail to set PCD's value size, which maybe exceed maximum size

**/
BOOLEAN
SetPtrTypeSize (
  IN          UINTN             LocalTokenNumberTableIdx,
  IN    OUT   UINTN             *CurrentSize,
  IN          PEI_PCD_DATABASE  *Database
  )
{
  INTN       SizeTableIdx;
  UINTN      LocalTokenNumber;
  SIZE_INFO  *SizeTable;
  UINTN      MaxSize;

  SizeTableIdx = GetSizeTableIndex (LocalTokenNumberTableIdx, Database);

  LocalTokenNumber = *((UINT32 *)((UINT8 *)Database + Database->LocalTokenNumberTableOffset) + LocalTokenNumberTableIdx);

  ASSERT ((LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) == PCD_DATUM_TYPE_POINTER);

  SizeTable = (SIZE_INFO *)((UINT8 *)Database + Database->SizeTableOffset);

  MaxSize = SizeTable[SizeTableIdx];
  //
  // SizeTable only contain record for PCD_DATUM_TYPE_POINTER type
  // PCD entry.
  //
  if ((LocalTokenNumber & PCD_TYPE_VPD) != 0) {
    //
    // We shouldn't come here as we don't support SET for VPD
    //
    ASSERT (FALSE);
    return FALSE;
  } else {
    if ((*CurrentSize > MaxSize) ||
        (*CurrentSize == MAX_ADDRESS))
    {
      *CurrentSize = MaxSize;
      return FALSE;
    }

    //
    // We have only two entry for Non-Sku enabled PCD entry:
    // 1) MAX SIZE
    // 2) Current Size
    //
    SizeTable[SizeTableIdx + 1] = (SIZE_INFO)*CurrentSize;
    return TRUE;
  }
}
