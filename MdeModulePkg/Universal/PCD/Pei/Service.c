/** @file
  The driver internal functions are implmented here.
  They build Pei PCD database, and provide access service to PCD database.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Service.h"

/**
  The function registers the CallBackOnSet fucntion
  according to TokenNumber and EFI_GUID space.

  @param  ExTokenNumber       The token number.
  @param  Guid              The GUID space.
  @param  CallBackFunction  The Callback function to be registered.
  @param  Register          To register or unregister the callback function.

  @retval EFI_SUCCESS If the Callback function is registered.
  @retval EFI_NOT_FOUND If the PCD Entry is not found according to Token Number and GUID space.
  @retval EFI_OUT_OF_RESOURCES If the callback function can't be registered because there is not free
                                slot left in the CallbackFnTable.
  @retval EFI_INVALID_PARAMETER If the callback function want to be de-registered can not be found.
**/
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
    if (TokenNumber == PCD_INVALID_TOKEN_NUMBER) {
      return EFI_NOT_FOUND;
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
  }


  LocalTokenNumber = GetPcdDatabase()->Init.LocalTokenNumberTable[TokenNumber];

  //
  // We don't support SET for HII and VPD type PCD entry in PEI phase.
  // So we will assert if any register callback for such PCD entry.
  //
  ASSERT ((LocalTokenNumber & PCD_TYPE_HII) == 0);
  ASSERT ((LocalTokenNumber & PCD_TYPE_VPD) == 0);

  GuidHob = GetFirstGuidHob (&gEfiCallerIdGuid);
  ASSERT (GuidHob != NULL);
  
  CallbackTable = GET_GUID_HOB_DATA (GuidHob);
  CallbackTable = CallbackTable + (TokenNumber * PcdGet32 (PcdMaxPeiPcdCallBackNumberPerPcdEntry));

  Compare = Register? NULL: CallBackFunction;
  Assign  = Register? CallBackFunction: NULL;


  for (Idx = 0; Idx < PcdGet32 (PcdMaxPeiPcdCallBackNumberPerPcdEntry); Idx++) {
    if (CallbackTable[Idx] == Compare) {
      CallbackTable[Idx] = Assign;
      return EFI_SUCCESS;
    }
  }

  return Register? EFI_OUT_OF_RESOURCES : EFI_INVALID_PARAMETER;

}

/**
  The function builds the PCD database.
**/
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

  SizeOfCallbackFnTable = PEI_LOCAL_TOKEN_NUMBER * sizeof (PCD_PPI_CALLBACK) * PcdGet32 (PcdMaxPeiPcdCallBackNumberPerPcdEntry);

  CallbackFnTable = BuildGuidHob (&gEfiCallerIdGuid, SizeOfCallbackFnTable);
  
  ZeroMem (CallbackFnTable, SizeOfCallbackFnTable);
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
**/
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
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *VariablePpi;

  Status = PeiServicesLocatePpi (&gEfiPeiReadOnlyVariable2PpiGuid, 0, NULL, (VOID **) &VariablePpi);
  ASSERT_EFI_ERROR (Status);

  Size = 0;
  Status = VariablePpi->GetVariable (
                          VariablePpi,
                          VariableName,
                          (EFI_GUID *) VariableGuid,
                          NULL,
                          &Size,
                          NULL
                          );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    Status = PeiServicesAllocatePool (Size, &Buffer);
    ASSERT_EFI_ERROR (Status);

    Status = VariablePpi->GetVariable (
                              VariablePpi,
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
  }

  return EFI_NOT_FOUND;
}

/**
  Find the local token number according to system SKU ID.

  @param LocalTokenNumber PCD token number
  @param Size             The size of PCD entry.

  @return Token number according to system SKU ID.

**/
UINT32
GetSkuEnabledTokenNumber (
  UINT32 LocalTokenNumber,
  UINTN  Size
  ) 
{
  PEI_PCD_DATABASE      *PeiPcdDb;
  SKU_HEAD              *SkuHead;
  SKU_ID                *SkuIdTable;
  INTN                  Index;
  UINT8                 *Value;

  PeiPcdDb = GetPcdDatabase ();

  ASSERT ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == 0);

  SkuHead     = (SKU_HEAD *) ((UINT8 *)PeiPcdDb + (LocalTokenNumber & PCD_DATABASE_OFFSET_MASK));
  Value       = (UINT8 *) ((UINT8 *)PeiPcdDb + (SkuHead->SkuDataStartOffset));
  SkuIdTable  = (SKU_ID *) ((UINT8 *)PeiPcdDb + (SkuHead->SkuIdTableOffset));
        
  for (Index = 0; Index < SkuIdTable[0]; Index++) {
    if (PeiPcdDb->Init.SystemSkuId == SkuIdTable[Index + 1]) {
      break;
    }
  }

  switch (LocalTokenNumber & PCD_TYPE_ALL_SET) {
    case PCD_TYPE_VPD:
      Value = (UINT8 *) &(((VPD_HEAD *) Value)[Index]);
      return (UINT32) ((Value - (UINT8 *) PeiPcdDb) | PCD_TYPE_VPD);

    case PCD_TYPE_HII:
      Value = (UINT8 *) &(((VARIABLE_HEAD *) Value)[Index]);
      return (UINT32) ((Value - (UINT8 *) PeiPcdDb) | PCD_TYPE_HII);
      
    case PCD_TYPE_STRING:
      Value = (UINT8 *) &(((STRING_HEAD *) Value)[Index]);
      return (UINT32) ((Value - (UINT8 *) PeiPcdDb) | PCD_TYPE_STRING);

    case PCD_TYPE_DATA:
      Value += Size * Index;
      return (UINT32) (Value - (UINT8 *) PeiPcdDb);

    default:
      ASSERT (FALSE);
  }

  ASSERT (FALSE);

  return 0;
}

/**
  Invoke the callback function when dynamic PCD entry was set, if this PCD entry 
  has registered callback function.

  @param ExTokenNumber   DynamicEx PCD's token number, if this PCD entry is dyanmicEx
                         type PCD.
  @param Guid            DynamicEx PCD's guid, if this PCD entry is dynamicEx type
                         PCD.
  @param TokenNumber     PCD token number generated by build tools.
  @param Data            Value want to be set for this PCD entry
  @param Size            The size of value

**/
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

  GuidHob = GetFirstGuidHob (&gEfiCallerIdGuid);
  ASSERT (GuidHob != NULL);
  
  CallbackTable = GET_GUID_HOB_DATA (GuidHob);

  CallbackTable += (TokenNumber * PcdGet32 (PcdMaxPeiPcdCallBackNumberPerPcdEntry));

  for (Idx = 0; Idx < PcdGet32 (PcdMaxPeiPcdCallBackNumberPerPcdEntry); Idx++) {
    if (CallbackTable[Idx] != NULL) {
      CallbackTable[Idx] (Guid,
                          (Guid == NULL) ? (TokenNumber + 1) : ExTokenNumber,
                          Data,
                          Size
                          );
    }
  }
}

/**
  Wrapper function for setting non-pointer type value for a PCD entry.

  @param TokenNumber     Pcd token number autogenerated by build tools.
  @param Data            Value want to be set for PCD entry
  @param Size            Size of value.

  @return status of SetWorker.

**/
EFI_STATUS
SetValueWorker (
  IN          UINTN              TokenNumber,
  IN          VOID               *Data,
  IN          UINTN              Size
  )
{
  return SetWorker (TokenNumber, Data, &Size, FALSE);
}

/**
  Set value for an PCD entry

  @param TokenNumber     Pcd token number autogenerated by build tools.
  @param Data            Value want to be set for PCD entry
  @param Size            Size of value.
  @param PtrType         If TRUE, the type of PCD entry's value is Pointer.
                         If False, the type of PCD entry's value is not Pointer.

  @retval EFI_INVALID_PARAMETER  If this PCD type is VPD, VPD PCD can not be set.
  @retval EFI_INVALID_PARAMETER  If Size can not be set to size table.
  @retval EFI_INVALID_PARAMETER  If Size of non-Ptr type PCD does not match the size information in PCD database.
  @retval EFI_NOT_FOUND          If value type of PCD entry is intergrate, but not in
                                 range of UINT8, UINT16, UINT32, UINT64
  @retval EFI_NOT_FOUND          Can not find the PCD type according to token number.                                
**/
EFI_STATUS
SetWorker (
  IN          UINTN               TokenNumber,
  IN          VOID                *Data,
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

  if (!FeaturePcdGet(PcdPeiFullPcdDatabaseEnable)) {
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

  if (PtrType) {
    //
    // Get MaxSize first, then check new size with max buffer size.
    //
    GetPtrTypeSize (TokenNumber, &MaxSize, PeiPcdDb);
    if (*Size > MaxSize) {
      *Size = MaxSize;
      return EFI_INVALID_PARAMETER;
    }
  } else {
    if (*Size != PeiPcdGetSize (TokenNumber + 1)) {
      return EFI_INVALID_PARAMETER;
    }
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
      GetPtrTypeSize (TokenNumber, &MaxSize, PeiPcdDb);
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
    case PCD_TYPE_HII|PCD_TYPE_STRING:
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

/**
  Wrapper function for set PCD value for non-Pointer type dynamic-ex PCD.

  @param ExTokenNumber   Token number for dynamic-ex PCD.
  @param Guid            Token space guid for dynamic-ex PCD.
  @param Data            Value want to be set.
  @param SetSize         The size of value.

  @return status of ExSetWorker().

**/
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

/**
  Set value for a dynamic PCD entry.
  
  This routine find the local token number according to dynamic-ex PCD's token 
  space guid and token number firstly, and invoke callback function if this PCD
  entry registered callback function. Finally, invoken general SetWorker to set
  PCD value.
  
  @param ExTokenNumber   Dynamic-ex PCD token number.
  @param Guid            Token space guid for dynamic-ex PCD.
  @param Data            PCD value want to be set
  @param SetSize         Size of value.
  @param PtrType         If TRUE, this PCD entry is pointer type.
                         If FALSE, this PCD entry is not pointer type.

  @return status of SetWorker().

**/
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

  if (!FeaturePcdGet(PcdPeiFullPcdDatabaseEnable)) {
    return EFI_UNSUPPORTED;
  }

  TokenNumber = GetExPcdTokenNumber (Guid, ExTokenNumber);
  if (TokenNumber == PCD_INVALID_TOKEN_NUMBER) {
    return EFI_NOT_FOUND;
  }
  
  InvokeCallbackOnSet (ExTokenNumber, Guid, TokenNumber, Data, *Size);

  return SetWorker (TokenNumber, Data, Size, PtrType);

}

/**
  Wrapper function for get PCD value for dynamic-ex PCD.

  @param Guid            Token space guid for dynamic-ex PCD.
  @param ExTokenNumber   Token number for dyanmic-ex PCD.
  @param GetSize         The size of dynamic-ex PCD value.

  @return PCD entry in PCD database.

**/
VOID *
ExGetWorker (
  IN CONST  EFI_GUID  *Guid,
  IN UINTN            ExTokenNumber,
  IN UINTN            GetSize
  )
{ 
  return GetWorker (GetExPcdTokenNumber (Guid, ExTokenNumber), GetSize);
}

/**
  Get the PCD entry pointer in PCD database.
  
  This routine will visit PCD database to find the PCD entry according to given
  token number. The given token number is autogened by build tools and it will be 
  translated to local token number. Local token number contains PCD's type and 
  offset of PCD entry in PCD database.

  @param TokenNumber     Token's number, it is autogened by build tools
  @param GetSize         The size of token's value

  @return PCD entry pointer in PCD database

**/
VOID *
GetWorker (
  IN UINTN               TokenNumber,
  IN UINTN               GetSize
  )
{
  UINT32              Offset;
  EFI_GUID            *Guid;
  UINT16              *Name;
  VARIABLE_HEAD       *VariableHead;
  EFI_STATUS          Status;
  UINTN               DataSize;
  VOID                *Data;
  UINT8               *StringTable;
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
      GetPtrTypeSize (TokenNumber, &MaxSize, PeiPcdDb);
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
      return (VOID *) (UINTN) (PcdGet32 (PcdVpdBaseAddress) + VpdHead->Offset);
    }
      
    case PCD_TYPE_HII|PCD_TYPE_STRING:
    case PCD_TYPE_HII:
    {
      VariableHead = (VARIABLE_HEAD *) ((UINT8 *)PeiPcdDb + Offset);
      
      Guid = &(PeiPcdDb->Init.GuidTable[VariableHead->GuidTableIndex]);
      Name = (UINT16*)&StringTable[VariableHead->StringIndex];

      Status = GetHiiVariable (Guid, Name, &Data, &DataSize);

      if (Status == EFI_SUCCESS) {
        return (VOID *) ((UINT8 *) Data + VariableHead->Offset);
      } else {
        //
        // Return the default value specified by Platform Integrator 
        //
        if ((LocalTokenNumber & PCD_TYPE_ALL_SET) == (PCD_TYPE_HII|PCD_TYPE_STRING)) {
          return (VOID*)&StringTable[*(UINT16*)((UINT8*)PeiPcdDb + VariableHead->DefaultValueOffset)];
        } else {
          return (VOID *) ((UINT8 *) PeiPcdDb + VariableHead->DefaultValueOffset);
        }
      }
    }

    case PCD_TYPE_DATA:
      return (VOID *) ((UINT8 *)PeiPcdDb + Offset);

    case PCD_TYPE_STRING:
      StringTableIdx = * (UINT16*) ((UINT8 *) PeiPcdDb + Offset);
      return (VOID *) (&StringTable[StringTableIdx]);

    default:
      ASSERT (FALSE);
      break;
      
  }

  ASSERT (FALSE);
      
  return NULL;
  
}

/**
  Get local token number according to dynamic-ex PCD's {token space guid:token number}

  A dynamic-ex type PCD, developer must provide pair of token space guid: token number
  in DEC file. PCD database maintain a mapping table that translate pair of {token
  space guid: token number} to local token number.
  
  @param Guid            Token space guid for dynamic-ex PCD entry.
  @param ExTokenNumber   EDES_TODO: Add parameter description

  @return local token number for dynamic-ex PCD.

**/
UINTN           
GetExPcdTokenNumber (
  IN CONST EFI_GUID             *Guid,
  IN UINTN                      ExTokenNumber
  )
{
  UINT32              Index;
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
  
  for (Index = 0; Index < PEI_EXMAPPING_TABLE_SIZE; Index++) {
    if ((ExTokenNumber == ExMap[Index].ExTokenNumber) && 
        (MatchGuidIdx == ExMap[Index].ExGuidIndex)) {
      return ExMap[Index].LocalTokenNumber;
    }
  }
  
  return PCD_INVALID_TOKEN_NUMBER;
}

/**
  Get PCD database from GUID HOB in PEI phase.

  @return Pointer to PCD database.

**/
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

/**
  Get SKU ID tabble from PCD database.

  @param LocalTokenNumberTableIdx Index of local token number in token number table.
  @param Database                 PCD database.

  @return Pointer to SKU ID array table

**/
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

/**
  Get index of PCD entry in size table.

  @param LocalTokenNumberTableIdx Index of this PCD in local token number table.
  @param Database                 Pointer to PCD database in PEI phase.

  @return index of PCD entry in size table.

**/
UINTN
GetSizeTableIndex (
  IN    UINTN             LocalTokenNumberTableIdx,
  IN    PEI_PCD_DATABASE  *Database
  )
{
  UINTN       Index;
  UINTN        SizeTableIdx;
  UINTN       LocalTokenNumber;
  SKU_ID      *SkuIdTable;
  
  SizeTableIdx = 0;

  for (Index=0; Index<LocalTokenNumberTableIdx; Index++) {
    LocalTokenNumber = Database->Init.LocalTokenNumberTable[Index];

    if ((LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) == PCD_DATUM_TYPE_POINTER) {
      //
      // SizeTable only contain record for PCD_DATUM_TYPE_POINTER type 
      // PCD entry.
      //
      if ((LocalTokenNumber & PCD_TYPE_VPD) != 0) {
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
          SkuIdTable = GetSkuIdArray (Index, Database);
          SizeTableIdx += (UINTN)*SkuIdTable + 1;
        }
      }
    }

  }

  return SizeTableIdx;
}
