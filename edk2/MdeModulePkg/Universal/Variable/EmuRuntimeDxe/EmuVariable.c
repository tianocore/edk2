/** @file

  Emulation Variable services operate on the runtime volatile memory.
  The nonvolatile variable space doesn't exist.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"

///
/// Don't use module globals after the SetVirtualAddress map is signaled
///
ESAL_VARIABLE_GLOBAL  *mVariableModuleGlobal;


/**
  Acquires lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiAcquireLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiAcquireLock() at boot time, and simply returns
  at runtime

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
  at runtime

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
  Gets pointer to the variable data.

  This function gets the pointer to the variable data according
  to the input pointer to the variable header.

  @param  Variable      Pointer to the variable header.

  @return Pointer to variable data

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if (Variable->StartId != VARIABLE_DATA) {
    return NULL;
  }
  //
  // Be careful about pad size for alignment
  //
  return (UINT8 *) ((UINTN) GET_VARIABLE_NAME_PTR (Variable) + Variable->NameSize + GET_PAD_SIZE (Variable->NameSize));
}

/**
  Gets pointer to header of the next variable.

  This function gets the pointer to the next variable header according
  to the input point to the variable header.

  @param  Variable      Pointer to header of the next variable

  @return Pointer to next variable header.

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  VARIABLE_HEADER *VarHeader;

  if (Variable->StartId != VARIABLE_DATA) {
    return NULL;
  }
  //
  // Be careful about pad size for alignment
  //
  VarHeader = (VARIABLE_HEADER *) (GetVariableDataPtr (Variable) + Variable->DataSize + GET_PAD_SIZE (Variable->DataSize));

  if (VarHeader->StartId != VARIABLE_DATA ||
      (sizeof (VARIABLE_HEADER) + VarHeader->DataSize + VarHeader->NameSize) > FixedPcdGet32(PcdMaxVariableSize)
      ) {
    return NULL;
  }

  return VarHeader;
}

/**
  Gets pointer to the end of the variable storage area.

  This function gets pointer to the end of the variable storage
  area, according to the input variable store header.

  @param  VolHeader     Pointer to the variale store header

  @return Pointer to the end of the variable storage area.

**/
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER       *VolHeader
  )
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *) ((UINTN) VolHeader + VolHeader->Size);
}

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  Otherwise, VariableName and VendorGuid are compared.

  @param  VariableName                Name of the variable to be found.
  @param  VendorGuid                  Vendor GUID to be found.
  @param  PtrTrack                    VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.
  @param  Global                      Pointer to VARIABLE_GLOBAL structure, including
                                      base of volatile variable storage area, base of
                                      NV variable storage area, and a lock.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL.
  @retval EFI_SUCCESS                 Variable successfully found.
  @retval EFI_NOT_FOUND               Variable not found.

**/
EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global
  )
{
  VARIABLE_HEADER       *Variable[2];
  VARIABLE_STORE_HEADER *VariableStoreHeader[2];
  UINTN                 Index;

  //
  // We aquire the lock at the entry of FindVariable as GetVariable, GetNextVariableName
  // SetVariable all call FindVariable at entry point. Please move "Aquire Lock" to
  // the correct places if this assumption does not hold TRUE anymore.
  //
  AcquireLockOnlyAtBootTime(&Global->VariableServicesLock);

  //
  // 0: Non-Volatile, 1: Volatile
  //
  VariableStoreHeader[0]  = (VARIABLE_STORE_HEADER *) ((UINTN) Global->NonVolatileVariableBase);
  VariableStoreHeader[1]  = (VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase);

  //
  // Start Pointers for the variable.
  // Actual Data Pointer where data can be written.
  //
  Variable[0] = (VARIABLE_HEADER *) (VariableStoreHeader[0] + 1);
  Variable[1] = (VARIABLE_HEADER *) (VariableStoreHeader[1] + 1);

  if (VariableName[0] != 0 && VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Find the variable by walk through non-volatile and volatile variable store
  //
  for (Index = 0; Index < 2; Index++) {
    PtrTrack->StartPtr  = (VARIABLE_HEADER *) (VariableStoreHeader[Index] + 1);
    PtrTrack->EndPtr    = GetEndPointer (VariableStoreHeader[Index]);

    while ((Variable[Index] != NULL) && (Variable[Index] <= GetEndPointer (VariableStoreHeader[Index]))) {
      if (Variable[Index]->StartId == VARIABLE_DATA && Variable[Index]->State == VAR_ADDED) {
        if (!(EfiAtRuntime () && ((Variable[Index]->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0))) {
          if (VariableName[0] == 0) {
            PtrTrack->CurrPtr   = Variable[Index];
            PtrTrack->Volatile  = (BOOLEAN) Index;
            return EFI_SUCCESS;
          } else {
            if (CompareGuid (VendorGuid, &Variable[Index]->VendorGuid)) {
              if (CompareMem (VariableName, GET_VARIABLE_NAME_PTR (Variable[Index]), Variable[Index]->NameSize) == 0) {
                PtrTrack->CurrPtr   = Variable[Index];
                PtrTrack->Volatile  = (BOOLEAN) Index;
                return EFI_SUCCESS;
              }
            }
          }
        }
      }

      Variable[Index] = GetNextVariablePtr (Variable[Index]);
    }
  }
  PtrTrack->CurrPtr = NULL;
  return EFI_NOT_FOUND;
}

/**
  This code finds variable in storage blocks (Volatile or Non-Volatile).
  
  @param  VariableName           A Null-terminated Unicode string that is the name of
                                 the vendor's variable.
  @param  VendorGuid             A unique identifier for the vendor.
  @param  Attributes             If not NULL, a pointer to the memory location to return the 
                                 attributes bitmask for the variable.
  @param  DataSize               Size of Data found. If size is less than the
                                 data, this value contains the required size.
  @param  Data                   On input, the size in bytes of the return Data buffer.  
                                 On output, the size of data returned in Data.
  @param  Global                 Pointer to VARIABLE_GLOBAL structure
  @param  Instance               Instance of the Firmware Volume.

  @retval EFI_SUCCESS            The function completed successfully. 
  @retval EFI_NOT_FOUND          The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   DataSize is too small for the result.  DataSize has 
                                 been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER  VariableName or VendorGuid or DataSize is NULL.

**/
EFI_STATUS
EFIAPI
GetVariable (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          * VendorGuid,
  OUT     UINT32            *Attributes OPTIONAL,
  IN OUT  UINTN             *DataSize,
  OUT     VOID              *Data,
  IN      VARIABLE_GLOBAL   * Global,
  IN      UINT32            Instance
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarDataSize;
  EFI_STATUS              Status;

  if (VariableName == NULL || VendorGuid == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Find existing variable
  //
  Status = FindVariable (VariableName, VendorGuid, &Variable, Global);

  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Get data size
  //
  VarDataSize = Variable.CurrPtr->DataSize;
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
    Status = EFI_SUCCESS;
    goto Done;
  } else {
    *DataSize = VarDataSize;
    Status = EFI_BUFFER_TOO_SMALL;
    goto Done;
  }

Done:
  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return Status;
}

/**

  This code Finds the Next available variable.

  @param  VariableNameSize       Size of the variable.
  @param  VariableName           On input, supplies the last VariableName that was returned by GetNextVariableName().
                                 On output, returns the Null-terminated Unicode string of the current variable.
  @param  VendorGuid             On input, supplies the last VendorGuid that was returned by GetNextVariableName().
                                 On output, returns the VendorGuid of the current variable.  
  @param  Global                 Pointer to VARIABLE_GLOBAL structure.
  @param  Instance               Instance of the Firmware Volume.

  @retval EFI_SUCCESS            The function completed successfully. 
  @retval EFI_NOT_FOUND          The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   VariableNameSize is too small for the result. 
                                 VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER  VariableNameSize or VariableName or VendorGuid is NULL.

**/
EFI_STATUS
EFIAPI
GetNextVariableName (
  IN OUT  UINTN             *VariableNameSize,
  IN OUT  CHAR16            *VariableName,
  IN OUT  EFI_GUID          *VendorGuid,
  IN      VARIABLE_GLOBAL   *Global,
  IN      UINT32            Instance
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarNameSize;
  EFI_STATUS              Status;

  if (VariableNameSize == NULL || VariableName == NULL || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FindVariable (VariableName, VendorGuid, &Variable, Global);

  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    goto Done;
  }

  while (TRUE) {
    if (VariableName[0] != 0) {
      //
      // If variable name is not NULL, get next variable
      //
      Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
    }
    //
    // If both volatile and non-volatile variable store are parsed,
    // return not found
    //
    if (Variable.CurrPtr >= Variable.EndPtr || Variable.CurrPtr == NULL) {
      Variable.Volatile = (BOOLEAN) (Variable.Volatile ^ ((BOOLEAN) 0x1));
      if (Variable.Volatile) {
        Variable.StartPtr = (VARIABLE_HEADER *) ((UINTN) (Global->VolatileVariableBase + sizeof (VARIABLE_STORE_HEADER)));
        Variable.EndPtr = (VARIABLE_HEADER *) GetEndPointer ((VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase));
      } else {
        Status = EFI_NOT_FOUND;
        goto Done;
      }

      Variable.CurrPtr = Variable.StartPtr;
      if (Variable.CurrPtr->StartId != VARIABLE_DATA) {
        continue;
      }
    }
    //
    // Variable is found
    //
    if (Variable.CurrPtr->StartId == VARIABLE_DATA && Variable.CurrPtr->State == VAR_ADDED) {
      if (!(EfiAtRuntime () && ((Variable.CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0))) {
        VarNameSize = Variable.CurrPtr->NameSize;
        if (VarNameSize <= *VariableNameSize) {
          CopyMem (
            VariableName,
            GET_VARIABLE_NAME_PTR (Variable.CurrPtr),
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
  }

Done:
  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return Status;

}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  @param  VariableName           A Null-terminated Unicode string that is the name of the vendor's
                                 variable.  Each VariableName is unique for each 
                                 VendorGuid.  VariableName must contain 1 or more 
                                 Unicode characters.  If VariableName is an empty Unicode 
                                 string, then EFI_INVALID_PARAMETER is returned.
  @param  VendorGuid             A unique identifier for the vendor
  @param  Attributes             Attributes bitmask to set for the variable
  @param  DataSize               The size in bytes of the Data buffer.  A size of zero causes the
                                 variable to be deleted.
  @param  Data                   The contents for the variable
  @param  Global                 Pointer to VARIABLE_GLOBAL structure
  @param  VolatileOffset         The offset of last volatile variable
  @param  NonVolatileOffset      The offset of last non-volatile variable
  @param  Instance               Instance of the Firmware Volume.

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as 
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits was supplied, or the 
                                 DataSize exceeds the maximum allowed, or VariableName is an empty 
                                 Unicode string, or VendorGuid is NULL.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved due to a hardware failure.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only or cannot be deleted.
  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.

**/
EFI_STATUS
EFIAPI
SetVariable (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINT32                  Attributes,
  IN UINTN                   DataSize,
  IN VOID                    *Data,
  IN VARIABLE_GLOBAL         *Global,
  IN UINTN                   *VolatileOffset,
  IN UINTN                   *NonVolatileOffset,
  IN UINT32                  Instance
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  EFI_STATUS              Status;
  VARIABLE_HEADER         *NextVariable;
  UINTN                   VarNameSize;
  UINTN                   VarNameOffset;
  UINTN                   VarDataOffset;
  UINTN                   VarSize;

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
  //
  // Check whether the input variable is already existed
  //
  
  Status = FindVariable (VariableName, VendorGuid, &Variable, Global);

  if (Status == EFI_SUCCESS && Variable.CurrPtr != NULL) {
    //
    // Update/Delete existing variable
    //

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
      if ((Variable.CurrPtr->Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
    }

    //
    // Setting a data variable with no access, or zero DataSize attributes
    // specified causes it to be deleted.
    //
    if (DataSize == 0 || (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {
      Variable.CurrPtr->State &= VAR_DELETED;
      Status = EFI_SUCCESS;
      goto Done;
    }

    //
    // If the variable is marked valid and the same data has been passed in
    // then return to the caller immediately.
    //
    if (Variable.CurrPtr->DataSize == DataSize &&
        CompareMem (Data, GetVariableDataPtr (Variable.CurrPtr), DataSize) == 0
          ) {
      Status = EFI_SUCCESS;
      goto Done;
    } else if (Variable.CurrPtr->State == VAR_ADDED) {
      //
      // Mark the old variable as in delete transition
      //
      Variable.CurrPtr->State &= VAR_IN_DELETED_TRANSITION;
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
  
  VarNameOffset = sizeof (VARIABLE_HEADER);
  VarNameSize   = StrSize (VariableName);
  VarDataOffset = VarNameOffset + VarNameSize + GET_PAD_SIZE (VarNameSize);
  VarSize       = VarDataOffset + DataSize + GET_PAD_SIZE (DataSize);

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
    if ((UINT32) (VarSize +*NonVolatileOffset) >
          ((VARIABLE_STORE_HEADER *) ((UINTN) (Global->NonVolatileVariableBase)))->Size
          ) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    NextVariable        = (VARIABLE_HEADER *) (UINT8 *) (*NonVolatileOffset + (UINTN) Global->NonVolatileVariableBase);
    *NonVolatileOffset  = *NonVolatileOffset + VarSize;
  } else {
    if ((UINT32) (VarSize +*VolatileOffset) >
          ((VARIABLE_STORE_HEADER *) ((UINTN) (Global->VolatileVariableBase)))->Size
          ) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    NextVariable    = (VARIABLE_HEADER *) (UINT8 *) (*VolatileOffset + (UINTN) Global->VolatileVariableBase);
    *VolatileOffset = *VolatileOffset + VarSize;
  }

  NextVariable->StartId     = VARIABLE_DATA;
  NextVariable->Attributes  = Attributes;
  NextVariable->State       = VAR_ADDED;
  NextVariable->Reserved    = 0;

  //
  // There will be pad bytes after Data, the NextVariable->NameSize and
  // NextVariable->NameSize should not include pad size so that variable
  // service can get actual size in GetVariable
  //
  NextVariable->NameSize  = (UINT32)VarNameSize;
  NextVariable->DataSize  = (UINT32)DataSize;

  CopyMem (&NextVariable->VendorGuid, VendorGuid, sizeof (EFI_GUID));
  CopyMem (
    (UINT8 *) ((UINTN) NextVariable + VarNameOffset),
    VariableName,
    VarNameSize
    );
  CopyMem (
    (UINT8 *) ((UINTN) NextVariable + VarDataOffset),
    Data,
    DataSize
    );

  //
  // Mark the old variable as deleted
  //
  if (!EFI_ERROR (Status)) {
    Variable.CurrPtr->State &= VAR_DELETED;
  }
  
  Status = EFI_SUCCESS;
Done:
  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return Status;
}

/**

  This code returns information about the EFI variables.

  @param  Attributes                   Attributes bitmask to specify the type of variables
                                       on which to return information.
  @param  MaximumVariableStorageSize   On output the maximum size of the storage space available for 
                                       the EFI variables associated with the attributes specified.  
  @param  RemainingVariableStorageSize Returns the remaining size of the storage space available for EFI 
                                       variables associated with the attributes specified.
  @param  MaximumVariableSize          Returns the maximum size of an individual EFI variable 
                                       associated with the attributes specified.
  @param  Global                       Pointer to VARIABLE_GLOBAL structure.
  @param  Instance                     Instance of the Firmware Volume.

  @retval EFI_SUCCESS                  Valid answer returned.
  @retval EFI_INVALID_PARAMETER        An invalid combination of attribute bits was supplied
  @retval EFI_UNSUPPORTED              The attribute is not supported on this platform, and the 
                                       MaximumVariableStorageSize, RemainingVariableStorageSize, 
                                       MaximumVariableSize are undefined.

**/
EFI_STATUS
EFIAPI
QueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize,
  IN  VARIABLE_GLOBAL        *Global,
  IN  UINT32                 Instance
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
    //   Make sure RT Attribute is set if we are in Runtime phase.
    //
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&Global->VariableServicesLock);

  if((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
    //
    // Query is Volatile related.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase);
  } else {
    //
    // Query is Non-Volatile related.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) ((UINTN) Global->NonVolatileVariableBase);
  }

  //
  // Now let's fill *MaximumVariableStorageSize *RemainingVariableStorageSize
  // with the storage size (excluding the storage header size)
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
  Variable = (VARIABLE_HEADER *) (VariableStoreHeader + 1);

  //
  // Now walk through the related variable store.
  //
  while (Variable < GetEndPointer (VariableStoreHeader)) {
    if (Variable->StartId != VARIABLE_DATA) {
      break;
    }

    NextVariable = (VARIABLE_HEADER *) (GetVariableDataPtr (Variable) + Variable->DataSize + GET_PAD_SIZE (Variable->DataSize));
    VariableSize = (UINT64) (UINTN) NextVariable - (UINT64) (UINTN) Variable;

    if (Variable->State == VAR_ADDED) {
      *RemainingVariableStorageSize -= VariableSize;
    }

    //
    // Go to the next one.
    //
    Variable = NextVariable;
  }

  if (*RemainingVariableStorageSize < sizeof (VARIABLE_HEADER)) {
    *MaximumVariableSize = 0;
  } else if ((*RemainingVariableStorageSize - sizeof (VARIABLE_HEADER)) < *MaximumVariableSize) {
    *MaximumVariableSize = *RemainingVariableStorageSize - sizeof (VARIABLE_HEADER);
  }
  
  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return EFI_SUCCESS;
}

/**
  Initializes variable store area.

  This function allocates memory space for variable store area and initializes its attributes.

  @param  VariableBase         Base of the variable store area created
  @param  LastVariableOffset   Size of VARIABLE_STORE_HEADER

**/
EFI_STATUS
InitializeVariableStore (
  OUT EFI_PHYSICAL_ADDRESS  *VariableBase,
  OUT UINTN                 *LastVariableOffset
  )
{
  VARIABLE_STORE_HEADER *VariableStore;

  //
  // Allocate memory for volatile variable store
  //
  VariableStore = (VARIABLE_STORE_HEADER *) AllocateRuntimePool (
                                              FixedPcdGet32(PcdVariableStoreSize)
                                              );
  if (NULL == VariableStore) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (VariableStore, FixedPcdGet32(PcdVariableStoreSize), 0xff);

  //
  // Variable Specific Data
  //
  *VariableBase             = (EFI_PHYSICAL_ADDRESS) (UINTN) VariableStore;
  *LastVariableOffset       = sizeof (VARIABLE_STORE_HEADER);

  VariableStore->Signature  = VARIABLE_STORE_SIGNATURE;
  VariableStore->Size       = FixedPcdGet32(PcdVariableStoreSize);
  VariableStore->Format     = VARIABLE_STORE_FORMATTED;
  VariableStore->State      = VARIABLE_STORE_HEALTHY;
  VariableStore->Reserved   = 0;
  VariableStore->Reserved1  = 0;

  return EFI_SUCCESS;
}

/**
  Initializes variable store area for non-volatile and volatile variable.

  This function allocates and initializes memory space for global context of ESAL
  variable service and variable store area for non-volatile and volatile variable.

  @param  ImageHandle           The Image handle of this driver.
  @param  SystemTable           The pointer of EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
EFIAPI
VariableCommonInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Allocate memory for mVariableModuleGlobal
  //
  mVariableModuleGlobal = (ESAL_VARIABLE_GLOBAL *) AllocateRuntimePool (
                                                    sizeof (ESAL_VARIABLE_GLOBAL)
                                                   );
  if (NULL == mVariableModuleGlobal) {
    return EFI_OUT_OF_RESOURCES;
  }

  EfiInitializeLock(&mVariableModuleGlobal->VariableGlobal[Physical].VariableServicesLock, TPL_NOTIFY);

  //
  // Intialize volatile variable store
  //
  Status = InitializeVariableStore (
            &mVariableModuleGlobal->VariableGlobal[Physical].VolatileVariableBase,
            &mVariableModuleGlobal->VolatileLastVariableOffset
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Intialize non volatile variable store
  //
  Status = InitializeVariableStore (
            &mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase,
            &mVariableModuleGlobal->NonVolatileLastVariableOffset
            );

  return Status;
}
