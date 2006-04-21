/** @file
PCD DXE driver

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name: Pcd.c

**/

#include "../Common/PcdCommon.h"
#include "Service.h"


PCD_PROTOCOL mPcdInstance = {
  DxePcdSetSku,

  DxePcdGet8,
  DxePcdGet16,
  DxePcdGet32,
  DxePcdGet64,
  DxePcdGetPtr,
  DxePcdGetBool,
  DxePcdGetSize,

  DxePcdGet8Ex,
  DxePcdGet16Ex,
  DxePcdGet32Ex,
  DxePcdGet64Ex,
  DxePcdGetPtrEx,
  DxePcdGetBoolEx,
  DxePcdGetSizeEx,

  DxePcdSet8,
  DxePcdSet16,
  DxePcdSet32,
  DxePcdSet64,
  DxePcdSetPtr,
  DxePcdSetBool,

  DxePcdSet8Ex,
  DxePcdSet16Ex,
  DxePcdSet32Ex,
  DxePcdSet64Ex,
  DxePcdSetPtrEx,
  DxePcdSetBoolEx,

  PcdRegisterCallBackOnSet,
  PcdUnRegisterCallBackOnSet,
  DxePcdGetNextToken
};


//
// Static global to reduce the code size
//
static EFI_HANDLE NewHandle = NULL;

EFI_STATUS
EFIAPI
PcdDxeInit (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS  Status;
  
  InitPcdDxeDataBase ();
  
  Status = gBS->InstallProtocolInterface (
                  &NewHandle,
                  &gPcdProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPcdInstance
                  );

  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;

}


EFI_STATUS
EFIAPI
DxePcdSetSku (
  IN  UINTN                  SkuId
  )
{
  return DxeSetSku(SkuId);
}



UINT8
EFIAPI
DxePcdGet8 (
  IN UINTN  TokenNumber
  )
{
  return DxePcdGet8Ex (NULL, TokenNumber);
}



UINT16
EFIAPI
DxePcdGet16 (
  IN UINTN  TokenNumber
  )
{
  return DxePcdGet16Ex (NULL, TokenNumber);
}



UINT32
EFIAPI
DxePcdGet32 (
  IN UINTN  TokenNumber
  )
{
  return DxePcdGet32Ex (NULL, TokenNumber);
}



UINT64
EFIAPI
DxePcdGet64 (
  IN UINTN  TokenNumber
  )
{
  return DxePcdGet32Ex (NULL, TokenNumber);
}



VOID *
EFIAPI
DxePcdGetPtr (
  IN UINTN  TokenNumber
  )
{
  return DxePcdGetPtrEx (NULL, TokenNumber);
}



BOOLEAN
EFIAPI
DxePcdGetBool (
  IN UINTN  TokenNumber
  )
{
  return DxePcdGetBoolEx (NULL, TokenNumber);
}



UINTN
EFIAPI
DxePcdGetSize (
  IN UINTN  TokenNumber
  )
{
  return  DxePcdGetSizeEx (NULL, TokenNumber);
}



UINT8
EFIAPI
DxePcdGet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
{
  UINT8 Data;

  DxeGetPcdEntryWorker (TokenNumber, Guid, PcdByte8, &Data);

  return Data;
}



UINT16
EFIAPI
DxePcdGet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
{
  UINT16 Data;

  DxeGetPcdEntryWorker (TokenNumber, Guid, PcdByte16, &Data);

  return Data;
}



UINT32
EFIAPI
DxePcdGet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
{
  UINT32 Data;

  DxeGetPcdEntryWorker (TokenNumber, Guid, PcdByte32, &Data);

  return Data;
}



UINT64
EFIAPI
DxePcdGet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
{
  UINT64 Data;

  DxeGetPcdEntryWorker (TokenNumber, Guid, PcdByte64, &Data);

  return Data;
}



VOID *
EFIAPI
DxePcdGetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
{
  VOID *Data;

  DxeGetPcdEntryWorker (TokenNumber, Guid, PcdPointer, &Data);

  return Data;
}



BOOLEAN
EFIAPI
DxePcdGetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
{
  BOOLEAN Data;
  DxeGetPcdEntryWorker (TokenNumber, Guid, PcdBoolean, &Data);
  return Data;
}



UINTN
EFIAPI
DxePcdGetSizeEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
{
  return DxeGetPcdEntrySizeWorker (TokenNumber, Guid);
}



EFI_STATUS
EFIAPI
DxePcdSet8 (
  IN UINTN  TokenNumber,
  IN UINT8             Value
  )
{
  return DxePcdSet8Ex (NULL, TokenNumber, Value);
}



EFI_STATUS
EFIAPI
DxePcdSet16 (
  IN UINTN  TokenNumber,
  IN UINT16             Value
  )
{
  return DxePcdSet16Ex (NULL, TokenNumber, Value);
}



EFI_STATUS
EFIAPI
DxePcdSet32 (
  IN UINTN  TokenNumber,
  IN UINT32             Value
  )
{
  return DxePcdSet32Ex (NULL, TokenNumber, Value);
}



EFI_STATUS
EFIAPI
DxePcdSet64 (
  IN UINTN  TokenNumber,
  IN UINT64            Value
  )
{
  return DxePcdSet64Ex (NULL, TokenNumber, Value);
}



EFI_STATUS
EFIAPI
DxePcdSetPtr (
  IN UINTN  TokenNumber,
  IN CONST VOID        *Value
  )
{
  return DxePcdSetPtrEx (NULL, TokenNumber, Value);
}



EFI_STATUS
EFIAPI
DxePcdSetBool (
  IN UINTN  TokenNumber,
  IN BOOLEAN           Value
  )
{
  return DxePcdSetBoolEx (NULL, TokenNumber, Value);
}



EFI_STATUS
EFIAPI
DxePcdSet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN UINT8             Value
  )
{
  return DxeSetPcdEntryWorker (TokenNumber, Guid, PcdByte8, &Value);
}



EFI_STATUS
EFIAPI
DxePcdSet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN UINT16            Value
  )
{
  return DxeSetPcdEntryWorker (TokenNumber, Guid, PcdByte16, &Value);
}



EFI_STATUS
EFIAPI
DxePcdSet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN UINT32             Value
  )
{
  return DxeSetPcdEntryWorker (TokenNumber, Guid, PcdByte32, &Value);
}



EFI_STATUS
EFIAPI
DxePcdSet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN UINT64            Value
  )
{
  return DxeSetPcdEntryWorker (TokenNumber, Guid, PcdByte64, &Value);
}



EFI_STATUS
EFIAPI
DxePcdSetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN CONST VOID        *Value
  )
{
  return DxeSetPcdEntryWorker (TokenNumber, Guid, PcdPointer, Value);
}



EFI_STATUS
EFIAPI
DxePcdSetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber,
  IN BOOLEAN           Value
  )
{
  return DxeSetPcdEntryWorker (TokenNumber, Guid, PcdBoolean, &Value);

}




EFI_STATUS
EFIAPI
PcdRegisterCallBackOnSet (
  IN  UINTN        TokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
{
  return DxeRegisterCallBackWorker (TokenNumber, Guid, CallBackFunction, TRUE);
}



EFI_STATUS
EFIAPI
PcdUnRegisterCallBackOnSet (
  IN  UINTN        TokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
{
  return DxeRegisterCallBackWorker (TokenNumber, Guid, CallBackFunction, FALSE);
}



EFI_STATUS
EFIAPI
DxePcdGetNextToken (
  IN CONST EFI_GUID               *Guid, OPTIONAL
  IN OUT  UINTN    *TokenNumber
  )
{
  return DxeGetNextTokenWorker (TokenNumber, Guid);
}

