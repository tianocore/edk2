/** @file
  Functions in this module are associated with variable parsing operations and
  are intended to be usable across variable driver source files.

Copyright (c) 2019 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"
#include "VariableParsing.h"

/**

  This code checks if variable header is valid or not.

  @param[in] Variable           Pointer to the Variable Header.
  @param[in] VariableStoreEnd   Pointer to the Variable Store End.

  @retval TRUE              Variable header is valid.
  @retval FALSE             Variable header is not valid.

**/
BOOLEAN
IsValidVariableHeader (
  IN  VARIABLE_HEADER       *Variable,
  IN  VARIABLE_HEADER       *VariableStoreEnd,
  IN  BOOLEAN               AuthFormat
  )
{
  if ((Variable == NULL)
      || (((UINTN)Variable + GetVariableHeaderSize (AuthFormat)) >= (UINTN)VariableStoreEnd)
      || (Variable->StartId != VARIABLE_DATA)) {
    //
    // Variable is NULL or has reached the end of variable store,
    // or the StartId is not correct.
    //
    return FALSE;
  }

  return TRUE;
}

/**

  This code gets the current status of Variable Store.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @retval EfiRaw         Variable store status is raw.
  @retval EfiValid       Variable store status is valid.
  @retval EfiInvalid     Variable store status is invalid.

**/
VARIABLE_STORE_STATUS
GetVariableStoreStatus (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  if ((CompareGuid (&VarStoreHeader->Signature, &gEfiAuthenticatedVariableGuid) ||
       CompareGuid (&VarStoreHeader->Signature, &gEfiVariableGuid)) &&
      VarStoreHeader->Format == VARIABLE_STORE_FORMATTED &&
      VarStoreHeader->State == VARIABLE_STORE_HEALTHY
      ) {

    return EfiValid;
  } else if ((((UINT32 *)(&VarStoreHeader->Signature))[0] == 0xffffffff) &&
             (((UINT32 *)(&VarStoreHeader->Signature))[1] == 0xffffffff) &&
             (((UINT32 *)(&VarStoreHeader->Signature))[2] == 0xffffffff) &&
             (((UINT32 *)(&VarStoreHeader->Signature))[3] == 0xffffffff) &&
             (VarStoreHeader->Size == 0xffffffff) &&
             (VarStoreHeader->Format == 0xff) &&
             (VarStoreHeader->State == 0xff)
             ) {
    return EfiRaw;
  } else {
    return EfiInvalid;
  }
}

/**
  This code gets the size of variable header.

  @param[in]  AuthFormat    TRUE indicates authenticated variables are used.
                            FALSE indicates authenticated variables are not used.

  @return Size of variable header in bytes in type UINTN.

**/
UINTN
GetVariableHeaderSize (
  IN  BOOLEAN  AuthFormat
  )
{
  UINTN  Value;

  if (AuthFormat) {
    Value = sizeof (AUTHENTICATED_VARIABLE_HEADER);
  } else {
    Value = sizeof (VARIABLE_HEADER);
  }

  return Value;
}

/**

  This code gets the size of name of variable.

  @param[in]  Variable      Pointer to the variable header.
  @param[in]  AuthFormat    TRUE indicates authenticated variables are used.
                            FALSE indicates authenticated variables are not used.

  @return UINTN          Size of variable in bytes.

**/
UINTN
NameSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (AuthFormat) {
    if ((AuthVariable->State == (UINT8)(-1)) ||
        (AuthVariable->DataSize == (UINT32)(-1)) ||
        (AuthVariable->NameSize == (UINT32)(-1)) ||
        (AuthVariable->Attributes == (UINT32)(-1))) {
      return 0;
    }

    return (UINTN)AuthVariable->NameSize;
  } else {
    if ((Variable->State == (UINT8)(-1)) ||
        (Variable->DataSize == (UINT32)(-1)) ||
        (Variable->NameSize == (UINT32)(-1)) ||
        (Variable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)Variable->NameSize;
  }
}

/**
  This code sets the size of name of variable.

  @param[in]  Variable      Pointer to the Variable Header.
  @param[in]  NameSize      Name size to set.
  @param[in]  AuthFormat    TRUE indicates authenticated variables are used.
                            FALSE indicates authenticated variables are not used.

**/
VOID
SetNameSizeOfVariable (
  IN VARIABLE_HEADER  *Variable,
  IN UINTN            NameSize,
  IN BOOLEAN          AuthFormat
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (AuthFormat) {
    AuthVariable->NameSize = (UINT32)NameSize;
  } else {
    Variable->NameSize = (UINT32)NameSize;
  }
}

/**

  This code gets the size of variable data.

  @param[in]  Variable      Pointer to the Variable Header.
  @param[in]  AuthFormat    TRUE indicates authenticated variables are used.
                            FALSE indicates authenticated variables are not used.

  @return Size of variable in bytes.

**/
UINTN
DataSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (AuthFormat) {
    if ((AuthVariable->State == (UINT8)(-1)) ||
        (AuthVariable->DataSize == (UINT32)(-1)) ||
        (AuthVariable->NameSize == (UINT32)(-1)) ||
        (AuthVariable->Attributes == (UINT32)(-1))) {
      return 0;
    }

    return (UINTN)AuthVariable->DataSize;
  } else {
    if ((Variable->State == (UINT8)(-1)) ||
        (Variable->DataSize == (UINT32)(-1)) ||
        (Variable->NameSize == (UINT32)(-1)) ||
        (Variable->Attributes == (UINT32)(-1))) {
      return 0;
    }

    return (UINTN)Variable->DataSize;
  }
}

/**
  This code sets the size of variable data.

  @param[in] Variable   Pointer to the Variable Header.
  @param[in] DataSize   Data size to set.
  @param[in] AuthFormat TRUE indicates authenticated variables are used.
                        FALSE indicates authenticated variables are not used.

**/
VOID
SetDataSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  UINTN            DataSize,
  IN  BOOLEAN          AuthFormat
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (AuthFormat) {
    AuthVariable->DataSize = (UINT32)DataSize;
  } else {
    Variable->DataSize = (UINT32)DataSize;
  }
}

/**

  This code gets the pointer to the variable name.

  @param[in] Variable     Pointer to the Variable Header.
  @param[in] AuthFormat   TRUE indicates authenticated variables are used.
                          FALSE indicates authenticated variables are not used.

  @return Pointer to Variable Name which is Unicode encoding.

**/
CHAR16 *
GetVariableNamePtr (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  )
{
  return (CHAR16 *)((UINTN)Variable + GetVariableHeaderSize (AuthFormat));
}

/**
  This code gets the pointer to the variable guid.

  @param[in] Variable     Pointer to the Variable Header.
  @param[in] AuthFormat   TRUE indicates authenticated variables are used.
                          FALSE indicates authenticated variables are not used.

  @return A EFI_GUID* pointer to Vendor Guid.

**/
EFI_GUID *
GetVendorGuidPtr (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (AuthFormat) {
    return &AuthVariable->VendorGuid;
  } else {
    return &Variable->VendorGuid;
  }
}

/**

  This code gets the pointer to the variable data.

  @param[in] Variable     Pointer to the Variable Header.
  @param[in] AuthFormat   TRUE indicates authenticated variables are used.
                          FALSE indicates authenticated variables are not used.

  @return Pointer to Variable Data.

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  )
{
  UINTN  Value;

  //
  // Be careful about pad size for alignment.
  //
  Value  =  (UINTN)GetVariableNamePtr (Variable, AuthFormat);
  Value += NameSizeOfVariable (Variable, AuthFormat);
  Value += GET_PAD_SIZE (NameSizeOfVariable (Variable, AuthFormat));

  return (UINT8 *)Value;
}

/**
  This code gets the variable data offset related to variable header.

  @param[in] Variable     Pointer to the Variable Header.
  @param[in] AuthFormat   TRUE indicates authenticated variables are used.
                          FALSE indicates authenticated variables are not used.

  @return Variable Data offset.

**/
UINTN
GetVariableDataOffset (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  )
{
  UINTN  Value;

  //
  // Be careful about pad size for alignment
  //
  Value  = GetVariableHeaderSize (AuthFormat);
  Value += NameSizeOfVariable (Variable, AuthFormat);
  Value += GET_PAD_SIZE (NameSizeOfVariable (Variable, AuthFormat));

  return Value;
}

/**
  Get variable data payload.

  @param[in]      Variable     Pointer to the Variable Header.
  @param[out]     Data         Pointer to buffer used to store the variable data.
  @param[in]      DataSize     Size of buffer passed by Data.
  @param[out]     DataSize     Size of data copied into Data buffer.
  @param[in]      AuthFlag     Auth-variable indicator.

  @return EFI_SUCCESS             Data was fetched.
  @return EFI_INVALID_PARAMETER   DataSize is NULL.
  @return EFI_BUFFER_TOO_SMALL    DataSize is smaller than size of variable data.

**/
EFI_STATUS
GetVariableData (
  IN      VARIABLE_HEADER     *Variable,
  IN  OUT VOID                *Data,
  IN  OUT UINT32              *DataSize,
  IN      BOOLEAN             AuthFlag
  )
{
  UINT32            Size;

  if (DataSize == NULL) {
    ASSERT (DataSize != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Size = (UINT32)DataSizeOfVariable (Variable, AuthFlag);
  if (*DataSize < Size) {
    *DataSize = Size;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Data == NULL) {
    ASSERT (Data != NULL);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (Data, GetVariableDataPtr (Variable, AuthFlag), Size);
  *DataSize = Size;

  return EFI_SUCCESS;
}

/**

  This code gets the pointer to the next variable header.

  @param[in] Variable     Pointer to the Variable Header.
  @param[in] AuthFormat   TRUE indicates authenticated variables are used.
                          FALSE indicates authenticated variables are not used.

  @return Pointer to next variable header.

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFormat
  )
{
  UINTN  Value;

  Value  =  (UINTN)GetVariableDataPtr (Variable, AuthFormat);
  Value += DataSizeOfVariable (Variable, AuthFormat);
  Value += GET_PAD_SIZE (DataSizeOfVariable (Variable, AuthFormat));

  //
  // Be careful about pad size for alignment.
  //
  return (VARIABLE_HEADER *)HEADER_ALIGN (Value);
}

/**

  Gets the pointer to the first variable header in given variable store area.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the first variable header.

**/
VARIABLE_HEADER *
GetStartPointer (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  //
  // The start of variable store.
  //
  return (VARIABLE_HEADER *)HEADER_ALIGN (VarStoreHeader + 1);
}

/**

  Gets the pointer to the end of the variable storage area.

  This function gets pointer to the end of the variable storage
  area, according to the input variable store header.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the end of the variable storage area.

**/
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *)HEADER_ALIGN ((UINTN)VarStoreHeader + VarStoreHeader->Size);
}

/**
  Compare two EFI_TIME data.


  @param[in] FirstTime       A pointer to the first EFI_TIME data.
  @param[in] SecondTime      A pointer to the second EFI_TIME data.

  @retval  TRUE              The FirstTime is not later than the SecondTime.
  @retval  FALSE             The FirstTime is later than the SecondTime.

**/
BOOLEAN
VariableCompareTimeStampInternal (
  IN EFI_TIME  *FirstTime,
  IN EFI_TIME  *SecondTime
  )
{
  if (FirstTime->Year != SecondTime->Year) {
    return (BOOLEAN)(FirstTime->Year < SecondTime->Year);
  } else if (FirstTime->Month != SecondTime->Month) {
    return (BOOLEAN)(FirstTime->Month < SecondTime->Month);
  } else if (FirstTime->Day != SecondTime->Day) {
    return (BOOLEAN)(FirstTime->Day < SecondTime->Day);
  } else if (FirstTime->Hour != SecondTime->Hour) {
    return (BOOLEAN)(FirstTime->Hour < SecondTime->Hour);
  } else if (FirstTime->Minute != SecondTime->Minute) {
    return (BOOLEAN)(FirstTime->Minute < SecondTime->Minute);
  }

  return (BOOLEAN)(FirstTime->Second <= SecondTime->Second);
}

/**
  Find the variable in the specified variable store.

  @param[in]       VariableName        Name of the variable to be found
  @param[in]       VendorGuid          Vendor GUID to be found.
  @param[in]       IgnoreRtCheck       Ignore EFI_VARIABLE_RUNTIME_ACCESS attribute
                                       check at runtime when searching variable.
  @param[in, out]  PtrTrack            Variable Track Pointer structure that contains Variable Information.
  @param[in]       AuthFormat          TRUE indicates authenticated variables are used.
                                       FALSE indicates authenticated variables are not used.

  @retval          EFI_SUCCESS         Variable found successfully
  @retval          EFI_NOT_FOUND       Variable not found
**/
EFI_STATUS
FindVariableEx (
  IN     CHAR16                  *VariableName,
  IN     EFI_GUID                *VendorGuid,
  IN     BOOLEAN                 IgnoreRtCheck,
  IN OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN     BOOLEAN                 AuthFormat
  )
{
  VARIABLE_HEADER  *InDeletedVariable;
  VOID             *Point;

  PtrTrack->InDeletedTransitionPtr = NULL;

  //
  // Find the variable by walk through HOB, volatile and non-volatile variable store.
  //
  InDeletedVariable = NULL;

  for ( PtrTrack->CurrPtr = PtrTrack->StartPtr
      ; IsValidVariableHeader (PtrTrack->CurrPtr, PtrTrack->EndPtr, AuthFormat)
      ; PtrTrack->CurrPtr = GetNextVariablePtr (PtrTrack->CurrPtr, AuthFormat)
      ) {
    if (PtrTrack->CurrPtr->State == VAR_ADDED ||
        PtrTrack->CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)
       ) {
      if (IgnoreRtCheck || !AtRuntime () || ((PtrTrack->CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) != 0)) {
        if (VariableName[0] == 0) {
          if (PtrTrack->CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
            InDeletedVariable = PtrTrack->CurrPtr;
          } else {
            PtrTrack->InDeletedTransitionPtr = InDeletedVariable;
            return EFI_SUCCESS;
          }
        } else {
          if (CompareGuid (VendorGuid, GetVendorGuidPtr (PtrTrack->CurrPtr, AuthFormat))) {
            Point = (VOID *)GetVariableNamePtr (PtrTrack->CurrPtr, AuthFormat);

            ASSERT (NameSizeOfVariable (PtrTrack->CurrPtr, AuthFormat) != 0);
            if (CompareMem (VariableName, Point, NameSizeOfVariable (PtrTrack->CurrPtr, AuthFormat)) == 0) {
              if (PtrTrack->CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
                InDeletedVariable = PtrTrack->CurrPtr;
              } else {
                PtrTrack->InDeletedTransitionPtr = InDeletedVariable;
                return EFI_SUCCESS;
              }
            }
          }
        }
      }
    }
  }

  PtrTrack->CurrPtr = InDeletedVariable;
  return (PtrTrack->CurrPtr  == NULL) ? EFI_NOT_FOUND : EFI_SUCCESS;
}

/**
  This code finds the next available variable.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param[in]  VariableName      Pointer to variable name.
  @param[in]  VendorGuid        Variable Vendor Guid.
  @param[in]  VariableStoreList A list of variable stores that should be used to get the next variable.
                                The maximum number of entries is the max value of VARIABLE_STORE_TYPE.
  @param[out] VariablePtr       Pointer to variable header address.
  @param[in]  AuthFormat        TRUE indicates authenticated variables are used.
                                FALSE indicates authenticated variables are not used.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The next variable was not found.
  @retval EFI_INVALID_PARAMETER If VariableName is not an empty string, while VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER The input values of VariableName and VendorGuid are not a name and
                                GUID of an existing variable.

**/
EFI_STATUS
EFIAPI
VariableServiceGetNextVariableInternal (
  IN  CHAR16                 *VariableName,
  IN  EFI_GUID               *VendorGuid,
  IN  VARIABLE_STORE_HEADER  **VariableStoreList,
  OUT VARIABLE_HEADER        **VariablePtr,
  IN  BOOLEAN                AuthFormat
  )
{
  EFI_STATUS              Status;
  VARIABLE_STORE_TYPE     StoreType;
  VARIABLE_POINTER_TRACK  Variable;
  VARIABLE_POINTER_TRACK  VariableInHob;
  VARIABLE_POINTER_TRACK  VariablePtrTrack;

  Status = EFI_NOT_FOUND;

  if (VariableStoreList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&Variable, sizeof (Variable));

  // Check if the variable exists in the given variable store list
  for (StoreType = (VARIABLE_STORE_TYPE)0; StoreType < VariableStoreTypeMax; StoreType++) {
    if (VariableStoreList[StoreType] == NULL) {
      continue;
    }

    Variable.StartPtr = GetStartPointer (VariableStoreList[StoreType]);
    Variable.EndPtr   = GetEndPointer (VariableStoreList[StoreType]);
    Variable.Volatile = (BOOLEAN)(StoreType == VariableStoreTypeVolatile);

    Status = FindVariableEx (VariableName, VendorGuid, FALSE, &Variable, AuthFormat);
    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  if ((Variable.CurrPtr == NULL) || EFI_ERROR (Status)) {
    //
    // For VariableName is an empty string, FindVariableEx() will try to find and return
    // the first qualified variable, and if FindVariableEx() returns error (EFI_NOT_FOUND)
    // as no any variable is found, still go to return the error (EFI_NOT_FOUND).
    //
    if (VariableName[0] != 0) {
      //
      // For VariableName is not an empty string, and FindVariableEx() returns error as
      // VariableName and VendorGuid are not a name and GUID of an existing variable,
      // there is no way to get next variable, follow spec to return EFI_INVALID_PARAMETER.
      //
      Status = EFI_INVALID_PARAMETER;
    }

    goto Done;
  }

  if (VariableName[0] != 0) {
    //
    // If variable name is not empty, get next variable.
    //
    Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr, AuthFormat);
  }

  while (TRUE) {
    //
    // Switch to the next variable store if needed
    //
    while (!IsValidVariableHeader (Variable.CurrPtr, Variable.EndPtr, AuthFormat)) {
      //
      // Find current storage index
      //
      for (StoreType = (VARIABLE_STORE_TYPE)0; StoreType < VariableStoreTypeMax; StoreType++) {
        if ((VariableStoreList[StoreType] != NULL) && (Variable.StartPtr == GetStartPointer (VariableStoreList[StoreType]))) {
          break;
        }
      }

      ASSERT (StoreType < VariableStoreTypeMax);
      //
      // Switch to next storage
      //
      for (StoreType++; StoreType < VariableStoreTypeMax; StoreType++) {
        if (VariableStoreList[StoreType] != NULL) {
          break;
        }
      }

      //
      // Capture the case that
      // 1. current storage is the last one, or
      // 2. no further storage
      //
      if (StoreType == VariableStoreTypeMax) {
        Status = EFI_NOT_FOUND;
        goto Done;
      }

      Variable.StartPtr = GetStartPointer (VariableStoreList[StoreType]);
      Variable.EndPtr   = GetEndPointer (VariableStoreList[StoreType]);
      Variable.CurrPtr  = Variable.StartPtr;
    }

    //
    // Variable is found
    //
    if ((Variable.CurrPtr->State == VAR_ADDED) || (Variable.CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED))) {
      if (!AtRuntime () || ((Variable.CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) != 0)) {
        if (Variable.CurrPtr->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
          //
          // If it is a IN_DELETED_TRANSITION variable,
          // and there is also a same ADDED one at the same time,
          // don't return it.
          //
          VariablePtrTrack.StartPtr = Variable.StartPtr;
          VariablePtrTrack.EndPtr   = Variable.EndPtr;
          Status                    = FindVariableEx (
                                        GetVariableNamePtr (Variable.CurrPtr, AuthFormat),
                                        GetVendorGuidPtr (Variable.CurrPtr, AuthFormat),
                                        FALSE,
                                        &VariablePtrTrack,
                                        AuthFormat
                                        );
          if (!EFI_ERROR (Status) && (VariablePtrTrack.CurrPtr->State == VAR_ADDED)) {
            Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr, AuthFormat);
            continue;
          }
        }

        //
        // Don't return NV variable when HOB overrides it
        //
        if ((VariableStoreList[VariableStoreTypeHob] != NULL) && (VariableStoreList[VariableStoreTypeNv] != NULL) &&
            (Variable.StartPtr == GetStartPointer (VariableStoreList[VariableStoreTypeNv]))
           ) {
          VariableInHob.StartPtr = GetStartPointer (VariableStoreList[VariableStoreTypeHob]);
          VariableInHob.EndPtr   = GetEndPointer (VariableStoreList[VariableStoreTypeHob]);
          Status                 = FindVariableEx (
                                     GetVariableNamePtr (Variable.CurrPtr, AuthFormat),
                                     GetVendorGuidPtr (Variable.CurrPtr, AuthFormat),
                                     FALSE,
                                     &VariableInHob,
                                     AuthFormat
                                     );
          if (!EFI_ERROR (Status)) {
            Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr, AuthFormat);
            continue;
          }
        }

        *VariablePtr = Variable.CurrPtr;
        Status       = EFI_SUCCESS;
        goto Done;
      }
    }

    Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr, AuthFormat);
  }

Done:
  return Status;
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

  @param[in]      VariableName   Name of the Variable to track.
  @param[in]      VendorGuid     Guid of the Variable to track.
  @param[in]      Volatile       TRUE if volatile FALSE if non-volatile.
  @param[in]      Read           TRUE if GetVariable() was called.
  @param[in]      Write          TRUE if SetVariable() was called.
  @param[in]      Delete         TRUE if deleted via SetVariable().
  @param[in]      Cache          TRUE for a cache hit.
  @param[in,out]  VariableInfo   Pointer to a pointer of VARIABLE_INFO_ENTRY structures.

**/
VOID
UpdateVariableInfo (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  IN  BOOLEAN                 Volatile,
  IN  BOOLEAN                 Read,
  IN  BOOLEAN                 Write,
  IN  BOOLEAN                 Delete,
  IN  BOOLEAN                 Cache,
  IN OUT VARIABLE_INFO_ENTRY  **VariableInfo
  )
{
  VARIABLE_INFO_ENTRY  *Entry;

  if (FeaturePcdGet (PcdVariableCollectStatistics)) {
    if ((VariableName == NULL) || (VendorGuid == NULL) || (VariableInfo == NULL)) {
      return;
    }

    if (AtRuntime ()) {
      // Don't collect statistics at runtime.
      return;
    }

    if (*VariableInfo == NULL) {
      //
      // On the first call allocate a entry and place a pointer to it in
      // the EFI System Table.
      //
      *VariableInfo = AllocateZeroPool (sizeof (VARIABLE_INFO_ENTRY));
      ASSERT (*VariableInfo != NULL);

      CopyGuid (&(*VariableInfo)->VendorGuid, VendorGuid);
      (*VariableInfo)->Name = AllocateZeroPool (StrSize (VariableName));
      ASSERT ((*VariableInfo)->Name != NULL);
      StrCpyS ((*VariableInfo)->Name, StrSize (VariableName)/sizeof (CHAR16), VariableName);
      (*VariableInfo)->Volatile = Volatile;
    }

    for (Entry = (*VariableInfo); Entry != NULL; Entry = Entry->Next) {
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
        // Next iteration of the loop will fill in the data.
        //
        Entry->Next = AllocateZeroPool (sizeof (VARIABLE_INFO_ENTRY));
        ASSERT (Entry->Next != NULL);

        CopyGuid (&Entry->Next->VendorGuid, VendorGuid);
        Entry->Next->Name = AllocateZeroPool (StrSize (VariableName));
        ASSERT (Entry->Next->Name != NULL);
        StrCpyS (Entry->Next->Name, StrSize (VariableName)/sizeof (CHAR16), VariableName);
        Entry->Next->Volatile = Volatile;
      }
    }
  }
}

/**

  Retrieve details about a variable and return them in VariableInfo->Header.

  If VariableInfo->Address is given, this function will calculate its offset
  relative to given variable storage via VariableStore; Otherwise, it will try
  other internal variable storages or cached copies. It's assumed that, for all
  copies of NV variable storage, all variables are stored in the same relative
  position. If VariableInfo->Address is found in the range of any storage copies,
  its offset relative to that storage should be the same in other copies.

  If VariableInfo->Offset is given (non-zero) but not VariableInfo->Address,
  this function will return the variable memory address inside VariableStore,
  if given, via VariableInfo->Address; Otherwise, the address of other storage
  copies will be returned, if any.

  For a new variable whose offset has not been determined, a value of -1 as
  VariableInfo->Offset should be passed to skip the offset calculation.

  @param VariableStore            Pointer to a variable storage. It's optional.
  @param VariableInfo             Pointer to variable information.

  @retval EFI_INVALID_PARAMETER  VariableInfo is NULL or both VariableInfo->Address
                                 and VariableInfo->Offset are NULL (0).
  @retval EFI_NOT_FOUND          If given Address or Offset is out of range of
                                 any given or internal storage copies.
  @retval EFI_SUCCESS            Variable details are retrieved successfully.

**/
EFI_STATUS
EFIAPI
GetVariableInfo (
  IN OUT  PROTECTED_VARIABLE_INFO   *VariableInfo
  )
{
  VARIABLE_STORE_HEADER         *Stores[2];
  UINTN                         Index;
  VARIABLE_HEADER               *VariablePtr;
  VARIABLE_HEADER               *VariableBuffer;
  AUTHENTICATED_VARIABLE_HEADER *AuthVariablePtr;
  BOOLEAN                       AuthFlag;
  UINTN                         NameSize;
  UINTN                         DataSize;
  UINTN                         VariableSize;

  if (VariableInfo == NULL || (VariableInfo->Buffer == NULL
                          && VariableInfo->StoreIndex == VAR_INDEX_INVALID))
  {
    ASSERT (VariableInfo != NULL);
    ASSERT (VariableInfo->Buffer != NULL || VariableInfo->StoreIndex != VAR_INDEX_INVALID);
    return EFI_INVALID_PARAMETER;
  }

  Stores[0] = mNvVariableCache;
  Stores[1] = (mVariableModuleGlobal != NULL)
              ? (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase
              : NULL;

  VariableBuffer  = VariableInfo->Buffer;
  VariablePtr     = NULL;
  if (VariableInfo->StoreIndex != VAR_INDEX_INVALID) {
    for (Index = 0; Index < ARRAY_SIZE (Stores); ++Index) {
      if (Stores[Index] == NULL) {
        continue;
      }

      if ((UINTN)VariableInfo->StoreIndex
          < ((UINTN)GetEndPointer (Stores[Index]) - (UINTN)Stores[Index]))
      {
        VariablePtr = (VARIABLE_HEADER *)((UINTN)Stores[Index] + (UINTN)VariableInfo->StoreIndex);
        VariableInfo->Buffer = VariablePtr;
        break;
      }
    }
  } else {
    VariablePtr = VariableInfo->Buffer;
  }

  if (VariablePtr == NULL) {
    return EFI_NOT_FOUND;
  }

  AuthFlag = VariableInfo->Flags.Auth;
  ASSERT (AuthFlag == TRUE || AuthFlag == FALSE);

  //
  // Make a copy of the whole variable if a buffer is passed in.
  //
  if (VariableBuffer != NULL && VariableBuffer != VariablePtr) {
    VariableSize = (UINTN)GetNextVariablePtr (VariablePtr, AuthFlag)
                   - (UINTN)VariablePtr;
    CopyMem (VariableBuffer, VariablePtr, VariableSize);
  }

  //
  // AuthVariable header
  //
  if (AuthFlag) {
    AuthVariablePtr = (AUTHENTICATED_VARIABLE_HEADER *)VariablePtr;

    VariableInfo->Header.State          = AuthVariablePtr->State;
    VariableInfo->Header.Attributes     = AuthVariablePtr->Attributes;
    VariableInfo->Header.PubKeyIndex    = AuthVariablePtr->PubKeyIndex;
    VariableInfo->Header.MonotonicCount = ReadUnaligned64 (
                                            &(AuthVariablePtr->MonotonicCount)
                                            );
    if (VariableInfo->Header.TimeStamp != NULL) {
      CopyMem (VariableInfo->Header.TimeStamp, &AuthVariablePtr->TimeStamp,
               sizeof (EFI_TIME));
    } else if (VariableBuffer != NULL) {
      AuthVariablePtr = (AUTHENTICATED_VARIABLE_HEADER *)VariableBuffer;
      VariableInfo->Header.TimeStamp    = &AuthVariablePtr->TimeStamp;
    }
  } else {
    VariableInfo->Header.State          = VariablePtr->State;
    VariableInfo->Header.Attributes     = VariablePtr->Attributes;
    VariableInfo->Header.PubKeyIndex    = 0;
    VariableInfo->Header.MonotonicCount = 0;
    VariableInfo->Header.TimeStamp      = NULL;
  }

  //
  // VendorGuid
  //
  if (VariableInfo->Header.VendorGuid != NULL) {
    CopyGuid (VariableInfo->Header.VendorGuid,
              GetVendorGuidPtr (VariablePtr, AuthFlag));
  } else {
    VariableInfo->Header.VendorGuid = GetVendorGuidPtr (VariablePtr, AuthFlag);
  }

  //
  // VariableName
  //
  NameSize = NameSizeOfVariable (VariablePtr, AuthFlag);
  if (VariableInfo->Header.VariableName != NULL
      && VariableInfo->Header.NameSize >= NameSize)
  {
    CopyMem (
      VariableInfo->Header.VariableName,
      GetVariableNamePtr (VariablePtr, AuthFlag),
      NameSize
      );
  } else if (VariableInfo->Header.VariableName != NULL) {
    return EFI_BUFFER_TOO_SMALL;
  } else {
    VariableInfo->Header.VariableName = GetVariableNamePtr (VariablePtr, AuthFlag);
  }

  //
  // Data
  //
  DataSize = DataSizeOfVariable (VariablePtr, AuthFlag);
  if (VariableInfo->Header.Data != NULL
      && VariableInfo->Header.DataSize >= DataSize)
  {
    CopyMem (
      VariableInfo->Header.Data,
      GetVariableDataPtr (VariablePtr, AuthFlag),
      NameSize
      );
  } else if (VariableInfo->Header.Data != NULL) {
    return EFI_BUFFER_TOO_SMALL;
  } else {
    VariableInfo->Header.Data = GetVariableDataPtr (VariablePtr, AuthFlag);
  }

  //
  // Update size information about name & data.
  //
  VariableInfo->Header.NameSize = NameSize;
  VariableInfo->Header.DataSize = DataSize;

  return EFI_SUCCESS;
}

/**

  Retrieve details of the variable next to given variable within VariableStore.

  If VarInfo->Address is NULL, the first one in VariableStore is returned.

  VariableStart and/or VariableEnd can be given optionally for the situation
  in which the valid storage space is smaller than the VariableStore->Size.
  This usually happens when PEI variable services make a compact variable
  cache to save memory, which cannot make use VariableStore->Size to determine
  the correct variable storage range.

  @param VariableStore            Pointer to a variable storage. It's optional.
  @param VariableStart            Start point of valid range in VariableStore.
  @param VariableEnd              End point of valid range in VariableStore.
  @param VariableInfo             Pointer to variable information.

  @retval EFI_INVALID_PARAMETER  VariableInfo or VariableStore is NULL.
  @retval EFI_NOT_FOUND          If the end of VariableStore is reached.
  @retval EFI_SUCCESS            The next variable is retrieved successfully.

**/
EFI_STATUS
EFIAPI
GetNextVariableInfo (
  IN OUT  PROTECTED_VARIABLE_INFO   *VariableInfo
  )
{
  VARIABLE_STORE_HEADER         *VarStore;
  VARIABLE_HEADER               *VariablePtr;
  VARIABLE_HEADER               *VariableStart;
  VARIABLE_HEADER               *VariableEnd;
  BOOLEAN                       AuthFlag;

  if (VariableInfo == NULL) {
    ASSERT (VariableInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (mNvVariableCache != NULL) {
    VarStore = mNvVariableCache;
  } else if (mVariableModuleGlobal != NULL) {
    VarStore = (VARIABLE_STORE_HEADER *)(UINTN)
               mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase;
  } else {
    return EFI_NOT_FOUND;
  }

  VariableStart = GetStartPointer (VarStore);
  VariableEnd   = GetEndPointer (VarStore);

  if (VariableInfo->Flags.Auth != TRUE && VariableInfo->Flags.Auth != FALSE) {
    VariableInfo->Flags.Auth = CompareGuid (
                            &VarStore->Signature,
                            &gEfiAuthenticatedVariableGuid
                            );
  }
  AuthFlag = VariableInfo->Flags.Auth;


  if (VariableInfo->StoreIndex == VAR_INDEX_INVALID) {
    VariablePtr = VariableStart;
  } else {
    VariablePtr = (VARIABLE_HEADER *)
                  ((UINTN)VarStore + (UINTN)VariableInfo->StoreIndex);
    if (VariablePtr >= VariableEnd) {
      return EFI_NOT_FOUND;
    }
    VariablePtr = GetNextVariablePtr (VariablePtr, AuthFlag);
  }

  if (!IsValidVariableHeader (VariablePtr, VariableEnd, AuthFlag)) {
    return EFI_NOT_FOUND;
  }

  VariableInfo->StoreIndex = (UINTN)VariablePtr - (UINTN)VarStore;
  return GetVariableInfo (VariableInfo);
}
