/** @file
  Implementation functions and structures for var check services.

Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/VarCheckLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Guid/GlobalVariable.h>
#include <Guid/HardwareErrorVariable.h>

BOOLEAN mVarCheckLibEndOfDxe    = FALSE;

#define VAR_CHECK_TABLE_SIZE    0x8

UINTN                                   mVarCheckLibEndOfDxeCallbackCount = 0;
UINTN                                   mVarCheckLibEndOfDxeCallbackMaxCount = 0;
VAR_CHECK_END_OF_DXE_CALLBACK           *mVarCheckLibEndOfDxeCallback = NULL;

UINTN                                   mVarCheckLibAddressPointerCount = 0;
UINTN                                   mVarCheckLibAddressPointerMaxCount = 0;
VOID                                    ***mVarCheckLibAddressPointer = NULL;

UINTN                                   mNumberOfVarCheckHandler = 0;
UINTN                                   mMaxNumberOfVarCheckHandler = 0;
VAR_CHECK_SET_VARIABLE_CHECK_HANDLER    *mVarCheckHandlerTable = NULL;

typedef struct {
  EFI_GUID                      Guid;
  VAR_CHECK_VARIABLE_PROPERTY   VariableProperty;
  //CHAR16                        *Name;
} VAR_CHECK_VARIABLE_ENTRY;

UINTN                                   mNumberOfVarCheckVariable = 0;
UINTN                                   mMaxNumberOfVarCheckVariable = 0;
VARIABLE_ENTRY_PROPERTY                 **mVarCheckVariableTable = NULL;

//
// Handle variables with wildcard name specially.
//
VARIABLE_ENTRY_PROPERTY mVarCheckVariableWithWildcardName[] = {
  {
    &gEfiGlobalVariableGuid,
    L"Boot####",
    {
      0
    },
  },
  {
    &gEfiGlobalVariableGuid,
    L"Driver####",
    {
      0
    },
  },
  {
    &gEfiGlobalVariableGuid,
    L"SysPrep####",
    {
      0
    },
  },
  {
    &gEfiGlobalVariableGuid,
    L"Key####",
    {
      0
    },
  },
  {
    &gEfiGlobalVariableGuid,
    L"PlatformRecovery####",
    {
      0
    },
  },
  {
    &gEfiHardwareErrorVariableGuid,
    L"HwErrRec####",
    {
      0
    },
  },
};

/**
  Check if a Unicode character is an upper case hexadecimal character.

  This function checks if a Unicode character is an upper case
  hexadecimal character.  The valid upper case hexadecimal character is
  L'0' to L'9', or L'A' to L'F'.


  @param[in] Char       The character to check against.

  @retval TRUE          If the Char is an upper case hexadecmial character.
  @retval FALSE         If the Char is not an upper case hexadecmial character.

**/
BOOLEAN
EFIAPI
VarCheckInternalIsHexaDecimalDigitCharacter (
  IN CHAR16             Char
  )
{
  return (BOOLEAN) ((Char >= L'0' && Char <= L'9') || (Char >= L'A' && Char <= L'F'));
}

/**
  Variable property get with wildcard name.

  @param[in] VariableName       Pointer to variable name.
  @param[in] VendorGuid         Pointer to variable vendor GUID.
  @param[in] WildcardMatch      Try wildcard match or not.

  @return Pointer to variable property.

**/
VAR_CHECK_VARIABLE_PROPERTY *
VariablePropertyGetWithWildcardName (
  IN CHAR16                         *VariableName,
  IN EFI_GUID                       *VendorGuid,
  IN BOOLEAN                        WildcardMatch
  )
{
  UINTN     Index;
  UINTN     NameLength;

  NameLength = StrLen (VariableName) - 4;
  for (Index = 0; Index < sizeof (mVarCheckVariableWithWildcardName)/sizeof (mVarCheckVariableWithWildcardName[0]); Index++) {
    if (CompareGuid (mVarCheckVariableWithWildcardName[Index].Guid, VendorGuid)){
      if (WildcardMatch) {
        if ((StrLen (VariableName) == StrLen (mVarCheckVariableWithWildcardName[Index].Name)) &&
            (StrnCmp (VariableName, mVarCheckVariableWithWildcardName[Index].Name, NameLength) == 0) &&
            VarCheckInternalIsHexaDecimalDigitCharacter (VariableName[NameLength]) &&
            VarCheckInternalIsHexaDecimalDigitCharacter (VariableName[NameLength + 1]) &&
            VarCheckInternalIsHexaDecimalDigitCharacter (VariableName[NameLength + 2]) &&
            VarCheckInternalIsHexaDecimalDigitCharacter (VariableName[NameLength + 3])) {
          return &mVarCheckVariableWithWildcardName[Index].VariableProperty;
        }
      }
      if (StrCmp (mVarCheckVariableWithWildcardName[Index].Name, VariableName) == 0) {
        return  &mVarCheckVariableWithWildcardName[Index].VariableProperty;
      }
    }
  }

  return NULL;
}

/**
  Variable property get function.

  @param[in] Name           Pointer to the variable name.
  @param[in] Guid           Pointer to the vendor GUID.
  @param[in] WildcardMatch  Try wildcard match or not.

  @return Pointer to the property of variable specified by the Name and Guid.

**/
VAR_CHECK_VARIABLE_PROPERTY *
VariablePropertyGetFunction (
  IN CHAR16                 *Name,
  IN EFI_GUID               *Guid,
  IN BOOLEAN                WildcardMatch
  )
{
  UINTN                     Index;
  VAR_CHECK_VARIABLE_ENTRY  *Entry;
  CHAR16                    *VariableName;

  for (Index = 0; Index < mNumberOfVarCheckVariable; Index++) {
    Entry = (VAR_CHECK_VARIABLE_ENTRY *) mVarCheckVariableTable[Index];
    VariableName = (CHAR16 *) ((UINTN) Entry + sizeof (*Entry));
    if (CompareGuid (&Entry->Guid, Guid) && (StrCmp (VariableName, Name) == 0)) {
      return &Entry->VariableProperty;
    }
  }

  return VariablePropertyGetWithWildcardName (Name, Guid, WildcardMatch);
}

/**
  Var check add table entry.

  @param[in, out] Table         Pointer to table buffer.
  @param[in, out] MaxNumber     Pointer to maximum number of entry in the table.
  @param[in, out] CurrentNumber Pointer to current number of entry in the table.
  @param[in]      Entry         Entry will be added to the table.

  @retval EFI_SUCCESS           Reallocate memory successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to allocate.

**/
EFI_STATUS
VarCheckAddTableEntry (
  IN OUT UINTN      **Table,
  IN OUT UINTN      *MaxNumber,
  IN OUT UINTN      *CurrentNumber,
  IN UINTN          Entry
  )
{
  UINTN     *TempTable;

  //
  // Check whether the table is enough to store new entry.
  //
  if (*CurrentNumber == *MaxNumber) {
    //
    // Reallocate memory for the table.
    //
    TempTable = ReallocateRuntimePool (
                  *MaxNumber * sizeof (UINTN),
                  (*MaxNumber + VAR_CHECK_TABLE_SIZE) * sizeof (UINTN),
                  *Table
                  );

    //
    // No enough resource to allocate.
    //
    if (TempTable == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *Table = TempTable;
    //
    // Increase max number.
    //
    *MaxNumber += VAR_CHECK_TABLE_SIZE;
  }

  //
  // Add entry to the table.
  //
  (*Table)[*CurrentNumber] = Entry;
  (*CurrentNumber)++;

  return EFI_SUCCESS;
}

/**
  Register END_OF_DXE callback.
  The callback will be invoked by VarCheckLibInitializeAtEndOfDxe().

  @param[in] Callback           END_OF_DXE callback.

  @retval EFI_SUCCESS           The callback was registered successfully.
  @retval EFI_INVALID_PARAMETER Callback is NULL.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the callback register request.

**/
EFI_STATUS
EFIAPI
VarCheckLibRegisterEndOfDxeCallback (
  IN VAR_CHECK_END_OF_DXE_CALLBACK  Callback
  )
{
  EFI_STATUS    Status;

  if (Callback == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mVarCheckLibEndOfDxe) {
    return EFI_ACCESS_DENIED;
  }

  Status = VarCheckAddTableEntry (
           (UINTN **) &mVarCheckLibEndOfDxeCallback,
           &mVarCheckLibEndOfDxeCallbackMaxCount,
           &mVarCheckLibEndOfDxeCallbackCount,
           (UINTN) Callback
           );

  DEBUG ((EFI_D_INFO, "VarCheckLibRegisterEndOfDxeCallback - 0x%x %r\n", Callback, Status));

  return Status;
}

/**
  Var check initialize at END_OF_DXE.

  This function needs to be called at END_OF_DXE.
  Address pointers may be returned,
  and caller needs to ConvertPointer() for the pointers.

  @param[in, out] AddressPointerCount   Output pointer to address pointer count.

  @return Address pointer buffer, NULL if input AddressPointerCount is NULL.

**/
VOID ***
EFIAPI
VarCheckLibInitializeAtEndOfDxe (
  IN OUT UINTN                  *AddressPointerCount OPTIONAL
  )
{
  VOID                          *TempTable;
  UINTN                         TotalCount;
  UINTN                         Index;

  for (Index = 0; Index < mVarCheckLibEndOfDxeCallbackCount; Index++) {
    //
    // Invoke the callback registered by VarCheckLibRegisterEndOfDxeCallback().
    //
    mVarCheckLibEndOfDxeCallback[Index] ();
  }
  if (mVarCheckLibEndOfDxeCallback != NULL) {
    //
    // Free the callback buffer.
    //
    mVarCheckLibEndOfDxeCallbackCount = 0;
    mVarCheckLibEndOfDxeCallbackMaxCount = 0;
    FreePool ((VOID *) mVarCheckLibEndOfDxeCallback);
    mVarCheckLibEndOfDxeCallback = NULL;
  }

  mVarCheckLibEndOfDxe = TRUE;

  if (AddressPointerCount == NULL) {
    if (mVarCheckLibAddressPointer != NULL) {
      //
      // Free the address pointer buffer.
      //
      mVarCheckLibAddressPointerCount = 0;
      mVarCheckLibAddressPointerMaxCount = 0;
      FreePool ((VOID *) mVarCheckLibAddressPointer);
      mVarCheckLibAddressPointer = NULL;
    }
    return NULL;
  }

  //
  // Get the total count needed.
  // Also cover VarCheckHandler and the entries, and VarCheckVariable and the entries.
  //
  TotalCount = mVarCheckLibAddressPointerCount + (mNumberOfVarCheckHandler + 1) + (mNumberOfVarCheckVariable + 1);
  TempTable = ReallocateRuntimePool (
                mVarCheckLibAddressPointerMaxCount * sizeof (VOID **),
                TotalCount * sizeof (VOID **),
                (VOID *) mVarCheckLibAddressPointer
                );

  if (TempTable != NULL) {
    mVarCheckLibAddressPointer = (VOID ***) TempTable;

    //
    // Cover VarCheckHandler and the entries.
    //
    mVarCheckLibAddressPointer[mVarCheckLibAddressPointerCount++] = (VOID **) &mVarCheckHandlerTable;
    for (Index = 0; Index < mNumberOfVarCheckHandler; Index++) {
      mVarCheckLibAddressPointer[mVarCheckLibAddressPointerCount++] = (VOID **) &mVarCheckHandlerTable[Index];
    }

    //
    // Cover VarCheckVariable and the entries.
    //
    mVarCheckLibAddressPointer[mVarCheckLibAddressPointerCount++] = (VOID **) &mVarCheckVariableTable;
    for (Index = 0; Index < mNumberOfVarCheckVariable; Index++) {
      mVarCheckLibAddressPointer[mVarCheckLibAddressPointerCount++] = (VOID **) &mVarCheckVariableTable[Index];
    }

    ASSERT (mVarCheckLibAddressPointerCount == TotalCount);
    mVarCheckLibAddressPointerMaxCount = mVarCheckLibAddressPointerCount;
  }

  *AddressPointerCount = mVarCheckLibAddressPointerCount;
  return mVarCheckLibAddressPointer;
}

/**
  Register address pointer.
  The AddressPointer may be returned by VarCheckLibInitializeAtEndOfDxe().

  @param[in] AddressPointer     Address pointer.

  @retval EFI_SUCCESS           The address pointer was registered successfully.
  @retval EFI_INVALID_PARAMETER AddressPointer is NULL.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the address pointer register request.

**/
EFI_STATUS
EFIAPI
VarCheckLibRegisterAddressPointer (
  IN VOID                       **AddressPointer
  )
{
  EFI_STATUS    Status;

  if (AddressPointer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mVarCheckLibEndOfDxe) {
    return EFI_ACCESS_DENIED;
  }

  Status = VarCheckAddTableEntry(
           (UINTN **) &mVarCheckLibAddressPointer,
           &mVarCheckLibAddressPointerMaxCount,
           &mVarCheckLibAddressPointerCount,
           (UINTN) AddressPointer
           );

  DEBUG ((EFI_D_INFO, "VarCheckLibRegisterAddressPointer - 0x%x %r\n", AddressPointer, Status));

  return Status;
}

/**
  Register SetVariable check handler.

  @param[in] Handler            Pointer to check handler.

  @retval EFI_SUCCESS           The SetVariable check handler was registered successfully.
  @retval EFI_INVALID_PARAMETER Handler is NULL.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the SetVariable check handler register request.
  @retval EFI_UNSUPPORTED       This interface is not implemented.
                                For example, it is unsupported in VarCheck protocol if both VarCheck and SmmVarCheck protocols are present.

**/
EFI_STATUS
EFIAPI
VarCheckLibRegisterSetVariableCheckHandler (
  IN VAR_CHECK_SET_VARIABLE_CHECK_HANDLER   Handler
  )
{
  EFI_STATUS    Status;

  if (Handler == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mVarCheckLibEndOfDxe) {
    return EFI_ACCESS_DENIED;
  }

  Status =  VarCheckAddTableEntry(
             (UINTN **) &mVarCheckHandlerTable,
             &mMaxNumberOfVarCheckHandler,
             &mNumberOfVarCheckHandler,
             (UINTN) Handler
             );

  DEBUG ((EFI_D_INFO, "VarCheckLibRegisterSetVariableCheckHandler - 0x%x %r\n", Handler, Status));

  return Status;
}

/**
  Variable property set.

  @param[in] Name               Pointer to the variable name.
  @param[in] Guid               Pointer to the vendor GUID.
  @param[in] VariableProperty   Pointer to the input variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was set successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string,
                                or the fields of VariableProperty are not valid.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the variable property set request.

**/
EFI_STATUS
EFIAPI
VarCheckLibVariablePropertySet (
  IN CHAR16                         *Name,
  IN EFI_GUID                       *Guid,
  IN VAR_CHECK_VARIABLE_PROPERTY    *VariableProperty
  )
{
  EFI_STATUS                    Status;
  VAR_CHECK_VARIABLE_ENTRY      *Entry;
  CHAR16                        *VariableName;
  VAR_CHECK_VARIABLE_PROPERTY   *Property;

  if (Name == NULL || Name[0] == 0 || Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableProperty == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableProperty->Revision != VAR_CHECK_VARIABLE_PROPERTY_REVISION) {
    return EFI_INVALID_PARAMETER;
  }

  if (mVarCheckLibEndOfDxe) {
    return EFI_ACCESS_DENIED;
  }

  Status = EFI_SUCCESS;

  //
  // Get the pointer of property data for set.
  //
  Property = VariablePropertyGetFunction (Name, Guid, FALSE);
  if (Property != NULL) {
    CopyMem (Property, VariableProperty, sizeof (*VariableProperty));
  } else {
    Entry = AllocateRuntimeZeroPool (sizeof (*Entry) + StrSize (Name));
    if (Entry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    VariableName = (CHAR16 *) ((UINTN) Entry + sizeof (*Entry));
    StrCpyS (VariableName, StrSize (Name)/sizeof (CHAR16), Name);
    CopyGuid (&Entry->Guid, Guid);
    CopyMem (&Entry->VariableProperty, VariableProperty, sizeof (*VariableProperty));

    Status = VarCheckAddTableEntry(
               (UINTN **) &mVarCheckVariableTable,
               &mMaxNumberOfVarCheckVariable,
               &mNumberOfVarCheckVariable,
               (UINTN) Entry
               );

    if (EFI_ERROR (Status)) {
      FreePool (Entry);
    }
  }

  return Status;
}

/**
  Variable property get.

  @param[in]  Name              Pointer to the variable name.
  @param[in]  Guid              Pointer to the vendor GUID.
  @param[out] VariableProperty  Pointer to the output variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was got successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string.
  @retval EFI_NOT_FOUND         The property of variable specified by the Name and Guid was not found.

**/
EFI_STATUS
EFIAPI
VarCheckLibVariablePropertyGet (
  IN CHAR16                         *Name,
  IN EFI_GUID                       *Guid,
  OUT VAR_CHECK_VARIABLE_PROPERTY   *VariableProperty
  )
{
  VAR_CHECK_VARIABLE_PROPERTY   *Property;

  if (Name == NULL || Name[0] == 0 || Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableProperty == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Property = VariablePropertyGetFunction (Name, Guid, TRUE);
  //
  // Also check the property revision before using the property data.
  // There is no property set to this variable(wildcard name)
  // if the revision is not VAR_CHECK_VARIABLE_PROPERTY_REVISION.
  //
  if ((Property != NULL) && (Property->Revision == VAR_CHECK_VARIABLE_PROPERTY_REVISION)) {
    CopyMem (VariableProperty, Property, sizeof (*VariableProperty));
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  SetVariable check.

  @param[in] VariableName       Name of Variable to set.
  @param[in] VendorGuid         Variable vendor GUID.
  @param[in] Attributes         Attribute value of the variable.
  @param[in] DataSize           Size of Data to set.
  @param[in] Data               Data pointer.
  @param[in] RequestSource      Request source.

  @retval EFI_SUCCESS           The SetVariable check result was success.
  @retval EFI_INVALID_PARAMETER An invalid combination of attribute bits, name, GUID,
                                DataSize and Data value was supplied.
  @retval EFI_WRITE_PROTECTED   The variable in question is read-only.
  @retval Others                The other return status from check handler.

**/
EFI_STATUS
EFIAPI
VarCheckLibSetVariableCheck (
  IN CHAR16                     *VariableName,
  IN EFI_GUID                   *VendorGuid,
  IN UINT32                     Attributes,
  IN UINTN                      DataSize,
  IN VOID                       *Data,
  IN VAR_CHECK_REQUEST_SOURCE   RequestSource
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  VAR_CHECK_VARIABLE_PROPERTY   *Property;

  if (!mVarCheckLibEndOfDxe) {
    //
    // Only do check after End Of Dxe.
    //
    return EFI_SUCCESS;
  }

  Property = VariablePropertyGetFunction (VariableName, VendorGuid, TRUE);
  //
  // Also check the property revision before using the property data.
  // There is no property set to this variable(wildcard name)
  // if the revision is not VAR_CHECK_VARIABLE_PROPERTY_REVISION.
  //
  if ((Property != NULL) && (Property->Revision == VAR_CHECK_VARIABLE_PROPERTY_REVISION)) {
    if ((RequestSource != VarCheckFromTrusted) && ((Property->Property & VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY) != 0)) {
      DEBUG ((EFI_D_INFO, "Variable Check ReadOnly variable fail %r - %g:%s\n", EFI_WRITE_PROTECTED, VendorGuid, VariableName));
      return EFI_WRITE_PROTECTED;
    }
    if (!((((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) && (DataSize == 0)) || (Attributes == 0))) {
      //
      // Not to delete variable.
      //
      if ((Property->Attributes != 0) && ((Attributes & (~EFI_VARIABLE_APPEND_WRITE)) != Property->Attributes)) {
        DEBUG ((EFI_D_INFO, "Variable Check Attributes(0x%08x to 0x%08x) fail %r - %g:%s\n", Property->Attributes, Attributes, EFI_INVALID_PARAMETER, VendorGuid, VariableName));
        return EFI_INVALID_PARAMETER;
      }
      if (DataSize != 0) {
        if ((DataSize < Property->MinSize) || (DataSize > Property->MaxSize)) {
          DEBUG ((EFI_D_INFO, "Variable Check DataSize fail(0x%x not in 0x%x - 0x%x) %r - %g:%s\n", DataSize, Property->MinSize, Property->MaxSize, EFI_INVALID_PARAMETER, VendorGuid, VariableName));
          return EFI_INVALID_PARAMETER;
        }
      }
    }
  }

  for (Index = 0; Index < mNumberOfVarCheckHandler; Index++) {
    Status = mVarCheckHandlerTable[Index] (
               VariableName,
               VendorGuid,
               Attributes,
               DataSize,
               Data
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "Variable Check handler fail %r - %g:%s\n", Status, VendorGuid, VariableName));
      return Status;
    }
  }
  return EFI_SUCCESS;
}
