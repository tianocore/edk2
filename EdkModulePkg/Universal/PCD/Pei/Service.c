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
#include "../Common/PcdCommon.h"
#include "Service.h"




/**
  This function expand the StateByte

  @param[out]   StateByte The output StateByte information.
  @param[in]    Byte The StateByte.

  @retval       VOID
--*/
VOID
PcdImageExpandStateByte (
  OUT PCD_STATEBYTE *StateByte,
  IN  UINT8          Byte
)
{
  switch (Byte & PCD_STATEBYTE_DATUMTYPE) {
    case PCD_BYTE8:
      StateByte->DataType = PcdByte8;
      break;
    case PCD_BYTE16:
      StateByte->DataType = PcdByte16;
      break;
    case PCD_BYTE32:
      StateByte->DataType = PcdByte32;
      break;
    case PCD_BYTE64:
      StateByte->DataType = PcdByte64;
      break;
    case PCD_POINTER:
      StateByte->DataType = PcdPointer;
      break;
    case PCD_BOOLEAN:
      StateByte->DataType = PcdBoolean;
      break;
    default:
      ASSERT (FALSE);
  }

  StateByte->ExtendedGuidPresent = (BOOLEAN) ((Byte & PCD_STATEBYTE_EXTENDEDGUIDPRESENT) != 0);
  StateByte->HiiEnable           = (BOOLEAN) ((Byte & PCD_STATEBYTE_HIIENABLE)           != 0);
  StateByte->SkuDataArrayEnable  = (BOOLEAN) ((Byte & PCD_STATEBYTE_SKUDATAARRAYENABLE)  != 0);
  StateByte->SkuEnable           = (BOOLEAN) ((Byte & PCD_STATEBYTE_SKUENABLE)           != 0);
  StateByte->VpdEnable           = (BOOLEAN) ((Byte & PCD_STATEBYTE_VPDENABLE)           != 0);

}



/**
  This function locates the <PCD_IMAGE> on the flash and 
  return a pointer to the Section Data on flash.

  @param[in]  VOID

  @retval     VOID
--*/
UINT8 *
LocatePcdImage (
  VOID
)
{
  EFI_STATUS                 Status;
  EFI_FIRMWARE_VOLUME_HEADER *FvHdr;
  EFI_FFS_FILE_HEADER        *FfsHdr;
  VOID                       *SectionData;

  Status = PeiCoreFfsFindNextVolume (0, &FvHdr);
  ASSERT_EFI_ERROR (Status);

  do {
    FfsHdr = NULL;
    Status = PeiCoreFfsFindNextFile (EFI_FV_FILETYPE_FREEFORM, FvHdr, &FfsHdr);
    if (Status == EFI_SUCCESS) {
      if (CompareGuid (&gPcdImageFileGuid, &FfsHdr->Name)) {

        Status = PeiCoreFfsFindSectionData (EFI_SECTION_RAW, FfsHdr, &SectionData);
        ASSERT_EFI_ERROR (Status);

        return (UINT8 *)SectionData;
      }
    }
  } while (Status == EFI_SUCCESS);

  ASSERT (FALSE);

  return NULL;
}

/**
  The function retrieves the PCD data value according to
  TokenNumber and Guid space given.

  @param[in]  TokenNumber The token number.
  @param[in]  Guid The Guid space.
  @param[in]  Type The storage type.
  @param[out] Data The output data.


  @retval EFI_SUCESS    If data value is found according to SKU_ID.
  @retval EFI_NOT_FOUND If not such a value is found.

--*/
VOID
PeiGetPcdEntryWorker (
  IN UINTN TokenNumber,
  IN CONST EFI_GUID       *Guid,  OPTIONAL
  IN PCD_DATA_TYPE        Type,
  OUT VOID                *Data
  )
{
  PCD_DATABASE *Database;
  EFI_HOB_GUID_TYPE *GuidHob;

  ASSERT (Data != NULL);

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  ASSERT (GuidHob != NULL);

  Database = GET_GUID_HOB_DATA (GuidHob);

  GetPcdEntryWorker ( &Database->Info,
                      TokenNumber,
                      Guid,
                      Type,
                      Data
                      );


  return;
}


/**
  The function set the PCD data value according to
  TokenNumber and Guid space given.

  @param[in] Database     The PCD Database Instance.
  @param[in] TokenNumber  The token number.
  @param[in] Guid         The Guid space.
  @param[in] Type         The storage type.
  @param[in] Data         The output data.


  @retval EFI_SUCESS      If data value is found according to SKU_ID.
  @retval EFI_NOT_FOUND   If not such a value is found.

--*/
EFI_STATUS
SetPcdEntryWorker (
  IN CONST PCD_DATABASE     *Database,
  IN UINTN                  TokenNumber,
  IN CONST EFI_GUID         *Guid,  OPTIONAL
  IN PCD_DATA_TYPE          Type,
  IN VOID                   *Data
  )
{
  PCD_INDEX               *PcdIndex;
  EFI_STATUS              Status;
  PCD_PPI_CALLBACK        *CallbackTable;
  UINTN                   Idx;

  ASSERT (Data != NULL);

  //
  // Find the PCD entry in list in memory first
  //
  PcdIndex = FindPcdIndex (TokenNumber, Guid, &Database->Info, &Idx);

  ASSERT (PcdIndex != NULL);

  ASSERT (PcdIndex->StateByte.DataType == Type);

  //
  // Invoke the callback function.
  //
  CallbackTable = (PCD_PPI_CALLBACK *)
                              GetAbsoluteAddress (Idx * Database->Info.MaxCallbackNum * sizeof(PCD_PPI_CALLBACK),
                              Database->Info.CallbackTableOffset,
                              &Database->Info
                              );
  
  for (Idx = 0; Idx < Database->Info.MaxCallbackNum; Idx++) {
    if (CallbackTable[Idx] != NULL) {
      CallbackTable[Idx] (Guid,
                          PcdIndex->TokenNumber,
                          Data,
                          PcdIndex->DatumSize
                          );
    }
  }

  Status = SetPcdData (PcdIndex, &Database->Info, Data);

  return Status;
}



/**
  (reviewed) The function set the PCD data value according to
  TokenNumber and Guid space given.

  @param[in] TokenNumber  The token number.
  @param[in] Guid         The Guid space.
  @param[in] Type         The storage type.
  @param[in] Data         The output data.


  @retval EFI_SUCESS    If data value is found according to SKU_ID.
  @retval EFI_NOT_FOUND If not such a value is found.

--*/
EFI_STATUS
PeiSetPcdEntryWorker (
  IN UINTN TokenNumber,
  IN CONST EFI_GUID       *Guid,  OPTIONAL
  IN PCD_DATA_TYPE        Type,
  IN VOID                 *Data
  )
{
  PCD_DATABASE *Database;
  EFI_HOB_GUID_TYPE *GuidHob;

  ASSERT (Data != NULL);

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  ASSERT (GuidHob != NULL);
  
  Database = GET_GUID_HOB_DATA (GuidHob);

  SetPcdEntryWorker (Database,
                     TokenNumber,
                     Guid,
                     Type,
                     Data
                     );

  return EFI_SUCCESS;
}



UINTN
PeiGetPcdEntrySizeWorker (
  IN UINTN                TokenNumber,
  IN CONST EFI_GUID       *Guid  OPTIONAL
  )
{
  PCD_DATABASE      *Database;
  EFI_HOB_GUID_TYPE *GuidHob;

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  ASSERT (GuidHob != NULL);

  Database = GET_GUID_HOB_DATA (GuidHob);

  return GetPcdEntrySizeWorker (&Database->Info,
                                TokenNumber,
                                Guid
                                );

}



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
  IN  UINTN                       TokenNumber,
  IN  CONST EFI_GUID              *Guid, OPTIONAL
  IN  PCD_PPI_CALLBACK            CallBackFunction,
  IN  BOOLEAN                     Register
)
{
  PCD_DATABASE        *Database;
  EFI_HOB_GUID_TYPE   *GuidHob;
  PCD_INDEX           *PcdIndex;
  UINTN               Idx;
  PCD_PPI_CALLBACK    *CallbackTable;
  PCD_PPI_CALLBACK    Compare;
  PCD_PPI_CALLBACK    Assign;

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  ASSERT (GuidHob != NULL);
  
  Database = GET_GUID_HOB_DATA (GuidHob);

  PcdIndex = FindPcdIndex (TokenNumber, Guid, &Database->Info, NULL);

  ASSERT (PcdIndex != NULL);

  if (PcdIndex->StateByte.VpdEnable) {
    return EFI_INVALID_PARAMETER;
  }

  Idx = ((UINTN) PcdIndex - Database->Info.CallbackTableOffset) / sizeof(PCD_INDEX);

  CallbackTable = (PCD_PPI_CALLBACK *) GetAbsoluteAddress (
                                              sizeof (PCD_PPI_CALLBACK) * Idx * Database->Info.MaxCallbackNum,
                                              Database->Info.CallbackTableOffset,
                                              &Database->Info
                                              );

  Compare = Register? NULL: CallBackFunction;
  Assign  = Register? CallBackFunction: NULL;

  for (Idx = 0; Idx < Database->Info.MaxCallbackNum; Idx++) {
    if (CallbackTable[Idx] == Compare) {
      CallbackTable[Idx] = Assign;
      return EFI_SUCCESS;
    }
  }

  return Register? EFI_OUT_OF_RESOURCES : EFI_NOT_FOUND;

}



EFI_STATUS
PeiGetNextTokenWorker (
  IN OUT UINTN                *TokenNumber,
  IN CONST EFI_GUID           *Guid     OPTIONAL
  )
{
  PCD_DATABASE        *Database;
  EFI_HOB_GUID_TYPE   *GuidHob;

  GuidHob = GetFirstGuidHob (&gPcdDataBaseHobGuid);
  ASSERT (GuidHob != NULL);
  
  Database = GET_GUID_HOB_DATA (GuidHob);

  return GetNextTokenWorker (&Database->Info,
                             TokenNumber,
                             Guid
                             );

}



VOID
GetPcdImageInfo (
  IN CONST UINT8          *PcdImageOnFlash,
  OUT PCD_IMAGE_RECORD    *ImageInfo
)
{
  PCD_FFS_ENCODING *PcdFfsHdr;

  PcdFfsHdr = (PCD_FFS_ENCODING *) PcdImageOnFlash;

  ZeroMem (ImageInfo, sizeof (*ImageInfo));

  ImageInfo->ImageStart = PcdImageOnFlash;

  CopyMem (&ImageInfo->EntryCount,              PcdFfsHdr->EntryCount,         3);
  
  CopyMem (&ImageInfo->GlobalDatumLength,       PcdFfsHdr->GlobalDatumLength,  1);
  ASSERT  (ImageInfo->GlobalDatumLength <= 3);
  
  CopyMem (&ImageInfo->GlobalOffsetLength,      PcdFfsHdr->GlobalOffsetLength, 1);
  ASSERT  (ImageInfo->GlobalOffsetLength <= 3);
  
  CopyMem (&ImageInfo->GlobalTokenLength,       PcdFfsHdr->GlobalTokenLength,  1);
  ASSERT  (ImageInfo->GlobalTokenLength <= 4);

  CopyMem (&ImageInfo->GlobalGuidTabIdxLength,  PcdFfsHdr->GuidLength,         1);
  ASSERT  (ImageInfo->GlobalGuidTabIdxLength <= 2);

  CopyMem (&ImageInfo->GlobalStrTabIdxLength,   PcdFfsHdr->GlobalStrTabIdxLength, 1);
  ASSERT  (ImageInfo->GlobalStrTabIdxLength <= 2);

  CopyMem (&ImageInfo->ImageLength,             PcdFfsHdr->ImageLength,        3);
  CopyMem (&ImageInfo->IndexLength,             PcdFfsHdr->PcdIndexLength,     3);
  CopyMem (&ImageInfo->WholeDataDefaultLength,  PcdFfsHdr->WholeDataBufferLength, 3);
  CopyMem (&ImageInfo->DataDefaultLength,       PcdFfsHdr->DataBufferLength, 3);
  CopyMem (&ImageInfo->GuidTableLength,         PcdFfsHdr->GuidTableLength,  3);

  ImageInfo->StringTableLength = ImageInfo->ImageLength
                                  - sizeof (PCD_FFS_ENCODING)
                                  - ImageInfo->DataDefaultLength
                                  - ImageInfo->IndexLength
                                  - ImageInfo->GuidTableLength;

  ImageInfo->DataDefaultStart = PcdImageOnFlash + sizeof (PCD_FFS_ENCODING);
  ImageInfo->IndexStart       = ImageInfo->DataDefaultStart + ImageInfo->DataDefaultLength;
  ImageInfo->GuidTableStart   = (CONST EFI_GUID *)(ImageInfo->IndexStart + ImageInfo->IndexLength);
  ImageInfo->StringTableStart = (CONST UINT16 *) ((UINT8 *) ImageInfo->GuidTableStart + ImageInfo->GuidTableLength);
  
  return;
}



/**
  The function builds the PCD database based on the
  PCD_IMAGE on the flash.

  The layout of the PCD_DATABASE is as follows:

  ---------------------------
  |  PCD_DATABASE_HEADER    |
  ---------------------------
  |  GUID_TABLE             |  Aligned on GUID    (128 bits)
  ---------------------------
  |  PCD_CALL_BACK_TABLE    |  Aligned on Pointer (32 bits or 64 bits)
  ---------------------------
  |  PCD_INDEX_TABLE        |  Aligned on PCD_INDEX (see PCD_INDEX's declaration)
  ---------------------------
  |  IMAGE_STRING_TABLE     |  Aligned on 16 Bits
  ---------------------------
  |  IMAGE_PCD_INDEX        |  Unaligned
  ---------------------------
  |  Data Defaults          |  Unaligned
  ---------------------------
  |  Data Buffer            |
  |  for entries without    |
  |  defaults               |
  ---------------------------

  @param[in] PcdImageOnFlash  The PCD image on flash.

  @retval VOID
--*/
UINTN
GetPcdDatabaseLen (
  IN CONST UINT8          *PcdImageOnFlash,
  OUT PCD_DATABASE_HEADER *Info,
  OUT PCD_IMAGE_RECORD    *ImageInfo
  )
{
  UINTN DatabaseLen;
  UINTN DatabaseHeaderLength;
  UINTN PcdIndexLength;
  UINTN CallbackBufferLength;


  GetPcdImageInfo (PcdImageOnFlash, ImageInfo);

  Info->MaxCallbackNum = FixedPcdGet32(PcdMaxPcdCallBackNumber) ;

  DatabaseHeaderLength = sizeof (PCD_DATABASE) - sizeof(UINT8);

  PcdIndexLength       = sizeof (PCD_INDEX) * ImageInfo->EntryCount;
  CallbackBufferLength = sizeof (PCD_PPI_CALLBACK) * Info->MaxCallbackNum * ImageInfo->EntryCount;

  Info->EntryCount          = ImageInfo->EntryCount;
  Info->GuidTableOffset     = DatabaseHeaderLength;
  Info->CallbackTableOffset = Info->GuidTableOffset + ImageInfo->GuidTableLength;
  Info->PcdIndexOffset      = Info->PcdIndexOffset + PcdIndexLength;
  Info->ImageIndexOffset    = Info->CallbackTableOffset + CallbackBufferLength;
  Info->DataBufferOffset    = Info->ImageIndexOffset + ImageInfo->DataDefaultLength;

  Info->HiiGuidOffsetLength = ImageInfo->GlobalGuidTabIdxLength;
  Info->HiiVariableOffsetLength = ImageInfo->GlobalStrTabIdxLength;
  Info->ExtendedOffsetLength    = ImageInfo->GlobalOffsetLength;

  Info->SkuId = 0;

  DatabaseLen = DatabaseHeaderLength
              + ImageInfo->GuidTableLength
              + PcdIndexLength
              + CallbackBufferLength
              + ImageInfo->IndexLength
              + ImageInfo->WholeDataDefaultLength;

  Info->DatabaseLen = DatabaseLen;

  return DatabaseLen;
}


/**
  The function constructs a single PCD_INDEX according a index in
  <PCD_IMAGE>.

  @param[in] ImageIndex  The starting address of a PCD index defined in PCD spec 0.51.
  @param[in] Index       The output PCD_INDEX. 
  @param[in] ImageInfo   The attributes of the PCD_IMAGE as this binary stream is highly
                          optimized for size.

  @retval UINTN The length of the current PCD index.
**/
UINTN
BuildPcdIndex (
  IN CONST UINT8            *ImageIndex,
  OUT PCD_INDEX             *Index,
  IN CONST PCD_IMAGE_RECORD *ImageInfo
)
{
  UINTN       SkuCount;
  CONST UINT8 *ImageIndexBackUp;

  ImageIndexBackUp = ImageIndex;
  
  //
  // Token Number
  //
  CopyMem (&Index->TokenNumber,
            ImageIndex,
            ImageInfo->GlobalTokenLength
            );

  ImageIndex += ImageInfo->GlobalTokenLength;
  
  //
  // State Byte
  //
  PcdImageExpandStateByte (&Index->StateByte,
                           *ImageIndex
                           );

  ImageIndex += 1;
  
  //
  // Dataum Size
  //
  CopyMem (&Index->DatumSize,
          ImageIndex,
          ImageInfo->GlobalDatumLength
          );

  ImageIndex += ImageInfo->GlobalDatumLength;

  //
  // SKU_DATA
  //
  if (Index->StateByte.SkuEnable) {
    Index->SkuCount     = *ImageIndex;
    SkuCount            = *ImageIndex;
    ImageIndex++;
    Index->SkuIdArray   = (UINT32) ImageIndex - (UINT32) ImageInfo->IndexStart;
    ImageIndex         += Index->SkuCount;
  } else {
    //
    // There is always a default SKU_ID of zero even 
    // if SKU is not enabled for this PCD entry.
    // 
    //
    SkuCount = 1;
  }

  //
  // Extended Offset
  //
  CopyMem (&Index->ExtendedDataOffset,
           ImageIndex,
           ImageInfo->GlobalOffsetLength
           );

  ImageIndex += ImageInfo->GlobalOffsetLength * SkuCount;

  //
  // DynamicEX Guid Offset
  //
  if (Index->StateByte.ExtendedGuidPresent) {
    CopyMem (&Index->DynamicExGuid,
             ImageIndex,
             ImageInfo->GlobalGuidTabIdxLength
             );

    ImageIndex += ImageInfo->GlobalGuidTabIdxLength;
  }

  //
  // HII_DATA
  //
  if (Index->StateByte.HiiEnable) {
    Index->HiiData = (UINT32) ImageIndex - (UINT32) ImageInfo->IndexStart;
    ImageIndex += ((ImageInfo->GlobalStrTabIdxLength + ImageInfo->GlobalGuidTabIdxLength) * SkuCount);
  }

  return (UINTN) (ImageIndex - ImageIndexBackUp);
}




/**
  The function builds the PCD database based on the
  PCD_IMAGE on the flash.

  @param[in] Database  The database instance.
  @param[in] ImageIndex The starting address of a PCD index defined in PCD spec 0.51.
  @param[in] ImageInfo The attributes of the PCD_IMAGE as this binary stream is highly
              optimized for size.

  @retval VOID
**/
VOID
BuildPcdDatabaseIndex (
  PCD_DATABASE     *Database,
  UINT8            *ImageIndex,
  PCD_IMAGE_RECORD *ImageInfo
  )
{
  UINTN     Idx;
  UINTN     Len;
  PCD_INDEX *IndexTable;

  IndexTable = (PCD_INDEX *) GetAbsoluteAddress (0, Database->Info.PcdIndexOffset, Database);

  for (Idx = 0; Idx < Database->Info.EntryCount; Idx++) {
    Len = BuildPcdIndex (ImageIndex, &IndexTable[Idx], ImageInfo);
    ImageIndex += Len;
  }

  return;
}


/**
  The function builds the PCD database based on the
  PCD_IMAGE on the flash.

  @param[in] PcdImageOnFlash  The PCD image on flash.

  @retval VOID
--*/
VOID
BuildPcdDatabase (
  UINT8 *PcdImageOnFlash
  )
{
  PCD_DATABASE        *Database;
  UINTN               Len;
  PCD_IMAGE_RECORD    ImageInfo;
  UINT8               *ImageIndex;
  PCD_DATABASE_HEADER DatabaseHeader;

  Len = GetPcdDatabaseLen(PcdImageOnFlash, &DatabaseHeader, &ImageInfo);

  Database = BuildGuidHob (&gPcdDataBaseHobGuid, Len);
  ASSERT (Database != NULL);

  ZeroMem (Database, Len);

  //
  // Update Database header
  //
  CopyMem (&Database->Info, &DatabaseHeader, sizeof (DatabaseHeader));

  //
  // I need this to get the GuidTableOffset as we don't
  // know if Database field of PCD_DATABASE starts from an aligned
  // address. The compilor may add padding after PCD_DATABASE_HEADER field.
  //
  Database->Info.GuidTableOffset = ((UINTN) &Database->GuidTable) - (UINTN)Database;
  
  //
  // Copy Guid Table from Flash
  //
  CopyMem ((UINT8 *) Database + Database->Info.GuidTableOffset,
            ImageInfo.GuidTableStart,
            ImageInfo.GuidTableLength
            );

  //
  // Copy ImageIndex from Flash
  //
  CopyMem ((UINT8 *) Database + Database->Info.ImageIndexOffset,
            ImageInfo.IndexStart,
            ImageInfo.IndexLength
            );

  //
  // Copy Default Value
  //
  CopyMem ((UINT8 *) Database + Database->Info.DataBufferOffset,
           ImageInfo.DataDefaultStart,
           ImageInfo.DataDefaultLength
           );

  //
  // Copy String Table
  //
  CopyMem ((UINT8 *) Database + Database->Info.StringTableOffset,
           ImageInfo.StringTableStart,
           ImageInfo.StringTableLength
           );

  ImageIndex = GetAbsoluteAddress (0, Database->Info.ImageIndexOffset, Database);

  BuildPcdDatabaseIndex (Database, ImageIndex, &ImageInfo);

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
  IN  EFI_GUID     *VariableGuid,
  IN  UINT16       *VariableName,
  OUT VOID         **VariableData,
  OUT UINTN        *VariableSize
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
                          VariableGuid,
                          NULL,
                          &Size,
                          NULL
                            );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  Status = PeiCoreAllocatePool (Size, &Buffer);
  ASSERT_EFI_ERROR (Status);

  // declare a local for STP.
  //
  Status = VariablePpi->PeiGetVariable (
                            GetPeiServicesTablePointer (),
                            (UINT16 *) VariableName,
                            VariableGuid,
                            NULL,
                            &Size,
                            Buffer
                            );
  ASSERT_EFI_ERROR (Status);

  *VariableSize = Size;
  *VariableData = Buffer;

  return EFI_SUCCESS;
}



/**
  The function is provided by PCD PEIM and PCD DXE driver to
  do the work of reading a HII variable from variable service.

  @param[in] VariableGuid     The Variable GUID.
  @param[in] VariableName     The Variable Name.
  @param[in] Data             The input data.
  @param[out] VariableSize    The size of the variable.
  @param[in] Offset           The offset of the variable data that a PCD entry will starts from.

  @retval EFI_SUCCESS         Operation successful.
  @retval EFI_SUCCESS         Variablel not found.
--*/
EFI_STATUS
SetHiiVariable (
  IN  EFI_GUID     *VariableGuid,
  IN  UINT16       *VariableName,
  IN  CONST VOID   *Data,
  IN  UINTN        VariableSize,
  IN  UINTN        Offset
  )
{
  ASSERT (FALSE);
  return EFI_INVALID_PARAMETER;
}


