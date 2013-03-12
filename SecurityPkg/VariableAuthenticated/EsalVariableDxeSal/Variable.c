/** @file
  The implementation of Extended SAL variable services.

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"
#include "AuthService.h"

//
// Don't use module globals after the SetVirtualAddress map is signaled
//
ESAL_VARIABLE_GLOBAL  *mVariableModuleGlobal;
CHAR16 *mVariableName[NUM_VAR_NAME] = {
  L"PlatformLangCodes",
  L"LangCodes",
  L"PlatformLang",
  L"Lang",
  L"HwErrRec",
  AUTHVAR_KEYDB_NAME,
  EFI_SETUP_MODE_NAME,
  EFI_PLATFORM_KEY_NAME,
  EFI_KEY_EXCHANGE_KEY_NAME
};

GLOBAL_REMOVE_IF_UNREFERENCED VARIABLE_INFO_ENTRY *gVariableInfo = NULL;

//
// The current Hii implementation accesses this variable a larg # of times on every boot.
// Other common variables are only accessed a single time. This is why this cache algorithm
// only targets a single variable. Probably to get an performance improvement out of
// a Cache you would need a cache that improves the search performance for a variable.
//
VARIABLE_CACHE_ENTRY mVariableCache[] = {
  {
    &gEfiGlobalVariableGuid,
    L"Lang",
    0x00000000,
    0x00,
    NULL
  },
  {
    &gEfiGlobalVariableGuid,
    L"PlatformLang",
    0x00000000,
    0x00,
    NULL
  }
};

/**
  Acquires lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiAcquireLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiAcquireLock() at boot time, and simply returns
  at runtime.

  @param[in]  Lock     A pointer to the lock to acquire.

**/
VOID
AcquireLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
  if (!EfiAtRuntime ()) {
    EfiAcquireLock (Lock);
  }
}

/**
  Releases lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiReleaseLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiReleaseLock() at boot time, and simply returns
  at runtime

  @param[in]  Lock    A pointer to the lock to release.

**/
VOID
ReleaseLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
  if (!EfiAtRuntime ()) {
    EfiReleaseLock (Lock);
  }
}

/**
  Reads/Writes variable storage, volatile or non-volatile.

  This function reads or writes volatile or non-volatile variable stroage.
  For volatile storage, it performs memory copy.
  For non-volatile storage, it accesses data on firmware storage. Data
  area to access can span multiple firmware blocks.

  @param[in]      Write           TRUE  - Write variable store.
                                  FALSE - Read variable store.
  @param[in]      Global          Pointer to VARAIBLE_GLOBAL structure.
  @param[in]      Volatile        TRUE  - Variable is volatile.
                                  FALSE - Variable is non-volatile.
  @param[in]      Instance        Instance of FV Block services.
  @param[in]      StartAddress    Start address of data to access.
  @param[in]      DataSize        Size of data to access.
  @param[in, out] Buffer          For write, pointer to the buffer from which data is written.
                                  For read, pointer to the buffer to hold the data read.

  @retval EFI_SUCCESS            Variable store successfully accessed.
  @retval EFI_INVALID_PARAMETER  Data area to access exceeds valid variable storage.

**/
EFI_STATUS
AccessVariableStore (
  IN     BOOLEAN                 Write,
  IN     VARIABLE_GLOBAL         *Global,
  IN     BOOLEAN                 Volatile,
  IN     UINTN                   Instance,
  IN     EFI_PHYSICAL_ADDRESS    StartAddress,
  IN     UINT32                  DataSize,
  IN OUT VOID                    *Buffer
  )
{
  EFI_FV_BLOCK_MAP_ENTRY      *PtrBlockMapEntry;
  UINTN                       BlockIndex;
  UINTN                       LinearOffset;
  UINTN                       CurrWriteSize;
  UINTN                       CurrWritePtr;
  UINT8                       *CurrBuffer;
  EFI_LBA                     LbaNumber;
  UINTN                       Size;
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  VARIABLE_STORE_HEADER       *VolatileBase;
  EFI_PHYSICAL_ADDRESS        FvVolHdr;
  EFI_STATUS                  Status;
  VARIABLE_STORE_HEADER       *VariableStoreHeader;

  FvVolHdr = 0;
  FwVolHeader = NULL;

  if (Volatile) {
    //
    // If data is volatile, simply calculate the data pointer and copy memory.
    // Data pointer should point to the actual address where data is to be
    // accessed.
    //
    VolatileBase = (VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase);

    if ((StartAddress + DataSize) > ((UINTN) ((UINT8 *) VolatileBase + VolatileBase->Size))) {
      return EFI_INVALID_PARAMETER;
    }
    
    //
    // For volatile variable, a simple memory copy is enough.
    //
    if (Write) {
      CopyMem ((VOID *) StartAddress, Buffer, DataSize);
    } else {
      CopyMem (Buffer, (VOID *) StartAddress, DataSize);
    }

    return EFI_SUCCESS;
  }

  //
  // If data is non-volatile, calculate firmware volume header and data pointer.
  //
  Status = (EFI_STATUS) EsalCall (
                          EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_LO,
                          EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_HI,
                          GetPhysicalAddressFunctionId, 
                          Instance, 
                          (UINT64) &FvVolHdr, 
                          0, 
                          0, 
                          0, 
                          0, 
                          0
                          ).Status;
  ASSERT_EFI_ERROR (Status);

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) FvVolHdr);
  ASSERT (FwVolHeader != NULL);
  VariableStoreHeader = (VARIABLE_STORE_HEADER *)(FwVolHeader + 1);

  if ((StartAddress + DataSize) > ((EFI_PHYSICAL_ADDRESS) (UINTN) ((CHAR8 *)VariableStoreHeader + VariableStoreHeader->Size))) {
    return EFI_INVALID_PARAMETER;
  }
  
  LinearOffset  = (UINTN) FwVolHeader;
  CurrWritePtr  = StartAddress;
  CurrWriteSize = DataSize;
  CurrBuffer    = Buffer;
  LbaNumber     = 0;

  if (CurrWritePtr < LinearOffset) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Traverse data blocks of this firmware storage to find the one where CurrWritePtr locates
  //
  for (PtrBlockMapEntry = FwVolHeader->BlockMap; PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {
    for (BlockIndex = 0; BlockIndex < PtrBlockMapEntry->NumBlocks; BlockIndex++) {
      if ((CurrWritePtr >= LinearOffset) && (CurrWritePtr < LinearOffset + PtrBlockMapEntry->Length)) {
        //
        // Check to see if the data area to access spans multiple blocks.
        //
        if ((CurrWritePtr + CurrWriteSize) <= (LinearOffset + PtrBlockMapEntry->Length)) {
          //
          // If data area to access is contained in one block, just access and return.
          //
          if (Write) {
            Status = (EFI_STATUS) EsalCall (
                                    EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_LO,
                                    EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_HI,
                                    WriteFunctionId, 
                                    Instance, 
                                    LbaNumber, 
                                    (CurrWritePtr - LinearOffset), 
                                    (UINT64) &CurrWriteSize, 
                                    (UINT64) CurrBuffer, 
                                    0, 
                                    0
                                    ).Status;
          } else {
            Status = (EFI_STATUS) EsalCall (
                                    EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_LO,
                                    EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_HI,
                                    ReadFunctionId, 
                                    Instance, 
                                    LbaNumber, 
                                    (CurrWritePtr - LinearOffset), 
                                    (UINT64) &CurrWriteSize, 
                                    (UINT64) CurrBuffer, 
                                    0, 
                                    0
                                    ).Status;
          }
          return Status;
        } else {
          //
          // If data area to access spans multiple blocks, access this one and adjust for the next one.
          //
          Size = (UINT32) (LinearOffset + PtrBlockMapEntry->Length - CurrWritePtr);
          if (Write) {
            Status = (EFI_STATUS) EsalCall (
                                    EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_LO,
                                    EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_HI,
                                    WriteFunctionId, 
                                    Instance, 
                                    LbaNumber, 
                                    (CurrWritePtr - LinearOffset), 
                                    (UINT64) &Size, 
                                    (UINT64) CurrBuffer, 
                                    0, 
                                    0
                                    ).Status;
          } else {
            Status = (EFI_STATUS) EsalCall (
                                    EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_LO,
                                    EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_HI,
                                    ReadFunctionId, 
                                    Instance, 
                                    LbaNumber, 
                                    (CurrWritePtr - LinearOffset), 
                                    (UINT64) &Size, 
                                    (UINT64) CurrBuffer, 
                                    0, 
                                    0
                                    ).Status;
          }
          if (EFI_ERROR (Status)) {
            return Status;
          }
          //
          // Adjust for the remaining data.
          //
          CurrWritePtr  = LinearOffset + PtrBlockMapEntry->Length;
          CurrBuffer    = CurrBuffer + Size;
          CurrWriteSize = CurrWriteSize - Size;
        }
      }

      LinearOffset += PtrBlockMapEntry->Length;
      LbaNumber++;
    }
  }

  return EFI_SUCCESS;
}

/**
  Retrieves header of volatile or non-volatile variable stroage.

  @param[in]  VarStoreAddress    Start address of variable storage.
  @param[in]  Volatile           TRUE  - Variable storage is volatile.
                                 FALSE - Variable storage is non-volatile.
  @param[in]  Global             Pointer to VARAIBLE_GLOBAL structure.
  @param[in]  Instance           Instance of FV Block services.
  @param[out] VarStoreHeader     Pointer to VARIABLE_STORE_HEADER for output.

**/
VOID
GetVarStoreHeader (
  IN  EFI_PHYSICAL_ADDRESS   VarStoreAddress,
  IN  BOOLEAN                Volatile,
  IN  VARIABLE_GLOBAL        *Global,
  IN  UINTN                  Instance,
  OUT VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  EFI_STATUS            Status;

  Status = AccessVariableStore (
             FALSE,
             Global,
             Volatile,
             Instance,
             VarStoreAddress,
             sizeof (VARIABLE_STORE_HEADER),
             VarStoreHeader    
             );
  ASSERT_EFI_ERROR (Status);
}

/**
  Checks variable header.

  This function checks if variable header is valid or not.

  @param[in]  VariableAddress    Start address of variable header.
  @param[in]  Volatile           TRUE  - Variable is volatile.
                                 FALSE - Variable is non-volatile.
  @param[in]  Global             Pointer to VARAIBLE_GLOBAL structure.
  @param[in]  Instance           Instance of FV Block services.
  @param[out] VariableHeader     Pointer to VARIABLE_HEADER for output.

  @retval TRUE                   Variable header is valid.
  @retval FALSE                  Variable header is not valid.

**/
BOOLEAN
IsValidVariableHeader (
  IN  EFI_PHYSICAL_ADDRESS   VariableAddress,
  IN  BOOLEAN                Volatile,
  IN  VARIABLE_GLOBAL        *Global,
  IN  UINTN                  Instance,
  OUT VARIABLE_HEADER        *VariableHeader  OPTIONAL
  )
{
  EFI_STATUS            Status;
  VARIABLE_HEADER       LocalVariableHeader;

  Status = AccessVariableStore (
             FALSE,
             Global,
             Volatile,
             Instance,
             VariableAddress,
             sizeof (VARIABLE_HEADER),
             &LocalVariableHeader    
             );

  if (EFI_ERROR (Status) || LocalVariableHeader.StartId != VARIABLE_DATA) {
    return FALSE;
  }

  if (VariableHeader != NULL) {
    CopyMem (VariableHeader, &LocalVariableHeader, sizeof (VARIABLE_HEADER));
  }

  return TRUE;
}

/**
  Gets status of variable store.

  This function gets the current status of variable store.

  @param[in] VarStoreHeader  Pointer to header of variable store.

  @retval EfiRaw          Variable store status is raw.
  @retval EfiValid        Variable store status is valid.
  @retval EfiInvalid      Variable store status is invalid.

**/
VARIABLE_STORE_STATUS
GetVariableStoreStatus (
  IN VARIABLE_STORE_HEADER *VarStoreHeader
  )
{

  if (CompareGuid (&VarStoreHeader->Signature, &gEfiAuthenticatedVariableGuid) &&
      VarStoreHeader->Format == VARIABLE_STORE_FORMATTED &&
      VarStoreHeader->State == VARIABLE_STORE_HEALTHY
      ) {

    return EfiValid;
  } else if (((UINT32 *)(&VarStoreHeader->Signature))[0] == 0xffffffff &&
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
  Gets the size of variable name.

  This function gets the size of variable name.
  The variable is specified by its variable header.
  If variable header contains raw data, just return 0.

  @param[in] Variable  Pointer to the variable header.

  @return              Size of variable name in bytes.

**/
UINTN
NameSizeOfVariable (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if (Variable->State    == (UINT8) (-1) ||
      Variable->DataSize == (UINT32) -1 ||
      Variable->NameSize == (UINT32) -1 ||
      Variable->Attributes == (UINT32) -1) {
    return 0;
  }
  return (UINTN) Variable->NameSize;
}

/**
  Gets the size of variable data area.

  This function gets the size of variable data area.
  The variable is specified by its variable header.
  If variable header contains raw data, just return 0.

  @param[in]  Variable  Pointer to the variable header.

  @return               Size of variable data area in bytes.

**/
UINTN
DataSizeOfVariable (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if (Variable->State    == (UINT8)  -1 ||
      Variable->DataSize == (UINT32) -1 ||
      Variable->NameSize == (UINT32) -1 ||
      Variable->Attributes == (UINT32) -1) {
    return 0;
  }
  return (UINTN) Variable->DataSize;
}

/**
  Gets the pointer to variable name.

  This function gets the pointer to variable name.
  The variable is specified by its variable header.

  @param[in]  VariableAddress    Start address of variable header.
  @param[in]  Volatile           TRUE  - Variable is volatile.
                                 FALSE - Variable is non-volatile.
  @param[in]  Global             Pointer to VARAIBLE_GLOBAL structure.
  @param[in]  Instance           Instance of FV Block services.
  @param[out] VariableName       Buffer to hold variable name for output.

**/
VOID
GetVariableNamePtr (
  IN  EFI_PHYSICAL_ADDRESS   VariableAddress,
  IN  BOOLEAN                Volatile,
  IN  VARIABLE_GLOBAL        *Global,
  IN  UINTN                  Instance,
  OUT CHAR16                 *VariableName
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;
  VARIABLE_HEADER       VariableHeader;
  BOOLEAN               IsValid;

  IsValid = IsValidVariableHeader (VariableAddress, Volatile, Global, Instance, &VariableHeader);
  ASSERT (IsValid);

  //
  // Name area follows variable header.
  //
  Address = VariableAddress + sizeof (VARIABLE_HEADER);

  Status = AccessVariableStore (
             FALSE,
             Global,
             Volatile,
             Instance,
             Address,
             VariableHeader.NameSize,
             VariableName    
             );
  ASSERT_EFI_ERROR (Status);
}

/**
  Gets the pointer to variable data area.

  This function gets the pointer to variable data area.
  The variable is specified by its variable header.

  @param[in]  VariableAddress    Start address of variable header.
  @param[in]  Volatile           TRUE  - Variable is volatile.
                                 FALSE - Variable is non-volatile.
  @param[in]  Global             Pointer to VARAIBLE_GLOBAL structure.
  @param[in]  Instance           Instance of FV Block services.
  @param[out] VariableData       Buffer to hold variable data for output.

**/
VOID
GetVariableDataPtr (
  IN  EFI_PHYSICAL_ADDRESS   VariableAddress,
  IN  BOOLEAN                Volatile,
  IN  VARIABLE_GLOBAL        *Global,
  IN  UINTN                  Instance,
  OUT CHAR16                 *VariableData
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;
  VARIABLE_HEADER       VariableHeader;
  BOOLEAN               IsValid;

  IsValid = IsValidVariableHeader (VariableAddress, Volatile, Global, Instance, &VariableHeader);
  ASSERT (IsValid);

  //
  // Data area follows variable name.
  // Be careful about pad size for alignment
  //
  Address =  VariableAddress + sizeof (VARIABLE_HEADER);
  Address += NameSizeOfVariable (&VariableHeader);
  Address += GET_PAD_SIZE (NameSizeOfVariable (&VariableHeader));

  Status = AccessVariableStore (
             FALSE,
             Global,
             Volatile,
             Instance,
             Address,
             VariableHeader.DataSize,
             VariableData    
             );
  ASSERT_EFI_ERROR (Status);
}


/**
  Gets the pointer to the next variable header.

  This function gets the pointer to the next variable header.
  The variable is specified by its variable header.

  @param[in]  VariableAddress    Start address of variable header.
  @param[in]  Volatile           TRUE  - Variable is volatile.
                                 FALSE - Variable is non-volatile.
  @param[in]  Global             Pointer to VARAIBLE_GLOBAL structure.
  @param[in]  Instance           Instance of FV Block services.

  @return                        Pointer to the next variable header.
                                 NULL if variable header is invalid.

**/
EFI_PHYSICAL_ADDRESS
GetNextVariablePtr (
  IN  EFI_PHYSICAL_ADDRESS   VariableAddress,
  IN  BOOLEAN                Volatile,
  IN  VARIABLE_GLOBAL        *Global,
  IN  UINTN                  Instance
  )
{
  EFI_PHYSICAL_ADDRESS  Address;
  VARIABLE_HEADER       VariableHeader;

  if (!IsValidVariableHeader (VariableAddress, Volatile, Global, Instance, &VariableHeader)) {
    return 0x0;
  }

  //
  // Header of next variable follows data area of this variable
  //
  Address =  VariableAddress + sizeof (VARIABLE_HEADER);
  Address += NameSizeOfVariable (&VariableHeader);
  Address += GET_PAD_SIZE (NameSizeOfVariable (&VariableHeader));
  Address += DataSizeOfVariable (&VariableHeader);
  Address += GET_PAD_SIZE (DataSizeOfVariable (&VariableHeader));

  //
  // Be careful about pad size for alignment
  //
  return HEADER_ALIGN (Address);
}

/**
  Gets the pointer to the first variable header in given variable store area.

  This function gets the pointer to the first variable header in given variable 
  store area. The variable store area is given by its start address.

  @param[in] VarStoreHeaderAddress  Pointer to the header of variable store area.

  @return                           Pointer to the first variable header.

**/
EFI_PHYSICAL_ADDRESS
GetStartPointer (
  IN EFI_PHYSICAL_ADDRESS  VarStoreHeaderAddress
  )
{
  return HEADER_ALIGN (VarStoreHeaderAddress + sizeof (VARIABLE_STORE_HEADER));
}

/**
  Gets the pointer to the end of given variable store area.

  This function gets the pointer to the end of given variable store area.
  The variable store area is given by its start address.

  @param[in]  VarStoreHeaderAddress  Pointer to the header of variable store area.
  @param[in]  Volatile               TRUE  - Variable is volatile.
                                     FALSE - Variable is non-volatile.
  @param[in]  Global                 Pointer to VARAIBLE_GLOBAL structure.
  @param[in]  Instance               Instance of FV Block services.

  @return                            Pointer to the end of given variable store area.

**/
EFI_PHYSICAL_ADDRESS
GetEndPointer (
  IN EFI_PHYSICAL_ADDRESS  VarStoreHeaderAddress,
  IN  BOOLEAN              Volatile,
  IN  VARIABLE_GLOBAL      *Global,
  IN  UINTN                Instance
  )
{
  EFI_STATUS            Status;
  VARIABLE_STORE_HEADER VariableStoreHeader;

  Status = AccessVariableStore (
             FALSE,
             Global,
             Volatile,
             Instance,
             VarStoreHeaderAddress,
             sizeof (VARIABLE_STORE_HEADER),
             &VariableStoreHeader    
             );

  ASSERT_EFI_ERROR (Status);
  return HEADER_ALIGN (VarStoreHeaderAddress + VariableStoreHeader.Size);
}

/**
  Updates variable info entry in EFI system table for statistical information.

  Routine used to track statistical information about variable usage. 
  The data is stored in the EFI system table so it can be accessed later.
  VariableInfo.efi can dump out the table. Only Boot Services variable 
  accesses are tracked by this code. The PcdVariableCollectStatistics
  build flag controls if this feature is enabled. 
  A read that hits in the cache will have Read and Cache true for 
  the transaction. Data is allocated by this routine, but never
  freed.

  @param[in]  VariableName   Name of the Variable to track.
  @param[in]  VendorGuid     Guid of the Variable to track.
  @param[in]  Volatile       TRUE if volatile FALSE if non-volatile.
  @param[in]  Read           TRUE if GetVariable() was called.
  @param[in]  Write          TRUE if SetVariable() was called.
  @param[in]  Delete         TRUE if deleted via SetVariable().
  @param[in]  Cache          TRUE for a cache hit.

**/
VOID
UpdateVariableInfo (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  IN  BOOLEAN                 Volatile,
  IN  BOOLEAN                 Read,
  IN  BOOLEAN                 Write,
  IN  BOOLEAN                 Delete,
  IN  BOOLEAN                 Cache
  )
{
  VARIABLE_INFO_ENTRY   *Entry;

  if (FeaturePcdGet (PcdVariableCollectStatistics)) {

    if (EfiAtRuntime ()) {
      //
      // Don't collect statistics at runtime
      //
      return;
    }

    if (gVariableInfo == NULL) {
      //
      // on the first call allocate a entry and place a pointer to it in
      // the EFI System Table
      //
      gVariableInfo = AllocateZeroPool (sizeof (VARIABLE_INFO_ENTRY));
      ASSERT (gVariableInfo != NULL);

      CopyGuid (&gVariableInfo->VendorGuid, VendorGuid);
      gVariableInfo->Name = AllocatePool (StrSize (VariableName));
      ASSERT (gVariableInfo->Name != NULL);
      StrCpy (gVariableInfo->Name, VariableName);
      gVariableInfo->Volatile = Volatile;

      gBS->InstallConfigurationTable (&gEfiAuthenticatedVariableGuid, gVariableInfo);
    }

    
    for (Entry = gVariableInfo; Entry != NULL; Entry = Entry->Next) {
      if (CompareGuid (VendorGuid, &Entry->VendorGuid)) {
        if (StrCmp (VariableName, Entry->Name) == 0) {
          //
          // Find the entry matching both variable name and vender GUID,
          // and update counters for all types.
          //
          if (Read) {
            Entry->ReadCount++;
          }
          if (Write) {
            Entry->WriteCount++;
          }
          if (Delete) {
            Entry->DeleteCount++;
          }
          if (Cache) {
            Entry->CacheCount++;
          }

          return;
        }
      }

      if (Entry->Next == NULL) {
        //
        // If the entry is not in the table add it.
        // Next iteration of the loop will fill in the data
        //
        Entry->Next = AllocateZeroPool (sizeof (VARIABLE_INFO_ENTRY));
        ASSERT (Entry->Next != NULL);

        CopyGuid (&Entry->Next->VendorGuid, VendorGuid);
        Entry->Next->Name = AllocatePool (StrSize (VariableName));
        ASSERT (Entry->Next->Name != NULL);
        StrCpy (Entry->Next->Name, VariableName);
        Entry->Next->Volatile = Volatile;
      }

    }
  }
}

/**
  Updates variable in cache.

  This function searches the variable cache. If the variable to set exists in the cache,
  it updates the variable in cache. It has the same parameters with UEFI SetVariable()
  service.

  @param[in]  VariableName  A Null-terminated Unicode string that is the name of the vendor's
                            variable.  Each VariableName is unique for each VendorGuid.
  @param[in]  VendorGuid    A unique identifier for the vendor.
  @param[in]  Attributes    Attributes bitmask to set for the variable.
  @param[in]  DataSize      The size in bytes of the Data buffer.  A size of zero causes the
                            variable to be deleted.
  @param[in]  Data          The contents for the variable.

**/
VOID
UpdateVariableCache (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          *VendorGuid,
  IN      UINT32            Attributes,
  IN      UINTN             DataSize,
  IN      VOID              *Data
  )
{
  VARIABLE_CACHE_ENTRY      *Entry;
  UINTN                     Index;

  if (EfiAtRuntime ()) {
    //
    // Don't use the cache at runtime
    //
    return;
  }

  //
  // Searches cache for the variable to update. If it exists, update it.
  //
  for (Index = 0, Entry = mVariableCache; Index < sizeof (mVariableCache)/sizeof (VARIABLE_CACHE_ENTRY); Index++, Entry++) {
    if (CompareGuid (VendorGuid, Entry->Guid)) {
      if (StrCmp (VariableName, Entry->Name) == 0) { 
        Entry->Attributes = Attributes;
        if (DataSize == 0) {
          //
          // If DataSize is 0, delete the variable.
          //
          if (Entry->DataSize != 0) {
            FreePool (Entry->Data);
          }
          Entry->DataSize = DataSize;
        } else if (DataSize == Entry->DataSize) {
          //
          // If size of data does not change, simply copy data
          //
          CopyMem (Entry->Data, Data, DataSize);
        } else {
          //
          // If size of data changes, allocate pool and copy data.
          //
          Entry->Data = AllocatePool (DataSize);
          ASSERT (Entry->Data != NULL);
          Entry->DataSize = DataSize;
          CopyMem (Entry->Data, Data, DataSize);
        }
      }
    }
  }
}


/**
  Search the cache to check if the variable is in it.

  This function searches the variable cache. If the variable to find exists, return its data
  and attributes.

  @param[in]      VariableName   A Null-terminated Unicode string that is the name of the vendor's
                                 variable.  Each VariableName is unique for each VendorGuid.
  @param[in]      VendorGuid     A unique identifier for the vendor
  @param[out]     Attributes     Pointer to the attributes bitmask of the variable for output.
  @param[in, out] DataSize       On input, size of the buffer of Data.
                                 On output, size of the variable's data.
  @param[out]     Data           Pointer to the data buffer for output.

  @retval EFI_SUCCESS           VariableGuid & VariableName data was returned.
  @retval EFI_NOT_FOUND         No matching variable found in cache.
  @retval EFI_BUFFER_TOO_SMALL  *DataSize is smaller than size of the variable's data to return.

**/
EFI_STATUS
FindVariableInCache (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          *VendorGuid,
  OUT     UINT32            *Attributes OPTIONAL,
  IN OUT  UINTN             *DataSize,
  OUT     VOID              *Data
  )
{
  VARIABLE_CACHE_ENTRY      *Entry;
  UINTN                     Index;

  if (EfiAtRuntime ()) {
    //
    // Don't use the cache at runtime
    //
    return EFI_NOT_FOUND;
  }

  //
  // Searches cache for the variable
  //
  for (Index = 0, Entry = mVariableCache; Index < sizeof (mVariableCache)/sizeof (VARIABLE_CACHE_ENTRY); Index++, Entry++) {
    if (CompareGuid (VendorGuid, Entry->Guid)) {
      if (StrCmp (VariableName, Entry->Name) == 0) {
        if (Entry->DataSize == 0) {
          //
          // Variable has been deleted so return EFI_NOT_FOUND
          //
          return EFI_NOT_FOUND;
        } else if (Entry->DataSize > *DataSize) {
          //
          // If buffer is too small, return the size needed and EFI_BUFFER_TOO_SMALL
          //
          *DataSize = Entry->DataSize;
          return EFI_BUFFER_TOO_SMALL;
        } else {
          //
          // If buffer is large enough, return the data
          //
          *DataSize = Entry->DataSize;
          CopyMem (Data, Entry->Data, Entry->DataSize);
          //
          // If Attributes is not NULL, return the variable's attribute.
          //
          if (Attributes != NULL) {
            *Attributes = Entry->Attributes;
          }
          return EFI_SUCCESS;
        }
      }
    }
  }
  
  return EFI_NOT_FOUND;
}

/**
  Finds variable in volatile and non-volatile storage areas.

  This code finds variable in volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  Otherwise, VariableName and VendorGuid are compared.

  @param[in]  VariableName            Name of the variable to be found.
  @param[in]  VendorGuid              Vendor GUID to be found.
  @param[out] PtrTrack                VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.
  @param[in]  Global                  Pointer to VARIABLE_GLOBAL structure, including
                                      base of volatile variable storage area, base of
                                      NV variable storage area, and a lock.
  @param[in]  Instance                Instance of FV Block services.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL.
  @retval EFI_SUCCESS                 Variable successfully found.
  @retval EFI_INVALID_PARAMETER       Variable not found.

**/
EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global,
  IN  UINTN                   Instance
  )
{
  EFI_PHYSICAL_ADDRESS    Variable[2];
  EFI_PHYSICAL_ADDRESS    InDeletedVariable;
  EFI_PHYSICAL_ADDRESS    VariableStoreHeader[2];
  UINTN                   InDeletedStorageIndex;
  UINTN                   Index;
  CHAR16                  LocalVariableName[MAX_NAME_SIZE];
  BOOLEAN                 Volatile;
  VARIABLE_HEADER         VariableHeader;

  //
  // 0: Volatile, 1: Non-Volatile
  // The index and attributes mapping must be kept in this order as RuntimeServiceGetNextVariableName
  // make use of this mapping to implement search algorithme.
  //
  VariableStoreHeader[0]  = Global->VolatileVariableBase;
  VariableStoreHeader[1]  = Global->NonVolatileVariableBase;

  //
  // Start Pointers for the variable.
  // Actual Data Pointer where data can be written.
  //
  Variable[0] = GetStartPointer (VariableStoreHeader[0]);
  Variable[1] = GetStartPointer (VariableStoreHeader[1]);

  if (VariableName[0] != 0 && VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the variable by walk through volatile and then non-volatile variable store
  //
  InDeletedVariable     = 0x0;
  InDeletedStorageIndex = 0;
  Volatile = TRUE;
  for (Index = 0; Index < 2; Index++) {
    if (Index == 1) {
      Volatile = FALSE;
    }
    while (IsValidVariableHeader (Variable[Index], Volatile, Global, Instance, &VariableHeader)) {
      if (VariableHeader.State == VAR_ADDED || 
          VariableHeader.State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)
         ) {
        if (!EfiAtRuntime () || ((VariableHeader.Attributes & EFI_VARIABLE_RUNTIME_ACCESS) != 0)) {
          if (VariableName[0] == 0) {
            //
            // If VariableName is an empty string, then we just find the first qualified variable
            // without comparing VariableName and VendorGuid
            //
            if (VariableHeader.State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
              //
              // If variable is in delete transition, record it.
              //
              InDeletedVariable     = Variable[Index];
              InDeletedStorageIndex = Index;
            } else {
              //
              // If variable is not in delete transition, return it.
              //
              PtrTrack->StartPtr  = GetStartPointer (VariableStoreHeader[Index]);
              PtrTrack->EndPtr    = GetEndPointer (VariableStoreHeader[Index], Volatile, Global, Instance);
              PtrTrack->CurrPtr   = Variable[Index];
              PtrTrack->Volatile  = Volatile;

              return EFI_SUCCESS;
            }
          } else {
            //
            // If VariableName is not an empty string, then VariableName and VendorGuid are compared.
            //
            if (CompareGuid (VendorGuid, &VariableHeader.VendorGuid)) {
              GetVariableNamePtr (
                Variable[Index],
                Volatile,
                Global,
                Instance,
                LocalVariableName
                );

              ASSERT (NameSizeOfVariable (&VariableHeader) != 0);
              if (CompareMem (VariableName, LocalVariableName, NameSizeOfVariable (&VariableHeader)) == 0) {
                if (VariableHeader.State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
                  //
                  // If variable is in delete transition, record it.
                  // We will use if only no VAR_ADDED variable is found.
                  //
                  InDeletedVariable     = Variable[Index];
                  InDeletedStorageIndex = Index;
                } else {
                  //
                  // If variable is not in delete transition, return it.
                  //
                  PtrTrack->StartPtr  = GetStartPointer (VariableStoreHeader[Index]);
                  PtrTrack->EndPtr    = GetEndPointer (VariableStoreHeader[Index], Volatile, Global, Instance);
                  PtrTrack->CurrPtr   = Variable[Index];
                  PtrTrack->Volatile  = Volatile;

                  return EFI_SUCCESS;
                }
              }
            }
          }
        }
      }

      Variable[Index] = GetNextVariablePtr (
                          Variable[Index],
                          Volatile,
                          Global,
                          Instance
                          );
    }
    if (InDeletedVariable != 0x0) {
      //
      // If no VAR_ADDED variable is found, and only variable in delete transition, then use this one.
      //
      PtrTrack->StartPtr  = GetStartPointer (VariableStoreHeader[InDeletedStorageIndex]);
      PtrTrack->EndPtr    = GetEndPointer (
                              VariableStoreHeader[InDeletedStorageIndex],
                              (BOOLEAN)(InDeletedStorageIndex == 0),
                              Global,
                              Instance
                              );
      PtrTrack->CurrPtr   = InDeletedVariable;
      PtrTrack->Volatile  = (BOOLEAN)(InDeletedStorageIndex == 0);
      return EFI_SUCCESS;
    }
  }
  PtrTrack->CurrPtr = 0x0;
  return EFI_NOT_FOUND;
}

/**
  Variable store garbage collection and reclaim operation.

  @param[in]  VariableBase        Base address of variable store area.
  @param[out] LastVariableOffset  Offset of last variable.
  @param[in]  IsVolatile          The variable store is volatile or not,
                                  if it is non-volatile, need FTW.
  @param[in]  VirtualMode         Current calling mode for this function.
  @param[in]  Global              Context of this Extended SAL Variable Services Class call.
  @param[in]  UpdatingVariable    Pointer to header of the variable that is being updated.

  @retval EFI_SUCCESS             Variable store successfully reclaimed.
  @retval EFI_OUT_OF_RESOURCES    Fail to allocate memory buffer to hold all valid variables.

**/
EFI_STATUS
Reclaim (
  IN  EFI_PHYSICAL_ADDRESS  VariableBase,
  OUT UINTN                 *LastVariableOffset,
  IN  BOOLEAN               IsVolatile,
  IN  BOOLEAN               VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL  *Global,
  IN  EFI_PHYSICAL_ADDRESS  UpdatingVariable
  )
{
  EFI_PHYSICAL_ADDRESS  Variable;
  EFI_PHYSICAL_ADDRESS  AddedVariable;
  EFI_PHYSICAL_ADDRESS  NextVariable;
  EFI_PHYSICAL_ADDRESS  NextAddedVariable;
  VARIABLE_STORE_HEADER VariableStoreHeader;
  VARIABLE_HEADER       VariableHeader;
  VARIABLE_HEADER       AddedVariableHeader;
  CHAR16                VariableName[MAX_NAME_SIZE];
  CHAR16                AddedVariableName[MAX_NAME_SIZE];
  UINT8                 *ValidBuffer;
  UINTN                 MaximumBufferSize;
  UINTN                 VariableSize;
  UINTN                 NameSize;
  UINT8                 *CurrPtr;
  BOOLEAN               FoundAdded;
  EFI_STATUS            Status;
  VARIABLE_GLOBAL       *VariableGlobal;
  UINT32                Instance;

  VariableGlobal = &Global->VariableGlobal[VirtualMode];
  Instance = Global->FvbInstance;

  GetVarStoreHeader (VariableBase, IsVolatile, VariableGlobal, Instance, &VariableStoreHeader);
  //
  // recaluate the total size of Common/HwErr type variables in non-volatile area.
  //
  if (!IsVolatile) {
    Global->CommonVariableTotalSize = 0;
    Global->HwErrVariableTotalSize  = 0;
  }

  //
  // Calculate the size of buffer needed to gather all valid variables
  //
  Variable          = GetStartPointer (VariableBase);
  MaximumBufferSize = sizeof (VARIABLE_STORE_HEADER);

  while (IsValidVariableHeader (Variable, IsVolatile, VariableGlobal, Instance, &VariableHeader)) {
    NextVariable = GetNextVariablePtr (Variable, IsVolatile, VariableGlobal, Instance);
    //
    // Collect VAR_ADDED variables, and variables in delete transition status.
    //
    if (VariableHeader.State == VAR_ADDED || 
        VariableHeader.State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)
       ) {
      VariableSize = NextVariable - Variable;
      MaximumBufferSize += VariableSize;
    }

    Variable = NextVariable;
  }

  //
  // Reserve the 1 Bytes with Oxff to identify the 
  // end of the variable buffer. 
  // 
  MaximumBufferSize += 1;
  ValidBuffer = AllocatePool (MaximumBufferSize);
  if (ValidBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (ValidBuffer, MaximumBufferSize, 0xff);

  //
  // Copy variable store header
  //
  CopyMem (ValidBuffer, &VariableStoreHeader, sizeof (VARIABLE_STORE_HEADER));
  CurrPtr = (UINT8 *) GetStartPointer ((EFI_PHYSICAL_ADDRESS) ValidBuffer);

  //
  // Reinstall all ADDED variables
  // 
  Variable = GetStartPointer (VariableBase);
  while (IsValidVariableHeader (Variable, IsVolatile, VariableGlobal, Instance, &VariableHeader)) {
    NextVariable = GetNextVariablePtr (Variable, IsVolatile, VariableGlobal, Instance);
    if (VariableHeader.State == VAR_ADDED) {
      VariableSize = NextVariable - Variable;
      CopyMem (CurrPtr, (UINT8 *) Variable, VariableSize);
      CurrPtr += VariableSize;
      if ((!IsVolatile) && ((((VARIABLE_HEADER*)Variable)->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
        Global->HwErrVariableTotalSize += VariableSize;
      } else if ((!IsVolatile) && ((((VARIABLE_HEADER*)Variable)->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
        Global->CommonVariableTotalSize += VariableSize;
      }
    }
    Variable = NextVariable;
  }
  //
  // Reinstall in delete transition variables
  // 
  Variable = GetStartPointer (VariableBase);
  while (IsValidVariableHeader (Variable, IsVolatile, VariableGlobal, Instance, &VariableHeader)) {
    NextVariable = GetNextVariablePtr (Variable, IsVolatile, VariableGlobal, Instance);
    if (VariableHeader.State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {

      //
      // Buffer has cached all ADDED variable. 
      // Per IN_DELETED variable, we have to guarantee that
      // no ADDED one in previous buffer. 
      // 
      FoundAdded = FALSE;
      AddedVariable = GetStartPointer ((EFI_PHYSICAL_ADDRESS) ValidBuffer);
      while (IsValidVariableHeader (AddedVariable, IsVolatile, VariableGlobal, Instance, &AddedVariableHeader)) {
        NextAddedVariable = GetNextVariablePtr (AddedVariable, IsVolatile, VariableGlobal, Instance);
        NameSize = NameSizeOfVariable (&AddedVariableHeader);
        if (CompareGuid (&AddedVariableHeader.VendorGuid, &VariableHeader.VendorGuid) &&
            NameSize == NameSizeOfVariable (&VariableHeader)
           ) {
          GetVariableNamePtr (Variable, IsVolatile, VariableGlobal, Instance, VariableName);
          GetVariableNamePtr (AddedVariable, IsVolatile, VariableGlobal, Instance, AddedVariableName);
          if (CompareMem (VariableName, AddedVariableName, NameSize) == 0) {
            //
            // If ADDED variable with the same name and vender GUID has been reinstalled,
            // then discard this IN_DELETED copy.
            //
            FoundAdded = TRUE;
            break;
          }
        }
        AddedVariable = NextAddedVariable;
      }
      //
      // Add IN_DELETE variables that have not been added to buffer
      //
      if (!FoundAdded) {
        VariableSize = NextVariable - Variable;
        CopyMem (CurrPtr, (UINT8 *) Variable, VariableSize);
        if (Variable != UpdatingVariable) {
          //
          // Make this IN_DELETE instance valid if:
          // 1. No valid instance of this variable exists.
          // 2. It is not the variable that is going to be updated.
          //
          ((VARIABLE_HEADER *) CurrPtr)->State = VAR_ADDED;
        }
        CurrPtr += VariableSize;
        if ((!IsVolatile) && ((((VARIABLE_HEADER*)Variable)->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
          Global->HwErrVariableTotalSize += VariableSize;
        } else if ((!IsVolatile) && ((((VARIABLE_HEADER*)Variable)->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
          Global->CommonVariableTotalSize += VariableSize;
        }
      }
    }
    Variable = NextVariable;
  }

  if (IsVolatile) {
    //
    // If volatile variable store, just copy valid buffer
    //
    SetMem ((UINT8 *) (UINTN) VariableBase, VariableStoreHeader.Size, 0xff);
    CopyMem ((UINT8 *) (UINTN) VariableBase, ValidBuffer, (UINTN) (CurrPtr - (UINT8 *) ValidBuffer));
    Status = EFI_SUCCESS;
  } else {
    //
    // If non-volatile variable store, perform FTW here.
    // Write ValidBuffer to destination specified by VariableBase.
    //
    Status = FtwVariableSpace (
               VariableBase,
               ValidBuffer,
               (UINTN) (CurrPtr - (UINT8 *) ValidBuffer)
               );
  }
  if (!EFI_ERROR (Status)) {
    *LastVariableOffset = (UINTN) (CurrPtr - (UINT8 *) ValidBuffer);
  } else {
    *LastVariableOffset = 0;
  }

  FreePool (ValidBuffer);

  return Status;
}

/**
  Get index from supported language codes according to language string.

  This code is used to get corresponding index in supported language codes. It can handle
  RFC4646 and ISO639 language tags.
  In ISO639 language tags, take 3-characters as a delimitation to find matched string and calculate the index.
  In RFC4646 language tags, take semicolon as a delimitation to find matched string and calculate the index.

  For example:
    SupportedLang  = "engfraengfra"
    Lang           = "eng"
    Iso639Language = TRUE
  The return value is "0".
  Another example:
    SupportedLang  = "en;fr;en-US;fr-FR"
    Lang           = "fr-FR"
    Iso639Language = FALSE
  The return value is "3".

  @param[in]  SupportedLang          Platform supported language codes.
  @param[in]  Lang                   Configured language.
  @param[in]  Iso639Language         A bool value to signify if the handler is operated on ISO639 or RFC4646.

  @return                            The index of language in the language codes.

**/
UINTN
GetIndexFromSupportedLangCodes(
  IN  CHAR8            *SupportedLang,
  IN  CHAR8            *Lang,
  IN  BOOLEAN          Iso639Language
  ) 
{
  UINTN    Index;
  UINTN    CompareLength;
  UINTN    LanguageLength;

  if (Iso639Language) {
    CompareLength = ISO_639_2_ENTRY_SIZE;
    for (Index = 0; Index < AsciiStrLen (SupportedLang); Index += CompareLength) {
      if (AsciiStrnCmp (Lang, SupportedLang + Index, CompareLength) == 0) {
        //
        // Successfully find the index of Lang string in SupportedLang string.
        //
        Index = Index / CompareLength;
        return Index;
      }
    }
    ASSERT (FALSE);
    return 0;
  } else {
    //
    // Compare RFC4646 language code
    //
    Index = 0;
    for (LanguageLength = 0; Lang[LanguageLength] != '\0'; LanguageLength++);

    for (Index = 0; *SupportedLang != '\0'; Index++, SupportedLang += CompareLength) {
      //
      // Skip ';' characters in SupportedLang
      //
      for (; *SupportedLang != '\0' && *SupportedLang == ';'; SupportedLang++);
      //
      // Determine the length of the next language code in SupportedLang
      //
      for (CompareLength = 0; SupportedLang[CompareLength] != '\0' && SupportedLang[CompareLength] != ';'; CompareLength++);
      
      if ((CompareLength == LanguageLength) && 
          (AsciiStrnCmp (Lang, SupportedLang, CompareLength) == 0)) {
        //
        // Successfully find the index of Lang string in SupportedLang string.
        //
        return Index;
      }
    }
    ASSERT (FALSE);
    return 0;
  }
}

/**
  Get language string from supported language codes according to index.

  This code is used to get corresponding language string in supported language codes. It can handle
  RFC4646 and ISO639 language tags.
  In ISO639 language tags, take 3-characters as a delimitation. Find language string according to the index.
  In RFC4646 language tags, take semicolon as a delimitation. Find language string according to the index.

  For example:
    SupportedLang  = "engfraengfra"
    Index          = "1"
    Iso639Language = TRUE
  The return value is "fra".
  Another example:
    SupportedLang  = "en;fr;en-US;fr-FR"
    Index          = "1"
    Iso639Language = FALSE
  The return value is "fr".

  @param[in]  SupportedLang   Platform supported language codes.
  @param[in]  Index           the index in supported language codes.
  @param[in]  Iso639Language  A bool value to signify if the handler is operated on ISO639 or RFC4646.
  @param[in]  VirtualMode     Current calling mode for this function.
  @param[in]  Global          Context of this Extended SAL Variable Services Class call.

  @return                     The language string in the language codes.

**/
CHAR8 *
GetLangFromSupportedLangCodes (
  IN  CHAR8                 *SupportedLang,
  IN  UINTN                 Index,
  IN  BOOLEAN               Iso639Language,
  IN  BOOLEAN               VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL  *Global
  )
{
  UINTN    SubIndex;
  UINTN    CompareLength;
  CHAR8    *Supported;

  SubIndex  = 0;
  Supported = SupportedLang;
  if (Iso639Language) {
    //
    // according to the index of Lang string in SupportedLang string to get the language.
    // As this code will be invoked in RUNTIME, therefore there is not memory allocate/free operation.
    // In driver entry, it pre-allocates a runtime attribute memory to accommodate this string.
    //
    CompareLength = ISO_639_2_ENTRY_SIZE;
    Global->Lang[CompareLength] = '\0';
    return CopyMem (Global->Lang, SupportedLang + Index * CompareLength, CompareLength);

  } else {
    while (TRUE) {
      //
      // take semicolon as delimitation, sequentially traverse supported language codes.
      //
      for (CompareLength = 0; *Supported != ';' && *Supported != '\0'; CompareLength++) {
        Supported++;
      }
      if ((*Supported == '\0') && (SubIndex != Index)) {
        //
        // Have completed the traverse, but not find corrsponding string.
        // This case is not allowed to happen.
        //
        ASSERT(FALSE);
        return NULL;
      }
      if (SubIndex == Index) {
        //
        // according to the index of Lang string in SupportedLang string to get the language.
        // As this code will be invoked in RUNTIME, therefore there is not memory allocate/free operation.
        // In driver entry, it pre-allocates a runtime attribute memory to accommodate this string.
        //
        Global->PlatformLang[VirtualMode][CompareLength] = '\0';
        return CopyMem (Global->PlatformLang[VirtualMode], Supported - CompareLength, CompareLength);
      }
      SubIndex++;

      //
      // Skip ';' characters in Supported
      //
      for (; *Supported != '\0' && *Supported == ';'; Supported++);
    }
  }
}

/**
  Returns a pointer to an allocated buffer that contains the best matching language 
  from a set of supported languages.  
  
  This function supports both ISO 639-2 and RFC 4646 language codes, but language 
  code types may not be mixed in a single call to this function. This function
  supports a variable argument list that allows the caller to pass in a prioritized
  list of language codes to test against all the language codes in SupportedLanguages.

  If SupportedLanguages is NULL, then ASSERT().

  @param[in]  SupportedLanguages  A pointer to a Null-terminated ASCII string that
                                  contains a set of language codes in the format 
                                  specified by Iso639Language.
  @param[in]  Iso639Language      If TRUE, then all language codes are assumed to be
                                  in ISO 639-2 format.  If FALSE, then all language
                                  codes are assumed to be in RFC 4646 language format.
  @param[in]  VirtualMode         Current calling mode for this function.
  @param[in]  ...                 A variable argument list that contains pointers to 
                                  Null-terminated ASCII strings that contain one or more
                                  language codes in the format specified by Iso639Language.
                                  The first language code from each of these language
                                  code lists is used to determine if it is an exact or
                                  close match to any of the language codes in 
                                  SupportedLanguages.  Close matches only apply to RFC 4646
                                  language codes, and the matching algorithm from RFC 4647
                                  is used to determine if a close match is present.  If 
                                  an exact or close match is found, then the matching
                                  language code from SupportedLanguages is returned.  If
                                  no matches are found, then the next variable argument
                                  parameter is evaluated.  The variable argument list 
                                  is terminated by a NULL.

  @retval NULL   The best matching language could not be found in SupportedLanguages.
  @retval NULL   There are not enough resources available to return the best matching 
                 language.
  @retval Other  A pointer to a Null-terminated ASCII string that is the best matching 
                 language in SupportedLanguages.

**/
CHAR8 *
VariableGetBestLanguage (
  IN CONST CHAR8  *SupportedLanguages, 
  IN BOOLEAN      Iso639Language,
  IN BOOLEAN      VirtualMode,
  ...
  )
{
  VA_LIST      Args;
  CHAR8        *Language;
  UINTN        CompareLength;
  UINTN        LanguageLength;
  CONST CHAR8  *Supported;
  CHAR8        *Buffer;

  ASSERT (SupportedLanguages != NULL);

  VA_START (Args, VirtualMode);
  while ((Language = VA_ARG (Args, CHAR8 *)) != NULL) {
    //
    // Default to ISO 639-2 mode
    //
    CompareLength  = 3;
    LanguageLength = MIN (3, AsciiStrLen (Language));

    //
    // If in RFC 4646 mode, then determine the length of the first RFC 4646 language code in Language
    //
    if (!Iso639Language) {
      for (LanguageLength = 0; Language[LanguageLength] != 0 && Language[LanguageLength] != ';'; LanguageLength++);
    }

    //
    // Trim back the length of Language used until it is empty
    //
    while (LanguageLength > 0) {
      //
      // Loop through all language codes in SupportedLanguages
      //
      for (Supported = SupportedLanguages; *Supported != '\0'; Supported += CompareLength) {
        //
        // In RFC 4646 mode, then Loop through all language codes in SupportedLanguages
        //
        if (!Iso639Language) {
          //
          // Skip ';' characters in Supported
          //
          for (; *Supported != '\0' && *Supported == ';'; Supported++);
          //
          // Determine the length of the next language code in Supported
          //
          for (CompareLength = 0; Supported[CompareLength] != 0 && Supported[CompareLength] != ';'; CompareLength++);
          //
          // If Language is longer than the Supported, then skip to the next language
          //
          if (LanguageLength > CompareLength) {
            continue;
          }
        }
        //
        // See if the first LanguageLength characters in Supported match Language
        //
        if (AsciiStrnCmp (Supported, Language, LanguageLength) == 0) {
          VA_END (Args);

          Buffer = Iso639Language ? mVariableModuleGlobal->Lang : mVariableModuleGlobal->PlatformLang[VirtualMode];
          Buffer[CompareLength] = '\0';
          return CopyMem (Buffer, Supported, CompareLength);
        }
      }

      if (Iso639Language) {
        //
        // If ISO 639 mode, then each language can only be tested once
        //
        LanguageLength = 0;
      } else {
        //
        // If RFC 4646 mode, then trim Language from the right to the next '-' character 
        //
        for (LanguageLength--; LanguageLength > 0 && Language[LanguageLength] != '-'; LanguageLength--);
      }
    }
  }
  VA_END (Args);

  //
  // No matches were found 
  //
  return NULL;
}

/**
  Hook the operations in PlatformLangCodes, LangCodes, PlatformLang and Lang.

  When setting Lang/LangCodes, simultaneously update PlatformLang/PlatformLangCodes.
  According to UEFI spec, PlatformLangCodes/LangCodes are only set once in firmware initialization,
  and are read-only. Therefore, in variable driver, only store the original value for other use.

  @param[in] VariableName  Name of variable.
  @param[in] Data          Variable data.
  @param[in] DataSize      Size of data. 0 means delete.
  @param[in] VirtualMode   Current calling mode for this function.
  @param[in] Global        Context of this Extended SAL Variable Services Class call.

**/
VOID
AutoUpdateLangVariable(
  IN  CHAR16                *VariableName,
  IN  VOID                  *Data,
  IN  UINTN                 DataSize,
  IN  BOOLEAN               VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL  *Global
  )
{
  EFI_STATUS              Status;
  CHAR8                   *BestPlatformLang;
  CHAR8                   *BestLang;
  UINTN                   Index;
  UINT32                  Attributes;
  VARIABLE_POINTER_TRACK  Variable;
  BOOLEAN                 SetLanguageCodes;
  CHAR16                  **PredefinedVariableName;
  VARIABLE_GLOBAL         *VariableGlobal;
  UINT32                  Instance;

  //
  // Don't do updates for delete operation
  //
  if (DataSize == 0) {
    return;
  }

  SetLanguageCodes = FALSE;
  VariableGlobal   = &Global->VariableGlobal[VirtualMode];
  Instance         = Global->FvbInstance;


  PredefinedVariableName = &Global->VariableName[VirtualMode][0];
  if (StrCmp (VariableName, PredefinedVariableName[VAR_PLATFORM_LANG_CODES]) == 0) {
    //
    // PlatformLangCodes is a volatile variable, so it can not be updated at runtime.
    //
    if (EfiAtRuntime ()) {
      return;
    }

    SetLanguageCodes = TRUE;

    //
    // According to UEFI spec, PlatformLangCodes is only set once in firmware initialization, and is read-only
    // Therefore, in variable driver, only store the original value for other use.
    //
    if (Global->PlatformLangCodes[VirtualMode] != NULL) {
      FreePool (Global->PlatformLangCodes[VirtualMode]);
    }
    Global->PlatformLangCodes[VirtualMode] = AllocateRuntimeCopyPool (DataSize, Data);
    ASSERT (mVariableModuleGlobal->PlatformLangCodes[VirtualMode] != NULL);

    //
    // PlatformLang holds a single language from PlatformLangCodes, 
    // so the size of PlatformLangCodes is enough for the PlatformLang.
    //
    if (Global->PlatformLang[VirtualMode] != NULL) {
      FreePool (Global->PlatformLang[VirtualMode]);
    }
    Global->PlatformLang[VirtualMode] = AllocateRuntimePool (DataSize);
    ASSERT (Global->PlatformLang[VirtualMode] != NULL);

  } else if (StrCmp (VariableName, PredefinedVariableName[VAR_LANG_CODES]) == 0) {
    //
    // LangCodes is a volatile variable, so it can not be updated at runtime.
    //
    if (EfiAtRuntime ()) {
      return;
    }

    SetLanguageCodes = TRUE;

    //
    // According to UEFI spec, LangCodes is only set once in firmware initialization, and is read-only
    // Therefore, in variable driver, only store the original value for other use.
    //
    if (Global->LangCodes[VirtualMode] != NULL) {
      FreePool (Global->LangCodes[VirtualMode]);
    }
    Global->LangCodes[VirtualMode] = AllocateRuntimeCopyPool (DataSize, Data);
    ASSERT (Global->LangCodes[VirtualMode] != NULL);
  }

  if (SetLanguageCodes 
      && (Global->PlatformLangCodes[VirtualMode] != NULL)
      && (Global->LangCodes[VirtualMode] != NULL)) {
    //
    // Update Lang if PlatformLang is already set
    // Update PlatformLang if Lang is already set
    //
    Status = FindVariable (PredefinedVariableName[VAR_PLATFORM_LANG], Global->GlobalVariableGuid[VirtualMode], &Variable, VariableGlobal, Instance);
    if (!EFI_ERROR (Status)) {
      //
      // Update Lang
      //
      VariableName = PredefinedVariableName[VAR_PLATFORM_LANG];
    } else {
      Status = FindVariable (PredefinedVariableName[VAR_LANG], Global->GlobalVariableGuid[VirtualMode], &Variable, VariableGlobal, Instance);
      if (!EFI_ERROR (Status)) {
        //
        // Update PlatformLang
        //
        VariableName = PredefinedVariableName[VAR_LANG];
      } else {
        //
        // Neither PlatformLang nor Lang is set, directly return
        //
        return;
      }
    }
    Data    = (VOID *) GetEndPointer (VariableGlobal->VolatileVariableBase, TRUE, VariableGlobal, Instance);
    GetVariableDataPtr ((EFI_PHYSICAL_ADDRESS) Variable.CurrPtr, Variable.Volatile, VariableGlobal, Instance, (CHAR16 *) Data);

    Status = AccessVariableStore (
               FALSE,
               VariableGlobal,
               Variable.Volatile,
               Instance,
               (UINTN) &(((VARIABLE_HEADER *)Variable.CurrPtr)->DataSize),
               sizeof (DataSize),
               &DataSize
               ); 
    ASSERT_EFI_ERROR (Status);
  }

  //
  // According to UEFI spec, "Lang" and "PlatformLang" is NV|BS|RT attributions.
  //
  Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

  if (StrCmp (VariableName, PredefinedVariableName[VAR_PLATFORM_LANG]) == 0) {
    //
    // Update Lang when PlatformLangCodes/LangCodes were set.
    //
    if ((Global->PlatformLangCodes[VirtualMode] != NULL) && (Global->LangCodes[VirtualMode] != NULL)) {
      //
      // When setting PlatformLang, firstly get most matched language string from supported language codes.
      //
      BestPlatformLang = VariableGetBestLanguage (Global->PlatformLangCodes[VirtualMode], FALSE, VirtualMode, Data, NULL);
      if (BestPlatformLang != NULL) {
        //
        // Get the corresponding index in language codes.
        //
        Index = GetIndexFromSupportedLangCodes (Global->PlatformLangCodes[VirtualMode], BestPlatformLang, FALSE);

        //
        // Get the corresponding ISO639 language tag according to RFC4646 language tag.
        //
        BestLang = GetLangFromSupportedLangCodes (Global->LangCodes[VirtualMode], Index, TRUE, VirtualMode, Global);

        //
        // Successfully convert PlatformLang to Lang, and set the BestLang value into Lang variable simultaneously.
        //
        FindVariable (PredefinedVariableName[VAR_LANG], Global->GlobalVariableGuid[VirtualMode], &Variable, VariableGlobal, Instance);

        Status = UpdateVariable (
                   PredefinedVariableName[VAR_LANG],
                   Global->GlobalVariableGuid[VirtualMode],
                   BestLang,
                   ISO_639_2_ENTRY_SIZE + 1,
                   Attributes,
                   0,
                   0,
                   VirtualMode,
                   Global,
                   &Variable
                   );

        DEBUG ((EFI_D_INFO, "Variable Driver Auto Update PlatformLang, PlatformLang:%a, Lang:%a\n", BestPlatformLang, BestLang));

        ASSERT_EFI_ERROR (Status);
      }
    }

  } else if (StrCmp (VariableName, PredefinedVariableName[VAR_LANG]) == 0) {
    //
    // Update PlatformLang when PlatformLangCodes/LangCodes were set.
    //
    if ((Global->PlatformLangCodes[VirtualMode] != NULL) && (Global->LangCodes[VirtualMode] != NULL)) {
      //
      // When setting Lang, firstly get most matched language string from supported language codes.
      //
      BestLang = VariableGetBestLanguage (Global->LangCodes[VirtualMode], TRUE, VirtualMode, Data, NULL);
      if (BestLang != NULL) {
        //
        // Get the corresponding index in language codes.
        //
        Index = GetIndexFromSupportedLangCodes (Global->LangCodes[VirtualMode], BestLang, TRUE);

        //
        // Get the corresponding RFC4646 language tag according to ISO639 language tag.
        //
        BestPlatformLang = GetLangFromSupportedLangCodes (Global->PlatformLangCodes[VirtualMode], Index, FALSE, VirtualMode, Global);

        //
        // Successfully convert Lang to PlatformLang, and set the BestPlatformLang value into PlatformLang variable simultaneously.
        //
        FindVariable (PredefinedVariableName[VAR_PLATFORM_LANG], Global->GlobalVariableGuid[VirtualMode], &Variable, VariableGlobal, Instance);

        Status = UpdateVariable (
                   PredefinedVariableName[VAR_PLATFORM_LANG], 
                   Global->GlobalVariableGuid[VirtualMode], 
                   BestPlatformLang, 
                   AsciiStrSize (BestPlatformLang), 
                   Attributes, 
                   0,
                   0,
                   VirtualMode, 
                   Global, 
                   &Variable
                   );

        DEBUG ((EFI_D_INFO, "Variable Driver Auto Update Lang, Lang:%a, PlatformLang:%a\n", BestLang, BestPlatformLang));
        ASSERT_EFI_ERROR (Status);
      }
    }
  }
}

/**
  Update the variable region with Variable information. These are the same 
  arguments as the EFI Variable services.

  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Data               Variable data.
  @param[in] DataSize           Size of data. 0 means delete.
  @param[in] Attributes         Attributes of the variable.
  @param[in] KeyIndex           Index of associated public key.
  @param[in] MonotonicCount     Value of associated monotonic count. 
  @param[in] VirtualMode        Current calling mode for this function.
  @param[in] Global             Context of this Extended SAL Variable Services Class call.
  @param[in] Variable           The variable information which is used to keep track of variable usage.

  @retval EFI_SUCCESS           The update operation is success.
  @retval EFI_OUT_OF_RESOURCES  Variable region is full, can not write other data into this region.

**/
EFI_STATUS
EFIAPI
UpdateVariable (
  IN      CHAR16                  *VariableName,
  IN      EFI_GUID                *VendorGuid,
  IN      VOID                    *Data,
  IN      UINTN                   DataSize,
  IN      UINT32                  Attributes OPTIONAL,  
  IN      UINT32                  KeyIndex  OPTIONAL,
  IN      UINT64                  MonotonicCount  OPTIONAL,
  IN      BOOLEAN                 VirtualMode,
  IN      ESAL_VARIABLE_GLOBAL    *Global,
  IN      VARIABLE_POINTER_TRACK  *Variable
  )
{
  EFI_STATUS                          Status;
  VARIABLE_HEADER                     *NextVariable;
  UINTN                               VarNameOffset;
  UINTN                               VarDataOffset;
  UINTN                               VarNameSize;
  UINTN                               VarSize;
  BOOLEAN                             Volatile;
  UINT8                               State;
  VARIABLE_HEADER                     VariableHeader;
  VARIABLE_HEADER                     *NextVariableHeader;
  BOOLEAN                             Valid;
  BOOLEAN                             Reclaimed;
  VARIABLE_STORE_HEADER               VariableStoreHeader;
  UINTN                               ScratchSize;
  VARIABLE_GLOBAL                     *VariableGlobal;
  UINT32                              Instance;

  VariableGlobal = &Global->VariableGlobal[VirtualMode];
  Instance = Global->FvbInstance;

  Reclaimed  = FALSE;

  if (Variable->CurrPtr != 0) {

    Valid = IsValidVariableHeader (Variable->CurrPtr, Variable->Volatile, VariableGlobal, Instance, &VariableHeader);
    if (!Valid) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }

    //
    // Update/Delete existing variable
    //
    Volatile = Variable->Volatile;
    
    if (EfiAtRuntime ()) {        
      //
      // If EfiAtRuntime and the variable is Volatile and Runtime Access,  
      // the volatile is ReadOnly, and SetVariable should be aborted and 
      // return EFI_WRITE_PROTECTED.
      //
      if (Variable->Volatile) {
        Status = EFI_WRITE_PROTECTED;
        goto Done;
      }
      //
      // Only variable have NV attribute can be updated/deleted in Runtime
      //
      if ((VariableHeader.Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;      
      }
    }
    //
    // Setting a data variable with no access, or zero DataSize attributes
    // specified causes it to be deleted.
    //
    if (DataSize == 0 || (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {    
      State = VariableHeader.State;
      State &= VAR_DELETED;

      Status = AccessVariableStore (
                 TRUE,
                 VariableGlobal,
                 Variable->Volatile,
                 Instance,
                 (UINTN) &(((VARIABLE_HEADER *)Variable->CurrPtr)->State),
                 sizeof (UINT8),
                 &State
                 ); 
      if (!EFI_ERROR (Status)) {
        UpdateVariableInfo (VariableName, VendorGuid, Volatile, FALSE, FALSE, TRUE, FALSE);
        UpdateVariableCache (VariableName, VendorGuid, Attributes, DataSize, Data);
      }
      goto Done;     
    }
    //
    // Logic comes here to update variable.
    // If the variable is marked valid and the same data has been passed in
    // then return to the caller immediately.
    //
    if (DataSizeOfVariable (&VariableHeader) == DataSize) {
      NextVariable = (VARIABLE_HEADER *)GetEndPointer (VariableGlobal->VolatileVariableBase, TRUE, VariableGlobal, Instance);
      GetVariableDataPtr (Variable->CurrPtr, Variable->Volatile, VariableGlobal, Instance, (CHAR16 *) NextVariable);
      if  (CompareMem (Data, (VOID *) NextVariable, DataSize) == 0) {
        UpdateVariableInfo (VariableName, VendorGuid, Volatile, FALSE, TRUE, FALSE, FALSE);
        Status = EFI_SUCCESS;
        goto Done;
      }
    }
    if ((VariableHeader.State == VAR_ADDED) ||
        (VariableHeader.State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION))) {
      //
      // If new data is different from the old one, mark the old one as VAR_IN_DELETED_TRANSITION.
      // It will be deleted if new variable is successfully written.
      //
      State = VariableHeader.State;
      State &= VAR_IN_DELETED_TRANSITION;

      Status = AccessVariableStore (
                 TRUE,
                 VariableGlobal,
                 Variable->Volatile,
                 Instance,
                 (UINTN) &(((VARIABLE_HEADER *)Variable->CurrPtr)->State),
                 sizeof (UINT8),
                 &State
                 );      
      if (EFI_ERROR (Status)) {
        goto Done;  
      }
    }    
  } else {
    //
    // Create a new variable
    //  
    
    //
    // Make sure we are trying to create a new variable.
    // Setting a data variable with no access, or zero DataSize attributes means to delete it.    
    //
    if (DataSize == 0 || (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
    
    //
    // Only variable have NV|RT attribute can be created in Runtime
    //
    if (EfiAtRuntime () &&
        (((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0) || ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0))) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }         
  }

  //
  // Function part - create a new variable and copy the data.
  // Both update a variable and create a variable will come here.
  //
  // Tricky part: Use scratch data area at the end of volatile variable store
  // as a temporary storage.
  //
  NextVariable = (VARIABLE_HEADER *)GetEndPointer (VariableGlobal->VolatileVariableBase, TRUE, VariableGlobal, Instance);
  ScratchSize = MAX (PcdGet32 (PcdMaxVariableSize), PcdGet32 (PcdMaxHardwareErrorVariableSize));
  NextVariableHeader = (VARIABLE_HEADER *) NextVariable;

  SetMem (NextVariableHeader, ScratchSize, 0xff);

  NextVariableHeader->StartId         = VARIABLE_DATA;
  NextVariableHeader->Attributes      = Attributes;
  NextVariableHeader->PubKeyIndex     = KeyIndex;
  NextVariableHeader->MonotonicCount  = MonotonicCount;
  NextVariableHeader->Reserved        = 0;
  VarNameOffset                       = sizeof (VARIABLE_HEADER);
  VarNameSize                         = StrSize (VariableName);
  CopyMem (
    (UINT8 *) ((UINTN)NextVariable + VarNameOffset),
    VariableName,
    VarNameSize
    );
  VarDataOffset = VarNameOffset + VarNameSize + GET_PAD_SIZE (VarNameSize);
  CopyMem (
    (UINT8 *) ((UINTN)NextVariable + VarDataOffset),
    Data,
    DataSize
    );
  CopyMem (&NextVariableHeader->VendorGuid, VendorGuid, sizeof (EFI_GUID));
  //
  // There will be pad bytes after Data, the NextVariable->NameSize and
  // NextVariable->DataSize should not include pad size so that variable
  // service can get actual size in GetVariable.
  //
  NextVariableHeader->NameSize  = (UINT32)VarNameSize;
  NextVariableHeader->DataSize  = (UINT32)DataSize;

  //
  // The actual size of the variable that stores in storage should
  // include pad size.
  //
  VarSize = VarDataOffset + DataSize + GET_PAD_SIZE (DataSize);
  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
    //
    // Create a nonvolatile variable
    //
    Volatile = FALSE;
    
    GetVarStoreHeader (VariableGlobal->NonVolatileVariableBase, FALSE, VariableGlobal, Instance, &VariableStoreHeader);
    if ((((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) 
             && ((HEADER_ALIGN (VarSize) + Global->HwErrVariableTotalSize) > PcdGet32(PcdHwErrStorageSize)))
             || (((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == 0) 
             && ((HEADER_ALIGN (VarSize) + Global->CommonVariableTotalSize) > VariableStoreHeader.Size - sizeof (VARIABLE_STORE_HEADER) - PcdGet32(PcdHwErrStorageSize)))) {
      if (EfiAtRuntime ()) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      //
      // Perform garbage collection & reclaim operation
      //
      Status = Reclaim (VariableGlobal->NonVolatileVariableBase, &(Global->NonVolatileLastVariableOffset), FALSE, VirtualMode, Global, Variable->CurrPtr);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      Reclaimed = TRUE;
      //
      // If still no enough space, return out of resources
      //
      if ((((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) 
               && ((HEADER_ALIGN (VarSize) + Global->HwErrVariableTotalSize) > PcdGet32(PcdHwErrStorageSize)))
               || (((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == 0) 
               && ((HEADER_ALIGN (VarSize) + Global->CommonVariableTotalSize) > VariableStoreHeader.Size - sizeof (VARIABLE_STORE_HEADER) - PcdGet32(PcdHwErrStorageSize)))) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
    }
    //
    // Four steps
    // 1. Write variable header
    // 2. Set variable state to header valid  
    // 3. Write variable data
    // 4. Set variable state to valid
    //
    //
    // Step 1:
    //
    Status = AccessVariableStore (
               TRUE,
               VariableGlobal,
               FALSE,
               Instance,
               VariableGlobal->NonVolatileVariableBase + Global->NonVolatileLastVariableOffset,
               sizeof (VARIABLE_HEADER),
               (UINT8 *) NextVariable
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Step 2:
    //
    NextVariableHeader->State = VAR_HEADER_VALID_ONLY;
    Status = AccessVariableStore (
               TRUE,
               VariableGlobal,
               FALSE,
               Instance,
               VariableGlobal->NonVolatileVariableBase + Global->NonVolatileLastVariableOffset,
               sizeof (VARIABLE_HEADER),
               (UINT8 *) NextVariable
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }
    //
    // Step 3:
    //
    Status = AccessVariableStore (
               TRUE,
               VariableGlobal,
               FALSE,
               Instance,
               VariableGlobal->NonVolatileVariableBase + Global->NonVolatileLastVariableOffset + sizeof (VARIABLE_HEADER),
               (UINT32) VarSize - sizeof (VARIABLE_HEADER),
               (UINT8 *) NextVariable + sizeof (VARIABLE_HEADER)
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }
    //
    // Step 4:
    //
    NextVariableHeader->State = VAR_ADDED;
    Status = AccessVariableStore (
               TRUE,
               VariableGlobal,
               FALSE,
               Instance,
               VariableGlobal->NonVolatileVariableBase + Global->NonVolatileLastVariableOffset,
               sizeof (VARIABLE_HEADER),
               (UINT8 *) NextVariable
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Global->NonVolatileLastVariableOffset += HEADER_ALIGN (VarSize);

    if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
      Global->HwErrVariableTotalSize += HEADER_ALIGN (VarSize);
    } else {
      Global->CommonVariableTotalSize += HEADER_ALIGN (VarSize);
    }
  } else {
    //
    // Create a volatile variable
    //      
    Volatile = TRUE;

    if ((UINT32) (HEADER_ALIGN(VarSize) + Global->VolatileLastVariableOffset) >
        ((VARIABLE_STORE_HEADER *) ((UINTN) (VariableGlobal->VolatileVariableBase)))->Size) {
      //
      // Perform garbage collection & reclaim operation
      //
      Status = Reclaim (VariableGlobal->VolatileVariableBase, &Global->VolatileLastVariableOffset, TRUE, VirtualMode, Global, Variable->CurrPtr);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      //
      // If still no enough space, return out of resources
      //
      if ((UINT32) (HEADER_ALIGN (VarSize) + Global->VolatileLastVariableOffset) >
            ((VARIABLE_STORE_HEADER *) ((UINTN) (VariableGlobal->VolatileVariableBase)))->Size
            ) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      Reclaimed = TRUE;
    }

    NextVariableHeader->State = VAR_ADDED;
    Status = AccessVariableStore (
               TRUE,
               VariableGlobal,
               TRUE,
               Instance,
               VariableGlobal->VolatileVariableBase + Global->VolatileLastVariableOffset,
               (UINT32) VarSize,
               (UINT8 *) NextVariable
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Global->VolatileLastVariableOffset += HEADER_ALIGN (VarSize);
  }
  //
  // Mark the old variable as deleted
  // If storage has just been reclaimed, the old variable marked as VAR_IN_DELETED_TRANSITION
  // has already been eliminated, so no need to delete it.
  //
  if (!Reclaimed && !EFI_ERROR (Status) && Variable->CurrPtr != 0) {
    State = ((VARIABLE_HEADER *)Variable->CurrPtr)->State;
    State &= VAR_DELETED;

    Status = AccessVariableStore (
               TRUE,
               VariableGlobal,
               Variable->Volatile,
               Instance,
               (UINTN) &(((VARIABLE_HEADER *)Variable->CurrPtr)->State),
               sizeof (UINT8),
               &State
               );
  }

  if (!EFI_ERROR (Status)) {
    UpdateVariableInfo (VariableName, VendorGuid, Volatile, FALSE, TRUE, FALSE, FALSE);
    UpdateVariableCache (VariableName, VendorGuid, Attributes, DataSize, Data);
  }

Done:
  return Status;
}

/**
  Implements EsalGetVariable function of Extended SAL Variable Services Class.

  This function implements EsalGetVariable function of Extended SAL Variable Services Class.
  It is equivalent in functionality to the EFI Runtime Service GetVariable().
  
  @param[in]      VariableName    A Null-terminated Unicode string that is the name of
                                  the vendor's variable.
  @param[in]      VendorGuid      A unique identifier for the vendor.
  @param[out]     Attributes      If not NULL, a pointer to the memory location to return the 
                                  attributes bitmask for the variable.
  @param[in, out] DataSize        Size of Data found. If size is less than the
                                  data, this value contains the required size.
  @param[out]     Data            On input, the size in bytes of the return Data buffer.  
                                  On output, the size of data returned in Data.
  @param[in]      VirtualMode     Current calling mode for this function.
  @param[in]      Global          Context of this Extended SAL Variable Services Class call.

  @retval EFI_SUCCESS            The function completed successfully. 
  @retval EFI_NOT_FOUND          The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   DataSize is too small for the result.  DataSize has 
                                 been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER  VariableName is NULL.
  @retval EFI_INVALID_PARAMETER  VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER  DataSize is NULL.
  @retval EFI_INVALID_PARAMETER  DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION The variable could not be retrieved due to an authentication failure.

**/
EFI_STATUS
EFIAPI
EsalGetVariable (
  IN      CHAR16                *VariableName,
  IN      EFI_GUID              *VendorGuid,
  OUT     UINT32                *Attributes OPTIONAL,
  IN OUT  UINTN                 *DataSize,
  OUT     VOID                  *Data,
  IN      BOOLEAN               VirtualMode,
  IN      ESAL_VARIABLE_GLOBAL  *Global
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarDataSize;
  EFI_STATUS              Status;
  VARIABLE_HEADER         VariableHeader;
  BOOLEAN                 Valid;
  VARIABLE_GLOBAL         *VariableGlobal;
  UINT32                  Instance;

  if (VariableName == NULL || VendorGuid == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VariableGlobal = &Global->VariableGlobal[VirtualMode];
  Instance = Global->FvbInstance;

  AcquireLockOnlyAtBootTime(&VariableGlobal->VariableServicesLock);

  //
  // Check if this variable exists in cache.
  //
  Status = FindVariableInCache (VariableName, VendorGuid, Attributes, DataSize, Data);
  if ((Status == EFI_BUFFER_TOO_SMALL) || (Status == EFI_SUCCESS)){
    //
    // If variable exists in cache, just update statistical information for it and finish.
    // Here UpdateVariableInfo() has already retrieved data & attributes for output.
    //
    UpdateVariableInfo (VariableName, VendorGuid, FALSE, TRUE, FALSE, FALSE, TRUE);
    goto Done;
  }
  //
  // If variable does not exist in cache, search for it in variable storage area.
  //
  Status = FindVariable (VariableName, VendorGuid, &Variable, VariableGlobal, Instance);
  if (Variable.CurrPtr == 0x0 || EFI_ERROR (Status)) {
    //
    // If it cannot be found in variable storage area, goto Done.
    //
    goto Done;
  }

  Valid = IsValidVariableHeader (Variable.CurrPtr, Variable.Volatile, VariableGlobal, Instance, &VariableHeader);
  if (!Valid) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }
  //
  // If variable exists, but not in cache, get its data and attributes, update
  // statistical information, and update cache.
  //
  VarDataSize = DataSizeOfVariable (&VariableHeader);
  ASSERT (VarDataSize != 0);

  if (*DataSize >= VarDataSize) {
    if (Data == NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    GetVariableDataPtr (
      Variable.CurrPtr,
      Variable.Volatile,
      VariableGlobal,
      Instance,
      Data
      );
    if (Attributes != NULL) {
      *Attributes = VariableHeader.Attributes;
    }

    *DataSize = VarDataSize;
    UpdateVariableInfo (VariableName, VendorGuid, Variable.Volatile, TRUE, FALSE, FALSE, FALSE);
    UpdateVariableCache (VariableName, VendorGuid, VariableHeader.Attributes, VarDataSize, Data);
 
    Status = EFI_SUCCESS;
    goto Done;
  } else {
    //
    // If DataSize is too small for the result, return EFI_BUFFER_TOO_SMALL.
    //
    *DataSize = VarDataSize;
    Status = EFI_BUFFER_TOO_SMALL;
    goto Done;
  }

Done:
  ReleaseLockOnlyAtBootTime (&VariableGlobal->VariableServicesLock);
  return Status;
}

/**
  Implements EsalGetNextVariableName function of Extended SAL Variable Services Class.

  This function implements EsalGetNextVariableName function of Extended SAL Variable Services Class.
  It is equivalent in functionality to the EFI Runtime Service GetNextVariableName().
  
  @param[in, out] VariableNameSize Size of the variable
  @param[in, out] VariableName     On input, supplies the last VariableName that was returned by GetNextVariableName().
                                   On output, returns the Null-terminated Unicode string of the current variable.
  @param[in, out] VendorGuid       On input, supplies the last VendorGuid that was returned by GetNextVariableName().
                                   On output, returns the VendorGuid of the current variable.  
  @param[in]      VirtualMode      Current calling mode for this function.
  @param[in]      Global           Context of this Extended SAL Variable Services Class call.

  @retval EFI_SUCCESS             The function completed successfully. 
  @retval EFI_NOT_FOUND           The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL    VariableNameSize is too small for the result. 
                                  VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER   VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER   VariableName is NULL.
  @retval EFI_INVALID_PARAMETER   VendorGuid is NULL.
  @retval EFI_DEVICE_ERROR        The variable name could not be retrieved due to a hardware error.

**/
EFI_STATUS
EFIAPI
EsalGetNextVariableName (
  IN OUT  UINTN                 *VariableNameSize,
  IN OUT  CHAR16                *VariableName,
  IN OUT  EFI_GUID              *VendorGuid,
  IN      BOOLEAN               VirtualMode,
  IN      ESAL_VARIABLE_GLOBAL  *Global
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarNameSize;
  EFI_STATUS              Status;
  VARIABLE_HEADER         VariableHeader;
  VARIABLE_GLOBAL         *VariableGlobal;
  UINT32                  Instance;

  if (VariableNameSize == NULL || VariableName == NULL || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VariableGlobal = &Global->VariableGlobal[VirtualMode];
  Instance = Global->FvbInstance;

  AcquireLockOnlyAtBootTime(&VariableGlobal->VariableServicesLock);

  Status = FindVariable (VariableName, VendorGuid, &Variable, VariableGlobal, Instance);
  //
  // If the variable does not exist, goto Done and return.
  //
  if (Variable.CurrPtr == 0x0 || EFI_ERROR (Status)) {
    goto Done;
  }

  if (VariableName[0] != 0) {
    //
    // If variable name is not NULL, get next variable
    //
    Variable.CurrPtr = GetNextVariablePtr (
                         Variable.CurrPtr,
                         Variable.Volatile,
                         VariableGlobal,
                         Instance
                         );
  }

  while (TRUE) {
    if (Variable.CurrPtr >= Variable.EndPtr || Variable.CurrPtr == 0x0) {
      //
      // If fail to find a variable in current area, reverse the volatile attribute of area to search.
      //
      Variable.Volatile = (BOOLEAN) (Variable.Volatile ^ ((BOOLEAN) 0x1));
      //
      // Here we depend on the searching sequence of FindVariable().
      // It first searches volatile area, then NV area.
      // So if the volatile attribute after switching is non-volatile, it means that we have finished searching volatile area,
      // and EFI_NOT_FOUND is returnd.
      // Otherwise, it means that we have finished searchig non-volatile area, and we will continue to search volatile area.
      //
      if (!Variable.Volatile) {
        Variable.StartPtr = GetStartPointer (VariableGlobal->NonVolatileVariableBase);
        Variable.EndPtr   = GetEndPointer (VariableGlobal->NonVolatileVariableBase, FALSE, VariableGlobal, Instance);
      } else {
        Status = EFI_NOT_FOUND;
        goto Done;
      }

      Variable.CurrPtr = Variable.StartPtr;
      if (!IsValidVariableHeader (Variable.CurrPtr, Variable.Volatile, VariableGlobal, Instance, NULL)) {
        continue;
      }
    }
    //
    // Variable is found
    //
    if (IsValidVariableHeader (Variable.CurrPtr, Variable.Volatile, VariableGlobal, Instance, &VariableHeader)) {
      if ((VariableHeader.State == VAR_ADDED) &&
          (!(EfiAtRuntime () && ((VariableHeader.Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0)))) {
        VarNameSize = NameSizeOfVariable (&VariableHeader);
        ASSERT (VarNameSize != 0);

        if (VarNameSize <= *VariableNameSize) {
          GetVariableNamePtr (
            Variable.CurrPtr,
            Variable.Volatile,
            VariableGlobal,
            Instance,
            VariableName
            );
          CopyMem (
            VendorGuid,
            &VariableHeader.VendorGuid,
            sizeof (EFI_GUID)
            );
          Status = EFI_SUCCESS;
        } else {
          Status = EFI_BUFFER_TOO_SMALL;
        }

        *VariableNameSize = VarNameSize;
        goto Done;
      }
    }

    Variable.CurrPtr = GetNextVariablePtr (
                         Variable.CurrPtr,
                         Variable.Volatile,
                         VariableGlobal,
                         Instance
                         );
  }

Done:
  ReleaseLockOnlyAtBootTime (&VariableGlobal->VariableServicesLock);
  return Status;
}

/**
  Implements EsalSetVariable function of Extended SAL Variable Services Class.

  This function implements EsalSetVariable function of Extended SAL Variable Services Class.
  It is equivalent in functionality to the EFI Runtime Service SetVariable().
  
  @param[in]  VariableName       A Null-terminated Unicode string that is the name of the vendor's
                                 variable.  Each VariableName is unique for each 
                                 VendorGuid.  VariableName must contain 1 or more 
                                 Unicode characters.  If VariableName is an empty Unicode 
                                 string, then EFI_INVALID_PARAMETER is returned.
  @param[in]  VendorGuid         A unique identifier for the vendor.
  @param[in]  Attributes         Attributes bitmask to set for the variable.
  @param[in]  DataSize           The size in bytes of the Data buffer.  A size of zero causes the
                                 variable to be deleted.
  @param[in]  Data               The contents for the variable.
  @param[in]  VirtualMode        Current calling mode for this function.
  @param[in]  Global             Context of this Extended SAL Variable Services Class call.

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as 
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits was supplied, or the 
                                 DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER  VariableName is an empty Unicode string.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved due to a hardware failure.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION The variable could not be retrieved due to an authentication failure.
  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.

**/
EFI_STATUS
EFIAPI
EsalSetVariable (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINT32                  Attributes,
  IN UINTN                   DataSize,
  IN VOID                    *Data,
  IN BOOLEAN                 VirtualMode,
  IN ESAL_VARIABLE_GLOBAL    *Global
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  EFI_STATUS              Status;
  EFI_PHYSICAL_ADDRESS    NextVariable;
  EFI_PHYSICAL_ADDRESS    Point;
  VARIABLE_GLOBAL         *VariableGlobal;
  UINT32                  Instance;
  UINT32                  KeyIndex;
  UINT64                  MonotonicCount;
  UINTN                   PayloadSize;

  //
  // Check input parameters
  //
  if (VariableName == NULL || VariableName[0] == 0 || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }  

  if (DataSize != 0 && Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // EFI_VARIABLE_RUNTIME_ACCESS bit cannot be set without EFI_VARIABLE_BOOTSERVICE_ACCESS bit.
  //
  if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) {
    if (DataSize < AUTHINFO_SIZE) {
      //
      // Try to write Authencated Variable without AuthInfo
      //
      return EFI_SECURITY_VIOLATION;
    } 
    PayloadSize = DataSize - AUTHINFO_SIZE; 
  } else {
    PayloadSize = DataSize; 
  }

  VariableGlobal = &Global->VariableGlobal[VirtualMode];
  Instance = Global->FvbInstance;

  if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    //
    // For variable for hardware error record, the size of the VariableName, including the Unicode Null
    // in bytes plus the DataSize is limited to maximum size of PcdGet32(PcdMaxHardwareErrorVariableSize) bytes.
    //
    if ((PayloadSize > PcdGet32(PcdMaxHardwareErrorVariableSize)) ||                                                       
        (sizeof (VARIABLE_HEADER) + StrSize (VariableName) + PayloadSize > PcdGet32(PcdMaxHardwareErrorVariableSize))) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // According to UEFI spec, HARDWARE_ERROR_RECORD variable name convention should be L"HwErrRecXXXX"
    //
    if (StrnCmp (VariableName, \
                 Global->VariableName[VirtualMode][VAR_HW_ERR_REC], \
                 StrLen(Global->VariableName[VirtualMode][VAR_HW_ERR_REC])) != 0) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // For variable not for hardware error record, the size of the VariableName, including the
    // Unicode Null in bytes plus the DataSize is limited to maximum size of PcdGet32(PcdMaxVariableSize) bytes.
    //
    if ((PayloadSize > PcdGet32(PcdMaxVariableSize)) ||
        (sizeof (VARIABLE_HEADER) + StrSize (VariableName) + PayloadSize > PcdGet32(PcdMaxVariableSize))) {
      return EFI_INVALID_PARAMETER;
    }  
  }  

  AcquireLockOnlyAtBootTime(&VariableGlobal->VariableServicesLock);

  //
  // Consider reentrant in MCA/INIT/NMI. It needs be reupdated;
  //
  if (InterlockedIncrement (&Global->ReentrantState) > 1) {
    Point = VariableGlobal->NonVolatileVariableBase;;
    //
    // Parse non-volatile variable data and get last variable offset
    //
    NextVariable  = GetStartPointer (Point);
    while (IsValidVariableHeader (NextVariable, FALSE, VariableGlobal, Instance, NULL)) {
      NextVariable = GetNextVariablePtr (NextVariable, FALSE, VariableGlobal, Instance);
    }
    Global->NonVolatileLastVariableOffset = NextVariable - Point;
  }

  //
  // Check whether the input variable exists
  //

  Status = FindVariable (VariableName, VendorGuid, &Variable, VariableGlobal, Instance);

  //
  // Hook the operation of setting PlatformLangCodes/PlatformLang and LangCodes/Lang
  //
  AutoUpdateLangVariable (VariableName, Data, PayloadSize, VirtualMode, Global);

  //
  // Process PK, KEK, Sigdb seperately
  //
  if (CompareGuid (VendorGuid, Global->GlobalVariableGuid[VirtualMode]) && (StrCmp (VariableName, Global->VariableName[VirtualMode][VAR_PLATFORM_KEY]) == 0)) {
    Status = ProcessVarWithPk (VariableName, VendorGuid, Data, DataSize, VirtualMode, Global, &Variable, Attributes, TRUE);
  } else if (CompareGuid (VendorGuid, Global->GlobalVariableGuid[VirtualMode]) && (StrCmp (VariableName, Global->VariableName[VirtualMode][VAR_KEY_EXCHANGE_KEY]) == 0)) {
    Status = ProcessVarWithPk (VariableName, VendorGuid, Data, DataSize, VirtualMode, Global, &Variable, Attributes, FALSE);
  } else if (CompareGuid (VendorGuid, Global->ImageSecurityDatabaseGuid[VirtualMode])) {
    Status = ProcessVarWithKek (VariableName, VendorGuid, Data, DataSize, VirtualMode, Global, &Variable, Attributes);
  } else {
    Status = VerifyVariable (Data, DataSize, VirtualMode, Global, &Variable, Attributes, &KeyIndex, &MonotonicCount);
    if (!EFI_ERROR(Status)) {
      //
      // Verification pass
      //
      if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
        //
        // Cut the certificate size before set
        //
        Status = UpdateVariable (
                   VariableName, 
                   VendorGuid, 
                   (UINT8*)Data + AUTHINFO_SIZE, 
                   DataSize - AUTHINFO_SIZE, 
                   Attributes, 
                   KeyIndex, 
                   MonotonicCount, 
                   VirtualMode, 
                   Global, 
                   &Variable
                   );
      } else {
        //
        // Update variable as usual 
        //
        Status = UpdateVariable (
                   VariableName, 
                   VendorGuid, 
                   Data, 
                   DataSize, 
                   Attributes, 
                   0, 
                   0, 
                   VirtualMode, 
                   Global, 
                   &Variable
                   );
      }
    }
  }

  InterlockedDecrement (&Global->ReentrantState);
  ReleaseLockOnlyAtBootTime (&VariableGlobal->VariableServicesLock);
  return Status;
}

/**
  Implements EsalQueryVariableInfo function of Extended SAL Variable Services Class.

  This function implements EsalQueryVariableInfo function of Extended SAL Variable Services Class.
  It is equivalent in functionality to the EFI Runtime Service QueryVariableInfo().

  @param[in]  Attributes                   Attributes bitmask to specify the type of variables
                                           on which to return information.
  @param[out] MaximumVariableStorageSize   On output the maximum size of the storage space available for 
                                           the EFI variables associated with the attributes specified.  
  @param[out] RemainingVariableStorageSize Returns the remaining size of the storage space available for EFI 
                                           variables associated with the attributes specified.
  @param[out] MaximumVariableSize          Returns the maximum size of an individual EFI variable 
                                           associated with the attributes specified.
  @param[in]  VirtualMode                  Current calling mode for this function
  @param[in]  Global                       Context of this Extended SAL Variable Services Class call

  @retval EFI_SUCCESS                      Valid answer returned.
  @retval EFI_INVALID_PARAMETER            An invalid combination of attribute bits was supplied.
  @retval EFI_UNSUPPORTED                  The attribute is not supported on this platform, and the 
                                           MaximumVariableStorageSize, RemainingVariableStorageSize, 
                                           MaximumVariableSize are undefined.
**/
EFI_STATUS
EFIAPI
EsalQueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize,
  IN  BOOLEAN                VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL   *Global
  )
{
  EFI_PHYSICAL_ADDRESS   Variable;
  EFI_PHYSICAL_ADDRESS   NextVariable;
  UINT64                 VariableSize;
  EFI_PHYSICAL_ADDRESS   VariableStoreHeaderAddress;
  BOOLEAN                Volatile;
  VARIABLE_STORE_HEADER  VarStoreHeader;
  VARIABLE_HEADER        VariableHeader;
  UINT64                 CommonVariableTotalSize;
  UINT64                 HwErrVariableTotalSize;
  VARIABLE_GLOBAL        *VariableGlobal;
  UINT32                 Instance;

  CommonVariableTotalSize = 0;
  HwErrVariableTotalSize = 0;

  if(MaximumVariableStorageSize == NULL || RemainingVariableStorageSize == NULL || MaximumVariableSize == NULL || Attributes == 0) {
    return EFI_INVALID_PARAMETER;
  }
  
  if((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == 0) {
    //
    // Make sure the Attributes combination is supported by the platform.
    //
    return EFI_UNSUPPORTED;  
  } else if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    //
    // Make sure if runtime bit is set, boot service bit is set also.
    //
    return EFI_INVALID_PARAMETER;
  } else if (EfiAtRuntime () && ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0)) {
    //
    // Make sure RT Attribute is set if we are in Runtime phase.
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    //
    // Make sure Hw Attribute is set with NV.
    //
    return EFI_INVALID_PARAMETER;
  }

  VariableGlobal = &Global->VariableGlobal[VirtualMode];
  Instance = Global->FvbInstance;

  AcquireLockOnlyAtBootTime(&VariableGlobal->VariableServicesLock);

  if((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
    //
    // Query is Volatile related.
    //
    Volatile = TRUE;
    VariableStoreHeaderAddress = VariableGlobal->VolatileVariableBase;
  } else {
    //
    // Query is Non-Volatile related.
    //
    Volatile = FALSE;
    VariableStoreHeaderAddress = VariableGlobal->NonVolatileVariableBase;
  }

  //
  // Now let's fill *MaximumVariableStorageSize *RemainingVariableStorageSize
  // with the storage size (excluding the storage header size).
  //
  GetVarStoreHeader (VariableStoreHeaderAddress, Volatile, VariableGlobal, Instance, &VarStoreHeader);

  *MaximumVariableStorageSize   = VarStoreHeader.Size - sizeof (VARIABLE_STORE_HEADER);

  // Harware error record variable needs larger size.
  //
  if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
    *MaximumVariableStorageSize = PcdGet32(PcdHwErrStorageSize);
    *MaximumVariableSize = PcdGet32(PcdMaxHardwareErrorVariableSize) - sizeof (VARIABLE_HEADER);
  } else {
    if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      ASSERT (PcdGet32(PcdHwErrStorageSize) < VarStoreHeader.Size);
      *MaximumVariableStorageSize = VarStoreHeader.Size - sizeof (VARIABLE_STORE_HEADER) - PcdGet32(PcdHwErrStorageSize);
    }

    //
    // Let *MaximumVariableSize be PcdGet32(PcdMaxVariableSize) with the exception of the variable header size.
    //
    *MaximumVariableSize = PcdGet32(PcdMaxVariableSize) - sizeof (VARIABLE_HEADER);
  }

  //
  // Point to the starting address of the variables.
  //
  Variable = GetStartPointer (VariableStoreHeaderAddress);

  //
  // Now walk through the related variable store.
  //
  while (IsValidVariableHeader (Variable, Volatile, VariableGlobal, Instance, &VariableHeader) &&
         (Variable < GetEndPointer (VariableStoreHeaderAddress, Volatile, VariableGlobal, Instance))) {
    NextVariable = GetNextVariablePtr (Variable, Volatile, VariableGlobal, Instance);
    VariableSize = NextVariable - Variable;

    if (EfiAtRuntime ()) {
      //
      // we don't take the state of the variables in mind
      // when calculating RemainingVariableStorageSize,
      // since the space occupied by variables not marked with
      // VAR_ADDED is not allowed to be reclaimed in Runtime.
      //
      if ((VariableHeader.Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
        HwErrVariableTotalSize += VariableSize;
      } else {
        CommonVariableTotalSize += VariableSize;
      }
    } else {
      //
      // Only care about Variables with State VAR_ADDED,because
      // the space not marked as VAR_ADDED is reclaimable now.
      //
      if (VariableHeader.State == VAR_ADDED) {
        if ((VariableHeader.Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
          HwErrVariableTotalSize += VariableSize;
        } else {
          CommonVariableTotalSize += VariableSize;
        }
      }
    }

    //
    // Go to the next one
    //
    Variable = NextVariable;
  }

  if ((Attributes  & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD){
    *RemainingVariableStorageSize = *MaximumVariableStorageSize - HwErrVariableTotalSize;
  }else {
    *RemainingVariableStorageSize = *MaximumVariableStorageSize - CommonVariableTotalSize;
  }

  if (*RemainingVariableStorageSize < sizeof (VARIABLE_HEADER)) {
    *MaximumVariableSize = 0;
  } else if ((*RemainingVariableStorageSize - sizeof (VARIABLE_HEADER)) < *MaximumVariableSize) {
    *MaximumVariableSize = *RemainingVariableStorageSize - sizeof (VARIABLE_HEADER);
  }

  ReleaseLockOnlyAtBootTime (&VariableGlobal->VariableServicesLock);
  return EFI_SUCCESS;
}

/**
  Notification function of EVT_GROUP_READY_TO_BOOT event group.

  This is a notification function registered on EVT_GROUP_READY_TO_BOOT event group.
  When the Boot Manager is about to load and execute a boot option, it reclaims variable
  storage if free size is below the threshold.

  @param[in]  Event        Event whose notification function is being invoked.
  @param[in]  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
ReclaimForOS(
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINT32                          VarSize;
  EFI_STATUS                      Status;
  UINTN                           CommonVariableSpace;
  UINTN                           RemainingCommonVariableSpace;
  UINTN                           RemainingHwErrVariableSpace;

  VarSize = ((VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase))->Size;
  Status  = EFI_SUCCESS; 
  //
  //Allowable max size of common variable storage space
  //
  CommonVariableSpace = VarSize - sizeof (VARIABLE_STORE_HEADER) - PcdGet32(PcdHwErrStorageSize);

  RemainingCommonVariableSpace = CommonVariableSpace - mVariableModuleGlobal->CommonVariableTotalSize;
 
  RemainingHwErrVariableSpace = PcdGet32 (PcdHwErrStorageSize) - mVariableModuleGlobal->HwErrVariableTotalSize;
  //
  // If the free area is below a threshold, then performs reclaim operation.
  //
  if ((RemainingCommonVariableSpace < PcdGet32 (PcdMaxVariableSize))
    || ((PcdGet32 (PcdHwErrStorageSize) != 0) && 
       (RemainingHwErrVariableSpace < PcdGet32 (PcdMaxHardwareErrorVariableSize)))){
    Status = Reclaim (
               mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase,
               &mVariableModuleGlobal->NonVolatileLastVariableOffset,
               FALSE,
               Physical,
               mVariableModuleGlobal,
               0x0
               );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Flush the HOB variable to NV variable storage.
**/
VOID
FlushHob2Nv (
  VOID
  )
{
  EFI_STATUS                      Status;
  VOID                            *GuidHob;
  VARIABLE_STORE_HEADER           *VariableStoreHeader;
  VARIABLE_HEADER                 *VariableHeader;
  //
  // Get HOB variable store.
  //
  GuidHob = GetFirstGuidHob (&gEfiAuthenticatedVariableGuid);
  if (GuidHob != NULL) {
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) GET_GUID_HOB_DATA (GuidHob);
    if (CompareGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid) &&
        (VariableStoreHeader->Format == VARIABLE_STORE_FORMATTED) &&
        (VariableStoreHeader->State == VARIABLE_STORE_HEALTHY)
       ) {
      DEBUG ((EFI_D_INFO, "HOB Variable Store appears to be valid.\n"));
      //
      // Flush the HOB variable to NV Variable storage.
      //
      for ( VariableHeader = (VARIABLE_HEADER *) HEADER_ALIGN (VariableStoreHeader + 1)
          ; (VariableHeader < (VARIABLE_HEADER *) HEADER_ALIGN ((UINTN) VariableStoreHeader + VariableStoreHeader->Size)
            &&
            (VariableHeader->StartId == VARIABLE_DATA))
          ; VariableHeader = (VARIABLE_HEADER *) HEADER_ALIGN ((UINTN) (VariableHeader + 1)
                           + VariableHeader->NameSize + GET_PAD_SIZE (VariableHeader->NameSize)
                           + VariableHeader->DataSize + GET_PAD_SIZE (VariableHeader->DataSize)
                           )
          ) {
        ASSERT (VariableHeader->State == VAR_ADDED);
        ASSERT ((VariableHeader->Attributes & EFI_VARIABLE_NON_VOLATILE) != 0);
        Status = EsalSetVariable (
                   (CHAR16 *) (VariableHeader + 1),
                   &VariableHeader->VendorGuid,
                   VariableHeader->Attributes,
                   VariableHeader->DataSize,
                   (UINT8 *) (VariableHeader + 1) + VariableHeader->NameSize + GET_PAD_SIZE (VariableHeader->NameSize),
                   Physical,
                   mVariableModuleGlobal
                   );
        ASSERT_EFI_ERROR (Status);
      }
    }
  }
}

/**
  Initializes variable store area for non-volatile and volatile variable.

  This function allocates and initializes memory space for global context of ESAL
  variable service and variable store area for non-volatile and volatile variable.

  @param[in]  ImageHandle       The Image handle of this driver.
  @param[in]  SystemTable       The pointer of EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
VariableCommonInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_FIRMWARE_VOLUME_HEADER      *FwVolHeader;
  EFI_PHYSICAL_ADDRESS            CurrPtr;
  VARIABLE_STORE_HEADER           *VolatileVariableStore;
  VARIABLE_STORE_HEADER           *VariableStoreHeader;
  EFI_PHYSICAL_ADDRESS            Variable;
  EFI_PHYSICAL_ADDRESS            NextVariable;
  UINTN                           VariableSize;
  UINT32                          Instance;
  EFI_PHYSICAL_ADDRESS            FvVolHdr;
  EFI_PHYSICAL_ADDRESS            TempVariableStoreHeader;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR GcdDescriptor;
  UINT64                          BaseAddress;
  UINT64                          Length;
  UINTN                           Index;
  UINT8                           Data;
  EFI_PHYSICAL_ADDRESS            VariableStoreBase;
  UINT64                          VariableStoreLength;
  EFI_EVENT                       ReadyToBootEvent;
  UINTN                           ScratchSize;

  //
  // Allocate memory for mVariableModuleGlobal
  //
  mVariableModuleGlobal = AllocateRuntimeZeroPool (sizeof (ESAL_VARIABLE_GLOBAL));
  if (mVariableModuleGlobal == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mVariableModuleGlobal->GlobalVariableGuid[Physical] = &gEfiGlobalVariableGuid;
  CopyMem (
    mVariableModuleGlobal->VariableName[Physical],
    mVariableName,
    sizeof (mVariableName)
    );

  EfiInitializeLock(&mVariableModuleGlobal->VariableGlobal[Physical].VariableServicesLock, TPL_NOTIFY);

  //
  // Note that in EdkII variable driver implementation, Hardware Error Record type variable
  // is stored with common variable in the same NV region. So the platform integrator should
  // ensure that the value of PcdHwErrStorageSize is less than or equal to the value of 
  // PcdFlashNvStorageVariableSize.
  //
  ASSERT (PcdGet32(PcdHwErrStorageSize) <= PcdGet32 (PcdFlashNvStorageVariableSize));

  //
  // Allocate memory for volatile variable store
  //
  ScratchSize = MAX (PcdGet32 (PcdMaxVariableSize), PcdGet32 (PcdMaxHardwareErrorVariableSize));
  VolatileVariableStore = AllocateRuntimePool (PcdGet32 (PcdVariableStoreSize) + ScratchSize);
  if (VolatileVariableStore == NULL) {
    FreePool (mVariableModuleGlobal);
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (VolatileVariableStore, PcdGet32 (PcdVariableStoreSize) + ScratchSize, 0xff);

  //
  // Variable Specific Data
  //
  mVariableModuleGlobal->VariableGlobal[Physical].VolatileVariableBase = (EFI_PHYSICAL_ADDRESS) (UINTN) VolatileVariableStore;
  mVariableModuleGlobal->VolatileLastVariableOffset = (UINTN) GetStartPointer ((EFI_PHYSICAL_ADDRESS) VolatileVariableStore) - (UINTN) VolatileVariableStore;

  CopyGuid (&VolatileVariableStore->Signature, &gEfiAuthenticatedVariableGuid);
  VolatileVariableStore->Size                       = PcdGet32 (PcdVariableStoreSize);
  VolatileVariableStore->Format                     = VARIABLE_STORE_FORMATTED;
  VolatileVariableStore->State                      = VARIABLE_STORE_HEALTHY;
  VolatileVariableStore->Reserved                   = 0;
  VolatileVariableStore->Reserved1                  = 0;

  //
  // Get non volatile varaible store
  //
  TempVariableStoreHeader = (UINT64) PcdGet32 (PcdFlashNvStorageVariableBase);
  VariableStoreBase = TempVariableStoreHeader + \
                              (((EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) (TempVariableStoreHeader)) -> HeaderLength);
  VariableStoreLength = (UINT64) PcdGet32 (PcdFlashNvStorageVariableSize) - \
                                (((EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) (TempVariableStoreHeader)) -> HeaderLength);
  //
  // Mark the variable storage region of the FLASH as RUNTIME
  //
  BaseAddress = VariableStoreBase & (~EFI_PAGE_MASK);
  Length      = VariableStoreLength + (VariableStoreBase - BaseAddress);
  Length      = (Length + EFI_PAGE_SIZE - 1) & (~EFI_PAGE_MASK);

  Status      = gDS->GetMemorySpaceDescriptor (BaseAddress, &GcdDescriptor);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  BaseAddress,
                  Length,
                  GcdDescriptor.Attributes | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Get address of non volatile variable store base.
  //
  mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase = VariableStoreBase;

  //
  // Check Integrity
  //
  //
  // Find the Correct Instance of the FV Block Service.
  //
  Instance  = 0;
  CurrPtr   = mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase;

  do {
    FvVolHdr = 0;
    Status    = (EFI_STATUS) EsalCall (
                               EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_LO,
                               EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_HI,
                               GetPhysicalAddressFunctionId, 
                               Instance, 
                               (UINT64) &FvVolHdr, 
                               0, 
                               0, 
                               0, 
                               0, 
                               0
                               ).Status;
    if (EFI_ERROR (Status)) {
      break;
    }
    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) FvVolHdr);
    ASSERT (FwVolHeader != NULL);
    if (CurrPtr >= (EFI_PHYSICAL_ADDRESS) FwVolHeader &&
        CurrPtr <  ((EFI_PHYSICAL_ADDRESS) FwVolHeader + FwVolHeader->FvLength)) {
      mVariableModuleGlobal->FvbInstance = Instance;
      break;
    }

    Instance++;
  } while (Status == EFI_SUCCESS);

  VariableStoreHeader = (VARIABLE_STORE_HEADER *) CurrPtr;
  if (GetVariableStoreStatus (VariableStoreHeader) == EfiValid) {
    if (~VariableStoreHeader->Size == 0) {
      Status = AccessVariableStore (
                 TRUE,
                 &mVariableModuleGlobal->VariableGlobal[Physical],
                 FALSE,
                 mVariableModuleGlobal->FvbInstance,
                 (UINTN) &VariableStoreHeader->Size,
                 sizeof (UINT32),
                 (UINT8 *) &VariableStoreLength
                 );
      //
      // As Variables are stored in NV storage, which are slow devices,such as flash.
      // Variable operation may skip checking variable program result to improve performance,
      // We can assume Variable program is OK through some check point.
      // Variable Store Size Setting should be the first Variable write operation,
      // We can assume all Read/Write is OK if we can set Variable store size successfully.
      // If write fail, we will assert here.
      //
      ASSERT(VariableStoreHeader->Size == VariableStoreLength);

      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }

    mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase = (EFI_PHYSICAL_ADDRESS) ((UINTN) CurrPtr);
    //
    // Parse non-volatile variable data and get last variable offset.
    //
    Variable = GetStartPointer (CurrPtr);
    Status   = EFI_SUCCESS;

    while (IsValidVariableHeader (Variable, FALSE, &(mVariableModuleGlobal->VariableGlobal[Physical]), Instance, NULL)) {
      NextVariable = GetNextVariablePtr (
                       Variable,
                       FALSE,
                       &(mVariableModuleGlobal->VariableGlobal[Physical]),
                       Instance
                       );
      VariableSize = NextVariable - Variable;
      if ((((VARIABLE_HEADER *)Variable)->Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
        mVariableModuleGlobal->HwErrVariableTotalSize += VariableSize;
      } else {
        mVariableModuleGlobal->CommonVariableTotalSize += VariableSize;
      }

      Variable = NextVariable;
    }

    mVariableModuleGlobal->NonVolatileLastVariableOffset = (UINTN) Variable - (UINTN) CurrPtr;

    //
    // Check if the free area is really free.
    //
    for (Index = mVariableModuleGlobal->NonVolatileLastVariableOffset; Index < VariableStoreHeader->Size; Index++) {
      Data = ((UINT8 *) (UINTN) mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase)[Index];
      if (Data != 0xff) {
        //
        // There must be something wrong in variable store, do reclaim operation.
        //
        Status = Reclaim (
                   mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase,
                   &mVariableModuleGlobal->NonVolatileLastVariableOffset,
                   FALSE,
                   Physical,
                   mVariableModuleGlobal,
                   0x0
                   );
        if (EFI_ERROR (Status)) {
          goto Done;
        }
        break;
      }
    }

    //
    // Register the event handling function to reclaim variable for OS usage.
    //
    Status = EfiCreateEventReadyToBootEx (
               TPL_NOTIFY, 
               ReclaimForOS, 
               NULL, 
               &ReadyToBootEvent
               );
  } else {
    Status = EFI_VOLUME_CORRUPTED;
    DEBUG((EFI_D_INFO, "Variable Store header is corrupted\n"));
  }

Done:
  if (EFI_ERROR (Status)) {
    FreePool (mVariableModuleGlobal);
    FreePool (VolatileVariableStore);
  }

  return Status;
}
