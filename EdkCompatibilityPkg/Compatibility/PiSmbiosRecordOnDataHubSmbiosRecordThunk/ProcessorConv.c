/** @file
  Routines that support Processor SubClass data records translation.
  
Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Thunk.h"

/**
  Field Filling Function for Processor SubClass record type 17 -- Cache association.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldProcessorType17 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_SUBCLASS_TYPE1_HEADER *DataHeader;
  EFI_INTER_LINK_DATA       *LinkData;
  UINT16                    FieldOffset;
  UINT8                     *Pointer;

  DataHeader  = (EFI_SUBCLASS_TYPE1_HEADER *) RecordData;
  LinkData    = (EFI_INTER_LINK_DATA *) (DataHeader + 1);
  if (RecordDataSize != sizeof (EFI_INTER_LINK_DATA) + sizeof (EFI_SUBCLASS_TYPE1_HEADER)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Determine the cache level
  //
  Pointer = (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE4, L1CacheHandle);
  if ((*Pointer == 0) && (*(Pointer + 1) == 0)) {
    SetMem ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE4, L1CacheHandle), 2, 0xFF);
  }

  Pointer = (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE4, L2CacheHandle);
  if ((*Pointer == 0) && (*(Pointer + 1) == 0)) {
    SetMem ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE4, L2CacheHandle), 2, 0xFF);
  }

  Pointer = (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE4, L3CacheHandle);
  if ((*Pointer == 0) && (*(Pointer + 1) == 0)) {
    SetMem ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE4, L3CacheHandle), 2, 0xFF);
  }

  if (LinkData->SubInstance == 1) {
    FieldOffset = (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE4, L1CacheHandle);
  } else if (LinkData->SubInstance == 2) {
    FieldOffset = (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE4, L2CacheHandle);
  } else if (LinkData->SubInstance == 3) {
    FieldOffset = (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE4, L3CacheHandle);
  } else {
    return EFI_INVALID_PARAMETER;
  }

  return SmbiosFldInterLink (
          StructureNode,
          FieldOffset,
          7,                  // Smbios type 7 -- Cache Information
          LinkData,
          &gEfiCacheSubClassGuid // gProcessorSubClassName
          );
}

/**
  Field Filling Function for Processor SubClass record type 6 -- ProcessorID.
  Offset is mandatory.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldProcessorType6 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_PROCESSOR_ID_DATA *ProcessorIdData;

  ProcessorIdData = RecordData;
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + Offset,
    &(ProcessorIdData->Signature),
    4
    );

  CopyMem (
    (UINT8 *) (StructureNode->Structure) + Offset + 4,
    &(ProcessorIdData->FeatureFlags),
    4
    );

  return EFI_SUCCESS;
}

/**
  Field Filling Function for Processor SubClass record type 9 -- Voltage.
  Offset is mandatory.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldProcessorType9 (
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

  Exponent += 1;
  while (Exponent != 0) {
    if (Exponent > 0) {
      Value = (INT16) (Value * 10);
      Exponent--;
    } else {
      Value = (INT16) (Value / 10);
      Exponent++;
    }
  }

  * (UINT8 *) ((UINT8 *) (StructureNode->Structure) + Offset) = (UINT8) (Value | BIT7);

  return EFI_SUCCESS;
}
