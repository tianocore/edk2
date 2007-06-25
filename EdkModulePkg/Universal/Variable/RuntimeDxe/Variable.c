/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Variable.c

Abstract:

Revision History

--*/

#include "Variable.h"
#include "reclaim.h"

//
// Don't use module globals after the SetVirtualAddress map is signaled
//
ESAL_VARIABLE_GLOBAL  *mVariableModuleGlobal;

//
// This is a temperary function which will be removed
// when EfiAcquireLock in UefiLib can handle the
// the call in UEFI Runtimer driver in RT phase.
//
STATIC
VOID
AcquireLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
  if (!EfiAtRuntime ()) {
    EfiAcquireLock (Lock);
  }
}

//
// This is a temperary function which will be removed
// when EfiAcquireLock in UefiLib can handle the
// the call in UEFI Runtimer driver in RT phase.
//
STATIC
VOID
ReleaseLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
  if (!EfiAtRuntime ()) {
    EfiReleaseLock (Lock);
  }
}

STATIC
BOOLEAN
EFIAPI
IsValidVariableHeader (
  IN  VARIABLE_HEADER   *Variable
  )
/*++

Routine Description:

  This code checks if variable header is valid or not.

Arguments:
  Variable              Pointer to the Variable Header.

Returns:
  TRUE            Variable header is valid.
  FALSE           Variable header is not valid.

--*/
{
  if (Variable == NULL ||
      Variable->StartId != VARIABLE_DATA ||
      (sizeof (VARIABLE_HEADER) + Variable->NameSize + Variable->DataSize) > MAX_VARIABLE_SIZE
      ) {
    return FALSE;
  }

  return TRUE;
}

STATIC
EFI_STATUS
EFIAPI
UpdateVariableStore (
  IN  VARIABLE_GLOBAL         *Global,
  IN  BOOLEAN                 Volatile,
  IN  BOOLEAN                 SetByIndex,
  IN  UINTN                   Instance,
  IN  UINTN                   DataPtrIndex,
  IN  UINT32                  DataSize,
  IN  UINT8                   *Buffer
  )
/*++

Routine Description:

  This function writes data to the FWH at the correct LBA even if the LBAs
  are fragmented.

Arguments:

  Global            Pointer to VARAIBLE_GLOBAL structure
  Volatile          If the Variable is Volatile or Non-Volatile
  SetByIndex        TRUE: Target pointer is given as index
                    FALSE: Target pointer is absolute
  Instance          Instance of FV Block services
  DataPtrIndex      Pointer to the Data from the end of VARIABLE_STORE_HEADER
                    structure
  DataSize          Size of data to be written.
  Buffer            Pointer to the buffer from which data is written

Returns:

  EFI STATUS

--*/
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
      DataPtr += Global->NonVolatileVariableBase;
    }

    if ((DataPtr + DataSize) >= ((EFI_PHYSICAL_ADDRESS) (UINTN) ((UINT8 *) FwVolHeader + FwVolHeader->FvLength))) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // Data Pointer should point to the actual Address where data is to be
    // written
    //
    VolatileBase = (VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase);
    if (SetByIndex) {
      DataPtr += Global->VolatileVariableBase;
    }

    if ((DataPtr + DataSize) >= ((UINTN) ((UINT8 *) VolatileBase + VolatileBase->Size))) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // If Volatile Variable just do a simple mem copy.
  //
  if (Volatile) {
    CopyMem ((UINT8 *) ((UINTN) DataPtr), Buffer, DataSize);
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

  for (PtrBlockMapEntry = FwVolHeader->FvBlockMap; PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {
    for (BlockIndex2 = 0; BlockIndex2 < PtrBlockMapEntry->NumBlocks; BlockIndex2++) {
      //
      // Check to see if the Variable Writes are spanning through multiple
      // blocks.
      //
      if ((CurrWritePtr >= LinearOffset) && (CurrWritePtr < LinearOffset + PtrBlockMapEntry->BlockLength)) {
        if ((CurrWritePtr + CurrWriteSize) <= (LinearOffset + PtrBlockMapEntry->BlockLength)) {
          Status = EfiFvbWriteBlock (
                    Instance,
                    LbaNumber,
                    (UINTN) (CurrWritePtr - LinearOffset),
                    &CurrWriteSize,
                    CurrBuffer
                    );
          if (EFI_ERROR (Status)) {
            return Status;
          }
        } else {
          Size = (UINT32) (LinearOffset + PtrBlockMapEntry->BlockLength - CurrWritePtr);
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

          CurrWritePtr  = LinearOffset + PtrBlockMapEntry->BlockLength;
          CurrBuffer    = CurrBuffer + Size;
          CurrWriteSize = CurrWriteSize - Size;
        }
      }

      LinearOffset += PtrBlockMapEntry->BlockLength;
      LbaNumber++;
    }
  }

  return EFI_SUCCESS;
}

STATIC
VARIABLE_STORE_STATUS
EFIAPI
GetVariableStoreStatus (
  IN VARIABLE_STORE_HEADER *VarStoreHeader
  )
/*++

Routine Description:

  This code gets the current status of Variable Store.

Arguments:

  VarStoreHeader  Pointer to the Variable Store Header.

Returns:

  EfiRaw        Variable store status is raw
  EfiValid      Variable store status is valid
  EfiInvalid    Variable store status is invalid

--*/
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

STATIC
UINT8 *
EFIAPI
GetVariableDataPtr (
  IN  VARIABLE_HEADER   *Variable
  )
/*++

Routine Description:

  This code gets the pointer to the variable data.

Arguments:

  Variable            Pointer to the Variable Header.

Returns:

  UINT8*              Pointer to Variable Data

--*/
{
  //
  // Be careful about pad size for alignment
  //
  return (UINT8 *) ((UINTN) GET_VARIABLE_NAME_PTR (Variable) + Variable->NameSize + GET_PAD_SIZE (Variable->NameSize));
}

STATIC
VARIABLE_HEADER *
EFIAPI
GetNextVariablePtr (
  IN  VARIABLE_HEADER   *Variable
  )
/*++

Routine Description:

  This code gets the pointer to the next variable header.

Arguments:

  Variable              Pointer to the Variable Header.

Returns:

  VARIABLE_HEADER*      Pointer to next variable header.

--*/
{
  if (!IsValidVariableHeader (Variable)) {
    return NULL;
  }
  //
  // Be careful about pad size for alignment
  //
  return (VARIABLE_HEADER *) ((UINTN) GetVariableDataPtr (Variable) + Variable->DataSize + GET_PAD_SIZE (Variable->DataSize));
}

STATIC
VARIABLE_HEADER *
EFIAPI
GetEndPointer (
  IN VARIABLE_STORE_HEADER       *VarStoreHeader
  )
/*++

Routine Description:

  This code gets the pointer to the last variable memory pointer byte

Arguments:

  VarStoreHeader        Pointer to the Variable Store Header.

Returns:

  VARIABLE_HEADER*      Pointer to last unavailable Variable Header

--*/
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *) ((UINTN) VarStoreHeader + VarStoreHeader->Size);
}

STATIC
EFI_STATUS
EFIAPI
Reclaim (
  IN  EFI_PHYSICAL_ADDRESS  VariableBase,
  OUT UINTN                 *LastVariableOffset,
  IN  BOOLEAN               IsVolatile
  )
/*++

Routine Description:

  Variable store garbage collection and reclaim operation

Arguments:

  VariableBase                Base address of variable store
  LastVariableOffset          Offset of last variable
  IsVolatile                  The variable store is volatile or not,
                              if it is non-volatile, need FTW

Returns:

  EFI STATUS

--*/
{
  VARIABLE_HEADER       *Variable;
  VARIABLE_HEADER       *NextVariable;
  VARIABLE_STORE_HEADER *VariableStoreHeader;
  UINT8                 *ValidBuffer;
  UINTN                 ValidBufferSize;
  UINTN                 VariableSize;
  UINT8                 *CurrPtr;
  EFI_STATUS            Status;

  VariableStoreHeader = (VARIABLE_STORE_HEADER *) ((UINTN) VariableBase);

  //
  // Start Pointers for the variable.
  //
  Variable        = (VARIABLE_HEADER *) (VariableStoreHeader + 1);

  ValidBufferSize = sizeof (VARIABLE_STORE_HEADER);

  while (IsValidVariableHeader (Variable)) {
    NextVariable = GetNextVariablePtr (Variable);
    if (Variable->State == VAR_ADDED) {
      VariableSize = (UINTN) NextVariable - (UINTN) Variable;
      ValidBufferSize += VariableSize;
    }

    Variable = NextVariable;
  }

  ValidBuffer = AllocatePool (ValidBufferSize);
  if (ValidBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (ValidBuffer, ValidBufferSize, 0xff);

  CurrPtr = ValidBuffer;

  //
  // Copy variable store header
  //
  CopyMem (CurrPtr, VariableStoreHeader, sizeof (VARIABLE_STORE_HEADER));
  CurrPtr += sizeof (VARIABLE_STORE_HEADER);

  //
  // Start Pointers for the variable.
  //
  Variable = (VARIABLE_HEADER *) (VariableStoreHeader + 1);

  while (IsValidVariableHeader (Variable)) {
    NextVariable = GetNextVariablePtr (Variable);
    if (Variable->State == VAR_ADDED) {
      VariableSize = (UINTN) NextVariable - (UINTN) Variable;
      CopyMem (CurrPtr, (UINT8 *) Variable, VariableSize);
      CurrPtr += VariableSize;
    }

    Variable = NextVariable;
  }

  if (IsVolatile) {
    //
    // If volatile variable store, just copy valid buffer
    //
    SetMem ((UINT8 *) (UINTN) VariableBase, VariableStoreHeader->Size, 0xff);
    CopyMem ((UINT8 *) (UINTN) VariableBase, ValidBuffer, ValidBufferSize);
    *LastVariableOffset = ValidBufferSize;
    Status              = EFI_SUCCESS;
  } else {
    //
    // If non-volatile variable store, perform FTW here.
    //
    Status = FtwVariableSpace (
              VariableBase,
              ValidBuffer,
              ValidBufferSize
              );
    if (!EFI_ERROR (Status)) {
      *LastVariableOffset = ValidBufferSize;
    }
  }

  FreePool (ValidBuffer);

  if (EFI_ERROR (Status)) {
    *LastVariableOffset = 0;
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global
  )
/*++

Routine Description:

  This code finds variable in storage blocks (Volatile or Non-Volatile)

Arguments:

  VariableName                Name of the variable to be found
  VendorGuid                  Vendor GUID to be found.
  PtrTrack                    Variable Track Pointer structure that contains
                              Variable Information.
                              Contains the pointer of Variable header.
  Global                      VARIABLE_GLOBAL pointer

Returns:

  EFI STATUS

--*/
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

    while (IsValidVariableHeader (Variable[Index]) && (Variable[Index] <= GetEndPointer (VariableStoreHeader[Index]))) {
      if (Variable[Index]->State == VAR_ADDED) {
        if (!(EfiAtRuntime () && !(Variable[Index]->Attributes & EFI_VARIABLE_RUNTIME_ACCESS))) {
          if (VariableName[0] == 0) {
            PtrTrack->CurrPtr   = Variable[Index];
            PtrTrack->Volatile  = (BOOLEAN) Index;
            return EFI_SUCCESS;
          } else {
            if (CompareGuid (VendorGuid, &Variable[Index]->VendorGuid)) {
              if (!CompareMem (VariableName, GET_VARIABLE_NAME_PTR (Variable[Index]), Variable[Index]->NameSize)) {
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
    //
    // While (...)
    //
  }
  //
  // for (...)
  //
  PtrTrack->CurrPtr = NULL;
  return EFI_NOT_FOUND;
}

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
/*++

Routine Description:

  This code finds variable in storage blocks (Volatile or Non-Volatile)

Arguments:

  VariableName                    Name of Variable to be found
  VendorGuid                      Variable vendor GUID
  Attributes OPTIONAL             Attribute value of the variable found
  DataSize                        Size of Data found. If size is less than the
                                  data, this value contains the required size.
  Data                            Data pointer
  Global                          Pointer to VARIABLE_GLOBAL structure
  Instance                        Instance of the Firmware Volume.

Returns:

  EFI STATUS

--*/
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

EFI_STATUS
EFIAPI
GetNextVariableName (
  IN OUT  UINTN             *VariableNameSize,
  IN OUT  CHAR16            *VariableName,
  IN OUT  EFI_GUID          *VendorGuid,
  IN      VARIABLE_GLOBAL   *Global,
  IN      UINT32            Instance
  )
/*++

Routine Description:

  This code Finds the Next available variable

Arguments:

  VariableNameSize            Size of the variable
  VariableName                Pointer to variable name
  VendorGuid                  Variable Vendor Guid
  Global                      VARIABLE_GLOBAL structure pointer.
  Instance                    FV instance

Returns:

  EFI STATUS

--*/
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
      if (Variable.Volatile) {
        Variable.StartPtr = (VARIABLE_HEADER *) ((UINTN) (Global->VolatileVariableBase + sizeof (VARIABLE_STORE_HEADER)));
        Variable.EndPtr = (VARIABLE_HEADER *) GetEndPointer ((VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase));
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

    Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
  }

Done:
  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return Status;
}

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
/*++

Routine Description:

  This code sets variable in storage blocks (Volatile or Non-Volatile)

Arguments:

  VariableName                    Name of Variable to be found
  VendorGuid                      Variable vendor GUID
  Attributes                      Attribute value of the variable found
  DataSize                        Size of Data found. If size is less than the
                                  data, this value contains the required size.
  Data                            Data pointer
  Global                          Pointer to VARIABLE_GLOBAL structure
  VolatileOffset                  The offset of last volatile variable
  NonVolatileOffset               The offset of last non-volatile variable
  Instance                        Instance of the Firmware Volume.

Returns:

  EFI STATUS
  EFI_INVALID_PARAMETER           - Invalid parameter
  EFI_SUCCESS                     - Set successfully
  EFI_OUT_OF_RESOURCES            - Resource not enough to set variable
  EFI_NOT_FOUND                   - Not found

--*/
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

  Reclaimed = FALSE;

  if (VariableName == NULL || VariableName[0] == 0 || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FindVariable (VariableName, VendorGuid, &Variable, Global);

  if (Status == EFI_INVALID_PARAMETER) {
    goto Done;
  } else if (!EFI_ERROR (Status) && Variable.Volatile && EfiAtRuntime()) {
    //
    // If EfiAtRuntime and the variable is Volatile and Runtime Access,
    // the volatile is ReadOnly, and SetVariable should be aborted and
    // return EFI_WRITE_PROTECTED.
    //
    Status = EFI_WRITE_PROTECTED;
    goto Done;
  } else if (sizeof (VARIABLE_HEADER) + StrSize (VariableName) + DataSize > MAX_VARIABLE_SIZE) {
    //
    //  The size of the VariableName, including the Unicode Null in bytes plus
    //  the DataSize is limited to maximum size of MAX_VARIABLE_SIZE (1024) bytes.
    //
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  } else if (Attributes == EFI_VARIABLE_NON_VOLATILE) {
    //
    //  Make sure not only EFI_VARIABLE_NON_VOLATILE is set
    //
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  } else if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) ==
                EFI_VARIABLE_RUNTIME_ACCESS) {
    //
    //  Make sure if runtime bit is set, boot service bit is set also
    //
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  } else if (EfiAtRuntime () && Attributes && !(Attributes & EFI_VARIABLE_RUNTIME_ACCESS)) {
    //
    // Runtime but Attribute is not Runtime
    //
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  } else if (EfiAtRuntime () && Attributes && !(Attributes & EFI_VARIABLE_NON_VOLATILE)) {
    //
    // Cannot set volatile variable in Runtime
    //
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  } else if (DataSize == 0 || (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {
    //
    // Setting a data variable with no access, or zero DataSize attributes
    // specified causes it to be deleted.
    //
    if (!EFI_ERROR (Status)) {
      State = Variable.CurrPtr->State;
      State &= VAR_DELETED;

      Status = UpdateVariableStore (
                Global,
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

      Status = EFI_SUCCESS;
      goto Done;
    }

    Status = EFI_NOT_FOUND;
    goto Done;
  } else {
    if (!EFI_ERROR (Status)) {
      //
      // If the variable is marked valid and the same data has been passed in
      // then return to the caller immediately.
      //
      if (Variable.CurrPtr->DataSize == DataSize &&
          !CompareMem (Data, GetVariableDataPtr (Variable.CurrPtr), DataSize)
            ) {
        Status = EFI_SUCCESS;
        goto Done;
      } else if (Variable.CurrPtr->State == VAR_ADDED) {
        //
        // Mark the old variable as in delete transition
        //
        State = Variable.CurrPtr->State;
        State &= VAR_IN_DELETED_TRANSITION;

        Status = UpdateVariableStore (
                  Global,
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
    }
    //
    // Create a new variable and copy the data.
    //
    // Tricky part: Use scratch data area at the end of volatile variable store
    // as a temporary storage.
    //
    NextVariable = GetEndPointer ((VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase));

    SetMem (NextVariable, SCRATCH_SIZE, 0xff);

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
    if (Attributes & EFI_VARIABLE_NON_VOLATILE) {
      if ((UINT32) (VarSize +*NonVolatileOffset) >
            ((VARIABLE_STORE_HEADER *) ((UINTN) (Global->NonVolatileVariableBase)))->Size
            ) {
        if (EfiAtRuntime ()) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }
        //
        // Perform garbage collection & reclaim operation
        //
        Status = Reclaim (Global->NonVolatileVariableBase, NonVolatileOffset, FALSE);
        if (EFI_ERROR (Status)) {
          goto Done;
        }
        //
        // If still no enough space, return out of resources
        //
        if ((UINT32) (VarSize +*NonVolatileOffset) >
              ((VARIABLE_STORE_HEADER *) ((UINTN) (Global->NonVolatileVariableBase)))->Size
              ) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }

        Reclaimed = TRUE;
      }
      //
      // Three steps
      // 1. Write variable header
      // 2. Write variable data
      // 3. Set variable state to valid
      //
      //
      // Step 1:
      //
      Status = UpdateVariableStore (
                Global,
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
      Status = UpdateVariableStore (
                Global,
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
      // Step 3:
      //
      NextVariable->State = VAR_ADDED;
      Status = UpdateVariableStore (
                Global,
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

      *NonVolatileOffset = *NonVolatileOffset + VarSize;

    } else {
      if (EfiAtRuntime ()) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }

      if ((UINT32) (VarSize +*VolatileOffset) >
            ((VARIABLE_STORE_HEADER *) ((UINTN) (Global->VolatileVariableBase)))->Size
            ) {
        //
        // Perform garbage collection & reclaim operation
        //
        Status = Reclaim (Global->VolatileVariableBase, VolatileOffset, TRUE);
        if (EFI_ERROR (Status)) {
          goto Done;
        }
        //
        // If still no enough space, return out of resources
        //
        if ((UINT32) (VarSize +*VolatileOffset) >
              ((VARIABLE_STORE_HEADER *) ((UINTN) (Global->VolatileVariableBase)))->Size
              ) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }

        Reclaimed = TRUE;
      }

      NextVariable->State = VAR_ADDED;
      Status = UpdateVariableStore (
                Global,
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

      *VolatileOffset = *VolatileOffset + VarSize;
    }
    //
    // Mark the old variable as deleted
    //
    if (!Reclaimed && !EFI_ERROR (Status) && Variable.CurrPtr != NULL) {
      State = Variable.CurrPtr->State;
      State &= VAR_DELETED;

      Status = UpdateVariableStore (
                Global,
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
  }

  Status = EFI_SUCCESS;
Done:
  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return Status;
}

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
/*++

Routine Description:

  This code returns information about the EFI variables.

Arguments:

  Attributes                      Attributes bitmask to specify the type of variables
                                  on which to return information.
  MaximumVariableStorageSize      Pointer to the maximum size of the storage space available
                                  for the EFI variables associated with the attributes specified.
  RemainingVariableStorageSize    Pointer to the remaining size of the storage space available
                                  for the EFI variables associated with the attributes specified.
  MaximumVariableSize             Pointer to the maximum size of the individual EFI variables
                                  associated with the attributes specified.
  Global                          Pointer to VARIABLE_GLOBAL structure.
  Instance                        Instance of the Firmware Volume.

Returns:

  EFI STATUS
  EFI_INVALID_PARAMETER           - An invalid combination of attribute bits was supplied.
  EFI_SUCCESS                     - Query successfully.
  EFI_UNSUPPORTED                 - The attribute is not supported on this platform.

--*/
{
  VARIABLE_HEADER        *Variable;
  VARIABLE_HEADER        *NextVariable;
  UINT64                 VariableSize;
  VARIABLE_STORE_HEADER  *VariableStoreHeader;

  if(MaximumVariableStorageSize == NULL || RemainingVariableStorageSize == NULL || MaximumVariableSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)) == 0) {
    //
    // Make sure the Attributes combination is supported by the platform.
    //
    return EFI_UNSUPPORTED;
  } else if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    //
    // Make sure if runtime bit is set, boot service bit is set also.
    //
    return EFI_INVALID_PARAMETER;
  } else if (EfiAtRuntime () && !(Attributes & EFI_VARIABLE_RUNTIME_ACCESS)) {
    //
    // Make sure RT Attribute is set if we are in Runtime phase.
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
  // with the storage size (excluding the storage header size).
  //
  *MaximumVariableStorageSize   = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER);
  *RemainingVariableStorageSize = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER);

  //
  // Let *MaximumVariableSize be MAX_VARIABLE_SIZE.
  //
  *MaximumVariableSize = MAX_VARIABLE_SIZE;

  //
  // Point to the starting address of the variables.
  //
  Variable = (VARIABLE_HEADER *) (VariableStoreHeader + 1);

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

  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VariableCommonInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:
  This function does common initialization for variable services

Arguments:

  ImageHandle   - The firmware allocated handle for the EFI image.
  SystemTable   - A pointer to the EFI System Table.

Returns:

  Status code.

  EFI_NOT_FOUND     - Variable store area not found.
  EFI_UNSUPPORTED   - Currently only one non-volatile variable store is supported.
  EFI_SUCCESS       - Variable services successfully initialized.

--*/
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
  EFI_FLASH_SUBAREA_ENTRY         VariableStoreEntry;
  UINT64                          BaseAddress;
  UINT64                          Length;
  UINTN                           Index;
  UINT8                           Data;

  mVariableModuleGlobal = AllocateRuntimePool (sizeof (ESAL_VARIABLE_GLOBAL));
  if (mVariableModuleGlobal == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EfiInitializeLock(&mVariableModuleGlobal->VariableGlobal[Physical].VariableServicesLock, TPL_NOTIFY);

  //
  // Allocate memory for volatile variable store
  //
  VolatileVariableStore = AllocateRuntimePool (VARIABLE_STORE_SIZE + SCRATCH_SIZE);
  if (VolatileVariableStore == NULL) {
    FreePool (mVariableModuleGlobal);
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (VolatileVariableStore, VARIABLE_STORE_SIZE + SCRATCH_SIZE, 0xff);

  //
  //  Variable Specific Data
  //
  mVariableModuleGlobal->VariableGlobal[Physical].VolatileVariableBase = (EFI_PHYSICAL_ADDRESS) (UINTN) VolatileVariableStore;
  mVariableModuleGlobal->VolatileLastVariableOffset = sizeof (VARIABLE_STORE_HEADER);

  VolatileVariableStore->Signature                  = VARIABLE_STORE_SIGNATURE;
  VolatileVariableStore->Size                       = VARIABLE_STORE_SIZE;
  VolatileVariableStore->Format                     = VARIABLE_STORE_FORMATTED;
  VolatileVariableStore->State                      = VARIABLE_STORE_HEALTHY;
  VolatileVariableStore->Reserved                   = 0;
  VolatileVariableStore->Reserved1                  = 0;

  //
  // Get non volatile varaible store
  //

  TempVariableStoreHeader = (UINT64) PcdGet32 (PcdFlashNvStorageVariableBase);
  VariableStoreEntry.Base = TempVariableStoreHeader + \
                              (((EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) (TempVariableStoreHeader)) -> HeaderLength);
  VariableStoreEntry.Length = (UINT64) PcdGet32 (PcdFlashNvStorageVariableSize) - \
                                (((EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) (TempVariableStoreHeader)) -> HeaderLength);
  //
  // Mark the variable storage region of the FLASH as RUNTIME
  //
  BaseAddress = VariableStoreEntry.Base & (~EFI_PAGE_MASK);
  Length      = VariableStoreEntry.Length + (VariableStoreEntry.Base - BaseAddress);
  Length      = (Length + EFI_PAGE_SIZE - 1) & (~EFI_PAGE_MASK);

  Status      = gDS->GetMemorySpaceDescriptor (BaseAddress, &GcdDescriptor);
  if (EFI_ERROR (Status)) {
    FreePool (mVariableModuleGlobal);
    FreePool (VolatileVariableStore);
    return EFI_UNSUPPORTED;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  BaseAddress,
                  Length,
                  GcdDescriptor.Attributes | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    FreePool (mVariableModuleGlobal);
    FreePool (VolatileVariableStore);
    return EFI_UNSUPPORTED;
  }
  //
  // Get address of non volatile variable store base
  //
  mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase = VariableStoreEntry.Base;

  //
  // Check Integrity
  //
  //
  // Find the Correct Instance of the FV Block Service.
  //
  Instance  = 0;
  CurrPtr   = (CHAR8 *) ((UINTN) mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase);
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
                &mVariableModuleGlobal->VariableGlobal[Physical],
                FALSE,
                FALSE,
                mVariableModuleGlobal->FvbInstance,
                (UINTN) &VariableStoreHeader->Size,
                sizeof (UINT32),
                (UINT8 *) &VariableStoreEntry.Length
                );
      // 
      // As Variables are stored in NV storage, which are slow devices,such as flash.
      // Variable operation may skip checking variable program result to improve performance,
      // We can assume Variable program is OK through some check point.
      // Variable Store Size Setting should be the first Variable write operation,
      // We can assume all Read/Write is OK if we can set Variable store size successfully.
      // If write fail, we will assert here
      //
      ASSERT(VariableStoreHeader->Size == VariableStoreEntry.Length);

      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase = (EFI_PHYSICAL_ADDRESS) ((UINTN) CurrPtr);
    //
    // Parse non-volatile variable data and get last variable offset
    //
    NextVariable  = (VARIABLE_HEADER *) (CurrPtr + sizeof (VARIABLE_STORE_HEADER));
    Status        = EFI_SUCCESS;

    while (IsValidVariableHeader (NextVariable)) {
      NextVariable = GetNextVariablePtr (NextVariable);
    }

    mVariableModuleGlobal->NonVolatileLastVariableOffset = (UINTN) NextVariable - (UINTN) CurrPtr;

    //
    // Check if the free area is blow a threshold
    //
    if ((((VARIABLE_STORE_HEADER *)((UINTN) CurrPtr))->Size - mVariableModuleGlobal->NonVolatileLastVariableOffset) < VARIABLE_RECLAIM_THRESHOLD) {
      Status = Reclaim (
                mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase,
                &mVariableModuleGlobal->NonVolatileLastVariableOffset,
                FALSE
                );
    }

    if (EFI_ERROR (Status)) {
      FreePool (mVariableModuleGlobal);
      FreePool (VolatileVariableStore);
      return Status;
    }

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
                  FALSE
                  );
        break;
      }
    }
  }

  if (EFI_ERROR (Status)) {
    FreePool (mVariableModuleGlobal);
    FreePool (VolatileVariableStore);
  }

  return Status;
}
