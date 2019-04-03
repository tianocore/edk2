/** @file
  Esrt management implementation.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EsrtImpl.h"

/**
  Find Esrt Entry stored in ESRT repository.

  @param[in]     FwClass           Firmware class guid in Esrt entry
  @param[in]     Attribute         Esrt from Non FMP or FMP instance
  @param[out]    Entry             Esrt entry returned

  @retval EFI_SUCCESS            Successfully find an Esrt entry
  @retval EF_NOT_FOUND           No Esrt entry found

**/
EFI_STATUS
GetEsrtEntry (
  IN  EFI_GUID              *FwClass,
  IN  UINTN                 Attribute,
  OUT EFI_SYSTEM_RESOURCE_ENTRY *Entry
  )
{
  EFI_STATUS                 Status;
  CHAR16                     *VariableName;
  EFI_SYSTEM_RESOURCE_ENTRY  *EsrtRepository;
  UINTN                      RepositorySize;
  UINTN                      Index;
  UINTN                      EsrtNum;

  EsrtRepository = NULL;

  //
  // Get Esrt index buffer
  //
  if (Attribute == ESRT_FROM_FMP) {
    VariableName = EFI_ESRT_FMP_VARIABLE_NAME;
  } else {
    VariableName = EFI_ESRT_NONFMP_VARIABLE_NAME;
  }

  Status = GetVariable2 (
             VariableName,
             &gEfiCallerIdGuid,
             (VOID **) &EsrtRepository,
             &RepositorySize
             );

  if (EFI_ERROR(Status)) {
    goto EXIT;
  }

  if (RepositorySize % sizeof(EFI_SYSTEM_RESOURCE_ENTRY) != 0) {
    DEBUG((EFI_D_ERROR, "Repository Corrupt. Need to rebuild Repository.\n"));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  Status  = EFI_NOT_FOUND;
  EsrtNum = RepositorySize/sizeof(EFI_SYSTEM_RESOURCE_ENTRY);
  for (Index = 0; Index < EsrtNum; Index++) {
    if (CompareGuid(FwClass, &EsrtRepository[Index].FwClass)) {
      CopyMem(Entry, &EsrtRepository[Index], sizeof(EFI_SYSTEM_RESOURCE_ENTRY));
      Status = EFI_SUCCESS;
      break;
    }
  }

EXIT:
  if (EsrtRepository != NULL) {
    FreePool(EsrtRepository);
  }

  return Status;
}

/**
  Insert a new ESRT entry into ESRT Cache repository.

  @param[in]  Entry                Esrt entry to be set
  @param[in]  Attribute            Esrt from Esrt private protocol or FMP instance

  @retval EFI_SUCCESS          Successfully set a variable.

**/
EFI_STATUS
InsertEsrtEntry(
  IN EFI_SYSTEM_RESOURCE_ENTRY *Entry,
  UINTN                        Attribute
  )
{
  EFI_STATUS                 Status;
  CHAR16                     *VariableName;
  EFI_SYSTEM_RESOURCE_ENTRY  *EsrtRepository;
  UINTN                      RepositorySize;
  EFI_SYSTEM_RESOURCE_ENTRY  *EsrtRepositoryNew;

  EsrtRepository    = NULL;
  EsrtRepositoryNew = NULL;

  //
  // Get Esrt index buffer
  //
  if (Attribute == ESRT_FROM_FMP) {
    VariableName = EFI_ESRT_FMP_VARIABLE_NAME;
  } else {
    VariableName = EFI_ESRT_NONFMP_VARIABLE_NAME;
  }

  Status = GetVariable2 (
             VariableName,
             &gEfiCallerIdGuid,
             (VOID **) &EsrtRepository,
             &RepositorySize
             );

  if (Status == EFI_NOT_FOUND) {
    //
    // If not exist, create new Esrt cache repository
    //
    Status = gRT->SetVariable(
                    VariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof(EFI_SYSTEM_RESOURCE_ENTRY),
                    Entry
                    );
    return Status;

  } else if (Status == EFI_SUCCESS) {
    //
    // if exist, update Esrt cache repository
    //
    if (RepositorySize % sizeof(EFI_SYSTEM_RESOURCE_ENTRY) != 0) {
      DEBUG((EFI_D_ERROR, "Repository Corrupt. Need to rebuild Repository.\n"));
      //
      // Repository is corrupt. Clear Repository before insert new entry
      //
      Status = gRT->SetVariable(
                      VariableName,
                      &gEfiCallerIdGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                      0,
                      EsrtRepository
                      );
      FreePool(EsrtRepository);
      RepositorySize = 0;
      EsrtRepository = NULL;
    }

    //
    // Check Repository size constraint
    //
    if ((Attribute == ESRT_FROM_FMP && RepositorySize >= PcdGet32(PcdMaxFmpEsrtCacheNum) * sizeof(EFI_SYSTEM_RESOURCE_ENTRY))
      ||(Attribute == ESRT_FROM_NONFMP && RepositorySize >= PcdGet32(PcdMaxNonFmpEsrtCacheNum) * sizeof(EFI_SYSTEM_RESOURCE_ENTRY)) ) {
      Status = EFI_OUT_OF_RESOURCES;
      goto EXIT;
    }

    EsrtRepositoryNew = AllocatePool(RepositorySize + sizeof(EFI_SYSTEM_RESOURCE_ENTRY));
    if (EsrtRepositoryNew == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto EXIT;
    }

    if (RepositorySize != 0 && EsrtRepository != NULL) {
      CopyMem(EsrtRepositoryNew, EsrtRepository, RepositorySize);
    }
    CopyMem((UINT8 *)EsrtRepositoryNew + RepositorySize, Entry, sizeof(EFI_SYSTEM_RESOURCE_ENTRY));

    Status = gRT->SetVariable(
                    VariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    RepositorySize + sizeof(EFI_SYSTEM_RESOURCE_ENTRY),
                    EsrtRepositoryNew
                    );
  }

EXIT:
  if (EsrtRepository != NULL) {
    FreePool(EsrtRepository);
  }

  if (EsrtRepositoryNew != NULL) {
    FreePool(EsrtRepositoryNew);
  }

  return Status;
}

/**
  Delete ESRT Entry from ESRT repository.

  @param[in]    FwClass              FwClass of Esrt entry to delete
  @param[in]    Attribute            Esrt from Esrt private protocol or FMP instance

  @retval EFI_SUCCESS         Insert all entries Successfully
  @retval EFI_NOT_FOUND       ESRT entry with FwClass doesn't exsit

**/
EFI_STATUS
DeleteEsrtEntry(
  IN  EFI_GUID        *FwClass,
  IN  UINTN           Attribute
  )
{
  EFI_STATUS                 Status;
  CHAR16                     *VariableName;
  EFI_SYSTEM_RESOURCE_ENTRY  *EsrtRepository;
  UINTN                      RepositorySize;
  UINTN                      Index;
  UINTN                      EsrtNum;

  EsrtRepository = NULL;

  //
  // Get Esrt index buffer
  //
  if (Attribute == ESRT_FROM_FMP) {
    VariableName = EFI_ESRT_FMP_VARIABLE_NAME;
  } else {
    VariableName = EFI_ESRT_NONFMP_VARIABLE_NAME;
  }

  Status = GetVariable2 (
             VariableName,
             &gEfiCallerIdGuid,
             (VOID **) &EsrtRepository,
             &RepositorySize
             );

  if (EFI_ERROR(Status)) {
    goto EXIT;
  }

  if ((RepositorySize % sizeof(EFI_SYSTEM_RESOURCE_ENTRY)) != 0) {
    DEBUG((EFI_D_ERROR, "Repository Corrupt. Need to rebuild Repository.\n"));
    //
    // Repository is corrupt. Clear Repository before insert new entry
    //
    Status = gRT->SetVariable(
                    VariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    0,
                    EsrtRepository
                    );
    goto EXIT;
  }

  Status = EFI_NOT_FOUND;
  EsrtNum = RepositorySize/sizeof(EFI_SYSTEM_RESOURCE_ENTRY);
  for (Index = 0; Index < EsrtNum; Index++) {
    //
    // Delete Esrt entry if it is found in repository
    //
    if (CompareGuid(FwClass, &EsrtRepository[Index].FwClass)) {
      //
      // If delete Esrt entry is not at the rail
      //
      if (Index < EsrtNum - 1) {
        CopyMem(&EsrtRepository[Index], &EsrtRepository[Index + 1], (EsrtNum - Index - 1) * sizeof(EFI_SYSTEM_RESOURCE_ENTRY));
      }

      //
      // Update New Repository
      //
      Status = gRT->SetVariable(
                      VariableName,
                      &gEfiCallerIdGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                      (EsrtNum - 1) * sizeof(EFI_SYSTEM_RESOURCE_ENTRY),
                      EsrtRepository
                      );
      break;
    }
  }

EXIT:
  if (EsrtRepository != NULL) {
    FreePool(EsrtRepository);
  }

  return Status;

}

/**
  Update one ESRT entry in ESRT repository

  @param[in]    Entry                Esrt entry to be set
  @param[in]    Attribute            Esrt from Non Esrt or FMP instance

  @retval EFI_SUCCESS          Successfully Update a variable.
  @retval EFI_NOT_FOUND        The Esrt enry doesn't exist

**/
EFI_STATUS
UpdateEsrtEntry(
  IN EFI_SYSTEM_RESOURCE_ENTRY *Entry,
  UINTN                        Attribute
  )
{
  EFI_STATUS                 Status;
  CHAR16                     *VariableName;
  EFI_SYSTEM_RESOURCE_ENTRY  *EsrtRepository;
  UINTN                      RepositorySize;
  UINTN                      Index;
  UINTN                      EsrtNum;

  EsrtRepository    = NULL;

  //
  // Get Esrt index buffer
  //
  if (Attribute == ESRT_FROM_FMP) {
    VariableName = EFI_ESRT_FMP_VARIABLE_NAME;
  } else {
    VariableName = EFI_ESRT_NONFMP_VARIABLE_NAME;
  }

  Status = GetVariable2 (
             VariableName,
             &gEfiCallerIdGuid,
             (VOID **) &EsrtRepository,
             &RepositorySize
             );

  if (!EFI_ERROR(Status)) {
    //
    // if exist, update Esrt cache repository
    //
    if (RepositorySize % sizeof(EFI_SYSTEM_RESOURCE_ENTRY) != 0) {
      DEBUG((EFI_D_ERROR, "Repository Corrupt. Need to rebuild Repository.\n"));
      //
      // Repository is corrupt. Clear Repository before insert new entry
      //
      Status = gRT->SetVariable(
                      VariableName,
                      &gEfiCallerIdGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                      0,
                      EsrtRepository
                      );
      Status = EFI_NOT_FOUND;
      goto EXIT;
    }

    Status = EFI_NOT_FOUND;
    EsrtNum = RepositorySize/sizeof(EFI_SYSTEM_RESOURCE_ENTRY);
    for (Index = 0; Index < EsrtNum; Index++) {
      //
      // Update Esrt entry if it is found in repository
      //
      if (CompareGuid(&Entry->FwClass, &EsrtRepository[Index].FwClass)) {

        CopyMem(&EsrtRepository[Index], Entry, sizeof(EFI_SYSTEM_RESOURCE_ENTRY));
        //
        // Update New Repository
        //
        Status = gRT->SetVariable(
                        VariableName,
                        &gEfiCallerIdGuid,
                        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                        RepositorySize,
                        EsrtRepository
                        );
        break;
      }
    }
  }

EXIT:
  if (EsrtRepository != NULL) {
    FreePool(EsrtRepository);
  }

  return Status;
}

/**
  Return if this FMP is a system FMP or a device FMP, based upon FmpImageInfo.

  @param[in] FmpImageInfo A pointer to EFI_FIRMWARE_IMAGE_DESCRIPTOR

  @return TRUE  It is a system FMP.
  @return FALSE It is a device FMP.
**/
BOOLEAN
IsSystemFmp (
  IN EFI_FIRMWARE_IMAGE_DESCRIPTOR   *FmpImageInfo
  )
{
  GUID      *Guid;
  UINTN     Count;
  UINTN     Index;

  Guid = PcdGetPtr(PcdSystemFmpCapsuleImageTypeIdGuid);
  Count = PcdGetSize(PcdSystemFmpCapsuleImageTypeIdGuid)/sizeof(GUID);

  for (Index = 0; Index < Count; Index++, Guid++) {
    if (CompareGuid(&FmpImageInfo->ImageTypeId, Guid)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Init one ESRT entry according to input FmpImageInfo (V1, V2, V3) .

  @param[in, out]     EsrtEntry            Esrt entry to be Init
  @param[in]          FmpImageInfo         FMP image info descriptor
  @param[in]          DescriptorVersion    FMP Image info descriptor version

**/
VOID
SetEsrtEntryFromFmpInfo (
  IN OUT EFI_SYSTEM_RESOURCE_ENTRY   *EsrtEntry,
  IN EFI_FIRMWARE_IMAGE_DESCRIPTOR   *FmpImageInfo,
  IN UINT32                          DescriptorVersion
  )
{
  EsrtEntry->FwVersion                = FmpImageInfo->Version;
  EsrtEntry->FwClass                  = FmpImageInfo->ImageTypeId;
  if (IsSystemFmp(FmpImageInfo)) {
    EsrtEntry->FwType                   = ESRT_FW_TYPE_SYSTEMFIRMWARE;
  } else {
    EsrtEntry->FwType                   = ESRT_FW_TYPE_DEVICEFIRMWARE;
  }
  EsrtEntry->LowestSupportedFwVersion = 0;
  EsrtEntry->CapsuleFlags             = 0;
  EsrtEntry->LastAttemptVersion       = 0;
  EsrtEntry->LastAttemptStatus        = LAST_ATTEMPT_STATUS_SUCCESS;

  if (DescriptorVersion >= 2) {
    //
    // LowestSupportedImageVersion only available in FMP V2 or higher
    //
    EsrtEntry->LowestSupportedFwVersion = FmpImageInfo->LowestSupportedImageVersion;
  }

  if (DescriptorVersion >= 3) {
    //
    // LastAttemptVersion & LastAttemptStatus only available in FMP V3 or higher
    //
    EsrtEntry->LastAttemptVersion = FmpImageInfo->LastAttemptVersion;
    EsrtEntry->LastAttemptStatus  = FmpImageInfo->LastAttemptStatus;
  }

  //
  // Set capsule customized flag
  //
  if ((FmpImageInfo->AttributesSupported & IMAGE_ATTRIBUTE_RESET_REQUIRED) != 0
   && (FmpImageInfo->AttributesSetting & IMAGE_ATTRIBUTE_RESET_REQUIRED) != 0) {
    EsrtEntry->CapsuleFlags = PcdGet16(PcdSystemRebootAfterCapsuleProcessFlag);
  }
}
