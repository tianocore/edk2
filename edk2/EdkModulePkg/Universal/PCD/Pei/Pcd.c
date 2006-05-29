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

  PeiRegisterCallBackOnSet,
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

VOID
EFIAPI
PeiPcdSetSku (
  IN  SKU_ID                  SkuId
  )
{

  GetPcdDatabase()->Init.SystemSkuId = SkuId;

  return;
}



UINT8
EFIAPI
PeiPcdGet8 (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
{
  return *((UINT8 *) GetWorker (TokenNumber, sizeof (UINT8)));
}



UINT16
EFIAPI
PeiPcdGet16 (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
{
  return ReadUnaligned16 (GetWorker (TokenNumber, sizeof (UINT16)));
}



UINT32
EFIAPI
PeiPcdGet32 (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
{
  return ReadUnaligned32 (GetWorker (TokenNumber, sizeof (UINT32)));
}



UINT64
EFIAPI
PeiPcdGet64 (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
{
  return ReadUnaligned64 (GetWorker (TokenNumber, sizeof (UINT64)));
}



VOID *
EFIAPI
PeiPcdGetPtr (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
{
  return GetWorker (TokenNumber, 0);
}



BOOLEAN
EFIAPI
PeiPcdGetBool (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
{
  return *((BOOLEAN *) GetWorker (TokenNumber, sizeof (BOOLEAN)));
}



UINTN
EFIAPI
PeiPcdGetSize (
  IN PCD_TOKEN_NUMBER  TokenNumber
  )
{
  ASSERT (TokenNumber < PEI_LOCAL_TOKEN_NUMBER);

  return GetPcdDatabase()->Init.SizeTable[TokenNumber];
}



UINT8
EFIAPI
PeiPcdGet8Ex (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  ExTokenNumber
  )
{
  return *((UINT8 *) ExGetWorker (Guid, ExTokenNumber, sizeof (UINT8)));
}



UINT16
EFIAPI
PeiPcdGet16Ex (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  ExTokenNumber
  )
{
  return ReadUnaligned16 (ExGetWorker (Guid, ExTokenNumber, sizeof (UINT16)));
}



UINT32
EFIAPI
PeiPcdGet32Ex (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  ExTokenNumber
  )
{
  return ReadUnaligned32 (ExGetWorker (Guid, ExTokenNumber, sizeof (UINT32)));
}



UINT64
EFIAPI
PeiPcdGet64Ex (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  ExTokenNumber
  )
{
  return ReadUnaligned64 (ExGetWorker (Guid, ExTokenNumber, sizeof (UINT64)));
}



VOID *
EFIAPI
PeiPcdGetPtrEx (
  IN CONST EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER  ExTokenNumber
  )
{
  return ExGetWorker (Guid, ExTokenNumber, 0);
}



BOOLEAN
EFIAPI
PeiPcdGetBoolEx (
  IN CONST  EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER                  ExTokenNumber
  )
{
  return *((BOOLEAN *) ExGetWorker (Guid, ExTokenNumber, sizeof (BOOLEAN)));
}



UINTN
EFIAPI
PeiPcdGetSizeEx (
  IN CONST  EFI_GUID        *Guid,
  IN PCD_TOKEN_NUMBER                  ExTokenNumber
  )
{
  EX_PCD_ENTRY_ATTRIBUTE      Attr;

  GetExPcdTokenAttributes (Guid, ExTokenNumber, &Attr);
  
  return Attr.Size;
}



EFI_STATUS
EFIAPI
PeiPcdSet8 (
  IN PCD_TOKEN_NUMBER             TokenNumber,
  IN UINT8             Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
PeiPcdSet16 (
  IN PCD_TOKEN_NUMBER              TokenNumber,
  IN UINT16             Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
PeiPcdSet32 (
  IN PCD_TOKEN_NUMBER              TokenNumber,
  IN UINT32             Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
PeiPcdSet64 (
  IN PCD_TOKEN_NUMBER              TokenNumber,
  IN UINT64             Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}


EFI_STATUS
EFIAPI
PeiPcdSetPtr (
  IN PCD_TOKEN_NUMBER              TokenNumber,
  IN UINTN                         SizeOfBuffer,
  IN VOID                          *Buffer
  )
{
  return SetWorker (TokenNumber, Buffer, SizeOfBuffer, TRUE);
}



EFI_STATUS
EFIAPI
PeiPcdSetBool (
  IN PCD_TOKEN_NUMBER              TokenNumber,
  IN BOOLEAN            Value
  )
{
  return SetWorker (TokenNumber, &Value, sizeof (Value), FALSE);
}



EFI_STATUS
EFIAPI
PeiPcdSet8Ex (
  IN CONST EFI_GUID         *Guid,
  IN PCD_TOKEN_NUMBER       ExTokenNumber,
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
  IN PCD_TOKEN_NUMBER       ExTokenNumber,
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
  IN PCD_TOKEN_NUMBER       ExTokenNumber,
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
  IN PCD_TOKEN_NUMBER       ExTokenNumber,
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
  IN PCD_TOKEN_NUMBER       ExTokenNumber,
  IN UINTN                  SizeOfBuffer,
  IN VOID                   *Value
  )
{
  return          ExSetWorker(
                              ExTokenNumber, 
                              Guid,
                              Value, 
                              SizeOfBuffer, 
                              TRUE
                              );
}



EFI_STATUS
EFIAPI
PeiPcdSetBoolEx (
  IN CONST EFI_GUID       *Guid,
  IN PCD_TOKEN_NUMBER     ExTokenNumber,
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
PeiRegisterCallBackOnSet (
  IN  PCD_TOKEN_NUMBER            ExTokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK            CallBackFunction
  )
{
  ASSERT (CallBackFunction != NULL);
  
  return PeiRegisterCallBackWorker (ExTokenNumber, Guid, CallBackFunction, TRUE);
}



EFI_STATUS
EFIAPI
PcdUnRegisterCallBackOnSet (
  IN  PCD_TOKEN_NUMBER            ExTokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK            CallBackFunction
  )
{
  ASSERT (CallBackFunction != NULL);
  
  return PeiRegisterCallBackWorker (ExTokenNumber, Guid, CallBackFunction, FALSE);
}



EFI_STATUS
EFIAPI
PeiPcdGetNextToken (
  IN CONST EFI_GUID               *Guid, OPTIONAL
  IN OUT  PCD_TOKEN_NUMBER        *TokenNumber
  )
{
  UINTN               GuidTableIdx;
  PEI_PCD_DATABASE    *PeiPcdDb;
  EFI_GUID            *MatchGuid;
  DYNAMICEX_MAPPING   *ExMapTable;
  UINTN               i;
  BOOLEAN             Found;
    
  if (Guid == NULL) {
    (*TokenNumber)++;

    if (*TokenNumber >= PEI_NEX_TOKEN_NUMBER) {
      *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
    }
    
  } else {

    if (PEI_EXMAP_TABLE_EMPTY) {
      *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
      return EFI_NOT_FOUND;
    }
    
    //
    // Assume PCD Database AutoGen tool is sorting the ExMap based on the following order
    // 1) ExGuid
    // 2) ExTokenNumber
    //
    PeiPcdDb = GetPcdDatabase ();
    
    MatchGuid = ScanGuid (PeiPcdDb->Init.GuidTable, sizeof(PeiPcdDb->Init.GuidTable), Guid);

    if (MatchGuid == NULL) {
      *TokenNumber = (UINTN) PCD_INVALID_TOKEN_NUMBER;
      return EFI_NOT_FOUND;
    }

    GuidTableIdx = MatchGuid - PeiPcdDb->Init.GuidTable;

    ExMapTable = PeiPcdDb->Init.ExMapTable;

    Found = FALSE;
    for (i = 0; i < PEI_EXMAPPING_TABLE_SIZE; i++) {
      if (ExMapTable[i].ExGuidIndex == GuidTableIdx) {
        Found = TRUE;
        break;
      }
    }

    if (Found) {
      if (*TokenNumber == PCD_INVALID_TOKEN_NUMBER) {
        *TokenNumber = ExMapTable[i].ExTokenNumber;
         return EFI_SUCCESS;
      }
      
      for ( ; ExMapTable[i].ExGuidIndex == GuidTableIdx; i++) {
        if (ExMapTable[i].ExTokenNumber == *TokenNumber) {
          i++;
          if (ExMapTable[i].ExGuidIndex == GuidTableIdx) {
            *TokenNumber = ExMapTable[i].ExTokenNumber;
            return EFI_SUCCESS;
          } else {
            *TokenNumber = (UINTN) PCD_INVALID_TOKEN_NUMBER;
            return EFI_SUCCESS;
          }
        }
      }

      return EFI_NOT_FOUND;
    }
    
  }

  return EFI_SUCCESS;
}

EFI_GUID *
EFIAPI
PeiPcdGetNextTokenSpaceGuid (
  IN CONST EFI_GUID               *Guid
  )
{
  UINTN               GuidTableIdx;
  EFI_GUID            *MatchGuid;
  PEI_PCD_DATABASE    *PeiPcdDb;
  DYNAMICEX_MAPPING   *ExMapTable;
  UINTN               i;
  BOOLEAN             Found;

  if (PEI_EXMAP_TABLE_EMPTY) {
    return NULL;
  }

  //
  // Assume PCD Database AutoGen tool is sorting the ExMap based on the following order
  // 1) ExGuid
  // 2) ExTokenNumber
  //
  PeiPcdDb = GetPcdDatabase ();

  MatchGuid = ScanGuid (PeiPcdDb->Init.GuidTable, sizeof(PeiPcdDb->Init.GuidTable), Guid);

  if (MatchGuid == NULL) {
    return NULL;
  }
  
  GuidTableIdx = MatchGuid - PeiPcdDb->Init.GuidTable;

  ExMapTable = PeiPcdDb->Init.ExMapTable;

  Found = FALSE;
  for (i = 0; i < PEI_EXMAPPING_TABLE_SIZE; i++) {
    if (ExMapTable[i].ExGuidIndex == GuidTableIdx) {
      Found = TRUE;
      break;
    }
  }

  if (Found) {
    for ( ; i < PEI_EXMAPPING_TABLE_SIZE; i++ ) {
      if (ExMapTable[i].ExGuidIndex != GuidTableIdx ) {
        if (i < PEI_EXMAPPING_TABLE_SIZE) {
          return &PeiPcdDb->Init.GuidTable[ExMapTable[i].ExGuidIndex];
        } else {
          return NULL;
        }
      }
    }
  }

  return NULL;

}

