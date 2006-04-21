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
#include "../Common/PcdCommon.h"
#include "Service.h"

static PCD_DATABASE *PrivatePcdDxeDatabase;
static LIST_ENTRY   mPcdDatabaseListHead = INITIALIZE_LIST_HEAD_VARIABLE(mPcdDatabaseListHead);

LIST_ENTRY *
GetPcdDatabaseListHead (
  VOID
  )
{
  return &mPcdDatabaseListHead;
}

PCD_DATABASE *
GetPcdDxeDataBaseInstance (
  VOID
)
{
  return PrivatePcdDxeDatabase;
}

PCD_DATABASE *
SetPcdDxeDataBaseInstance (
  PCD_DATABASE *PcdDatabase
)
{
  return PrivatePcdDxeDatabase = PcdDatabase;
}


VOID
DxeGetPcdEntryWorker (
  IN UINTN            TokenNumber,
  IN CONST GUID       *Guid,  OPTIONAL
  IN PCD_DATA_TYPE    Type,
  OUT VOID            *Data
  )
{
  PCD_DATABASE *Database;
  Database = GetPcdDxeDataBaseInstance ();

  GetPcdEntryWorker ( &Database->Info,
                      TokenNumber,
                      Guid,
                      Type,
                      Data
                      );


  return;
}



EFI_STATUS
DxeSetPcdEntryWorker (
  IN UINTN              TokenNumber,
  IN CONST GUID         *Guid,  OPTIONAL
  IN PCD_DATA_TYPE      Type,
  IN CONST VOID         *Data
  )
{
  PCD_DATABASE            *Database;
  PCD_INDEX               *PcdIndex;
  EFI_STATUS              Status;

  Database = GetPcdDxeDataBaseInstance ();


  ASSERT (Data != NULL);

  PcdIndex = FindPcdIndex (TokenNumber, Guid, &Database->Info, NULL);

  ASSERT (PcdIndex != NULL);

  ASSERT (PcdIndex->StateByte.DataType == Type);

  //
  // Invoke the callback function.
  //

  Status = SetPcdData (PcdIndex, &Database->Info, Data);

  return Status;


}



UINTN
DxeGetPcdEntrySizeWorker (
  IN UINTN            TokenNumber,
  IN CONST GUID       *Guid  OPTIONAL
  )
{
  PCD_DATABASE *Database;
  Database = GetPcdDxeDataBaseInstance ();

  return GetPcdEntrySizeWorker (&Database->Info,
                                TokenNumber,
                                Guid
                                );
}



LIST_ENTRY *
InsertToGuidSpaceListI (
  IN LIST_ENTRY       *GuidSpaceListHead,
  IN CONST EFI_GUID   *Guid
  )
{
  PCD_GUID_SPACE    *GuidSpaceEntry;

  GuidSpaceEntry = AllocatePool (sizeof (PCD_GUID_SPACE));
  ASSERT (GuidSpaceEntry != NULL);

  GuidSpaceEntry->GuidSpace= Guid;
  InitializeListHead (&GuidSpaceEntry->TokenSpaceHead);
  
  InsertTailList (GuidSpaceListHead, &GuidSpaceEntry->ListNode);

  return &GuidSpaceEntry->TokenSpaceHead;
}



LIST_ENTRY *
InsertToTokenSpaceListI (
  IN LIST_ENTRY *TokenSpaceListHead,
  IN UINTN      TokenNumber
  )
{
  PCD_TOKEN_SPACE   *TokenSpaceEntry;

  TokenSpaceEntry = AllocatePool (sizeof (PCD_TOKEN_SPACE));
  ASSERT (TokenSpaceEntry != NULL);

  TokenSpaceEntry->TokeNumber = TokenNumber;
  InitializeListHead (&TokenSpaceEntry->CallbackListHead);
  
  InsertTailList (TokenSpaceListHead, &TokenSpaceEntry->ListNode);

  return &TokenSpaceEntry->CallbackListHead;
}



VOID
InsertToCallbackListI (
  IN  LIST_ENTRY *CallbackListHead,
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
{
  PCD_CALLBACK_ENTRY *CallbackEntry;
  
  CallbackEntry = AllocatePool (sizeof (PCD_CALLBACK_ENTRY));
  ASSERT (CallbackEntry != NULL);
  CallbackEntry->CallbackFunction = CallBackFunction;
  InsertTailList (CallbackListHead, &CallbackEntry->ListNode);
  
  return;
}




VOID
InsertToCallbackList (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid,
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
{
  LIST_ENTRY        *GuidListNode;
  LIST_ENTRY        *GuidListHead;
  LIST_ENTRY        *TokenListNode;
  LIST_ENTRY        *TokenListHead;
  LIST_ENTRY        *CallbackListHead;
  PCD_GUID_SPACE    *GuidSpaceEntry;
  PCD_TOKEN_SPACE   *TokenSpaceEntry;

  
  GuidListHead = GetPcdDatabaseListHead ();

  GuidListNode = GetFirstNode (GuidListHead);
  while (!IsNull (GuidListNode, GuidListHead)) {
    GuidSpaceEntry = PCD_GUID_SPACE_FROM_LISTNODE(GuidListNode);

    if (CompareGuid (GuidSpaceEntry->GuidSpace, Guid)) {
      TokenListHead = &GuidSpaceEntry->TokenSpaceHead;
      TokenListNode = GetFirstNode (TokenListHead);
      while (!IsNull (TokenListNode, TokenListHead)) {
        TokenSpaceEntry = PCD_TOKEN_SPACE_FROM_LISTNODE(TokenListNode);
        if (TokenSpaceEntry->TokeNumber == TokenNumber) {
          InsertToCallbackListI (&TokenSpaceEntry->CallbackListHead , CallBackFunction);
        }
      }

      //
      // No TokenNumber match input found in this GuidSpace
      //
      CallbackListHead = InsertToTokenSpaceListI (TokenListHead, TokenNumber);
      InsertToCallbackListI (CallbackListHead , CallBackFunction);
    }
    
    GuidListNode = GetNextNode (GuidListHead, GuidListNode);
  }

  //
  // No GuidSpace match the input Guid, so build the GuidSpace, TokenNumberSpace and Callback
  //
  TokenListHead = InsertToGuidSpaceListI (GuidListHead, Guid);
  CallbackListHead = InsertToTokenSpaceListI (TokenListHead, TokenNumber);
  InsertToCallbackListI (CallbackListHead , CallBackFunction);

  return;
  
}

EFI_STATUS
RemoveFromCallbackListI (
  IN  LIST_ENTRY              *CallbackListHead,
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
{
  LIST_ENTRY          *ListNode;
  PCD_CALLBACK_ENTRY  *CallbackEntry;

  ListNode = GetFirstNode (CallbackListHead);

  while (!IsNull(CallbackListHead, ListNode)) {
    CallbackEntry = PCD_CALLBACK_ENTRY_FROM_LISTNODE(ListNode);

    if (CallbackEntry->CallbackFunction == CallBackFunction) {
      RemoveEntryList (ListNode);
      FreePool (CallbackEntry);
      return EFI_SUCCESS;
    }
    ListNode = GetNextNode (CallbackListHead, ListNode);
  }

  return EFI_NOT_FOUND;
}



EFI_STATUS
RemoveFromCallbackList (
  IN  UINTN                   TokenNumber,
  IN  CONST GUID              *Guid,
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
{
  LIST_ENTRY        *GuidListNode;
  LIST_ENTRY        *GuidListHead;
  LIST_ENTRY        *TokenListNode;
  LIST_ENTRY        *TokenListHead;
  PCD_GUID_SPACE    *GuidSpaceEntry;
  PCD_TOKEN_SPACE   *TokenSpaceEntry;

  
  GuidListHead = GetPcdDatabaseListHead ();

  GuidListNode = GetFirstNode (GuidListHead);
  while (!IsNull (GuidListNode, GuidListHead)) {
    
    GuidSpaceEntry = PCD_GUID_SPACE_FROM_LISTNODE(GuidListNode);
    if (CompareGuid (GuidSpaceEntry->GuidSpace, Guid)) {
      
      TokenListHead = &GuidSpaceEntry->TokenSpaceHead;
      TokenListNode = GetFirstNode (TokenListHead);
      while (!IsNull (TokenListNode, TokenListHead)) {
        
        TokenSpaceEntry = PCD_TOKEN_SPACE_FROM_LISTNODE(TokenListNode);
        if (TokenSpaceEntry->TokeNumber == TokenNumber) {
          return RemoveFromCallbackListI (&TokenSpaceEntry->CallbackListHead , CallBackFunction);
        }
      }

      //
      // No TokenNumber match input found in this GuidSpace
      //
      return EFI_NOT_FOUND;
    }
    
    GuidListNode = GetNextNode (GuidListHead, GuidListNode);
  }


  return EFI_NOT_FOUND;
  
}



EFI_STATUS
DxeRegisterCallBackWorker (
  IN  UINTN                   TokenNumber,
  IN  CONST GUID              *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction,
  IN  BOOLEAN                 Register
)
{
  PCD_DATABASE *Database;
  PCD_INDEX    *PcdIndex;
  
  Database = GetPcdDxeDataBaseInstance ();

  PcdIndex = FindPcdIndex (TokenNumber, Guid, &Database->Info, NULL);

  if (PcdIndex == NULL) {
    return EFI_NOT_FOUND;
  }

  if (Register) {
    InsertToCallbackList (TokenNumber, Guid, CallBackFunction);
    return EFI_SUCCESS;
  } else {
    return RemoveFromCallbackList (TokenNumber, Guid, CallBackFunction); 
  }

 }



EFI_STATUS
DxeSetSku (
  UINTN Id
)
{
  PCD_DATABASE * Database;

  Database = GetPcdDxeDataBaseInstance ();

  return Database->Info.SkuId = Id;

}



EFI_STATUS
DxeGetNextTokenWorker (
  IN OUT UINTN            *TokenNumber,
  IN CONST GUID           *Guid     OPTIONAL
  )
{
  PCD_DATABASE * Database;

  Database = GetPcdDxeDataBaseInstance ();

  return GetNextTokenWorker (&Database->Info,
                             TokenNumber,
                             Guid
                             );
}



VOID
InitPcdDxeDataBase (
  VOID
)
{
  PCD_DATABASE        *PeiDatabase;
  PCD_DATABASE        *DxeDatabase;
  EFI_HOB_GUID_TYPE   *GuidHob;

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  ASSERT (GuidHob != NULL);

  PeiDatabase = (PCD_DATABASE *) GET_GUID_HOB_DATA (GuidHob);

  DxeDatabase = AllocateCopyPool (PeiDatabase->Info.DatabaseLen, PeiDatabase);

  ASSERT (DxeDatabase != NULL);

  SetPcdDxeDataBaseInstance (DxeDatabase);

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



EFI_STATUS
SetHiiVariable (
  IN  EFI_GUID     *VariableGuid,
  IN  UINT16       *VariableName,
  IN  CONST VOID   *Data,
  IN  UINTN        DataSize,
  IN  UINTN        Offset
  )
{
  UINTN Size;
  VOID  *Buffer;
  EFI_STATUS Status;

  Size = 0;

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


  CopyMem ((UINT8 *)Buffer + Offset, Data, DataSize);

  return EfiSetVariable (
    VariableName,
    VariableGuid,
    0,
    Size,
    Buffer
    );

}

