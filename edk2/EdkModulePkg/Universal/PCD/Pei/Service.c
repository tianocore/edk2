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

  @param[in]  TokenNumber       The token number.
  @param[in]  Guid              The GUID space.
  @param[in]  CallBackFunction  The Callback function to be registered.

  @retval EFI_SUCCESS If the Callback function is registered.
  @retval EFI_NOT_FOUND If the PCD Entry is not found according to Token Number and GUID space.
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
  EX_PCD_ENTRY_ATTRIBUTE  Attr;

  if (Guid == NULL) {
    TokenNumber = ExTokenNumber;
    ASSERT (TokenNumber < PEI_NEX_TOKEN_NUMBER);
    LocalTokenNumber = GetPcdDatabase()->Init.LocalTokenNumberTable[TokenNumber];
  } else {
    GetExPcdTokenAttributes (Guid, ExTokenNumber, &Attr);
    TokenNumber = Attr.TokenNumber;
    LocalTokenNumber = Attr.LocalTokenNumberAlias;
  }

  ASSERT ((LocalTokenNumber & PCD_TYPE_HII) == 0);
  ASSERT ((LocalTokenNumber & PCD_TYPE_VPD) == 0);

  GuidHob = GetFirstGuidHob (&gPcdPeiCallbackFnTableHobGuid);
  ASSERT (GuidHob != NULL);
  
  CallbackTable = GET_GUID_HOB_DATA (GuidHob);

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
  The function builds the PCD database based on the
  PCD_IMAGE on the flash.

  @param[in] PcdImageOnFlash  The PCD image on flash.

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

  @param[in] VariableGuid     The Variable GUID.
  @param[in] VariableName     The Variable Name.
  @param[out] VariableData    The output data.
  @param[out] VariableSize    The size of the variable.

  @retval EFI_SUCCESS         Operation successful.
  @retval EFI_SUCCESS         Variablel not found.
--*/
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

  Status = PeiCoreLocatePpi (&gEfiPeiReadOnlyVariablePpiGuid, 0, NULL, &VariablePpi);
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
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  Status = PeiCoreAllocatePool (Size, &Buffer);
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
}


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

  switch (LocalTokenNumber & ~PCD_DATABASE_OFFSET_MASK) {
    case PCD_TYPE_VPD:
      Value += sizeof(VPD_HEAD) * i;
      return ((Value - (UINT8 *) PeiPcdDb) | PCD_TYPE_VPD);

    case PCD_TYPE_HII:
      Value += sizeof(VARIABLE_HEAD) * i;
      return ((Value - (UINT8 *) PeiPcdDb) | PCD_TYPE_HII);
      
    case PCD_TYPE_DATA:
      Value += Size * i;
      return (Value - (UINT8 *) PeiPcdDb);
      
    default:
      ASSERT (FALSE);
  }

  ASSERT (FALSE);

  return 0;
  
}




VOID
InvokeCallbackOnSet (
  UINT32            ExTokenNumber,
  CONST EFI_GUID    *Guid, OPTIONAL
  UINTN             TokenNumber,
  VOID              *Data,
  UINTN             Size
  )
{
  EFI_HOB_GUID_TYPE   *GuidHob;
  PCD_PPI_CALLBACK    *CallbackTable;
  UINTN               Idx;

  if (Guid == NULL)
    ASSERT (TokenNumber < PEI_LOCAL_TOKEN_NUMBER);

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
SetWorker (
  UINTN         TokenNumber,
  VOID          *Data,
  UINTN         Size,
  BOOLEAN       PtrType
  )
{
  UINT32              LocalTokenNumber;
  PEI_PCD_DATABASE    *PeiPcdDb;

  ASSERT (TokenNumber < PEI_LOCAL_TOKEN_NUMBER);
    
  PeiPcdDb = GetPcdDatabase ();

  LocalTokenNumber = PeiPcdDb->Init.LocalTokenNumberTable[TokenNumber];

  if (PtrType) {
    ASSERT (PeiPcdDb->Init.SizeTable[TokenNumber] >= Size);
  } else {
    ASSERT (PeiPcdDb->Init.SizeTable[TokenNumber] == Size);
  }

  InvokeCallbackOnSet (0, NULL, TokenNumber, Data, Size);

  return SetWorkerByLocalTokenNumber (LocalTokenNumber, Data, Size, PtrType);

}




EFI_STATUS
ExSetWorker (
  IN UINT32               ExTokenNumber,
  IN CONST EFI_GUID       *Guid,
  VOID                    *Data,
  UINTN                   Size,
  BOOLEAN                 PtrType
  )
{
  PEI_PCD_DATABASE          *PeiPcdDb;
  EX_PCD_ENTRY_ATTRIBUTE    Attr;


  PeiPcdDb = GetPcdDatabase ();

  GetExPcdTokenAttributes (Guid, ExTokenNumber, &Attr);

  ASSERT (!PtrType && Attr.Size);

  ASSERT (PtrType && Attr.Size >= Size);

  InvokeCallbackOnSet (ExTokenNumber, Guid, Attr.TokenNumber, Data, Size);

  SetWorkerByLocalTokenNumber (Attr.LocalTokenNumberAlias, Data, Size, PtrType);

  return EFI_SUCCESS;
  
}




EFI_STATUS
SetWorkerByLocalTokenNumber (
  UINT32        LocalTokenNumber,
  VOID          *Data,
  UINTN         Size,
  BOOLEAN       PtrType
  )
{
  PEI_PCD_DATABASE      *PeiPcdDb;
  UINT8                 *PeiPcdDbRaw;
  UINT16                StringTableIdx;
  UINTN                 Offset;
  VOID                  *InternalData;
 

  PeiPcdDb    =    GetPcdDatabase ();
  PeiPcdDbRaw =    (UINT8 *) PeiPcdDb;
  
  if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == PCD_TYPE_SKU_ENABLED) {
    LocalTokenNumber = GetSkuEnabledTokenNumber (LocalTokenNumber & ~PCD_TYPE_SKU_ENABLED, Size);
  }

  Offset          = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;
  InternalData    = (VOID *) (PeiPcdDbRaw + Offset);
  
  switch (LocalTokenNumber & ~PCD_DATABASE_OFFSET_MASK) {
    case PCD_TYPE_VPD:
    case PCD_TYPE_HII:
    {
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    }

    case PCD_TYPE_STRING:
      StringTableIdx = *((UINT16 *)InternalData);
      CopyMem (&PeiPcdDb->Init.StringTable[StringTableIdx], Data, Size);
      break;

    case PCD_TYPE_DATA:
    {
      
      if (PtrType) {
        CopyMem (InternalData, Data, Size);
        return EFI_SUCCESS;
      }

      switch (Size) {
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


VOID *
GetWorkerByLocalTokenNumber (
  PEI_PCD_DATABASE    *PeiPcdDb,
  UINT32              LocalTokenNumber,
  UINTN               Size
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
      
  PeiPcdDb        = GetPcdDatabase ();

  if ((LocalTokenNumber & PCD_TYPE_SKU_ENABLED) == PCD_TYPE_SKU_ENABLED) {
    LocalTokenNumber = GetSkuEnabledTokenNumber (LocalTokenNumber & ~PCD_TYPE_SKU_ENABLED, Size);
  }

  Offset      = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;
  StringTable = PeiPcdDb->Init.StringTable;
  
  switch (LocalTokenNumber & ~PCD_DATABASE_OFFSET_MASK) {
    case PCD_TYPE_VPD:
    {
      VPD_HEAD *VpdHead;
      VpdHead = (VPD_HEAD *) ((UINT8 *)PeiPcdDb + Offset);
      return (VOID *) (FixedPcdGet32(PcdVpdBaseAddress) + VpdHead->Offset);
    }
      
    case PCD_TYPE_HII:
    {
      VariableHead = (VARIABLE_HEAD *) ((UINT8 *)PeiPcdDb + Offset);
      
      Guid = &(PeiPcdDb->Init.GuidTable[VariableHead->GuidTableIndex]);
      Name = &StringTable[VariableHead->StringIndex];

      Status = GetHiiVariable (Guid, Name, &Data, &DataSize);
      ASSERT_EFI_ERROR (Status);
      ASSERT (DataSize >= (UINTN) (VariableHead->Offset + Size));

      return (VOID *) ((UINT8 *) Data + VariableHead->Offset);
    }

    case PCD_TYPE_DATA:
      return (VOID *) ((UINT8 *)PeiPcdDb + Offset);
      break;

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


VOID *
ExGetWorker (
  IN CONST  EFI_GUID  *Guid,
  IN UINT32           ExTokenNumber,
  IN UINTN            GetSize
  )
{
  EX_PCD_ENTRY_ATTRIBUTE      Attr;

  GetExPcdTokenAttributes (Guid, ExTokenNumber, &Attr);
  
  ASSERT ((GetSize == Attr.Size) || (GetSize == 0));

  return GetWorkerByLocalTokenNumber (GetPcdDatabase(),
                                      Attr.LocalTokenNumberAlias,
                                      Attr.Size
                                      );
}

VOID *
GetWorker (
  UINTN  TokenNumber,
  UINTN   GetSize
  )
{
  PEI_PCD_DATABASE      *PeiPcdDb;

  ASSERT (TokenNumber < PEI_LOCAL_TOKEN_NUMBER);

  ASSERT (GetSize == PeiPcdGetSize (TokenNumber) || GetSize == 0);

  PeiPcdDb = GetPcdDatabase ();
  
  return  GetWorkerByLocalTokenNumber (PeiPcdDb, PeiPcdDb->Init.LocalTokenNumberTable[TokenNumber], GetSize);
}


VOID
GetExPcdTokenAttributes (
  IN CONST EFI_GUID             *Guid,
  IN UINT32                     ExTokenNumber,
  OUT EX_PCD_ENTRY_ATTRIBUTE    *ExAttr
  )
{
  UINT32              i;
  DYNAMICEX_MAPPING   *ExMap;
  EFI_GUID            *GuidTable;
  PEI_PCD_DATABASE    *PeiPcdDb;

  PeiPcdDb    = GetPcdDatabase();
  
  ExMap       = PeiPcdDb->Init.ExMapTable;
  GuidTable   = PeiPcdDb->Init.GuidTable;
  
  for (i = 0; i < PEI_EXMAPPING_TABLE_SIZE; i++) {
    if ((ExTokenNumber == ExMap[i].ExTokenNumber) && 
        CompareGuid (Guid, (CONST EFI_GUID *) &GuidTable[ExMap[i].ExGuidIndex])) {
      ExAttr->TokenNumber               = i + PEI_NEX_TOKEN_NUMBER;
      ExAttr->Size                      = PeiPcdDb->Init.SizeTable[i + PEI_NEX_TOKEN_NUMBER];
      ExAttr->LocalTokenNumberAlias     = ExMap[i].LocalTokenNumber;
    }
  }
  
  ASSERT (FALSE);
  
  return;
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

