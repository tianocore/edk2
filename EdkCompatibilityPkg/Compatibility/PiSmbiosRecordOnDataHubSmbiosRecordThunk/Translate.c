/** @file
  Translate the DataHub records via EFI_DATA_HUB_PROTOCOL to Smbios recorders 
  via EFI_SMBIOS_PROTOCOL.
  
Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Thunk.h"

EFI_GUID  ZeroGuid = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };
EFI_SMBIOS_PROTOCOL  *mSmbiosProtocol = NULL;

/**
  Release the structure Node. 
  
  @param StructureNode  Point to SMBIOS_STRUCTURE_NODE which will be removed.
**/
VOID
ReleaseStructureNode (
  SMBIOS_STRUCTURE_NODE  *StructureNode
  )
{
  EFI_SMBIOS_PROTOCOL *Smbios;
  
  RemoveEntryList (&(StructureNode->Link));
  Smbios = GetSmbiosProtocol();
  ASSERT (Smbios != NULL);
  Smbios->Remove (Smbios, StructureNode->SmbiosHandle);
  gBS->FreePool (StructureNode);
}

/**
  Process a datahub's record and find corresponding translation way to translate
  to SMBIOS record.
  
  @param Record  Point to datahub record.
**/
VOID
SmbiosProcessDataRecord (
  IN EFI_DATA_RECORD_HEADER  *Record
  )
{
  EFI_DATA_RECORD_HEADER        *RecordHeader;
  EFI_SUBCLASS_TYPE1_HEADER     *DataHeader;
  UINTN                         Index;
  SMBIOS_CONVERSION_TABLE_ENTRY *Conversion;
  UINT8                         *SrcData;
  UINTN                         SrcDataSize;
  LIST_ENTRY                    *Link;
  SMBIOS_STRUCTURE_NODE         *StructureNode;
  BOOLEAN                       StructureCreated;
  EFI_STATUS                    Status;
  
  Conversion        = NULL;
  StructureNode     = NULL;
  StructureCreated  = FALSE;
  RecordHeader      = Record;
  DataHeader        = (EFI_SUBCLASS_TYPE1_HEADER *) (Record + 1);
  SrcData           = (UINT8 *) (DataHeader + 1);
  SrcDataSize       = RecordHeader->RecordSize - RecordHeader->HeaderSize - sizeof (EFI_SUBCLASS_TYPE1_HEADER);

  if (DataHeader->HeaderSize != sizeof (EFI_SUBCLASS_TYPE1_HEADER) ||
      DataHeader->Instance == EFI_SUBCLASS_INSTANCE_RESERVED ||
      DataHeader->SubInstance == EFI_SUBCLASS_INSTANCE_RESERVED
      ) {
    //
    // Invalid Data Record
    //
    goto Done;
  }

  Index = 0;
  while(TRUE) {
    //
    // Find a matching entry in the conversion table for this
    // (SubClass, RecordNumber) pair
    //
    for (; !CompareGuid (&(mConversionTable[Index].SubClass), &ZeroGuid); Index++) {
      if (CompareGuid (
            &(mConversionTable[Index].SubClass),
            &(RecordHeader->DataRecordGuid)
            )) {
        if (mConversionTable[Index].RecordType == DataHeader->RecordType) {
          break;
        }
      }
    }

    if (CompareGuid (&(mConversionTable[Index].SubClass), &ZeroGuid)) {
      //
      // We cannot find a matching entry in conversion table,
      // this means this data record cannot be used for SMBIOS.
      // Just skip it.
      //
      goto Done;
    }

    Conversion = &mConversionTable[Index++];

    //
    // Find corresponding structure in the Structure List
    //
    for (Link = mStructureList.ForwardLink; Link != &mStructureList; Link = Link->ForwardLink) {

      StructureNode = CR (
                        Link,
                        SMBIOS_STRUCTURE_NODE,
                        Link,
                        SMBIOS_STRUCTURE_NODE_SIGNATURE
                        );

      if (Conversion->StructureLocatingMethod == BySubclassInstanceSubinstanceProducer) {
        //
        // Look at SubClass, Instance, SubInstance and ProducerName for a matching
        // node
        //
        if (CompareGuid (&(StructureNode->SubClass), &(RecordHeader->DataRecordGuid)) &&
            StructureNode->Instance == DataHeader->Instance &&
            StructureNode->SubInstance == DataHeader->SubInstance &&
            CompareGuid (&(StructureNode->ProducerName), &(RecordHeader->ProducerName))
              ) {
          if (Conversion->SmbiosType >= 0x80) {
            if (StructureNode->SmbiosType == ((SMBIOS_STRUCTURE_HDR *) SrcData)->Type) {
              break;
            }
          } else if (StructureNode->SmbiosType == Conversion->SmbiosType) {
            break;
          }
        }

      } else if (Conversion->StructureLocatingMethod == BySubClassInstanceProducer) {
        //
        // Look at SubClass, Instance and ProducerName for a matching node
        //
        if (CompareGuid (&(StructureNode->SubClass), &(RecordHeader->DataRecordGuid)) &&
            StructureNode->Instance == DataHeader->Instance &&
            CompareGuid (&(StructureNode->ProducerName), &(RecordHeader->ProducerName))
              ) {
          if (Conversion->SmbiosType >= 0x80) {
            if (StructureNode->SmbiosType == ((SMBIOS_STRUCTURE_HDR *) SrcData)->Type) {
              break;
            }
          } else if (StructureNode->SmbiosType == Conversion->SmbiosType) {
            break;
          }
        }

      } else {
        //
        // Invalid conversion table entry
        //
        goto Done;
      }
    }

    if (Link == &mStructureList || StructureNode == NULL) {

      //
      // Not found, create a new structure
      //
      StructureNode = AllocateZeroPool (sizeof (SMBIOS_STRUCTURE_NODE));

      if (StructureNode == NULL) {
        goto Done;
      }

      if (Conversion->StructureLocatingMethod == BySubclassInstanceSubinstanceProducer) {
        //
        // Fill in SubClass, Instance, SubInstance and ProducerName
        //
        CopyMem (&(StructureNode->SubClass), &(RecordHeader->DataRecordGuid), sizeof (EFI_GUID));
        StructureNode->Instance     = DataHeader->Instance;
        StructureNode->SubInstance  = DataHeader->SubInstance;
        CopyMem (&(StructureNode->ProducerName), &(RecordHeader->ProducerName), sizeof (EFI_GUID));

      } else if (Conversion->StructureLocatingMethod == BySubClassInstanceProducer) {
        //
        // Fill in at SubClass, Instance and ProducerName, mark SubInstance as Non
        // Applicable
        //
        CopyMem (&(StructureNode->SubClass), &(RecordHeader->DataRecordGuid), sizeof (EFI_GUID));
        StructureNode->Instance     = DataHeader->Instance;
        StructureNode->SubInstance  = EFI_SUBCLASS_INSTANCE_NON_APPLICABLE;
        CopyMem (&(StructureNode->ProducerName), &(RecordHeader->ProducerName), sizeof (EFI_GUID));

      }
      //
      // Allocate the structure instance
      //
      StructureNode->StructureSize = SmbiosGetTypeMinimalLength (Conversion->SmbiosType);
      
      //
      // StructureSize include the TWO trailing zero byte.
      //
      if (StructureNode->StructureSize < (sizeof(SMBIOS_STRUCTURE) + 2)) {
        //
        // Invalid Type
        //
        gBS->FreePool (StructureNode);
        goto Done;
      }

      //
      // Assign correct SmbiosType when OEM type and Non-OEM type
      //
      if (Conversion->SmbiosType >= 0x80) {
        StructureNode->SmbiosType = ((SMBIOS_STRUCTURE_HDR *) SrcData)->Type;
      } else {
        StructureNode->SmbiosType = Conversion->SmbiosType;
      }
      
      StructureNode->SmbiosHandle        = SMBIOS_HANDLE_PI_RESERVED;
      Status = SmbiosProtocolCreateRecord (
                 NULL, 
                 StructureNode
                 );
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      //
      // Temporary cache the structrue pointer to Smbios database.
      //
      StructureNode->Structure = GetSmbiosBufferFromHandle (StructureNode->SmbiosHandle, StructureNode->SmbiosType, NULL);
      
      InitializeListHead (&StructureNode->LinkDataFixup);

      //
      // Insert the Structure Node into the Strucutre List
      //
      StructureNode->Signature = SMBIOS_STRUCTURE_NODE_SIGNATURE;
      InsertTailList (&mStructureList, &(StructureNode->Link));

      StructureCreated = TRUE;

    }
    
    
    //
    // Re-calculate the structure pointer to Smbios database.
    //
    StructureNode->Structure = GetSmbiosBufferFromHandle (StructureNode->SmbiosHandle, StructureNode->SmbiosType, NULL);
    
    //
    // Fill the Structure's field corresponding to this data record
    //
    if (Conversion->FieldFillingMethod == RecordDataUnchangedOffsetSpecified) {
      //
      // Field data is just the record data without transforming and
      // offset is specified directly in the conversion table entry
      //
      if (Conversion->FieldOffset + SrcDataSize > StructureNode->Structure->Length) {
        //
        // Invalid Conversion Table Entry
        //
        if (StructureCreated) {
          ReleaseStructureNode (StructureNode);
        }

        goto Done;
      }
      
      CopyMem ((UINT8 *) (StructureNode->Structure) + Conversion->FieldOffset, SrcData, SrcDataSize);

    } else if (Conversion->FieldFillingMethod == ByFunctionWithOffsetSpecified) {
      //
      // Field offfset is specified in the conversion table entry, but
      // record data needs to be transformed to be filled into the field,
      // so let the FieldFillingFunction do it.
      //
      if (Conversion->FieldFillingFunction == NULL) {
        //
        // Invalid Conversion Table Entry
        //
        if (StructureCreated) {
          ReleaseStructureNode (StructureNode);
        }

        goto Done;
      }

      Status = Conversion->FieldFillingFunction (
                            StructureNode,
                            Conversion->FieldOffset,
                            SrcData,
                            (UINT32) SrcDataSize
                            );
      if (EFI_ERROR (Status)) {
        if (StructureCreated) {
          ReleaseStructureNode (StructureNode);
        }

        goto Done;
      }
    } else if (Conversion->FieldFillingMethod == ByFunction) {
      //
      // Both field offset and field content are determined by
      // FieldFillingFunction
      //
      if (Conversion->FieldFillingFunction == NULL) {
        //
        // Invalid Conversion Table Entry
        //
        if (StructureCreated) {
          ReleaseStructureNode (StructureNode);
        }

        goto Done;
      }

      Status = Conversion->FieldFillingFunction (
                            StructureNode,
                            0,
                            SrcData,
                            (UINT32) SrcDataSize
                            );
      if (EFI_ERROR (Status)) {
        if (StructureCreated) {
          ReleaseStructureNode (StructureNode);
        }

        goto Done;
      }
    } else if (Conversion->FieldFillingMethod == ByFunctionWithWholeDataRecord) {
      //
      // Both field offset and field content are determined by
      // FieldFillingFunction and the function accepts the whole data record
      // including the data header
      //
      if (Conversion->FieldFillingFunction == NULL) {
        //
        // Invalid Conversion Table Entry
        //
        if (StructureCreated) {
          ReleaseStructureNode (StructureNode);
        }

        goto Done;
      }

      Status = Conversion->FieldFillingFunction (
                            StructureNode,
                            0,
                            DataHeader,
                            RecordHeader->RecordSize - RecordHeader->HeaderSize
                            );
      if (EFI_ERROR (Status)) {
        if (StructureCreated) {
          ReleaseStructureNode (StructureNode);
        }

        goto Done;
      }
    } else {
      //
      // Invalid Conversion Table Entry
      //
      if (StructureCreated) {
        ReleaseStructureNode (StructureNode);
      }

      goto Done;
    }
    
    //
    // SmbiosEnlargeStructureBuffer is called to remove and add again
    // this SMBIOS entry to reflash SMBIOS table in configuration table.
    //
    SmbiosEnlargeStructureBuffer (
      StructureNode,
      StructureNode->Structure->Length,
      StructureNode->StructureSize,
      StructureNode->StructureSize
      );
  }
Done:
  return ;
}

/**
  Calculate the minimal length for a SMBIOS type. This length maybe not equal
  to sizeof (SMBIOS_RECORD_STRUCTURE), but defined in conformance chapter in SMBIOS specification.
  
  @param Type  SMBIOS's type.
  
  @return the minimal length of a smbios record.
**/
UINT32
SmbiosGetTypeMinimalLength (
  IN UINT8  Type
  )
{
  UINTN Index;

  for (Index = 0; mTypeInfoTable[Index].MinLength != 0; Index++) {
    if (mTypeInfoTable[Index].Type == Type) {
      return mTypeInfoTable[Index].MinLength;
    }
  }

  return 0;
}

/**
  Get pointer of EFI_SMBIOS_PROTOCOL.
  
  @return pointer of EFI_SMBIOS_PROTOCOL.
**/
EFI_SMBIOS_PROTOCOL*
GetSmbiosProtocol(
  VOID
  )
{
  EFI_STATUS  Status;
  
  if (mSmbiosProtocol == NULL) {
    Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID*) &mSmbiosProtocol);
    ASSERT_EFI_ERROR (Status);
  } 
  
  ASSERT (mSmbiosProtocol != NULL);
  return mSmbiosProtocol; 
}  

/**
  Create a blank smbios record. The datahub record is only a field of smbios record.
  So before fill any field from datahub's record. A blank smbios record need to be 
  created.
  
  @param ProducerHandle   The produce handle for a datahub record
  @param StructureNode    Point to SMBIOS_STRUCTURE_NODE
  
  @retval EFI_OUT_OF_RESOURCES Fail to allocate memory for new blank SMBIOS record.
  @retval EFI_SUCCESS          Success to create blank smbios record.
**/
EFI_STATUS
SmbiosProtocolCreateRecord (
  IN      EFI_HANDLE              ProducerHandle, OPTIONAL
  IN      SMBIOS_STRUCTURE_NODE   *StructureNode
  )
{
  EFI_SMBIOS_PROTOCOL         *Smbios;
  EFI_SMBIOS_TABLE_HEADER     *BlankRecord;
  EFI_STATUS                  Status;
  SMBIOS_STRUCTURE_NODE       *RefStructureNode;
  LIST_ENTRY                  *Link;
  LIST_ENTRY                  *Link1;
  LIST_ENTRY                  *Link2;  
  SMBIOS_LINK_DATA_FIXUP_NODE *LinkDataFixupNode;
  UINT8                       *BufferPointer;
  
  Smbios = GetSmbiosProtocol();
  ASSERT (Smbios != NULL);
  
  //
  // Prepare a blank smbios record.
  //
  BlankRecord = (EFI_SMBIOS_TABLE_HEADER*) AllocateZeroPool (StructureNode->StructureSize);
  if (BlankRecord == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  BlankRecord->Type   = StructureNode->SmbiosType;
  BlankRecord->Length = (UINT8) (StructureNode->StructureSize - 2);
  
  //
  // Add blank record into SMBIOS database.
  //
  Status = Smbios->Add (Smbios, NULL, &StructureNode->SmbiosHandle, BlankRecord);
  FreePool (BlankRecord);
  
  //
  // Fix up the InterLink node for new added smbios record if some other
  // existing smbios record want to link this new record's handle.
  //
  for (Link = mStructureList.ForwardLink; Link != &mStructureList; Link = Link->ForwardLink) {
    RefStructureNode = CR (Link, SMBIOS_STRUCTURE_NODE, Link, SMBIOS_STRUCTURE_NODE_SIGNATURE);
    for (Link1 = RefStructureNode->LinkDataFixup.ForwardLink; Link1 != &RefStructureNode->LinkDataFixup;) {
      LinkDataFixupNode = CR (Link1, SMBIOS_LINK_DATA_FIXUP_NODE, Link, SMBIOS_LINK_DATA_FIXUP_NODE_SIGNATURE);
      Link2 = Link1;
      Link1 = Link1->ForwardLink;
            
      if ((StructureNode->SmbiosType != LinkDataFixupNode->TargetType) ||
          !(CompareGuid (&StructureNode->SubClass, &LinkDataFixupNode->SubClass)) ||
          (StructureNode->Instance != LinkDataFixupNode->LinkData.Instance) ||
          (StructureNode->SubInstance != LinkDataFixupNode->LinkData.SubInstance)) {
        continue;
      }
      
      //
      // Fill the field with the handle found
      //
      BufferPointer         = (UINT8 *) (RefStructureNode->Structure) + LinkDataFixupNode->Offset;
      *BufferPointer        = (UINT8) (StructureNode->SmbiosHandle & 0xFF);
      *(BufferPointer + 1)  = (UINT8) ((StructureNode->SmbiosHandle >> 8) & 0xFF);
      BufferPointer         = NULL;

      RemoveEntryList (Link2);
      FreePool (LinkDataFixupNode);
    }
  }
  
  return Status;
}  

/**
  Get pointer of a SMBIOS record's buffer according to its handle.
  
  @param Handle         The handle of SMBIOS record want to be searched.
  @param Type           The type of SMBIOS record want to be searched.
  @param ProducerHandle The producer handle of SMBIOS record.
  
  @return EFI_SMBIOS_TABLE_HEADER Point to a SMBIOS record's buffer.
**/  
EFI_SMBIOS_TABLE_HEADER*
GetSmbiosBufferFromHandle (
  IN  EFI_SMBIOS_HANDLE  Handle,
  IN  EFI_SMBIOS_TYPE    Type,
  IN  EFI_HANDLE         ProducerHandle  OPTIONAL
  )
{
  EFI_SMBIOS_PROTOCOL*    Smbios;
  EFI_SMBIOS_HANDLE       SearchingHandle;
  EFI_SMBIOS_TABLE_HEADER *RecordInSmbiosDatabase;
  EFI_STATUS              Status;
  
  SearchingHandle = SMBIOS_HANDLE_PI_RESERVED;
  Smbios          = GetSmbiosProtocol();
  ASSERT (Smbios != NULL);
  
  do {
    Status = Smbios->GetNext (Smbios, &SearchingHandle, &Type, &RecordInSmbiosDatabase, NULL);
  } while ((SearchingHandle != Handle) && (Status != EFI_NOT_FOUND));
  
  return RecordInSmbiosDatabase;
}  

/**

  Get the full size of smbios structure including optional strings that follow the formatted structure.

  @param Head                   Pointer to the beginning of smbios structure.
  @param Size                   The returned size.
  @param NumberOfStrings        The returned number of optional strings that follow the formatted structure.

  @retval EFI_SUCCESS           Size retured in Size.
  @retval EFI_INVALID_PARAMETER Input smbios structure mal-formed or Size is NULL.
  
**/
EFI_STATUS
EFIAPI
GetSmbiosStructureSize (
  IN   EFI_SMBIOS_TABLE_HEADER          *Head,
  OUT  UINT32                           *Size,
  OUT  UINT8                            *NumberOfStrings
  )
{
  UINT32 FullSize;
  UINT8  StrLen;
  INT8*  CharInStr;
  
  if (Size == NULL || NumberOfStrings == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FullSize = Head->Length;
  CharInStr = (INT8*)Head + Head->Length;
  *Size = FullSize;
  *NumberOfStrings = 0;
  StrLen = 0;
  //
  // look for the two consecutive zeros, check the string limit by the way.
  //
  while (*CharInStr != 0 || *(CharInStr+1) != 0) { 
    if (*CharInStr == 0) {
      *Size += 1;
      CharInStr++;
    }

    for (StrLen = 0 ; StrLen < SMBIOS_STRING_MAX_LENGTH; StrLen++) {
      if (*(CharInStr+StrLen) == 0) {
        break;
      }  
    }

    if (StrLen == SMBIOS_STRING_MAX_LENGTH) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // forward the pointer
    //
    CharInStr += StrLen;
    *Size += StrLen;
    *NumberOfStrings += 1;
    
  }

  //
  // count ending two zeros.
  //
  *Size += 2;
  return EFI_SUCCESS;  
}
