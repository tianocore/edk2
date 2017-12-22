/** @file
  The driver internal functions are implmented here.
  They build Pei PCD database, and provide access service to PCD database.

Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Service.h"

/**
  Get Local Token Number by Token Number.

  @param[in]    Database    PCD database.
  @param[in]    TokenNumber The PCD token number.

  @return       Local Token Number.
**/
UINT32
GetLocalTokenNumber (
  IN PEI_PCD_DATABASE   *Database,
  IN UINTN              TokenNumber
  )
{
  UINT32                LocalTokenNumber;

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  LocalTokenNumber = *((UINT32 *)((UINT8 *)Database + Database->LocalTokenNumberTableOffset) + TokenNumber);

  return LocalTokenNumber;
}

/**
  Get PCD type by Local Token Number.

  @param[in]    LocalTokenNumber The PCD local token number.

  @return       PCD type.
**/
EFI_PCD_TYPE
GetPcdType (
  IN UINT32             LocalTokenNumber
  )
{
  switch (LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) {
    case PCD_DATUM_TYPE_POINTER:
      return EFI_PCD_TYPE_PTR;
    case PCD_DATUM_TYPE_UINT8:
      if ((LocalTokenNumber & PCD_DATUM_TYPE_UINT8_BOOLEAN) == PCD_DATUM_TYPE_UINT8_BOOLEAN) {
        return EFI_PCD_TYPE_BOOL;
      } else {
        return EFI_PCD_TYPE_8;
      }
    case PCD_DATUM_TYPE_UINT16:
      return EFI_PCD_TYPE_16;
    case PCD_DATUM_TYPE_UINT32:
      return EFI_PCD_TYPE_32;
    case PCD_DATUM_TYPE_UINT64:
      return EFI_PCD_TYPE_64;
    default:
      ASSERT (FALSE);
      return EFI_PCD_TYPE_8;
  }
}

/**
  Get PCD name.

  @param[in]    OnlyTokenSpaceName  If TRUE, only need to get the TokenSpaceCName.
                                    If FALSE, need to get the full PCD name.
  @param[in]    Database            PCD database.
  @param[in]    TokenNumber         The PCD token number.

  @return       The TokenSpaceCName or full PCD name.
**/
CHAR8 *
GetPcdName (
  IN BOOLEAN            OnlyTokenSpaceName,
  IN PEI_PCD_DATABASE   *Database,
  IN UINTN              TokenNumber
  )
{
  UINT8             *StringTable;
  UINTN             NameSize;
  PCD_NAME_INDEX    *PcdNameIndex;
  CHAR8             *TokenSpaceName;
  CHAR8             *PcdName;
  CHAR8             *Name;

  //
  // Return NULL when PCD name table is absent. 
  //
  if (Database->PcdNameTableOffset == 0) {
    return NULL;
  }

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  StringTable = (UINT8 *) Database + Database->StringTableOffset;

  //
  // Get the PCD name index.
  //
  PcdNameIndex = (PCD_NAME_INDEX *)((UINT8 *) Database + Database->PcdNameTableOffset) + TokenNumber;
  TokenSpaceName = (CHAR8 *)&StringTable[PcdNameIndex->TokenSpaceCNameIndex];
  PcdName = (CHAR8 *)&StringTable[PcdNameIndex->PcdCNameIndex];

  if (OnlyTokenSpaceName) {
    //
    // Only need to get the TokenSpaceCName.
    //
    Name = AllocateCopyPool (AsciiStrSize (TokenSpaceName), TokenSpaceName);
  } else {
    //
    // Need to get the full PCD name.
    //
    NameSize = AsciiStrSize (TokenSpaceName) + AsciiStrSize (PcdName);
    Name = AllocateZeroPool (NameSize);
    ASSERT (Name != NULL);
    //
    // Catenate TokenSpaceCName and PcdCName with a '.' to form the full PCD name.
    //
    AsciiStrCatS (Name, NameSize, TokenSpaceName);
    Name[AsciiStrSize (TokenSpaceName) - sizeof (CHAR8)] = '.';
    AsciiStrCatS (Name, NameSize, PcdName);  
  }

  return Name;
}

/**
  Retrieve additional information associated with a PCD token.

  This includes information such as the type of value the TokenNumber is associated with as well as possible
  human readable name that is associated with the token.

  @param[in]    Database    PCD database.
  @param[in]    Guid        The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]    TokenNumber The PCD token number.
  @param[out]   PcdInfo     The returned information associated with the requested TokenNumber.
                            The caller is responsible for freeing the buffer that is allocated by callee for PcdInfo->PcdName. 

  @retval  EFI_SUCCESS      The PCD information was returned successfully
  @retval  EFI_NOT_FOUND    The PCD service could not find the requested token number.
**/
EFI_STATUS
ExGetPcdInfo (
  IN        PEI_PCD_DATABASE    *Database,
  IN CONST  EFI_GUID            *Guid,
  IN        UINTN               TokenNumber,
  OUT       EFI_PCD_INFO        *PcdInfo
  )
{
  UINTN                 GuidTableIdx;
  EFI_GUID              *MatchGuid;
  EFI_GUID              *GuidTable;
  DYNAMICEX_MAPPING     *ExMapTable;
  UINTN                 Index;
  UINT32                LocalTokenNumber;

  GuidTable = (EFI_GUID *)((UINT8 *)Database + Database->GuidTableOffset);
  MatchGuid = ScanGuid (GuidTable, Database->GuidTableCount * sizeof(EFI_GUID), Guid);

  if (MatchGuid == NULL) {
    return EFI_NOT_FOUND;
  }

  GuidTableIdx = MatchGuid - GuidTable;

  ExMapTable = (DYNAMICEX_MAPPING *)((UINT8 *)Database + Database->ExMapTableOffset);

  //
  // Find the PCD by GuidTableIdx and ExTokenNumber in ExMapTable.
  //
  for (Index = 0; Index < Database->ExTokenCount; Index++) {
    if (ExMapTable[Index].ExGuidIndex == GuidTableIdx) {
      if (TokenNumber == PCD_INVALID_TOKEN_NUMBER) {
        //
        // TokenNumber is 0, follow spec to set PcdType to EFI_PCD_TYPE_8,
        // PcdSize to 0 and PcdName to the null-terminated ASCII string
        // associated with the token's namespace Guid.
        //
        PcdInfo->PcdType = EFI_PCD_TYPE_8;
        PcdInfo->PcdSize = 0;
        //
        // Here use one representative in the token space to get the TokenSpaceCName.
        // 
        PcdInfo->PcdName = GetPcdName (TRUE, Database, ExMapTable[Index].TokenNumber);
        return EFI_SUCCESS;
      } else if (ExMapTable[Index].ExTokenNumber == TokenNumber) {
        PcdInfo->PcdSize = PeiPcdGetSize (ExMapTable[Index].TokenNumber);
        LocalTokenNumber = GetLocalTokenNumber (Database, ExMapTable[Index].TokenNumber);
        PcdInfo->PcdType = GetPcdType (LocalTokenNumber);
        PcdInfo->PcdName = GetPcdName (FALSE, Database, ExMapTable[Index].TokenNumber);
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Retrieve additional information associated with a PCD token.

  This includes information such as the type of value the TokenNumber is associated with as well as possible
  human readable name that is associated with the token.

  @param[in]    Guid        The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]    TokenNumber The PCD token number.
  @param[out]   PcdInfo     The returned information associated with the requested TokenNumber.
                            The caller is responsible for freeing the buffer that is allocated by callee for PcdInfo->PcdName.

  @retval  EFI_SUCCESS      The PCD information was returned successfully.
  @retval  EFI_NOT_FOUND    The PCD service could not find the requested token number.
**/
EFI_STATUS
PeiGetPcdInfo (
  IN CONST  EFI_GUID        *Guid,
  IN        UINTN           TokenNumber,
  OUT       EFI_PCD_INFO    *PcdInfo
  )
{
  PEI_PCD_DATABASE      *PeiPcdDb;
  BOOLEAN               PeiExMapTableEmpty;
  UINTN                 PeiNexTokenNumber;
  UINT32                LocalTokenNumber;

  ASSERT (PcdInfo != NULL);

  PeiPcdDb          = GetPcdDatabase ();
  PeiNexTokenNumber = PeiPcdDb->LocalTokenCount - PeiPcdDb->ExTokenCount;

  if (PeiPcdDb->ExTokenCount == 0) {
    PeiExMapTableEmpty = TRUE;
  } else {
    PeiExMapTableEmpty = FALSE;
  }

  if (Guid == NULL) {
    if (TokenNumber > PeiNexTokenNumber) {
      return EFI_NOT_FOUND;
    } else if (TokenNumber == PCD_INVALID_TOKEN_NUMBER) {
      //
      // TokenNumber is 0, follow spec to set PcdType to EFI_PCD_TYPE_8,
      // PcdSize to 0 and PcdName to NULL for default Token Space.
      //
      PcdInfo->PcdType = EFI_PCD_TYPE_8;
      PcdInfo->PcdSize = 0;
      PcdInfo->PcdName = NULL;
    } else {
      PcdInfo->PcdSize = PeiPcdGetSize (TokenNumber);
      LocalTokenNumber = GetLocalTokenNumber (PeiPcdDb, TokenNumber);
      PcdInfo->PcdType = GetPcdType (LocalTokenNumber);
      PcdInfo->PcdName = GetPcdName (FALSE, PeiPcdDb, TokenNumber);
    }
    return EFI_SUCCESS;
  } else {
    if (PeiExMapTableEmpty) {
      return EFI_NOT_FOUND;
    }
    return ExGetPcdInfo (
             PeiPcdDb,
             Guid,
             TokenNumber,
             PcdInfo
             );
  }
}

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
  UINT32                  LocalTokenCount;
  UINTN                   PeiNexTokenNumber;
  UINTN                   TokenNumber;
  UINTN                   Idx;
  PEI_PCD_DATABASE        *PeiPcdDb;

  PeiPcdDb          = GetPcdDatabase();
  LocalTokenCount   = PeiPcdDb->LocalTokenCount;
  PeiNexTokenNumber = PeiPcdDb->LocalTokenCount - PeiPcdDb->ExTokenCount;

  if (Guid == NULL) {
    TokenNumber = ExTokenNumber;
    //
    // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
    // We have to decrement TokenNumber by 1 to make it usable
    // as the array index.
    //
    TokenNumber--;
    ASSERT (TokenNumber + 1 < (PeiNexTokenNumber + 1));
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
    ASSERT ((TokenNumber + 1) < (LocalTokenCount + 1));
  }


  LocalTokenNumber = *((UINT32 *)((UINT8 *)PeiPcdDb + PeiPcdDb->LocalTokenNumberTableOffset) + TokenNumber);

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
  Find the Pcd database. 

  @param  FileHandle  Handle of the file the external PCD database binary located.

  @retval The base address of external PCD database binary.
  @retval NULL         Return NULL if not find.
**/
VOID *
LocateExPcdBinary (
  IN EFI_PEI_FILE_HANDLE    FileHandle
  )
{
  EFI_STATUS            Status;
  VOID                  *PcdDb;

  PcdDb       = NULL;

  ASSERT (FileHandle != NULL);

  Status = PeiServicesFfsFindSectionData (EFI_SECTION_RAW, FileHandle, &PcdDb);
  ASSERT_EFI_ERROR (Status);

  //
  // Check the first bytes (Header Signature Guid) and build version.
  //
  if (!CompareGuid (PcdDb, &gPcdDataBaseSignatureGuid) ||
      (((PEI_PCD_DATABASE *) PcdDb)->BuildVersion != PCD_SERVICE_PEIM_VERSION)) {
    ASSERT (FALSE);
  }
  return PcdDb;
}


/**
  The function builds the PCD database.

  @param  FileHandle  Handle of the file the external PCD database binary located.

  @return Pointer to PCD database.
**/
PEI_PCD_DATABASE *
BuildPcdDatabase (
  IN EFI_PEI_FILE_HANDLE    FileHandle
  )
{
  PEI_PCD_DATABASE       *Database;
  PEI_PCD_DATABASE       *PeiPcdDbBinary;
  VOID                   *CallbackFnTable;
  UINTN                  SizeOfCallbackFnTable;

  //
  // Locate the external PCD database binary for one section of current FFS
  //
  PeiPcdDbBinary = LocateExPcdBinary (FileHandle);

  ASSERT(PeiPcdDbBinary != NULL);

  Database = BuildGuidHob (&gPcdDataBaseHobGuid, PeiPcdDbBinary->Length + PeiPcdDbBinary->UninitDataBaseSize);

  ZeroMem (Database, PeiPcdDbBinary->Length  + PeiPcdDbBinary->UninitDataBaseSize);

  //
  // PeiPcdDbBinary is smaller than Database
  //
  CopyMem (Database, PeiPcdDbBinary, PeiPcdDbBinary->Length);

  SizeOfCallbackFnTable = Database->LocalTokenCount * sizeof (PCD_PPI_CALLBACK) * PcdGet32 (PcdMaxPeiPcdCallBackNumberPerPcdEntry);

  CallbackFnTable = BuildGuidHob (&gEfiCallerIdGuid, SizeOfCallbackFnTable);
  
  ZeroMem (CallbackFnTable, SizeOfCallbackFnTable);

  return Database;
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
  PEI_PCD_DATABASE    *PeiPcdDb;
  UINT32              LocalTokenCount;

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  PeiPcdDb        = GetPcdDatabase ();
  LocalTokenCount = PeiPcdDb->LocalTokenCount;

  if (Guid == NULL) {
    // EBC compiler is very choosy. It may report warning about comparison
    // between UINTN and 0 . So we add 1 in each size of the 
    // comparison.
    ASSERT (TokenNumber + 1 < (LocalTokenCount + 1));
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
  UINTN               PeiNexTokenNumber;
  PEI_PCD_DATABASE    *PeiPcdDb;
  STRING_HEAD         StringTableIdx;
  UINTN               Offset;
  VOID                *InternalData;
  UINTN               MaxSize;
  UINT32              LocalTokenCount;

  if (!FeaturePcdGet(PcdPeiFullPcdDatabaseEnable)) {
    return EFI_UNSUPPORTED;
  }
  
  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;
  PeiPcdDb        = GetPcdDatabase ();
  LocalTokenCount = PeiPcdDb->LocalTokenCount;

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  ASSERT (TokenNumber + 1 < (LocalTokenCount + 1));

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
  PeiNexTokenNumber = PeiPcdDb->LocalTokenCount - PeiPcdDb->ExTokenCount;
  if (TokenNumber + 1 < PeiNexTokenNumber + 1) {
    InvokeCallbackOnSet (0, NULL, TokenNumber + 1, Data, *Size);
  }

  LocalTokenNumber = GetLocalTokenNumber (PeiPcdDb, TokenNumber + 1);

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
        StringTableIdx = *((STRING_HEAD *)InternalData);
        CopyMem ((UINT8 *)PeiPcdDb + PeiPcdDb->StringTableOffset + StringTableIdx, Data, *Size);
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
  Set value for a dynamic-ex PCD entry.
  
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
  STRING_HEAD         StringTableIdx;
  PEI_PCD_DATABASE    *PeiPcdDb;
  UINT32              LocalTokenNumber;
  UINT32              LocalTokenCount;
  UINT8               *VaraiableDefaultBuffer;

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  PeiPcdDb        = GetPcdDatabase ();
  LocalTokenCount = PeiPcdDb->LocalTokenCount;

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the 
  // comparison.
  ASSERT (TokenNumber + 1 < (LocalTokenCount + 1));

  ASSERT ((GetSize == PeiPcdGetSize(TokenNumber + 1)) || (GetSize == 0));

  LocalTokenNumber = GetLocalTokenNumber (PeiPcdDb, TokenNumber + 1);

  Offset      = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;
  StringTable = (UINT8 *)PeiPcdDb + PeiPcdDb->StringTableOffset;

  switch (LocalTokenNumber & PCD_TYPE_ALL_SET) {
    case PCD_TYPE_VPD:
    {
      VPD_HEAD *VpdHead;
      VpdHead = (VPD_HEAD *) ((UINT8 *)PeiPcdDb + Offset);
      return (VOID *) ((UINTN) PcdGet32 (PcdVpdBaseAddress) + VpdHead->Offset);
    }
      
    case PCD_TYPE_HII|PCD_TYPE_STRING:
    case PCD_TYPE_HII:
    {
      VariableHead = (VARIABLE_HEAD *) ((UINT8 *)PeiPcdDb + Offset);
      
      Guid = (EFI_GUID *) ((UINT8 *)PeiPcdDb + PeiPcdDb->GuidTableOffset) + VariableHead->GuidTableIndex;
      Name = (UINT16*)&StringTable[VariableHead->StringIndex];

      if ((LocalTokenNumber & PCD_TYPE_ALL_SET) == (PCD_TYPE_HII|PCD_TYPE_STRING)) {
        //
        // If a HII type PCD's datum type is VOID*, the DefaultValueOffset is the index of 
        // string array in string table.
        //
        VaraiableDefaultBuffer = (UINT8 *) &StringTable[*(STRING_HEAD*)((UINT8*) PeiPcdDb + VariableHead->DefaultValueOffset)];   
      } else {
        VaraiableDefaultBuffer = (UINT8 *) PeiPcdDb + VariableHead->DefaultValueOffset;
      }
      Status = GetHiiVariable (Guid, Name, &Data, &DataSize);
      if ((Status == EFI_SUCCESS) && (DataSize >= (VariableHead->Offset + GetSize))) {
        if (GetSize == 0) {
          //
          // It is a pointer type. So get the MaxSize reserved for
          // this PCD entry.
          //
          GetPtrTypeSize (TokenNumber, &GetSize, PeiPcdDb);
          if (GetSize > (DataSize - VariableHead->Offset)) {
            //
            // Use actual valid size.
            //
            GetSize = DataSize - VariableHead->Offset;
          }
        }
        //
        // If the operation is successful, we copy the data
        // to the default value buffer in the PCD Database.
        //
        CopyMem (VaraiableDefaultBuffer, (UINT8 *) Data + VariableHead->Offset, GetSize);
      }
      return (VOID *) VaraiableDefaultBuffer;
    }

    case PCD_TYPE_DATA:
      return (VOID *) ((UINT8 *)PeiPcdDb + Offset);

    case PCD_TYPE_STRING:
      StringTableIdx = * (STRING_HEAD*) ((UINT8 *) PeiPcdDb + Offset);
      return (VOID *) (&StringTable[StringTableIdx]);

    default:
      ASSERT (FALSE);
      break;
      
  }

  ASSERT (FALSE);
      
  return NULL;
  
}

/**
  Get Token Number according to dynamic-ex PCD's {token space guid:token number}

  A dynamic-ex type PCD, developer must provide pair of token space guid: token number
  in DEC file. PCD database maintain a mapping table that translate pair of {token
  space guid: token number} to Token Number.
  
  @param Guid            Token space guid for dynamic-ex PCD entry.
  @param ExTokenNumber   Dynamic-ex PCD token number.

  @return Token Number for dynamic-ex PCD.

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

  ExMap       = (DYNAMICEX_MAPPING *)((UINT8 *)PeiPcdDb + PeiPcdDb->ExMapTableOffset);
  GuidTable   = (EFI_GUID *)((UINT8 *)PeiPcdDb + PeiPcdDb->GuidTableOffset);

  MatchGuid = ScanGuid (GuidTable, PeiPcdDb->GuidTableCount * sizeof(EFI_GUID), Guid);
  //
  // We need to ASSERT here. If GUID can't be found in GuidTable, this is a
  // error in the BUILD system.
  //
  ASSERT (MatchGuid != NULL);
  
  MatchGuidIdx = MatchGuid - GuidTable;
  
  for (Index = 0; Index < PeiPcdDb->ExTokenCount; Index++) {
    if ((ExTokenNumber == ExMap[Index].ExTokenNumber) && 
        (MatchGuidIdx == ExMap[Index].ExGuidIndex)) {
      return ExMap[Index].TokenNumber;
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
  UINTN       SizeTableIdx;
  UINTN       LocalTokenNumber;

  SizeTableIdx = 0;

  for (Index = 0; Index < LocalTokenNumberTableIdx; Index++) {
    LocalTokenNumber = *((UINT32 *)((UINT8 *)Database + Database->LocalTokenNumberTableOffset) + Index);

    if ((LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) == PCD_DATUM_TYPE_POINTER) {
      //
      // SizeTable only contain record for PCD_DATUM_TYPE_POINTER type 
      // PCD entry.
      //
      if ((LocalTokenNumber & PCD_TYPE_VPD) != 0) {
          //
          // We have only two entry for VPD enabled PCD entry:
          // 1) MAX Size.
          // 2) Current Size
          // Current size is equal to MAX size.
          //
          SizeTableIdx += 2;
      } else {
          //
          // We have only two entry for Non-Sku enabled PCD entry:
          // 1) MAX SIZE
          // 2) Current Size
          //
          SizeTableIdx += 2;
      }
    }

  }

  return SizeTableIdx;
}
