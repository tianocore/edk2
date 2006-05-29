/** @file
Private functions used by PCD DXE driver.s

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name: Service.c

**/
#include "Service.h"


//
// Build Tool will generate DXE_PCD_DB_INIT_VALUE in Autogen.h
// Compression Algorithm will take care of the size optimization.
//

PCD_DATABASE * mPcdDatabase;

LIST_ENTRY mCallbackFnTable[PCD_TOTAL_TOKEN_NUMBER];

VOID *
GetWorkerByLocalTokenNumber (
  UINT32      LocalTokenNumber,
  BOOLEAN     IsPeiDb,
  UINTN       Size
  ) 
{
  UINT32              Offset;
  EFI_GUID            *GuidTable;
  UINT16              *StringTable;
  EFI_GUID            *Guid;
  UINT16              *Name;
  VARIABLE_HEAD       *VariableHead;
  EFI_STATUS          Status;
  UINTN               DataSize;
  VOID                *Data;
  VPD_HEAD            *VpdHead;
  UINT8               *PcdDb;
  UINT16              StringTableIdx;      

  if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == PCD_TYPE_SKU_ENABLED) {
    LocalTokenNumber = GetSkuEnabledTokenNumber (LocalTokenNumber & ~PCD_TYPE_SKU_ENABLED, Size, IsPeiDb);
  }

  PcdDb = IsPeiDb ? ((UINT8 *) &mPcdDatabase->PeiDb) : ((UINT8 *) &mPcdDatabase->DxeDb);
  StringTable = IsPeiDb ? mPcdDatabase->PeiDb.Init.StringTable :
                          mPcdDatabase->DxeDb.Init.StringTable;
  
  Offset     = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;
  
  switch (LocalTokenNumber & ~PCD_DATABASE_OFFSET_MASK) {
    case PCD_TYPE_VPD:
      VpdHead = (VPD_HEAD *) ((UINT8 *) PcdDb + Offset);
      return (VOID *) (FixedPcdGet32(PcdVpdBaseAddress) + VpdHead->Offset);
      
    case PCD_TYPE_HII:
      GuidTable   = IsPeiDb ? mPcdDatabase->PeiDb.Init.GuidTable :
                              mPcdDatabase->DxeDb.Init.GuidTable;
                              
      VariableHead = (VARIABLE_HEAD *) (PcdDb + Offset);
      
      Guid = &(GuidTable[VariableHead->GuidTableIndex]);
      Name = &(StringTable[VariableHead->StringIndex]);

      Status = GetHiiVariable (Guid, Name, &Data, &DataSize);
      ASSERT_EFI_ERROR (Status);
      ASSERT (DataSize >= (UINTN) (VariableHead->Offset + Size));

      return (UINT8 *) Data + VariableHead->Offset;

    case PCD_TYPE_STRING:
      StringTableIdx = (UINT16) *((UINT8 *) PcdDb + Offset);
      return (VOID *) &StringTable[StringTableIdx];

    case PCD_TYPE_DATA:
      return (VOID *) ((UINT8 *) PcdDb + Offset);
      break;

    default:
      ASSERT (FALSE);
      break;
      
  }

  ASSERT (FALSE);
      
  return NULL;
}
  
VOID *
GetWorker (
  UINTN  TokenNumber
  )
{
  UINT32        *LocalTokenNumberTable;
  UINT16        *SizeTable;
  BOOLEAN       IsPeiDb;

  ASSERT (TokenNumber < PCD_TOTAL_TOKEN_NUMBER);
  
  IsPeiDb = (TokenNumber <= PEI_LOCAL_TOKEN_NUMBER) ? TRUE : FALSE;

  LocalTokenNumberTable  = IsPeiDb ? mPcdDatabase->PeiDb.Init.LocalTokenNumberTable : 
                                     mPcdDatabase->DxeDb.Init.LocalTokenNumberTable;

  SizeTable              = IsPeiDb ? mPcdDatabase->PeiDb.Init.SizeTable: 
                                     mPcdDatabase->DxeDb.Init.SizeTable;

  TokenNumber            = IsPeiDb ? TokenNumber :
                                     TokenNumber - PEI_LOCAL_TOKEN_NUMBER;
  return GetWorkerByLocalTokenNumber (LocalTokenNumberTable[TokenNumber], IsPeiDb, SizeTable[TokenNumber]);
}



EFI_STATUS
DxeRegisterCallBackWorker (
  IN  UINTN                   TokenNumber,
  IN  CONST GUID              *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
)
{
  CALLBACK_FN_ENTRY       *FnTableEntry;
  EX_PCD_ENTRY_ATTRIBUTE  ExAttr;
  LIST_ENTRY              *ListHead;
  LIST_ENTRY              *ListNode;

  if (Guid != NULL) {
    GetExPcdTokenAttributes (Guid, TokenNumber, &ExAttr);
    TokenNumber = ExAttr.LocalTokenNumberAlias;
  }

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
  EX_PCD_ENTRY_ATTRIBUTE  ExAttr;
  LIST_ENTRY              *ListHead;
  LIST_ENTRY              *ListNode;

  if (Guid != NULL) {
    GetExPcdTokenAttributes (Guid, TokenNumber, &ExAttr);
    TokenNumber = ExAttr.LocalTokenNumberAlias;
  }

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



PCD_TOKEN_NUMBER
ExGetNextTokeNumber (
  IN CONST EFI_GUID         *Guid,
  IN PCD_TOKEN_NUMBER       TokenNumber,
  IN EFI_GUID               *GuidTable,
  IN UINTN                  SizeOfGuidTable,
  IN DYNAMICEX_MAPPING      *ExMapTable,
  IN UINTN                  SizeOfExMapTable
  )
{
  EFI_GUID         *MatchGuid;
  UINTN            Idx;
  UINTN            GuidTableIdx;
  BOOLEAN          Found;

  MatchGuid = ScanGuid (GuidTable, SizeOfGuidTable, Guid);
  if (MatchGuid == NULL) {
    return PCD_INVALID_TOKEN_NUMBER;
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
    if (TokenNumber == PCD_INVALID_TOKEN_NUMBER) {
      return ExMapTable[Idx].ExTokenNumber;
    }
    
    for ( ; Idx < SizeOfExMapTable; Idx++) {
      if (ExMapTable[Idx].ExTokenNumber == TokenNumber) {
        Idx++;
        if (Idx == SizeOfExMapTable) {
          //
          // Exceed the length of ExMap Table
          //
          return PCD_INVALID_TOKEN_NUMBER;
        } else if (ExMapTable[Idx].ExGuidIndex == GuidTableIdx) {
          //
          // Found the next match
          //
          return ExMapTable[Idx].ExTokenNumber;
        } else {
          //
          // Guid has been changed. It is the next Token Space Guid.
          // We should flag no more TokenNumber.
          //
          return PCD_INVALID_TOKEN_NUMBER;
        }
      }
    }
  }
  
  return PCD_INVALID_TOKEN_NUMBER;
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
  ASSERT (GuidHob != NULL);

  PeiDatabase = (PEI_PCD_DATABASE *) GET_GUID_HOB_DATA (GuidHob);
  //
  // Copy PCD Entries refereneced in PEI phase to PCD DATABASE
  //
  CopyMem (&mPcdDatabase->PeiDb, PeiDatabase, sizeof (PEI_PCD_DATABASE));

  //
  // Copy PCD Entries with default value to PCD DATABASE
  //
  CopyMem (&mPcdDatabase->DxeDb.Init, &gDXEPcdDbInit, sizeof(DXE_PCD_DATABASE_INIT));


  //
  // Initialized the Callback Function Table
  //
  for (Idx = 0; Idx < PCD_TOTAL_TOKEN_NUMBER; Idx++) {
    InitializeListHead (&mCallbackFnTable[Idx]);
  }
    
  return;
}



EFI_STATUS
GetHiiVariable (
  IN  EFI_GUID      *VariableGuid,
  IN  UINT16        *VariableName,
  OUT VOID          ** VariableData,
  OUT UINTN         *VariableSize
  )
{
  UINTN      Size;
  EFI_STATUS Status;
  VOID       *Buffer;

  Status = EfiGetVariable (
    (UINT16 *)VariableName,
    VariableGuid,
    NULL,
    &Size,
    NULL
    );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  Buffer = AllocatePool (Size);

  ASSERT (Buffer != NULL);

  Status = EfiGetVariable (
    VariableName,
    VariableGuid,
    NULL,
    &Size,
    Buffer
    );

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

  switch (LocalTokenNumber & ~PCD_DATABASE_OFFSET_MASK) {
    case PCD_TYPE_VPD:
      Value = (UINT8 *) &(((VPD_HEAD *) Value)[i]);
      return ((Value - PcdDb) | PCD_TYPE_VPD);

    case PCD_TYPE_HII:
      Value = (UINT8 *) &(((VARIABLE_HEAD *) Value)[i]);
      return ((Value - PcdDb) | PCD_TYPE_HII);
      
    case PCD_TYPE_DATA:
      Value += Size * i;
      return (Value - PcdDb);
      
    default:
      ASSERT (FALSE);
  }

  ASSERT (FALSE);

  return 0;
  
}





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
SetWorker (
  PCD_TOKEN_NUMBER        TokenNumber,
  VOID                    *Data,
  UINTN                   Size,
  BOOLEAN                 PtrType
  )
{
  UINT32              *LocalTokenNumberTable;
  BOOLEAN             IsPeiDb;


  ASSERT (TokenNumber < PCD_TOTAL_TOKEN_NUMBER);

  if (PtrType) {
    ASSERT (Size <= DxePcdGetSize (TokenNumber));
  } else {
    ASSERT (Size == DxePcdGetSize (TokenNumber));
  }
  
  IsPeiDb = (TokenNumber <= PEI_LOCAL_TOKEN_NUMBER) ? TRUE : FALSE;

  LocalTokenNumberTable  = IsPeiDb ? mPcdDatabase->PeiDb.Init.LocalTokenNumberTable : 
                                     mPcdDatabase->DxeDb.Init.LocalTokenNumberTable;

  InvokeCallbackOnSet (0, NULL, TokenNumber, Data, Size);

  TokenNumber = IsPeiDb ? TokenNumber
                        : TokenNumber - PEI_LOCAL_TOKEN_NUMBER;
  
  return SetWorkerByLocalTokenNumber (LocalTokenNumberTable[TokenNumber], Data, Size, PtrType, IsPeiDb);

}





VOID *
ExGetWorker (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber,
  IN UINTN                  GetSize
  ) 
{
  EX_PCD_ENTRY_ATTRIBUTE Attr;

  GetExPcdTokenAttributes (Guid, ExTokenNumber, &Attr);

  ASSERT ((GetSize == Attr.Size) || (GetSize == 0));

  return GetWorkerByLocalTokenNumber (Attr.LocalTokenNumberAlias,
                                                Attr.IsPeiDb,
                                                Attr.Size
                                                );
}





EFI_STATUS
ExSetWorker (
  IN PCD_TOKEN_NUMBER     ExTokenNumber,
  IN CONST EFI_GUID       *Guid,
  VOID                    *Data,
  UINTN                   SetSize,
  BOOLEAN                 PtrType
  )
{
  EX_PCD_ENTRY_ATTRIBUTE Attr;

  GetExPcdTokenAttributes (Guid, ExTokenNumber, &Attr);

  ASSERT (!PtrType && (SetSize == Attr.Size));

  ASSERT (PtrType && (SetSize <= Attr.Size));

  InvokeCallbackOnSet (ExTokenNumber, Guid, Attr.TokenNumber, Data, Attr.Size);

  SetWorkerByLocalTokenNumber (Attr.LocalTokenNumberAlias, Data, Attr.Size, PtrType, Attr.IsPeiDb);

  return EFI_SUCCESS;
  
}




EFI_STATUS
SetWorkerByLocalTokenNumber (
  UINT32        LocalTokenNumber,
  VOID          *Data,
  UINTN         Size,
  BOOLEAN       PtrType,
  BOOLEAN       IsPeiDb
  )
{
  EFI_GUID            *GuidTable;
  UINT16              *StringTable;
  EFI_GUID            *Guid;
  UINT16              *Name;
  VOID                *InternalData;
  VARIABLE_HEAD       *VariableHead;
  UINTN               Offset;
  UINT8               *PcdDb;


  if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == PCD_TYPE_SKU_ENABLED) {
    LocalTokenNumber = GetSkuEnabledTokenNumber (LocalTokenNumber & ~PCD_TYPE_SKU_ENABLED, Size, IsPeiDb);
  }

  Offset = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;

  PcdDb = IsPeiDb ? ((UINT8 *) &mPcdDatabase->PeiDb) : ((UINT8 *) &mPcdDatabase->DxeDb);

  StringTable = IsPeiDb ? mPcdDatabase->PeiDb.Init.StringTable :
                          mPcdDatabase->DxeDb.Init.StringTable;
  
  InternalData = PcdDb + Offset;

  switch (LocalTokenNumber & ~PCD_DATABASE_OFFSET_MASK) {
    case PCD_TYPE_VPD:
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    
    case PCD_TYPE_STRING:
      CopyMem (&StringTable[*((UINT16 *)InternalData)], Data, Size);
      break;

    case PCD_TYPE_HII:
      //
      // Bug Bug: Please implement this
      //
      GuidTable   = IsPeiDb ? mPcdDatabase->PeiDb.Init.GuidTable :
                              mPcdDatabase->DxeDb.Init.GuidTable;
                              
      VariableHead = (VARIABLE_HEAD *) (PcdDb + Offset);
      
      Guid = &(GuidTable[VariableHead->GuidTableIndex]);
      Name = &(StringTable[VariableHead->StringIndex]);

      return EFI_SUCCESS;

    case PCD_TYPE_DATA:
      if (PtrType) {
        CopyMem (InternalData, Data, Size);
        return EFI_SUCCESS;
      }

      switch (Size) {
        case sizeof(UINT8):
          *((UINT8 *) InternalData) = *((UINT8 *) Data);
          return EFI_SUCCESS;

        case sizeof(UINT16):
          *((UINT16 *) InternalData) = *((UINT16 *) Data);
          return EFI_SUCCESS;

        case sizeof(UINT32):
          *((UINT32 *) InternalData) = *((UINT32 *) Data);
          return EFI_SUCCESS;

        case sizeof(UINT64):
          *((UINT64 *) InternalData) = *((UINT64 *) Data);
          return EFI_SUCCESS;

        default:
          ASSERT (FALSE);
          return EFI_NOT_FOUND;
      }

    default:
      ASSERT (FALSE);
      break;
    }
      
  ASSERT (FALSE);
  return EFI_NOT_FOUND;
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

  Status = EfiGetVariable (
    (UINT16 *)VariableName,
    VariableGuid,
    &Attribute,
    &Size,
    NULL
    );

  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  Buffer = AllocatePool (Size);

  ASSERT (Buffer != NULL);

  Status = EfiGetVariable (
    VariableName,
    VariableGuid,
    &Attribute,
    &Size,
    Buffer
    );


  CopyMem ((UINT8 *)Buffer + Offset, Data, DataSize);

  return EfiSetVariable (
    VariableName,
    VariableGuid,
    Attribute,
    Size,
    Buffer
    );

}





VOID
GetExPcdTokenAttributes (
  IN CONST EFI_GUID             *Guid,
  IN PCD_TOKEN_NUMBER           ExTokenNumber,
  OUT EX_PCD_ENTRY_ATTRIBUTE    *ExAttr
  )
{
  UINT32              i;
  DYNAMICEX_MAPPING   *ExMap;
  EFI_GUID            *GuidTable;
  UINT16              *SizeTable;

  ExMap       = mPcdDatabase->PeiDb.Init.ExMapTable;
  GuidTable   = mPcdDatabase->PeiDb.Init.GuidTable;
  SizeTable   = mPcdDatabase->PeiDb.Init.SizeTable;
  
  for (i = 0; i < PEI_EXMAPPING_TABLE_SIZE; i++) {
    if ((ExTokenNumber == ExMap[i].ExTokenNumber) &&
        CompareGuid (Guid, (CONST EFI_GUID *) &GuidTable[ExMap[i].ExGuidIndex])
      ) {

        ExAttr->IsPeiDb               = TRUE;
        ExAttr->Size                  = SizeTable[i + PEI_NEX_TOKEN_NUMBER];
        ExAttr->TokenNumber           = i + PEI_NEX_TOKEN_NUMBER;
        ExAttr->LocalTokenNumberAlias = ExMap[i].LocalTokenNumber;
        return;

    }
  }
  
  ExMap       = mPcdDatabase->DxeDb.Init.ExMapTable;
  GuidTable   = mPcdDatabase->DxeDb.Init.GuidTable;
  SizeTable   = mPcdDatabase->DxeDb.Init.SizeTable;
  
  for (i = 0; i < DXE_EXMAPPING_TABLE_SIZE; i++) {
    if ((ExTokenNumber == ExMap[i].ExTokenNumber) &&
         CompareGuid (Guid, (CONST EFI_GUID *) &GuidTable[ExMap[i].ExGuidIndex])
      ) {

        ExAttr->IsPeiDb               = FALSE;
        ExAttr->Size                  = SizeTable[i + DXE_NEX_TOKEN_NUMBER];
        ExAttr->TokenNumber           = i + PEI_LOCAL_TOKEN_NUMBER;
        ExAttr->LocalTokenNumberAlias = ExMap[i].LocalTokenNumber;
        return;

    }
  }

  ASSERT (FALSE);

  return;
}

