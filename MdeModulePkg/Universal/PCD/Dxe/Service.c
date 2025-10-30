/** @file
    Help functions used by PCD DXE driver.

Copyright (c) 2014, Hewlett-Packard Development Company, L.P.<BR>
Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016-2021 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Service.h"
#include <Library/DxeServicesLib.h>

PCD_DATABASE  mPcdDatabase;

UINT32  mPcdTotalTokenCount;
UINT32  mPeiLocalTokenCount;
UINT32  mDxeLocalTokenCount;
UINT32  mPeiNexTokenCount;
UINT32  mDxeNexTokenCount;
UINT32  mPeiExMapppingTableSize;
UINT32  mDxeExMapppingTableSize;
UINT32  mPeiGuidTableSize;
UINT32  mDxeGuidTableSize;

BOOLEAN  mPeiExMapTableEmpty;
BOOLEAN  mDxeExMapTableEmpty;
BOOLEAN  mPeiDatabaseEmpty;

LIST_ENTRY  *mCallbackFnTable;
EFI_GUID    **TmpTokenSpaceBuffer;
UINTN       TmpTokenSpaceBufferCount;

UINTN             mPeiPcdDbSize    = 0;
PEI_PCD_DATABASE  *mPeiPcdDbBinary = NULL;
UINTN             mDxePcdDbSize    = 0;
DXE_PCD_DATABASE  *mDxePcdDbBinary = NULL;

/**
  Get Local Token Number by Token Number.

  @param[in]    IsPeiDb     If TRUE, the pcd entry is initialized in PEI phase,
                            If FALSE, the pcd entry is initialized in DXE phase.
  @param[in]    TokenNumber The PCD token number.

  @return       Local Token Number.
**/
UINT32
GetLocalTokenNumber (
  IN BOOLEAN  IsPeiDb,
  IN UINTN    TokenNumber
  )
{
  UINT32  *LocalTokenNumberTable;

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  LocalTokenNumberTable = IsPeiDb ? (UINT32 *)((UINT8 *)mPcdDatabase.PeiDb + mPcdDatabase.PeiDb->LocalTokenNumberTableOffset) :
                          (UINT32 *)((UINT8 *)mPcdDatabase.DxeDb + mPcdDatabase.DxeDb->LocalTokenNumberTableOffset);
  TokenNumber = IsPeiDb ? TokenNumber : TokenNumber - mPeiLocalTokenCount;

  return LocalTokenNumberTable[TokenNumber];
}

/**
  Get PCD type by Local Token Number.

  @param[in]    LocalTokenNumber The PCD local token number.

  @return       PCD type.
**/
EFI_PCD_TYPE
GetPcdType (
  IN UINT32  LocalTokenNumber
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
  @param[in]    IsPeiDb             If TRUE, the pcd entry is initialized in PEI phase,
                                    If FALSE, the pcd entry is initialized in DXE phase.
  @param[in]    TokenNumber         The PCD token number.

  @return       The TokenSpaceCName or full PCD name.
**/
CHAR8 *
GetPcdName (
  IN BOOLEAN  OnlyTokenSpaceName,
  IN BOOLEAN  IsPeiDb,
  IN UINTN    TokenNumber
  )
{
  PCD_DATABASE_INIT  *Database;
  UINT8              *StringTable;
  UINTN              NameSize;
  PCD_NAME_INDEX     *PcdNameIndex;
  CHAR8              *TokenSpaceName;
  CHAR8              *PcdName;
  CHAR8              *Name;

  //
  // Return NULL when PCD name table is absent.
  //
  if (IsPeiDb) {
    if (mPcdDatabase.PeiDb->PcdNameTableOffset == 0) {
      return NULL;
    }
  } else {
    if (mPcdDatabase.DxeDb->PcdNameTableOffset == 0) {
      return NULL;
    }
  }

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  Database    = IsPeiDb ? mPcdDatabase.PeiDb : mPcdDatabase.DxeDb;
  TokenNumber = IsPeiDb ? TokenNumber : TokenNumber - mPeiLocalTokenCount;

  StringTable = (UINT8 *)Database + Database->StringTableOffset;

  //
  // Get the PCD name index.
  //
  PcdNameIndex   = (PCD_NAME_INDEX *)((UINT8 *)Database + Database->PcdNameTableOffset) + TokenNumber;
  TokenSpaceName = (CHAR8 *)&StringTable[PcdNameIndex->TokenSpaceCNameIndex];
  PcdName        = (CHAR8 *)&StringTable[PcdNameIndex->PcdCNameIndex];

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
    Name     = AllocateZeroPool (NameSize);
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

  @param[in]    IsPeiDb     If TRUE, the pcd entry is initialized in PEI phase,
                            If FALSE, the pcd entry is initialized in DXE phase.
  @param[in]    Guid        The 128-bit unique value that designates the namespace from which to extract the value.
  @param[in]    TokenNumber The PCD token number.
  @param[out]   PcdInfo     The returned information associated with the requested TokenNumber.
                            The caller is responsible for freeing the buffer that is allocated by callee for PcdInfo->PcdName.

  @retval  EFI_SUCCESS      The PCD information was returned successfully
  @retval  EFI_NOT_FOUND    The PCD service could not find the requested token number.
**/
EFI_STATUS
ExGetPcdInfo (
  IN        BOOLEAN       IsPeiDb,
  IN CONST  EFI_GUID      *Guid,
  IN        UINTN         TokenNumber,
  OUT       EFI_PCD_INFO  *PcdInfo
  )
{
  PCD_DATABASE_INIT  *Database;
  UINTN              GuidTableIdx;
  EFI_GUID           *MatchGuid;
  EFI_GUID           *GuidTable;
  DYNAMICEX_MAPPING  *ExMapTable;
  UINTN              Index;
  UINT32             LocalTokenNumber;

  Database = IsPeiDb ? mPcdDatabase.PeiDb : mPcdDatabase.DxeDb;

  GuidTable = (EFI_GUID *)((UINT8 *)Database + Database->GuidTableOffset);
  MatchGuid = ScanGuid (GuidTable, Database->GuidTableCount * sizeof (EFI_GUID), Guid);

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
        PcdInfo->PcdName = GetPcdName (TRUE, IsPeiDb, ExMapTable[Index].TokenNumber);
        return EFI_SUCCESS;
      } else if (ExMapTable[Index].ExTokenNumber == TokenNumber) {
        PcdInfo->PcdSize = DxePcdGetSize (ExMapTable[Index].TokenNumber);
        LocalTokenNumber = GetLocalTokenNumber (IsPeiDb, ExMapTable[Index].TokenNumber);
        PcdInfo->PcdType = GetPcdType (LocalTokenNumber);
        PcdInfo->PcdName = GetPcdName (FALSE, IsPeiDb, ExMapTable[Index].TokenNumber);
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
DxeGetPcdInfo (
  IN CONST  EFI_GUID      *Guid,
  IN        UINTN         TokenNumber,
  OUT       EFI_PCD_INFO  *PcdInfo
  )
{
  EFI_STATUS  Status;
  BOOLEAN     PeiExMapTableEmpty;
  BOOLEAN     DxeExMapTableEmpty;
  UINT32      LocalTokenNumber;
  BOOLEAN     IsPeiDb;

  ASSERT (PcdInfo != NULL);

  Status             = EFI_NOT_FOUND;
  PeiExMapTableEmpty = mPeiExMapTableEmpty;
  DxeExMapTableEmpty = mDxeExMapTableEmpty;

  if (Guid == NULL) {
    if (((TokenNumber + 1 > mPeiNexTokenCount + 1) && (TokenNumber + 1 <= mPeiLocalTokenCount + 1)) ||
        ((TokenNumber + 1 > (mPeiLocalTokenCount + mDxeNexTokenCount + 1))))
    {
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
      PcdInfo->PcdSize = DxePcdGetSize (TokenNumber);
      IsPeiDb          = FALSE;
      if ((TokenNumber + 1 <= mPeiNexTokenCount + 1)) {
        IsPeiDb = TRUE;
      }

      LocalTokenNumber = GetLocalTokenNumber (IsPeiDb, TokenNumber);
      PcdInfo->PcdType = GetPcdType (LocalTokenNumber);
      PcdInfo->PcdName = GetPcdName (FALSE, IsPeiDb, TokenNumber);
    }

    return EFI_SUCCESS;
  }

  if (PeiExMapTableEmpty && DxeExMapTableEmpty) {
    return EFI_NOT_FOUND;
  }

  if (!PeiExMapTableEmpty) {
    Status = ExGetPcdInfo (
               TRUE,
               Guid,
               TokenNumber,
               PcdInfo
               );
  }

  if (Status == EFI_SUCCESS) {
    return Status;
  }

  if (!DxeExMapTableEmpty) {
    Status = ExGetPcdInfo (
               FALSE,
               Guid,
               TokenNumber,
               PcdInfo
               );
  }

  return Status;
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
  IN UINTN  TokenNumber,
  IN UINTN  GetSize
  )
{
  EFI_GUID       *GuidTable;
  UINT8          *StringTable;
  EFI_GUID       *Guid;
  UINT16         *Name;
  VARIABLE_HEAD  *VariableHead;
  UINT8          *VaraiableDefaultBuffer;
  UINT8          *Data;
  VPD_HEAD       *VpdHead;
  UINT8          *PcdDb;
  VOID           *RetPtr;
  UINTN          TmpTokenNumber;
  UINTN          DataSize;
  EFI_STATUS     Status;
  UINT32         LocalTokenNumber;
  UINT32         Offset;
  STRING_HEAD    StringTableIdx;
  BOOLEAN        IsPeiDb;

  //
  // Aquire lock to prevent reentrance from TPL_CALLBACK level
  //
  EfiAcquireLock (&mPcdDatabaseLock);

  RetPtr = NULL;

  ASSERT (TokenNumber > 0);
  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  TmpTokenNumber = TokenNumber;

  //
  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the
  // comparison.
  //
  ASSERT (TokenNumber + 1 < mPcdTotalTokenCount + 1);

  ASSERT ((GetSize == DxePcdGetSize (TokenNumber + 1)) || (GetSize == 0));

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the
  // comparison.
  IsPeiDb = (BOOLEAN)((TokenNumber + 1 < mPeiLocalTokenCount + 1) ? TRUE : FALSE);

  LocalTokenNumber = GetLocalTokenNumber (IsPeiDb, TokenNumber + 1);

  PcdDb = IsPeiDb ? ((UINT8 *)mPcdDatabase.PeiDb) : ((UINT8 *)mPcdDatabase.DxeDb);

  if (IsPeiDb) {
    StringTable = (UINT8 *)((UINT8 *)mPcdDatabase.PeiDb + mPcdDatabase.PeiDb->StringTableOffset);
  } else {
    StringTable = (UINT8 *)((UINT8 *)mPcdDatabase.DxeDb + mPcdDatabase.DxeDb->StringTableOffset);
  }

  Offset = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;

  switch (LocalTokenNumber & PCD_TYPE_ALL_SET) {
    case PCD_TYPE_VPD:
      VpdHead = (VPD_HEAD *)((UINT8 *)PcdDb + Offset);
      ASSERT (mVpdBaseAddress != 0);
      RetPtr = (VOID *)(mVpdBaseAddress + VpdHead->Offset);

      break;

    case PCD_TYPE_HII|PCD_TYPE_STRING:
    case PCD_TYPE_HII:
      if (IsPeiDb) {
        GuidTable = (EFI_GUID *)((UINT8 *)mPcdDatabase.PeiDb + mPcdDatabase.PeiDb->GuidTableOffset);
      } else {
        GuidTable = (EFI_GUID *)((UINT8 *)mPcdDatabase.DxeDb + mPcdDatabase.DxeDb->GuidTableOffset);
      }

      VariableHead = (VARIABLE_HEAD *)(PcdDb + Offset);
      Guid         = GuidTable + VariableHead->GuidTableIndex;
      Name         = (UINT16 *)(StringTable + VariableHead->StringIndex);

      if ((LocalTokenNumber & PCD_TYPE_ALL_SET) == (PCD_TYPE_HII|PCD_TYPE_STRING)) {
        //
        // If a HII type PCD's datum type is VOID*, the DefaultValueOffset is the index of
        // string array in string table.
        //
        StringTableIdx         = *(STRING_HEAD *)((UINT8 *)PcdDb + VariableHead->DefaultValueOffset);
        VaraiableDefaultBuffer = (UINT8 *)(StringTable + StringTableIdx);
      } else {
        VaraiableDefaultBuffer = (UINT8 *)PcdDb + VariableHead->DefaultValueOffset;
      }

      Status = GetHiiVariable (Guid, Name, &Data, &DataSize);
      if (Status == EFI_SUCCESS) {
        if (DataSize >= (VariableHead->Offset + GetSize)) {
          if (GetSize == 0) {
            //
            // It is a pointer type. So get the MaxSize reserved for
            // this PCD entry.
            //
            GetPtrTypeSize (TmpTokenNumber, &GetSize);
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
          // So that we can free the Data allocated in GetHiiVariable.
          //
          CopyMem (VaraiableDefaultBuffer, Data + VariableHead->Offset, GetSize);
        }

        FreePool (Data);
      }

      RetPtr = (VOID *)VaraiableDefaultBuffer;
      break;

    case PCD_TYPE_STRING:
      StringTableIdx = *(STRING_HEAD *)((UINT8 *)PcdDb + Offset);
      RetPtr         = (VOID *)(StringTable + StringTableIdx);
      break;

    case PCD_TYPE_DATA:
      RetPtr = (VOID *)((UINT8 *)PcdDb + Offset);
      break;

    default:
      ASSERT (FALSE);
      break;
  }

  EfiReleaseLock (&mPcdDatabaseLock);

  return RetPtr;
}

/**
  Register the callback function for a PCD entry.

  This routine will register a callback function to a PCD entry by given token number
  and token space guid.

  @param TokenNumber        PCD token's number, it is autogened by build tools.
  @param Guid               PCD token space's guid,
                            if not NULL, this PCD is dynamicEx type PCD.
  @param CallBackFunction   Callback function pointer

  @return EFI_SUCCESS Always success for registering callback function.

**/
EFI_STATUS
DxeRegisterCallBackWorker (
  IN  UINTN                  TokenNumber,
  IN  CONST EFI_GUID         *Guid  OPTIONAL,
  IN  PCD_PROTOCOL_CALLBACK  CallBackFunction
  )
{
  CALLBACK_FN_ENTRY  *FnTableEntry;
  LIST_ENTRY         *ListHead;
  LIST_ENTRY         *ListNode;

  if (Guid != NULL) {
    TokenNumber = GetExPcdTokenNumber (Guid, (UINT32)TokenNumber);
  }

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index of mCallbackFnTable[].
  //
  ListHead = &mCallbackFnTable[TokenNumber - 1];
  ListNode = GetFirstNode (ListHead);

  while (ListNode != ListHead) {
    FnTableEntry = CR_FNENTRY_FROM_LISTNODE (ListNode, CALLBACK_FN_ENTRY, Node);

    if (FnTableEntry->CallbackFn == CallBackFunction) {
      //
      // We only allow a Callback function to be register once
      // for a TokenNumber. So just return EFI_SUCCESS
      //
      return EFI_SUCCESS;
    }

    ListNode = GetNextNode (ListHead, ListNode);
  }

  FnTableEntry = AllocatePool (sizeof (CALLBACK_FN_ENTRY));
  ASSERT (FnTableEntry != NULL);

  FnTableEntry->CallbackFn = CallBackFunction;
  InsertTailList (ListHead, &FnTableEntry->Node);

  return EFI_SUCCESS;
}

/**
  UnRegister the callback function for a PCD entry.

  This routine will unregister a callback function to a PCD entry by given token number
  and token space guid.

  @param TokenNumber        PCD token's number, it is autogened by build tools.
  @param Guid               PCD token space's guid.
                            if not NULL, this PCD is dynamicEx type PCD.
  @param CallBackFunction   Callback function pointer

  @retval EFI_SUCCESS               Callback function is success to be unregister.
  @retval EFI_INVALID_PARAMETER     Can not find the PCD entry by given token number.
**/
EFI_STATUS
DxeUnRegisterCallBackWorker (
  IN  UINTN                  TokenNumber,
  IN  CONST EFI_GUID         *Guid  OPTIONAL,
  IN  PCD_PROTOCOL_CALLBACK  CallBackFunction
  )
{
  CALLBACK_FN_ENTRY  *FnTableEntry;
  LIST_ENTRY         *ListHead;
  LIST_ENTRY         *ListNode;

  if (Guid != NULL) {
    TokenNumber = GetExPcdTokenNumber (Guid, (UINT32)TokenNumber);
  }

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index of mCallbackFnTable[].
  //
  ListHead = &mCallbackFnTable[TokenNumber - 1];
  ListNode = GetFirstNode (ListHead);

  while (ListNode != ListHead) {
    FnTableEntry = CR_FNENTRY_FROM_LISTNODE (ListNode, CALLBACK_FN_ENTRY, Node);

    if (FnTableEntry->CallbackFn == CallBackFunction) {
      //
      // We only allow a Callback function to be register once
      // for a TokenNumber. So we can safely remove the Node from
      // the Link List and return EFI_SUCCESS.
      //
      RemoveEntryList (ListNode);
      FreePool (FnTableEntry);

      return EFI_SUCCESS;
    }

    ListNode = GetNextNode (ListHead, ListNode);
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Get next token number in given token space.

  This routine is used for dynamicEx type PCD. It will firstly scan token space
  table to get token space according to given token space guid. Then scan given
  token number in found token space, if found, then return next token number in
  this token space.

  @param Guid            Token space guid. Next token number will be scaned in
                         this token space.
  @param TokenNumber     Token number.
                         If PCD_INVALID_TOKEN_NUMBER, return first token number in
                         token space table.
                         If not PCD_INVALID_TOKEN_NUMBER, return next token number
                         in token space table.
  @param GuidTable       Token space guid table. It will be used for scan token space
                         by given token space guid.
  @param SizeOfGuidTable The size of guid table.
  @param ExMapTable      DynamicEx token number mapping table.
  @param SizeOfExMapTable The size of dynamicEx token number mapping table.

  @retval EFI_NOT_FOUND  Can not given token space or token number.
  @retval EFI_SUCCESS    Success to get next token number.

**/
EFI_STATUS
ExGetNextTokeNumber (
  IN      CONST EFI_GUID     *Guid,
  IN OUT  UINTN              *TokenNumber,
  IN      EFI_GUID           *GuidTable,
  IN      UINTN              SizeOfGuidTable,
  IN      DYNAMICEX_MAPPING  *ExMapTable,
  IN      UINTN              SizeOfExMapTable
  )
{
  EFI_GUID  *MatchGuid;
  UINTN     Index;
  UINTN     GuidTableIdx;
  BOOLEAN   Found;
  UINTN     ExMapTableCount;

  //
  // Scan token space guid
  //
  MatchGuid = ScanGuid (GuidTable, SizeOfGuidTable, Guid);
  if (MatchGuid == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Find the token space table in dynamicEx mapping table.
  //
  Found           = FALSE;
  GuidTableIdx    = MatchGuid - GuidTable;
  ExMapTableCount = SizeOfExMapTable / sizeof (ExMapTable[0]);
  for (Index = 0; Index < ExMapTableCount; Index++) {
    if (ExMapTable[Index].ExGuidIndex == GuidTableIdx) {
      Found = TRUE;
      break;
    }
  }

  if (Found) {
    //
    // If given token number is PCD_INVALID_TOKEN_NUMBER, then return the first
    // token number in found token space.
    //
    if (*TokenNumber == PCD_INVALID_TOKEN_NUMBER) {
      *TokenNumber = ExMapTable[Index].ExTokenNumber;
      return EFI_SUCCESS;
    }

    for ( ; Index < ExMapTableCount; Index++) {
      if ((ExMapTable[Index].ExTokenNumber == *TokenNumber) && (ExMapTable[Index].ExGuidIndex == GuidTableIdx)) {
        break;
      }
    }

    while (Index < ExMapTableCount) {
      Index++;
      if (Index == ExMapTableCount) {
        //
        // Exceed the length of ExMap Table
        //
        *TokenNumber = PCD_INVALID_TOKEN_NUMBER;
        return EFI_NOT_FOUND;
      } else if (ExMapTable[Index].ExGuidIndex == GuidTableIdx) {
        //
        // Found the next match
        //
        *TokenNumber = ExMapTable[Index].ExTokenNumber;
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Find the PCD database.

  @retval The base address of external PCD database binary.
  @retval NULL         Return NULL if not find.
**/
DXE_PCD_DATABASE *
LocateExPcdBinary (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Search the External Pcd database from one section of current FFS,
  // and read it to memory
  //
  Status = GetSectionFromFfs (
             EFI_SECTION_RAW,
             0,
             (VOID **)&mDxePcdDbBinary,
             &mDxePcdDbSize
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Check the first bytes (Header Signature Guid) and build version.
  //
  if (!CompareGuid ((VOID *)mDxePcdDbBinary, &gPcdDataBaseSignatureGuid) ||
      (mDxePcdDbBinary->BuildVersion != PCD_SERVICE_DXE_VERSION))
  {
    ASSERT (FALSE);
  }

  return mDxePcdDbBinary;
}

/**
  Update PCD database base on current SkuId

  @param   SkuId     Current SkuId
  @param   IsPeiDb   Whether to update PEI PCD database.

  @retval EFI_SUCCESS    Update PCD database successfully.
  @retval EFI_NOT_FOUND  Not found PCD database for current SkuId.
**/
EFI_STATUS
UpdatePcdDatabase (
  IN SKU_ID   SkuId,
  IN BOOLEAN  IsPeiDb
  )
{
  UINTN                   Index;
  PCD_DATABASE_SKU_DELTA  *SkuDelta;
  PCD_DATA_DELTA          *SkuDeltaData;

  if (IsPeiDb && (mPeiPcdDbBinary != NULL)) {
    //
    // Find the delta data for PEI DB
    //
    Index    = (mPcdDatabase.PeiDb->Length + 7) & (~7);
    SkuDelta = NULL;
    while (Index < mPeiPcdDbSize) {
      SkuDelta = (PCD_DATABASE_SKU_DELTA *)((UINT8 *)mPeiPcdDbBinary + Index);
      if ((SkuDelta->SkuId == SkuId) && (SkuDelta->SkuIdCompared == 0)) {
        break;
      }

      Index = (Index + SkuDelta->Length + 7) & (~7);
    }

    //
    // Patch the delta data into current PCD database
    //
    if ((Index < mPeiPcdDbSize) && (SkuDelta != NULL)) {
      SkuDeltaData = (PCD_DATA_DELTA *)(SkuDelta + 1);
      while ((UINT8 *)SkuDeltaData < (UINT8 *)SkuDelta + SkuDelta->Length) {
        *((UINT8 *)mPcdDatabase.PeiDb + SkuDeltaData->Offset) = (UINT8)SkuDeltaData->Value;
        SkuDeltaData++;
      }
    } else {
      return EFI_NOT_FOUND;
    }
  }

  //
  // Find the delta data for DXE DB
  //
  Index    = (mPcdDatabase.DxeDb->Length + 7) & (~7);
  SkuDelta = NULL;

  if (Index == mDxePcdDbSize) {
    return EFI_SUCCESS;
  }

  while (Index < mDxePcdDbSize) {
    SkuDelta = (PCD_DATABASE_SKU_DELTA *)((UINT8 *)mDxePcdDbBinary + Index);
    if ((SkuDelta->SkuId == SkuId) && (SkuDelta->SkuIdCompared == 0)) {
      break;
    }

    Index = (Index + SkuDelta->Length + 7) & (~7);
  }

  //
  // Patch the delta data into current PCD database
  //
  if ((Index < mDxePcdDbSize) && (SkuDelta != NULL)) {
    SkuDeltaData = (PCD_DATA_DELTA *)(SkuDelta + 1);
    while ((UINT8 *)SkuDeltaData < (UINT8 *)SkuDelta + SkuDelta->Length) {
      *((UINT8 *)mPcdDatabase.DxeDb + SkuDeltaData->Offset) = (UINT8)SkuDeltaData->Value;
      SkuDeltaData++;
    }

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Initialize the PCD database in DXE phase.

  PCD database in DXE phase also contains PCD database in PEI phase which is copied
  from GUID Hob.

**/
VOID
BuildPcdDxeDataBase (
  VOID
  )
{
  PEI_PCD_DATABASE   *PeiDatabase;
  EFI_HOB_GUID_TYPE  *GuidHob;
  UINTN              Index;
  UINT32             PcdDxeDbLen;
  VOID               *PcdDxeDb;
  EFI_STATUS         Status;

  //
  // Assign PCD Entries with default value to PCD DATABASE
  //
  mPcdDatabase.DxeDb = LocateExPcdBinary ();
  ASSERT (mPcdDatabase.DxeDb != NULL);
  PcdDxeDbLen = mPcdDatabase.DxeDb->Length + mPcdDatabase.DxeDb->UninitDataBaseSize;
  PcdDxeDb    = AllocateZeroPool (PcdDxeDbLen);
  ASSERT (PcdDxeDb != NULL);
  CopyMem (PcdDxeDb, mPcdDatabase.DxeDb, mPcdDatabase.DxeDb->Length);
  mPcdDatabase.DxeDb = PcdDxeDb;

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  if (GuidHob != NULL) {
    //
    // If no PEIMs use dynamic Pcd Entry, the Pcd Service PEIM
    // should not be included at all. So the GuidHob could
    // be NULL. If it is NULL, we just copy over the DXE Default
    // Value to PCD Database.
    //
    PeiDatabase = (PEI_PCD_DATABASE *)GET_GUID_HOB_DATA (GuidHob);

    //
    // Get next one that stores full PEI data
    //
    GuidHob = GetNextGuidHob (&gPcdDataBaseHobGuid, GET_NEXT_HOB (GuidHob));
    if (GuidHob != NULL) {
      mPeiPcdDbBinary = (PEI_PCD_DATABASE *)GET_GUID_HOB_DATA (GuidHob);
      mPeiPcdDbSize   = (UINTN)GET_GUID_HOB_DATA_SIZE (GuidHob);
    }

    //
    // Assign PCD Entries refereneced in PEI phase to PCD DATABASE
    //
    mPcdDatabase.PeiDb = PeiDatabase;
    //
    // Inherit the SystemSkuId from PEI phase.
    //
    if (mPcdDatabase.PeiDb->SystemSkuId != 0) {
      Status = UpdatePcdDatabase (mPcdDatabase.PeiDb->SystemSkuId, FALSE);
      ASSERT_EFI_ERROR (Status);
    }

    mPcdDatabase.DxeDb->SystemSkuId = mPcdDatabase.PeiDb->SystemSkuId;
  } else {
    mPcdDatabase.PeiDb = AllocateZeroPool (sizeof (PEI_PCD_DATABASE));
    ASSERT (mPcdDatabase.PeiDb != NULL);
  }

  //
  // Initialized the external PCD database local variables
  //
  mPeiLocalTokenCount = mPcdDatabase.PeiDb->LocalTokenCount;
  mDxeLocalTokenCount = mPcdDatabase.DxeDb->LocalTokenCount;

  mPeiExMapppingTableSize = mPcdDatabase.PeiDb->ExTokenCount * sizeof (DYNAMICEX_MAPPING);
  mDxeExMapppingTableSize = mPcdDatabase.DxeDb->ExTokenCount * sizeof (DYNAMICEX_MAPPING);
  mPeiGuidTableSize       = mPcdDatabase.PeiDb->GuidTableCount * sizeof (GUID);
  mDxeGuidTableSize       = mPcdDatabase.DxeDb->GuidTableCount * sizeof (GUID);

  mPcdTotalTokenCount = mPeiLocalTokenCount + mDxeLocalTokenCount;
  mPeiNexTokenCount   = mPeiLocalTokenCount - mPcdDatabase.PeiDb->ExTokenCount;
  mDxeNexTokenCount   = mDxeLocalTokenCount - mPcdDatabase.DxeDb->ExTokenCount;

  mPeiExMapTableEmpty = (mPcdDatabase.PeiDb->ExTokenCount == 0) ? TRUE : FALSE;
  mDxeExMapTableEmpty = (mPcdDatabase.DxeDb->ExTokenCount == 0) ? TRUE : FALSE;
  mPeiDatabaseEmpty   = (mPeiLocalTokenCount == 0) ? TRUE : FALSE;

  TmpTokenSpaceBufferCount = mPcdDatabase.PeiDb->ExTokenCount + mPcdDatabase.DxeDb->ExTokenCount;
  TmpTokenSpaceBuffer      = (EFI_GUID **)AllocateZeroPool (TmpTokenSpaceBufferCount * sizeof (EFI_GUID *));

  //
  // Initialized the Callback Function Table
  //
  mCallbackFnTable = AllocateZeroPool (mPcdTotalTokenCount * sizeof (LIST_ENTRY));
  ASSERT (mCallbackFnTable != NULL);

  //
  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the
  // comparison.
  //
  for (Index = 0; Index + 1 < mPcdTotalTokenCount + 1; Index++) {
    InitializeListHead (&mCallbackFnTable[Index]);
  }
}

/**
  Get Variable which contains HII type PCD entry.

  @param VariableGuid    Variable's guid
  @param VariableName    Variable's unicode name string
  @param VariableData    Variable's data pointer,
  @param VariableSize    Variable's size.

  @return the status of gRT->GetVariable
**/
EFI_STATUS
GetHiiVariable (
  IN  EFI_GUID  *VariableGuid,
  IN  UINT16    *VariableName,
  OUT UINT8     **VariableData,
  OUT UINTN     *VariableSize
  )
{
  UINTN       Size;
  EFI_STATUS  Status;
  UINT8       *Buffer;

  Size   = 0;
  Buffer = NULL;

  //
  // Firstly get the real size of HII variable
  //
  Status = gRT->GetVariable (
                  (UINT16 *)VariableName,
                  VariableGuid,
                  NULL,
                  &Size,
                  Buffer
                  );

  //
  // Allocate buffer to hold whole variable data according to variable size.
  //
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = (UINT8 *)AllocatePool (Size);

    ASSERT (Buffer != NULL);

    Status = gRT->GetVariable (
                    VariableName,
                    VariableGuid,
                    NULL,
                    &Size,
                    Buffer
                    );

    ASSERT (Status == EFI_SUCCESS);
    *VariableData = Buffer;
    *VariableSize = Size;
  } else {
    //
    // Use Default Data only when variable is not found.
    // For other error status, correct data can't be got, and trig ASSERT().
    //
    ASSERT (Status == EFI_NOT_FOUND);
  }

  return Status;
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
  UINT32          ExTokenNumber,
  CONST EFI_GUID  *Guid  OPTIONAL,
  UINTN           TokenNumber,
  VOID            *Data,
  UINTN           Size
  )
{
  CALLBACK_FN_ENTRY  *FnTableEntry;
  LIST_ENTRY         *ListHead;
  LIST_ENTRY         *ListNode;

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index of mCallbackFnTable[].
  //
  ListHead = &mCallbackFnTable[TokenNumber - 1];
  ListNode = GetFirstNode (ListHead);

  while (ListNode != ListHead) {
    FnTableEntry = CR_FNENTRY_FROM_LISTNODE (ListNode, CALLBACK_FN_ENTRY, Node);

    FnTableEntry->CallbackFn (
                    Guid,
                    (Guid == NULL) ? TokenNumber : ExTokenNumber,
                    Data,
                    Size
                    );

    ListNode = GetNextNode (ListHead, ListNode);
  }

  return;
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
  IN UINTN  TokenNumber,
  IN VOID   *Data,
  IN UINTN  Size
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
  IN          UINTN    TokenNumber,
  IN          VOID     *Data,
  IN OUT      UINTN    *Size,
  IN          BOOLEAN  PtrType
  )
{
  BOOLEAN        IsPeiDb;
  UINT32         LocalTokenNumber;
  EFI_GUID       *GuidTable;
  UINT8          *StringTable;
  EFI_GUID       *Guid;
  UINT16         *Name;
  UINTN          VariableOffset;
  UINT32         Attributes;
  VOID           *InternalData;
  VARIABLE_HEAD  *VariableHead;
  UINTN          Offset;
  UINT8          *PcdDb;
  EFI_STATUS     Status;
  UINTN          MaxSize;
  UINTN          TmpTokenNumber;

  //
  // TokenNumber Zero is reserved as PCD_INVALID_TOKEN_NUMBER.
  // We have to decrement TokenNumber by 1 to make it usable
  // as the array index.
  //
  TokenNumber--;

  TmpTokenNumber = TokenNumber;

  //
  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the
  // comparison.
  //
  ASSERT (TokenNumber + 1 < mPcdTotalTokenCount + 1);

  if (PtrType) {
    //
    // Get MaxSize first, then check new size with max buffer size.
    //
    GetPtrTypeSize (TokenNumber, &MaxSize);
    if (*Size > MaxSize) {
      *Size = MaxSize;
      return EFI_INVALID_PARAMETER;
    }
  } else {
    if (*Size != DxePcdGetSize (TokenNumber + 1)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the
  // comparison.
  //
  if ((TokenNumber + 1 < mPeiNexTokenCount + 1) ||
      ((TokenNumber + 1 >= mPeiLocalTokenCount + 1) && (TokenNumber + 1 < (mPeiLocalTokenCount + mDxeNexTokenCount + 1))))
  {
    InvokeCallbackOnSet (0, NULL, TokenNumber + 1, Data, *Size);
  }

  //
  // Aquire lock to prevent reentrance from TPL_CALLBACK level
  //
  EfiAcquireLock (&mPcdDatabaseLock);

  //
  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the
  // comparison.
  //
  IsPeiDb = (BOOLEAN)((TokenNumber + 1 < mPeiLocalTokenCount + 1) ? TRUE : FALSE);

  LocalTokenNumber = GetLocalTokenNumber (IsPeiDb, TokenNumber + 1);

  Offset = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;

  PcdDb = IsPeiDb ? ((UINT8 *)mPcdDatabase.PeiDb) : ((UINT8 *)mPcdDatabase.DxeDb);

  if (IsPeiDb) {
    StringTable = (UINT8 *)((UINT8 *)mPcdDatabase.PeiDb + mPcdDatabase.PeiDb->StringTableOffset);
  } else {
    StringTable = (UINT8 *)((UINT8 *)mPcdDatabase.DxeDb + mPcdDatabase.DxeDb->StringTableOffset);
  }

  InternalData = PcdDb + Offset;

  switch (LocalTokenNumber & PCD_TYPE_ALL_SET) {
    case PCD_TYPE_VPD:
      ASSERT (FALSE);
      Status = EFI_INVALID_PARAMETER;
      break;

    case PCD_TYPE_STRING:
      if (SetPtrTypeSize (TmpTokenNumber, Size)) {
        CopyMem (StringTable + *((STRING_HEAD *)InternalData), Data, *Size);
        Status = EFI_SUCCESS;
      } else {
        Status = EFI_INVALID_PARAMETER;
      }

      break;

    case PCD_TYPE_HII|PCD_TYPE_STRING:
    case PCD_TYPE_HII:
      if (PtrType) {
        if (!SetPtrTypeSize (TmpTokenNumber, Size)) {
          Status = EFI_INVALID_PARAMETER;
          break;
        }
      }

      if (IsPeiDb) {
        GuidTable = (EFI_GUID *)((UINT8 *)mPcdDatabase.PeiDb + mPcdDatabase.PeiDb->GuidTableOffset);
      } else {
        GuidTable = (EFI_GUID *)((UINT8 *)mPcdDatabase.DxeDb + mPcdDatabase.DxeDb->GuidTableOffset);
      }

      VariableHead = (VARIABLE_HEAD *)(PcdDb + Offset);

      Guid           = GuidTable + VariableHead->GuidTableIndex;
      Name           = (UINT16 *)(StringTable + VariableHead->StringIndex);
      VariableOffset = VariableHead->Offset;
      Attributes     = VariableHead->Attributes;
      Status         = SetHiiVariable (Guid, Name, Attributes, Data, *Size, VariableOffset);
      break;

    case PCD_TYPE_DATA:
      if (PtrType) {
        if (SetPtrTypeSize (TmpTokenNumber, Size)) {
          CopyMem (InternalData, Data, *Size);
          Status = EFI_SUCCESS;
        } else {
          Status = EFI_INVALID_PARAMETER;
        }

        break;
      }

      Status = EFI_SUCCESS;
      switch (*Size) {
        case sizeof (UINT8):
          *((UINT8 *)InternalData) = *((UINT8 *)Data);
          break;

        case sizeof (UINT16):
          *((UINT16 *)InternalData) = *((UINT16 *)Data);
          break;

        case sizeof (UINT32):
          *((UINT32 *)InternalData) = *((UINT32 *)Data);
          break;

        case sizeof (UINT64):
          *((UINT64 *)InternalData) = *((UINT64 *)Data);
          break;

        default:
          ASSERT (FALSE);
          Status = EFI_NOT_FOUND;
          break;
      }

      break;

    default:
      ASSERT (FALSE);
      Status = EFI_NOT_FOUND;
      break;
  }

  EfiReleaseLock (&mPcdDatabaseLock);

  return Status;
}

/**
  Wrapper function for get PCD value for dynamic-ex PCD.

  @param Guid            Token space guid for dynamic-ex PCD.
  @param ExTokenNumber   Token number for dynamic-ex PCD.
  @param GetSize         The size of dynamic-ex PCD value.

  @return PCD entry in PCD database.

**/
VOID *
ExGetWorker (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           ExTokenNumber,
  IN UINTN           GetSize
  )
{
  return GetWorker (GetExPcdTokenNumber (Guid, (UINT32)ExTokenNumber), GetSize);
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
  IN          UINTN           ExTokenNumber,
  IN          CONST EFI_GUID  *Guid,
  IN          VOID            *Data,
  IN          UINTN           SetSize
  )
{
  return ExSetWorker (ExTokenNumber, Guid, Data, &SetSize, FALSE);
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
  IN          UINTN           ExTokenNumber,
  IN          CONST EFI_GUID  *Guid,
  IN          VOID            *Data,
  IN OUT      UINTN           *SetSize,
  IN          BOOLEAN         PtrType
  )
{
  UINTN  TokenNumber;

  TokenNumber = GetExPcdTokenNumber (Guid, (UINT32)ExTokenNumber);

  InvokeCallbackOnSet ((UINT32)ExTokenNumber, Guid, TokenNumber, Data, *SetSize);

  return SetWorker (TokenNumber, Data, SetSize, PtrType);
}

/**
  Get variable size and data from HII-type PCDs.

  @param[in]  VariableGuid   Guid of variable which stored value of a HII-type PCD.
  @param[in]  VariableName   Unicode name of variable which stored value of a HII-type PCD.
  @param[out] VariableSize   Pointer to variable size got from HII-type PCDs.
  @param[out] VariableData   Pointer to variable data got from HII-type PCDs.

**/
VOID
GetVariableSizeAndDataFromHiiPcd (
  IN EFI_GUID  *VariableGuid,
  IN UINT16    *VariableName,
  OUT UINTN    *VariableSize,
  OUT VOID     *VariableData OPTIONAL
  )
{
  BOOLEAN            IsPeiDb;
  PCD_DATABASE_INIT  *Database;
  UINTN              TokenNumber;
  UINT32             LocalTokenNumber;
  UINTN              Offset;
  EFI_GUID           *GuidTable;
  UINT8              *StringTable;
  VARIABLE_HEAD      *VariableHead;
  EFI_GUID           *Guid;
  UINT16             *Name;
  UINTN              PcdDataSize;
  UINTN              Size;
  UINT8              *VaraiableDefaultBuffer;
  STRING_HEAD        StringTableIdx;

  *VariableSize = 0;

  //
  // Go through PCD database to find out DynamicHii PCDs.
  //
  for (TokenNumber = 1; TokenNumber <= mPcdTotalTokenCount; TokenNumber++) {
    IsPeiDb          = (BOOLEAN)((TokenNumber + 1 < mPeiLocalTokenCount + 1) ? TRUE : FALSE);
    Database         = IsPeiDb ? mPcdDatabase.PeiDb : mPcdDatabase.DxeDb;
    LocalTokenNumber = GetLocalTokenNumber (IsPeiDb, TokenNumber);
    if ((LocalTokenNumber & PCD_TYPE_HII) != 0) {
      //
      // Get the Variable Guid and Name pointer.
      //
      Offset       = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;
      VariableHead = (VARIABLE_HEAD *)((UINT8 *)Database + Offset);
      StringTable  = (UINT8 *)((UINT8 *)Database + Database->StringTableOffset);
      GuidTable    = (EFI_GUID *)((UINT8 *)Database + Database->GuidTableOffset);
      Guid         = GuidTable + VariableHead->GuidTableIndex;
      Name         = (UINT16 *)(StringTable + VariableHead->StringIndex);
      if (CompareGuid (VariableGuid, Guid) && (StrCmp (VariableName, Name) == 0)) {
        //
        // It is the matched DynamicHii PCD.
        //
        PcdDataSize = DxePcdGetSize (TokenNumber);
        Size        = VariableHead->Offset + PcdDataSize;
        if (Size > *VariableSize) {
          *VariableSize = Size;
        }

        if (VariableData != NULL) {
          if ((LocalTokenNumber & PCD_TYPE_ALL_SET) == (PCD_TYPE_HII|PCD_TYPE_STRING)) {
            //
            // If a HII type PCD's datum type is VOID*, the DefaultValueOffset is the index of
            // string array in string table.
            //
            StringTableIdx         = *(STRING_HEAD *)((UINT8 *)Database + VariableHead->DefaultValueOffset);
            VaraiableDefaultBuffer = (UINT8 *)(StringTable + StringTableIdx);
          } else {
            VaraiableDefaultBuffer = (UINT8 *)Database + VariableHead->DefaultValueOffset;
          }

          CopyMem ((UINT8 *)VariableData + VariableHead->Offset, VaraiableDefaultBuffer, PcdDataSize);
        }
      }
    }
  }
}

/**
  Set value for HII-type PCD.

  A HII-type PCD's value is stored in a variable. Setting/Getting the value of
  HII-type PCD is to visit this variable.

  @param VariableGuid    Guid of variable which stored value of a HII-type PCD.
  @param VariableName    Unicode name of variable which stored value of a HII-type PCD.
  @param SetAttributes   Attributes bitmask to set for the variable.
  @param Data            Value want to be set.
  @param DataSize        Size of value
  @param Offset          Value offset of HII-type PCD in variable.

  @return status of GetVariable()/SetVariable().

**/
EFI_STATUS
SetHiiVariable (
  IN  EFI_GUID    *VariableGuid,
  IN  UINT16      *VariableName,
  IN  UINT32      SetAttributes,
  IN  CONST VOID  *Data,
  IN  UINTN       DataSize,
  IN  UINTN       Offset
  )
{
  UINTN       Size;
  VOID        *Buffer;
  EFI_STATUS  Status;
  UINT32      Attribute;
  UINTN       SetSize;

  Size    = 0;
  SetSize = 0;

  //
  // Try to get original variable size information.
  //
  Status = gRT->GetVariable (
                  (UINT16 *)VariableName,
                  VariableGuid,
                  NULL,
                  &Size,
                  NULL
                  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Patch new PCD's value to offset in given HII variable.
    //
    if (Size >= (DataSize + Offset)) {
      SetSize = Size;
    } else {
      SetSize = DataSize + Offset;
    }

    Buffer = AllocatePool (SetSize);
    ASSERT (Buffer != NULL);

    Status = gRT->GetVariable (
                    VariableName,
                    VariableGuid,
                    &Attribute,
                    &Size,
                    Buffer
                    );

    ASSERT_EFI_ERROR (Status);

    CopyMem ((UINT8 *)Buffer + Offset, Data, DataSize);

    if (SetAttributes == 0) {
      SetAttributes = Attribute;
    }

    Status = gRT->SetVariable (
                    VariableName,
                    VariableGuid,
                    SetAttributes,
                    SetSize,
                    Buffer
                    );

    FreePool (Buffer);
    return Status;
  } else if (Status == EFI_NOT_FOUND) {
    //
    // If variable does not exist, a new variable need to be created.
    //

    //
    // Get size, allocate buffer and get data.
    //
    GetVariableSizeAndDataFromHiiPcd (VariableGuid, VariableName, &Size, NULL);
    Buffer = AllocateZeroPool (Size);
    ASSERT (Buffer != NULL);
    GetVariableSizeAndDataFromHiiPcd (VariableGuid, VariableName, &Size, Buffer);

    //
    // Update buffer.
    //
    CopyMem ((UINT8 *)Buffer + Offset, Data, DataSize);

    if (SetAttributes == 0) {
      SetAttributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE;
    }

    Status = gRT->SetVariable (
                    VariableName,
                    VariableGuid,
                    SetAttributes,
                    Size,
                    Buffer
                    );

    FreePool (Buffer);
    return Status;
  }

  //
  // If we drop to here, the value is failed to be written in to variable area.
  //
  return Status;
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
  IN CONST EFI_GUID  *Guid,
  IN UINT32          ExTokenNumber
  )
{
  UINT32             Index;
  DYNAMICEX_MAPPING  *ExMap;
  EFI_GUID           *GuidTable;
  EFI_GUID           *MatchGuid;
  UINTN              MatchGuidIdx;

  if (!mPeiDatabaseEmpty) {
    ExMap     = (DYNAMICEX_MAPPING *)((UINT8 *)mPcdDatabase.PeiDb + mPcdDatabase.PeiDb->ExMapTableOffset);
    GuidTable = (EFI_GUID *)((UINT8 *)mPcdDatabase.PeiDb + mPcdDatabase.PeiDb->GuidTableOffset);

    MatchGuid = ScanGuid (GuidTable, mPeiGuidTableSize, Guid);

    if (MatchGuid != NULL) {
      MatchGuidIdx = MatchGuid - GuidTable;

      for (Index = 0; Index < mPcdDatabase.PeiDb->ExTokenCount; Index++) {
        if ((ExTokenNumber == ExMap[Index].ExTokenNumber) &&
            (MatchGuidIdx == ExMap[Index].ExGuidIndex))
        {
          return ExMap[Index].TokenNumber;
        }
      }
    }
  }

  ExMap     = (DYNAMICEX_MAPPING *)((UINT8 *)mPcdDatabase.DxeDb + mPcdDatabase.DxeDb->ExMapTableOffset);
  GuidTable = (EFI_GUID *)((UINT8 *)mPcdDatabase.DxeDb + mPcdDatabase.DxeDb->GuidTableOffset);

  MatchGuid = ScanGuid (GuidTable, mDxeGuidTableSize, Guid);
  //
  // We need to ASSERT here. If GUID can't be found in GuidTable, this is a
  // error in the BUILD system.
  //
  ASSERT (MatchGuid != NULL);

  MatchGuidIdx = MatchGuid - GuidTable;

  for (Index = 0; Index < mPcdDatabase.DxeDb->ExTokenCount; Index++) {
    if ((ExTokenNumber == ExMap[Index].ExTokenNumber) &&
        (MatchGuidIdx == ExMap[Index].ExGuidIndex))
    {
      return ExMap[Index].TokenNumber;
    }
  }

  DEBUG ((DEBUG_ERROR, "%a: Failed to find PCD with GUID: %g and token number: %d\n", __func__, Guid, ExTokenNumber));
  ASSERT (FALSE);

  return 0;
}

/**
  Wrapper function of getting index of PCD entry in size table.

  @param LocalTokenNumberTableIdx Index of this PCD in local token number table.
  @param IsPeiDb                  If TRUE, the pcd entry is initialized in PEI phase,
                                  If FALSE, the pcd entry is initialized in DXE phase.

  @return index of PCD entry in size table.
**/
UINTN
GetSizeTableIndex (
  IN    UINTN    LocalTokenNumberTableIdx,
  IN    BOOLEAN  IsPeiDb
  )
{
  UINT32  *LocalTokenNumberTable;
  UINTN   LocalTokenNumber;
  UINTN   Index;
  UINTN   SizeTableIdx;

  if (IsPeiDb) {
    LocalTokenNumberTable = (UINT32 *)((UINT8 *)mPcdDatabase.PeiDb + mPcdDatabase.PeiDb->LocalTokenNumberTableOffset);
  } else {
    LocalTokenNumberTable = (UINT32 *)((UINT8 *)mPcdDatabase.DxeDb + mPcdDatabase.DxeDb->LocalTokenNumberTableOffset);
  }

  SizeTableIdx = 0;

  for (Index = 0; Index < LocalTokenNumberTableIdx; Index++) {
    LocalTokenNumber = LocalTokenNumberTable[Index];

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

/**
  Get size of POINTER type PCD value.

  @param LocalTokenNumberTableIdx Index of local token number in local token number table.
  @param MaxSize                  Maxmium size of POINTER type PCD value.

  @return size of POINTER type PCD value.

**/
UINTN
GetPtrTypeSize (
  IN    UINTN  LocalTokenNumberTableIdx,
  OUT   UINTN  *MaxSize
  )
{
  INTN       SizeTableIdx;
  UINTN      LocalTokenNumber;
  SIZE_INFO  *SizeTable;
  BOOLEAN    IsPeiDb;
  UINT32     *LocalTokenNumberTable;

  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the
  // comparison.
  IsPeiDb = (BOOLEAN)(LocalTokenNumberTableIdx + 1 < mPeiLocalTokenCount + 1);

  if (IsPeiDb) {
    LocalTokenNumberTable = (UINT32 *)((UINT8 *)mPcdDatabase.PeiDb + mPcdDatabase.PeiDb->LocalTokenNumberTableOffset);
    SizeTable             = (SIZE_INFO *)((UINT8 *)mPcdDatabase.PeiDb + mPcdDatabase.PeiDb->SizeTableOffset);
  } else {
    LocalTokenNumberTableIdx -= mPeiLocalTokenCount;
    LocalTokenNumberTable     = (UINT32 *)((UINT8 *)mPcdDatabase.DxeDb + mPcdDatabase.DxeDb->LocalTokenNumberTableOffset);
    SizeTable                 = (SIZE_INFO *)((UINT8 *)mPcdDatabase.DxeDb + mPcdDatabase.DxeDb->SizeTableOffset);
  }

  LocalTokenNumber = LocalTokenNumberTable[LocalTokenNumberTableIdx];

  ASSERT ((LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) == PCD_DATUM_TYPE_POINTER);

  SizeTableIdx = GetSizeTableIndex (LocalTokenNumberTableIdx, IsPeiDb);

  *MaxSize = SizeTable[SizeTableIdx];
  //
  // SizeTable only contain record for PCD_DATUM_TYPE_POINTER type
  // PCD entry.
  //
  if ((LocalTokenNumber & PCD_TYPE_VPD) != 0) {
    //
    // We have only two entry for VPD enabled PCD entry:
    // 1) MAX Size.
    // 2) Current Size
    // We consider current size is equal to MAX size.
    //
    return *MaxSize;
  } else {
    //
    // We have only two entry for Non-Sku enabled PCD entry:
    // 1) MAX SIZE
    // 2) Current Size
    //
    return SizeTable[SizeTableIdx + 1];
  }
}

/**
  Set size of POINTER type PCD value. The size should not exceed the maximum size
  of this PCD value.

  @param LocalTokenNumberTableIdx Index of local token number in local token number table.
  @param CurrentSize              Size of POINTER type PCD value.

  @retval TRUE  Success to set size of PCD value.
  @retval FALSE Fail to set size of PCD value.
**/
BOOLEAN
SetPtrTypeSize (
  IN          UINTN  LocalTokenNumberTableIdx,
  IN    OUT   UINTN  *CurrentSize
  )
{
  INTN       SizeTableIdx;
  UINTN      LocalTokenNumber;
  SIZE_INFO  *SizeTable;
  UINTN      MaxSize;
  BOOLEAN    IsPeiDb;
  UINT32     *LocalTokenNumberTable;

  //
  // EBC compiler is very choosy. It may report warning about comparison
  // between UINTN and 0 . So we add 1 in each size of the
  // comparison.
  //
  IsPeiDb = (BOOLEAN)(LocalTokenNumberTableIdx + 1 < mPeiLocalTokenCount + 1);

  if (IsPeiDb) {
    LocalTokenNumberTable = (UINT32 *)((UINT8 *)mPcdDatabase.PeiDb + mPcdDatabase.PeiDb->LocalTokenNumberTableOffset);
    SizeTable             = (SIZE_INFO *)((UINT8 *)mPcdDatabase.PeiDb + mPcdDatabase.PeiDb->SizeTableOffset);
  } else {
    LocalTokenNumberTableIdx -= mPeiLocalTokenCount;
    LocalTokenNumberTable     = (UINT32 *)((UINT8 *)mPcdDatabase.DxeDb + mPcdDatabase.DxeDb->LocalTokenNumberTableOffset);
    SizeTable                 = (SIZE_INFO *)((UINT8 *)mPcdDatabase.DxeDb + mPcdDatabase.DxeDb->SizeTableOffset);
  }

  LocalTokenNumber = LocalTokenNumberTable[LocalTokenNumberTableIdx];

  ASSERT ((LocalTokenNumber & PCD_DATUM_TYPE_ALL_SET) == PCD_DATUM_TYPE_POINTER);

  SizeTableIdx = GetSizeTableIndex (LocalTokenNumberTableIdx, IsPeiDb);

  MaxSize = SizeTable[SizeTableIdx];
  //
  // SizeTable only contain record for PCD_DATUM_TYPE_POINTER type
  // PCD entry.
  //
  if ((LocalTokenNumber & PCD_TYPE_VPD) != 0) {
    //
    // We shouldn't come here as we don't support SET for VPD
    //
    ASSERT (FALSE);
    return FALSE;
  } else {
    if ((*CurrentSize > MaxSize) ||
        (*CurrentSize == MAX_ADDRESS))
    {
      *CurrentSize = MaxSize;
      return FALSE;
    }

    //
    // We have only two entry for Non-Sku enabled PCD entry:
    // 1) MAX SIZE
    // 2) Current Size
    //
    SizeTable[SizeTableIdx + 1] = (SIZE_INFO)*CurrentSize;
    return TRUE;
  }
}

/**
  VariableLock DynamicHiiPcd.

  @param[in] IsPeiDb        If TRUE, the pcd entry is initialized in PEI phase,
                            If FALSE, the pcd entry is initialized in DXE phase.
  @param[in] VariableLock   Pointer to VariablePolicyProtocol.

**/
VOID
VariableLockDynamicHiiPcd (
  IN BOOLEAN                         IsPeiDb,
  IN EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy
  )
{
  EFI_STATUS         Status;
  PCD_DATABASE_INIT  *Database;
  UINT32             LocalTokenCount;
  UINTN              TokenNumber;
  UINT32             LocalTokenNumber;
  UINTN              Offset;
  EFI_GUID           *GuidTable;
  UINT8              *StringTable;
  VARIABLE_HEAD      *VariableHead;
  EFI_GUID           *Guid;
  UINT16             *Name;

  Database        = IsPeiDb ? mPcdDatabase.PeiDb : mPcdDatabase.DxeDb;
  LocalTokenCount = IsPeiDb ? mPeiLocalTokenCount : mDxeLocalTokenCount;

  //
  // Go through PCD database to find out DynamicHii PCDs.
  //
  for (TokenNumber = 1; TokenNumber <= LocalTokenCount; TokenNumber++) {
    if (IsPeiDb) {
      LocalTokenNumber = GetLocalTokenNumber (TRUE, TokenNumber);
    } else {
      LocalTokenNumber = GetLocalTokenNumber (FALSE, TokenNumber + mPeiLocalTokenCount);
    }

    if ((LocalTokenNumber & PCD_TYPE_HII) != 0) {
      Offset       = LocalTokenNumber & PCD_DATABASE_OFFSET_MASK;
      VariableHead = (VARIABLE_HEAD *)((UINT8 *)Database + Offset);
      //
      // Why not to set property by VarCheckProtocol with Attributes and Property directly here?
      // It is because that set property by VarCheckProtocol will indicate the variable to
      // be a system variable, but the unknown max size of the variable is dangerous to
      // the system variable region.
      //
      if ((VariableHead->Property & VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY) != 0) {
        //
        // DynamicHii PCD with RO property set in *.dsc.
        //
        StringTable = (UINT8 *)((UINT8 *)Database + Database->StringTableOffset);
        GuidTable   = (EFI_GUID *)((UINT8 *)Database + Database->GuidTableOffset);
        Guid        = GuidTable + VariableHead->GuidTableIndex;
        Name        = (UINT16 *)(StringTable + VariableHead->StringIndex);
        Status      = RegisterBasicVariablePolicy (
                        VariablePolicy,
                        Guid,
                        Name,
                        VARIABLE_POLICY_NO_MIN_SIZE,
                        VARIABLE_POLICY_NO_MAX_SIZE,
                        VARIABLE_POLICY_NO_MUST_ATTR,
                        VARIABLE_POLICY_NO_CANT_ATTR,
                        VARIABLE_POLICY_TYPE_LOCK_NOW
                        );

        ASSERT_EFI_ERROR (Status);
      }
    }
  }
}

/**
  VariablePolicyProtocol callback
  to lock the variables referenced by DynamicHii PCDs with RO property set in *.dsc.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    Pointer to the notification function's context.

**/
VOID
EFIAPI
VariableLockCallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                      Status;
  EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy;

  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
  if (!EFI_ERROR (Status)) {
    VariableLockDynamicHiiPcd (TRUE, VariablePolicy);
    VariableLockDynamicHiiPcd (FALSE, VariablePolicy);
  }
}
