/** @file
  Esrt management module.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "EsrtImpl.h"


//
// Module globals.
//

ESRT_PRIVATE_DATA mPrivate;

ESRT_MANAGEMENT_PROTOCOL  mEsrtManagementProtocolTemplate = { 
                            EsrtDxeGetEsrtEntry,
                            EsrtDxeUpdateEsrtEntry,
                            EsrtDxeRegisterEsrtEntry,
                            EsrtDxeUnRegisterEsrtEntry,
                            EsrtDxeSyncFmp,
                            EsrtDxeLockEsrtRepository
                            };

/**
  Get ESRT entry from ESRT Cache by FwClass Guid 

  @param[in]       FwClass                FwClass of Esrt entry to get  
  @param[in, out]  Entry                  Esrt entry returned 
  
  @retval EFI_SUCCESS                   The variable saving this Esrt Entry exists.
  @retval EF_NOT_FOUND                  No correct variable found.
  @retval EFI_WRITE_PROTECTED           ESRT Cache repository is locked

**/
EFI_STATUS
EFIAPI
EsrtDxeGetEsrtEntry(
  IN     EFI_GUID                  *FwClass,
  IN OUT EFI_SYSTEM_RESOURCE_ENTRY *Entry
  )
{
  EFI_STATUS                Status;

  if (FwClass == NULL || Entry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EfiAcquireLockOrFail (&mPrivate.NonFmpLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find in Non-FMP Cached Esrt Repository
  //
  Status = GetEsrtEntry(
             FwClass,
             ESRT_FROM_NONFMP,
             Entry
             );

  EfiReleaseLock(&mPrivate.NonFmpLock);

  if (EFI_ERROR(Status)) {
    Status = EfiAcquireLockOrFail (&mPrivate.FmpLock);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Find in FMP Cached Esrt NV Variable
    //
    Status = GetEsrtEntry(
               FwClass,
               ESRT_FROM_FMP,
               Entry
               );

    EfiReleaseLock(&mPrivate.FmpLock);
  }

  return Status;
}

/**
  Update one ESRT entry in ESRT Cache.

  @param[in]  Entry                         Esrt entry to be updated
  
  @retval EFI_SUCCESS                   Successfully update an ESRT entry in cache.
  @retval EFI_INVALID_PARAMETER         Entry does't exist in ESRT Cache
  @retval EFI_WRITE_PROTECTED           ESRT Cache repositoy is locked

**/
EFI_STATUS
EFIAPI
EsrtDxeUpdateEsrtEntry(
  IN EFI_SYSTEM_RESOURCE_ENTRY *Entry
  )
{
  EFI_STATUS                Status;
  
  if (Entry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EfiAcquireLockOrFail (&mPrivate.FmpLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UpdateEsrtEntry(Entry, ESRT_FROM_FMP);

  if (!EFI_ERROR(Status)) {
    EfiReleaseLock(&mPrivate.FmpLock);
    return Status;
  }
  EfiReleaseLock(&mPrivate.FmpLock);


  Status = EfiAcquireLockOrFail (&mPrivate.NonFmpLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UpdateEsrtEntry(Entry, ESRT_FROM_NONFMP);

  EfiReleaseLock(&mPrivate.NonFmpLock);

  return Status;
}

/**
  Non-FMP instance to unregister Esrt Entry from ESRT Cache. 

  @param[in]    FwClass                FwClass of Esrt entry to Unregister  
  
  @retval EFI_SUCCESS             Insert all entries Successfully 
  @retval EFI_NOT_FOUND           Entry of FwClass does not exsit

**/
EFI_STATUS
EFIAPI
EsrtDxeUnRegisterEsrtEntry(
  IN  EFI_GUID        *FwClass
  )
{
  EFI_STATUS Status; 

  if (FwClass == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EfiAcquireLockOrFail (&mPrivate.NonFmpLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DeleteEsrtEntry(FwClass, ESRT_FROM_NONFMP);

  EfiReleaseLock(&mPrivate.NonFmpLock);

  return Status;
}

/**
  Non-FMP instance to register one ESRT entry into ESRT Cache.

  @param[in]  Entry                Esrt entry to be set

  @retval EFI_SUCCESS              Successfully set a variable.
  @retval EFI_INVALID_PARAMETER    ESRT Entry is already exist
  @retval EFI_OUT_OF_RESOURCES     Non-FMP ESRT repository is full

**/
EFI_STATUS
EFIAPI
EsrtDxeRegisterEsrtEntry(
  IN EFI_SYSTEM_RESOURCE_ENTRY *Entry
  )
{
  EFI_STATUS                Status;
  EFI_SYSTEM_RESOURCE_ENTRY EsrtEntryTmp;

  if (Entry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EfiAcquireLockOrFail (&mPrivate.NonFmpLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetEsrtEntry(
             &Entry->FwClass,
             ESRT_FROM_NONFMP,
             &EsrtEntryTmp
             );

  if (Status == EFI_NOT_FOUND) {
    Status = InsertEsrtEntry(Entry, ESRT_FROM_NONFMP);
  }

  EfiReleaseLock(&mPrivate.NonFmpLock);

  return Status;
}

/**
  This function syn up Cached ESRT with data from FMP instances
  Function should be called after Connect All in order to locate all FMP protocols
  installed.

  @retval EFI_SUCCESS                      Successfully sync cache repository from FMP instances
  @retval EFI_NOT_FOUND                   No FMP Instance are found
  @retval EFI_OUT_OF_RESOURCES     Resource allocaton fail

**/
EFI_STATUS
EFIAPI
EsrtDxeSyncFmp(
  VOID
  ) 
{
  EFI_STATUS                                Status;
  UINTN                                     Index1;
  UINTN                                     Index2;
  UINTN                                     Index3;
  EFI_HANDLE                                *HandleBuffer;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL          **FmpBuf;
  UINTN                                     NumberOfHandles;
  UINTN                                     *DescriptorSizeBuf;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR             **FmpImageInfoBuf;  
  EFI_FIRMWARE_IMAGE_DESCRIPTOR             *TempFmpImageInfo;  
  UINT8                                     *FmpImageInfoCountBuf;
  UINT32                                    *FmpImageInfoDescriptorVerBuf;
  UINTN                                     ImageInfoSize;
  UINT32                                    PackageVersion;
  CHAR16                                    *PackageVersionName;
  EFI_SYSTEM_RESOURCE_ENTRY                 *EsrtRepositoryNew;
  UINTN                                     EntryNumNew;

  NumberOfHandles              = 0;
  EntryNumNew                  = 0;
  FmpBuf                       = NULL;
  HandleBuffer                 = NULL;
  FmpImageInfoBuf              = NULL;
  FmpImageInfoCountBuf         = NULL;
  PackageVersionName           = NULL;
  DescriptorSizeBuf            = NULL;
  FmpImageInfoDescriptorVerBuf = NULL;
  EsrtRepositoryNew            = NULL;

  //
  // Get image information from all FMP protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareManagementProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );

 
  if (Status == EFI_NOT_FOUND) { 
    EntryNumNew = 0;
    goto UPDATE_REPOSITORY;
  } else if (EFI_ERROR(Status)){ 
    goto END;
  }

  //
  // Allocate buffer to hold new FMP ESRT Cache repository
  //
  EsrtRepositoryNew = AllocateZeroPool(PcdGet32(PcdMaxFmpEsrtCacheNum) * sizeof(EFI_SYSTEM_RESOURCE_ENTRY));
  if (EsrtRepositoryNew == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto END;
  }

  FmpBuf = AllocatePool(sizeof(EFI_FIRMWARE_MANAGEMENT_PROTOCOL *) * NumberOfHandles);
  if (FmpBuf == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto END;
  }

  FmpImageInfoBuf = AllocateZeroPool(sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR *) * NumberOfHandles);
  if (FmpImageInfoBuf == NULL) {
    Status = EFI_OUT_OF_RESOURCES;    
    goto END;
  }

  FmpImageInfoCountBuf = AllocateZeroPool(sizeof(UINT8) * NumberOfHandles);
  if (FmpImageInfoCountBuf == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto END;
  }

  DescriptorSizeBuf = AllocateZeroPool(sizeof(UINTN) * NumberOfHandles);
  if (DescriptorSizeBuf == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto END;
  }

  FmpImageInfoDescriptorVerBuf = AllocateZeroPool(sizeof(UINT32) * NumberOfHandles);
   if (FmpImageInfoDescriptorVerBuf == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto END;
  }

  //
  // Get all FmpImageInfo Descriptor into FmpImageInfoBuf
  //
  for (Index1 = 0; Index1 < NumberOfHandles; Index1++){
    Status = gBS->HandleProtocol(
                    HandleBuffer[Index1],
                    &gEfiFirmwareManagementProtocolGuid,
                    (VOID **)&FmpBuf[Index1]
                    );

    if (EFI_ERROR(Status)) {
      continue;
    }

    ImageInfoSize = 0;
    Status = FmpBuf[Index1]->GetImageInfo (
                               FmpBuf[Index1],
                               &ImageInfoSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL
                               );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      FmpImageInfoBuf[Index1] = AllocateZeroPool(ImageInfoSize);
      if (FmpImageInfoBuf[Index1] == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto END;
      }
    } else {
      continue;
    }

    PackageVersionName = NULL;
    Status = FmpBuf[Index1]->GetImageInfo (
                               FmpBuf[Index1],
                               &ImageInfoSize,
                               FmpImageInfoBuf[Index1],
                               &FmpImageInfoDescriptorVerBuf[Index1],
                               &FmpImageInfoCountBuf[Index1],
                               &DescriptorSizeBuf[Index1],
                               &PackageVersion,
                               &PackageVersionName
                               );

    //
    // If FMP GetInformation interface failed, skip this resource
    //
    if (EFI_ERROR(Status)){
      FmpImageInfoCountBuf[Index1] = 0;
      continue;
    }

    if (PackageVersionName != NULL) {
      FreePool(PackageVersionName);
    }
  }

  //
  // Create new FMP cache repository based on FmpImageInfoBuf
  // 
  for (Index2 = 0; Index2 < NumberOfHandles; Index2++){
    TempFmpImageInfo = FmpImageInfoBuf[Index2];
    for (Index3 = 0; Index3 < FmpImageInfoCountBuf[Index2]; Index3++){
      if ((TempFmpImageInfo->AttributesSupported & IMAGE_ATTRIBUTE_IN_USE) != 0 
      && (TempFmpImageInfo->AttributesSetting & IMAGE_ATTRIBUTE_IN_USE) != 0){
        //
        // Always put the first smallest version of Image info into ESRT cache 
        //
        for(Index1 = 0; Index1 < EntryNumNew; Index1++) {
          if (CompareGuid(&EsrtRepositoryNew[Index1].FwClass, &TempFmpImageInfo->ImageTypeId)) {
            if(EsrtRepositoryNew[Index1].FwVersion > TempFmpImageInfo->Version) {
              SetEsrtEntryFromFmpInfo(&EsrtRepositoryNew[Index1], TempFmpImageInfo, FmpImageInfoDescriptorVerBuf[Index2]);
            }
            break;
          }
        }
        //
        // New ImageTypeId can't be found in EsrtRepositoryNew. Create a new one
        //
        if (Index1 == EntryNumNew){
          SetEsrtEntryFromFmpInfo(&EsrtRepositoryNew[EntryNumNew], TempFmpImageInfo, FmpImageInfoDescriptorVerBuf[Index2]);
          EntryNumNew++; 
          if (EntryNumNew >= PcdGet32(PcdMaxFmpEsrtCacheNum)) {
            break;
          }
        }
      }

      //
      // Use DescriptorSize to move ImageInfo Pointer to stay compatible with different ImageInfo version
      //
      TempFmpImageInfo = (EFI_FIRMWARE_IMAGE_DESCRIPTOR *)((UINT8 *)TempFmpImageInfo + DescriptorSizeBuf[Index2]);
    }
  }

UPDATE_REPOSITORY:

  Status = EfiAcquireLockOrFail (&mPrivate.FmpLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable(
                  EFI_ESRT_FMP_VARIABLE_NAME,
                  &gEfiCallerIdGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  EntryNumNew * sizeof(EFI_SYSTEM_RESOURCE_ENTRY),
                  EsrtRepositoryNew
                  );

  EfiReleaseLock(&mPrivate.FmpLock);

END:
  if (EsrtRepositoryNew != NULL) {
    FreePool(EsrtRepositoryNew);
  }

  if (HandleBuffer != NULL) {
    FreePool(HandleBuffer);
  }

  if (FmpBuf != NULL) {
    FreePool(FmpBuf);
  }

  if (FmpImageInfoCountBuf != NULL) {
    FreePool(FmpImageInfoCountBuf);
  }

  if (DescriptorSizeBuf != NULL) {
    FreePool(DescriptorSizeBuf);
  }

  if (FmpImageInfoDescriptorVerBuf != NULL) {
    FreePool(FmpImageInfoDescriptorVerBuf);
  }

  if (FmpImageInfoBuf != NULL) {
    for (Index1 = 0; Index1 < NumberOfHandles; Index1++){
      if (FmpImageInfoBuf[Index1] != NULL) {
        FreePool(FmpImageInfoBuf[Index1]);
      }
    }
    FreePool(FmpImageInfoBuf);
  }

  return Status;
}

/**
  This function locks up Esrt repository to be readonly. It should be called 
  before gEfiEndOfDxeEventGroupGuid event signaled

  @retval EFI_SUCCESS              Locks up FMP Non-FMP repository successfully 

**/
EFI_STATUS
EFIAPI
EsrtDxeLockEsrtRepository(
  VOID
  )
{
  EFI_STATUS                    Status;
  EDKII_VARIABLE_LOCK_PROTOCOL  *VariableLock;
  //
  // Mark ACPI_GLOBAL_VARIABLE variable to read-only if the Variable Lock protocol exists
  //
  Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **) &VariableLock);
  if (!EFI_ERROR (Status)) {
    Status = VariableLock->RequestToLock (VariableLock, EFI_ESRT_FMP_VARIABLE_NAME, &gEfiCallerIdGuid);
    DEBUG((EFI_D_INFO, "EsrtDxe Lock EsrtFmp Variable Status 0x%x", Status));

    Status = VariableLock->RequestToLock (VariableLock, EFI_ESRT_NONFMP_VARIABLE_NAME, &gEfiCallerIdGuid);
    DEBUG((EFI_D_INFO, "EsrtDxe Lock EsrtNonFmp Variable Status 0x%x", Status));
  }

  return Status;
}

/**
  Notify function for event group EFI_EVENT_GROUP_READY_TO_BOOT. This is used to
  install the Esrt Table into system configuration table

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
VOID
EFIAPI
EsrtReadyToBootEventNotify (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{  
  EFI_STATUS                 Status;
  EFI_SYSTEM_RESOURCE_TABLE  *EsrtTable;
  EFI_SYSTEM_RESOURCE_ENTRY  *FmpEsrtRepository;
  EFI_SYSTEM_RESOURCE_ENTRY  *NonFmpEsrtRepository;
  UINTN                      FmpRepositorySize;
  UINTN                      NonFmpRepositorySize;
  

  FmpEsrtRepository    = NULL;
  NonFmpEsrtRepository = NULL;
  FmpRepositorySize    = 0;
  NonFmpRepositorySize = 0;

  Status = EfiAcquireLockOrFail (&mPrivate.NonFmpLock);
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = GetVariable2 (
             EFI_ESRT_NONFMP_VARIABLE_NAME,
             &gEfiCallerIdGuid,
             (VOID **) &NonFmpEsrtRepository,
             &NonFmpRepositorySize
             );

  if (EFI_ERROR(Status)) {
    NonFmpRepositorySize = 0;
  }

  if (NonFmpRepositorySize % sizeof(EFI_SYSTEM_RESOURCE_ENTRY) != 0) {
    DEBUG((EFI_D_ERROR, "NonFmp Repository Corrupt. Need to rebuild NonFmp Repository.\n"));
    NonFmpRepositorySize = 0;
  }

  EfiReleaseLock(&mPrivate.NonFmpLock);

  Status = EfiAcquireLockOrFail (&mPrivate.FmpLock);
  Status = GetVariable2 (
             EFI_ESRT_FMP_VARIABLE_NAME,
             &gEfiCallerIdGuid,
             (VOID **) &FmpEsrtRepository,
             &FmpRepositorySize
             );

  if (EFI_ERROR(Status)) {
    FmpRepositorySize = 0;
  }

  if (FmpRepositorySize % sizeof(EFI_SYSTEM_RESOURCE_ENTRY) != 0) {
    DEBUG((EFI_D_ERROR, "Fmp Repository Corrupt. Need to rebuild Fmp Repository.\n"));
    FmpRepositorySize = 0;
  }

  EfiReleaseLock(&mPrivate.FmpLock);

  //
  // Skip ESRT table publish if no ESRT entry exists
  //
  if (NonFmpRepositorySize + FmpRepositorySize == 0) {
    goto EXIT;
  }

  EsrtTable = AllocatePool(sizeof(EFI_SYSTEM_RESOURCE_TABLE) + NonFmpRepositorySize + FmpRepositorySize);
  if (EsrtTable == NULL) {
    DEBUG ((EFI_D_ERROR, "Esrt table memory allocation failure\n"));
    goto EXIT;
  }

  EsrtTable->FwResourceVersion  = EFI_SYSTEM_RESOURCE_TABLE_FIRMWARE_RESOURCE_VERSION;                                            
  EsrtTable->FwResourceCount    = (UINT32)((NonFmpRepositorySize + FmpRepositorySize) / sizeof(EFI_SYSTEM_RESOURCE_ENTRY));  
  EsrtTable->FwResourceCountMax = PcdGet32(PcdMaxNonFmpEsrtCacheNum) + PcdGet32(PcdMaxFmpEsrtCacheNum);

  if (NonFmpRepositorySize != 0 && NonFmpEsrtRepository != NULL) {
    CopyMem(EsrtTable + 1, NonFmpEsrtRepository, NonFmpRepositorySize);
  }

  if (FmpRepositorySize != 0 && FmpEsrtRepository != NULL) {
    CopyMem((UINT8 *)(EsrtTable + 1) + NonFmpRepositorySize, FmpEsrtRepository, FmpRepositorySize);
  }

  //
  // Publish Esrt to system config table
  //
  Status = gBS->InstallConfigurationTable (&gEfiSystemResourceTableGuid, EsrtTable);

  //
  // Only one successful install
  //
  gBS->CloseEvent(Event);

EXIT:

  if (FmpEsrtRepository != NULL) {
    FreePool(FmpEsrtRepository);
  }

  if (NonFmpEsrtRepository != NULL) {
    FreePool(NonFmpEsrtRepository);
  }
}

/**
  The module Entry Point of the Esrt DXE driver that manages cached ESRT repository 
  & publishes ESRT table

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
EsrtDxeEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{  
  EFI_STATUS                    Status;

  EfiInitializeLock (&mPrivate.FmpLock,    TPL_CALLBACK);
  EfiInitializeLock (&mPrivate.NonFmpLock, TPL_CALLBACK);

  //
  // Install Esrt management Protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mPrivate.Handle,
                  &gEsrtManagementProtocolGuid,
                  &mEsrtManagementProtocolTemplate,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register notify function to install Esrt Table on ReadyToBoot Event.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  EsrtReadyToBootEventNotify,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &mPrivate.Event
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
