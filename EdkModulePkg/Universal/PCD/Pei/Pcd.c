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
  PeiPcdGetNextToken,
  PeiPcdGetNextTokenSpace
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
  
  Status = PeiServicesInstallPpi (&mPpiPCD);

  ASSERT_EFI_ERROR (Status);
  
  return EFI_SUCCESS;
}

VOID
EFIAPI
PeiPcdSetSku (
  IN  UINTN                  SkuId
  )
{

  GetPcdDatabase()->Init.SystemSkuId = (SKU_ID) SkuId;

  return;
}



UINT8
EFIAPI
PeiPcdGet8 (
  IN UINTN                    TokenNumber
  )
{
  return *((UINT8 *) GetWorker (TokenNumber, sizeof (UINT8)));
}



UINT16
EFIAPI
PeiPcdGet16 (
  IN UINTN                    TokenNumber
  )
{
  return ReadUnaligned16 (GetWorker (TokenNumber, sizeof (UINT16)));
}



UINT32
EFIAPI
PeiPcdGet32 (
  IN UINTN                    TokenNumber
  )
{
  return ReadUnaligned32 (GetWorker (TokenNumber, sizeof (UINT32)));
}



UINT64
EFIAPI
PeiPcdGet64 (
  IN UINTN                    TokenNumber
  )
{
  return ReadUnaligned64 (GetWorker (TokenNumber, sizeof (UINT64)));
}



VOID *
EFIAPI
PeiPcdGetPtr (
  IN UINTN                    TokenNumber
  )
{
  return GetWorker (TokenNumber, 0);
}



BOOLEAN
EFIAPI
PeiPcdGetBool (
  IN UINTN                    TokenNumber
  )
{
  return *((BOOLEAN *) GetWorker (TokenNumber, sizeof (BOOLEAN)));
}



UINTN
EFIAPI
PeiPcdGetSize (
  IN UINTN                    TokenNumber
  )
{
  PEI_PCD_DATABASE    *PeiPcdDb;
  UINTN               Size;
  UINTN               MaxSize;

  //
  // If DebugAssertEnabled is TRUE, we still need to provide the GET size
  // function as GetWorker and SetWoker need this function to do ASSERT.
  //
  if ((!FeaturePcdGet(PcdPeiPcdDatabaseGetSizeEnabled)) &&
      (!DebugAssertEnabled ())) {
    return 0;
  }

  PeiPcdDb = GetPcdDatabase ();
  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  ASSERT (TokenNumber + 1 < PEI_LOCAL_TOKEN_NUMBER + 1);

  Size = (PeiPcdDb->Init.LocalTokenNumberTable[TokenNumber] & PCD_DATUM_TYPE_ALL_SET) >> PCD_DATUM_TYPE_SHIFT;

  if (Size == 0) {
    //
    // For pointer type, we need to scan the SIZE_TABLE to get the current size.
    //
    return GetPtrTypeSize (TokenNumber, &MaxSize, PeiPcdDb);
  } else {
    return Size;
  }

}



UINT8
EFIAPI
PeiPcdGet8Ex (
  IN CONST EFI_GUID             *Guid,
  IN UINTN                      ExTokenNumber
  )
{
  return *((UINT8 *) ExGetWorker (Guid, ExTokenNumber, sizeof (UINT8)));
}



UINT16
EFIAPI
PeiPcdGet16Ex (
  IN CONST EFI_GUID             *Guid,
  IN UINTN                      ExTokenNumber
  )
{
  return ReadUnaligned16 (ExGetWorker (Guid, ExTokenNumber, sizeof (UINT16)));
}



UINT32
EFIAPI
PeiPcdGet32Ex (
  IN CONST EFI_GUID             *Guid,
  IN UINTN                      ExTokenNumber
  )
{
  return ReadUnaligned32 (ExGetWorker (Guid, ExTokenNumber, sizeof (UINT32)));
}



UINT64
EFIAPI
PeiPcdGet64Ex (
  IN CONST EFI_GUID             *Guid,
  IN UINTN                      ExTokenNumber
  )
{
  return ReadUnaligned64 (ExGetWorker (Guid, ExTokenNumber, sizeof (UINT64)));
}



VOID *
EFIAPI
PeiPcdGetPtrEx (
  IN CONST EFI_GUID             *Guid,
  IN UINTN                      ExTokenNumber
  )
{
  return ExGetWorker (Guid, ExTokenNumber, 0);
}



BOOLEAN
EFIAPI
PeiPcdGetBoolEx (
  IN CONST  EFI_GUID              *Guid,
  IN UINTN                        ExTokenNumber
  )
{
  return *((BOOLEAN *) ExGetWorker (Guid, ExTokenNumber, sizeof (BOOLEAN)));
}



UINTN
EFIAPI
PeiPcdGetSizeEx (
  IN CONST  EFI_GUID              *Guid,
  IN UINTN                        ExTokenNumber
  )
{
  if ((!FeaturePcdGet (PcdPeiPcdDatabaseGetSizeEnabled)) ||  !FeaturePcdGet (PcdPeiPcdDatabaseExEnabled)) {
    return 0;
  }

  return PeiPcdGetSize (GetExPcdTokenNumber (Guid, ExTokenNumber));
}



EFI_STATUS
EFIAPI
PeiPcdSet8 (
  IN UINTN                        TokenNumber,
  IN UINT8                        Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
PeiPcdSet16 (
  IN UINTN                         TokenNumber,
  IN UINT16                        Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
PeiPcdSet32 (
  IN UINTN                         TokenNumber,
  IN UINT32                        Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
PeiPcdSet64 (
  IN UINTN                         TokenNumber,
  IN UINT64                        Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}


EFI_STATUS
EFIAPI
PeiPcdSetPtr (
  IN      UINTN                    TokenNumber,
  IN OUT  UINTN                    *SizeOfBuffer,
  IN      VOID                     *Buffer
  )
{
  return SetWorker (TokenNumber, Buffer, SizeOfBuffer, TRUE);
}



EFI_STATUS
EFIAPI
PeiPcdSetBool (
  IN UINTN                         TokenNumber,
  IN BOOLEAN                       Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
PeiPcdSet8Ex (
  IN CONST EFI_GUID               *Guid,
  IN UINTN                        ExTokenNumber,
  IN UINT8                        Value
  )
{
  return ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
PeiPcdSet16Ex (
  IN CONST EFI_GUID               *Guid,
  IN UINTN                        ExTokenNumber,
  IN UINT16                       Value
  )
{
  return ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
PeiPcdSet32Ex (
  IN CONST EFI_GUID               *Guid,
  IN UINTN                        ExTokenNumber,
  IN UINT32                       Value
  )
{
  return ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
PeiPcdSet64Ex (
  IN CONST EFI_GUID               *Guid,
  IN UINTN                        ExTokenNumber,
  IN UINT64                       Value
  )
{
  return ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
PeiPcdSetPtrEx (
  IN CONST EFI_GUID               *Guid,
  IN UINTN                        ExTokenNumber,
  IN UINTN                        *SizeOfBuffer,
  IN VOID                         *Value
  )
{
  return ExSetWorker (ExTokenNumber, Guid, Value, SizeOfBuffer, TRUE);
}



EFI_STATUS
EFIAPI
PeiPcdSetBoolEx (
  IN CONST EFI_GUID             *Guid,
  IN UINTN                      ExTokenNumber,
  IN BOOLEAN                    Value
  )
{
  return ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}




EFI_STATUS
EFIAPI
PeiRegisterCallBackOnSet (
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  UINTN                       ExTokenNumber,
  IN  PCD_PPI_CALLBACK            CallBackFunction
  )
{
  if (!FeaturePcdGet(PcdPeiPcdDatabaseCallbackOnSetEnabled)) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (CallBackFunction != NULL);
  
  return PeiRegisterCallBackWorker (ExTokenNumber, Guid, CallBackFunction, TRUE);
}



EFI_STATUS
EFIAPI
PcdUnRegisterCallBackOnSet (
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  UINTN                       ExTokenNumber,
  IN  PCD_PPI_CALLBACK            CallBackFunction
  )
{
  if (!FeaturePcdGet(PcdPeiPcdDatabaseCallbackOnSetEnabled)) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (CallBackFunction != NULL);
  
  return PeiRegisterCallBackWorker (ExTokenNumber, Guid, CallBackFunction, FALSE);
}



EFI_STATUS
EFIAPI
PeiPcdGetNextToken (
  IN CONST EFI_GUID               *Guid, OPTIONAL
  IN OUT  UINTN                   *TokenNumber
  )
{
  UINTN               GuidTableIdx;
  PEI_PCD_DATABASE    *PeiPcdDb;
  EFI_GUID            *MatchGuid;
  DYNAMICEX_MAPPING   *ExMapTable;
  UINTN               i;
  BOOLEAN             Found;
  BOOLEAN             PeiExMapTableEmpty;

  if (!FeaturePcdGet (PcdPeiPcdDatabaseTraverseEnabled)) {
    return EFI_UNSUPPORTED;
  }

  PeiExMapTableEmpty = PEI_EXMAP_TABLE_EMPTY;

  if (Guid == NULL) {
    if (*TokenNumber > PEI_NEX_TOKEN_NUMBER) {
      return EFI_NOT_FOUND;
    }
    (*TokenNumber)++;
    if (*TokenNumber > PEI_NEX_TOKEN_NUMBER) {
      *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
    }
    return EFI_SUCCESS;
  } else {
    if (PeiExMapTableEmpty) {
      *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
      return EFI_SUCCESS;
    }
    
    //
    // Assume PCD Database AutoGen tool is sorting the ExMap based on the following order
    // 1) ExGuid
    // 2) ExTokenNumber
    //
    PeiPcdDb = GetPcdDatabase ();
    
    MatchGuid = ScanGuid (PeiPcdDb->Init.GuidTable, sizeof(PeiPcdDb->Init.GuidTable), Guid);

    if (MatchGuid == NULL) {
      *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
      return EFI_NOT_FOUND;
    }

    GuidTableIdx = MatchGuid - PeiPcdDb->Init.GuidTable;

    ExMapTable = PeiPcdDb->Init.ExMapTable;

    Found = FALSE;
    //
    // Locate the GUID in ExMapTable first.
    //
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

      for ( ; i < PEI_EXMAPPING_TABLE_SIZE; i++) {
        if (ExMapTable[i].ExTokenNumber == *TokenNumber) {
          i++;
          if (i == PEI_EXMAPPING_TABLE_SIZE) {
            //
            // Exceed the length of ExMap Table
            //
            *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
            return EFI_SUCCESS;
          }
          if (ExMapTable[i].ExGuidIndex == GuidTableIdx) {
            *TokenNumber = ExMapTable[i].ExTokenNumber;
            return EFI_SUCCESS;
          } else {
            *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
            return EFI_SUCCESS;
          }
        }
      }
      return EFI_NOT_FOUND;
    }
  }

  return EFI_NOT_FOUND;
}



EFI_STATUS
EFIAPI
PeiPcdGetNextTokenSpace (
  IN OUT CONST EFI_GUID          **Guid
  )
{
  UINTN               GuidTableIdx;
  EFI_GUID            *MatchGuid;
  PEI_PCD_DATABASE    *PeiPcdDb;
  DYNAMICEX_MAPPING   *ExMapTable;
  UINTN               i;
  BOOLEAN             Found;
  BOOLEAN             PeiExMapTableEmpty;

  if (!FeaturePcdGet (PcdPeiPcdDatabaseTraverseEnabled)) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (Guid != NULL);

  PeiExMapTableEmpty = PEI_EXMAP_TABLE_EMPTY;

  if (PeiExMapTableEmpty) {
    if (*Guid != NULL) {
      return EFI_NOT_FOUND;
    } else {
      return EFI_SUCCESS;
    }
  }

  //
  // Assume PCD Database AutoGen tool is sorting the ExMap based on the following order
  // 1) ExGuid
  // 2) ExTokenNumber
  //
  PeiPcdDb = GetPcdDatabase ();

  ExMapTable = PeiPcdDb->Init.ExMapTable;

  if (*Guid == NULL) {
    //
    // return the first Token Space Guid.
    //
    *Guid = &PeiPcdDb->Init.GuidTable[ExMapTable[0].ExGuidIndex];
    return EFI_SUCCESS;
  }

  MatchGuid = ScanGuid (PeiPcdDb->Init.GuidTable, sizeof(PeiPcdDb->Init.GuidTable), *Guid);

  if (MatchGuid == NULL) {
    return EFI_NOT_FOUND;
  }
  
  GuidTableIdx = MatchGuid - PeiPcdDb->Init.GuidTable;

  Found = FALSE;
  for (i = 0; i < PEI_EXMAPPING_TABLE_SIZE; i++) {
    if (ExMapTable[i].ExGuidIndex == GuidTableIdx) {
      Found = TRUE;
      break;
    }
  }

  if (Found) {
    i++;
    for ( ; i < PEI_EXMAPPING_TABLE_SIZE; i++ ) {
      if (ExMapTable[i].ExGuidIndex != GuidTableIdx ) {
        *Guid = &PeiPcdDb->Init.GuidTable[ExMapTable[i].ExGuidIndex];
        return EFI_SUCCESS;
      }
    }
    *Guid = NULL;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;

}

UINTN
GetPtrTypeSize (
  IN    UINTN             LocalTokenNumberTableIdx,
  OUT   UINTN             *MaxSize,
  IN    PEI_PCD_DATABASE  *Database
  )
{
  INTN        SizeTableIdx;
  UINTN       LocalTokenNumber;
  SKU_ID      *SkuIdTable;
  SIZE_INFO   *SizeTable;
  UINTN       i;

  SizeTableIdx = GetSizeTableIndex (LocalTokenNumberTableIdx, Database);

  LocalTokenNumber = Database->Init.LocalTokenNumberTable[LocalTokenNumberTableIdx];

  ASSERT ((LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) == PCD_DATUM_TYPE_POINTER);
  
  SizeTable = Database->Init.SizeTable;

  *MaxSize = SizeTable[SizeTableIdx];
  //
  // SizeTable only contain record for PCD_DATUM_TYPE_POINTER type 
  // PCD entry.
  //
  if (LocalTokenNumber & PCD_TYPE_VPD) {
      //
      // We have only one entry for VPD enabled PCD entry:
      // 1) MAX Size.
      // We consider current size is equal to MAX size.
      //
      return *MaxSize;
  } else {
    if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == 0) {
      //
      // We have only two entry for Non-Sku enabled PCD entry:
      // 1) MAX SIZE
      // 2) Current Size
      //
      return SizeTable[SizeTableIdx + 1];
    } else {
      //
      // We have these entry for SKU enabled PCD entry
      // 1) MAX SIZE
      // 2) Current Size for each SKU_ID (It is equal to MaxSku).
      //
      SkuIdTable = GetSkuIdArray (LocalTokenNumberTableIdx, Database);
      for (i = 0; i < SkuIdTable[0]; i++) {
        if (SkuIdTable[1 + i] == Database->Init.SystemSkuId) {
          return SizeTable[SizeTableIdx + 1 + i];
        }
      }
      return SizeTable[SizeTableIdx + 1];
    }
  }
}



BOOLEAN
SetPtrTypeSize (
  IN          UINTN             LocalTokenNumberTableIdx,
  IN    OUT   UINTN             *CurrentSize,
  IN          PEI_PCD_DATABASE  *Database
  )
{
  INTN        SizeTableIdx;
  UINTN       LocalTokenNumber;
  SKU_ID      *SkuIdTable;
  SIZE_INFO   *SizeTable;
  UINTN       i;
  UINTN       MaxSize;
  
  SizeTableIdx = GetSizeTableIndex (LocalTokenNumberTableIdx, Database);

  LocalTokenNumber = Database->Init.LocalTokenNumberTable[LocalTokenNumberTableIdx];

  ASSERT ((LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) == PCD_DATUM_TYPE_POINTER);
  
  SizeTable = Database->Init.SizeTable;

  MaxSize = SizeTable[SizeTableIdx];
  //
  // SizeTable only contain record for PCD_DATUM_TYPE_POINTER type 
  // PCD entry.
  //
  if (LocalTokenNumber & PCD_TYPE_VPD) {
      //
      // We shouldn't come here as we don't support SET for VPD
      //
      ASSERT (FALSE);
      return FALSE;
  } else {
    if ((*CurrentSize > MaxSize) ||
      (*CurrentSize == MAX_ADDRESS)) {
       *CurrentSize = MaxSize;
       return FALSE;
    } 
    
    if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == 0) {
      //
      // We have only two entry for Non-Sku enabled PCD entry:
      // 1) MAX SIZE
      // 2) Current Size
      //
      SizeTable[SizeTableIdx + 1] = (SIZE_INFO) *CurrentSize;
      return TRUE;
    } else {
      //
      // We have these entry for SKU enabled PCD entry
      // 1) MAX SIZE
      // 2) Current Size for each SKU_ID (It is equal to MaxSku).
      //
      SkuIdTable = GetSkuIdArray (LocalTokenNumberTableIdx, Database);
      for (i = 0; i < SkuIdTable[0]; i++) {
        if (SkuIdTable[1 + i] == Database->Init.SystemSkuId) {
          SizeTable[SizeTableIdx + 1 + i] = (SIZE_INFO) *CurrentSize;
          return TRUE;
        }
      }
      SizeTable[SizeTableIdx + 1] = (SIZE_INFO) *CurrentSize;
      return TRUE;
    }
  }

}
