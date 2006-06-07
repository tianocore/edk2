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

  DxeRegisterCallBackOnSet,
  DxeUnRegisterCallBackOnSet,
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


VOID
EFIAPI
DxePcdSetSku (
  IN  UINTN         SkuId
  )
{
  mPcdDatabase->PeiDb.Init.SystemSkuId = (SKU_ID) SkuId;
  
  return;
}



UINT8
EFIAPI
DxePcdGet8 (
  IN UINTN                    TokenNumber
  )
{
  return *((UINT8 *) GetWorker (TokenNumber, sizeof (UINT8)));
}



UINT16
EFIAPI
DxePcdGet16 (
  IN UINTN                    TokenNumber
  )
{
  return ReadUnaligned16 (GetWorker (TokenNumber, sizeof (UINT16)));
}



UINT32
EFIAPI
DxePcdGet32 (
  IN UINTN                    TokenNumber
  )
{
  return ReadUnaligned32 (GetWorker (TokenNumber, sizeof (UINT32)));
}



UINT64
EFIAPI
DxePcdGet64 (
  IN UINTN                     TokenNumber
  )
{
  return ReadUnaligned64(GetWorker (TokenNumber, sizeof (UINT64)));
}



VOID *
EFIAPI
DxePcdGetPtr (
  IN UINTN                     TokenNumber
  )
{
  return GetWorker (TokenNumber, 0);
}



BOOLEAN
EFIAPI
DxePcdGetBool (
  IN UINTN                     TokenNumber
  )
{
  return *((BOOLEAN *) GetWorker (TokenNumber, sizeof (BOOLEAN)));
}



UINTN
EFIAPI
DxePcdGetSize (
  IN UINTN                     TokenNumber
  )
{
  UINT16 * SizeTable;

  SizeTable = (TokenNumber < PEI_LOCAL_TOKEN_NUMBER) ? mPcdDatabase->PeiDb.Init.SizeTable :
                                                    mPcdDatabase->DxeDb.Init.SizeTable;


  TokenNumber = (TokenNumber < PEI_LOCAL_TOKEN_NUMBER) ? TokenNumber : (TokenNumber - PEI_LOCAL_TOKEN_NUMBER);

  return SizeTable[TokenNumber];
}



UINT8
EFIAPI
DxePcdGet8Ex (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                 ExTokenNumber
  )
{
  return *((UINT8 *) ExGetWorker (Guid, ExTokenNumber, sizeof(UINT8)));
}



UINT16
EFIAPI
DxePcdGet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                ExTokenNumber
  )
{
  return ReadUnaligned16 (ExGetWorker (Guid, ExTokenNumber, sizeof(UINT16)));
}



UINT32
EFIAPI
DxePcdGet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                 ExTokenNumber
  )
{
  return ReadUnaligned32 (ExGetWorker (Guid, ExTokenNumber, sizeof(UINT32)));
}



UINT64
EFIAPI
DxePcdGet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                 ExTokenNumber
  )
{
  return ReadUnaligned64 (ExGetWorker (Guid, ExTokenNumber, sizeof(UINT64)));
}



VOID *
EFIAPI
DxePcdGetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                 ExTokenNumber
  )
{
  return ExGetWorker (Guid, ExTokenNumber, 0);
}



BOOLEAN
EFIAPI
DxePcdGetBoolEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                 ExTokenNumber
  )
{
  return *((BOOLEAN *) ExGetWorker (Guid, ExTokenNumber, sizeof(BOOLEAN)));
}



UINTN
EFIAPI
DxePcdGetSizeEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN                 ExTokenNumber
  )
{
  return DxePcdGetSize(GetExPcdTokenNumber (Guid, ExTokenNumber));
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
  IN UINTN              SizeOfBuffer,
  IN VOID               *Buffer
  )
{
  return SetWorker (TokenNumber, Buffer, SizeOfBuffer, TRUE);
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
  IN CONST EFI_GUID         *Guid,
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
  IN CONST EFI_GUID    *Guid,
  IN UINTN             ExTokenNumber,
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
  IN CONST EFI_GUID     *Guid,
  IN UINTN              ExTokenNumber,
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
  IN CONST EFI_GUID    *Guid,
  IN UINTN             ExTokenNumber,
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
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber,
  IN UINTN                  SizeOfBuffer,
  IN VOID                   *Buffer
  )
{
  return          ExSetWorker(
                              ExTokenNumber, 
                              Guid,
                              Buffer, 
                              SizeOfBuffer, 
                              TRUE
                              );
}



EFI_STATUS
EFIAPI
DxePcdSetBoolEx (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             ExTokenNumber,
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
DxeRegisterCallBackOnSet (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
{
  ASSERT (CallBackFunction != NULL);
  
  return DxeRegisterCallBackWorker (TokenNumber, Guid, CallBackFunction);
}



EFI_STATUS
EFIAPI
DxeUnRegisterCallBackOnSet (
  IN  UINTN                   TokenNumber,
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
{
  ASSERT (CallBackFunction != NULL);
  
  return DxeRegisterCallBackWorker (TokenNumber, Guid, CallBackFunction);
}



EFI_STATUS
EFIAPI
DxePcdGetNextToken (
  IN CONST EFI_GUID         *Guid, OPTIONAL
  IN OUT   UINTN            *TokenNumber
  )
{
  UINTN               ExTokenNumber;
  
  //
  // Scan the local token space
  //
  if (Guid == NULL) {
    (*TokenNumber)++;
    if (*TokenNumber == PCD_INVALID_TOKEN_NUMBER) {
      return EFI_SUCCESS;
    } else {
      if (*TokenNumber >= PEI_NEX_TOKEN_NUMBER &&
          *TokenNumber < PEI_LOCAL_TOKEN_NUMBER) {
        //
        // The first Non-Ex type Token Number for DXE PCD 
        // database is PEI_LOCAL_TOKEN_NUMBER
        //
        *TokenNumber = PEI_LOCAL_TOKEN_NUMBER;
        return EFI_SUCCESS;
      } else if (*TokenNumber >= DXE_NEX_TOKEN_NUMBER + PEI_LOCAL_TOKEN_NUMBER) {
        *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
        return EFI_SUCCESS;
      }
    }
  }

  if (PEI_EXMAP_TABLE_EMPTY && PEI_EXMAP_TABLE_EMPTY) {
    *TokenNumber = (UINTN) PCD_INVALID_TOKEN_NUMBER;
    return EFI_NOT_FOUND;
  }

  ExTokenNumber = *TokenNumber;
  if (!PEI_EXMAP_TABLE_EMPTY) {
    ExTokenNumber = ExGetNextTokeNumber (
                        Guid,
                        ExTokenNumber,
                        mPcdDatabase->PeiDb.Init.GuidTable,
                        sizeof(mPcdDatabase->PeiDb.Init.GuidTable),
                        mPcdDatabase->PeiDb.Init.ExMapTable,
                        sizeof(mPcdDatabase->PeiDb.Init.ExMapTable)
                        );
  }

  if (!DXE_EXMAP_TABLE_EMPTY) {
    ExTokenNumber = ExGetNextTokeNumber (
                        Guid,
                        ExTokenNumber,
                        mPcdDatabase->PeiDb.Init.GuidTable,
                        sizeof(mPcdDatabase->PeiDb.Init.GuidTable),
                        mPcdDatabase->PeiDb.Init.ExMapTable,
                        sizeof(mPcdDatabase->PeiDb.Init.ExMapTable)
                        );
  }

  *TokenNumber = ExTokenNumber;

  return EFI_SUCCESS;
}

