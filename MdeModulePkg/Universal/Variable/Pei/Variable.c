/** @file
  Implement ReadOnly Variable Services required by PEIM and install
  PEI ReadOnly Varaiable2 PPI. These services operates the non volatile storage space.

Copyright (c) 2006 - 2022, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"
#include "VariableParsing.h"
#include "VariableStore.h"

//
// Module globals
//
EFI_PEI_READ_ONLY_VARIABLE2_PPI  mVariablePpi = {
  PeiGetVariableEx,
  PeiGetNextVariableNameEx
};

EFI_PEI_PPI_DESCRIPTOR  mPpiListVariable = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiReadOnlyVariable2PpiGuid,
  &mVariablePpi
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
  EFI_STATUS                     Status;
  PROTECTED_VARIABLE_CONTEXT_IN  ContextIn;

  //
  // If protected variable services are not supported, EFI_UNSUPPORTED should
  // be always returned. Check it here.
  //
  ContextIn.StructVersion = PROTECTED_VARIABLE_CONTEXT_IN_STRUCT_VERSION;
  ContextIn.StructSize    = sizeof (ContextIn);

  ContextIn.MaxVariableSize             = 0;
  ContextIn.VariableServiceUser         = FromPeiModule;
  ContextIn.GetVariableInfo             = GetVariableInfo;
  ContextIn.GetNextVariableInfo         = GetNextVariableInfo;
  ContextIn.FindVariableSmm             = NULL;
  ContextIn.UpdateVariableStore         = NULL;
  ContextIn.UpdateVariable              = NULL;
  ContextIn.IsHobVariableStoreAvailable = IsHobVariableStoreAvailable;

  Status = ProtectedVariableLibInitialize (&ContextIn);
  if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
    return Status;
  }

  return PeiServicesInstallPpi (&mPpiListVariable);
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
      GetVariableHeader (StoreInfo, MaxIndex, &VariableHeader);
      if (CompareWithValidVariable (StoreInfo, MaxIndex, VariableHeader, VariableName, VendorGuid, PtrTrack) == EFI_SUCCESS) {
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

  if (MaxIndex != NULL) {
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
  if ((Variable.CurrPtr == NULL) || (Status != EFI_SUCCESS)) {
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
PeiGetVariableEx (
  IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *This,
  IN CONST  CHAR16                           *VariableName,
  IN CONST  EFI_GUID                         *VariableGuid,
  OUT       UINT32                           *Attributes,
  IN OUT    UINTN                            *DataSize,
  OUT       VOID                             *Data OPTIONAL
  )
{
  EFI_STATUS  Status;

  //
  // If variable protection is employed, always get variable data through
  // ProtectedVariableLib.
  //
  Status = ProtectedVariableLibGetByName (VariableName, VariableGuid, Attributes, DataSize, Data);
  if (Status != EFI_UNSUPPORTED) {
    return Status;
  }

  return PeiGetVariable (This, VariableName, VariableGuid, Attributes, DataSize, Data);
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
PeiGetNextVariableNameEx (
  IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *This,
  IN OUT UINTN                               *VariableNameSize,
  IN OUT CHAR16                              *VariableName,
  IN OUT EFI_GUID                            *VariableGuid
  )
{
  EFI_STATUS  Status;

  //
  // If variable protection is employed, always get next variable through
  // ProtectedVariableLib.
  //
  Status = ProtectedVariableLibFindNext (VariableNameSize, VariableName, VariableGuid);
  if (Status != EFI_UNSUPPORTED) {
    return Status;
  }

  return PeiGetNextVariableName (This, VariableNameSize, VariableName, VariableGuid);
}
