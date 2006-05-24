/** @file PCD PEIM

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


PCD_PPI mPcdPpiInstance = {
  PeiPcdSetSku,

  PeiPcdGet8,
  PeiPcdGet16,          
  PeiPcdGet32,          
  PeiPcdGet64,          
  PeiPcdGetPtr,         
  PeiPcdGetBool,     
  PeiPcdGetSize,

  PeiPcdGet8Ex,
  PeiPcdGet16Ex,          
  PeiPcdGet32Ex,          
  PeiPcdGet64Ex,          
  PeiPcdGetPtrEx,         
  PeiPcdGetBoolEx,     
  PeiPcdGetSizeEx,
  
  PeiPcdSet8,
  PeiPcdSet16,          
  PeiPcdSet32,          
  PeiPcdSet64,          
  PeiPcdSetPtr,         
  PeiPcdSetBool,     

  PeiPcdSet8Ex,
  PeiPcdSet16Ex,          
  PeiPcdSet32Ex,          
  PeiPcdSet64Ex,          
  PeiPcdSetPtrEx,         
  PeiPcdSetBoolEx,

  PcdRegisterCallBackOnSet,
  PcdUnRegisterCallBackOnSet,
  PeiPcdGetNextToken
};



STATIC EFI_PEI_PPI_DESCRIPTOR  mPpiPCD = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPcdPpiGuid,
  &mPcdPpiInstance
};



EFI_STATUS
EFIAPI
PcdPeimInit (
  IN EFI_FFS_FILE_HEADER      *FfsHeader,
  IN EFI_PEI_SERVICES         **PeiServices
  )
{
  EFI_STATUS Status;

  BuildPcdDatabase ();
  
  Status = PeiCoreInstallPpi (&mPpiPCD);

  ASSERT_EFI_ERROR (Status);
  
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PeiPcdSetSku (
  IN  UINTN                  SkuId
  )
{

  GetPcdDatabase()->Init.SystemSkuId = (SKU_ID) SkuId;

  return  EFI_SUCCESS;
}



UINT8
EFIAPI
PeiPcdGet8 (
  IN UINTN  TokenNumber
  )
{
  return *((UINT8 *) GetWorker (TokenNumber, sizeof (UINT8)));
}



UINT16
EFIAPI
PeiPcdGet16 (
  IN UINTN  TokenNumber
  )
{
  return ReadUnaligned16 (GetWorker (TokenNumber, sizeof (UINT16)));
}



UINT32
EFIAPI
PeiPcdGet32 (
  IN UINTN  TokenNumber
  )
{
  return ReadUnaligned32 (GetWorker (TokenNumber, sizeof (UINT32)));
}



UINT64
EFIAPI
PeiPcdGet64 (
  IN UINTN  TokenNumber
  )
{
  return ReadUnaligned64 (GetWorker (TokenNumber, sizeof (UINT64)));
}



VOID *
EFIAPI
PeiPcdGetPtr (
  IN UINTN  TokenNumber
  )
{
  return GetWorker (TokenNumber, 0);
}



BOOLEAN
EFIAPI
PeiPcdGetBool (
  IN UINTN  TokenNumber
  )
{
  return *((BOOLEAN *) GetWorker (TokenNumber, sizeof (BOOLEAN)));
}



UINTN
EFIAPI
PeiPcdGetSize (
  IN UINTN  TokenNumber
  )
{
  ASSERT (TokenNumber < PEI_LOCAL_TOKEN_NUMBER);

  return GetPcdDatabase()->Init.SizeTable[TokenNumber];
}



UINT8
EFIAPI
PeiPcdGet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  ExTokenNumber
  )
{
  return *((UINT8 *) ExGetWorker (Guid, ExTokenNumber, sizeof (UINT8)));
}



UINT16
EFIAPI
PeiPcdGet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  ExTokenNumber
  )
{
  return ReadUnaligned16 (ExGetWorker (Guid, ExTokenNumber, sizeof (UINT16)));
}



UINT32
EFIAPI
PeiPcdGet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  ExTokenNumber
  )
{
  return ReadUnaligned32 (ExGetWorker (Guid, ExTokenNumber, sizeof (UINT32)));
}



UINT64
EFIAPI
PeiPcdGet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  ExTokenNumber
  )
{
  return ReadUnaligned64 (ExGetWorker (Guid, ExTokenNumber, sizeof (UINT64)));
}



VOID *
EFIAPI
PeiPcdGetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  ExTokenNumber
  )
{
  return ExGetWorker (Guid, ExTokenNumber, 0);
}



BOOLEAN
EFIAPI
PeiPcdGetBoolEx (
  IN CONST  EFI_GUID        *Guid,
  IN UINTN                  ExTokenNumber
  )
{
  return *((BOOLEAN *) ExGetWorker (Guid, ExTokenNumber, sizeof (BOOLEAN)));
}



UINTN
EFIAPI
PeiPcdGetSizeEx (
  IN CONST  EFI_GUID        *Guid,
  IN UINTN                  ExTokenNumber
  )
{
  EX_PCD_ENTRY_ATTRIBUTE      Attr;

  GetExPcdTokenAttributes (Guid, ExTokenNumber, &Attr);
  
  return Attr.Size;
}



EFI_STATUS
EFIAPI
PeiPcdSet8 (
  IN UINTN             TokenNumber,
  IN UINT8             Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
PeiPcdSet16 (
  IN UINTN              TokenNumber,
  IN UINT16             Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
PeiPcdSet32 (
  IN UINTN              TokenNumber,
  IN UINT32             Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
PeiPcdSet64 (
  IN UINTN              TokenNumber,
  IN UINT64             Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}


EFI_STATUS
EFIAPI
PeiPcdSetPtr (
  IN UINTN              TokenNumber,
  IN CONST VOID         *Value
  )
{
  //
  // BugBug, please change the Size to Input size when sync with spec
  //
  //ASSERT (sizeof (Value) == GetPcdDatabase()->Init.SizeTable[TokenNumber]);

  return SetWorker (TokenNumber, (VOID *) Value, GetPcdDatabase()->Init.SizeTable[TokenNumber], TRUE);
}



EFI_STATUS
EFIAPI
PeiPcdSetBool (
  IN UINTN              TokenNumber,
  IN BOOLEAN            Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
PeiPcdSet8Ex (
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
PeiPcdSet16Ex (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber,
  IN UINT16                 Value
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
PeiPcdSet32Ex (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber,
  IN UINT32                 Value
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
PeiPcdSet64Ex (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber,
  IN UINT64                 Value
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
PeiPcdSetPtrEx (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber,
  IN CONST VOID             *Value
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
PeiPcdSetBoolEx (
  IN CONST EFI_GUID       *Guid,
  IN UINTN                ExTokenNumber,
  IN BOOLEAN              Value
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
PcdRegisterCallBackOnSet (
  IN  UINTN                       ExTokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK            CallBackFunction
  )
{
  return PeiRegisterCallBackWorker (ExTokenNumber, Guid, CallBackFunction, TRUE);
}



EFI_STATUS
EFIAPI
PcdUnRegisterCallBackOnSet (
  IN  UINTN                       ExTokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK            CallBackFunction
  )
{
  return PeiRegisterCallBackWorker (ExTokenNumber, Guid, CallBackFunction, FALSE);
}



EFI_STATUS
EFIAPI
PeiPcdGetNextToken (
  IN CONST EFI_GUID               *Guid, OPTIONAL
  IN OUT  UINTN                   *TokenNumber
  )
{
  if (Guid == NULL) {
    *TokenNumber++;

    if (*TokenNumber >= PEI_LOCAL_TOKEN_NUMBER) {
      *TokenNumber = 0;
    }
  }

  //
  // BugBug: Haven't implemented the portion to get Next Token for GuidSpace is not Local GuidSpace.
  //

  return EFI_SUCCESS;
}


