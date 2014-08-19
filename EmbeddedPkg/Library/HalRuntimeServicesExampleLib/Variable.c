/** @file
  Variable services implemented from system memory

  There is just a single runtime memory buffer that contans all the data.

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/


UINT64    mMaximumVariableStorageSize;
UINT64    mRemainingVariableStorageSize;
UINT64    mMaximumVariableSize;

typedef struct {
  EFI_GUID        VendorGuid;
  UINT32          Attribute;
  UINTN           DataSize;
} VARIABLE_ARRAY_ENTRY;
// CHAR16         VariableName[]
// UINT8          Data[]

VARIABLE_ARRAY_ENTRY  *mVariableArray         = NULL;
VARIABLE_ARRAY_ENTRY  *mVariableArrayNextFree = NULL;
VARIABLE_ARRAY_ENTRY  *mVariableArrayEnd      = NULL;


VARIABLE_ARRAY_ENTRY  *
AddEntry (
  IN CHAR16        *VariableName,
  IN EFI_GUID      *VendorGuid,
  IN UINT32        Attributes,
  IN UINTN         DataSize,
  IN VOID          *Data
  )
{
  UINTN                   Size;
  UINTN                   SizeOfString;
  VARIABLE_ARRAY_ENTRY    *Entry;
  EFI_TPL                 CurrentTpl;


  SizeOfString = StrSize (VariableName);
  Size = SizeOfString + sizeof (VARIABLE_ARRAY_ENTRY) + DataSize;
  if ((VARIABLE_ARRAY_ENTRY *)(((UINT8 *)mVariableArrayNextFree) + Size) > mVariableArrayEnd) {
    // ran out of space
    return NULL;
  }

  if (!EfiAtRuntime ()) {
    // Enter critical section
    CurrentTpl = gBS->RaiseTPL (EFI_TPL_HIGH_LEVEL);
  }

  Entry = mVariableArrayNextFree;
  CopyGuid (&Entry->VendorGuid, VendorGuid);
  Entry->Attribute = Attributes;
  Entry->DataSize = DataSize;
  StrCpy ((CHAR16 *)++mVariableArrayNextFree, VariableName);
  mVariableArrayNextFree = (VARIABLE_ARRAY_ENTRY *)(((UINT8 *)mVariableArrayNextFree) + SizeOfString);
  CopyMem (mVariableArrayNextFree, Data, DataSize);
  mVariableArrayNextFree = (VARIABLE_ARRAY_ENTRY *)(((UINT8 *)mVariableArrayNextFree) + DataSize);

  if (!EfiAtRuntime ()) {
    // Exit Critical section
    gBS->RestoreTPL (CurrentTpl);
  }

  return Entry;
}

VOID
DeleteEntry (
  IN  VARIABLE_ARRAY_ENTRY *Entry
  )
{
  UINTN       Size;
  UINT8       *Data;
  EFI_TPL     CurrentTpl;

  Size = StrSize ((CHAR16 *)(Entry + 1)) + sizeof (VARIABLE_ARRAY_ENTRY) + Entry->DataSize;
  Data = ((UINT8 *)Entry) + Size;

  CopyMem (Entry, Data, (UINTN)mVariableArrayNextFree - (UINTN)Data);

  if (!EfiAtRuntime ()) {
    // Enter critical section
    CurrentTpl = gBS->RaiseTPL (EFI_TPL_HIGH_LEVEL);
  }

  mVariableArrayNextFree = (VARIABLE_ARRAY_ENTRY *)(((UINT8 *)mVariableArrayNextFree) - Size);

  if (!EfiAtRuntime ()) {
    // Exit Critical section
    gBS->RestoreTPL (CurrentTpl);
  }
}


VARIABLE_ARRAY_ENTRY *
GetVariableArrayEntry (
  IN CHAR16        *VariableName,
  IN EFI_GUID      *VendorGuid,
  OUT VOID         **Data          OPTIONAL
  )
{
  VARIABLE_ARRAY_ENTRY    *Entry;
  UINTN                   Size;

  if (*VariableName == L'\0') {
    // by definition first entry is null-terminated string
    if (mVariableArray == mVariableArrayNextFree) {
      return NULL;
    }
    return mVariableArray;
  }

  for (Entry = mVariableArray; Entry < mVariableArrayEnd;) {
    if (CompareGuid (VendorGuid, &Entry->VendorGuid)) {
      if (StrCmp (VariableName, (CHAR16 *)(Entry + 1))) {
        Size = StrSize ((CHAR16 *)(Entry + 1));
        if (Data != NULL) {
          *Data = (VOID *)(((UINT8 *)Entry) + (Size + sizeof (VARIABLE_ARRAY_ENTRY)));
        }
        return Entry;
      }
    }

    Size = StrSize ((CHAR16 *)(Entry + 1)) + sizeof (VARIABLE_ARRAY_ENTRY) + Entry->DataSize;
    Entry = (VARIABLE_ARRAY_ENTRY *)(((UINT8 *)Entry) + Size);
  }

  return NULL;
}


EFI_STATUS
LibGetVariable (
  IN CHAR16        *VariableName,
  IN EFI_GUID      *VendorGuid,
  OUT UINT32       *Attributes OPTIONAL,
  IN OUT UINTN     *DataSize,
  OUT VOID         *Data
  )
{
  VARIABLE_ARRAY_ENTRY    *Entry;
  VOID                    *InternalData;

  if (EfiAtRuntime () && (Attributes != NULL)) {
    if ((*Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0) {
      return EFI_NOT_FOUND;
    }
  }

  Entry = GetVariableArrayEntry (VariableName, VendorGuid, &InternalData);
  if (Entry == NULL) {
    return EFI_NOT_FOUND;
  }

  if (*DataSize < Entry->DataSize) {
    *DataSize = Entry->DataSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  *DataSize = Entry->DataSize;
  if (Attributes != NULL) {
    *Attributes = Entry->Attribute;
  }

  CopyMem (Data, InternalData, *DataSize);
  return EFI_SUCCESS;
}


EFI_STATUS
LibGetNextVariableName (
  IN OUT UINTN     *VariableNameSize,
  IN OUT CHAR16    *VariableName,
  IN OUT EFI_GUID  *VendorGuid
  )
{
  VARIABLE_ARRAY_ENTRY    *Entry;
  VOID                    *InternalData;
  UINTN                   StringSize;
  BOOLEAN                 Done;

  for (Done = FALSE; !Done; ) {
    Entry = GetVariableArrayEntry (VariableName, VendorGuid, &InternalData);
    if (Entry == NULL) {
      return EFI_NOT_FOUND;
    }

    // If we are at runtime skip variables that do not have the Runitme attribute set.
    Done = (EfiAtRuntime () && ((Entry->Attribute & EFI_VARIABLE_RUNTIME_ACCESS) == 0)) ? FALSE : TRUE;
  }

  StringSize = StrSize ((CHAR16 *)(Entry + 1));
  Entry = (VARIABLE_ARRAY_ENTRY *)(((UINT8 *)Entry) + (StringSize + sizeof (VARIABLE_ARRAY_ENTRY) + Entry->DataSize));
  if (Entry >= mVariableArrayEnd) {
    return EFI_NOT_FOUND;
  }

  if (*VariableNameSize < StringSize) {
    *VariableNameSize = StringSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  *VariableNameSize = StringSize;
  CopyMem (VariableName, (CHAR16 *)(Entry + 1), StringSize);
  CopyMem (VendorGuid, &Entry->VendorGuid, sizeof (EFI_GUID));
  return EFI_SUCCESS;
}



EFI_STATUS
LibSetVariable (
  IN CHAR16        *VariableName,
  IN EFI_GUID      *VendorGuid,
  IN UINT32        Attributes,
  IN UINTN         DataSize,
  IN VOID          *Data
  )
{
  VARIABLE_ARRAY_ENTRY    *Entry;
  VOID                    *InternalData;

  if (EfiAtRuntime () && ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0)) {
    return EFI_NOT_FOUND;
  }

  Entry = GetVariableArrayEntry (VariableName, VendorGuid, &InternalData);
  if (Entry == NULL) {
    if (DataSize == 0) {
      return EFI_NOT_FOUND;
    }
    Entry = AddEntry (VariableName, VendorGuid, Attributes, DataSize, Data);
    return (Entry == NULL) ? EFI_OUT_OF_RESOURCES : EFI_SUCCESS;

  } else if (DataSize == 0) {
    // DataSize is zero so delete
    DeleteEntry (Entry);
  } else if (DataSize == Entry->DataSize) {
    // No change is size so just update the store
    Entry->Attribute |= Attributes;
    CopyMem (InternalData, Data, DataSize);
  } else {
    // Grow the entry by deleting and adding back. Don't lose previous Attributes
    Attributes |= Entry->Attribute;
    DeleteEntry (Entry);
    Entry = AddEntry (VariableName, VendorGuid, Attributes, DataSize, Data);
    return (Entry == NULL) ? EFI_OUT_OF_RESOURCES : EFI_SUCCESS;
  }
}


EFI_STATUS
LibQueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize
  )
{
  *MaximumVariableStorageSize   = mMaximumVariableStorageSize;
  *RemainingVariableStorageSize = mRemainingVariableStorageSize;
  *MaximumVariableStorageSize   = mRemainingVariableStorageSize;
  return EFI_SUCCESS;
}


VOID
LibVariableVirtualAddressChangeEvent (VOID)
{
  EfiConvertPointer (0, (VOID **)&mVariableArray);
  EfiConvertPointer (0, (VOID **)&mVariableArrayNextFree);
  EfiConvertPointer (0, (VOID **)&mVariableArrayEnd);
}


VOID
LibVariableInitialize (VOID)
{
  UINTN     Size;

  Size = PcdGet32 (PcdEmbeddedMemVariableStoreSize);
  mVariableArray = mVariableArrayNextFree = (VARIABLE_ARRAY_ENTRY *)AllocateRuntimePool (Size);
  ASSERT (mVariableArray != NULL);

  mVariableArrayEnd = (VARIABLE_ARRAY_ENTRY *)(((UINT8 *)mVariableArray) + Size);

  mMaximumVariableStorageSize   = Size - sizeof (VARIABLE_ARRAY_ENTRY);
  mRemainingVariableStorageSize = mMaximumVariableStorageSize;
  mMaximumVariableSize          = mMaximumVariableStorageSize;
}

