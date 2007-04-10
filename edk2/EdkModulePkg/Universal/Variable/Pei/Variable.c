/*++

Copyright (c) 2006 - 2007 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:

  Variable.c

Abstract:

  Framework PEIM to provide the Variable functionality

--*/

#include <Variable.h>

//
// Module globals
//
static EFI_PEI_READ_ONLY_VARIABLE_PPI mVariablePpi = {
  PeiGetVariable,
  PeiGetNextVariableName
};

static EFI_PEI_PPI_DESCRIPTOR     mPpiListVariable = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiReadOnlyVariablePpiGuid,
  &mVariablePpi
};

EFI_GUID mEfiVariableIndexTableGuid = EFI_VARIABLE_INDEX_TABLE_GUID;

EFI_STATUS
EFIAPI
PeimInitializeVariableServices (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:

  Provide the functionality of the variable services.

Arguments:

  FfsHeadher  - The FFS file header
  PeiServices - General purpose services available to every PEIM.

Returns:

  Status -  EFI_SUCCESS if the interface could be successfully
            installed

--*/
{
  //
  // Publish the variable capability to other modules
  //
  return (**PeiServices).InstallPpi (PeiServices, &mPpiListVariable);

}

STATIC
VARIABLE_HEADER *
GetNextVariablePtr (
  IN VARIABLE_HEADER  *Variable
  )
/*++

Routine Description:

  This code checks if variable header is valid or not.

Arguments:
  Variable       Pointer to the Variable Header.

Returns:
  TRUE            Variable header is valid.
  FALSE           Variable header is not valid.

--*/
{
  return (VARIABLE_HEADER *) ((UINTN) GET_VARIABLE_DATA_PTR (Variable) + Variable->DataSize + GET_PAD_SIZE (Variable->DataSize));
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
      (sizeof (VARIABLE_HEADER) + Variable->DataSize + Variable->NameSize) > MAX_VARIABLE_SIZE
      ) {
    return FALSE;
  }

  return TRUE;
}

STATIC
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

  EfiRaw        Variable store is raw
  EfiValid      Variable store is valid
  EfiInvalid    Variable store is invalid

--*/
{
  if (VarStoreHeader->Signature == VARIABLE_STORE_SIGNATURE &&
      VarStoreHeader->Format == VARIABLE_STORE_FORMATTED &&
      VarStoreHeader->State == VARIABLE_STORE_HEALTHY
      ) {

    return EfiValid;
  }

  if (VarStoreHeader->Signature == 0xffffffff &&
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
EFI_STATUS
CompareWithValidVariable (
  IN  VARIABLE_HEADER         *Variable,
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack
  )
/*++

Routine Description:

  This function compares a variable with variable entries in database

Arguments:

  Variable       - Pointer to the variable in our database
  VariableName   - Name of the variable to compare to 'Variable'
  VendorGuid     - GUID of the variable to compare to 'Variable'
  PtrTrack       - Variable Track Pointer structure that contains
                   Variable Information.

Returns:

  EFI_SUCCESS    - Found match variable
  EFI_NOT_FOUND  - Variable not found

--*/
{
  if (VariableName[0] == 0) {
    PtrTrack->CurrPtr = Variable;
    return EFI_SUCCESS;
  } else {
    //
    // Don't use CompareGuid function here for performance reasons.
    // Instead we compare the GUID a UINT32 at a time and branch
    // on the first failed comparison.
    //
    if ((((INT32 *) VendorGuid)[0] == ((INT32 *) &Variable->VendorGuid)[0]) &&
        (((INT32 *) VendorGuid)[1] == ((INT32 *) &Variable->VendorGuid)[1]) &&
        (((INT32 *) VendorGuid)[2] == ((INT32 *) &Variable->VendorGuid)[2]) &&
        (((INT32 *) VendorGuid)[3] == ((INT32 *) &Variable->VendorGuid)[3])
        ) {
      if (!CompareMem (VariableName, GET_VARIABLE_NAME_PTR (Variable), Variable->NameSize)) {
        PtrTrack->CurrPtr = Variable;
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
FindVariable (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack
  )
/*++

Routine Description:

  This code finds variable in storage blocks (Non-Volatile)

Arguments:

  PeiServices    - General purpose services available to every PEIM.
  VariableName   - Name of the variable to be found
  VendorGuid     - Vendor GUID to be found.
  PtrTrack       - Variable Track Pointer structure that contains
                   Variable Information.

Returns:

  EFI_SUCCESS      - Variable found successfully
  EFI_NOT_FOUND    - Variable not found
  EFI_INVALID_PARAMETER  - Invalid variable name

--*/
{
  EFI_HOB_GUID_TYPE       *GuidHob;
  VARIABLE_STORE_HEADER   *VariableStoreHeader;
  VARIABLE_HEADER         *Variable;
  VARIABLE_HEADER         *MaxIndex;
  VARIABLE_INDEX_TABLE    *IndexTable;
  UINT32                  Count;
  UINT8                   *VariableBase;

  if (VariableName != 0 && VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // No Variable Address equals zero, so 0 as initial value is safe.
  //
  MaxIndex = 0;

  GuidHob = GetFirstGuidHob (&mEfiVariableIndexTableGuid);
  if (GuidHob == NULL) {
    IndexTable = BuildGuidHob (&mEfiVariableIndexTableGuid, sizeof (VARIABLE_INDEX_TABLE));
    IndexTable->Length      = 0;
    IndexTable->StartPtr    = NULL;
    IndexTable->EndPtr      = NULL;
    IndexTable->GoneThrough = 0;
  } else {
    IndexTable = GET_GUID_HOB_DATA (GuidHob);
    for (Count = 0; Count < IndexTable->Length; Count++)
    {
      MaxIndex = GetVariableByIndex (IndexTable, Count);

      if (CompareWithValidVariable (MaxIndex, VariableName, VendorGuid, PtrTrack) == EFI_SUCCESS) {
        PtrTrack->StartPtr  = IndexTable->StartPtr;
        PtrTrack->EndPtr    = IndexTable->EndPtr;

        return EFI_SUCCESS;
      }
    }

    if (IndexTable->GoneThrough) {
      return EFI_NOT_FOUND;
    }
  }
  //
  // If not found in HOB, then let's start from the MaxIndex we've found.
  //
  if (MaxIndex != NULL) {
    Variable = GetNextVariablePtr (MaxIndex);
  } else {
    if (IndexTable->StartPtr || IndexTable->EndPtr) {
      Variable = IndexTable->StartPtr;
    } else {
      VariableBase = (UINT8 *) (UINTN) PcdGet32 (PcdFlashNvStorageVariableBase);
      VariableStoreHeader = (VARIABLE_STORE_HEADER *) (VariableBase + \
                            ((EFI_FIRMWARE_VOLUME_HEADER *) (VariableBase)) -> HeaderLength);

      if (GetVariableStoreStatus (VariableStoreHeader) != EfiValid) {
        return EFI_UNSUPPORTED;
      }

      if (~VariableStoreHeader->Size == 0) {
        return EFI_NOT_FOUND;
      }
      //
      // Find the variable by walk through non-volatile variable store
      //
      IndexTable->StartPtr  = (VARIABLE_HEADER *) (VariableStoreHeader + 1);
      IndexTable->EndPtr    = (VARIABLE_HEADER *) ((UINTN) VariableStoreHeader + VariableStoreHeader->Size);

      //
      // Start Pointers for the variable.
      // Actual Data Pointer where data can be written.
      //
      Variable = IndexTable->StartPtr;
    }
  }
  //
  // Find the variable by walk through non-volatile variable store
  //
  PtrTrack->StartPtr  = IndexTable->StartPtr;
  PtrTrack->EndPtr    = IndexTable->EndPtr;

  while (IsValidVariableHeader (Variable) && (Variable <= IndexTable->EndPtr)) {
    if (Variable->State == VAR_ADDED) {
      //
      // Record Variable in VariableIndex HOB
      //
      if (IndexTable->Length < VARIABLE_INDEX_TABLE_VOLUME)
      {
        VariableIndexTableUpdate (IndexTable, Variable);
      }

      if (CompareWithValidVariable (Variable, VariableName, VendorGuid, PtrTrack) == EFI_SUCCESS) {
        return EFI_SUCCESS;
      }
    }

    Variable = GetNextVariablePtr (Variable);
  }
  //
  // If gone through the VariableStore, that means we never find in Firmware any more.
  //
  if (IndexTable->Length < VARIABLE_INDEX_TABLE_VOLUME) {
    IndexTable->GoneThrough = 1;
  }

  PtrTrack->CurrPtr = NULL;

  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
PeiGetVariable (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN CHAR16                       *VariableName,
  IN EFI_GUID                     * VendorGuid,
  OUT UINT32                      *Attributes OPTIONAL,
  IN OUT UINTN                    *DataSize,
  OUT VOID                        *Data
  )
/*++

Routine Description:

  Provide the read variable functionality of the variable services.

Arguments:

  PeiServices - General purpose services available to every PEIM.

  VariableName     - The variable name

  VendorGuid       - The vendor's GUID

  Attributes       - Pointer to the attribute

  DataSize         - Size of data

  Data             - Pointer to data

Returns:

  EFI_SUCCESS           - The interface could be successfully installed

  EFI_NOT_FOUND         - The variable could not be discovered

  EFI_BUFFER_TOO_SMALL  - The caller buffer is not large enough

--*/
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarDataSize;
  EFI_STATUS              Status;

  if (VariableName == NULL || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Find existing variable
  //
  Status = FindVariable (PeiServices, VariableName, VendorGuid, &Variable);

  if (Variable.CurrPtr == NULL || Status != EFI_SUCCESS) {
    return Status;
  }
  //
  // Get data size
  //
  VarDataSize = Variable.CurrPtr->DataSize;
  if (*DataSize >= VarDataSize) {
    (*PeiServices)->CopyMem (Data, GET_VARIABLE_DATA_PTR (Variable.CurrPtr), VarDataSize);

    if (Attributes != NULL) {
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
PeiGetNextVariableName (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN OUT UINTN                    *VariableNameSize,
  IN OUT CHAR16                   *VariableName,
  IN OUT EFI_GUID                 *VendorGuid
  )
/*++

Routine Description:

  Provide the get next variable functionality of the variable services.

Arguments:

  PeiServices        - General purpose services available to every PEIM.
  VariabvleNameSize  - The variable name's size.
  VariableName       - A pointer to the variable's name.
  VendorGuid         - A pointer to the EFI_GUID structure.

  VariableNameSize - Size of the variable name

  VariableName     - The variable name

  VendorGuid       - The vendor's GUID

Returns:

  EFI_SUCCESS - The interface could be successfully installed

  EFI_NOT_FOUND - The variable could not be discovered

--*/
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarNameSize;
  EFI_STATUS              Status;

  if (VariableName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FindVariable (PeiServices, VariableName, VendorGuid, &Variable);

  if (Variable.CurrPtr == NULL || Status != EFI_SUCCESS) {
    return Status;
  }

  if (VariableName[0] != 0) {
    //
    // If variable name is not NULL, get next variable
    //
    Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
  }

  while (!(Variable.CurrPtr >= Variable.EndPtr || Variable.CurrPtr == NULL)) {
    if (IsValidVariableHeader (Variable.CurrPtr)) {
      if (Variable.CurrPtr->State == VAR_ADDED) {
        VarNameSize = (UINTN) Variable.CurrPtr->NameSize;
        if (VarNameSize <= *VariableNameSize) {
          (*PeiServices)->CopyMem (VariableName, GET_VARIABLE_NAME_PTR (Variable.CurrPtr), VarNameSize);

          (*PeiServices)->CopyMem (VendorGuid, &Variable.CurrPtr->VendorGuid, sizeof (EFI_GUID));

          Status = EFI_SUCCESS;
        } else {
          Status = EFI_BUFFER_TOO_SMALL;
        }

        *VariableNameSize = VarNameSize;
        return Status;
        //
        // Variable is found
        //
      } else {
        Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
      }
    } else {
      break;
    }
  }

  return EFI_NOT_FOUND;
}
