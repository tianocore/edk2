/** @file
Private functions used by PCD DXE driver.

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name: Service.c

**/

#include "Service.h"


PCD_DATABASE * mPcdDatabase;

LIST_ENTRY *mCallbackFnTable;

VOID *
GetWorker (
  UINTN             TokenNumber,
  UINTN             GetSize
  )
{
  UINT32              *LocalTokenNumberTable;
  EFI_GUID            *GuidTable;
  UINT16              *StringTable;
  EFI_GUID            *Guid;
  UINT16              *Name;
  VARIABLE_HEAD       *VariableHead;
  UINT8               *VaraiableDefaultBuffer;
  UINT8               *Data;
  VPD_HEAD            *VpdHead;
  UINT8               *PcdDb;
  VOID                *RetPtr;
  UINTN               MaxSize;
  UINTN               TmpTokenNumber;
  UINTN               DataSize;
  EFI_STATUS          Status;
  UINT32              LocalTokenNumber;
  UINT32              Offset;
  UINT16              StringTableIdx;      
  BOOLEAN             IsPeiDb;

  //
  // Aquire lock to prevent reentrance from TPL_CALLBACK level
  //
  EfiAcquireLock (&mPcdDatabaseLock);

  RetPtr = NULL;
  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  TmpTokenNumber = TokenNumber;
  
  //
  // PCD_TOTAL_TOKEN_NUMBER is a auto-generated constant.
  // It could be zero. EBC compiler is very choosy. It may
  // report warning. So we add 1 in each size of the 
  // comparison.
  //
  ASSERT (TokenNumber + 1 < PCD_TOTAL_TOKEN_NUMBER + 1);

  ASSERT ((GetSize == DxePcdGetSize (TokenNumber + 1)) || (GetSize == 0));

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  IsPeiDb = (BOOLEAN) ((TokenNumber + 1 < PEI_LOCAL_TOKEN_NUMBER + 1) ? TRUE : FALSE);

  LocalTokenNumberTable  = IsPeiDb ? mPcdDatabase->PeiDb.Init.LocalTokenNumberTable : 
                                     mPcdDatabase->DxeDb.Init.LocalTokenNumberTable;

  TokenNumber            = IsPeiDb ? TokenNumber :
                                     TokenNumber - PEI_LOCAL_TOKEN_NUMBER;

  LocalTokenNumber = LocalTokenNumberTable[TokenNumber];
  
  if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == PCD_TYPE_SKU_ENABLED) {
    if (GetSize == 0) {
      GetPtrTypeSize (TmpTokenNumber, &MaxSize);
    } else {
      MaxSize = GetSize;
    }
    LocalTokenNumber = GetSkuEnabledTokenNumber (LocalTokenNumber & ~PCD_TYPE_SKU_ENABLED, MaxSize, IsPeiDb);
  }

  PcdDb = IsPeiDb ? ((UINT8 *) &mPcdDatabase->PeiDb) : ((UINT8 *) &mPcdDatabase->DxeDb);
  StringTable = IsPeiDb ? mPcdDatabase->PeiDb.Init.StringTable :
                          mPcdDatabase->DxeDb.Init.StringTable;
  
  Offset     = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;
  
  switch (LocalTokenNumber & PCD_TYPE_ALL_SET) {
    case PCD_TYPE_VPD:
      VpdHead = (VPD_HEAD *) ((UINT8 *) PcdDb + Offset);
      RetPtr = (VOID *) (UINTN) (FixedPcdGet32(PcdVpdBaseAddress) + VpdHead->Offset);
      break;
      
    case PCD_TYPE_HII:
      GuidTable   = IsPeiDb ? mPcdDatabase->PeiDb.Init.GuidTable :
                              mPcdDatabase->DxeDb.Init.GuidTable;
                              
      VariableHead = (VARIABLE_HEAD *) (PcdDb + Offset);
      
      Guid = &(GuidTable[VariableHead->GuidTableIndex]);
      Name = &(StringTable[VariableHead->StringIndex]);
      VaraiableDefaultBuffer = (UINT8 *) PcdDb + VariableHead->DefaultValueOffset;

      Status = GetHiiVariable (Guid, Name, &Data, &DataSize);
      if (Status == EFI_SUCCESS) {
        if (GetSize == 0) {
          //
          // It is a pointer type. So get the MaxSize reserved for
          // this PCD entry.
          //
          GetPtrTypeSize (TmpTokenNumber, &GetSize);
        }
        CopyMem (VaraiableDefaultBuffer, Data + VariableHead->Offset, GetSize);
        FreePool (Data);
      }
      //
      // If the operation is successful, we copy the data
      // to the default value buffer in the PCD Database.
      // So that we can free the Data allocated in GetHiiVariable.
      //
      //
      // If the operation is not successful, 
      // Return 1) either the default value specified by Platform Integrator 
      //        2) Or the value Set by a PCD set operation.
      //
      RetPtr = (VOID *) VaraiableDefaultBuffer;
      break;

    case PCD_TYPE_STRING:
      StringTableIdx = (UINT16) *((UINT8 *) PcdDb + Offset);
      RetPtr = (VOID *) &StringTable[StringTableIdx];
      break;

    case PCD_TYPE_DATA:
      RetPtr = (VOID *) ((UINT8 *) PcdDb + Offset);
      break;

    default:
      ASSERT (FALSE);
      break;
      
  }

  EfiReleaseLock (&mPcdDatabaseLock);
  
  return RetPtr;
  
}



EFI_STATUS
DxeRegisterCallBackWorker (
  IN  UINTN                   TokenNumber,
  IN  CONST GUID              *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
)
{
  CALLBACK_FN_ENTRY       *FnTableEntry;
  LIST_ENTRY              *ListHead;
  LIST_ENTRY              *ListNode;

  if (Guid != NULL) {
    TokenNumber = GetExPcdTokenNumber (Guid, (UINT32) TokenNumber);
  }

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  ListHead = &mCallbackFnTable[TokenNumber];
  ListNode = GetFirstNode (ListHead);

  while (ListNode != ListHead) {
    FnTableEntry = CR_FNENTRY_FROM_LISTNODE(ListNode, CALLBACK_FN_ENTRY, Node);

    if (FnTableEntry->CallbackFn == CallBackFunction) {
      //
      // We only allow a Callback function to be register once
      // for a TokenNumber. So just return EFI_SUCCESS
      //
      return EFI_SUCCESS;
    }
    ListNode = GetNextNode (ListHead, ListNode);
  }

  FnTableEntry = AllocatePool (sizeof(CALLBACK_FN_ENTRY));
  ASSERT (FnTableEntry != NULL);

  FnTableEntry->CallbackFn = CallBackFunction;
  InsertTailList (ListHead, &FnTableEntry->Node);
  
  return EFI_SUCCESS;
}




EFI_STATUS
DxeUnRegisterCallBackWorker (
  IN  UINTN                   TokenNumber,
  IN  CONST GUID              *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
)
{
  CALLBACK_FN_ENTRY       *FnTableEntry;
  LIST_ENTRY              *ListHead;
  LIST_ENTRY              *ListNode;

  if (Guid != NULL) {
    TokenNumber = GetExPcdTokenNumber (Guid, (UINT32) TokenNumber);
  }

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  ListHead = &mCallbackFnTable[TokenNumber];
  ListNode = GetFirstNode (ListHead);

  while (ListNode != ListHead) {
    FnTableEntry = CR_FNENTRY_FROM_LISTNODE(ListNode, CALLBACK_FN_ENTRY, Node);

    if (FnTableEntry->CallbackFn == CallBackFunction) {
      //
      // We only allow a Callback function to be register once
      // for a TokenNumber. So we can safely remove the Node from
      // the Link List and return EFI_SUCCESS.
      //
      RemoveEntryList (ListNode);
      FreePool (FnTableEntry);
      
      return EFI_SUCCESS;
    }
    ListNode = GetNextNode (ListHead, ListNode);
  }

  return EFI_INVALID_PARAMETER;
}



EFI_STATUS
ExGetNextTokeNumber (
  IN      CONST EFI_GUID         *Guid,
  IN OUT  UINTN                  *TokenNumber,
  IN      EFI_GUID               *GuidTable,
  IN      UINTN                  SizeOfGuidTable,
  IN      DYNAMICEX_MAPPING      *ExMapTable,
  IN      UINTN                  SizeOfExMapTable
  )
{
  EFI_GUID         *MatchGuid;
  UINTN            Idx;
  UINTN            GuidTableIdx;
  BOOLEAN          Found;

  MatchGuid = ScanGuid (GuidTable, SizeOfGuidTable, Guid);
  if (MatchGuid == NULL) {
    return EFI_NOT_FOUND;
  }

  Found = FALSE;
  GuidTableIdx = MatchGuid - GuidTable;
  for (Idx = 0; Idx < SizeOfExMapTable; Idx++) {
    if (ExMapTable[Idx].ExGuidIndex == GuidTableIdx) {
      Found = TRUE;
      break;
    }
  }

  if (Found) {
    if (*TokenNumber == PCD_INVALID_TOKEN_NUMBER) {
      *TokenNumber = ExMapTable[Idx].ExTokenNumber;
      return EFI_SUCCESS;
    }

    for ( ; Idx < SizeOfExMapTable; Idx++) {
      if (ExMapTable[Idx].ExTokenNumber == *TokenNumber) {
        Idx++;
        if (Idx == SizeOfExMapTable) {
          //
          // Exceed the length of ExMap Table
          //
          *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
          return EFI_SUCCESS;
        } else if (ExMapTable[Idx].ExGuidIndex == GuidTableIdx) {
          //
          // Found the next match
          //
          *TokenNumber = ExMapTable[Idx].ExTokenNumber;
          return EFI_SUCCESS;
        } else {
          //
          // Guid has been changed. It is the next Token Space Guid.
          // We should flag no more TokenNumber.
          //
          *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
          return EFI_SUCCESS;
        }
      }
    }
  }
  
  return EFI_NOT_FOUND;
}
  



VOID
BuildPcdDxeDataBase (
  VOID
  )
{
  PEI_PCD_DATABASE    *PeiDatabase;
  EFI_HOB_GUID_TYPE   *GuidHob;
  UINTN               Idx;

  mPcdDatabase = AllocateZeroPool (sizeof(PCD_DATABASE));
  ASSERT (mPcdDatabase != NULL);

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  if (GuidHob != NULL) {

    //
    // We will copy over the PEI phase's PCD Database.
    // 
    // If no PEIMs use dynamic Pcd Entry, the Pcd Service PEIM
    // should not be included at all. So the GuidHob could
    // be NULL. If it is NULL, we just copy over the DXE Default
    // Value to PCD Database.
    //
    
    PeiDatabase = (PEI_PCD_DATABASE *) GET_GUID_HOB_DATA (GuidHob);
    //
    // Copy PCD Entries refereneced in PEI phase to PCD DATABASE
    //
    CopyMem (&mPcdDatabase->PeiDb, PeiDatabase, sizeof (PEI_PCD_DATABASE));
  }

  //
  // Copy PCD Entries with default value to PCD DATABASE
  //
  CopyMem (&mPcdDatabase->DxeDb.Init, &gDXEPcdDbInit, sizeof(DXE_PCD_DATABASE_INIT));


  //
  // Initialized the Callback Function Table
  //

  mCallbackFnTable = AllocateZeroPool (PCD_TOTAL_TOKEN_NUMBER * sizeof (LIST_ENTRY));
  
  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  for (Idx = 0; Idx + 1 < PCD_TOTAL_TOKEN_NUMBER + 1; Idx++) {
    InitializeListHead (&mCallbackFnTable[Idx]);
  }
    
  return;
}



EFI_STATUS
GetHiiVariable (
  IN  EFI_GUID      *VariableGuid,
  IN  UINT16        *VariableName,
  OUT UINT8         **VariableData,
  OUT UINTN         *VariableSize
  )
{
  UINTN      Size;
  EFI_STATUS Status;
  UINT8      *Buffer;

  Size = 0;
  Buffer = NULL;
  
  Status = gRT->GetVariable (
    (UINT16 *)VariableName,
    VariableGuid,
    NULL,
    &Size,
    Buffer
    );
  
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = (UINT8 *) AllocatePool (Size);

    ASSERT (Buffer != NULL);

    Status = gRT->GetVariable (
      VariableName,
      VariableGuid,
      NULL,
      &Size,
      Buffer
      );

    ASSERT (Status == EFI_SUCCESS);
    *VariableData = Buffer;
    *VariableSize = Size;
  }

  return Status;
}


UINT32
GetSkuEnabledTokenNumber (
  UINT32    LocalTokenNumber,
  UINTN     Size,
  BOOLEAN   IsPeiDb
  ) 
{
  SKU_HEAD              *SkuHead;
  SKU_ID                *SkuIdTable;
  INTN                  i;
  UINT8                 *Value;
  SKU_ID                *PhaseSkuIdTable;
  UINT8                 *PcdDb;

  ASSERT ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == 0);

  PcdDb = IsPeiDb ? (UINT8 *) &mPcdDatabase->PeiDb : (UINT8 *) &mPcdDatabase->DxeDb;

  SkuHead     = (SKU_HEAD *) (PcdDb + (LocalTokenNumber & PCD_DATABASE_OFFSET_MASK));
  Value       = (UINT8 *) (PcdDb + SkuHead->SkuDataStartOffset); 

  PhaseSkuIdTable = IsPeiDb ? mPcdDatabase->PeiDb.Init.SkuIdTable :
                              mPcdDatabase->DxeDb.Init.SkuIdTable;
                              
  SkuIdTable  = &PhaseSkuIdTable[SkuHead->SkuIdTableOffset];
        
  for (i = 0; i < SkuIdTable[0]; i++) {
    if (mPcdDatabase->PeiDb.Init.SystemSkuId == SkuIdTable[i + 1]) {
      break;
    }
  }
  ASSERT (i < SkuIdTable[0]);

  switch (LocalTokenNumber & PCD_TYPE_ALL_SET) {
    case PCD_TYPE_VPD:
      Value = (UINT8 *) &(((VPD_HEAD *) Value)[i]);
      return (UINT32) ((Value - PcdDb) | PCD_TYPE_VPD);

    case PCD_TYPE_HII:
      Value = (UINT8 *) &(((VARIABLE_HEAD *) Value)[i]);
      return (UINT32) ((Value - PcdDb) | PCD_TYPE_HII);

    case PCD_TYPE_STRING:
      Value = (UINT8 *) &(((STRING_HEAD *) Value)[i]);
      return (UINT32) ((Value - PcdDb) | PCD_TYPE_STRING);
      
    case PCD_TYPE_DATA:
      Value += Size * i;
      return (UINT32) (Value - PcdDb);

    default:
      ASSERT (FALSE);
  }

  ASSERT (FALSE);

  return 0;
  
}




STATIC
VOID
InvokeCallbackOnSet (
  UINT32            ExTokenNumber,
  CONST EFI_GUID    *Guid, OPTIONAL
  UINTN             TokenNumber,
  VOID              *Data,
  UINTN             Size
  )
{
  CALLBACK_FN_ENTRY       *FnTableEntry;
  LIST_ENTRY              *ListHead;
  LIST_ENTRY              *ListNode;

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;
  
  ListHead = &mCallbackFnTable[TokenNumber];
  ListNode = GetFirstNode (ListHead);

  while (ListNode != ListHead) {
    FnTableEntry = CR_FNENTRY_FROM_LISTNODE(ListNode, CALLBACK_FN_ENTRY, Node);

    FnTableEntry->CallbackFn(Guid, 
                    (Guid == NULL) ? TokenNumber : ExTokenNumber,
                    Data,
                    Size);
    
    ListNode = GetNextNode (ListHead, ListNode);
  }
  
  return;
}


EFI_STATUS
SetValueWorker (
  IN UINTN                   TokenNumber,
  IN VOID                    *Data,
  IN UINTN                   Size
  )
{
  return SetWorker (TokenNumber, Data, &Size, FALSE);
}


EFI_STATUS
SetWorker (
  IN          UINTN                   TokenNumber,
  IN          VOID                    *Data,
  IN OUT      UINTN                   *Size,
  IN          BOOLEAN                 PtrType
  )
{
  UINT32              *LocalTokenNumberTable;
  BOOLEAN             IsPeiDb;
  UINT32              LocalTokenNumber;
  EFI_GUID            *GuidTable;
  UINT16              *StringTable;
  EFI_GUID            *Guid;
  UINT16              *Name;
  UINTN               VariableOffset;
  VOID                *InternalData;
  VARIABLE_HEAD       *VariableHead;
  UINTN               Offset;
  UINT8               *PcdDb;
  EFI_STATUS          Status;
  UINTN               MaxSize;
  UINTN               TmpTokenNumber;

  //
  // Aquire lock to prevent reentrance from TPL_CALLBACK level
  //
  EfiAcquireLock (&mPcdDatabaseLock);

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  TmpTokenNumber = TokenNumber;
  
  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.

  ASSERT (TokenNumber + 1 < PCD_TOTAL_TOKEN_NUMBER + 1);

  if (!PtrType) {
    ASSERT (*Size == DxePcdGetSize (TokenNumber + 1));
  }
  
  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  IsPeiDb = (BOOLEAN) ((TokenNumber + 1 < PEI_LOCAL_TOKEN_NUMBER + 1) ? TRUE : FALSE);

  LocalTokenNumberTable  = IsPeiDb ? mPcdDatabase->PeiDb.Init.LocalTokenNumberTable : 
                                     mPcdDatabase->DxeDb.Init.LocalTokenNumberTable;

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  if ((TokenNumber + 1 < PEI_NEX_TOKEN_NUMBER + 1) ||
      (TokenNumber + 1 >= PEI_LOCAL_TOKEN_NUMBER + 1 || TokenNumber + 1 < (PEI_LOCAL_TOKEN_NUMBER + DXE_NEX_TOKEN_NUMBER + 1))) {
    InvokeCallbackOnSet (0, NULL, TokenNumber + 1, Data, *Size);
  }

  TokenNumber = IsPeiDb ? TokenNumber
                        : TokenNumber - PEI_LOCAL_TOKEN_NUMBER;

  LocalTokenNumber = LocalTokenNumberTable[TokenNumber];
  
  if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == PCD_TYPE_SKU_ENABLED) {
    if (PtrType) {
      GetPtrTypeSize (TmpTokenNumber, &MaxSize);
    } else {
      MaxSize = *Size;
    }
    LocalTokenNumber = GetSkuEnabledTokenNumber (LocalTokenNumber & ~PCD_TYPE_SKU_ENABLED, MaxSize, IsPeiDb);
  }

  Offset = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;

  PcdDb = IsPeiDb ? ((UINT8 *) &mPcdDatabase->PeiDb) : ((UINT8 *) &mPcdDatabase->DxeDb);

  StringTable = IsPeiDb ? mPcdDatabase->PeiDb.Init.StringTable :
                          mPcdDatabase->DxeDb.Init.StringTable;
  
  InternalData = PcdDb + Offset;

  switch (LocalTokenNumber & PCD_TYPE_ALL_SET) {
    case PCD_TYPE_VPD:
      ASSERT (FALSE);
      Status = EFI_INVALID_PARAMETER;
      break;
    
    case PCD_TYPE_STRING:
      if (SetPtrTypeSize (TmpTokenNumber, Size)) {
        CopyMem (&StringTable[*((UINT16 *)InternalData)], Data, *Size);
        Status = EFI_SUCCESS;
      } else {
        Status = EFI_INVALID_PARAMETER;
      }
      break;

    case PCD_TYPE_HII:
      if (PtrType) {
        if (!SetPtrTypeSize (TmpTokenNumber, Size)) {
          Status = EFI_INVALID_PARAMETER;
          break;
        }
      }
      
      GuidTable   = IsPeiDb ? mPcdDatabase->PeiDb.Init.GuidTable :
                              mPcdDatabase->DxeDb.Init.GuidTable;
                              
      VariableHead = (VARIABLE_HEAD *) (PcdDb + Offset);
      
      Guid = &(GuidTable[VariableHead->GuidTableIndex]);
      Name = &(StringTable[VariableHead->StringIndex]);
      VariableOffset = VariableHead->Offset;

      Status = SetHiiVariable (Guid, Name, Data, *Size, VariableOffset);

      if (EFI_NOT_FOUND == Status) {
        CopyMem (PcdDb + VariableHead->DefaultValueOffset, Data, *Size);
        Status = EFI_SUCCESS;
      } 
      break;
      
    case PCD_TYPE_DATA:
      if (PtrType) {
        if (SetPtrTypeSize (TmpTokenNumber, Size)) {
          CopyMem (InternalData, Data, *Size);
          Status = EFI_SUCCESS;
        } else {
          Status = EFI_INVALID_PARAMETER;
        }
        break;
      }

      Status = EFI_SUCCESS;
      switch (*Size) {
        case sizeof(UINT8):
          *((UINT8 *) InternalData) = *((UINT8 *) Data);
          break;

        case sizeof(UINT16):
          *((UINT16 *) InternalData) = *((UINT16 *) Data);
          break;

        case sizeof(UINT32):
          *((UINT32 *) InternalData) = *((UINT32 *) Data);
          break;

        case sizeof(UINT64):
          *((UINT64 *) InternalData) = *((UINT64 *) Data);
          break;

        default:
          ASSERT (FALSE);
          Status = EFI_NOT_FOUND;
          break;
      }
      break;

    default:
      ASSERT (FALSE);
      Status = EFI_NOT_FOUND;
      break;
    }

  EfiReleaseLock (&mPcdDatabaseLock);
  
  return Status;
}





VOID *
ExGetWorker (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber,
  IN UINTN                  GetSize
  ) 
{
  return GetWorker(GetExPcdTokenNumber (Guid, (UINT32) ExTokenNumber), GetSize);
}




EFI_STATUS
ExSetValueWorker (
  IN          UINTN                ExTokenNumber,
  IN          CONST EFI_GUID       *Guid,
  IN          VOID                 *Data,
  IN          UINTN                SetSize
  )
{
  return ExSetWorker (ExTokenNumber, Guid, Data, &SetSize, FALSE);
}


EFI_STATUS
ExSetWorker (
  IN          UINTN                ExTokenNumber,
  IN          CONST EFI_GUID       *Guid,
  IN          VOID                 *Data,
  IN OUT      UINTN                *SetSize,
  IN          BOOLEAN              PtrType
  )
{
  UINTN                   TokenNumber;
  
  TokenNumber = GetExPcdTokenNumber (Guid, (UINT32) ExTokenNumber);

  InvokeCallbackOnSet ((UINT32) ExTokenNumber, Guid, TokenNumber, Data, *SetSize);

  return SetWorker (TokenNumber, Data, SetSize, PtrType);

}




EFI_STATUS
SetHiiVariable (
  IN  EFI_GUID     *VariableGuid,
  IN  UINT16       *VariableName,
  IN  CONST VOID   *Data,
  IN  UINTN        DataSize,
  IN  UINTN        Offset
  )
{
  UINTN       Size;
  VOID        *Buffer;
  EFI_STATUS  Status;
  UINT32      Attribute;

  Size = 0;

  Status = gRT->GetVariable (
    (UINT16 *)VariableName,
    VariableGuid,
    NULL,
    &Size,
    NULL
    );

  if (Status == EFI_BUFFER_TOO_SMALL) {

    Buffer = AllocatePool (Size);

    ASSERT (Buffer != NULL);

    Status = gRT->GetVariable (
      VariableName,
      VariableGuid,
      &Attribute,
      &Size,
      Buffer
      );
    
    ASSERT_EFI_ERROR (Status);

    CopyMem ((UINT8 *)Buffer + Offset, Data, DataSize);

    Status = gRT->SetVariable (
              VariableName,
              VariableGuid,
              Attribute,
              Size,
              Buffer
              );

    FreePool (Buffer);
    return Status;

  } 
  
  //
  // If we drop to here, we don't have a Variable entry in
  // the variable service yet. So, we will save the data
  // in the PCD Database's volatile area.
  //
  return Status;
}





UINTN           
GetExPcdTokenNumber (
  IN CONST EFI_GUID             *Guid,
  IN UINT32                     ExTokenNumber
  )
{
  UINT32              i;
  DYNAMICEX_MAPPING   *ExMap;
  EFI_GUID            *GuidTable;
  EFI_GUID            *MatchGuid;
  UINTN               MatchGuidIdx;

  if (!PEI_DATABASE_EMPTY) {
    ExMap       = mPcdDatabase->PeiDb.Init.ExMapTable;
    GuidTable   = mPcdDatabase->PeiDb.Init.GuidTable;
    
    MatchGuid   = ScanGuid (GuidTable, sizeof(mPcdDatabase->PeiDb.Init.GuidTable), Guid);
    
    if (MatchGuid != NULL) {

      MatchGuidIdx = MatchGuid - GuidTable;
      
      for (i = 0; i < PEI_EXMAPPING_TABLE_SIZE; i++) {
        if ((ExTokenNumber == ExMap[i].ExTokenNumber) &&
            (MatchGuidIdx == ExMap[i].ExGuidIndex)) {
            return ExMap[i].LocalTokenNumber;

        }
      }
    }
  }
  
  ExMap       = mPcdDatabase->DxeDb.Init.ExMapTable;
  GuidTable   = mPcdDatabase->DxeDb.Init.GuidTable;

  MatchGuid   = ScanGuid (GuidTable, sizeof(mPcdDatabase->DxeDb.Init.GuidTable), Guid);
  //
  // We need to ASSERT here. If GUID can't be found in GuidTable, this is a
  // error in the BUILD system.
  //
  ASSERT (MatchGuid != NULL);

  MatchGuidIdx = MatchGuid - GuidTable;
  
  for (i = 0; i < DXE_EXMAPPING_TABLE_SIZE; i++) {
    if ((ExTokenNumber == ExMap[i].ExTokenNumber) &&
         (MatchGuidIdx == ExMap[i].ExGuidIndex)) {
        return ExMap[i].LocalTokenNumber;
    }
  }

  ASSERT (FALSE);

  return 0;
}


STATIC
SKU_ID *
GetSkuIdArray (
  IN    UINTN             LocalTokenNumberTableIdx,
  IN    BOOLEAN           IsPeiPcd
  )
{
  SKU_HEAD  *SkuHead;
  UINTN     LocalTokenNumber;
  UINT8     *Database;

  if (IsPeiPcd) {
    LocalTokenNumber = mPcdDatabase->PeiDb.Init.LocalTokenNumberTable[LocalTokenNumberTableIdx];
    Database         = (UINT8 *) &mPcdDatabase->PeiDb;
  } else {
    LocalTokenNumber = mPcdDatabase->DxeDb.Init.LocalTokenNumberTable[LocalTokenNumberTableIdx - PEI_LOCAL_TOKEN_NUMBER];
    Database         = (UINT8 *) &mPcdDatabase->DxeDb;
  }

  ASSERT ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) != 0);

  SkuHead = (SKU_HEAD *) ((UINT8 *)Database + (LocalTokenNumber & PCD_DATABASE_OFFSET_MASK));

  return (SKU_ID *) (Database + SkuHead->SkuIdTableOffset);
  
}


STATIC
UINTN
GetSizeTableIndexA (
  IN UINTN        LocalTokenNumberTableIdx,
  IN UINT32       *LocalTokenNumberTable,
  IN BOOLEAN      IsPeiDb
  )
{
  UINTN       i;
  UINTN       SizeTableIdx;
  UINTN       LocalTokenNumber;
  SKU_ID      *SkuIdTable;
  
  SizeTableIdx = 0;

  for (i=0; i<LocalTokenNumberTableIdx; i++) {
    LocalTokenNumber = LocalTokenNumberTable[i];

    if ((LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) == PCD_DATUM_TYPE_POINTER) {
      //
      // SizeTable only contain record for PCD_DATUM_TYPE_POINTER type 
      // PCD entry.
      //
      if (LocalTokenNumber & PCD_TYPE_VPD) {
          //
          // We have only one entry for VPD enabled PCD entry:
          // 1) MAX Size.
          // We consider current size is equal to MAX size.
          //
          SizeTableIdx++;
      } else {
        if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == 0) {
          //
          // We have only two entry for Non-Sku enabled PCD entry:
          // 1) MAX SIZE
          // 2) Current Size
          //
          SizeTableIdx += 2;
        } else {
          //
          // We have these entry for SKU enabled PCD entry
          // 1) MAX SIZE
          // 2) Current Size for each SKU_ID (It is equal to MaxSku).
          //
          SkuIdTable = GetSkuIdArray (i, IsPeiDb);
          SizeTableIdx += (UINTN)*SkuIdTable + 1;
        }
      }
    }

  }

  return SizeTableIdx;
}



STATIC
UINTN
GetSizeTableIndex (
  IN    UINTN             LocalTokenNumberTableIdx,
  IN    BOOLEAN           IsPeiDb
  )
{
  UINT32 *LocalTokenNumberTable;
  
  if (IsPeiDb) {
    LocalTokenNumberTable = mPcdDatabase->PeiDb.Init.LocalTokenNumberTable;
  } else {
    LocalTokenNumberTable = mPcdDatabase->DxeDb.Init.LocalTokenNumberTable;
  }
  return GetSizeTableIndexA (LocalTokenNumberTableIdx, 
                              LocalTokenNumberTable,
                              IsPeiDb);
}



UINTN
GetPtrTypeSize (
  IN    UINTN             LocalTokenNumberTableIdx,
  OUT   UINTN             *MaxSize
  )
{
  INTN        SizeTableIdx;
  UINTN       LocalTokenNumber;
  SKU_ID      *SkuIdTable;
  SIZE_INFO   *SizeTable;
  UINTN       i;
  BOOLEAN     IsPeiDb;
  UINT32      *LocalTokenNumberTable;

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  IsPeiDb = (BOOLEAN) (LocalTokenNumberTableIdx + 1 < PEI_LOCAL_TOKEN_NUMBER + 1);


  if (IsPeiDb) {
    LocalTokenNumberTable = mPcdDatabase->PeiDb.Init.LocalTokenNumberTable;
    SizeTable = mPcdDatabase->PeiDb.Init.SizeTable;
  } else {
    LocalTokenNumberTableIdx -= PEI_LOCAL_TOKEN_NUMBER;
    LocalTokenNumberTable = mPcdDatabase->DxeDb.Init.LocalTokenNumberTable;
    SizeTable = mPcdDatabase->DxeDb.Init.SizeTable;
  }

  LocalTokenNumber = LocalTokenNumberTable[LocalTokenNumberTableIdx];

  ASSERT ((LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) == PCD_DATUM_TYPE_POINTER);
  
  SizeTableIdx = GetSizeTableIndex (LocalTokenNumberTableIdx, IsPeiDb);

  *MaxSize = SizeTable[SizeTableIdx];
  //
  // SizeTable only contain record for PCD_DATUM_TYPE_POINTER type 
  // PCD entry.
  //
  if (LocalTokenNumber & PCD_TYPE_VPD) {
      //
      // We have only one entry for VPD enabled PCD entry:
      // 1) MAX Size.
      // We consider current size is equal to MAX size.
      //
      return *MaxSize;
  } else {
    if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == 0) {
      //
      // We have only two entry for Non-Sku enabled PCD entry:
      // 1) MAX SIZE
      // 2) Current Size
      //
      return SizeTable[SizeTableIdx + 1];
    } else {
      //
      // We have these entry for SKU enabled PCD entry
      // 1) MAX SIZE
      // 2) Current Size for each SKU_ID (It is equal to MaxSku).
      //
      SkuIdTable = GetSkuIdArray (LocalTokenNumberTableIdx, IsPeiDb);
      for (i = 0; i < SkuIdTable[0]; i++) {
        if (SkuIdTable[1 + i] == mPcdDatabase->PeiDb.Init.SystemSkuId) {
          return SizeTable[SizeTableIdx + 1 + i];
        }
      }
      return SizeTable[SizeTableIdx + 1];
    }
  }
}



BOOLEAN
SetPtrTypeSize (
  IN          UINTN             LocalTokenNumberTableIdx,
  IN    OUT   UINTN             *CurrentSize
  )
{
  INTN        SizeTableIdx;
  UINTN       LocalTokenNumber;
  SKU_ID      *SkuIdTable;
  SIZE_INFO   *SizeTable;
  UINTN       i;
  UINTN       MaxSize;
  BOOLEAN     IsPeiDb;
  UINT32      *LocalTokenNumberTable;

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  IsPeiDb = (BOOLEAN) (LocalTokenNumberTableIdx + 1 < PEI_LOCAL_TOKEN_NUMBER + 1);

  if (IsPeiDb) {
    LocalTokenNumberTable = mPcdDatabase->PeiDb.Init.LocalTokenNumberTable;
    SizeTable = mPcdDatabase->PeiDb.Init.SizeTable;
  } else {
    LocalTokenNumberTableIdx -= PEI_LOCAL_TOKEN_NUMBER;
    LocalTokenNumberTable = mPcdDatabase->DxeDb.Init.LocalTokenNumberTable;
    SizeTable = mPcdDatabase->DxeDb.Init.SizeTable;
  }

  LocalTokenNumber = LocalTokenNumberTable[LocalTokenNumberTableIdx];

  ASSERT ((LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) == PCD_DATUM_TYPE_POINTER);
  
  SizeTableIdx = GetSizeTableIndex (LocalTokenNumberTableIdx, IsPeiDb);

  MaxSize = SizeTable[SizeTableIdx];
  //
  // SizeTable only contain record for PCD_DATUM_TYPE_POINTER type 
  // PCD entry.
  //
  if (LocalTokenNumber & PCD_TYPE_VPD) {
      //
      // We shouldn't come here as we don't support SET for VPD
      //
      ASSERT (FALSE);
      return FALSE;
  } else {
    if ((*CurrentSize > MaxSize) ||
      (*CurrentSize == MAX_ADDRESS)) {
       *CurrentSize = MaxSize;
       return FALSE;
    } 
    
    if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == 0) {
      //
      // We have only two entry for Non-Sku enabled PCD entry:
      // 1) MAX SIZE
      // 2) Current Size
      //
      SizeTable[SizeTableIdx + 1] = (SIZE_INFO) *CurrentSize;
      return TRUE;
    } else {
      //
      // We have these entry for SKU enabled PCD entry
      // 1) MAX SIZE
      // 2) Current Size for each SKU_ID (It is equal to MaxSku).
      //
      SkuIdTable = GetSkuIdArray (LocalTokenNumberTableIdx, IsPeiDb);
      for (i = 0; i < SkuIdTable[0]; i++) {
        if (SkuIdTable[1 + i] == mPcdDatabase->PeiDb.Init.SystemSkuId) {
          SizeTable[SizeTableIdx + 1 + i] = (SIZE_INFO) *CurrentSize;
          return TRUE;
        }
      }
      SizeTable[SizeTableIdx + 1] = (SIZE_INFO) *CurrentSize;
      return TRUE;
    }
  }
}

