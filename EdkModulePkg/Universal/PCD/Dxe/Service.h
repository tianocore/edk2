/** @file
Private functions used by PCD DXE driver.

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name: Service.h

**/

#ifndef _SERVICE_H
#define _SERVICE_H

VOID 
DxeGetPcdEntryWorker (
  IN UINTN Token,
  IN CONST EFI_GUID       *Guid,  OPTIONAL
  IN PCD_DATA_TYPE    Type,
  OUT VOID            *Data
  );

EFI_STATUS 
DxeSetPcdEntryWorker (
  IN UINTN Token,
  IN CONST EFI_GUID       *Guid,  OPTIONAL
  IN PCD_DATA_TYPE    Type,
  IN CONST VOID       *Data
  );

UINTN
DxeGetPcdEntrySizeWorker (
  IN UINTN Token,
  IN CONST EFI_GUID       *Guid  OPTIONAL
  );

EFI_STATUS
DxeRegisterCallBackWorker (
  IN  UINTN        TokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction,
  IN  BOOLEAN                 Reigster
);

EFI_STATUS
DxeSetSku (
  UINTN Id
);

EFI_STATUS
DxeGetNextTokenWorker (
  IN OUT UINTN *Token,
  IN CONST EFI_GUID           *Guid     OPTIONAL
  );

VOID
InitPcdDxeDataBase (
  VOID
);

//
// Protocol Interface function declaration.
//
EFI_STATUS
EFIAPI
DxePcdSetSku (
  IN  UINTN                  SkuId
  )
;


UINT8
EFIAPI
DxePcdGet8 (
  IN UINTN  TokenNumber
  )
;


UINT16
EFIAPI
DxePcdGet16 (
  IN UINTN  TokenNumber
  )
;


UINT32
EFIAPI
DxePcdGet32 (
  IN UINTN  TokenNumber
  )
;


UINT64
EFIAPI
DxePcdGet64 (
  IN UINTN  TokenNumber
  )
;


VOID *
EFIAPI
DxePcdGetPtr (
  IN UINTN  TokenNumber
  )
;


BOOLEAN
EFIAPI
DxePcdGetBool (
  IN UINTN  TokenNumber
  )
;


UINTN
EFIAPI
DxePcdGetSize (
  IN UINTN  TokenNumber
  )
;


UINT8
EFIAPI
DxePcdGet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;


UINT16
EFIAPI
DxePcdGet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;


UINT32
EFIAPI
DxePcdGet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;



UINT64
EFIAPI
DxePcdGet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;



VOID *
EFIAPI
DxePcdGetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;


BOOLEAN
EFIAPI
DxePcdGetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;


UINTN
EFIAPI
DxePcdGetSizeEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
;


EFI_STATUS
EFIAPI
DxePcdSet8 (
  IN UINTN  TokenNumber,
  IN UINT8             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet16 (
  IN UINTN  TokenNumber,
  IN UINT16             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet32 (
  IN UINTN  TokenNumber,
  IN UINT32             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet64 (
  IN UINTN  TokenNumber,
  IN UINT64            Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSetPtr (
  IN UINTN  TokenNumber,
  IN CONST VOID        *Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSetBool (
  IN UINTN  TokenNumber,
  IN BOOLEAN           Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN UINT8             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN UINT16            Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN UINT32             Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN UINT64            Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN CONST VOID        *Value
  )
;


EFI_STATUS
EFIAPI
DxePcdSetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN BOOLEAN           Value
  )
;



EFI_STATUS
EFIAPI
PcdRegisterCallBackOnSet (
  IN  UINTN        TokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
;


EFI_STATUS
EFIAPI
PcdUnRegisterCallBackOnSet (
  IN  UINTN        TokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
;


EFI_STATUS
EFIAPI
DxePcdGetNextToken (
  IN CONST EFI_GUID               *Guid, OPTIONAL
  IN OUT  UINTN    *TokenNumber
  )
;


/*
  This DXE_PCD_DATABASE layout. The difference of DXE_PCD_DATABASE
  and PEI_PCD_DATABASE is as follows:

  1) No PCD_CALL_BACK_TABLE; DXE_PCD_DATABASE maintain a LinkList for the
    callback function registered.

  ---------------------------
  |  LIST_ENTRY GuidSpaceHead|
  ---------------------------
  |  PCD_DATABASE_HEADER    |
  ---------------------------
  |  GUID_TABLE             |  Aligned on GUID    (128 bits)
  ---------------------------
  |  PCD_INDEX_TABLE        |  Aligned on PCD_INDEX (see PCD_INDEX's declaration)
  ---------------------------
  |  IMAGE_STRING_TABLE     |  Aligned on 16 Bits
  ---------------------------
  |  IMAGE_PCD_INDEX        |  Unaligned
  ---------------------------
  |  Data Defaults          |  Unaligned
  ---------------------------
  |  Data Buffer            |
  |  for entries without    |
  |  defaults               |
  ---------------------------

*/


typedef struct {
  LIST_ENTRY       ListNode;
  LIST_ENTRY       TokenSpaceHead;
  CONST EFI_GUID   *GuidSpace;
} PCD_GUID_SPACE;

typedef struct {
  LIST_ENTRY ListNode;
  LIST_ENTRY CallbackListHead;
  UINTN      TokeNumber;
} PCD_TOKEN_SPACE;

typedef struct {
  LIST_ENTRY            ListNode;
  PCD_PROTOCOL_CALLBACK CallbackFunction;
} PCD_CALLBACK_ENTRY;

#define PCD_GUID_SPACE_FROM_LISTNODE(a) \
  _CR(a, PCD_GUID_SPACE, ListNode)

#define PCD_TOKEN_SPACE_FROM_LISTNODE(a) \
  _CR(a, PCD_TOKEN_SPACE, ListNode)

#define PCD_CALLBACK_ENTRY_FROM_LISTNODE(a) \
  _CR(a, PCD_CALLBACK_ENTRY, ListNode)

#endif
