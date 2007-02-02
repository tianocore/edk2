/** @file
Private functions used by PCD PEIM.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name: Service.c

**/
#include "Service.h"

/**
  The function registers the CallBackOnSet fucntion
  according to TokenNumber and EFI_GUID space.

  @param  TokenNumber       The token number.
  @param  Guid              The GUID space.
  @param  CallBackFunction  The Callback function to be registered.
  @param  Register          To register or unregister the callback function.

  @retval EFI_SUCCESS If the Callback function is registered.
  @retval EFI_NOT_FOUND If the PCD Entry is not found according to Token Number and GUID space.
  @retval EFI_OUT_OF_RESOURCES If the callback function can't be registered because there is not free
                                slot left in the CallbackFnTable.
--*/
EFI_STATUS
PeiRegisterCallBackWorker (
  IN  UINTN                       ExTokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK            CallBackFunction,
  IN  BOOLEAN                     Register
)
{
  EFI_HOB_GUID_TYPE       *GuidHob;
  PCD_PPI_CALLBACK        *CallbackTable;
  PCD_PPI_CALLBACK        Compare;
  PCD_PPI_CALLBACK        Assign;
  UINT32                  LocalTokenNumber;
  UINTN                   TokenNumber;
  UINTN                   Idx;

  if (Guid == NULL) {
    TokenNumber = ExTokenNumber;

    //
    // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
    // We have to decrement TokenNumber by 1 to make it usable
    // as the array index.
    //
    TokenNumber--;
    ASSERT (TokenNumber + 1 < PEI_NEX_TOKEN_NUMBER + 1);
  } else {
    TokenNumber = GetExPcdTokenNumber (Guid, ExTokenNumber);

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
  }


  LocalTokenNumber = GetPcdDatabase()->Init.LocalTokenNumberTable[TokenNumber];

  //
  // We don't support SET for HII and VPD type PCD entry in PEI phase.
  // So we will assert if any register callback for such PCD entry.
  //
  ASSERT ((LocalTokenNumber & PCD_TYPE_HII) == 0);
  ASSERT ((LocalTokenNumber & PCD_TYPE_VPD) == 0);

  GuidHob = GetFirstGuidHob (&gPcdPeiCallbackFnTableHobGuid);
  ASSERT (GuidHob != NULL);
  
  CallbackTable = GET_GUID_HOB_DATA (GuidHob);
  CallbackTable = CallbackTable + (TokenNumber * FixedPcdGet32(PcdMaxPeiPcdCallBackNumberPerPcdEntry));

  Compare = Register? NULL: CallBackFunction;
  Assign  = Register? CallBackFunction: NULL;


  for (Idx = 0; Idx < FixedPcdGet32(PcdMaxPeiPcdCallBackNumberPerPcdEntry); Idx++) {
    if (CallbackTable[Idx] == Compare) {
      CallbackTable[Idx] = Assign;
      return EFI_SUCCESS;
    }
  }

  return Register? EFI_OUT_OF_RESOURCES : EFI_NOT_FOUND;

}




/**
  The function builds the PCD database.

  @param VOID

  @retval VOID
--*/
VOID
BuildPcdDatabase (
  VOID
  )
{
  PEI_PCD_DATABASE  *Database;
  VOID              *CallbackFnTable;
  UINTN             SizeOfCallbackFnTable;
  
  Database = BuildGuidHob (&gPcdDataBaseHobGuid, sizeof (PEI_PCD_DATABASE));

  ZeroMem (Database, sizeof (PEI_PCD_DATABASE));

  //
  // gPEIPcdDbInit is smaller than PEI_PCD_DATABASE
  //
  
  CopyMem (&Database->Init, &gPEIPcdDbInit, sizeof (gPEIPcdDbInit));

  SizeOfCallbackFnTable = PEI_LOCAL_TOKEN_NUMBER * sizeof (PCD_PPI_CALLBACK) * FixedPcdGet32(PcdMaxPeiPcdCallBackNumberPerPcdEntry);

  CallbackFnTable = BuildGuidHob (&gPcdPeiCallbackFnTableHobGuid, SizeOfCallbackFnTable);
  
  ZeroMem (CallbackFnTable, SizeOfCallbackFnTable);
  
  return;
}



/**
  The function is provided by PCD PEIM and PCD DXE driver to
  do the work of reading a HII variable from variable service.

  @param VariableGuid     The Variable GUID.
  @param VariableName     The Variable Name.
  @param VariableData    The output data.
  @param VariableSize    The size of the variable.

  @retval EFI_SUCCESS         Operation successful.
  @retval EFI_NOT_FOUND         Variablel not found.
--*/
STATIC
EFI_STATUS
GetHiiVariable (
  IN  CONST EFI_GUID      *VariableGuid,
  IN  UINT16              *VariableName,
  OUT VOID                **VariableData,
  OUT UINTN               *VariableSize
  )
{
  UINTN      Size;
  EFI_STATUS Status;
  VOID       *Buffer;
  EFI_PEI_READ_ONLY_VARIABLE_PPI *VariablePpi;

  Status = PeiServicesLocatePpi (&gEfiPeiReadOnlyVariablePpiGuid, 0, NULL, (VOID **) &VariablePpi);
  ASSERT_EFI_ERROR (Status);

  Size = 0;
  Status = VariablePpi->PeiGetVariable (
                          GetPeiServicesTablePointer (),
                          VariableName,
                          (EFI_GUID *) VariableGuid,
                          NULL,
                          &Size,
                          NULL
                            );
  if (Status == EFI_BUFFER_TOO_SMALL) {


    Status = PeiServicesAllocatePool (Size, &Buffer);
    ASSERT_EFI_ERROR (Status);

    Status = VariablePpi->PeiGetVariable (
                              GetPeiServicesTablePointer (),
                              (UINT16 *) VariableName,
                              (EFI_GUID *) VariableGuid,
                              NULL,
                              &Size,
                              Buffer
                              );
    ASSERT_EFI_ERROR (Status);

    *VariableSize = Size;
    *VariableData = Buffer;

    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }

}

STATIC
UINT32
GetSkuEnabledTokenNumber (
  UINT32 LocalTokenNumber,
  UINTN  Size
  ) 
{
  PEI_PCD_DATABASE      *PeiPcdDb;
  SKU_HEAD              *SkuHead;
  SKU_ID                *SkuIdTable;
  INTN                  i;
  UINT8                 *Value;

  PeiPcdDb = GetPcdDatabase ();

  ASSERT ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == 0);

  SkuHead     = (SKU_HEAD *) ((UINT8 *)PeiPcdDb + (LocalTokenNumber & PCD_DATABASE_OFFSET_MASK));
  Value       = (UINT8 *) ((UINT8 *)PeiPcdDb + (SkuHead->SkuDataStartOffset));
  SkuIdTable  = (SKU_ID *) ((UINT8 *)PeiPcdDb + (SkuHead->SkuIdTableOffset));
        
  for (i = 0; i < SkuIdTable[0]; i++) {
    if (PeiPcdDb->Init.SystemSkuId == SkuIdTable[i + 1]) {
      break;
    }
  }

  switch (LocalTokenNumber & PCD_TYPE_ALL_SET) {
    case PCD_TYPE_VPD:
      Value = (UINT8 *) &(((VPD_HEAD *) Value)[i]);
      return (UINT32) ((Value - (UINT8 *) PeiPcdDb) | PCD_TYPE_VPD);

    case PCD_TYPE_HII:
      Value = (UINT8 *) &(((VARIABLE_HEAD *) Value)[i]);
      return (UINT32) ((Value - (UINT8 *) PeiPcdDb) | PCD_TYPE_HII);
      
    case PCD_TYPE_STRING:
      Value = (UINT8 *) &(((STRING_HEAD *) Value)[i]);
      return (UINT32) ((Value - (UINT8 *) PeiPcdDb) | PCD_TYPE_STRING);

    case PCD_TYPE_DATA:
      Value += Size * i;
      return (UINT32) (Value - (UINT8 *) PeiPcdDb);

    default:
      ASSERT (FALSE);
  }

  ASSERT (FALSE);

  return 0;
  
}



STATIC
VOID
InvokeCallbackOnSet (
  UINTN             ExTokenNumber,
  CONST EFI_GUID    *Guid, OPTIONAL
  UINTN             TokenNumber,
  VOID              *Data,
  UINTN             Size
  )
{
  EFI_HOB_GUID_TYPE   *GuidHob;
  PCD_PPI_CALLBACK    *CallbackTable;
  UINTN               Idx;

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;
  
  if (Guid == NULL) {
    // EBC compiler is very choosy. It may report warning about comparison
    // between UINTN and 0 . So we add 1 in each size of the 
    // comparison.
    ASSERT (TokenNumber + 1 < PEI_LOCAL_TOKEN_NUMBER + 1);
  }

  GuidHob = GetFirstGuidHob (&gPcdPeiCallbackFnTableHobGuid);
  ASSERT (GuidHob != NULL);
  
  CallbackTable = GET_GUID_HOB_DATA (GuidHob);

  CallbackTable += (TokenNumber * FixedPcdGet32(PcdMaxPeiPcdCallBackNumberPerPcdEntry));

  for (Idx = 0; Idx < FixedPcdGet32(PcdMaxPeiPcdCallBackNumberPerPcdEntry); Idx++) {
    if (CallbackTable[Idx] != NULL) {
      CallbackTable[Idx] (Guid,
                          (Guid == NULL)? TokenNumber: ExTokenNumber,
                          Data,
                          Size
                          );
    }
  }
  
}



EFI_STATUS
SetValueWorker (
  IN          UINTN              TokenNumber,
  IN          VOID               *Data,
  IN          UINTN              Size
  )
{
  return SetWorker (TokenNumber, Data, &Size, FALSE);
}



EFI_STATUS
SetWorker (
  IN          UINTN               TokenNumber,
  IN OUT      VOID                *Data,
  IN OUT      UINTN               *Size,
  IN          BOOLEAN             PtrType
  )
{
  UINT32              LocalTokenNumber;
  PEI_PCD_DATABASE    *PeiPcdDb;
  UINT16              StringTableIdx;
  UINTN               Offset;
  VOID                *InternalData;
  UINTN               MaxSize;

  if (!FeaturePcdGet(PcdPeiPcdDatabaseSetEnabled)) {
    return EFI_UNSUPPORTED;
  }
  
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
    
  PeiPcdDb = GetPcdDatabase ();

  LocalTokenNumber = PeiPcdDb->Init.LocalTokenNumberTable[TokenNumber];

  if (!PtrType) {
    ASSERT (PeiPcdGetSize(TokenNumber + 1) == *Size);
  }

  //
  // We only invoke the callback function for Dynamic Type PCD Entry.
  // For Dynamic EX PCD entry, we have invoked the callback function for Dynamic EX
  // type PCD entry in ExSetWorker.
  //
  if (TokenNumber + 1 < PEI_NEX_TOKEN_NUMBER + 1) {
    InvokeCallbackOnSet (0, NULL, TokenNumber + 1, Data, *Size);
  }

  if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == PCD_TYPE_SKU_ENABLED) {
    if (PtrType) {
      MaxSize = GetPtrTypeSize (TokenNumber, &MaxSize, PeiPcdDb);
    } else {
      MaxSize = *Size;
    }
    LocalTokenNumber = GetSkuEnabledTokenNumber (LocalTokenNumber & ~PCD_TYPE_SKU_ENABLED, MaxSize);
  }

  Offset          = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;
  InternalData    = (VOID *) ((UINT8 *) PeiPcdDb + Offset);
  
  switch (LocalTokenNumber & PCD_TYPE_ALL_SET) {
    case PCD_TYPE_VPD:
    case PCD_TYPE_HII:
    {
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    }

    case PCD_TYPE_STRING:
      if (SetPtrTypeSize (TokenNumber, Size, PeiPcdDb)) {
        StringTableIdx = *((UINT16 *)InternalData);
        CopyMem (&PeiPcdDb->Init.StringTable[StringTableIdx], Data, *Size);
        return EFI_SUCCESS;
      } else {
        return EFI_INVALID_PARAMETER;
      }

    case PCD_TYPE_DATA:
    {
      if (PtrType) {
        if (SetPtrTypeSize (TokenNumber, Size, PeiPcdDb)) {
          CopyMem (InternalData, Data, *Size);
          return EFI_SUCCESS;
        } else {
          return EFI_INVALID_PARAMETER;
        }
      }

      switch (*Size) {
        case sizeof(UINT8):
          *((UINT8 *) InternalData) = *((UINT8 *) Data);
          return EFI_SUCCESS;

        case sizeof(UINT16):
          *((UINT16 *) InternalData) = *((UINT16 *) Data);
          return EFI_SUCCESS;

        case sizeof(UINT32):
          *((UINT32 *) InternalData) = *((UINT32 *) Data);
          return EFI_SUCCESS;

        case sizeof(UINT64):
          *((UINT64 *) InternalData) = *((UINT64 *) Data);
          return EFI_SUCCESS;

        default:
          ASSERT (FALSE);
          return EFI_NOT_FOUND;
      }
    }
      
  }

  ASSERT (FALSE);
  return EFI_NOT_FOUND;

}



EFI_STATUS
ExSetValueWorker (
  IN          UINTN                ExTokenNumber,
  IN          CONST EFI_GUID       *Guid,
  IN          VOID                 *Data,
  IN          UINTN                Size
  )
{
  return ExSetWorker (ExTokenNumber, Guid, Data, &Size, FALSE);
}



EFI_STATUS
ExSetWorker (
  IN            UINTN                ExTokenNumber,
  IN            CONST EFI_GUID       *Guid,
  IN            VOID                 *Data,
  IN OUT        UINTN                *Size,
  IN            BOOLEAN              PtrType
  )
{
  UINTN                     TokenNumber;

  if (!FeaturePcdGet(PcdPeiPcdDatabaseSetEnabled)) {
    return EFI_UNSUPPORTED;
  }

  TokenNumber = GetExPcdTokenNumber (Guid, ExTokenNumber);

  InvokeCallbackOnSet (ExTokenNumber, Guid, TokenNumber, Data, *Size);

  return SetWorker (TokenNumber, Data, Size, PtrType);

}




VOID *
ExGetWorker (
  IN CONST  EFI_GUID  *Guid,
  IN UINTN            ExTokenNumber,
  IN UINTN            GetSize
  )
{
  if (!FeaturePcdGet (PcdPeiPcdDatabaseExEnabled)) {
    ASSERT (FALSE);
    return 0;
  }
  
  return GetWorker (GetExPcdTokenNumber (Guid, ExTokenNumber), GetSize);
}




VOID *
GetWorker (
  UINTN               TokenNumber,
  UINTN               GetSize
  )
{
  UINT32              Offset;
  EFI_GUID            *Guid;
  UINT16              *Name;
  VARIABLE_HEAD       *VariableHead;
  EFI_STATUS          Status;
  UINTN               DataSize;
  VOID                *Data;
  UINT16              *StringTable;
  UINT16              StringTableIdx;
  PEI_PCD_DATABASE    *PeiPcdDb;
  UINT32              LocalTokenNumber;
  UINTN               MaxSize;

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

  ASSERT ((GetSize == PeiPcdGetSize(TokenNumber + 1)) || (GetSize == 0));

  PeiPcdDb        = GetPcdDatabase ();

  LocalTokenNumber = PeiPcdDb->Init.LocalTokenNumberTable[TokenNumber];

  if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == PCD_TYPE_SKU_ENABLED) {
    if (GetSize == 0) {
      MaxSize = GetPtrTypeSize (TokenNumber, &MaxSize, PeiPcdDb);
    } else {
      MaxSize = GetSize;
    }
    LocalTokenNumber = GetSkuEnabledTokenNumber (LocalTokenNumber & ~PCD_TYPE_SKU_ENABLED, MaxSize);
  }

  Offset      = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;
  StringTable = PeiPcdDb->Init.StringTable;
  
  switch (LocalTokenNumber & PCD_TYPE_ALL_SET) {
    case PCD_TYPE_VPD:
    {
      VPD_HEAD *VpdHead;
      VpdHead = (VPD_HEAD *) ((UINT8 *)PeiPcdDb + Offset);
      return (VOID *) (UINTN) (FixedPcdGet32(PcdVpdBaseAddress) + VpdHead->Offset);
    }
      
    case PCD_TYPE_HII:
    {
      VariableHead = (VARIABLE_HEAD *) ((UINT8 *)PeiPcdDb + Offset);
      
      Guid = &(PeiPcdDb->Init.GuidTable[VariableHead->GuidTableIndex]);
      Name = &StringTable[VariableHead->StringIndex];

      Status = GetHiiVariable (Guid, Name, &Data, &DataSize);

      if (Status == EFI_SUCCESS) {
        return (VOID *) ((UINT8 *) Data + VariableHead->Offset);
      } else {
        //
        // Return the default value specified by Platform Integrator 
        //
        return (VOID *) ((UINT8 *) PeiPcdDb + VariableHead->DefaultValueOffset);
      }
    }

    case PCD_TYPE_DATA:
      return (VOID *) ((UINT8 *)PeiPcdDb + Offset);

    case PCD_TYPE_STRING:
      StringTableIdx = (UINT16) *((UINT8 *) PeiPcdDb + Offset);
      return (VOID *) (&StringTable[StringTableIdx]);

    default:
      ASSERT (FALSE);
      break;
      
  }

  ASSERT (FALSE);
      
  return NULL;
  
}



UINTN           
GetExPcdTokenNumber (
  IN CONST EFI_GUID             *Guid,
  IN UINTN                      ExTokenNumber
  )
{
  UINT32              i;
  DYNAMICEX_MAPPING   *ExMap;
  EFI_GUID            *GuidTable;
  EFI_GUID            *MatchGuid;
  UINTN               MatchGuidIdx;
  PEI_PCD_DATABASE    *PeiPcdDb;

  PeiPcdDb    = GetPcdDatabase();
  
  ExMap       = PeiPcdDb->Init.ExMapTable;
  GuidTable   = PeiPcdDb->Init.GuidTable;

  MatchGuid = ScanGuid (GuidTable, sizeof(PeiPcdDb->Init.GuidTable), Guid);
  //
  // We need to ASSERT here. If GUID can't be found in GuidTable, this is a
  // error in the BUILD system.
  //
  ASSERT (MatchGuid != NULL);
  
  MatchGuidIdx = MatchGuid - GuidTable;
  
  for (i = 0; i < PEI_EXMAPPING_TABLE_SIZE; i++) {
    if ((ExTokenNumber == ExMap[i].ExTokenNumber) && 
        (MatchGuidIdx == ExMap[i].ExGuidIndex)) {
      return ExMap[i].LocalTokenNumber;
    }
  }
  
  ASSERT (FALSE);
  
  return 0;
}



PEI_PCD_DATABASE *
GetPcdDatabase (
  VOID
  )
{
  EFI_HOB_GUID_TYPE *GuidHob;

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  ASSERT (GuidHob != NULL);
  
  return (PEI_PCD_DATABASE *) GET_GUID_HOB_DATA (GuidHob);
}



SKU_ID *
GetSkuIdArray (
  IN    UINTN             LocalTokenNumberTableIdx,
  IN    PEI_PCD_DATABASE  *Database
  )
{
  SKU_HEAD *SkuHead;
  UINTN     LocalTokenNumber;

  LocalTokenNumber = Database->Init.LocalTokenNumberTable[LocalTokenNumberTableIdx];

  ASSERT ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) != 0);

  SkuHead = (SKU_HEAD *) ((UINT8 *)Database + (LocalTokenNumber & PCD_DATABASE_OFFSET_MASK));

  return (SKU_ID *) ((UINT8 *)Database + SkuHead->SkuIdTableOffset);
  
}



UINTN
GetSizeTableIndex (
  IN    UINTN             LocalTokenNumberTableIdx,
  IN    PEI_PCD_DATABASE  *Database
  )
{
  UINTN       i;
  UINTN        SizeTableIdx;
  UINTN       LocalTokenNumber;
  SKU_ID      *SkuIdTable;
  
  SizeTableIdx = 0;

  for (i=0; i<LocalTokenNumberTableIdx; i++) {
    LocalTokenNumber = Database->Init.LocalTokenNumberTable[i];

    if ((LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) == PCD_DATUM_TYPE_POINTER) {
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
          SizeTableIdx++;
      } else {
        if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == 0) {
          //
          // We have only two entry for Non-Sku enabled PCD entry:
          // 1) MAX SIZE
          // 2) Current Size
          //
          SizeTableIdx += 2;
        } else {
          //
          // We have these entry for SKU enabled PCD entry
          // 1) MAX SIZE
          // 2) Current Size for each SKU_ID (It is equal to MaxSku).
          //
          SkuIdTable = GetSkuIdArray (i, Database);
          SizeTableIdx += (UINTN)*SkuIdTable + 1;
        }
      }
    }

  }

  return SizeTableIdx;
}
