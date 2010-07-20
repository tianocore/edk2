/** @file
  Common filling functions used in translating Datahub's record
  to PI SMBIOS's record.
  
Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Thunk.h"

/**
  Field Filling Function for Cache SubClass record type 5&6 -- Cache SRAM type.
  Offset is mandatory
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldCacheType5 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_CACHE_SRAM_TYPE_DATA SramData;
  UINT32                   Temp;

  SramData = *(EFI_CACHE_SRAM_TYPE_DATA*)RecordData;

  //
  // Swap two fields because of inconsistency between smbios and datahub specs
  //
  Temp                  = SramData.Asynchronous;
  SramData.Asynchronous = SramData.Synchronous;
  SramData.Synchronous  = Temp;
  
  //
  // Truncate the data to word
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + Offset,
    (EFI_CACHE_SRAM_TYPE_DATA *) &SramData,
    2
    );

  return EFI_SUCCESS;
}

/**
  Field Filling Function for Cache SubClass record type 10 -- Cache Config.
  Offset is mandatory

  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldCacheType10 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{

  UINT16  CacheConfig;

  CopyMem (&CacheConfig, RecordData, 2);

  if ((CacheConfig & 0x07) == 0) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Truncate the data to 2 bytes and make cache level zero-based.
  //
  CacheConfig--;
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + Offset,
    &CacheConfig,
    2
    );

  return EFI_SUCCESS;
}

/**
  Enlarge the structure buffer of a structure node in SMBIOS database.
  The function maybe lead the structure pointer for SMBIOS record changed.
  
  @param StructureNode The structure node whose structure buffer is to be enlarged.
  @param NewLength     The new length of SMBIOS record which does not include unformat area.
  @param OldBufferSize The old size of SMBIOS record buffer.
  @param NewBufferSize The new size is targeted for enlarged.
  
  @retval EFI_OUT_OF_RESOURCES  No more memory to allocate new record
  @retval EFI_SUCCESS           Success to enlarge the record buffer size.
**/
EFI_STATUS
SmbiosEnlargeStructureBuffer (
  IN OUT  SMBIOS_STRUCTURE_NODE *StructureNode,
  UINT8                         NewLength,
  UINTN                         OldBufferSize,
  UINTN                         NewBufferSize
  )
{
  EFI_SMBIOS_TABLE_HEADER   *NewRecord;
  EFI_SMBIOS_PROTOCOL       *Smbios;
  EFI_STATUS                Status;
  UINT8                     CountOfString;
  
  NewRecord = NULL;
  Smbios    = GetSmbiosProtocol();
  ASSERT (Smbios != NULL);
  
  NewRecord = (EFI_SMBIOS_TABLE_HEADER*) AllocateZeroPool (NewBufferSize);
  if (NewRecord == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (NewRecord, StructureNode->Structure, OldBufferSize);
  
  
  Status = Smbios->Remove (Smbios, StructureNode->SmbiosHandle);
  ASSERT_EFI_ERROR (Status);
  
  //
  // try to use original handle to enlarge the buffer.
  //
  NewRecord->Length = NewLength;
  Status = Smbios->Add (Smbios, NULL, &StructureNode->SmbiosHandle, NewRecord);
  ASSERT_EFI_ERROR (Status);
  FreePool (NewRecord);
  
  StructureNode->Structure = GetSmbiosBufferFromHandle (
                               StructureNode->SmbiosHandle, 
                               StructureNode->SmbiosType, 
                               NULL
                               );
  GetSmbiosStructureSize (
    StructureNode->Structure,
    &StructureNode->StructureSize,
    &CountOfString
    );
  return EFI_SUCCESS;
}

/**
  Update the structure buffer of a structure node in SMBIOS database.
  The function lead the structure pointer for SMBIOS record changed.
  
  @param StructureNode The structure node whose structure buffer is to be enlarged.
  @param NewRecord     The new SMBIOS record.
  
**/
VOID
SmbiosUpdateStructureBuffer (
  IN OUT  SMBIOS_STRUCTURE_NODE *StructureNode,
  IN EFI_SMBIOS_TABLE_HEADER    *NewRecord
  )
{
  EFI_SMBIOS_PROTOCOL       *Smbios;
  EFI_STATUS                Status;
  UINT8                     CountOfString;
  
  Smbios    = GetSmbiosProtocol();
  ASSERT (Smbios != NULL);
  
  Status = Smbios->Remove (Smbios, StructureNode->SmbiosHandle);
  ASSERT_EFI_ERROR (Status);
  
  //
  // try to use original handle to enlarge the buffer.
  //
  Status = Smbios->Add (Smbios, NULL, &StructureNode->SmbiosHandle, NewRecord);
  ASSERT_EFI_ERROR (Status);
  
  StructureNode->Structure = GetSmbiosBufferFromHandle (
                               StructureNode->SmbiosHandle, 
                               StructureNode->SmbiosType, 
                               NULL
                               );
  GetSmbiosStructureSize (
    StructureNode->Structure,
    &StructureNode->StructureSize,
    &CountOfString
    );
  return ;
}

/**
  Fill a standard Smbios string field. 
  
  This function will convert the unicode string to single byte chars, and only
  English language is supported.
  This function changes the Structure pointer value of the structure node, 
  which should be noted by Caller.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER   RecordDataSize is too larger
  @retval EFI_OUT_OF_RESOURCES    No memory to allocate new buffer for string
  @retval EFI_SUCCESS             Sucess append string for a SMBIOS record.
**/
EFI_STATUS
SmbiosFldString (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS              Status;
  UINT16                  *Data;
  CHAR8                   AsciiData[SMBIOS_STRING_MAX_LENGTH];
  UINT8                   CountOfStrings;
  UINTN                   OrigStringNumber;
  EFI_SMBIOS_PROTOCOL     *Smbios;
  EFI_SMBIOS_HANDLE       SmbiosHandle;
  UINT32                  OrigStructureSize;
  UINTN                   NewStructureSize;
  EFI_SMBIOS_TABLE_HEADER *NewRecord;
  UINT32                  StringLength;
  
  Status            = EFI_SUCCESS;
  OrigStringNumber  = 0;
  OrigStructureSize = 0;

  //
  // if we have a NULL token,
  //
  if (0 == *((STRING_REF *) RecordData)) {
    *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + Offset) = 0;
    return EFI_SUCCESS;
  }
  
  //
  // Get the String from the Hii Database
  //
  Data = HiiGetPackageString (
             &(StructureNode->ProducerName),
             *((EFI_STRING_ID *) RecordData),
             NULL
             );
  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StringLength = (UINT32)StrLen (Data);
  //
  // Count the string size including the terminating 0.
  //
  if (StringLength == 0) {
    *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + Offset) = 0;
    FreePool (Data);
    return EFI_SUCCESS;
  }
  
  if (StringLength > SMBIOS_STRING_MAX_LENGTH) {
    //
    // Too long a string
    //
    FreePool (Data);
    return EFI_INVALID_PARAMETER;
  }
    
  Smbios = GetSmbiosProtocol();
  ASSERT (Smbios != NULL);
  
  //
  // Convert Unicode string to Ascii string which only supported by SMBIOS.
  //
  ZeroMem (AsciiData, SMBIOS_STRING_MAX_LENGTH);
  UnicodeStrToAsciiStr (Data, AsciiData);
  
  //
  // if the field at offset is already filled with some value,
  // find out the string it points to
  //
  OrigStringNumber = *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + Offset);
  if (OrigStringNumber != 0) {
    DEBUG ((EFI_D_ERROR, "[SMBIOSThunk] Update %dth string for type[%d],offset[0x%x],handle[0x%x] to [%s]\n", 
            OrigStringNumber, StructureNode->SmbiosType, Offset, StructureNode->SmbiosHandle, AsciiData));  
    
    //
    // If original string number is not zero, just update string
    //
    Status = Smbios->UpdateString (Smbios, &StructureNode->SmbiosHandle, &OrigStringNumber, AsciiData);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[SMBIOSThunk] Fail to update %dth string in offset 0x%x for handle:0x%x type:0x%x\n", 
             OrigStringNumber, Offset, StructureNode->SmbiosHandle, StructureNode->SmbiosType));
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  } else {
    //
    // If the string has not been filled in SMBIOS database, remove it and add again
    // with string appended.
    //
    Status = GetSmbiosStructureSize (StructureNode->Structure, &OrigStructureSize, &CountOfStrings);
    ASSERT_EFI_ERROR (Status);
    
    if (CountOfStrings == 0) {
      NewStructureSize = OrigStructureSize + StringLength;
    } else {
      NewStructureSize = OrigStructureSize + StringLength + 1;
    }
    
    NewRecord = AllocateZeroPool (NewStructureSize);
    if (NewRecord == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (NewRecord, StructureNode->Structure, OrigStructureSize);
    
    //
    // Copy new string into tail of original SMBIOS record buffer.
    //
    if (CountOfStrings == 0) {
      AsciiStrCpy ((CHAR8 *)NewRecord + OrigStructureSize - 2, AsciiData);
    } else {
      AsciiStrCpy ((CHAR8 *)NewRecord + OrigStructureSize - 1, AsciiData);
    }
    DEBUG ((EFI_D_ERROR, "[SMBIOSThunk] Type(%d) offset(0x%x) StringNumber:%d\n", 
            StructureNode->SmbiosType, Offset, CountOfStrings + 1));
    //
    // Update string reference number
    //
    *(UINT8 *) ((UINT8 *) NewRecord + Offset) = (UINT8) (CountOfStrings + 1);
    SmbiosHandle = StructureNode->SmbiosHandle;
    
    //
    // Remove original SMBIOS record and add new one
    //
    Status = Smbios->Remove (Smbios, StructureNode->SmbiosHandle);
    ASSERT_EFI_ERROR (Status);
    
    //
    // Add new SMBIOS record
    //
    Status = Smbios->Add (Smbios, NULL, &SmbiosHandle, NewRecord);
    ASSERT_EFI_ERROR (Status);
    
    StructureNode->SmbiosHandle = SmbiosHandle;
    
    FreePool (NewRecord);
  }
  
  //
  // The SMBIOS record buffer maybe re-allocated in SMBIOS database,
  // so update cached buffer pointer in DataHub structure list.
  //
  StructureNode->Structure = GetSmbiosBufferFromHandle (
                               StructureNode->SmbiosHandle, 
                               StructureNode->SmbiosType,
                               NULL
                               );
  ASSERT (StructureNode->Structure != NULL);
  
  //
  // The string update action maybe lead the record is re-allocated in SMBIOS database
  // so update cached record pointer
  //
  Status = GetSmbiosStructureSize (StructureNode->Structure, &StructureNode->StructureSize, &CountOfStrings);
  ASSERT_EFI_ERROR (Status);
  
  return EFI_SUCCESS;
}

/**
  Find a handle that matches the Link Data and the target Smbios type.
  
  @param  TargetType     the Smbios type
  @param  SubClass       the SubClass
  @param  LinkData       Specifies Instance, SubInstance and ProducerName
  @param  Handle         the HandleNum found  
  
  @retval EFI_NOT_FOUND Can not find the record according to handle
  @retval EFI_SUCCESS   Success to find the handle
**/
EFI_STATUS
SmbiosFindHandle (
  IN      UINT8               TargetType,
  IN      EFI_GUID            *SubClass,
  IN      EFI_INTER_LINK_DATA *LinkData,
  IN OUT  UINT16              *HandleNum
  )
{
  LIST_ENTRY        *Link;
  SMBIOS_STRUCTURE_NODE *StructureNode;

  StructureNode = NULL;

  //
  // Find out the matching handle
  //
  for (Link = mStructureList.ForwardLink; Link != &mStructureList; Link = Link->ForwardLink) {
    StructureNode = CR (Link, SMBIOS_STRUCTURE_NODE, Link, SMBIOS_STRUCTURE_NODE_SIGNATURE);
    if (StructureNode->Structure->Type == TargetType &&
        CompareGuid (&(StructureNode->SubClass), SubClass) &&
        StructureNode->Instance == LinkData->Instance &&
        StructureNode->SubInstance == LinkData->SubInstance
        ) {
      break;
    }
  }

  if (Link == &mStructureList || StructureNode == NULL) {
    return EFI_NOT_FOUND;
  } else {
    *HandleNum = StructureNode->Structure->Handle;
    return EFI_SUCCESS;
  }
}

/**
  Fill the inter link field for a SMBIOS recorder.
  
  Some SMBIOS recorder need to reference the handle of another SMBIOS record. But
  maybe another SMBIOS record has not been added, so put the InterLink request into
  a linked list and the interlink will be fixedup when a new SMBIOS record is added.
  
  @param StructureNode        Point to SMBIOS_STRUCTURE_NODE which reference another record's handle
  @param LinkSmbiosNodeOffset The offset in this record for holding the handle of another SMBIOS record
  @param LinkSmbiosType       The type of SMBIOS record want to be linked.
  @param InterLink            Point to EFI_INTER_LINK_DATA will be put linked list.
  @param SubClassGuid         The guid of subclass for linked SMBIOS record.
  
  @retval EFI_SUCESS  The linked record is found and no need fixup in future.
  @retval !EFI_SUCESS The linked record can not be found and InterLink is put a fixing-p linked list.
**/
EFI_STATUS
SmbiosFldInterLink (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT16                    LinkSmbiosNodeOffset,
  IN      UINT8                     LinkSmbiosType,
  IN      EFI_INTER_LINK_DATA       *InterLink,
  IN      EFI_GUID                  *SubClassGuid
  )
{
  EFI_STATUS                  Status;
  SMBIOS_LINK_DATA_FIXUP_NODE *LinkDataFixupNode;
  UINT16                      StructureHandle;

  Status            = EFI_SUCCESS;
  LinkDataFixupNode = NULL;

  Status = SmbiosFindHandle (
            LinkSmbiosType, // Smbios type
            SubClassGuid,
            InterLink,
            &StructureHandle
            );
  if (!EFI_ERROR (Status)) {
    //
    // Set the handle
    //
    CopyMem (
      (UINT8 *) (StructureNode->Structure) + LinkSmbiosNodeOffset,
      &StructureHandle,
      sizeof (EFI_SMBIOS_HANDLE)
      );
  } else {
    //
    // Hang this in the link data fixup node
    //
    LinkDataFixupNode = AllocateZeroPool (sizeof (SMBIOS_LINK_DATA_FIXUP_NODE));
    if (LinkDataFixupNode == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    LinkDataFixupNode->Signature  = SMBIOS_LINK_DATA_FIXUP_NODE_SIGNATURE;
    LinkDataFixupNode->Offset     = LinkSmbiosNodeOffset;
    LinkDataFixupNode->TargetType = LinkSmbiosType;
    CopyMem (
      &LinkDataFixupNode->SubClass,
      SubClassGuid,
      sizeof (EFI_GUID)
      );
    CopyMem (
      &LinkDataFixupNode->LinkData,
      InterLink,
      sizeof (EFI_INTER_LINK_DATA)
      );
    InsertTailList (
      &StructureNode->LinkDataFixup,
      &(LinkDataFixupNode->Link)
      );
  }

  return Status;
}

/**
  Field Filling Function. Transform an EFI_EXP_BASE10_DATA to a word, with 'Mega'
  as the unit.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER  RecordDataSize is invalid. 
  @retval EFI_SUCCESS            RecordData is successed to be filled into given SMBIOS record.
**/
EFI_STATUS
SmbiosFldBase10ToWordWithMega (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_EXP_BASE10_DATA *Base10Data;
  INT16               Value;
  INT16               Exponent;

  if (RecordDataSize != sizeof (EFI_EXP_BASE10_DATA)) {
    return EFI_INVALID_PARAMETER;
  }

  Base10Data  = RecordData;
  Value       = Base10Data->Value;
  Exponent    = Base10Data->Exponent;

  Exponent -= 6;
  while (Exponent != 0) {
    if (Exponent > 0) {
      Value = (INT16) (Value * 10);
      Exponent--;
    } else {
      Value = (INT16) (Value / 10);
      Exponent++;
    }
  }

  CopyMem (
    (UINT8 *) (StructureNode->Structure) + Offset,
    &Value,
    2
    );

  return EFI_SUCCESS;
}

/**
  Field Filling Function. Transform an EFI_EXP_BASE2_DATA to a word, with 'Kilo'
  as the unit. Granularity implemented for Cache Size.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER  RecordDataSize is invalid. 
  @retval EFI_SUCCESS            RecordData is successed to be filled into given SMBIOS record.
**/
EFI_STATUS
SmbiosFldBase2ToWordWithKilo (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_EXP_BASE2_DATA  *Base2Data;
  UINT32              Value;
  UINT16              Exponent;

  if (RecordDataSize != sizeof (EFI_EXP_BASE2_DATA)) {
    return EFI_INVALID_PARAMETER;
  }

  Base2Data = RecordData;
  Value     = Base2Data->Value;
  Exponent  = Base2Data->Exponent;

  Exponent -= 10;
  Value <<= Exponent;
  
  //
  // Implement cache size granularity
  //
  if(Value >= 0x8000) {
    Value >>= 6;
    Value |=  0x8000;
  }
  
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + Offset,
    &Value,
    2
    );

  return EFI_SUCCESS;
}

/**
  Field Filling Function. Transform an EFI_EXP_BASE2_DATA to a byte, with '64k'
  as the unit.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER  RecordDataSize is invalid. 
  @retval EFI_SUCCESS            RecordData is successed to be filled into given SMBIOS record.
**/
EFI_STATUS
SmbiosFldBase2ToByteWith64K (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_EXP_BASE2_DATA  *Base2Data;
  UINT16              Value;
  UINT16              Exponent;

  if (RecordDataSize != sizeof (EFI_EXP_BASE2_DATA)) {
    return EFI_INVALID_PARAMETER;
  }

  Base2Data = RecordData;
  Value     = Base2Data->Value;
  Exponent  = Base2Data->Exponent;
  Exponent -= 16;
  Value <<= Exponent;

  CopyMem (
    (UINT8 *) (StructureNode->Structure) + Offset,
    &Value,
    1
    );

  return EFI_SUCCESS;
}

/**
  Field Filling Function. Transform an EFI_EXP_BASE2_DATA to a word, with 10exp-9
  as the unit.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER  RecordDataSize is invalid. 
  @retval EFI_SUCCESS            RecordData is successed to be filled into given SMBIOS record.
**/
EFI_STATUS
SmbiosFldBase10ToByteWithNano (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_EXP_BASE10_DATA *Base10Data;
  INT16               Value;
  INT16               Exponent;

  if (RecordDataSize != sizeof (EFI_EXP_BASE2_DATA)) {
    return EFI_INVALID_PARAMETER;
  }

  Base10Data  = RecordData;
  Value       = Base10Data->Value;
  Exponent    = Base10Data->Exponent;

  Exponent += 9;
  while (Exponent != 0) {
    if (Exponent > 0) {
      Value = (INT16) (Value * 10);
      Exponent--;
    } else {
      Value = (INT16) (Value / 10);
      Exponent++;
    }
  }

  * (UINT8 *) ((UINT8 *) (StructureNode->Structure) + Offset) = (UINT8) Value;

  return EFI_SUCCESS;
}

/**
  Field Filling Function: truncate record data to byte and fill in the
  field as indicated by Offset.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER  RecordDataSize is invalid. 
  @retval EFI_SUCCESS            RecordData is successed to be filled into given SMBIOS record.
**/
EFI_STATUS
SmbiosFldTruncateToByte (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  //
  // Truncate the data to 8 bits
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + Offset) = (UINT8) (*(UINT8 *) RecordData);

  return Status;
}

/**
  Field Filling Function: truncate record data to byte and fill in the
  field as indicated by Offset.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_INVALID_PARAMETER  RecordDataSize is invalid. 
  @retval EFI_SUCCESS            RecordData is successed to be filled into given SMBIOS record.
**/
EFI_STATUS
SmbiosFldTruncateToWord (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  //
  // Truncate the data to 8 bits
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + Offset,
    RecordData,
    2
    );

  return Status;
}

/**
  Check if OEM structure has included 2 trailing 0s in data record.
  
  @param RecordData       Point to record data will be checked.
  @param RecordDataSize   The size of record data.
  
  @retval 0    2 trailing 0s exist in unformatted section
  @retval 1    1 trailing 0 exists at the end of unformatted section
  @retval -1   There is no 0 at the end of unformatted section
**/
INT8
SmbiosCheckTrailingZero (
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  SMBIOS_STRUCTURE  *Smbios;
  CHAR8             *Start;
  CHAR8             *End;
  
  Smbios = (SMBIOS_STRUCTURE *) RecordData;
  
  //
  // Skip over formatted section
  //
  Start = (CHAR8 *) ((UINT8 *) Smbios + Smbios->Length);
  End   = (CHAR8 *) RecordData + RecordDataSize;
  
  //
  // Unformatted section exists
  //
  while (Start < End - 1) {
    //
    // Avoid unaligned issue on IPF
    //
    if ((*Start == 0) && (*(Start + 1) == 0)) {
      //
      // 2 trailing 0s exist in unformatted section
      //
      return 0;
    }
    Start++;
  }
  
  if (Start == End - 1) {
    //
    // Check if there has been already 1 trailing 0
    //
    if (*Start == 0) {
      return 1;
    } 
  }
  
  //
  // There is no 0 at the end of unformatted section
  //
  return -1;
}
