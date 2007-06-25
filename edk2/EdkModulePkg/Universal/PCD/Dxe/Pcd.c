/** @file
PCD DXE driver

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name: Pcd.c

**/

#include "Service.h"

EFI_LOCK mPcdDatabaseLock = EFI_INITIALIZE_LOCK_VARIABLE(TPL_CALLBACK);

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
  DxePcdGetNextToken,
  DxePcdGetNextTokenSpace
};


//
// Static global to reduce the code size
//
static EFI_HANDLE mNewHandle = NULL;

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

  Status = gBS->InstallProtocolInterface (
                  &mNewHandle,
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
  UINTN   Size;
  UINT32  *LocalTokenNumberTable;
  BOOLEAN IsPeiDb;
  UINTN   MaxSize;
  UINTN   TmpTokenNumber;
  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  //
  // Backup the TokenNumber passed in as GetPtrTypeSize need the original TokenNumber
  // 
  TmpTokenNumber = TokenNumber;

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  ASSERT (TokenNumber + 1 < PCD_TOTAL_TOKEN_NUMBER + 1);

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  IsPeiDb = (BOOLEAN) (TokenNumber + 1 < PEI_LOCAL_TOKEN_NUMBER + 1);
  
  TokenNumber = IsPeiDb ? TokenNumber : 
                          (TokenNumber - PEI_LOCAL_TOKEN_NUMBER);

  LocalTokenNumberTable = IsPeiDb ? mPcdDatabase->PeiDb.Init.LocalTokenNumberTable 
                                  : mPcdDatabase->DxeDb.Init.LocalTokenNumberTable;

  Size = (LocalTokenNumberTable[TokenNumber] & PCD_DATUM_TYPE_ALL_SET) >> PCD_DATUM_TYPE_SHIFT;

  if (Size == 0) {
    //
    // For pointer type, we need to scan the SIZE_TABLE to get the current size.
    //
    return GetPtrTypeSize (TmpTokenNumber, &MaxSize);
  } else {
    return Size;
  }

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
  return  ExGetWorker (Guid, ExTokenNumber, 0);
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
  return DxePcdGetSize(GetExPcdTokenNumber (Guid, (UINT32) ExTokenNumber));
}



EFI_STATUS
EFIAPI
DxePcdSet8 (
  IN UINTN              TokenNumber,
  IN UINT8              Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
DxePcdSet16 (
  IN UINTN              TokenNumber,
  IN UINT16             Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
DxePcdSet32 (
  IN UINTN              TokenNumber,
  IN UINT32             Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
DxePcdSet64 (
  IN UINTN              TokenNumber,
  IN UINT64             Value
  )
{
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
DxePcdSetPtr (
  IN          UINTN              TokenNumber,
  IN OUT      UINTN              *SizeOfBuffer,
  IN          VOID               *Buffer
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
  return SetValueWorker (TokenNumber, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
DxePcdSet8Ex (
  IN CONST EFI_GUID         *Guid,
  IN UINTN                  ExTokenNumber,
  IN UINT8                  Value
  )
{
  return  ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
DxePcdSet16Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             ExTokenNumber,
  IN UINT16            Value
  )
{
  return  ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
DxePcdSet32Ex (
  IN CONST EFI_GUID     *Guid,
  IN UINTN              ExTokenNumber,
  IN UINT32             Value
  )
{
  return  ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
DxePcdSet64Ex (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             ExTokenNumber,
  IN UINT64            Value
  )
{
  return  ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}



EFI_STATUS
EFIAPI
DxePcdSetPtrEx (
  IN            CONST EFI_GUID         *Guid,
  IN            UINTN                  ExTokenNumber,
  IN OUT        UINTN                  *SizeOfBuffer,
  IN            VOID                   *Buffer
  )
{
  return  ExSetWorker(ExTokenNumber, Guid, Buffer, SizeOfBuffer, TRUE);
}



EFI_STATUS
EFIAPI
DxePcdSetBoolEx (
  IN CONST EFI_GUID    *Guid,
  IN UINTN             ExTokenNumber,
  IN BOOLEAN           Value
  )
{
  return  ExSetValueWorker (ExTokenNumber, Guid, &Value, sizeof (Value));
}




EFI_STATUS
EFIAPI
DxeRegisterCallBackOnSet (
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  UINTN                   TokenNumber,
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
{
  EFI_STATUS Status;
  
  ASSERT (CallBackFunction != NULL);
  
  //
  // Aquire lock to prevent reentrance from TPL_CALLBACK level
  //
  EfiAcquireLock (&mPcdDatabaseLock);

  Status = DxeRegisterCallBackWorker (TokenNumber, Guid, CallBackFunction);

  EfiReleaseLock (&mPcdDatabaseLock);
  
  return Status;
}



EFI_STATUS
EFIAPI
DxeUnRegisterCallBackOnSet (
  IN  CONST EFI_GUID          *Guid, OPTIONAL
  IN  UINTN                   TokenNumber,
  IN  PCD_PROTOCOL_CALLBACK   CallBackFunction
  )
{
  EFI_STATUS Status;
  
  ASSERT (CallBackFunction != NULL);

  //
  // Aquire lock to prevent reentrance from TPL_CALLBACK level
  //
  EfiAcquireLock (&mPcdDatabaseLock);
  
  Status = DxeUnRegisterCallBackWorker (TokenNumber, Guid, CallBackFunction);

  EfiReleaseLock (&mPcdDatabaseLock);
  
  return Status;
}



EFI_STATUS
EFIAPI
DxePcdGetNextToken (
  IN CONST EFI_GUID         *Guid, OPTIONAL
  IN OUT   UINTN            *TokenNumber
  )
{
  EFI_STATUS          Status;
  BOOLEAN             PeiExMapTableEmpty;
  BOOLEAN             DxeExMapTableEmpty;

  if (!FeaturePcdGet (PcdDxePcdDatabaseTraverseEnabled)) {
    return EFI_UNSUPPORTED;
  }

  Status = EFI_NOT_FOUND;
  PeiExMapTableEmpty = PEI_EXMAP_TABLE_EMPTY;
  DxeExMapTableEmpty = DXE_EXMAP_TABLE_EMPTY;

  //
  // Scan the local token space
  //
  if (Guid == NULL) {
    // EBC compiler is very choosy. It may report warning about comparison
    // between UINTN and 0 . So we add 1 in each size of the 
    // comparison.
    if (((*TokenNumber + 1 > PEI_NEX_TOKEN_NUMBER + 1) && (*TokenNumber + 1 < PEI_LOCAL_TOKEN_NUMBER + 1)) ||
        ((*TokenNumber + 1 > (PEI_LOCAL_TOKEN_NUMBER + DXE_NEX_TOKEN_NUMBER + 1)))) {
        return EFI_NOT_FOUND;
    }
    
    (*TokenNumber)++;
    if ((*TokenNumber + 1 > PEI_NEX_TOKEN_NUMBER + 1) &&
        (*TokenNumber <= PEI_LOCAL_TOKEN_NUMBER)) {
      //
      // The first Non-Ex type Token Number for DXE PCD 
      // database is PEI_LOCAL_TOKEN_NUMBER
      //
      *TokenNumber = PEI_LOCAL_TOKEN_NUMBER;
    } else if (*TokenNumber + 1 > DXE_NEX_TOKEN_NUMBER + PEI_LOCAL_TOKEN_NUMBER + 1) {
      *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
    }
    return EFI_SUCCESS;
  }

  if (PeiExMapTableEmpty && DxeExMapTableEmpty) {
    *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
    return EFI_NOT_FOUND;
  }

  if (!PeiExMapTableEmpty) {
    Status = ExGetNextTokeNumber (
                        Guid,
                        TokenNumber,
                        mPcdDatabase->PeiDb.Init.GuidTable,
                        sizeof(mPcdDatabase->PeiDb.Init.GuidTable),
                        mPcdDatabase->PeiDb.Init.ExMapTable,
                        sizeof(mPcdDatabase->PeiDb.Init.ExMapTable)
                        );
  }

  if (Status == EFI_SUCCESS) {
    return Status;
  }

  if (!DxeExMapTableEmpty) {
    Status = ExGetNextTokeNumber (
                        Guid,
                        TokenNumber,
                        mPcdDatabase->DxeDb.Init.GuidTable,
                        sizeof(mPcdDatabase->DxeDb.Init.GuidTable),
                        mPcdDatabase->DxeDb.Init.ExMapTable,
                        sizeof(mPcdDatabase->DxeDb.Init.ExMapTable)
                        );
  }

  return Status;
}

STATIC
EFI_GUID **
GetDistinctTokenSpace (
  IN OUT    UINTN             *ExMapTableSize,
  IN        DYNAMICEX_MAPPING *ExMapTable,
  IN        EFI_GUID          *GuidTable
  )
{
  EFI_GUID  **DistinctTokenSpace;
  UINTN     OldGuidIndex;
  UINTN     TsIdx;
  UINTN     Idx;


  DistinctTokenSpace = AllocateZeroPool (*ExMapTableSize * sizeof (EFI_GUID *));
  ASSERT (DistinctTokenSpace != NULL);

  TsIdx = 0;
  OldGuidIndex = ExMapTable[0].ExGuidIndex;
  DistinctTokenSpace[TsIdx] = &GuidTable[OldGuidIndex];
  for (Idx = 1; Idx < *ExMapTableSize; Idx++) {
    if (ExMapTable[Idx].ExGuidIndex != OldGuidIndex) {
      OldGuidIndex = ExMapTable[Idx].ExGuidIndex;
      DistinctTokenSpace[++TsIdx] = &GuidTable[OldGuidIndex];
    }
  }

  //
  // The total number of Distinct Token Space
  // is TsIdx + 1 because we use TsIdx as a index
  // to the DistinctTokenSpace[]
  //
  *ExMapTableSize = TsIdx + 1;
  return DistinctTokenSpace;
    
}
  
//
// Just pre-allocate a memory buffer that is big enough to
// host all distinct TokenSpace guid in both
// PEI ExMap and DXE ExMap.
//
STATIC EFI_GUID *TmpTokenSpaceBuffer[PEI_EXMAPPING_TABLE_SIZE + DXE_EXMAPPING_TABLE_SIZE] = { 0 };

EFI_STATUS
EFIAPI
DxePcdGetNextTokenSpace (
  IN OUT CONST EFI_GUID               **Guid
  )
{
  UINTN               Idx;
  UINTN               Idx2;
  UINTN               Idx3;
  UINTN               PeiTokenSpaceTableSize;
  UINTN               DxeTokenSpaceTableSize;
  EFI_GUID            **PeiTokenSpaceTable;
  EFI_GUID            **DxeTokenSpaceTable;
  BOOLEAN             Match;
  BOOLEAN             PeiExMapTableEmpty;
  BOOLEAN             DxeExMapTableEmpty;

  if (!FeaturePcdGet (PcdDxePcdDatabaseTraverseEnabled)) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (Guid != NULL);
  
  PeiExMapTableEmpty = PEI_EXMAP_TABLE_EMPTY;
  DxeExMapTableEmpty = DXE_EXMAP_TABLE_EMPTY;

  if (PeiExMapTableEmpty && DxeExMapTableEmpty) {
    if (*Guid != NULL) {
      return EFI_NOT_FOUND;
    } else {
      return EFI_SUCCESS;
    }
  }
  
  
  if (TmpTokenSpaceBuffer[0] == NULL) {
    PeiTokenSpaceTableSize = 0;

    if (!PeiExMapTableEmpty) {
      PeiTokenSpaceTableSize = PEI_EXMAPPING_TABLE_SIZE;
      PeiTokenSpaceTable = GetDistinctTokenSpace (&PeiTokenSpaceTableSize,
                            mPcdDatabase->PeiDb.Init.ExMapTable,
                            mPcdDatabase->PeiDb.Init.GuidTable
                            );
      CopyMem (TmpTokenSpaceBuffer, PeiTokenSpaceTable, sizeof (EFI_GUID*) * PeiTokenSpaceTableSize);
    }

    if (!DxeExMapTableEmpty) {
      DxeTokenSpaceTableSize = DXE_EXMAPPING_TABLE_SIZE;
      DxeTokenSpaceTable = GetDistinctTokenSpace (&DxeTokenSpaceTableSize,
                            mPcdDatabase->DxeDb.Init.ExMapTable,
                            mPcdDatabase->DxeDb.Init.GuidTable
                            );

      //
      // Make sure EFI_GUID in DxeTokenSpaceTable does not exist in PeiTokenSpaceTable
      //
      for (Idx2 = 0, Idx3 = PeiTokenSpaceTableSize; Idx2 < DxeTokenSpaceTableSize; Idx2++) {
        Match = FALSE;
        for (Idx = 0; Idx < PeiTokenSpaceTableSize; Idx++) {
          if (CompareGuid (TmpTokenSpaceBuffer[Idx], DxeTokenSpaceTable[Idx2])) {
            Match = TRUE;
            break;
          }
        }
        if (!Match) {
          TmpTokenSpaceBuffer[Idx3++] = DxeTokenSpaceTable[Idx2];
        }
      }
    }
  }

  if (*Guid == NULL) {
    *Guid = TmpTokenSpaceBuffer[0];
    return EFI_SUCCESS;
  }
  
  for (Idx = 0; Idx < (PEI_EXMAPPING_TABLE_SIZE + DXE_EXMAPPING_TABLE_SIZE); Idx++) {
    if(CompareGuid (*Guid, TmpTokenSpaceBuffer[Idx])) {
      Idx++;
      *Guid = TmpTokenSpaceBuffer[Idx];
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;

}


