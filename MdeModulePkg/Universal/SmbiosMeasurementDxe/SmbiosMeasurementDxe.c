/** @file
  This driver measures SMBIOS table to TPM.

Copyright (c) 2015 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/Smbios.h>
#include <IndustryStandard/SmBios.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Guid/EventGroup.h>
#include <Guid/SmBios.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/TpmMeasurementLib.h>

#define FIELD_SIZE_OF(TYPE, Field)  ((UINTN)sizeof(((TYPE *)0)->Field))

typedef struct {
  UINT8     Type;
  UINTN     Offset;
  UINTN     Size;
  UINT32    Flags;
} SMBIOS_FILTER_TABLE;
#define SMBIOS_FILTER_TABLE_FLAG_IS_STRING  BIT0

typedef struct {
  UINT8                  Type;
  SMBIOS_FILTER_TABLE    *Filter; // NULL means all fields
  UINTN                  FilterCount;
} SMBIOS_FILTER_STRUCT;

//
// Platform Specific Policy
//
SMBIOS_FILTER_TABLE  mSmbiosFilterType1BlackList[] = {
  { 0x01, OFFSET_OF (SMBIOS_TABLE_TYPE1, SerialNumber), FIELD_SIZE_OF (SMBIOS_TABLE_TYPE1, SerialNumber), SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
  { 0x01, OFFSET_OF (SMBIOS_TABLE_TYPE1, Uuid),         FIELD_SIZE_OF (SMBIOS_TABLE_TYPE1, Uuid),         0                                  },
  { 0x01, OFFSET_OF (SMBIOS_TABLE_TYPE1, WakeUpType),   FIELD_SIZE_OF (SMBIOS_TABLE_TYPE1, WakeUpType),   0                                  },
};
SMBIOS_FILTER_TABLE  mSmbiosFilterType2BlackList[] = {
  { 0x02, OFFSET_OF (SMBIOS_TABLE_TYPE2, SerialNumber),      FIELD_SIZE_OF (SMBIOS_TABLE_TYPE2, SerialNumber),      SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
  { 0x02, OFFSET_OF (SMBIOS_TABLE_TYPE2, LocationInChassis), FIELD_SIZE_OF (SMBIOS_TABLE_TYPE2, LocationInChassis), SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
};
SMBIOS_FILTER_TABLE  mSmbiosFilterType3BlackList[] = {
  { 0x03, OFFSET_OF (SMBIOS_TABLE_TYPE3, SerialNumber), FIELD_SIZE_OF (SMBIOS_TABLE_TYPE3, SerialNumber), SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
  { 0x03, OFFSET_OF (SMBIOS_TABLE_TYPE3, AssetTag),     FIELD_SIZE_OF (SMBIOS_TABLE_TYPE3, AssetTag),     SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
};
SMBIOS_FILTER_TABLE  mSmbiosFilterType4BlackList[] = {
  { 0x04, OFFSET_OF (SMBIOS_TABLE_TYPE4, SerialNumber),      FIELD_SIZE_OF (SMBIOS_TABLE_TYPE4, SerialNumber),      SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
  { 0x04, OFFSET_OF (SMBIOS_TABLE_TYPE4, AssetTag),          FIELD_SIZE_OF (SMBIOS_TABLE_TYPE4, AssetTag),          SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
  { 0x04, OFFSET_OF (SMBIOS_TABLE_TYPE4, PartNumber),        FIELD_SIZE_OF (SMBIOS_TABLE_TYPE4, PartNumber),        SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
  { 0x04, OFFSET_OF (SMBIOS_TABLE_TYPE4, CoreCount),         FIELD_SIZE_OF (SMBIOS_TABLE_TYPE4, CoreCount),         0                                  },
  { 0x04, OFFSET_OF (SMBIOS_TABLE_TYPE4, EnabledCoreCount),  FIELD_SIZE_OF (SMBIOS_TABLE_TYPE4, EnabledCoreCount),  0                                  },
  { 0x04, OFFSET_OF (SMBIOS_TABLE_TYPE4, ThreadCount),       FIELD_SIZE_OF (SMBIOS_TABLE_TYPE4, ThreadCount),       0                                  },
  { 0x04, OFFSET_OF (SMBIOS_TABLE_TYPE4, CoreCount2),        FIELD_SIZE_OF (SMBIOS_TABLE_TYPE4, CoreCount2),        0                                  },
  { 0x04, OFFSET_OF (SMBIOS_TABLE_TYPE4, EnabledCoreCount2), FIELD_SIZE_OF (SMBIOS_TABLE_TYPE4, EnabledCoreCount2), 0                                  },
  { 0x04, OFFSET_OF (SMBIOS_TABLE_TYPE4, ThreadCount2),      FIELD_SIZE_OF (SMBIOS_TABLE_TYPE4, ThreadCount2),      0                                  },
  { 0x04, OFFSET_OF (SMBIOS_TABLE_TYPE4, Voltage),           FIELD_SIZE_OF (SMBIOS_TABLE_TYPE4, Voltage),           0                                  },
};
SMBIOS_FILTER_TABLE  mSmbiosFilterType17BlackList[] = {
  { 0x11, OFFSET_OF (SMBIOS_TABLE_TYPE17, SerialNumber), FIELD_SIZE_OF (SMBIOS_TABLE_TYPE17, SerialNumber), SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
  { 0x11, OFFSET_OF (SMBIOS_TABLE_TYPE17, AssetTag),     FIELD_SIZE_OF (SMBIOS_TABLE_TYPE17, AssetTag),     SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
  { 0x11, OFFSET_OF (SMBIOS_TABLE_TYPE17, PartNumber),   FIELD_SIZE_OF (SMBIOS_TABLE_TYPE17, PartNumber),   SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
};
SMBIOS_FILTER_TABLE  mSmbiosFilterType22BlackList[] = {
  { 0x16, OFFSET_OF (SMBIOS_TABLE_TYPE22, SerialNumber),        FIELD_SIZE_OF (SMBIOS_TABLE_TYPE22, SerialNumber),        SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
  { 0x16, OFFSET_OF (SMBIOS_TABLE_TYPE22, SBDSSerialNumber),    FIELD_SIZE_OF (SMBIOS_TABLE_TYPE22, SBDSSerialNumber),    0                                  },
  { 0x16, OFFSET_OF (SMBIOS_TABLE_TYPE22, SBDSManufactureDate), FIELD_SIZE_OF (SMBIOS_TABLE_TYPE22, SBDSManufactureDate), 0                                  },
};
SMBIOS_FILTER_TABLE  mSmbiosFilterType23BlackList[] = {
  { 0x17, OFFSET_OF (SMBIOS_TABLE_TYPE23, ResetCount), FIELD_SIZE_OF (SMBIOS_TABLE_TYPE23, ResetCount), 0 },
};
SMBIOS_FILTER_TABLE  mSmbiosFilterType27BlackList[] = {
  { 0x1B, OFFSET_OF (SMBIOS_TABLE_TYPE27, NominalSpeed), FIELD_SIZE_OF (SMBIOS_TABLE_TYPE27, NominalSpeed), 0 },
};
SMBIOS_FILTER_TABLE  mSmbiosFilterType39BlackList[] = {
  { 0x27, OFFSET_OF (SMBIOS_TABLE_TYPE39, SerialNumber),    FIELD_SIZE_OF (SMBIOS_TABLE_TYPE39, SerialNumber),    SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
  { 0x27, OFFSET_OF (SMBIOS_TABLE_TYPE39, AssetTagNumber),  FIELD_SIZE_OF (SMBIOS_TABLE_TYPE39, AssetTagNumber),  SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
  { 0x27, OFFSET_OF (SMBIOS_TABLE_TYPE39, ModelPartNumber), FIELD_SIZE_OF (SMBIOS_TABLE_TYPE39, ModelPartNumber), SMBIOS_FILTER_TABLE_FLAG_IS_STRING },
};

SMBIOS_FILTER_STRUCT  mSmbiosFilterStandardTableBlackList[] = {
  { 0x01, mSmbiosFilterType1BlackList,  sizeof (mSmbiosFilterType1BlackList)/sizeof (mSmbiosFilterType1BlackList[0])   },
  { 0x02, mSmbiosFilterType2BlackList,  sizeof (mSmbiosFilterType2BlackList)/sizeof (mSmbiosFilterType2BlackList[0])   },
  { 0x03, mSmbiosFilterType3BlackList,  sizeof (mSmbiosFilterType3BlackList)/sizeof (mSmbiosFilterType3BlackList[0])   },
  { 0x04, mSmbiosFilterType4BlackList,  sizeof (mSmbiosFilterType4BlackList)/sizeof (mSmbiosFilterType4BlackList[0])   },
  { 0x0B, NULL,                         0                                                                              },
  { 0x0F, NULL,                         0                                                                              },
  { 0x11, mSmbiosFilterType17BlackList, sizeof (mSmbiosFilterType17BlackList)/sizeof (mSmbiosFilterType17BlackList[0]) },
  { 0x12, NULL,                         0                                                                              },
  { 0x16, mSmbiosFilterType22BlackList, sizeof (mSmbiosFilterType22BlackList)/sizeof (mSmbiosFilterType22BlackList[0]) },
  { 0x17, mSmbiosFilterType23BlackList, sizeof (mSmbiosFilterType23BlackList)/sizeof (mSmbiosFilterType23BlackList[0]) },
  { 0x1B, mSmbiosFilterType27BlackList, sizeof (mSmbiosFilterType27BlackList)/sizeof (mSmbiosFilterType27BlackList[0]) },
  { 0x1F, NULL,                         0                                                                              },
  { 0x21, NULL,                         0                                                                              },
  { 0x27, mSmbiosFilterType39BlackList, sizeof (mSmbiosFilterType39BlackList)/sizeof (mSmbiosFilterType39BlackList[0]) },
};

EFI_SMBIOS_PROTOCOL  *mSmbios;
UINTN                mMaxLen;

#pragma pack (1)

#define SMBIOS_HANDOFF_TABLE_DESC  "SmbiosTable"
typedef struct {
  UINT8                      TableDescriptionSize;
  UINT8                      TableDescription[sizeof (SMBIOS_HANDOFF_TABLE_DESC)];
  UINT64                     NumberOfTables;
  EFI_CONFIGURATION_TABLE    TableEntry[1];
} SMBIOS_HANDOFF_TABLE_POINTERS2;

#pragma pack ()

/**

  This function dump raw data.

  @param  Data  raw data
  @param  Size  raw data size

**/
VOID
InternalDumpData (
  IN UINT8  *Data,
  IN UINTN  Size
  )
{
  UINTN  Index;

  for (Index = 0; Index < Size; Index++) {
    DEBUG ((DEBUG_VERBOSE, "%02x", (UINTN)Data[Index]));
  }
}

/**

  This function dump raw data with colume format.

  @param  Data  raw data
  @param  Size  raw data size

**/
VOID
InternalDumpHex (
  IN UINT8  *Data,
  IN UINTN  Size
  )
{
  UINTN  Index;
  UINTN  Count;
  UINTN  Left;

  #define COLUME_SIZE  (16 * 2)

  Count = Size / COLUME_SIZE;
  Left  = Size % COLUME_SIZE;
  for (Index = 0; Index < Count; Index++) {
    DEBUG ((DEBUG_VERBOSE, "%04x: ", Index * COLUME_SIZE));
    InternalDumpData (Data + Index * COLUME_SIZE, COLUME_SIZE);
    DEBUG ((DEBUG_VERBOSE, "\n"));
  }

  if (Left != 0) {
    DEBUG ((DEBUG_VERBOSE, "%04x: ", Index * COLUME_SIZE));
    InternalDumpData (Data + Index * COLUME_SIZE, Left);
    DEBUG ((DEBUG_VERBOSE, "\n"));
  }
}

/**

  This function get filter structure by SMBIOS type.

  @param  Type  SMBIOS type

**/
SMBIOS_FILTER_STRUCT *
GetFilterStructByType (
  IN UINT8  Type
  )
{
  UINTN  Index;

  for (Index = 0; Index < sizeof (mSmbiosFilterStandardTableBlackList)/sizeof (mSmbiosFilterStandardTableBlackList[0]); Index++) {
    if (mSmbiosFilterStandardTableBlackList[Index].Type == Type) {
      return &mSmbiosFilterStandardTableBlackList[Index];
    }
  }

  return NULL;
}

/**

  This function get SMBIOS string in SMBIOS table.

  @param  Head      SMBIOS table head
  @param  StringId  SMBIOS string ID
  @param  StringLen length of SMBIOS string

  @return SMBIOS string data
**/
CHAR8 *
GetSmbiosStringById (
  IN   EFI_SMBIOS_TABLE_HEADER  *Head,
  IN   SMBIOS_TABLE_STRING      StringId,
  OUT  UINTN                    *StringLen
  )
{
  UINTN  Size;
  UINTN  StrLen;
  CHAR8  *CharInStr;
  UINTN  StringsNumber;
  CHAR8  *String;

  CharInStr     = (CHAR8 *)Head + Head->Length;
  Size          = Head->Length;
  StringsNumber = 0;
  StrLen        = 0;
  //
  // look for the two consecutive zeros, check the string limit by the way.
  //
  String = NULL;
  while (*CharInStr != 0 || *(CharInStr+1) != 0) {
    if (*CharInStr == 0) {
      Size += 1;
      CharInStr++;
    }

    String = CharInStr;

    for (StrLen = 0; StrLen < mMaxLen; StrLen++) {
      if (*(CharInStr+StrLen) == 0) {
        break;
      }
    }

    *StringLen = StrLen;

    if (StrLen == mMaxLen) {
      return NULL;
    }

    //
    // forward the pointer
    //
    CharInStr     += StrLen;
    Size          += StrLen;
    StringsNumber += 1;
    if (StringsNumber == StringId) {
      break;
    }
  }

  return String;
}

/**

  This function update SMBIOS table based on policy.

  @param  TableEntry      SMBIOS table
  @param  TableEntrySize  SMBIOS table size

**/
VOID
FilterSmbiosEntry (
  IN OUT VOID  *TableEntry,
  IN UINTN     TableEntrySize
  )
{
  SMBIOS_FILTER_STRUCT  *FilterStruct;
  SMBIOS_FILTER_TABLE   *Filter;
  UINTN                 Index;
  SMBIOS_TABLE_STRING   StringId;
  CHAR8                 *String;
  UINTN                 StringLen;

  DEBUG ((DEBUG_INFO, "Smbios Table (Type - %d):\n", ((SMBIOS_STRUCTURE *)TableEntry)->Type));
  DEBUG_CODE (
    InternalDumpHex (TableEntry, TableEntrySize);
    );

  //
  // Skip measurement for OEM types.
  //
  if (((SMBIOS_STRUCTURE *)TableEntry)->Type >= SMBIOS_OEM_BEGIN) {
    // zero all table fields, except header
    ZeroMem ((UINT8 *)TableEntry + sizeof (SMBIOS_STRUCTURE), TableEntrySize - sizeof (SMBIOS_STRUCTURE));
  } else {
    FilterStruct = GetFilterStructByType (((SMBIOS_STRUCTURE *)TableEntry)->Type);
    if (FilterStruct != NULL) {
      if ((FilterStruct->Filter == NULL) || (FilterStruct->FilterCount == 0)) {
        // zero all table fields, except header
        ZeroMem ((UINT8 *)TableEntry + sizeof (SMBIOS_STRUCTURE), TableEntrySize - sizeof (SMBIOS_STRUCTURE));
      } else {
        Filter = FilterStruct->Filter;
        for (Index = 0; Index < FilterStruct->FilterCount; Index++) {
          if (((SMBIOS_STRUCTURE *)TableEntry)->Length >= (Filter[Index].Offset + Filter[Index].Size)) {
            //
            // The field is present in the SMBIOS entry.
            //
            if ((Filter[Index].Flags & SMBIOS_FILTER_TABLE_FLAG_IS_STRING) != 0) {
              CopyMem (&StringId, (UINT8 *)TableEntry + Filter[Index].Offset, sizeof (StringId));
              if (StringId != 0) {
                // set ' ' for string field
                String = GetSmbiosStringById (TableEntry, StringId, &StringLen);
                ASSERT (String != NULL);
                // DEBUG ((DEBUG_INFO,"StrId(0x%x)-%a(%d)\n", StringId, String, StringLen));
                SetMem (String, StringLen, ' ');
              }
            }

            // zero non-string field
            ZeroMem ((UINT8 *)TableEntry + Filter[Index].Offset, Filter[Index].Size);
          }
        }
      }
    }
  }

  DEBUG ((DEBUG_INFO, "Filter Smbios Table (Type - %d):\n", ((SMBIOS_STRUCTURE *)TableEntry)->Type));
  DEBUG_CODE (
    InternalDumpHex (TableEntry, TableEntrySize);
    );
}

/**

  Get the full size of SMBIOS structure including optional strings that follow the formatted structure.

  @param Head                   Pointer to the beginning of SMBIOS structure.
  @param NumberOfStrings        The returned number of optional strings that follow the formatted structure.

  @return Size                  The returned size.
**/
UINTN
GetSmbiosStructureSize (
  IN   EFI_SMBIOS_TABLE_HEADER  *Head,
  OUT  UINTN                    *NumberOfStrings
  )
{
  UINTN  Size;
  UINTN  StrLen;
  CHAR8  *CharInStr;
  UINTN  StringsNumber;

  CharInStr     = (CHAR8 *)Head + Head->Length;
  Size          = Head->Length;
  StringsNumber = 0;
  StrLen        = 0;
  //
  // look for the two consecutive zeros, check the string limit by the way.
  //
  while (*CharInStr != 0 || *(CharInStr+1) != 0) {
    if (*CharInStr == 0) {
      Size += 1;
      CharInStr++;
    }

    for (StrLen = 0; StrLen < mMaxLen; StrLen++) {
      if (*(CharInStr+StrLen) == 0) {
        break;
      }
    }

    if (StrLen == mMaxLen) {
      return 0;
    }

    //
    // forward the pointer
    //
    CharInStr     += StrLen;
    Size          += StrLen;
    StringsNumber += 1;
  }

  //
  // count ending two zeros.
  //
  Size += 2;

  if (NumberOfStrings != NULL) {
    *NumberOfStrings = StringsNumber;
  }

  return Size;
}

/**

  This function returns full SMBIOS table length.

  @param  TableAddress      SMBIOS table based address
  @param  TableMaximumSize  Maximum size of SMBIOS table

  @return SMBIOS table length

**/
UINTN
GetSmbiosTableLength (
  IN VOID   *TableAddress,
  IN UINTN  TableMaximumSize
  )
{
  VOID   *TableEntry;
  VOID   *TableAddressEnd;
  UINTN  TableEntryLength;

  TableAddressEnd = (VOID *)((UINTN)TableAddress + TableMaximumSize);
  TableEntry      = TableAddress;
  while (TableEntry < TableAddressEnd) {
    TableEntryLength = GetSmbiosStructureSize (TableEntry, NULL);
    if (TableEntryLength == 0) {
      break;
    }

    if (((SMBIOS_STRUCTURE *)TableEntry)->Type == 127) {
      TableEntry = (VOID *)((UINTN)TableEntry + TableEntryLength);
      break;
    }

    TableEntry = (VOID *)((UINTN)TableEntry + TableEntryLength);
  }

  return ((UINTN)TableEntry - (UINTN)TableAddress);
}

/**

  This function updatess full SMBIOS table length.

  @param  TableAddress      SMBIOS table based address
  @param  TableLength       SMBIOS table length

**/
VOID
FilterSmbiosTable (
  IN OUT VOID  *TableAddress,
  IN UINTN     TableLength
  )
{
  VOID   *TableAddressEnd;
  VOID   *TableEntry;
  UINTN  TableEntryLength;

  TableEntry      = TableAddress;
  TableAddressEnd = (VOID *)((UINTN)TableAddress + TableLength);
  while ((UINTN)TableEntry < (UINTN)TableAddressEnd) {
    TableEntryLength = GetSmbiosStructureSize (TableEntry, NULL);
    if (TableEntryLength == 0) {
      break;
    }

    FilterSmbiosEntry (TableEntry, TableEntryLength);

    TableEntry = (VOID *)((UINTN)TableEntry + TableEntryLength);
  }
}

/**
  Measure SMBIOS with EV_EFI_HANDOFF_TABLES to PCR[1].

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    Pointer to the notification function's context.

**/
VOID
EFIAPI
MeasureSmbiosTable (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  EFI_STATUS                      Status;
  EFI_HANDOFF_TABLE_POINTERS      HandoffTables;
  SMBIOS_HANDOFF_TABLE_POINTERS2  SmbiosHandoffTables2;
  UINT32                          EventType;
  VOID                            *EventLog;
  UINT32                          EventLogSize;
  SMBIOS_TABLE_ENTRY_POINT        *SmbiosTable;
  SMBIOS_TABLE_3_0_ENTRY_POINT    *Smbios3Table;
  VOID                            *SmbiosTableAddress;
  VOID                            *TableAddress;
  UINTN                           TableLength;

  SmbiosTable        = NULL;
  Smbios3Table       = NULL;
  SmbiosTableAddress = NULL;
  TableLength        = 0;

  if (mSmbios->MajorVersion >= 3) {
    Status = EfiGetSystemConfigurationTable (
               &gEfiSmbios3TableGuid,
               (VOID **)&Smbios3Table
               );
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Smbios3Table:\n"));
      DEBUG ((
        DEBUG_INFO,
        "  AnchorString                - '%c%c%c%c%c'\n",
        Smbios3Table->AnchorString[0],
        Smbios3Table->AnchorString[1],
        Smbios3Table->AnchorString[2],
        Smbios3Table->AnchorString[3],
        Smbios3Table->AnchorString[4]
        ));
      DEBUG ((DEBUG_INFO, "  EntryPointStructureChecksum - 0x%02x\n", Smbios3Table->EntryPointStructureChecksum));
      DEBUG ((DEBUG_INFO, "  EntryPointLength            - 0x%02x\n", Smbios3Table->EntryPointLength));
      DEBUG ((DEBUG_INFO, "  MajorVersion                - 0x%02x\n", Smbios3Table->MajorVersion));
      DEBUG ((DEBUG_INFO, "  MinorVersion                - 0x%02x\n", Smbios3Table->MinorVersion));
      DEBUG ((DEBUG_INFO, "  DocRev                      - 0x%02x\n", Smbios3Table->DocRev));
      DEBUG ((DEBUG_INFO, "  EntryPointRevision          - 0x%02x\n", Smbios3Table->EntryPointRevision));
      DEBUG ((DEBUG_INFO, "  TableMaximumSize            - 0x%08x\n", Smbios3Table->TableMaximumSize));
      DEBUG ((DEBUG_INFO, "  TableAddress                - 0x%016lx\n", Smbios3Table->TableAddress));
    }
  }

  if (Smbios3Table == NULL) {
    Status = EfiGetSystemConfigurationTable (
               &gEfiSmbiosTableGuid,
               (VOID **)&SmbiosTable
               );
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "SmbiosTable:\n"));
      DEBUG ((
        DEBUG_INFO,
        "  AnchorString                - '%c%c%c%c'\n",
        SmbiosTable->AnchorString[0],
        SmbiosTable->AnchorString[1],
        SmbiosTable->AnchorString[2],
        SmbiosTable->AnchorString[3]
        ));
      DEBUG ((DEBUG_INFO, "  EntryPointStructureChecksum - 0x%02x\n", SmbiosTable->EntryPointStructureChecksum));
      DEBUG ((DEBUG_INFO, "  EntryPointLength            - 0x%02x\n", SmbiosTable->EntryPointLength));
      DEBUG ((DEBUG_INFO, "  MajorVersion                - 0x%02x\n", SmbiosTable->MajorVersion));
      DEBUG ((DEBUG_INFO, "  MinorVersion                - 0x%02x\n", SmbiosTable->MinorVersion));
      DEBUG ((DEBUG_INFO, "  MaxStructureSize            - 0x%08x\n", SmbiosTable->MaxStructureSize));
      DEBUG ((DEBUG_INFO, "  EntryPointRevision          - 0x%02x\n", SmbiosTable->EntryPointRevision));
      DEBUG ((
        DEBUG_INFO,
        "  FormattedArea               - '%c%c%c%c%c'\n",
        SmbiosTable->FormattedArea[0],
        SmbiosTable->FormattedArea[1],
        SmbiosTable->FormattedArea[2],
        SmbiosTable->FormattedArea[3],
        SmbiosTable->FormattedArea[4]
        ));
      DEBUG ((
        DEBUG_INFO,
        "  IntermediateAnchorString    - '%c%c%c%c%c'\n",
        SmbiosTable->IntermediateAnchorString[0],
        SmbiosTable->IntermediateAnchorString[1],
        SmbiosTable->IntermediateAnchorString[2],
        SmbiosTable->IntermediateAnchorString[3],
        SmbiosTable->IntermediateAnchorString[4]
        ));
      DEBUG ((DEBUG_INFO, "  IntermediateChecksum        - 0x%02x\n", SmbiosTable->IntermediateChecksum));
      DEBUG ((DEBUG_INFO, "  TableLength                 - 0x%04x\n", SmbiosTable->TableLength));
      DEBUG ((DEBUG_INFO, "  TableAddress                - 0x%08x\n", SmbiosTable->TableAddress));
      DEBUG ((DEBUG_INFO, "  NumberOfSmbiosStructures    - 0x%04x\n", SmbiosTable->NumberOfSmbiosStructures));
      DEBUG ((DEBUG_INFO, "  SmbiosBcdRevision           - 0x%02x\n", SmbiosTable->SmbiosBcdRevision));
    }
  }

  if (Smbios3Table != NULL) {
    SmbiosTableAddress = (VOID *)(UINTN)Smbios3Table->TableAddress;
    TableLength        = GetSmbiosTableLength (SmbiosTableAddress, Smbios3Table->TableMaximumSize);
  } else if (SmbiosTable != NULL) {
    SmbiosTableAddress = (VOID *)(UINTN)SmbiosTable->TableAddress;
    TableLength        = SmbiosTable->TableLength;
  }

  if (SmbiosTableAddress != NULL) {
    DEBUG ((DEBUG_INFO, "The Smbios Table starts at: 0x%x\n", SmbiosTableAddress));
    DEBUG ((DEBUG_INFO, "The Smbios Table size: 0x%x\n", TableLength));
    DEBUG_CODE (
      InternalDumpHex ((UINT8 *)(UINTN)SmbiosTableAddress, TableLength);
      );

    TableAddress = AllocateCopyPool ((UINTN)TableLength, (VOID *)(UINTN)SmbiosTableAddress);
    if (TableAddress == NULL) {
      return;
    }

    FilterSmbiosTable (TableAddress, TableLength);

    DEBUG ((DEBUG_INFO, "The final Smbios Table starts at: 0x%x\n", TableAddress));
    DEBUG ((DEBUG_INFO, "The final Smbios Table size: 0x%x\n", TableLength));
    DEBUG_CODE (
      InternalDumpHex (TableAddress, TableLength);
      );

    HandoffTables.NumberOfTables = 1;
    if (Smbios3Table != NULL) {
      CopyGuid (&(HandoffTables.TableEntry[0].VendorGuid), &gEfiSmbios3TableGuid);
      HandoffTables.TableEntry[0].VendorTable = Smbios3Table;
    } else {
      CopyGuid (&(HandoffTables.TableEntry[0].VendorGuid), &gEfiSmbiosTableGuid);
      HandoffTables.TableEntry[0].VendorTable = SmbiosTable;
    }

    EventType    = EV_EFI_HANDOFF_TABLES;
    EventLog     = &HandoffTables;
    EventLogSize = sizeof (HandoffTables);

    if (PcdGet32 (PcdTcgPfpMeasurementRevision) >= TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2_REV_105) {
      SmbiosHandoffTables2.TableDescriptionSize = sizeof (SmbiosHandoffTables2.TableDescription);
      CopyMem (SmbiosHandoffTables2.TableDescription, SMBIOS_HANDOFF_TABLE_DESC, sizeof (SmbiosHandoffTables2.TableDescription));
      SmbiosHandoffTables2.NumberOfTables = HandoffTables.NumberOfTables;
      CopyMem (&(SmbiosHandoffTables2.TableEntry[0]), &(HandoffTables.TableEntry[0]), sizeof (SmbiosHandoffTables2.TableEntry[0]));
      EventType    = EV_EFI_HANDOFF_TABLES2;
      EventLog     = &SmbiosHandoffTables2;
      EventLogSize = sizeof (SmbiosHandoffTables2);
    }

    Status = TpmMeasureAndLogData (
               1,                       // PCRIndex
               EventType,               // EventType
               EventLog,                // EventLog
               EventLogSize,            // LogLen
               TableAddress,            // HashData
               TableLength              // HashDataLen
               );
    if (!EFI_ERROR (Status)) {
      gBS->CloseEvent (Event);
    }
  }

  return;
}

/**

  Driver to produce Smbios measurement.

  @param ImageHandle     Module's image handle
  @param SystemTable     Pointer of EFI_SYSTEM_TABLE

  @return Status returned from EfiCreateEventReadyToBootEx().

**/
EFI_STATUS
EFIAPI
SmbiosMeasurementDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;

  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&mSmbios);
  ASSERT_EFI_ERROR (Status);
  DEBUG ((DEBUG_INFO, "The Smbios Table Version: %x.%x\n", mSmbios->MajorVersion, mSmbios->MinorVersion));

  if ((mSmbios->MajorVersion < 2) || ((mSmbios->MajorVersion == 2) && (mSmbios->MinorVersion < 7))) {
    mMaxLen = SMBIOS_STRING_MAX_LENGTH;
  } else if (mSmbios->MajorVersion < 3) {
    //
    // Reference SMBIOS 2.7, chapter 6.1.3, it will have no limit on the length of each individual text string.
    // However, the length of the entire structure table (including all strings) must be reported
    // in the Structure Table Length field of the SMBIOS Structure Table Entry Point,
    // which is a WORD field limited to 65,535 bytes.
    //
    mMaxLen = SMBIOS_TABLE_MAX_LENGTH;
  } else {
    //
    // SMBIOS 3.0 defines the Structure table maximum size as DWORD field limited to 0xFFFFFFFF bytes.
    // Locate the end of string as long as possible.
    //
    mMaxLen = SMBIOS_3_0_TABLE_MAX_LENGTH;
  }

  //
  // Measure Smbios tables
  //
  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             MeasureSmbiosTable,
             NULL,
             &Event
             );

  return Status;
}
