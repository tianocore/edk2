/** @file

  Implement all four UEFI Runtime Variable services for the nonvolatile
  and volatile storage space and install variable architecture protocol.
  
Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "Variable.h"

VARIABLE_MODULE_GLOBAL  *mVariableModuleGlobal;
EFI_EVENT   mVirtualAddressChangeEvent = NULL;
EFI_HANDLE  mHandle = NULL;

///
/// The current Hii implementation accesses this variable many times on every boot.
/// Other common variables are only accessed once. This is why this cache algorithm
/// only targets a single variable. Probably to get an performance improvement out of
/// a Cache you would need a cache that improves the search performance for a variable.
///
VARIABLE_CACHE_ENTRY mVariableCache[] = {
  {
    &gEfiGlobalVariableGuid,
    L"Lang",
    0x00000000,
    0x00,
    NULL
  }
};

VARIABLE_INFO_ENTRY *gVariableInfo = NULL;

EFI_STATUS
FtwVariableSpace (
  IN EFI_PHYSICAL_ADDRESS   VariableBaseAddress,
  IN UINT8                  *Buffer,
  IN UINTN                  BufferSize
  );


/**
  Acquires lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiAcquireLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiAcquireLock() at boot time, and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to acquire

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
  at runtime.

  @param  Lock         A pointer to the lock to release

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
  Routine used to track statistical information about variable usage. 
  The data is stored in the EFI system table so it can be accessed later.
  VariableInfo.efi can dump out the table. Only Boot Services variable 
  accesses are tracked by this code. The PcdVariableCollectStatistics
  build flag controls if this feature is enabled. 

  A read that hits in the cache will have Read and Cache true for 
  the transaction. Data is allocated by this routine, but never
  freed.

  @param[in] VariableName   Name of the Variable to track
  @param[in] VendorGuid     Guid of the Variable to track
  @param[in] Volatile       TRUE if volatile FALSE if non-volatile
  @param[in] Read           TRUE if GetVariable() was called
  @param[in] Write          TRUE if SetVariable() was called
  @param[in] Delete         TRUE if deleted via SetVariable()
  @param[in] Cache          TRUE for a cache hit.

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
      // Don't collect statistics at runtime
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
      gVariableInfo->Name = AllocatePool (StrLen (VariableName));
      ASSERT (gVariableInfo->Name != NULL);
      StrCpy (gVariableInfo->Name, VariableName);
      gVariableInfo->Volatile = Volatile;

      gBS->InstallConfigurationTable (&gEfiVariableInfoGuid, gVariableInfo);
    }

    
    for (Entry = gVariableInfo; Entry != NULL; Entry = Entry->Next) {
      if (CompareGuid (VendorGuid, &Entry->VendorGuid)) {
        if (StrCmp (VariableName, Entry->Name) == 0) {
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
        Entry->Next->Name = AllocatePool (StrLen (VariableName));
        ASSERT (Entry->Next->Name != NULL);
        StrCpy (Entry->Next->Name, VariableName);
        Entry->Next->Volatile = Volatile;
      }

    }
  }
}


/**

  This code checks if variable header is valid or not.

  @param Variable        Pointer to the Variable Header.

  @retval TRUE           Variable header is valid.
  @retval FALSE          Variable header is not valid.

**/
BOOLEAN
IsValidVariableHeader (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if (Variable == NULL || Variable->StartId != VARIABLE_DATA) {
    return FALSE;
  }

  return TRUE;
}


/**

  This function writes data to the FWH at the correct LBA even if the LBAs
  are fragmented.

  @param Global                  Pointer to VARAIBLE_GLOBAL structure
  @param Volatile                Point out the Variable is Volatile or Non-Volatile
  @param SetByIndex              TRUE if target pointer is given as index
                                 FALSE if target pointer is absolute
  @param Instance                Instance of FV Block services
  @param DataPtrIndex            Pointer to the Data from the end of VARIABLE_STORE_HEADER
                                 structure
  @param DataSize                Size of data to be written
  @param Buffer                  Pointer to the buffer from which data is written

  @retval EFI_INVALID_PARAMETER  Parameters not valid
  @retval EFI_SUCCESS            Variable store successfully updated

**/
EFI_STATUS
UpdateVariableStore (
  IN  VARIABLE_GLOBAL         *Global,
  IN  BOOLEAN                 Volatile,
  IN  BOOLEAN                 SetByIndex,
  IN  UINTN                   Instance,
  IN  UINTN                   DataPtrIndex,
  IN  UINT32                  DataSize,
  IN  UINT8                   *Buffer
  )
{
  EFI_FV_BLOCK_MAP_ENTRY      *PtrBlockMapEntry;
  UINTN                       BlockIndex2;
  UINTN                       LinearOffset;
  UINTN                       CurrWriteSize;
  UINTN                       CurrWritePtr;
  UINT8                       *CurrBuffer;
  EFI_LBA                     LbaNumber;
  UINTN                       Size;
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  VARIABLE_STORE_HEADER       *VolatileBase;
  EFI_PHYSICAL_ADDRESS        FvVolHdr;
  EFI_PHYSICAL_ADDRESS        DataPtr;
  EFI_STATUS                  Status;

  FwVolHeader = NULL;
  DataPtr     = DataPtrIndex;

  //
  // Check if the Data is Volatile
  //
  if (!Volatile) {
    EfiFvbGetPhysicalAddress (Instance, &FvVolHdr);
    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) FvVolHdr);
    //
    // Data Pointer should point to the actual Address where data is to be
    // written
    //
    if (SetByIndex) {
      DataPtr += mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase;
    }

    if ((DataPtr + DataSize) >= ((EFI_PHYSICAL_ADDRESS) (UINTN) ((UINT8 *) FwVolHeader + FwVolHeader->FvLength))) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // Data Pointer should point to the actual Address where data is to be
    // written
    //
    VolatileBase = (VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal.VolatileVariableBase);
    if (SetByIndex) {
      DataPtr += mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
    }

    if ((DataPtr + DataSize) >= ((UINTN) ((UINT8 *) VolatileBase + VolatileBase->Size))) {
      return EFI_INVALID_PARAMETER;
    }
    
    //
    // If Volatile Variable just do a simple mem copy.
    //    
    CopyMem ((UINT8 *)(UINTN)DataPtr, Buffer, DataSize);
    return EFI_SUCCESS;
  }
  
  //
  // If we are here we are dealing with Non-Volatile Variables
  //
  LinearOffset  = (UINTN) FwVolHeader;
  CurrWritePtr  = (UINTN) DataPtr;
  CurrWriteSize = DataSize;
  CurrBuffer    = Buffer;
  LbaNumber     = 0;

  if (CurrWritePtr < LinearOffset) {
    return EFI_INVALID_PARAMETER;
  }

  for (PtrBlockMapEntry = FwVolHeader->BlockMap; PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {
    for (BlockIndex2 = 0; BlockIndex2 < PtrBlockMapEntry->NumBlocks; BlockIndex2++) {
      //
      // Check to see if the Variable Writes are spanning through multiple
      // blocks.
      //
      if ((CurrWritePtr >= LinearOffset) && (CurrWritePtr < LinearOffset + PtrBlockMapEntry->Length)) {
        if ((CurrWritePtr + CurrWriteSize) <= (LinearOffset + PtrBlockMapEntry->Length)) {
          Status = EfiFvbWriteBlock (
                    Instance,
                    LbaNumber,
                    (UINTN) (CurrWritePtr - LinearOffset),
                    &CurrWriteSize,
                    CurrBuffer
                    );
            return Status;
        } else {
          Size = (UINT32) (LinearOffset + PtrBlockMapEntry->Length - CurrWritePtr);
          Status = EfiFvbWriteBlock (
                    Instance,
                    LbaNumber,
                    (UINTN) (CurrWritePtr - LinearOffset),
                    &Size,
                    CurrBuffer
                    );
          if (EFI_ERROR (Status)) {
            return Status;
          }

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

  This code gets the current status of Variable Store.

  @param VarStoreHeader  Pointer to the Variable Store Header.

  @retval EfiRaw         Variable store status is raw
  @retval EfiValid       Variable store status is valid
  @retval EfiInvalid     Variable store status is invalid

**/
VARIABLE_STORE_STATUS
GetVariableStoreStatus (
  IN VARIABLE_STORE_HEADER *VarStoreHeader
  )
{
  if (VarStoreHeader->Signature == VARIABLE_STORE_SIGNATURE &&
      VarStoreHeader->Format == VARIABLE_STORE_FORMATTED &&
      VarStoreHeader->State == VARIABLE_STORE_HEALTHY
      ) {

    return EfiValid;
  } else if (VarStoreHeader->Signature == 0xffffffff &&
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

  This code gets the size of name of variable.

  @param Variable        Pointer to the Variable Header

  @return UINTN          Size of variable in bytes

**/
UINTN
NameSizeOfVariable (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if (Variable->State    == (UINT8) (-1) ||
      Variable->DataSize == (UINT32) (-1) ||
      Variable->NameSize == (UINT32) (-1) ||
      Variable->Attributes == (UINT32) (-1)) {
    return 0;
  }
  return (UINTN) Variable->NameSize;
}

/**

  This code gets the size of variable data.

  @param Variable        Pointer to the Variable Header

  @return Size of variable in bytes

**/
UINTN
DataSizeOfVariable (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if (Variable->State    == (UINT8)  (-1) ||
      Variable->DataSize == (UINT32) (-1) ||
      Variable->NameSize == (UINT32) (-1) ||
      Variable->Attributes == (UINT32) (-1)) {
    return 0;
  }
  return (UINTN) Variable->DataSize;
}

/**

  This code gets the pointer to the variable name.

  @param Variable        Pointer to the Variable Header

  @return Pointer to Variable Name which is Unicode encoding

**/
CHAR16 *
GetVariableNamePtr (
  IN  VARIABLE_HEADER   *Variable
  )
{

  return (CHAR16 *) (Variable + 1);
}

/**

  This code gets the pointer to the variable data.

  @param Variable        Pointer to the Variable Header

  @return Pointer to Variable Data

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  UINTN Value;
  
  //
  // Be careful about pad size for alignment
  //
  Value =  (UINTN) GetVariableNamePtr (Variable);
  Value += NameSizeOfVariable (Variable);
  Value += GET_PAD_SIZE (NameSizeOfVariable (Variable));

  return (UINT8 *) Value;
}


/**

  This code gets the pointer to the next variable header.

  @param Variable        Pointer to the Variable Header

  @return Pointer to next variable header

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  UINTN Value;

  if (!IsValidVariableHeader (Variable)) {
    return NULL;
  }

  Value =  (UINTN) GetVariableDataPtr (Variable);
  Value += DataSizeOfVariable (Variable);
  Value += GET_PAD_SIZE (DataSizeOfVariable (Variable));

  //
  // Be careful about pad size for alignment
  //
  return (VARIABLE_HEADER *) HEADER_ALIGN (Value);
}

/**

  Gets the pointer to the first variable header in given variable store area.

  @param VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the first variable header

**/
VARIABLE_HEADER *
GetStartPointer (
  IN VARIABLE_STORE_HEADER       *VarStoreHeader
  )
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *) HEADER_ALIGN (VarStoreHeader + 1);
}

/**

  Gets the pointer to the end of the variable storage area.

  This function gets pointer to the end of the variable storage
  area, according to the input variable store header.

  @param VarStoreHeader  Pointer to the Variable Store Header

  @return Pointer to the end of the variable storage area  

**/
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER       *VarStoreHeader
  )
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *) HEADER_ALIGN ((UINTN) VarStoreHeader + VarStoreHeader->Size);
}


/**

  Variable store garbage collection and reclaim operation.

  @param VariableBase            Base address of variable store
  @param LastVariableOffset      Offset of last variable
  @param IsVolatile              The variable store is volatile or not,
                                 if it is non-volatile, need FTW
  @param UpdatingVariable        Pointer to updateing variable.

  @return EFI_OUT_OF_RESOURCES
  @return EFI_SUCCESS
  @return Others

**/
EFI_STATUS
Reclaim (
  IN  EFI_PHYSICAL_ADDRESS  VariableBase,
  OUT UINTN                 *LastVariableOffset,
  IN  BOOLEAN               IsVolatile,
  IN  VARIABLE_HEADER       *UpdatingVariable
  )
{
  VARIABLE_HEADER       *Variable;
  VARIABLE_HEADER       *AddedVariable;
  VARIABLE_HEADER       *NextVariable;
  VARIABLE_HEADER       *NextAddedVariable;
  VARIABLE_STORE_HEADER *VariableStoreHeader;
  UINT8                 *ValidBuffer;
  UINTN                 MaximumBufferSize;
  UINTN                 VariableSize;
  UINTN                 VariableNameSize;
  UINTN                 UpdatingVariableNameSize;
  UINTN                 NameSize;
  UINT8                 *CurrPtr;
  VOID                  *Point0;
  VOID                  *Point1;
  BOOLEAN               FoundAdded;
  EFI_STATUS            Status;
  CHAR16                *VariableNamePtr;
  CHAR16                *UpdatingVariableNamePtr;

  VariableStoreHeader = (VARIABLE_STORE_HEADER *) ((UINTN) VariableBase);

  //
  // Start Pointers for the variable.
  //
  Variable          = GetStartPointer (VariableStoreHeader);
  MaximumBufferSize = sizeof (VARIABLE_STORE_HEADER);

  while (IsValidVariableHeader (Variable)) {
    NextVariable = GetNextVariablePtr (Variable);
    if (Variable->State == VAR_ADDED || 
        Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)
       ) {
      VariableSize = (UINTN) NextVariable - (UINTN) Variable;
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
  CopyMem (ValidBuffer, VariableStoreHeader, sizeof (VARIABLE_STORE_HEADER));
  CurrPtr = (UINT8 *) GetStartPointer ((VARIABLE_STORE_HEADER *) ValidBuffer);

  //
  // Reinstall all ADDED variables as long as they are not identical to Updating Variable
  // 
  Variable = GetStartPointer (VariableStoreHeader);
  while (IsValidVariableHeader (Variable)) {
    NextVariable = GetNextVariablePtr (Variable);
    if (Variable->State == VAR_ADDED) {
      if (UpdatingVariable != NULL) {
        if (UpdatingVariable == Variable) {
          Variable = NextVariable;
          continue;
        }

        VariableNameSize         = NameSizeOfVariable(Variable);
        UpdatingVariableNameSize = NameSizeOfVariable(UpdatingVariable);

        VariableNamePtr         = GetVariableNamePtr (Variable);
        UpdatingVariableNamePtr = GetVariableNamePtr (UpdatingVariable);
        if (CompareGuid (&Variable->VendorGuid, &UpdatingVariable->VendorGuid)    &&
            VariableNameSize == UpdatingVariableNameSize &&
            CompareMem (VariableNamePtr, UpdatingVariableNamePtr, VariableNameSize) == 0 ) {
          Variable = NextVariable;
          continue;
        }
      }
      VariableSize = (UINTN) NextVariable - (UINTN) Variable;
      CopyMem (CurrPtr, (UINT8 *) Variable, VariableSize);
      CurrPtr += VariableSize;
    }
    Variable = NextVariable;
  }

  //
  // Reinstall the variable being updated if it is not NULL
  //
  if (UpdatingVariable != NULL) {
    VariableSize = (UINTN)(GetNextVariablePtr (UpdatingVariable)) - (UINTN)UpdatingVariable;
    CopyMem (CurrPtr, (UINT8 *) UpdatingVariable, VariableSize);
    CurrPtr += VariableSize;
  }

  //
  // Reinstall all in delete transition variables
  // 
  Variable      = GetStartPointer (VariableStoreHeader);
  while (IsValidVariableHeader (Variable)) {
    NextVariable = GetNextVariablePtr (Variable);
    if (Variable != UpdatingVariable && Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {

      //
      // Buffer has cached all ADDED variable. 
      // Per IN_DELETED variable, we have to guarantee that
      // no ADDED one in previous buffer. 
      // 
     
      FoundAdded = FALSE;
      AddedVariable = GetStartPointer ((VARIABLE_STORE_HEADER *) ValidBuffer);
      while (IsValidVariableHeader (AddedVariable)) {
        NextAddedVariable = GetNextVariablePtr (AddedVariable);
        NameSize = NameSizeOfVariable (AddedVariable);
        if (CompareGuid (&AddedVariable->VendorGuid, &Variable->VendorGuid) &&
            NameSize == NameSizeOfVariable (Variable)
           ) {
          Point0 = (VOID *) GetVariableNamePtr (AddedVariable);
          Point1 = (VOID *) GetVariableNamePtr (Variable);
          if (CompareMem (Point0, Point1, NameSizeOfVariable (AddedVariable)) == 0) {
            FoundAdded = TRUE;
            break;
          }
        }
        AddedVariable = NextAddedVariable;
      }
      if (!FoundAdded) {
        //
        // Promote VAR_IN_DELETED_TRANSITION to VAR_ADDED
        //
        VariableSize = (UINTN) NextVariable - (UINTN) Variable;
        CopyMem (CurrPtr, (UINT8 *) Variable, VariableSize);
        ((VARIABLE_HEADER *) CurrPtr)->State = VAR_ADDED;
        CurrPtr += VariableSize;
      }
    }

    Variable = NextVariable;
  }

  if (IsVolatile) {
    //
    // If volatile variable store, just copy valid buffer
    //
    SetMem ((UINT8 *) (UINTN) VariableBase, VariableStoreHeader->Size, 0xff);
    CopyMem ((UINT8 *) (UINTN) VariableBase, ValidBuffer, (UINTN) (CurrPtr - (UINT8 *) ValidBuffer));
    Status              = EFI_SUCCESS;
  } else {
    //
    // If non-volatile variable store, perform FTW here.
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
  Update the Cache with Variable information. These are the same 
  arguments as the EFI Variable services.

  @param[in] VariableName  Name of variable
  @param[in] VendorGuid    Guid of variable
  @param[in] Attributes    Attribues of the variable
  @param[in] DataSize      Size of data. 0 means delete
  @param[in] Data          Variable data

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
    // Don't use the cache at runtime
    return;
  }

  for (Index = 0, Entry = mVariableCache; Index < sizeof (mVariableCache)/sizeof (VARIABLE_CACHE_ENTRY); Index++, Entry++) {
    if (CompareGuid (VendorGuid, Entry->Guid)) {
      if (StrCmp (VariableName, Entry->Name) == 0) { 
        Entry->Attributes = Attributes;
        if (DataSize == 0) {
          // Delete Case
          if (Entry->DataSize != 0) {
            FreePool (Entry->Data);
          }
          Entry->DataSize = DataSize;
        } else if (DataSize == Entry->DataSize) {
          CopyMem (Entry->Data, Data, DataSize);
        } else {
          Entry->Data = AllocatePool (DataSize);
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

  @param  VariableName          A Null-terminated Unicode string that is the name of the vendor's
                                variable.  Each VariableName is unique for each 
                                VendorGuid.
  @param  VendorGuid            A unique identifier for the vendor
  @param  Attributes            Pointer to the attributes bitmask of the variable for output.
  @param  DataSize              On input, size of the buffer of Data.
                                On output, size of the variable's data.
  @param  Data                  Pointer to the data buffer for output.

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
    // Don't use the cache at runtime
    return EFI_NOT_FOUND;
  }

  for (Index = 0, Entry = mVariableCache; Index < sizeof (mVariableCache)/sizeof (VARIABLE_CACHE_ENTRY); Index++, Entry++) {
    if (CompareGuid (VendorGuid, Entry->Guid)) {
      if (StrCmp (VariableName, Entry->Name) == 0) {
        if (Entry->DataSize == 0) {
          // Variable was deleted so return not found
          return EFI_NOT_FOUND;
        } else if (Entry->DataSize > *DataSize) {
          // If the buffer is too small return correct size
          *DataSize = Entry->DataSize;
          return EFI_BUFFER_TOO_SMALL;
        } else {
          *DataSize = Entry->DataSize;
          // Return the data
          CopyMem (Data, Entry->Data, Entry->DataSize);
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
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  Otherwise, VariableName and VendorGuid are compared.

  @param  VariableName                Name of the variable to be found
  @param  VendorGuid                  Vendor GUID to be found.
  @param  PtrTrack                    VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.
  @param  Global                      Pointer to VARIABLE_GLOBAL structure, including
                                      base of volatile variable storage area, base of
                                      NV variable storage area, and a lock.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL
  @retval EFI_SUCCESS                 Variable successfully found
  @retval EFI_INVALID_PARAMETER       Variable not found

**/
EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global
  )
{
  VARIABLE_HEADER         *Variable[2];
  VARIABLE_HEADER         *InDeletedVariable;
  VARIABLE_STORE_HEADER   *VariableStoreHeader[2];
  UINTN                   InDeletedStorageIndex;
  UINTN                   Index;
  VOID                    *Point;

  //
  // 0: Volatile, 1: Non-Volatile
  // The index and attributes mapping must be kept in this order as RuntimeServiceGetNextVariableName
  // make use of this mapping to implement search algorithme.
  //
  VariableStoreHeader[0]  = (VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal.VolatileVariableBase);
  VariableStoreHeader[1]  = (VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase);

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
  InDeletedVariable     = NULL;
  InDeletedStorageIndex = 0;
  for (Index = 0; Index < 2; Index++) {
    while (IsValidVariableHeader (Variable[Index]) && (Variable[Index] <= GetEndPointer (VariableStoreHeader[Index]))) {
      if (Variable[Index]->State == VAR_ADDED || 
          Variable[Index]->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)
         ) {
        if (!EfiAtRuntime () || (Variable[Index]->Attributes & EFI_VARIABLE_RUNTIME_ACCESS)) {
          if (VariableName[0] == 0) {
            if (Variable[Index]->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
              InDeletedVariable     = Variable[Index];
              InDeletedStorageIndex = Index;
            } else {
              PtrTrack->StartPtr  = GetStartPointer (VariableStoreHeader[Index]);
              PtrTrack->EndPtr    = GetEndPointer (VariableStoreHeader[Index]);
              PtrTrack->CurrPtr   = Variable[Index];
              PtrTrack->Volatile  = (BOOLEAN)(Index == 0);

              return EFI_SUCCESS;
            }
          } else {
            if (CompareGuid (VendorGuid, &Variable[Index]->VendorGuid)) {
              Point = (VOID *) GetVariableNamePtr (Variable[Index]);

              ASSERT (NameSizeOfVariable (Variable[Index]) != 0);
              if (!CompareMem (VariableName, Point, NameSizeOfVariable (Variable[Index]))) {
                if (Variable[Index]->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
                  InDeletedVariable     = Variable[Index];
                  InDeletedStorageIndex = Index;
                } else {
                  PtrTrack->StartPtr  = GetStartPointer (VariableStoreHeader[Index]);
                  PtrTrack->EndPtr    = GetEndPointer (VariableStoreHeader[Index]);
                  PtrTrack->CurrPtr   = Variable[Index];
                  PtrTrack->Volatile  = (BOOLEAN)(Index == 0);

                  return EFI_SUCCESS;
                }
              }
            }
          }
        }
      }

      Variable[Index] = GetNextVariablePtr (Variable[Index]);
    }
    if (InDeletedVariable != NULL) {
      PtrTrack->StartPtr  = GetStartPointer (VariableStoreHeader[InDeletedStorageIndex]);
      PtrTrack->EndPtr    = GetEndPointer (VariableStoreHeader[InDeletedStorageIndex]);
      PtrTrack->CurrPtr   = InDeletedVariable;
      PtrTrack->Volatile  = (BOOLEAN)(InDeletedStorageIndex == 0);
      return EFI_SUCCESS;
    }
  }
  PtrTrack->CurrPtr = NULL;
  return EFI_NOT_FOUND;
}


/**

  This code finds variable in storage blocks (Volatile or Non-Volatile).

  @param VariableName               Name of Variable to be found.
  @param VendorGuid                 Variable vendor GUID.
  @param Attributes                 Attribute value of the variable found.
  @param DataSize                   Size of Data found. If size is less than the
                                    data, this value contains the required size.
  @param Data                       Data pointer.
                      
  @return EFI_INVALID_PARAMETER     Invalid parameter
  @return EFI_SUCCESS               Find the specified variable
  @return EFI_NOT_FOUND             Not found
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result

**/
EFI_STATUS
EFIAPI
RuntimeServiceGetVariable (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          *VendorGuid,
  OUT     UINT32            *Attributes OPTIONAL,
  IN OUT  UINTN             *DataSize,
  OUT     VOID              *Data
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarDataSize;

  if (VariableName == NULL || VendorGuid == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  //
  // Find existing variable
  //
  Status = FindVariableInCache (VariableName, VendorGuid, Attributes, DataSize, Data);
  if ((Status == EFI_BUFFER_TOO_SMALL) || (Status == EFI_SUCCESS)){
    // Hit in the Cache
    UpdateVariableInfo (VariableName, VendorGuid, FALSE, TRUE, FALSE, FALSE, TRUE);
    goto Done;
  }
  
  Status = FindVariable (VariableName, VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal);
  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Get data size
  //
  VarDataSize = DataSizeOfVariable (Variable.CurrPtr);
  ASSERT (VarDataSize != 0);

  if (*DataSize >= VarDataSize) {
    if (Data == NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    CopyMem (Data, GetVariableDataPtr (Variable.CurrPtr), VarDataSize);
    if (Attributes != NULL) {
      *Attributes = Variable.CurrPtr->Attributes;
    }

    *DataSize = VarDataSize;
    UpdateVariableInfo (VariableName, VendorGuid, Variable.Volatile, TRUE, FALSE, FALSE, FALSE);
    UpdateVariableCache (VariableName, VendorGuid, Variable.CurrPtr->Attributes, VarDataSize, Data);
 
    Status = EFI_SUCCESS;
    goto Done;
  } else {
    *DataSize = VarDataSize;
    Status = EFI_BUFFER_TOO_SMALL;
    goto Done;
  }

Done:
  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
  return Status;
}



/**

  This code Finds the Next available variable.

  @param VariableNameSize           Size of the variable name
  @param VariableName               Pointer to variable name
  @param VendorGuid                 Variable Vendor Guid

  @return EFI_INVALID_PARAMETER     Invalid parameter
  @return EFI_SUCCESS               Find the specified variable
  @return EFI_NOT_FOUND             Not found
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result

**/
EFI_STATUS
EFIAPI
RuntimeServiceGetNextVariableName (
  IN OUT  UINTN             *VariableNameSize,
  IN OUT  CHAR16            *VariableName,
  IN OUT  EFI_GUID          *VendorGuid
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarNameSize;
  EFI_STATUS              Status;

  if (VariableNameSize == NULL || VariableName == NULL || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  Status = FindVariable (VariableName, VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal);
  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    goto Done;
  }

  if (VariableName[0] != 0) {
    //
    // If variable name is not NULL, get next variable
    //
    Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
  }

  while (TRUE) {
    //
    // If both volatile and non-volatile variable store are parsed,
    // return not found
    //
    if (Variable.CurrPtr >= Variable.EndPtr || Variable.CurrPtr == NULL) {
      Variable.Volatile = (BOOLEAN) (Variable.Volatile ^ ((BOOLEAN) 0x1));
      if (!Variable.Volatile) {
        Variable.StartPtr = GetStartPointer ((VARIABLE_STORE_HEADER *) (UINTN) mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase);
        Variable.EndPtr   = GetEndPointer ((VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase));
      } else {
        Status = EFI_NOT_FOUND;
        goto Done;
      }

      Variable.CurrPtr = Variable.StartPtr;
      if (!IsValidVariableHeader (Variable.CurrPtr)) {
        continue;
      }
    }
    //
    // Variable is found
    //
    if (IsValidVariableHeader (Variable.CurrPtr) && Variable.CurrPtr->State == VAR_ADDED) {
      if (!(EfiAtRuntime () && !(Variable.CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS))) {
        VarNameSize = NameSizeOfVariable (Variable.CurrPtr);
        ASSERT (VarNameSize != 0);

        if (VarNameSize <= *VariableNameSize) {
          CopyMem (
            VariableName,
            GetVariableNamePtr (Variable.CurrPtr),
            VarNameSize
            );
          CopyMem (
            VendorGuid,
            &Variable.CurrPtr->VendorGuid,
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

    Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
  }

Done:
  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
  return Status;
}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  @param VariableName                     Name of Variable to be found
  @param VendorGuid                       Variable vendor GUID
  @param Attributes                       Attribute value of the variable found
  @param DataSize                         Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param Data                             Data pointer

  @return EFI_INVALID_PARAMETER           Invalid parameter
  @return EFI_SUCCESS                     Set successfully
  @return EFI_OUT_OF_RESOURCES            Resource not enough to set variable
  @return EFI_NOT_FOUND                   Not found
  @return EFI_WRITE_PROTECTED             Variable is read-only

**/
EFI_STATUS
EFIAPI
RuntimeServiceSetVariable (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINT32                  Attributes,
  IN UINTN                   DataSize,
  IN VOID                    *Data
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  EFI_STATUS              Status;
  VARIABLE_HEADER         *NextVariable;
  UINTN                   VarNameSize;
  UINTN                   VarNameOffset;
  UINTN                   VarDataOffset;
  UINTN                   VarSize;
  UINT8                   State;
  BOOLEAN                 Reclaimed;
  UINTN                   *VolatileOffset;
  UINTN                   *NonVolatileOffset;
  UINT32                  Instance;
  BOOLEAN                 Volatile;
  EFI_PHYSICAL_ADDRESS    Point;

  //
  // Check input parameters
  //
  if (VariableName == NULL || VariableName[0] == 0 || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }  
  //
  //  Make sure if runtime bit is set, boot service bit is set also
  //
  if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    return EFI_INVALID_PARAMETER;
  }

  //
  //  The size of the VariableName, including the Unicode Null in bytes plus
  //  the DataSize is limited to maximum size of FixedPcdGet32(PcdMaxHardwareErrorVariableSize)
  //  bytes for HwErrRec, and FixedPcdGet32(PcdMaxVariableSize) bytes for the others.
  //
  if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    if ((DataSize > FixedPcdGet32(PcdMaxHardwareErrorVariableSize)) ||                                                       
        (sizeof (VARIABLE_HEADER) + StrSize (VariableName) + DataSize > FixedPcdGet32(PcdMaxHardwareErrorVariableSize))) {
      return EFI_INVALID_PARAMETER;
    }    
  } else {
  //
  //  The size of the VariableName, including the Unicode Null in bytes plus
  //  the DataSize is limited to maximum size of FixedPcdGet32(PcdMaxVariableSize) bytes.
  //
    if ((DataSize > FixedPcdGet32(PcdMaxVariableSize)) ||
        (sizeof (VARIABLE_HEADER) + StrSize (VariableName) + DataSize > FixedPcdGet32(PcdMaxVariableSize))) {
      return EFI_INVALID_PARAMETER;
    }  
  }  

  AcquireLockOnlyAtBootTime(&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  Reclaimed         = FALSE;
  Instance          = mVariableModuleGlobal->FvbInstance;
  VolatileOffset    = &mVariableModuleGlobal->VolatileLastVariableOffset;

  //
  // Consider reentrant in MCA/INIT/NMI. It needs be reupdated;
  //
  if (1 < InterlockedIncrement (&mVariableModuleGlobal->VariableGlobal.ReentrantState)) {
    Point = mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase;;
    //
    // Parse non-volatile variable data and get last variable offset
    //
    NextVariable  = GetStartPointer ((VARIABLE_STORE_HEADER *) (UINTN) Point);
    while (IsValidVariableHeader (NextVariable)) {
      NextVariable = GetNextVariablePtr (NextVariable);
    }
    mVariableModuleGlobal->NonVolatileLastVariableOffset = (UINTN) NextVariable - (UINTN) Point;
  }

  NonVolatileOffset = &mVariableModuleGlobal->NonVolatileLastVariableOffset;
  

  //
  // Check whether the input variable is already existed
  //
  
  Status = FindVariable (VariableName, VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal);
  if (Status == EFI_SUCCESS && Variable.CurrPtr != NULL) {
    //
    // Update/Delete existing variable
    //
    Volatile = Variable.Volatile;
    
    if (EfiAtRuntime ()) {        
      //
      // If EfiAtRuntime and the variable is Volatile and Runtime Access,  
      // the volatile is ReadOnly, and SetVariable should be aborted and 
      // return EFI_WRITE_PROTECTED.
      //
      if (Variable.Volatile) {
        Status = EFI_WRITE_PROTECTED;
        goto Done;
      }
      //
      // Only variable have NV attribute can be updated/deleted in Runtime
      //
      if (!(Variable.CurrPtr->Attributes & EFI_VARIABLE_NON_VOLATILE)) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;      
      }
    }
    //
    // Setting a data variable with no access, or zero DataSize attributes
    // specified causes it to be deleted.
    //
    if (DataSize == 0 || (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {    
      State = Variable.CurrPtr->State;
      State &= VAR_DELETED;

      Status = UpdateVariableStore (
                 &mVariableModuleGlobal->VariableGlobal,
                 Variable.Volatile,
                 FALSE,
                 Instance,
                 (UINTN) &Variable.CurrPtr->State,
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
    // If the variable is marked valid and the same data has been passed in
    // then return to the caller immediately.
    //
    if (DataSizeOfVariable (Variable.CurrPtr) == DataSize &&
        (CompareMem (Data, GetVariableDataPtr (Variable.CurrPtr), DataSize) == 0)) {
      
      UpdateVariableInfo (VariableName, VendorGuid, Volatile, FALSE, TRUE, FALSE, FALSE);
      Status = EFI_SUCCESS;
      goto Done;
    } else if ((Variable.CurrPtr->State == VAR_ADDED) ||
               (Variable.CurrPtr->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION))) {

      //
      // Mark the old variable as in delete transition
      //
      State = Variable.CurrPtr->State;
      State &= VAR_IN_DELETED_TRANSITION;

      Status = UpdateVariableStore (
                 &mVariableModuleGlobal->VariableGlobal,
                 Variable.Volatile,
                 FALSE,
                 Instance,
                 (UINTN) &Variable.CurrPtr->State,
                 sizeof (UINT8),
                 &State
                 );      
      if (EFI_ERROR (Status)) {
        goto Done;  
      } 
    }    
  } else if (Status == EFI_NOT_FOUND) {
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
  } else {
    //
    // Status should be EFI_INVALID_PARAMETER here according to return status of FindVariable().
    //
    ASSERT (Status == EFI_INVALID_PARAMETER);
    goto Done;
  }

  //
  // Function part - create a new variable and copy the data.
  // Both update a variable and create a variable will come here.
  //
  // Tricky part: Use scratch data area at the end of volatile variable store
  // as a temporary storage.
  //
  NextVariable = GetEndPointer ((VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal.VolatileVariableBase));

  SetMem (NextVariable, FixedPcdGet32(PcdMaxVariableSize), 0xff);

  NextVariable->StartId     = VARIABLE_DATA;
  NextVariable->Attributes  = Attributes;
  //
  // NextVariable->State = VAR_ADDED;
  //
  NextVariable->Reserved  = 0;
  VarNameOffset           = sizeof (VARIABLE_HEADER);
  VarNameSize             = StrSize (VariableName);
  CopyMem (
    (UINT8 *) ((UINTN) NextVariable + VarNameOffset),
    VariableName,
    VarNameSize
    );
  VarDataOffset = VarNameOffset + VarNameSize + GET_PAD_SIZE (VarNameSize);
  CopyMem (
    (UINT8 *) ((UINTN) NextVariable + VarDataOffset),
    Data,
    DataSize
    );
  CopyMem (&NextVariable->VendorGuid, VendorGuid, sizeof (EFI_GUID));
  //
  // There will be pad bytes after Data, the NextVariable->NameSize and
  // NextVariable->DataSize should not include pad size so that variable
  // service can get actual size in GetVariable
  //
  NextVariable->NameSize  = (UINT32)VarNameSize;
  NextVariable->DataSize  = (UINT32)DataSize;

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
    
    if ((UINT32) (VarSize +*NonVolatileOffset) >
          ((VARIABLE_STORE_HEADER *) ((UINTN) (mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase)))->Size
          ) {
      if (EfiAtRuntime ()) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      //
      // Perform garbage collection & reclaim operation
      //
      Status = Reclaim (mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase, NonVolatileOffset, FALSE, Variable.CurrPtr);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      //
      // If still no enough space, return out of resources
      //
      if ((UINT32) (VarSize +*NonVolatileOffset) >
            ((VARIABLE_STORE_HEADER *) ((UINTN) (mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase)))->Size
            ) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      
      Reclaimed = TRUE;
    }
    //
    // Three steps
    // 1. Write variable header
    // 2. Set variable state to header valid  
    // 3. Write variable data
    // 4. Set variable state to valid
    //
    //
    // Step 1:
    //
    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               FALSE,
               TRUE,
               Instance,
               *NonVolatileOffset,
               sizeof (VARIABLE_HEADER),
               (UINT8 *) NextVariable
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Step 2:
    //
    NextVariable->State = VAR_HEADER_VALID_ONLY;
    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               FALSE,
               TRUE,
               Instance,
               *NonVolatileOffset,
               sizeof (VARIABLE_HEADER),
               (UINT8 *) NextVariable
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }
    //
    // Step 3:
    //
    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               FALSE,
               TRUE,
               Instance,
               *NonVolatileOffset + sizeof (VARIABLE_HEADER),
               (UINT32) VarSize - sizeof (VARIABLE_HEADER),
               (UINT8 *) NextVariable + sizeof (VARIABLE_HEADER)
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }
    //
    // Step 4:
    //
    NextVariable->State = VAR_ADDED;
    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               FALSE,
               TRUE,
               Instance,
               *NonVolatileOffset,
               sizeof (VARIABLE_HEADER),
               (UINT8 *) NextVariable
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    *NonVolatileOffset = HEADER_ALIGN (*NonVolatileOffset + VarSize);

  } else {
    //
    // Create a volatile variable
    //      
    Volatile = TRUE;

    if ((UINT32) (VarSize +*VolatileOffset) >
        ((VARIABLE_STORE_HEADER *) ((UINTN) (mVariableModuleGlobal->VariableGlobal.VolatileVariableBase)))->Size) {
      //
      // Perform garbage collection & reclaim operation
      //
      Status = Reclaim (mVariableModuleGlobal->VariableGlobal.VolatileVariableBase, VolatileOffset, TRUE, Variable.CurrPtr);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      //
      // If still no enough space, return out of resources
      //
      if ((UINT32) (VarSize +*VolatileOffset) >
            ((VARIABLE_STORE_HEADER *) ((UINTN) (mVariableModuleGlobal->VariableGlobal.VolatileVariableBase)))->Size
            ) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      
      Reclaimed = TRUE;
    }

    NextVariable->State = VAR_ADDED;
    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               TRUE,
               TRUE,
               Instance,
               *VolatileOffset,
               (UINT32) VarSize,
               (UINT8 *) NextVariable
               );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    *VolatileOffset = HEADER_ALIGN (*VolatileOffset + VarSize);
  }
  //
  // Mark the old variable as deleted
  //
  if (!Reclaimed && !EFI_ERROR (Status) && Variable.CurrPtr != NULL) {
    State = Variable.CurrPtr->State;
    State &= VAR_DELETED;

    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               Variable.Volatile,
               FALSE,
               Instance,
               (UINTN) &Variable.CurrPtr->State,
               sizeof (UINT8),
               &State
               );
    
    if (!EFI_ERROR (Status)) {
      UpdateVariableInfo (VariableName, VendorGuid, Volatile, FALSE, TRUE, FALSE, FALSE);
      UpdateVariableCache (VariableName, VendorGuid, Attributes, DataSize, Data);
    }
    goto Done;      
  }

  Status = EFI_SUCCESS;
  UpdateVariableInfo (VariableName, VendorGuid, Volatile, FALSE, TRUE, FALSE, FALSE);
  UpdateVariableCache (VariableName, VendorGuid, Attributes, DataSize, Data);

Done:
  InterlockedDecrement (&mVariableModuleGlobal->VariableGlobal.ReentrantState);
  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  return Status;
}

/**

  This code returns information about the EFI variables.

  @param Attributes                     Attributes bitmask to specify the type of variables
                                        on which to return information.
  @param MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                        for the EFI variables associated with the attributes specified.
  @param RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                        for EFI variables associated with the attributes specified.
  @param MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                        associated with the attributes specified.

  @return EFI_INVALID_PARAMETER         An invalid combination of attribute bits was supplied.
  @return EFI_SUCCESS                   Query successfully.
  @return EFI_UNSUPPORTED               The attribute is not supported on this platform.

**/
EFI_STATUS
EFIAPI
RuntimeServiceQueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize
  )
{
  VARIABLE_HEADER        *Variable;
  VARIABLE_HEADER        *NextVariable;
  UINT64                 VariableSize;
  VARIABLE_STORE_HEADER  *VariableStoreHeader;

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
  }

  AcquireLockOnlyAtBootTime(&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  if((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
    //
    // Query is Volatile related.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal.VolatileVariableBase);
  } else {
    //
    // Query is Non-Volatile related.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase);
  }

  //
  // Now let's fill *MaximumVariableStorageSize *RemainingVariableStorageSize
  // with the storage size (excluding the storage header size).
  //
  *MaximumVariableStorageSize   = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER);
  *RemainingVariableStorageSize = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER);

  //
  // Let *MaximumVariableSize be FixedPcdGet32(PcdMaxVariableSize) with the exception of the variable header size.
  //
  *MaximumVariableSize = FixedPcdGet32(PcdMaxVariableSize) - sizeof (VARIABLE_HEADER);

  //
  // Harware error record variable needs larger size.
  //
  if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    *MaximumVariableSize = FixedPcdGet32(PcdMaxHardwareErrorVariableSize) - sizeof (VARIABLE_HEADER);
  }

  //
  // Point to the starting address of the variables.
  //
  Variable = GetStartPointer (VariableStoreHeader);

  //
  // Now walk through the related variable store.
  //
  while (IsValidVariableHeader (Variable) && (Variable < GetEndPointer (VariableStoreHeader))) {
    NextVariable = GetNextVariablePtr (Variable);
    VariableSize = (UINT64) (UINTN) NextVariable - (UINT64) (UINTN) Variable;

    if (EfiAtRuntime ()) {
      //
      // we don't take the state of the variables in mind
      // when calculating RemainingVariableStorageSize,
      // since the space occupied by variables not marked with
      // VAR_ADDED is not allowed to be reclaimed in Runtime.
      //
      *RemainingVariableStorageSize -= VariableSize;
    } else {
      //
      // Only care about Variables with State VAR_ADDED,because
      // the space not marked as VAR_ADDED is reclaimable now.
      //
      if (Variable->State == VAR_ADDED) {
        *RemainingVariableStorageSize -= VariableSize;
      }
    }

    //
    // Go to the next one
    //
    Variable = NextVariable;
  }

  if (*RemainingVariableStorageSize < sizeof (VARIABLE_HEADER)) {
    *MaximumVariableSize = 0;
  } else if ((*RemainingVariableStorageSize - sizeof (VARIABLE_HEADER)) < *MaximumVariableSize) {
    *MaximumVariableSize = *RemainingVariableStorageSize - sizeof (VARIABLE_HEADER);
  }

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
  return EFI_SUCCESS;
}


/**
  Notification function of EVT_GROUP_READY_TO_BOOT event group.

  This is a notification function registered on EVT_GROUP_READY_TO_BOOT event group.
  When the Boot Manager is about to load and execute a boot option, it reclaims variable
  storage if free size is below the threshold.

  @param  Event        Event whose notification function is being invoked
  @param  Context      Pointer to the notification function's context

**/
VOID
EFIAPI
ReclaimForOS(
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  UINT32                          VarSize;
  EFI_STATUS                      Status;

  VarSize = ((VARIABLE_STORE_HEADER *) ((UINTN) mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase))->Size;
  Status  = EFI_SUCCESS; 

  //
  // Check if the free area is blow a threshold
  //
  if ((VarSize - mVariableModuleGlobal->NonVolatileLastVariableOffset) < VARIABLE_RECLAIM_THRESHOLD) {
    Status = Reclaim (
              mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
              &mVariableModuleGlobal->NonVolatileLastVariableOffset,
              FALSE,
              NULL
              );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Initializes variable store area for non-volatile and volatile variable.

  @param  ImageHandle           The Image handle of this driver.
  @param  SystemTable           The pointer of EFI_SYSTEM_TABLE.

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
  CHAR8                           *CurrPtr;
  VARIABLE_STORE_HEADER           *VolatileVariableStore;
  VARIABLE_STORE_HEADER           *VariableStoreHeader;
  VARIABLE_HEADER                 *NextVariable;
  UINT32                          Instance;
  EFI_PHYSICAL_ADDRESS            FvVolHdr;
  UINT64                          TempVariableStoreHeader;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR GcdDescriptor;
  UINT64                          BaseAddress;
  UINT64                          Length;
  UINTN                           Index;
  UINT8                           Data;
  UINT64                          VariableStoreBase;
  UINT64                          VariableStoreLength;
  EFI_EVENT                       ReadyToBootEvent;

  Status = EFI_SUCCESS;
  //
  // Allocate runtime memory for variable driver global structure.
  //
  mVariableModuleGlobal = AllocateRuntimePool (sizeof (VARIABLE_MODULE_GLOBAL));
  if (mVariableModuleGlobal == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EfiInitializeLock(&mVariableModuleGlobal->VariableGlobal.VariableServicesLock, TPL_NOTIFY);
  mVariableModuleGlobal->VariableGlobal.ReentrantState = 0;

  //
  // Allocate memory for volatile variable store
  //
  VolatileVariableStore = AllocateRuntimePool (FixedPcdGet32(PcdVariableStoreSize) + FixedPcdGet32(PcdMaxVariableSize));
  if (VolatileVariableStore == NULL) {
    FreePool (mVariableModuleGlobal);
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (VolatileVariableStore, FixedPcdGet32(PcdVariableStoreSize) + FixedPcdGet32(PcdMaxVariableSize), 0xff);

  //
  //  Variable Specific Data
  //
  mVariableModuleGlobal->VariableGlobal.VolatileVariableBase = (EFI_PHYSICAL_ADDRESS) (UINTN) VolatileVariableStore;
  mVariableModuleGlobal->VolatileLastVariableOffset = (UINTN) GetStartPointer (VolatileVariableStore) - (UINTN) VolatileVariableStore;

  VolatileVariableStore->Signature                  = VARIABLE_STORE_SIGNATURE;
  VolatileVariableStore->Size                       = FixedPcdGet32(PcdVariableStoreSize);
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
  // Get address of non volatile variable store base
  //
  mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase = VariableStoreBase;

  //
  // Check Integrity
  //
  //
  // Find the Correct Instance of the FV Block Service.
  //
  Instance  = 0;
  CurrPtr   = (CHAR8 *) ((UINTN) mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase);
  while (EfiFvbGetPhysicalAddress (Instance, &FvVolHdr) == EFI_SUCCESS) {
    FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINTN) FvVolHdr);
    if (CurrPtr >= (CHAR8 *) FwVolHeader && CurrPtr < (((CHAR8 *) FwVolHeader) + FwVolHeader->FvLength)) {
      mVariableModuleGlobal->FvbInstance = Instance;
      break;
    }

    Instance++;
  }

  VariableStoreHeader = (VARIABLE_STORE_HEADER *) CurrPtr;
  if (GetVariableStoreStatus (VariableStoreHeader) == EfiValid) {
    if (~VariableStoreHeader->Size == 0) {
      Status = UpdateVariableStore (
                &mVariableModuleGlobal->VariableGlobal,
                FALSE,
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
      // If write fail, we will assert here
      //
      ASSERT(VariableStoreHeader->Size == VariableStoreLength);

      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }

    mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase = (EFI_PHYSICAL_ADDRESS) ((UINTN) CurrPtr);
    //
    // Parse non-volatile variable data and get last variable offset
    //
    NextVariable  = GetStartPointer ((VARIABLE_STORE_HEADER *) CurrPtr);
    Status        = EFI_SUCCESS;

    while (IsValidVariableHeader (NextVariable)) {
      NextVariable = GetNextVariablePtr (NextVariable);
    }

    mVariableModuleGlobal->NonVolatileLastVariableOffset = (UINTN) NextVariable - (UINTN) CurrPtr;

    //
    // Check if the free area is really free.
    //
    for (Index = mVariableModuleGlobal->NonVolatileLastVariableOffset; Index < VariableStoreHeader->Size; Index++) {
      Data = ((UINT8 *) (UINTN) mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase)[Index];
      if (Data != 0xff) {
        //
        // There must be something wrong in variable store, do reclaim operation.
        //
        Status = Reclaim (
                  mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
                  &mVariableModuleGlobal->NonVolatileLastVariableOffset,
                  FALSE,
                  NULL
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
  }

Done:
  if (EFI_ERROR (Status)) {
    FreePool (mVariableModuleGlobal);
    FreePool (VolatileVariableStore);
  }

  return Status;
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked
  @param  Context      Pointer to the notification function's context

**/
VOID
EFIAPI
VariableClassAddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EfiConvertPointer (
    0x0,
    (VOID **) &mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase
    );
  EfiConvertPointer (
    0x0,
    (VOID **) &mVariableModuleGlobal->VariableGlobal.VolatileVariableBase
    );
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal);
}


/**
  Variable Driver main entry point. The Variable driver places the 4 EFI
  runtime services in the EFI System Table and installs arch protocols 
  for variable read and write services being availible. It also registers
  notification function for EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       Variable service successfully initialized.

**/
EFI_STATUS
EFIAPI
VariableServiceInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = VariableCommonInitialize (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);

  SystemTable->RuntimeServices->GetVariable         = RuntimeServiceGetVariable;
  SystemTable->RuntimeServices->GetNextVariableName = RuntimeServiceGetNextVariableName;
  SystemTable->RuntimeServices->SetVariable         = RuntimeServiceSetVariable;
  SystemTable->RuntimeServices->QueryVariableInfo   = RuntimeServiceQueryVariableInfo;

  //
  // Now install the Variable Runtime Architectural Protocol on a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEfiVariableArchProtocolGuid,        NULL,
                  &gEfiVariableWriteArchProtocolGuid,   NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  TPL_NOTIFY,
                  VariableClassAddressChangeEvent,
                  NULL,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

