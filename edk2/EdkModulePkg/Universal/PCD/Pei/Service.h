/** @file
Private functions used by PCD PEIM.

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

/* Internal Function definitions */

PEI_PCD_DATABASE *
GetPcdDatabase (
  VOID
  )
;

EFI_STATUS
SetWorker (
  IN UINTN         TokenNumber,
  IN VOID          *Data,
  IN UINTN         Size,
  IN BOOLEAN       PtrType
  )
;

EFI_STATUS
SetWorkerByLocalTokenNumber (
  UINT32        LocalTokenNumber,
  VOID          *Data,
  UINTN         Size,
  BOOLEAN       PtrType
  )
;

EFI_STATUS
ExSetWorker (
  IN UINT32               ExTokenNumber,
  IN CONST EFI_GUID       *Guid,
  VOID                    *Data,
  UINTN                   Size,
  BOOLEAN                 PtrType
  )
;

VOID *
GetWorker (
  UINTN     TokenNumber,
  UINTN     GetSize
  )
;

VOID *
ExGetWorker (
  IN CONST EFI_GUID   *Guid,
  IN UINT32           ExTokenNumber,
  IN UINTN            GetSize
  )
;

typedef struct {
  UINTN   TokenNumber;
  UINTN   Size;
  UINT32  LocalTokenNumberAlias;
} EX_PCD_ENTRY_ATTRIBUTE;

VOID
GetExPcdTokenAttributes (
  IN CONST EFI_GUID             *Guid,
  IN UINT32                     ExTokenNumber,
  OUT EX_PCD_ENTRY_ATTRIBUTE    *ExAttr
  )
;

EFI_STATUS
PeiRegisterCallBackWorker (
  IN  UINTN              TokenNumber,
  IN  CONST GUID         *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK   CallBackFunction,
  IN  BOOLEAN            Register
);

VOID
BuildPcdDatabase (
  VOID
  )
;


//
// PPI Interface Implementation Declaration.
//
VOID
EFIAPI
PeiPcdSetSku (
  IN  SKU_ID                  SkuId
  )
;


UINT8
EFIAPI
PeiPcdGet8 (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINT16
EFIAPI
PeiPcdGet16 (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINT32
EFIAPI
PeiPcdGet32 (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINT64
EFIAPI
PeiPcdGet64 (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


VOID *
EFIAPI
PeiPcdGetPtr (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


BOOLEAN
EFIAPI
PeiPcdGetBool (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINTN
EFIAPI
PeiPcdGetSize (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINT8
EFIAPI
PeiPcdGet8Ex (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINT16
EFIAPI
PeiPcdGet16Ex (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;

UINT32
EFIAPI
PeiPcdGet32Ex (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINT64
EFIAPI
PeiPcdGet64Ex (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


VOID *
EFIAPI
PeiPcdGetPtrEx (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


BOOLEAN
EFIAPI
PeiPcdGetBoolEx (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


UINTN
EFIAPI
PeiPcdGetSizeEx (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet8 (
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT8             Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet16 (
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT16            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet32 (
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT32            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet64 (
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT64            Value
  )
;

EFI_STATUS
EFIAPI
PeiPcdSetPtr (
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINTN             SizeOfBuffer,
  IN VOID              *Buffer
  )
;


EFI_STATUS
EFIAPI
PeiPcdSetBool (
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN BOOLEAN           Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet8Ex (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT8             Value
  )
;

EFI_STATUS
EFIAPI
PeiPcdSet16Ex (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT16            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet32Ex (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT32            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSet64Ex (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINT64            Value
  )
;


EFI_STATUS
EFIAPI
PeiPcdSetPtrEx (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN UINTN             SizeOfBuffer,
  IN VOID              *Buffer
  )
;


EFI_STATUS
EFIAPI
PeiPcdSetBoolEx (
  IN CONST EFI_GUID    *Guid,
  IN PCD_TOKEN_NUMBER  TokenNumber,
  IN BOOLEAN           Value
  )
;



EFI_STATUS
EFIAPI
PcdRegisterCallBackOnSet (
  IN  PCD_TOKEN_NUMBER        TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK        CallBackFunction
  )
;


EFI_STATUS
EFIAPI
PcdUnRegisterCallBackOnSet (
  IN  PCD_TOKEN_NUMBER        TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK        CallBackFunction
  )
;


EFI_STATUS
EFIAPI
PeiPcdGetNextToken (
  IN CONST EFI_GUID           *Guid, OPTIONAL
  IN OUT  PCD_TOKEN_NUMBER    *TokenNumber
  )
;

extern EFI_GUID gPcdDataBaseHobGuid;

extern EFI_GUID gPcdPeiCallbackFnTableHobGuid;

extern PEI_PCD_DATABASE_INIT gPEIPcdDbInit;

#endif
