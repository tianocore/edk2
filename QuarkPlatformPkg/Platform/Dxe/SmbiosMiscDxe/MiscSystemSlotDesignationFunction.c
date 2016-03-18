/** @file
BIOS system slot designator information boot time changes.
SMBIOS type 9.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


MiscSystemSlotDesignatorFunction.c

**/


#include "CommonHeader.h"

#include "SmbiosMisc.h"

//
//
//

MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot1);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot2);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot3);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot4);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot5);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot6);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot7);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot8);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot9);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot10);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot11);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot12);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot13);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot14);

EFI_MISC_SYSTEM_SLOT_DESIGNATION  *mMiscSlotArray[SMBIOS_SYSTEM_SLOT_MAX_NUM] =
{
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot1),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot2),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot3),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot4),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot5),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot6),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot7),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot8),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot9),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot10),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot11),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot12),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot13),
  MISC_SMBIOS_DATA_TABLE_POINTER (MiscSystemSlot14),
};

BOOLEAN  PcdMiscSlotIsInit = FALSE;
SMBIOS_SLOT_COFNIG  SMBIOSlotConfig = {0};

/**
  Get Misc Slot Configuration information from PCD
  @param  SMBIOSPortConnector                 Pointer to SMBIOSPortConnector table.

**/

VOID
GetMiscSLotConfigFromPcd ()
{
  //
  // Type 9
  //
  SMBIOSlotConfig.SMBIOSSystemSlotNumber = PcdGet8 (PcdSMBIOSSystemSlotNumber);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot1Designation), SMBIOSlotConfig.SMBIOSSystemSlot[0].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[0].SlotType = PcdGet8(PcdSMBIOSSystemSlot1Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[0].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot1DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[0].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot1Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[0].SlotLength = PcdGet8(PcdSMBIOSSystemSlot1Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[0].SlotId = PcdGet16(PcdSMBIOSSystemSlot1Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[0].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot1Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot2Designation), SMBIOSlotConfig.SMBIOSSystemSlot[1].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[1].SlotType = PcdGet8(PcdSMBIOSSystemSlot2Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[1].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot2DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[1].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot2Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[1].SlotLength = PcdGet8(PcdSMBIOSSystemSlot2Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[1].SlotId = PcdGet16(PcdSMBIOSSystemSlot2Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[1].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot2Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot3Designation), SMBIOSlotConfig.SMBIOSSystemSlot[2].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[2].SlotType = PcdGet8(PcdSMBIOSSystemSlot3Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[2].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot3DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[2].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot3Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[2].SlotLength = PcdGet8(PcdSMBIOSSystemSlot3Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[2].SlotId = PcdGet16(PcdSMBIOSSystemSlot3Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[2].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot3Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot4Designation), SMBIOSlotConfig.SMBIOSSystemSlot[3].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[3].SlotType = PcdGet8(PcdSMBIOSSystemSlot4Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[3].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot4DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[3].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot4Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[3].SlotLength = PcdGet8(PcdSMBIOSSystemSlot4Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[3].SlotId = PcdGet16(PcdSMBIOSSystemSlot4Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[3].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot4Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot5Designation), SMBIOSlotConfig.SMBIOSSystemSlot[4].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[4].SlotType = PcdGet8(PcdSMBIOSSystemSlot5Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[4].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot5DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[4].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot5Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[4].SlotLength = PcdGet8(PcdSMBIOSSystemSlot5Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[4].SlotId = PcdGet16(PcdSMBIOSSystemSlot5Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[4].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot5Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot6Designation), SMBIOSlotConfig.SMBIOSSystemSlot[5].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[5].SlotType = PcdGet8(PcdSMBIOSSystemSlot6Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[5].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot6DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[5].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot6Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[5].SlotLength = PcdGet8(PcdSMBIOSSystemSlot6Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[5].SlotId = PcdGet16(PcdSMBIOSSystemSlot6Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[5].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot6Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot7Designation), SMBIOSlotConfig.SMBIOSSystemSlot[6].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[6].SlotType = PcdGet8(PcdSMBIOSSystemSlot7Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[6].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot7DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[6].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot7Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[6].SlotLength = PcdGet8(PcdSMBIOSSystemSlot7Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[6].SlotId = PcdGet16(PcdSMBIOSSystemSlot7Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[6].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot7Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot8Designation), SMBIOSlotConfig.SMBIOSSystemSlot[7].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[7].SlotType = PcdGet8(PcdSMBIOSSystemSlot8Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[7].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot8DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[7].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot8Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[7].SlotLength = PcdGet8(PcdSMBIOSSystemSlot8Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[7].SlotId = PcdGet16(PcdSMBIOSSystemSlot8Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[7].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot8Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot9Designation), SMBIOSlotConfig.SMBIOSSystemSlot[8].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[8].SlotType = PcdGet8(PcdSMBIOSSystemSlot9Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[8].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot9DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[8].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot9Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[8].SlotLength = PcdGet8(PcdSMBIOSSystemSlot9Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[8].SlotId = PcdGet16(PcdSMBIOSSystemSlot9Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[8].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot9Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot10Designation), SMBIOSlotConfig.SMBIOSSystemSlot[9].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[9].SlotType = PcdGet8(PcdSMBIOSSystemSlot10Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[9].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot10DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[9].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot10Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[9].SlotLength = PcdGet8(PcdSMBIOSSystemSlot10Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[9].SlotId = PcdGet16(PcdSMBIOSSystemSlot10Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[9].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot10Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot11Designation), SMBIOSlotConfig.SMBIOSSystemSlot[10].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[10].SlotType = PcdGet8(PcdSMBIOSSystemSlot11Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[10].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot11DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[10].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot11Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[10].SlotLength = PcdGet8(PcdSMBIOSSystemSlot11Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[10].SlotId = PcdGet16(PcdSMBIOSSystemSlot11Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[10].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot11Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot12Designation), SMBIOSlotConfig.SMBIOSSystemSlot[11].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[11].SlotType = PcdGet8(PcdSMBIOSSystemSlot12Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[11].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot12DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[11].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot12Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[11].SlotLength = PcdGet8(PcdSMBIOSSystemSlot12Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[11].SlotId = PcdGet16(PcdSMBIOSSystemSlot12Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[11].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot12Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot13Designation), SMBIOSlotConfig.SMBIOSSystemSlot[12].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[12].SlotType = PcdGet8(PcdSMBIOSSystemSlot13Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[12].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot13DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[12].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot13Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[12].SlotLength = PcdGet8(PcdSMBIOSSystemSlot13Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[12].SlotId = PcdGet16(PcdSMBIOSSystemSlot13Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[12].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot13Characteristics);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSlot14Designation), SMBIOSlotConfig.SMBIOSSystemSlot[13].SlotDesignation);
  SMBIOSlotConfig.SMBIOSSystemSlot[13].SlotType = PcdGet8(PcdSMBIOSSystemSlot14Type);
  SMBIOSlotConfig.SMBIOSSystemSlot[13].SlotDataBusWidth = PcdGet8(PcdSMBIOSSystemSlot14DataBusWidth);
  SMBIOSlotConfig.SMBIOSSystemSlot[13].SlotUsage = PcdGet8(PcdSMBIOSSystemSlot14Usage);
  SMBIOSlotConfig.SMBIOSSystemSlot[13].SlotLength = PcdGet8(PcdSMBIOSSystemSlot14Length);
  SMBIOSlotConfig.SMBIOSSystemSlot[13].SlotId = PcdGet16(PcdSMBIOSSystemSlot14Id);
  SMBIOSlotConfig.SMBIOSSystemSlot[13].SlotCharacteristics = PcdGet32(PcdSMBIOSSystemSlot14Characteristics);
}
/**
  This function makes boot time changes to the contents of the
  MiscSystemSlotDesignator structure (Type 9).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscSystemSlotDesignator)
{
  CHAR8                              *OptionalStrStart;
  UINTN                              SlotDesignationStrLen;
  EFI_STATUS                         Status;
  EFI_STRING                         SlotDesignation;
  STRING_REF                         TokenToUpdate;
  STRING_REF                         TokenToGet;
  SMBIOS_TABLE_TYPE9                 *SmbiosRecord;
  EFI_SMBIOS_HANDLE                  SmbiosHandle;
  EFI_MISC_SYSTEM_SLOT_DESIGNATION*  ForType9InputData;
  UINT8                              Index;

  ForType9InputData   = (EFI_MISC_SYSTEM_SLOT_DESIGNATION *)RecordData;

  TokenToGet          = 0;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!PcdMiscSlotIsInit) {
    GetMiscSLotConfigFromPcd ();
    PcdMiscSlotIsInit = TRUE;
  }

  for (Index = 0; Index < SMBIOS_SYSTEM_SLOT_MAX_NUM; Index++) {
    if (ForType9InputData->SlotDesignation == (mMiscSlotArray[Index])->SlotDesignation) {
      //DEBUG ((EFI_D_ERROR, "Found slot Data %d : ", Index));
      break;
    }
  }
  if (Index >= SMBIOSlotConfig.SMBIOSSystemSlotNumber) {
    return EFI_SUCCESS;
  }

  if (Index >= SMBIOS_SYSTEM_SLOT_MAX_NUM) {
    return EFI_INVALID_PARAMETER;
  }

  SlotDesignation = SMBIOSlotConfig.SMBIOSSystemSlot[Index].SlotDesignation;
  TokenToGet = STRING_TOKEN ((mMiscSlotArray[Index])->SlotDesignation);

  if (StrLen (SlotDesignation) > 0) {
    TokenToUpdate = STRING_TOKEN ((mMiscSlotArray[Index])->SlotDesignation);
    HiiSetString (mHiiHandle, TokenToUpdate, SlotDesignation, NULL);
  }

  SlotDesignation = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  SlotDesignationStrLen = StrLen(SlotDesignation);
  if (SlotDesignationStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }
  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE9) + SlotDesignationStrLen + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE9) +SlotDesignationStrLen + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_SLOTS;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE9);
  SmbiosRecord->Hdr.Handle = 0;
  SmbiosRecord->SlotDesignation = 1;
  SmbiosRecord->SlotType = SMBIOSlotConfig.SMBIOSSystemSlot[Index].SlotType;
  SmbiosRecord->SlotDataBusWidth = SMBIOSlotConfig.SMBIOSSystemSlot[Index].SlotDataBusWidth;
  SmbiosRecord->CurrentUsage = SMBIOSlotConfig.SMBIOSSystemSlot[Index].SlotUsage;
  SmbiosRecord->SlotLength = SMBIOSlotConfig.SMBIOSSystemSlot[Index].SlotLength;
  SmbiosRecord->SlotID = SMBIOSlotConfig.SMBIOSSystemSlot[Index].SlotId;
  *(UINT16 *)&SmbiosRecord->SlotCharacteristics1 = (UINT16)(SMBIOSlotConfig.SMBIOSSystemSlot[Index].SlotCharacteristics);

  //
  // Slot Characteristics
  //
  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStr(SlotDesignation, OptionalStrStart);
  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios-> Add(
                      Smbios,
                      NULL,
                      &SmbiosHandle,
                      (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord
                      );
  FreePool(SmbiosRecord);
  return Status;
}
