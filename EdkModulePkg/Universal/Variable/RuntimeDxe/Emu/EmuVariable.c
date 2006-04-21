/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    EmuVariable.c

Abstract:

Revision History

--*/

#include "Variable.h"

//
// Don't use module globals after the SetVirtualAddress map is signaled
//
ESAL_VARIABLE_GLOBAL  *mVariableModuleGlobal;

UINT32
EFIAPI
ArrayLength (
  IN CHAR16 *String
  )
/*++

Routine Description:

  Determine the length of null terminated char16 array.

Arguments:

  String    Null-terminated CHAR16 array pointer.

Returns:

  UINT32    Number of bytes in the string, including the double NULL at the end;

--*/
{
  UINT32  Count;

  if (NULL == String) {
    return 0;
  }

  Count = 0;

  while (0 != String[Count]) {
    Count++;
  }

  return (Count * 2) + 2;
}

UINTN
EFIAPI
GetPadSize (
  IN UINTN Value
  )
/*++

Routine Description:

  This function return the pad size for alignment

Arguments:

  Value  The value need to align

Returns:

  Pad size for value

--*/
{
  //
  // If alignment is 0 or 1, means no alignment required
  //
  if (ALIGNMENT == 0 || ALIGNMENT == 1) {
    return 0;
  }

  return ALIGNMENT - (Value % ALIGNMENT);
}

VARIABLE_STORE_STATUS
EFIAPI
GetVariableStoreStatus (
  IN VARIABLE_STORE_HEADER *VarStoreHeader
  )
/*++

Routine Description:

  This code gets the pointer to the variable name.

Arguments:

  VarStoreHeader  Pointer to the Variable Store Header.

Returns:

  EfiHealthy    Variable store is healthy
  EfiRaw        Variable store is raw
  EfiInvalid    Variable store is invalid

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
  if (Variable->StartId != VARIABLE_DATA) {
    return NULL;
  }
  //
  // Be careful about pad size for alignment
  //
  return (UINT8 *) ((UINTN) GET_VARIABLE_NAME_PTR (Variable) + Variable->NameSize + GetPadSize (Variable->NameSize));
}

VARIABLE_HEADER *
EFIAPI
GetNextVariablePtr (
  IN  VARIABLE_HEADER   *Variable
  )
/*++

Routine Description:

  This code gets the pointer to the next variable header.

Arguments:

  Variable                  Pointer to the Variable Header.

Returns:

  VARIABLE_HEADER*      Pointer to next variable header.

--*/
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
      (sizeof (VARIABLE_HEADER) + VarHeader->DataSize + VarHeader->NameSize) > MAX_VARIABLE_SIZE
      ) {
    return NULL;
  }

  return VarHeader;
}

VARIABLE_HEADER *
EFIAPI
GetEndPointer (
  IN VARIABLE_STORE_HEADER       *VolHeader
  )
/*++

Routine Description:

  This code gets the pointer to the last variable memory pointer byte

Arguments:

  Variable                  Pointer to the Variable Header.

Returns:

  VARIABLE_HEADER*      Pointer to last unavailable Variable Header

--*/
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *) ((UINTN) VolHeader + VolHeader->Size);
}

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
        if (!(EfiAtRuntime () && !(Variable[Index]->Attributes & EFI_VARIABLE_RUNTIME_ACCESS))) {
          if (VariableName[0] == 0) {
            PtrTrack->CurrPtr   = Variable[Index];
            PtrTrack->Volatile  = (BOOLEAN) Index;
            return EFI_SUCCESS;
          } else {
            if (CompareGuid (VendorGuid, &Variable[Index]->VendorGuid)) {
              if (!CompareMem (VariableName, GET_VARIABLE_NAME_PTR (Variable[Index]), ArrayLength (VariableName))) {
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
    return Status;
  }
  //
  // Get data size
  //
  VarDataSize = Variable.CurrPtr->DataSize;
  if (*DataSize >= VarDataSize) {
    CopyMem (Data, GetVariableDataPtr (Variable.CurrPtr), VarDataSize);
    if (Attributes) {
      *Attributes = Variable.CurrPtr->Attributes;
    }

    *DataSize = VarDataSize;
    return EFI_SUCCESS;
  } else {
    *DataSize = VarDataSize;
    return EFI_BUFFER_TOO_SMALL;
  }
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

  if (VariableNameSize == NULL || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FindVariable (VariableName, VendorGuid, &Variable, Global);

  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    return Status;
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
        return EFI_NOT_FOUND;
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
        return Status;
      }
    }
  }

  return EFI_NOT_FOUND;
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

--*/
{
  VARIABLE_POINTER_TRACK  Variable;
  EFI_STATUS              Status;
  VARIABLE_HEADER         *NextVariable;
  UINTN                   VarNameSize;
  UINTN                   VarNameOffset;
  UINTN                   VarDataOffset;
  UINTN                   VarSize;

  if (VariableName == NULL || VariableName[0] == 0 || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FindVariable (VariableName, VendorGuid, &Variable, Global);

  if (Status == EFI_INVALID_PARAMETER) {
    return Status;
  }
  //
  //  The size of the VariableName, including the Unicode Null in bytes plus
  //  the DataSize is limited to maximum size of MAX_VARIABLE_SIZE (1024) bytes.
  //
  else if (sizeof (VARIABLE_HEADER) + (ArrayLength (VariableName) + DataSize) > MAX_VARIABLE_SIZE) {
    return EFI_INVALID_PARAMETER;
  }
  //
  //  Make sure if runtime bit is set, boot service bit is set also
  //
  else if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS
          ) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Runtime but Attribute is not Runtime
  //
  else if (EfiAtRuntime () && Attributes && !(Attributes & EFI_VARIABLE_RUNTIME_ACCESS)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Cannot set volatile variable in Runtime
  //
  else if (EfiAtRuntime () && Attributes && !(Attributes & EFI_VARIABLE_NON_VOLATILE)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Setting a data variable with no access, or zero DataSize attributes
  // specified causes it to be deleted.
  //
  else if (DataSize == 0 || Attributes == 0) {
    if (!EFI_ERROR (Status)) {
      Variable.CurrPtr->State &= VAR_DELETED;
      return EFI_SUCCESS;
    }

    return EFI_NOT_FOUND;
  } else {
    if (!EFI_ERROR (Status)) {
      //
      // If the variable is marked valid and the same data has been passed in
      // then return to the caller immediately.
      //
      if (Variable.CurrPtr->DataSize == DataSize &&
          !CompareMem (Data, GetVariableDataPtr (Variable.CurrPtr), DataSize)
            ) {
        return EFI_SUCCESS;
      } else if (Variable.CurrPtr->State == VAR_ADDED) {
        //
        // Mark the old variable as in delete transition
        //
        Variable.CurrPtr->State &= VAR_IN_DELETED_TRANSITION;
      }
    }
    //
    // Create a new variable and copy the data.
    //
    VarNameOffset = sizeof (VARIABLE_HEADER);
    VarNameSize   = ArrayLength (VariableName);
    VarDataOffset = VarNameOffset + VarNameSize + GetPadSize (VarNameSize);
    VarSize       = VarDataOffset + DataSize + GetPadSize (DataSize);

    if (Attributes & EFI_VARIABLE_NON_VOLATILE) {
      if ((UINT32) (VarSize +*NonVolatileOffset) >
            ((VARIABLE_STORE_HEADER *) ((UINTN) (Global->NonVolatileVariableBase)))->Size
            ) {
        return EFI_OUT_OF_RESOURCES;
      }

      NextVariable        = (VARIABLE_HEADER *) (UINT8 *) (*NonVolatileOffset + (UINTN) Global->NonVolatileVariableBase);
      *NonVolatileOffset  = *NonVolatileOffset + VarSize;
    } else {
      if (EfiAtRuntime ()) {
        return EFI_INVALID_PARAMETER;
      }

      if ((UINT32) (VarSize +*VolatileOffset) >
            ((VARIABLE_STORE_HEADER *) ((UINTN) (Global->VolatileVariableBase)))->Size
            ) {
        return EFI_OUT_OF_RESOURCES;
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
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InitializeVariableStore (
  OUT EFI_PHYSICAL_ADDRESS  *VariableBase,
  OUT UINTN                 *LastVariableOffset
  )
/*++

Routine Description:
  This function initializes variable store

Arguments:

Returns:

--*/
{
  VARIABLE_STORE_HEADER *VariableStore;

  //
  // Allocate memory for volatile variable store
  //
  VariableStore = (VARIABLE_STORE_HEADER *) AllocateRuntimePool (
                                              VARIABLE_STORE_SIZE
                                              );
  if (NULL == VariableStore) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (VariableStore, VARIABLE_STORE_SIZE, 0xff);

  //
  // Variable Specific Data
  //
  *VariableBase             = (EFI_PHYSICAL_ADDRESS) (UINTN) VariableStore;
  *LastVariableOffset       = sizeof (VARIABLE_STORE_HEADER);

  VariableStore->Signature  = VARIABLE_STORE_SIGNATURE;
  VariableStore->Size       = VARIABLE_STORE_SIZE;
  VariableStore->Format     = VARIABLE_STORE_FORMATTED;
  VariableStore->State      = VARIABLE_STORE_HEALTHY;
  VariableStore->Reserved   = 0;
  VariableStore->Reserved1  = 0;

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

Returns:

--*/
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
  //
  // Intialize volatile variable store
  //
  Status = InitializeVariableStore (
            &mVariableModuleGlobal->VariableBase[Physical].VolatileVariableBase,
            &mVariableModuleGlobal->VolatileLastVariableOffset
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Intialize non volatile variable store
  //
  Status = InitializeVariableStore (
            &mVariableModuleGlobal->VariableBase[Physical].NonVolatileVariableBase,
            &mVariableModuleGlobal->NonVolatileLastVariableOffset
            );

  return Status;
}
