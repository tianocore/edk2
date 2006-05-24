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
  EFI_STATUS          Status;

  //
  // Make sure the Pcd Protocol is not already installed in the system
  //
  
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gPcdProtocolGuid);
  
  BuildPcdDxeDataBase ();

  //
  // BugBug Check if PcdDatabase is already installed.
  //
  
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
  IN  UINTN        SkuId
  )
{
  return gPcdDatabase->PeiDb.Init.SystemSkuId = (SKU_ID) SkuId;
}



UINT8
EFIAPI
DxePcdGet8 (
  IN UINTN         TokenNumber
  )
{
  ASSERT (sizeof (UINT8) == DxePcdGetSize (TokenNumber));
  
  return *((UINT8 *) GetWorker (TokenNumber));
}



UINT16
EFIAPI
DxePcdGet16 (
  IN UINTN         TokenNumber
  )
{
  ASSERT (sizeof (UINT16) == DxePcdGetSize (TokenNumber));
  
  return ReadUnaligned16 (GetWorker (TokenNumber));
}



UINT32
EFIAPI
DxePcdGet32 (
  IN UINTN         TokenNumber
  )
{
  ASSERT (sizeof (UINT32) == DxePcdGetSize (TokenNumber));
  
  return ReadUnaligned32 (GetWorker (TokenNumber));
}



UINT64
EFIAPI
DxePcdGet64 (
  IN UINTN          TokenNumber
  )
{
  ASSERT (sizeof (UINT64) == DxePcdGetSize (TokenNumber));
  
  return ReadUnaligned64(GetWorker (TokenNumber));
}



VOID *
EFIAPI
DxePcdGetPtr (
  IN UINTN          TokenNumber
  )
{
  return GetWorker (TokenNumber);
}



BOOLEAN
EFIAPI
DxePcdGetBool (
  IN UINTN          TokenNumber
  )
{
  ASSERT (sizeof (BOOLEAN) == DxePcdGetSize (TokenNumber));
  
  return *((BOOLEAN *) GetWorker (TokenNumber));
}



UINTN
EFIAPI
DxePcdGetSize (
  IN UINTN          TokenNumber
  )
{
  UINT16 * SizeTable;

  SizeTable = (TokenNumber < PEI_LOCAL_TOKEN_NUMBER) ? gPcdDatabase->PeiDb.Init.SizeTable :
                                                    gPcdDatabase->DxeDb.Init.SizeTable;


  TokenNumber = (TokenNumber < PEI_LOCAL_TOKEN_NUMBER) ? TokenNumber : (TokenNumber - PEI_LOCAL_TOKEN_NUMBER);

  return SizeTable[TokenNumber];
}



UINT8
EFIAPI
DxePcdGet8Ex (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber
  )
{
  return *((UINT8 *) ExGetWorker (Guid, ExTokenNumber, sizeof(UINT8)));
}



UINT16
EFIAPI
DxePcdGet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                 ExTokenNumber
  )
{
  return *((UINT16 *) ExGetWorker (Guid, ExTokenNumber, sizeof(UINT16)));
}



UINT32
EFIAPI
DxePcdGet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                  ExTokenNumber
  )
{
  return *((UINT32 *) ExGetWorker (Guid, ExTokenNumber, sizeof(UINT32)));
}



UINT64
EFIAPI
DxePcdGet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                  ExTokenNumber
  )
{
  //
  // BugBug: Must be changed to ReadUnaligned64
  //
  return *((UINT64 *) ExGetWorker (Guid, ExTokenNumber, sizeof(UINT64)));
}



VOID *
EFIAPI
DxePcdGetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                  ExTokenNumber
  )
{
  return ExGetWorker (Guid, ExTokenNumber, 0);
}



BOOLEAN
EFIAPI
DxePcdGetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                  ExTokenNumber
  )
{
  return *((BOOLEAN *) ExGetWorker (Guid, ExTokenNumber, sizeof(BOOLEAN)));
}



UINTN
EFIAPI
DxePcdGetSizeEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                  ExTokenNumber
  )
{
  EX_PCD_ENTRY_ATTRIBUTE    Attr;
  
  GetExPcdTokenAttributes (Guid, ExTokenNumber, &Attr);

  return Attr.Size;
}



EFI_STATUS
EFIAPI
DxePcdSet8 (
  IN UINTN              TokenNumber,
  IN UINT8              Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
DxePcdSet16 (
  IN UINTN              TokenNumber,
  IN UINT16             Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
DxePcdSet32 (
  IN UINTN              TokenNumber,
  IN UINT32             Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
DxePcdSet64 (
  IN UINTN              TokenNumber,
  IN UINT64             Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
DxePcdSetPtr (
  IN UINTN              TokenNumber,
  IN CONST VOID         *Value
  )
{
  //
  // BugBug, please change the Size to Input size when sync with spec
  //
  //ASSERT (sizeof (Value) == DxePcdGetSize (TokenNumber));

  return SetWorker (TokenNumber, (VOID *)Value, DxePcdGetSize (TokenNumber), TRUE);
}



EFI_STATUS
EFIAPI
DxePcdSetBool (
  IN UINTN              TokenNumber,
  IN BOOLEAN            Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
DxePcdSet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                  ExTokenNumber,
  IN UINT8                  Value
  )
{
  return          ExSetWorker(
                              ExTokenNumber, 
                              Guid,
                              &Value, 
                              sizeof (Value), 
                              FALSE
                              );
}



EFI_STATUS
EFIAPI
DxePcdSet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  ExTokenNumber,
  IN UINT16            Value
  )
{
  return          ExSetWorker(
                              ExTokenNumber, 
                              Guid,
                              &Value, 
                              sizeof (Value), 
                              FALSE
                              );
}



EFI_STATUS
EFIAPI
DxePcdSet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  ExTokenNumber,
  IN UINT32             Value
  )
{
  return          ExSetWorker(
                              ExTokenNumber, 
                              Guid,
                              &Value, 
                              sizeof (Value), 
                              FALSE
                              );
}



EFI_STATUS
EFIAPI
DxePcdSet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  ExTokenNumber,
  IN UINT64            Value
  )
{
  return          ExSetWorker(
                              ExTokenNumber, 
                              Guid,
                              &Value, 
                              sizeof (Value), 
                              FALSE
                              );
}



EFI_STATUS
EFIAPI
DxePcdSetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  ExTokenNumber,
  IN CONST VOID        *Value
  )
{
  return          ExSetWorker(
                              ExTokenNumber, 
                              Guid,
                              (VOID *) Value, 
                              sizeof (Value), 
                              TRUE
                              );
}



EFI_STATUS
EFIAPI
DxePcdSetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  ExTokenNumber,
  IN BOOLEAN           Value
  )
{
  return          ExSetWorker(
                              ExTokenNumber, 
                              Guid,
                              &Value, 
                              sizeof (Value), 
                              TRUE
                              );
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

