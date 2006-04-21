/** @file
Common functions used by PCD PEIM and PCD DXE.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name: PcdCommon.c

**/
#include "PcdCommon.h"



/**
  The function retrieves the PCD data value according to
  the PCD_DATA_TYPE specified.

  @param[in] Type The PCD_DATA_TYPE used to interpret the data.
  @param[in] InData The input data.
  @param[in] OutData The output data.
  @param[in] Len The length of the data; it is mainly used for PcdPointer type.

  @retval VOID
--*/
VOID
GetDataBasedOnType (
  IN PCD_DATA_TYPE Type,
  IN VOID          *InData,
  OUT VOID         *OutData,
  IN UINTN         Len
  )
{
  if (Type == PcdPointer) {
    //
    // When the Type is PcdPointer, we are returning 
    // the address of the internal buffer kpet by
    // PCD database. Therefore, we treat OutData as
    // a pointer to a "VOID *". Thus, the ugly type cast
    // (VOID **) is used.
    //

    *((VOID **) OutData) = InData;
  } else {
    CopyMem (OutData, InData, Len);
  }
  
  return;
  
}

UINTN
GetExtendedDataOffset (
  IN CONST PCD_INDEX           *PcdIndex,
  IN UINTN                     SkuIdx,
  IN CONST PCD_DATABASE_HEADER *Info
  )
{
  UINT8 *OffsetAddress;
  UINTN Offset;

  OffsetAddress = GetAbsoluteAddress (PcdIndex->ExtendedDataOffset, 
                                      Info->ImageIndexOffset, 
                                      Info
                                      );

  OffsetAddress += (SkuIdx * Info->ExtendedOffsetLength);


  CopyMem (&Offset, OffsetAddress, Info->ExtendedOffsetLength);

  return Offset;
}



VOID
GetHiiDataProperty (
  IN CONST PCD_INDEX           *PcdIndex,
  IN UINTN                     SkuIdx,
  IN CONST PCD_DATABASE_HEADER *Info,
  OUT EFI_GUID                 **VariableGuid,
  OUT UINT16                   **VariableName
  )
{
  UINT16 NameOffset;
  UINT16 GuidOffset;
  UINT8  *HiiDataOffset;

  HiiDataOffset = GetAbsoluteAddress (PcdIndex->HiiData, Info->ImageIndexOffset, Info);
  HiiDataOffset += (SkuIdx * (Info->HiiGuidOffsetLength + Info->HiiVariableOffsetLength));

  CopyMem (&GuidOffset, HiiDataOffset, Info->HiiGuidOffsetLength);
  CopyMem (&NameOffset, HiiDataOffset + Info->HiiGuidOffsetLength, Info->HiiVariableOffsetLength);
  
  *VariableGuid  = (EFI_GUID *) GetAbsoluteAddress (GuidOffset * sizeof (EFI_GUID), Info->GuidTableOffset, Info);
  *VariableName  = (UINT16 *)   GetAbsoluteAddress (NameOffset * sizeof (UINT16)  , Info->StringTableOffset, Info);
  
  return;
}



UINTN
GetSkuIdIdx (
  IN CONST PCD_INDEX           *PcdIndex,
  IN CONST PCD_DATABASE_HEADER *Info
  )
{
  UINT8      *SkuIdArray;
  UINTN      SkuIdx;

  SkuIdArray = GetAbsoluteAddress (PcdIndex->SkuIdArray, Info->ImageIndexOffset, Info);
  
  SkuIdx = 0;

  if (PcdIndex->StateByte.SkuEnable) {

     for (; SkuIdx < PcdIndex->SkuCount; SkuIdx++) {
       if (SkuIdArray[SkuIdx] == Info->SkuId) {
        break;
       }
     }

     if (SkuIdx > PcdIndex->SkuCount) {
       if (Info->SkuId == 0) {
         //
         // If no SKU_ID is set previously
         // Just retrieve the first value
         //
         SkuIdx = 0;
       } else {
         //
         // Just can't find the SKU_ID, ASSERT according to Spec.
         //
         ASSERT (FALSE);
       }
    }
      
  }

  return SkuIdx;

}



/**
  The function is the worker function to get the data of a PCD entry.

  @param[in] PcdIndex The PCD Index.
  @param[in] Info     The attributes of the PCD database.
  @param[out] Data    The output data.

  @retval VOID
--*/
UINT8*
GetPcdDataPtr (
  IN CONST PCD_INDEX           *PcdIndex,
  IN CONST PCD_DATABASE_HEADER *Info
  )
{
  UINTN      VariableDataSize;
  VOID       *VariableData;
  UINT16     *VariableName;
  UINT8      *PcdData;
  EFI_GUID   *VariableGuid;
  EFI_STATUS Status;
  UINTN      SkuIdx;
  UINTN      ExtendedOffset;

  //
  // If Sku is not enalbed for this PCD Entry.
  // SkuIdx 0 will be used to compute PcdData
  //
  SkuIdx = GetSkuIdIdx (PcdIndex, Info);

  if (PcdIndex->StateByte.HiiEnable) {

    GetHiiDataProperty (PcdIndex, SkuIdx, Info, &VariableGuid, &VariableName);

    Status = GetHiiVariable (VariableGuid, VariableName, &VariableData, &VariableDataSize);
    ASSERT_EFI_ERROR (Status);
    ASSERT (VariableDataSize >= (PcdIndex->DatumSize + PcdIndex->ExtendedDataOffset));

    PcdData = (UINT8 *) VariableData + PcdIndex->ExtendedDataOffset;

    return PcdData;
  }

  //
  // For VPD and Data type, we need the ExtendedOffset.
  // So get it here.
  //
  ExtendedOffset = GetExtendedDataOffset (PcdIndex, SkuIdx, Info);

  if (PcdIndex->StateByte.VpdEnable) {

    PcdData = (VOID *) (Info->VpdStart + ExtendedOffset);
    
    return PcdData;
  }

  //
  // For data type, we just need the pointer
  //
  PcdData = GetAbsoluteAddress (
              ExtendedOffset,
              Info->DataBufferOffset,
              Info
              );

  return PcdData;

}



/**
  The function locates the PCD_INDEX according to TokeNumber and GUID space given.

  @param[in] TokenNumber The token number.
  @param[in] Guid        The GUID token space.
  @param[out] Info       The attributes of the PCD database.

  @retval PCD_INDEX*     The PCD_INDEX found.
--*/
PCD_INDEX *
FindPcdIndex (
  IN   UINTN                     TokenNumber,
  IN   CONST EFI_GUID            *Guid,
  IN   CONST PCD_DATABASE_HEADER *Info,
  OUT  UINTN                     *Index
  )
{
  PCD_INDEX *PcdIndex;
  UINTN     Idx;
  EFI_GUID      *GuidSpace;

  PcdIndex = (PCD_INDEX *) GetAbsoluteAddress (0, Info->PcdIndexOffset, Info);

  for (Idx = 0; Idx < Info->EntryCount; Idx++, PcdIndex++) {
    if (Index != NULL) {
      *Index = Idx;
    }
    
    if (PcdIndex->TokenNumber == TokenNumber) {
      if (Guid == NULL) {
        if (!PcdIndex->StateByte.ExtendedGuidPresent) {
          return PcdIndex;
        }
      } else {
        if (PcdIndex->StateByte.ExtendedGuidPresent) {
          GuidSpace = (EFI_GUID *) GetAbsoluteAddress (PcdIndex->DynamicExGuid, Info->GuidTableOffset, Info);
          if (CompareGuid (GuidSpace, Guid)) {
            return PcdIndex;
          }
        }
      }
    }

  }

  if (Index != NULL) {
    *Index = 0;
  }
  
  return NULL;

}



/**
  The function set the PCD Entry data value according to the
  PCD_DATA_TYPE given.

  @param[out] OutData The output data.
  @param[in]  InData  The input data.
  @param[in]  Len     The length of the data.


  @retval EFI_SUCESS If data value is found according to SKU_ID.
  @retval EFI_NOT_FOUND If not such a value is found.

--*/
VOID
SetDataBasedOnType (
  OUT VOID *       OutData,
  IN CONST VOID *  InData,
  IN UINTN         Len
)
{
  CopyMem (OutData, InData, Len);

  return;
}



/**
  The function returns the actual address of item in the PCD
  database according to its Segment and Offset.

  @param[out] Offset        The offset within the segment.
  @param[in]  SegmentStart  The starting address of the segment.
  @param[in]  DatabaseStart The base address of the PCD DataBase.


  @retval UINT8* The absolute address.

--*/
UINT8 *
GetAbsoluteAddress (
  IN UINTN                        Offset,
  IN UINTN                        SegmentStart,
  IN CONST PCD_DATABASE_HEADER    *DatabaseStart
  )
{
  UINT8 *Address;

  Address = (UINT8 *) DatabaseStart + SegmentStart + Offset;

  return Address;
}



/**
  The function retrieves the PCD data value according to
  TokenNumber and Guid space given.

  @param[in]  Database    The PCD Database Instance.
  @param[in]  TokenNumber The token number.
  @param[in]  Guid        The Guid space.
  @param[in]  Type        The storage type.
  @param[out] Data        The output data.

  @retval VOID

--*/
VOID
GetPcdEntryWorker (
  IN CONST PCD_DATABASE_HEADER  *Info,
  IN UINTN                      TokenNumber,
  IN CONST EFI_GUID             *Guid,  OPTIONAL
  IN PCD_DATA_TYPE              Type,
  OUT VOID                      *Data
  )
{
  PCD_INDEX         *PcdIndex;
  UINT8             *PcdData;

  ASSERT (Data != NULL);

  //
  // Find the PCD entry in list in memory first
  //
  PcdIndex = FindPcdIndex (TokenNumber, Guid, Info, NULL);

  ASSERT (PcdIndex != NULL);

  ASSERT (PcdIndex->StateByte.DataType == Type);

  PcdData = GetPcdDataPtr (PcdIndex, Info);

  GetDataBasedOnType (PcdIndex->StateByte.DataType, PcdData, Data, PcdIndex->DatumSize);

  return;
}



/**
  The function retrieves the PCD data value according to
  TokenNumber and Guid space given.

  @param[in]  Database    The PCD Database Instance.
  @param[in]  TokenNumber The token number.
  @param[in]  Guid        The Guid space.

  @retval     UINTN       The size of the PCD Entry.

--*/
UINTN
GetPcdEntrySizeWorker (
  IN CONST PCD_DATABASE_HEADER  *Info,
  IN UINTN                      TokenNumber,
  IN CONST EFI_GUID             *Guid  OPTIONAL
  )
{
  PCD_INDEX         *PcdIndex;

  //
  // Find the PCD entry in list in memory first
  //
  PcdIndex = FindPcdIndex (TokenNumber, Guid, Info, NULL);

  ASSERT (PcdIndex != NULL);

  return PcdIndex->DatumSize;

}



/**
  The function checks if given GUID space match the record
  in the PCD_INDEX.

  @param[in]  Guid              The GUID space.
  @param[in]  PcdIndex          The PCD_INDEX.
  @param[in]  Info              The attribute of the PCD DATABASE.

  @retval     TRUE              The GUID space match the record.
  @retval     FALSE             Othewise.

--*/
BOOLEAN
PeiImageIndexMatchGuidSpace (
  IN  CONST EFI_GUID            *Guid,
  IN  CONST PCD_INDEX           *PcdIndex,
  IN  CONST PCD_DATABASE_HEADER *Info
)
{
  EFI_GUID *GuidSpace;

  if (PcdIndex->StateByte.ExtendedGuidPresent) {
    GuidSpace = (EFI_GUID *) GetAbsoluteAddress (PcdIndex->DynamicExGuid, Info->GuidTableOffset, Info);
    return CompareGuid (GuidSpace, Guid);
  }

  return FALSE;
}


/**
  The function looks for the next PCD ENTRY.
  If *TokenNumber is 0, the first TokenNumber in
  the GUID token space is return.
  If there is no next TokenNumber found,
  *TokenNumber will be 0.

  @param[in]      Database          The PCD Database Instance.
  @param[in,out]  TokenNumber       The token number.
  @param[in]      Guid              The Guid space.

  @retval     EFI_NOT_FOUND     Can't find the PCD_ENTRY.
  @retval     EFI_SUCCESS       Operation succesful.

--*/
EFI_STATUS
GetNextTokenWorker (
  IN CONST PCD_DATABASE_HEADER  *Info,
  IN OUT UINTN                  *TokenNumber,
  IN CONST EFI_GUID             *Guid     OPTIONAL
  )
{
  PCD_INDEX         *PcdIndex;
  UINTN             Idx;
  BOOLEAN           Found;

  Idx = 0;
  Found = FALSE;
  PcdIndex = (PCD_INDEX *) GetAbsoluteAddress (0, Info->PcdIndexOffset, Info);

  while ((Idx < Info->EntryCount) && !Found) {
    if (*TokenNumber == 0) {
      if (Guid == NULL || PeiImageIndexMatchGuidSpace (Guid, PcdIndex, Info)) {
        *TokenNumber = PcdIndex->TokenNumber;
        return EFI_SUCCESS;
      }
    } else {
      if (PcdIndex->TokenNumber == *TokenNumber) {
        if (Guid == NULL || PeiImageIndexMatchGuidSpace (Guid, PcdIndex, Info)) {
          Found = TRUE;
        }
      }
    }

    PcdIndex++;
    Idx++;
  }

  //
  // No PCD Entry in the database match the GUID space given.
  //
  if (*TokenNumber == 0) {
    return EFI_SUCCESS;
  }

  //
  // Can't find the PCD Entry
  //
  if (!Found) {
    return EFI_NOT_FOUND;
  }

  //
  // Move to the Next Entry
  //
  Idx++;
  PcdIndex++;

  //
  // Now look for the Next TokenNumber
  //
  while (Idx < Info->EntryCount) {
    if (Guid == NULL || PeiImageIndexMatchGuidSpace (Guid, PcdIndex, Info)) {
      *TokenNumber = PcdIndex->TokenNumber;
      return EFI_SUCCESS;
    }
    PcdIndex++;
    Idx++;
  }

  //
  // Reache the last TokeNumber.
  //
  *TokenNumber = 0;
  return EFI_SUCCESS;
}


/**
  The function is the worker function to set the data of a PCD entry.

  @param[in] PcdIndex The PCD Index.
  @param[in] Info     The attributes of the PCD database.
  @param[in] Data     The input data.

  @retval VOID
--*/
EFI_STATUS
SetPcdData (
  IN CONST PCD_INDEX           *PcdIndex,
  IN CONST PCD_DATABASE_HEADER *Info,
  IN CONST VOID                *Data
  )
{
  UINT16     *VariableName;
  UINT8      *PcdData;
  EFI_GUID   *VariableGuid;
  EFI_STATUS Status;
  UINTN      SkuIdx;
  UINTN      ExtendedOffset;

  if (PcdIndex->StateByte.VpdEnable) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  SkuIdx = GetSkuIdIdx (PcdIndex, Info);

  //
  // For Hii and Data type, we need the ExtendedOffset.
  // So get it here.
  //
  ExtendedOffset = GetExtendedDataOffset (PcdIndex, SkuIdx, Info);

  if (PcdIndex->StateByte.HiiEnable) {
  	GetHiiDataProperty (PcdIndex, SkuIdx, Info, &VariableGuid, &VariableName);

    Status = SetHiiVariable (VariableGuid,
                            VariableName,
                            Data,
                            PcdIndex->DatumSize,
                            ExtendedOffset
                            );

    return Status;
  }


  PcdData = GetAbsoluteAddress (
              ExtendedOffset,
              Info->DataBufferOffset,
              Info
              );

  CopyMem (PcdData, Data, PcdIndex->DatumSize);

  return EFI_SUCCESS;

}

