/** @file
PCD PEIM

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
  UINT8      *PcdImage;

  PcdImage = (UINT8 *) LocatePcdImage ();

  BuildPcdDatabase (PcdImage);

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
  PCD_DATABASE      *Database;
  EFI_HOB_GUID_TYPE *GuidHob;

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  ASSERT (GuidHob != NULL);
  
  Database = (PCD_DATABASE *) GET_GUID_HOB_DATA (GuidHob);

  Database->Info.SkuId = SkuId;

  return SkuId;
}



UINT8
EFIAPI
PeiPcdGet8 (
  IN UINTN  TokenNumber
  )
{
  return PeiPcdGet8Ex (NULL, TokenNumber);
}



UINT16
EFIAPI
PeiPcdGet16 (
  IN UINTN  TokenNumber
  )
{
  return PeiPcdGet16Ex (NULL, TokenNumber);
}



UINT32
EFIAPI
PeiPcdGet32 (
  IN UINTN  TokenNumber
  )
{
  return PeiPcdGet32Ex (NULL, TokenNumber);
}



UINT64
EFIAPI
PeiPcdGet64 (
  IN UINTN  TokenNumber
  )
{
  return PeiPcdGet64Ex (NULL, TokenNumber);
}



VOID *
EFIAPI
PeiPcdGetPtr (
  IN UINTN  TokenNumber
  )
{
  return PeiPcdGetPtrEx (NULL, TokenNumber);
}



BOOLEAN
EFIAPI
PeiPcdGetBool (
  IN UINTN  TokenNumber
  )
{
  return PeiPcdGetBoolEx (NULL, TokenNumber);
}



UINTN
EFIAPI
PeiPcdGetSize (
  IN UINTN  TokenNumber
  )
{
  return PeiPcdGetSizeEx (NULL, TokenNumber);
}



UINT8
EFIAPI
PeiPcdGet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
{
  UINT8 Data;
  
  PeiGetPcdEntryWorker (TokenNumber, Guid, PcdByte8, &Data);
  
  return Data;
}



UINT16
EFIAPI
PeiPcdGet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
{
  UINT16 Data;
  
  PeiGetPcdEntryWorker (TokenNumber, Guid, PcdByte16, &Data);
  
  return Data;
}



UINT32
EFIAPI
PeiPcdGet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
{
  UINT32 Data;
  
  PeiGetPcdEntryWorker (TokenNumber, Guid, PcdByte32, &Data);
  
  return Data;
}



UINT64
EFIAPI
PeiPcdGet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
{
  UINT64 Data;
  
  PeiGetPcdEntryWorker (TokenNumber, Guid, PcdByte64, &Data);
  
  return Data;
}



VOID *
EFIAPI
PeiPcdGetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN UINTN  TokenNumber
  )
{
  VOID *Data;
  
  PeiGetPcdEntryWorker (TokenNumber, Guid, PcdPointer, &Data);
  
  return Data;
}



BOOLEAN
EFIAPI
PeiPcdGetBoolEx (
  IN CONST  EFI_GUID        *Guid,
  IN UINTN                  TokenNumber
  )
{
  BOOLEAN Data;
  
  PeiGetPcdEntryWorker (TokenNumber, Guid, PcdBoolean, &Data);
  
  return Data;
}



UINTN
EFIAPI
PeiPcdGetSizeEx (
  IN CONST  EFI_GUID        *Guid,
  IN UINTN                  TokenNumber
  )
{
  return PeiGetPcdEntrySizeWorker (TokenNumber, Guid);
}



EFI_STATUS
EFIAPI
PeiPcdSet8 (
  IN UINTN             TokenNumber,
  IN UINT8             Value
  )
{
  return PeiPcdSet8Ex (NULL, TokenNumber, Value);
}



EFI_STATUS
EFIAPI
PeiPcdSet16 (
  IN UINTN              TokenNumber,
  IN UINT16             Value
  )
{
  return PeiPcdSet16Ex (NULL, TokenNumber, Value);
}



EFI_STATUS
EFIAPI
PeiPcdSet32 (
  IN UINTN              TokenNumber,
  IN UINT32             Value
  )
{
  return PeiPcdSet32Ex (NULL, TokenNumber, Value);
}



EFI_STATUS
EFIAPI
PeiPcdSet64 (
  IN UINTN              TokenNumber,
  IN UINT64             Value
  )
{
  return PeiPcdSet64Ex (NULL, TokenNumber, Value);
}


EFI_STATUS
EFIAPI
PeiPcdSetPtr (
  IN UINTN              TokenNumber,
  IN CONST VOID         *Value
  )
{
  return PeiPcdSetPtrEx (NULL, TokenNumber, Value);
}



EFI_STATUS
EFIAPI
PeiPcdSetBool (
  IN UINTN              TokenNumber,
  IN BOOLEAN            Value
  )
{
  return PeiPcdSetBoolEx (NULL, TokenNumber, Value);
}



EFI_STATUS
EFIAPI
PeiPcdSet8Ex (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  TokenNumber,
  IN UINT8                  Value
  )
{
  return PeiSetPcdEntryWorker (TokenNumber, Guid, PcdByte8, &Value);
}



EFI_STATUS
EFIAPI
PeiPcdSet16Ex (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  TokenNumber,
  IN UINT16                 Value
  )
{
  return PeiSetPcdEntryWorker (TokenNumber, Guid, PcdByte16, &Value);
}



EFI_STATUS
EFIAPI
PeiPcdSet32Ex (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  TokenNumber,
  IN UINT32                 Value
  )
{
  return PeiSetPcdEntryWorker (TokenNumber, Guid, PcdByte32, &Value);
}



EFI_STATUS
EFIAPI
PeiPcdSet64Ex (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  TokenNumber,
  IN UINT64                 Value
  )
{
  return PeiSetPcdEntryWorker (TokenNumber, Guid, PcdByte64, &Value);
}



EFI_STATUS
EFIAPI
PeiPcdSetPtrEx (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  TokenNumber,
  IN CONST VOID             *Value
  )
{
  return PeiSetPcdEntryWorker (TokenNumber, Guid, PcdPointer, (VOID *)Value);
}



EFI_STATUS
EFIAPI
PeiPcdSetBoolEx (
  IN CONST EFI_GUID       *Guid,
  IN UINTN                TokenNumber,
  IN BOOLEAN              Value
  )
{
  return PeiSetPcdEntryWorker (TokenNumber, Guid, PcdBoolean, &Value);

}




EFI_STATUS
EFIAPI
PcdRegisterCallBackOnSet (
  IN  UINTN                       TokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK            CallBackFunction
  )
{
  return PeiRegisterCallBackWorker (TokenNumber, Guid, CallBackFunction, TRUE);
}



EFI_STATUS
EFIAPI
PcdUnRegisterCallBackOnSet (
  IN  UINTN                       TokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK            CallBackFunction
  )
{
  return PeiRegisterCallBackWorker (TokenNumber, Guid, CallBackFunction, FALSE);
}



EFI_STATUS
EFIAPI
PeiPcdGetNextToken (
  IN CONST EFI_GUID               *Guid, OPTIONAL
  IN OUT  UINTN                   *TokenNumber
  )
{
  return PeiGetNextTokenWorker (TokenNumber, Guid);
}


