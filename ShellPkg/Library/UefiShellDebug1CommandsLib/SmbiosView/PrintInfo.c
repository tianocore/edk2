/** @file
  Module for clarifying the content of the smbios structure element information.

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2015-2019 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"
#include "PrintInfo.h"
#include "LibSmbiosView.h"
#include "QueryTable.h"
#include "EventLogInfo.h"

//
// Get the certain bit of 'value'
//
#define BIT(value, bit)  ((value) & ((UINT64) 1) << (bit))

//
// Check if above or equal to version
//
#define AE_SMBIOS_VERSION(MajorVersion, MinorVersion) \
  (SmbiosMajorVersion > (MajorVersion) || (SmbiosMajorVersion == (MajorVersion) && SmbiosMinorVersion >= (MinorVersion)))

//
//////////////////////////////////////////////////////////
//  Macros of print structure element, simplify coding.
//
#define PRINT_PENDING_STRING(pStruct, type, element) \
  do { \
    CHAR8 *StringBuf; \
    StringBuf = LibGetSmbiosString ((pStruct), (pStruct->type->element)); \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintEx(-1,-1,L": %a\n", (StringBuf != NULL) ? StringBuf: ""); \
  } while (0);

#define PRINT_SMBIOS_STRING(pStruct, stringnumber, element) \
  do { \
    CHAR8 *StringBuf; \
    StringBuf = LibGetSmbiosString ((pStruct), (stringnumber)); \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintEx(-1,-1,L": %a\n", (StringBuf != NULL) ? StringBuf: ""); \
  } while (0);

#define PRINT_STRUCT_VALUE(pStruct, type, element) \
  do { \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintEx(-1,-1,L": %u\n", (pStruct->type->element)); \
  } while (0);

#define PRINT_STRUCT_VALUE_H(pStruct, type, element) \
  do { \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintEx(-1,-1,L": 0x%x\n", (pStruct->type->element)); \
  } while (0);

#define PRINT_STRUCT_VALUE_LH(pStruct, type, element) \
  do { \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintEx(-1,-1,L": 0x%lx\n", (pStruct->type->element)); \
  } while (0);

#define PRINT_BIT_FIELD(pStruct, type, element, size) \
  do { \
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DUMP), gShellDebug1HiiHandle); \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SIZE), gShellDebug1HiiHandle, size); \
    DumpHex (0, 0, size, &(pStruct->type->element)); \
  } while (0);

#define PRINT_SMBIOS_BIT_FIELD(pStruct, startaddress, element, size) \
  do { \
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DUMP), gShellDebug1HiiHandle); \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SIZE), gShellDebug1HiiHandle, size); \
    DumpHex (0, 0, size, startaddress); \
  } while (0);

//
/////////////////////////////////////////
//

/**
  Copy Length of Src buffer to Dest buffer,
  add a NULL termination to Dest buffer.

  @param[in, out] Dest  Destination buffer head.
  @param[in] Src        Source buffer head.
  @param[in] Length     Length of buffer to be copied.
**/
VOID
MemToString (
  IN OUT VOID  *Dest,
  IN VOID      *Src,
  IN UINTN     Length
  )
{
  UINT8  *SrcBuffer;
  UINT8  *DestBuffer;

  SrcBuffer  = (UINT8 *)Src;
  DestBuffer = (UINT8 *)Dest;
  //
  // copy byte by byte
  //
  while ((Length--) != 0) {
    *DestBuffer++ = *SrcBuffer++;
  }

  //
  // append a NULL terminator
  //
  *DestBuffer = '\0';
}

//
//////////////////////////////////////////////
//
// Functions below is to show the information
//

/**
  Print the info of EPS(Entry Point Structure).

  @param[in] SmbiosTable    Pointer to the SMBIOS table entry point.
  @param[in] Option         Display option.
**/
VOID
SmbiosPrintEPSInfo (
  IN  SMBIOS_TABLE_ENTRY_POINT  *SmbiosTable,
  IN  UINT8                     Option
  )
{
  UINT8  Anchor[5];
  UINT8  InAnchor[6];

  if (SmbiosTable == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SMBIOSTABLE_NULL), gShellDebug1HiiHandle);
    return;
  }

  if (Option == SHOW_NONE) {
    return;
  }

  if (Option >= SHOW_NORMAL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENTRY_POINT_SIGN), gShellDebug1HiiHandle);
    MemToString (Anchor, SmbiosTable->AnchorString, 4);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ANCHOR_STR), gShellDebug1HiiHandle, Anchor);
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_EPS_CHECKSUM),
      gShellDebug1HiiHandle,
      SmbiosTable->EntryPointStructureChecksum
      );
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENTRY_POINT_LEN), gShellDebug1HiiHandle, SmbiosTable->EntryPointLength);
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VERSION),
      gShellDebug1HiiHandle,
      SmbiosTable->MajorVersion,
      SmbiosTable->MinorVersion
      );
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NUMBER_STRUCT),
      gShellDebug1HiiHandle,
      SmbiosTable->NumberOfSmbiosStructures
      );
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MAX_STRUCT_SIZE), gShellDebug1HiiHandle, SmbiosTable->MaxStructureSize);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TABLE_ADDR), gShellDebug1HiiHandle, SmbiosTable->TableAddress);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TABLE_LENGTH), gShellDebug1HiiHandle, SmbiosTable->TableLength);
  }

  //
  // If SHOW_ALL, also print followings.
  //
  if (Option >= SHOW_DETAIL) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENTRY_POINT_REVISION),
      gShellDebug1HiiHandle,
      SmbiosTable->EntryPointRevision
      );
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BCD_REVISION), gShellDebug1HiiHandle, SmbiosTable->SmbiosBcdRevision);
    //
    // Since raw data is not string, add a NULL terminater.
    //
    MemToString (InAnchor, SmbiosTable->IntermediateAnchorString, 5);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTER_ACHOR), gShellDebug1HiiHandle, InAnchor);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTER_CHECKSUM), gShellDebug1HiiHandle, SmbiosTable->IntermediateChecksum);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_FORMATTED_AREA), gShellDebug1HiiHandle);
    DumpHex (2, 0, 5, SmbiosTable->FormattedArea);
  }

  Print (L"\n");
}

/**
  Print the info of 64-bit EPS(Entry Point Structure).

  @param[in] SmbiosTable    Pointer to the SMBIOS table entry point.
  @param[in] Option         Display option.
**/
VOID
Smbios64BitPrintEPSInfo (
  IN  SMBIOS_TABLE_3_0_ENTRY_POINT  *SmbiosTable,
  IN  UINT8                         Option
  )
{
  UINT8  Anchor[5];

  if (SmbiosTable == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SMBIOSTABLE_NULL), gShellDebug1HiiHandle);
    return;
  }

  if (Option == SHOW_NONE) {
    return;
  }

  if (Option >= SHOW_NORMAL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_64_BIT_ENTRY_POINT_SIGN), gShellDebug1HiiHandle);

    MemToString (Anchor, SmbiosTable->AnchorString, 5);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ANCHOR_STR), gShellDebug1HiiHandle, Anchor);

    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_EPS_CHECKSUM),
      gShellDebug1HiiHandle,
      SmbiosTable->EntryPointStructureChecksum
      );

    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENTRY_POINT_LEN), gShellDebug1HiiHandle, SmbiosTable->EntryPointLength);

    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VERSION),
      gShellDebug1HiiHandle,
      SmbiosTable->MajorVersion,
      SmbiosTable->MinorVersion
      );

    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DOCREV),
      gShellDebug1HiiHandle,
      SmbiosTable->DocRev
      );

    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TABLE_MAX_SIZE), gShellDebug1HiiHandle, SmbiosTable->TableMaximumSize);

    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TABLE_ADDR), gShellDebug1HiiHandle, SmbiosTable->TableAddress);
  }

  //
  // If SHOW_ALL, also print followings.
  //
  if (Option >= SHOW_DETAIL) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENTRY_POINT_REVISION),
      gShellDebug1HiiHandle,
      SmbiosTable->EntryPointRevision
      );
  }

  Print (L"\n");
}

/**
  This function print the content of the structure pointed by Struct.

  @param[in] Struct       Point to the structure to be printed.
  @param[in] Option       Print option of information detail.

  @retval EFI_SUCCESS               Successfully Printing this function.
  @retval EFI_INVALID_PARAMETER     Invalid Structure.
  @retval EFI_UNSUPPORTED           Unsupported.
**/
EFI_STATUS
SmbiosPrintStructure (
  IN  SMBIOS_STRUCTURE_POINTER  *Struct,
  IN  UINT8                     Option
  )
{
  UINT8  Index;
  UINT8  Index2;
  UINT8  *Buffer;

  if (Struct == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Option == SHOW_NONE) {
    return EFI_SUCCESS;
  }

  Buffer = (UINT8 *)(UINTN)(Struct->Raw);

  //
  // Display structure header
  //
  DisplayStructureTypeInfo (Struct->Hdr->Type, SHOW_DETAIL);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_FORMAT_PART_LEN), gShellDebug1HiiHandle, Struct->Hdr->Length);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_STRUCT_HANDLE), gShellDebug1HiiHandle, Struct->Hdr->Handle);

  if (Option == SHOW_OUTLINE) {
    return EFI_SUCCESS;
  }

  switch (Struct->Hdr->Type) {
    //
    // BIOS Information (Type 0)
    //
    case 0:
      PRINT_PENDING_STRING (Struct, Type0, Vendor);
      PRINT_PENDING_STRING (Struct, Type0, BiosVersion);
      PRINT_STRUCT_VALUE_H (Struct, Type0, BiosSegment);
      PRINT_PENDING_STRING (Struct, Type0, BiosReleaseDate);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_SIZE), gShellDebug1HiiHandle, 64 * (Struct->Type0->BiosSize + 1));

      DisplayBiosCharacteristics (ReadUnaligned64 ((UINT64 *)(UINTN)&(Struct->Type0->BiosCharacteristics)), Option);

      if (Struct->Hdr->Length > 0x12) {
        DisplayBiosCharacteristicsExt1 (Struct->Type0->BIOSCharacteristicsExtensionBytes[0], Option);
      }

      if (Struct->Hdr->Length > 0x13) {
        DisplayBiosCharacteristicsExt2 (Struct->Type0->BIOSCharacteristicsExtensionBytes[1], Option);
      }

      if (AE_SMBIOS_VERSION (0x2, 0x4) && (Struct->Hdr->Length > 0x14)) {
        PRINT_STRUCT_VALUE (Struct, Type0, SystemBiosMajorRelease);
        PRINT_STRUCT_VALUE (Struct, Type0, SystemBiosMinorRelease);
        PRINT_STRUCT_VALUE (Struct, Type0, EmbeddedControllerFirmwareMajorRelease);
        PRINT_STRUCT_VALUE (Struct, Type0, EmbeddedControllerFirmwareMinorRelease);
      }

      if (AE_SMBIOS_VERSION (0x3, 0x1) && (Struct->Hdr->Length > 0x18)) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_EXTENDED_BIOS_SIZE),
          gShellDebug1HiiHandle,
          Struct->Type0->ExtendedBiosSize.Size,
          (Struct->Type0->ExtendedBiosSize.Unit == 0x0) ? L"MB" : L"GB"
          );
      }

      break;

    //
    // System Information (Type 1)
    //
    case 1:
      PRINT_PENDING_STRING (Struct, Type1, Manufacturer);
      PRINT_PENDING_STRING (Struct, Type1, ProductName);
      PRINT_PENDING_STRING (Struct, Type1, Version);
      PRINT_PENDING_STRING (Struct, Type1, SerialNumber);
      PRINT_BIT_FIELD (Struct, Type1, Uuid, 16);
      DisplaySystemWakeupType (Struct->Type1->WakeUpType, Option);
      if (AE_SMBIOS_VERSION (0x2, 0x4) && (Struct->Hdr->Length > 0x19)) {
        PRINT_PENDING_STRING (Struct, Type1, SKUNumber);
        PRINT_PENDING_STRING (Struct, Type1, Family);
      }

      break;

    //
    // Baseboard Information (Type 2)
    //
    case 2:
      PRINT_PENDING_STRING (Struct, Type2, Manufacturer);
      PRINT_PENDING_STRING (Struct, Type2, ProductName);
      PRINT_PENDING_STRING (Struct, Type2, Version);
      PRINT_PENDING_STRING (Struct, Type2, SerialNumber);
      if (Struct->Hdr->Length > 0x8) {
        PRINT_PENDING_STRING (Struct, Type2, AssetTag);
        DisplayBaseBoardFeatureFlags (*(UINT8 *)&Struct->Type2->FeatureFlag, Option);
        PRINT_PENDING_STRING (Struct, Type2, LocationInChassis);
        PRINT_STRUCT_VALUE_H (Struct, Type2, ChassisHandle);
        DisplayBaseBoardBoardType (Struct->Type2->BoardType, Option);
      }

      break;

    //
    // System Enclosure (Type 3)
    //
    case 3:
      PRINT_PENDING_STRING (Struct, Type3, Manufacturer);
      PRINT_STRUCT_VALUE (Struct, Type3, Type);
      DisplaySystemEnclosureType (Struct->Type3->Type, Option);
      PRINT_PENDING_STRING (Struct, Type3, Version);
      PRINT_PENDING_STRING (Struct, Type3, SerialNumber);
      PRINT_PENDING_STRING (Struct, Type3, AssetTag);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BOOTUP_STATE), gShellDebug1HiiHandle);
      DisplaySystemEnclosureStatus (Struct->Type3->BootupState, Option);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_STATE), gShellDebug1HiiHandle);
      DisplaySystemEnclosureStatus (Struct->Type3->PowerSupplyState, Option);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_THERMAL_STATE), gShellDebug1HiiHandle);
      DisplaySystemEnclosureStatus (Struct->Type3->ThermalState, Option);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SECURITY_STATUS), gShellDebug1HiiHandle);
      DisplaySESecurityStatus (Struct->Type3->SecurityStatus, Option);
      if (AE_SMBIOS_VERSION (0x2, 0x3)) {
        if (Struct->Hdr->Length > 0xD) {
          PRINT_BIT_FIELD (Struct, Type3, OemDefined, 4);
        }

        if (Struct->Hdr->Length > 0x11) {
          PRINT_STRUCT_VALUE (Struct, Type3, Height);
        }

        if (Struct->Hdr->Length > 0x12) {
          PRINT_STRUCT_VALUE (Struct, Type3, NumberofPowerCords);
        }

        if (Struct->Hdr->Length > 0x13) {
          PRINT_STRUCT_VALUE (Struct, Type3, ContainedElementCount);
        }

        if (Struct->Hdr->Length > 0x14) {
          PRINT_STRUCT_VALUE (Struct, Type3, ContainedElementRecordLength);
        }

        if (Struct->Hdr->Length > 0x15) {
          for (Index = 0; Index < Struct->Type3->ContainedElementCount; Index++) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CONTAINED_ELEMENT), gShellDebug1HiiHandle, Index+1);
            for (Index2 = 0; Index2 < Struct->Type3->ContainedElementRecordLength; Index2++) {
              Print (L"%02X ", Buffer[0x15 + (Index * Struct->Type3->ContainedElementRecordLength) + Index2]);
            }

            Print (L"\n");
          }
        }
      }

      if (AE_SMBIOS_VERSION (0x2, 0x7) && (Struct->Hdr->Length > 0x13)) {
        if (Struct->Hdr->Length > (0x15 + (Struct->Type3->ContainedElementCount * Struct->Type3->ContainedElementRecordLength))) {
          PRINT_SMBIOS_STRING (Struct, Buffer[0x15 + (Struct->Type3->ContainedElementCount * Struct->Type3->ContainedElementRecordLength)], SKUNumber);
        }
      }

      break;

    //
    // Processor Information (Type 4)
    //
    case 4:
      PRINT_SMBIOS_STRING (Struct, Struct->Type4->Socket, SocketDesignation)
      DisplayProcessorType (Struct->Type4->ProcessorType, Option);
      if (AE_SMBIOS_VERSION (0x2, 0x6) && (Struct->Hdr->Length > 0x28) &&
          (Struct->Type4->ProcessorFamily == 0xFE))
      {
        //
        // Get family from ProcessorFamily2 field
        //
        DisplayProcessorFamily2 (Struct->Type4->ProcessorFamily2, Option);
      } else {
        DisplayProcessorFamily (Struct->Type4->ProcessorFamily, Option);
      }

      PRINT_PENDING_STRING (Struct, Type4, ProcessorManufacturer);
      PRINT_BIT_FIELD (Struct, Type4, ProcessorId, 8);
      PRINT_PENDING_STRING (Struct, Type4, ProcessorVersion);
      DisplayProcessorVoltage (*(UINT8 *)&(Struct->Type4->Voltage), Option);
      PRINT_STRUCT_VALUE (Struct, Type4, ExternalClock);
      PRINT_STRUCT_VALUE (Struct, Type4, MaxSpeed);
      PRINT_STRUCT_VALUE (Struct, Type4, CurrentSpeed);
      DisplayProcessorStatus (Struct->Type4->Status, Option);
      DisplayProcessorUpgrade (Struct->Type4->ProcessorUpgrade, Option);
      PRINT_STRUCT_VALUE_H (Struct, Type4, L1CacheHandle);
      PRINT_STRUCT_VALUE_H (Struct, Type4, L2CacheHandle);
      PRINT_STRUCT_VALUE_H (Struct, Type4, L3CacheHandle);
      if (AE_SMBIOS_VERSION (0x2, 0x3) && (Struct->Hdr->Length > 0x20)) {
        PRINT_PENDING_STRING (Struct, Type4, SerialNumber);
        PRINT_PENDING_STRING (Struct, Type4, AssetTag);
        PRINT_PENDING_STRING (Struct, Type4, PartNumber);
      }

      if (AE_SMBIOS_VERSION (0x2, 0x5) && (Struct->Hdr->Length > 0x23)) {
        PRINT_STRUCT_VALUE (Struct, Type4, CoreCount);
        PRINT_STRUCT_VALUE (Struct, Type4, EnabledCoreCount);
        PRINT_STRUCT_VALUE (Struct, Type4, ThreadCount);
        DisplayProcessorCharacteristics (Struct->Type4->ProcessorCharacteristics, Option);
      }

      if ((SmbiosMajorVersion >= 0x3) && (Struct->Hdr->Length > 0x2A)) {
        PRINT_STRUCT_VALUE (Struct, Type4, CoreCount2);
        PRINT_STRUCT_VALUE (Struct, Type4, EnabledCoreCount2);
        PRINT_STRUCT_VALUE (Struct, Type4, ThreadCount2);
      }

      break;

    //
    // Memory Controller Information (Type 5)
    //
    case 5:
    {
      UINT8  SlotNum;
      SlotNum = Struct->Type5->AssociatedMemorySlotNum;

      DisplayMcErrorDetectMethod (Struct->Type5->ErrDetectMethod, Option);
      DisplayMcErrorCorrectCapability (*(UINT8 *)&(Struct->Type5->ErrCorrectCapability), Option);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SUPOPRT), gShellDebug1HiiHandle);
      DisplayMcInterleaveSupport (Struct->Type5->SupportInterleave, Option);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CURRENT), gShellDebug1HiiHandle);
      DisplayMcInterleaveSupport (Struct->Type5->CurrentInterleave, Option);
      DisplayMaxMemoryModuleSize (Struct->Type5->MaxMemoryModuleSize, SlotNum, Option);
      DisplayMcMemorySpeeds (*(UINT16 *)&(Struct->Type5->SupportSpeed), Option);
      DisplayMmMemoryType (Struct->Type5->SupportMemoryType, Option);
      DisplayMemoryModuleVoltage (Struct->Type5->MemoryModuleVoltage, Option);
      PRINT_STRUCT_VALUE (Struct, Type5, AssociatedMemorySlotNum);
      //
      // According to SMBIOS Specification, offset 0x0F
      //
      DisplayMemoryModuleConfigHandles ((UINT16 *)(&Buffer[0x0F]), SlotNum, Option);
      DisplayMcErrorCorrectCapability (Buffer[0x0F + 2 * SlotNum], Option);
      break;
    }

    //
    // Memory Module Information (Type 6)
    //
    case 6:
      PRINT_PENDING_STRING (Struct, Type6, SocketDesignation);
      DisplayMmBankConnections (Struct->Type6->BankConnections, Option);
      PRINT_STRUCT_VALUE (Struct, Type6, CurrentSpeed);
      DisplayMmMemoryType (*(UINT16 *)&(Struct->Type6->CurrentMemoryType), Option);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INSTALLED), gShellDebug1HiiHandle);
      DisplayMmMemorySize (*(UINT8 *)&(Struct->Type6->InstalledSize), Option);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENABLED), gShellDebug1HiiHandle);
      DisplayMmMemorySize (*(UINT8 *)&(Struct->Type6->EnabledSize), Option);
      DisplayMmErrorStatus (Struct->Type6->ErrorStatus, Option);
      break;

    //
    // Cache Information (Type 7)
    //
    case 7:
      PRINT_PENDING_STRING (Struct, Type7, SocketDesignation);
      DisplayCacheConfiguration (Struct->Type7->CacheConfiguration, Option);
      PRINT_STRUCT_VALUE_H (Struct, Type7, MaximumCacheSize);
      PRINT_STRUCT_VALUE_H (Struct, Type7, InstalledSize);
      PRINT_STRUCT_VALUE_H (Struct, Type7, SupportedSRAMType);
      PRINT_STRUCT_VALUE_H (Struct, Type7, CurrentSRAMType);
      DisplayCacheSRAMType (ReadUnaligned16 ((UINT16 *)(UINTN)&(Struct->Type7->CurrentSRAMType)), Option);
      PRINT_STRUCT_VALUE_H (Struct, Type7, CacheSpeed);
      DisplayCacheErrCorrectingType (Struct->Type7->ErrorCorrectionType, Option);
      DisplayCacheSystemCacheType (Struct->Type7->SystemCacheType, Option);
      DisplayCacheAssociativity (Struct->Type7->Associativity, Option);
      if (AE_SMBIOS_VERSION (0x3, 0x1) && (Struct->Hdr->Length > 0x13)) {
        PRINT_STRUCT_VALUE_H (Struct, Type7, MaximumCacheSize2);
        PRINT_STRUCT_VALUE_H (Struct, Type7, InstalledSize2);
      }

      break;

    //
    // Port Connector Information  (Type 8)
    //
    case 8:
      PRINT_PENDING_STRING (Struct, Type8, InternalReferenceDesignator);
      Print (L"Internal ");
      DisplayPortConnectorType (Struct->Type8->InternalConnectorType, Option);
      PRINT_PENDING_STRING (Struct, Type8, ExternalReferenceDesignator);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_EXTERNAL), gShellDebug1HiiHandle);
      DisplayPortConnectorType (Struct->Type8->ExternalConnectorType, Option);
      DisplayPortType (Struct->Type8->PortType, Option);
      break;

    //
    // System Slots (Type 9)
    //
    case 9:
    {
      MISC_SLOT_PEER_GROUP  *PeerGroupPtr;
      UINT8                 PeerGroupCount;

      PRINT_PENDING_STRING (Struct, Type9, SlotDesignation);
      DisplaySystemSlotType (Struct->Type9->SlotType, Option);
      DisplaySystemSlotDataBusWidth (Struct->Type9->SlotDataBusWidth, Option);
      DisplaySystemSlotCurrentUsage (Struct->Type9->CurrentUsage, Option);
      DisplaySystemSlotLength (Struct->Type9->SlotLength, Option);
      DisplaySystemSlotId (
        Struct->Type9->SlotID,
        Struct->Type9->SlotType,
        Option
        );
      DisplaySlotCharacteristics1 (*(UINT8 *)&(Struct->Type9->SlotCharacteristics1), Option);
      DisplaySlotCharacteristics2 (*(UINT8 *)&(Struct->Type9->SlotCharacteristics2), Option);
      if (AE_SMBIOS_VERSION (0x2, 0x6) && (Struct->Hdr->Length > 0xD)) {
        PRINT_STRUCT_VALUE_H (Struct, Type9, SegmentGroupNum);
        PRINT_STRUCT_VALUE_H (Struct, Type9, BusNum);
        PRINT_STRUCT_VALUE_H (Struct, Type9, DevFuncNum);
      }

      if (AE_SMBIOS_VERSION (0x3, 0x2)) {
        if (Struct->Hdr->Length > 0x11) {
          PRINT_STRUCT_VALUE (Struct, Type9, DataBusWidth);
        }

        if (Struct->Hdr->Length > 0x12) {
          PRINT_STRUCT_VALUE (Struct, Type9, PeerGroupingCount);

          PeerGroupCount = Struct->Type9->PeerGroupingCount;
          PeerGroupPtr   = Struct->Type9->PeerGroups;
          for (Index = 0; Index < PeerGroupCount; Index++) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SLOT_PEER_GROUPS), gShellDebug1HiiHandle, Index + 1);
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SEGMENT_GROUP_NUM), gShellDebug1HiiHandle, PeerGroupPtr[Index].SegmentGroupNum);
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BUS_NUM), gShellDebug1HiiHandle, PeerGroupPtr[Index].BusNum);
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DEV_FUNC_NUM), gShellDebug1HiiHandle, PeerGroupPtr[Index].DevFuncNum);
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DATA_BUS_WIDTH), gShellDebug1HiiHandle, PeerGroupPtr[Index].DataBusWidth);
          }
        }
      }

      break;
    }

    //
    // On Board Devices Information (Type 10)
    //
    case 10:
    {
      UINTN  NumOfDevice;
      NumOfDevice = (Struct->Type10->Hdr.Length - sizeof (SMBIOS_STRUCTURE)) / (2 * sizeof (UINT8));
      for (Index = 0; Index < NumOfDevice; Index++) {
        ShellPrintEx (-1, -1, (((Struct->Type10->Device[Index].DeviceType) & 0x80) != 0) ? L"Device Enabled\n" : L"Device Disabled\n");
        DisplayOnboardDeviceTypes ((Struct->Type10->Device[Index].DeviceType) & 0x7F, Option);
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DESC_STRING), gShellDebug1HiiHandle);
        ShellPrintEx (-1, -1, L"%a\n", LibGetSmbiosString (Struct, Struct->Type10->Device[Index].DescriptionString));
      }

      break;
    }

    //
    // Oem Strings (Type 11)
    //
    case 11:
      PRINT_STRUCT_VALUE (Struct, Type11, StringCount);
      for (Index = 1; Index <= Struct->Type11->StringCount; Index++) {
        ShellPrintEx (-1, -1, L"%a\n", LibGetSmbiosString (Struct, Index));
      }

      break;

    //
    // System Configuration Options (Type 12)
    //
    case 12:
      PRINT_STRUCT_VALUE (Struct, Type12, StringCount);
      for (Index = 1; Index <= Struct->Type12->StringCount; Index++) {
        ShellPrintEx (-1, -1, L"%a\n", LibGetSmbiosString (Struct, Index));
      }

      break;

    //
    // BIOS Language Information (Type 13)
    //
    case 13:
      PRINT_STRUCT_VALUE (Struct, Type13, InstallableLanguages);
      PRINT_STRUCT_VALUE (Struct, Type13, Flags);
      PRINT_BIT_FIELD (Struct, Type13, Reserved, 15);
      PRINT_PENDING_STRING (Struct, Type13, CurrentLanguages);
      break;

    //
    // Group Associations (Type 14)
    //
    case 14:
    {
      UINT8  NumOfItem;
      NumOfItem = (Struct->Type14->Hdr.Length - 5) / 3;
      PRINT_PENDING_STRING (Struct, Type14, GroupName);
      for (Index = 0; Index < NumOfItem; Index++) {
        ShellPrintEx (-1, -1, L"ItemType %u: %u\n", Index + 1, Struct->Type14->Group[Index].ItemType);
        ShellPrintEx (-1, -1, L"ItemHandle %u: %u\n", Index + 1, Struct->Type14->Group[Index].ItemHandle);
      }

      break;
    }

    //
    // System Event Log (Type 15)
    //
    case 15:
    {
      EVENT_LOG_TYPE  *Ptr;
      UINT8           Count;
      UINT8           *AccessMethodAddress;

      PRINT_STRUCT_VALUE_H (Struct, Type15, LogAreaLength);
      PRINT_STRUCT_VALUE_H (Struct, Type15, LogHeaderStartOffset);
      PRINT_STRUCT_VALUE_H (Struct, Type15, LogDataStartOffset);
      DisplaySELAccessMethod (Struct->Type15->AccessMethod, Option);
      PRINT_STRUCT_VALUE_H (Struct, Type15, AccessMethodAddress);
      DisplaySELLogStatus (Struct->Type15->LogStatus, Option);
      PRINT_STRUCT_VALUE_H (Struct, Type15, LogChangeToken);
      DisplaySysEventLogHeaderFormat (Struct->Type15->LogHeaderFormat, Option);
      PRINT_STRUCT_VALUE_H (Struct, Type15, NumberOfSupportedLogTypeDescriptors);
      PRINT_STRUCT_VALUE_H (Struct, Type15, LengthOfLogTypeDescriptor);

      Count = Struct->Type15->NumberOfSupportedLogTypeDescriptors;
      if (Count > 0) {
        Ptr = Struct->Type15->EventLogTypeDescriptors;

        //
        // Display all Event Log type descriptors supported by system
        //
        for (Index = 0; Index < Count; Index++, Ptr++) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SUPOPRTED_EVENT), gShellDebug1HiiHandle, Index + 1);
          DisplaySELTypes (Ptr->LogType, Option);
          DisplaySELVarDataFormatType (Ptr->DataFormatType, Option);
        }

        if (Option >= SHOW_DETAIL) {
          switch (Struct->Type15->AccessMethod) {
            case 03:
              AccessMethodAddress = (UINT8 *)(UINTN)(Struct->Type15->AccessMethodAddress);
              break;

            case 00:
            case 01:
            case 02:
            case 04:
            default:
              ShellPrintHiiEx (
                -1,
                -1,
                NULL,
                STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ACCESS_METHOD_NOT_SUPOPRTED),
                gShellDebug1HiiHandle,
                Struct->Type15->AccessMethod
                );
              return EFI_UNSUPPORTED;
          }

          //
          // Display Event Log Header
          //
          // Starting offset (or index) within the nonvolatile storage
          // of the event-log's header, from the Access Method Address
          //
          DisplaySysEventLogHeader (
            Struct->Type15->LogHeaderFormat,
            AccessMethodAddress + Struct->Type15->LogHeaderStartOffset
            );

          //
          // Display all Event Log data
          //
          // Starting offset (or index) within the nonvolatile storage
          // of the event-log's first data byte, from the Access Method Address(0x14)
          //
          DisplaySysEventLogData (
            AccessMethodAddress + Struct->Type15->LogDataStartOffset,
            (UINT16)
            (
             Struct->Type15->LogAreaLength -
             (Struct->Type15->LogDataStartOffset - Struct->Type15->LogHeaderStartOffset)
            )
            );
        }
      }

      break;
    }

    //
    // Physical Memory Array (Type 16)
    //
    case 16:
      DisplayPMALocation (Struct->Type16->Location, Option);
      DisplayPMAUse (Struct->Type16->Use, Option);
      DisplayPMAErrorCorrectionTypes (
        Struct->Type16->MemoryErrorCorrection,
        Option
        );
      PRINT_STRUCT_VALUE_H (Struct, Type16, MaximumCapacity);
      PRINT_STRUCT_VALUE_H (Struct, Type16, MemoryErrorInformationHandle);
      PRINT_STRUCT_VALUE_H (Struct, Type16, NumberOfMemoryDevices);
      if (AE_SMBIOS_VERSION (0x2, 0x7) && (Struct->Hdr->Length > 0xF)) {
        PRINT_STRUCT_VALUE_LH (Struct, Type16, ExtendedMaximumCapacity);
      }

      break;

    //
    // Memory Device (Type 17)
    //
    case 17:
      PRINT_STRUCT_VALUE_H (Struct, Type17, MemoryArrayHandle);
      PRINT_STRUCT_VALUE_H (Struct, Type17, MemoryErrorInformationHandle);
      PRINT_STRUCT_VALUE_H (Struct, Type17, TotalWidth);
      PRINT_STRUCT_VALUE_H (Struct, Type17, DataWidth);
      PRINT_STRUCT_VALUE (Struct, Type17, Size);
      DisplayMemoryDeviceFormFactor (Struct->Type17->FormFactor, Option);
      PRINT_STRUCT_VALUE_H (Struct, Type17, DeviceSet);
      PRINT_PENDING_STRING (Struct, Type17, DeviceLocator);
      PRINT_PENDING_STRING (Struct, Type17, BankLocator);
      DisplayMemoryDeviceType (Struct->Type17->MemoryType, Option);
      DisplayMemoryDeviceTypeDetail (ReadUnaligned16 ((UINT16 *)(UINTN)&(Struct->Type17->TypeDetail)), Option);
      PRINT_STRUCT_VALUE_H (Struct, Type17, Speed);
      PRINT_PENDING_STRING (Struct, Type17, Manufacturer);
      PRINT_PENDING_STRING (Struct, Type17, SerialNumber);
      PRINT_PENDING_STRING (Struct, Type17, AssetTag);
      PRINT_PENDING_STRING (Struct, Type17, PartNumber);
      if (AE_SMBIOS_VERSION (0x2, 0x6) && (Struct->Hdr->Length > 0x1B)) {
        PRINT_STRUCT_VALUE_H (Struct, Type17, Attributes);
      }

      if (AE_SMBIOS_VERSION (0x2, 0x7) && (Struct->Hdr->Length > 0x1C)) {
        PRINT_STRUCT_VALUE (Struct, Type17, ExtendedSize);
        PRINT_STRUCT_VALUE_H (Struct, Type17, ConfiguredMemoryClockSpeed);
      }

      if (AE_SMBIOS_VERSION (0x2, 0x8) && (Struct->Hdr->Length > 0x22)) {
        PRINT_STRUCT_VALUE (Struct, Type17, MinimumVoltage);
        PRINT_STRUCT_VALUE (Struct, Type17, MaximumVoltage);
        PRINT_STRUCT_VALUE (Struct, Type17, ConfiguredVoltage);
      }

      if (AE_SMBIOS_VERSION (0x3, 0x2)) {
        if (Struct->Hdr->Length > 0x28) {
          DisplayMemoryDeviceMemoryTechnology (Struct->Type17->MemoryTechnology, Option);
          DisplayMemoryDeviceMemoryOperatingModeCapability (Struct->Type17->MemoryOperatingModeCapability.Uint16, Option);
          PRINT_PENDING_STRING (Struct, Type17, FirmwareVersion);
          PRINT_STRUCT_VALUE_H (Struct, Type17, ModuleManufacturerID);
          PRINT_STRUCT_VALUE_H (Struct, Type17, ModuleProductID);
          PRINT_STRUCT_VALUE_H (Struct, Type17, MemorySubsystemControllerManufacturerID);
          PRINT_STRUCT_VALUE_H (Struct, Type17, MemorySubsystemControllerProductID);
        }

        if (Struct->Hdr->Length > 0x34) {
          PRINT_STRUCT_VALUE_LH (Struct, Type17, NonVolatileSize);
        }

        if (Struct->Hdr->Length > 0x3C) {
          PRINT_STRUCT_VALUE_LH (Struct, Type17, VolatileSize);
        }

        if (Struct->Hdr->Length > 0x44) {
          PRINT_STRUCT_VALUE_LH (Struct, Type17, CacheSize);
        }

        if (Struct->Hdr->Length > 0x4C) {
          PRINT_STRUCT_VALUE_LH (Struct, Type17, LogicalSize);
        }
      }

      break;

    //
    // 32-bit Memory Error Information (Type 18)
    //
    case 18:
      DisplayMemoryErrorType (Struct->Type18->ErrorType, Option);
      DisplayMemoryErrorGranularity (
        Struct->Type18->ErrorGranularity,
        Option
        );
      DisplayMemoryErrorOperation (Struct->Type18->ErrorOperation, Option);
      PRINT_STRUCT_VALUE_H (Struct, Type18, VendorSyndrome);
      PRINT_STRUCT_VALUE_H (Struct, Type18, MemoryArrayErrorAddress);
      PRINT_STRUCT_VALUE_H (Struct, Type18, DeviceErrorAddress);
      PRINT_STRUCT_VALUE_H (Struct, Type18, ErrorResolution);
      break;

    //
    // Memory Array Mapped Address (Type 19)
    //
    case 19:
      PRINT_STRUCT_VALUE_H (Struct, Type19, StartingAddress);
      PRINT_STRUCT_VALUE_H (Struct, Type19, EndingAddress);
      PRINT_STRUCT_VALUE_H (Struct, Type19, MemoryArrayHandle);
      PRINT_STRUCT_VALUE_H (Struct, Type19, PartitionWidth);
      if (AE_SMBIOS_VERSION (0x2, 0x7) && (Struct->Hdr->Length > 0xF)) {
        PRINT_STRUCT_VALUE_LH (Struct, Type19, ExtendedStartingAddress);
        PRINT_STRUCT_VALUE_LH (Struct, Type19, ExtendedEndingAddress);
      }

      break;

    //
    // Memory Device Mapped Address (Type 20)
    //
    case 20:
      PRINT_STRUCT_VALUE_H (Struct, Type20, StartingAddress);
      PRINT_STRUCT_VALUE_H (Struct, Type20, EndingAddress);
      PRINT_STRUCT_VALUE_H (Struct, Type20, MemoryDeviceHandle);
      PRINT_STRUCT_VALUE_H (Struct, Type20, MemoryArrayMappedAddressHandle);
      PRINT_STRUCT_VALUE_H (Struct, Type20, PartitionRowPosition);
      PRINT_STRUCT_VALUE_H (Struct, Type20, InterleavePosition);
      PRINT_STRUCT_VALUE_H (Struct, Type20, InterleavedDataDepth);
      if (AE_SMBIOS_VERSION (0x2, 0x7) && (Struct->Hdr->Length > 0x13)) {
        PRINT_STRUCT_VALUE_LH (Struct, Type19, ExtendedStartingAddress);
        PRINT_STRUCT_VALUE_LH (Struct, Type19, ExtendedEndingAddress);
      }

      break;

    //
    // Built-in Pointing Device (Type 21)
    //
    case 21:
      DisplayPointingDeviceType (Struct->Type21->Type, Option);
      DisplayPointingDeviceInterface (Struct->Type21->Interface, Option);
      PRINT_STRUCT_VALUE (Struct, Type21, NumberOfButtons);
      break;

    //
    // Portable Battery (Type 22)
    //
    case 22:
      PRINT_PENDING_STRING (Struct, Type22, Location);
      PRINT_PENDING_STRING (Struct, Type22, Manufacturer);
      PRINT_PENDING_STRING (Struct, Type22, ManufactureDate);
      PRINT_PENDING_STRING (Struct, Type22, SerialNumber);
      PRINT_PENDING_STRING (Struct, Type22, DeviceName);
      DisplayPBDeviceChemistry (
        Struct->Type22->DeviceChemistry,
        Option
        );
      PRINT_STRUCT_VALUE_H (Struct, Type22, DeviceCapacity);
      PRINT_STRUCT_VALUE_H (Struct, Type22, DesignVoltage);
      PRINT_PENDING_STRING (Struct, Type22, SBDSVersionNumber);
      PRINT_STRUCT_VALUE_H (Struct, Type22, MaximumErrorInBatteryData);
      PRINT_STRUCT_VALUE_H (Struct, Type22, SBDSSerialNumber);
      DisplaySBDSManufactureDate (
        Struct->Type22->SBDSManufactureDate,
        Option
        );
      PRINT_PENDING_STRING (Struct, Type22, SBDSDeviceChemistry);
      PRINT_STRUCT_VALUE_H (Struct, Type22, DesignCapacityMultiplier);
      PRINT_STRUCT_VALUE_H (Struct, Type22, OEMSpecific);
      break;

    //
    // System Reset (Type 23)
    //
    case 23:
      DisplaySystemResetCapabilities (
        Struct->Type23->Capabilities,
        Option
        );
      PRINT_STRUCT_VALUE_H (Struct, Type23, ResetCount);
      PRINT_STRUCT_VALUE_H (Struct, Type23, ResetLimit);
      PRINT_STRUCT_VALUE_H (Struct, Type23, TimerInterval);
      PRINT_STRUCT_VALUE_H (Struct, Type23, Timeout);
      break;

    //
    // Hardware Security (Type 24)
    //
    case 24:
      DisplayHardwareSecuritySettings (
        Struct->Type24->HardwareSecuritySettings,
        Option
        );
      break;

    //
    // System Power Controls (Type 25)
    //
    case 25:
      PRINT_STRUCT_VALUE_H (Struct, Type25, NextScheduledPowerOnMonth);
      PRINT_STRUCT_VALUE_H (Struct, Type25, NextScheduledPowerOnDayOfMonth);
      PRINT_STRUCT_VALUE_H (Struct, Type25, NextScheduledPowerOnHour);
      PRINT_STRUCT_VALUE_H (Struct, Type25, NextScheduledPowerOnMinute);
      PRINT_STRUCT_VALUE_H (Struct, Type25, NextScheduledPowerOnSecond);
      break;

    //
    // Voltage Probe (Type 26)
    //
    case 26:
      PRINT_PENDING_STRING (Struct, Type26, Description);
      DisplayVPLocation (*(UINT8 *)&(Struct->Type26->LocationAndStatus), Option);
      DisplayVPStatus (*(UINT8 *)&(Struct->Type26->LocationAndStatus), Option);
      PRINT_STRUCT_VALUE_H (Struct, Type26, MaximumValue);
      PRINT_STRUCT_VALUE_H (Struct, Type26, MinimumValue);
      PRINT_STRUCT_VALUE_H (Struct, Type26, Resolution);
      PRINT_STRUCT_VALUE_H (Struct, Type26, Tolerance);
      PRINT_STRUCT_VALUE_H (Struct, Type26, Accuracy);
      PRINT_STRUCT_VALUE_H (Struct, Type26, OEMDefined);
      PRINT_STRUCT_VALUE_H (Struct, Type26, NominalValue);
      break;

    //
    // Cooling Device (Type 27)
    //
    case 27:
      PRINT_STRUCT_VALUE_H (Struct, Type27, TemperatureProbeHandle);
      DisplayCoolingDeviceStatus (*(UINT8 *)&(Struct->Type27->DeviceTypeAndStatus), Option);
      DisplayCoolingDeviceType (*(UINT8 *)&(Struct->Type27->DeviceTypeAndStatus), Option);
      PRINT_STRUCT_VALUE_H (Struct, Type27, CoolingUnitGroup);
      PRINT_STRUCT_VALUE_H (Struct, Type27, OEMDefined);
      PRINT_STRUCT_VALUE_H (Struct, Type27, NominalSpeed);
      if (AE_SMBIOS_VERSION (0x2, 0x7) && (Struct->Hdr->Length > 0xE)) {
        PRINT_PENDING_STRING (Struct, Type27, Description);
      }

      break;

    //
    // Temperature Probe (Type 28)
    //
    case 28:
      PRINT_PENDING_STRING (Struct, Type28, Description);
      DisplayTemperatureProbeStatus (*(UINT8 *)&(Struct->Type28->LocationAndStatus), Option);
      DisplayTemperatureProbeLoc (*(UINT8 *)&(Struct->Type28->LocationAndStatus), Option);
      PRINT_STRUCT_VALUE_H (Struct, Type28, MaximumValue);
      PRINT_STRUCT_VALUE_H (Struct, Type28, MinimumValue);
      PRINT_STRUCT_VALUE_H (Struct, Type28, Resolution);
      PRINT_STRUCT_VALUE_H (Struct, Type28, Tolerance);
      PRINT_STRUCT_VALUE_H (Struct, Type28, Accuracy);
      PRINT_STRUCT_VALUE_H (Struct, Type28, OEMDefined);
      PRINT_STRUCT_VALUE_H (Struct, Type28, NominalValue);
      break;

    //
    // Electrical Current Probe (Type 29)
    //
    case 29:
      PRINT_PENDING_STRING (Struct, Type29, Description);
      DisplayECPStatus (*(UINT8 *)&(Struct->Type29->LocationAndStatus), Option);
      DisplayECPLoc (*(UINT8 *)&(Struct->Type29->LocationAndStatus), Option);
      PRINT_STRUCT_VALUE_H (Struct, Type29, MaximumValue);
      PRINT_STRUCT_VALUE_H (Struct, Type29, MinimumValue);
      PRINT_STRUCT_VALUE_H (Struct, Type29, Resolution);
      PRINT_STRUCT_VALUE_H (Struct, Type29, Tolerance);
      PRINT_STRUCT_VALUE_H (Struct, Type29, Accuracy);
      PRINT_STRUCT_VALUE_H (Struct, Type29, OEMDefined);
      PRINT_STRUCT_VALUE_H (Struct, Type29, NominalValue);
      break;

    //
    // Out-of-Band Remote Access (Type 30)
    //
    case 30:
      PRINT_PENDING_STRING (Struct, Type30, ManufacturerName);
      DisplayOBRAConnections (Struct->Type30->Connections, Option);
      break;

    //
    // Boot Integrity Services (BIS) Entry Point (Type 31)
    //
    case 31:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_STRUCT_TYPE31), gShellDebug1HiiHandle);
      break;

    //
    // System Boot Information (Type 32)
    //
    case 32:
      PRINT_BIT_FIELD (Struct, Type32, Reserved, 6);
      DisplaySystemBootStatus (Struct->Type32->BootStatus, Option);
      break;

    //
    // 64-Bit Memory Error Information (Type 33)
    //
    case 33:
      DisplayMemoryErrorType (Struct->Type33->ErrorType, Option);
      DisplayMemoryErrorGranularity (
        Struct->Type33->ErrorGranularity,
        Option
        );
      DisplayMemoryErrorOperation (Struct->Type33->ErrorOperation, Option);
      PRINT_STRUCT_VALUE_H (Struct, Type33, VendorSyndrome);
      PRINT_STRUCT_VALUE_LH (Struct, Type33, MemoryArrayErrorAddress);
      PRINT_STRUCT_VALUE_LH (Struct, Type33, DeviceErrorAddress);
      PRINT_STRUCT_VALUE_H (Struct, Type33, ErrorResolution);
      break;

    //
    // Management Device (Type 34)
    //
    case 34:
      PRINT_PENDING_STRING (Struct, Type34, Description);
      DisplayMDType (Struct->Type34->Type, Option);
      PRINT_STRUCT_VALUE_H (Struct, Type34, Address);
      DisplayMDAddressType (Struct->Type34->AddressType, Option);
      break;

    //
    // Management Device Component (Type 35)
    //
    case 35:
      PRINT_PENDING_STRING (Struct, Type35, Description);
      PRINT_STRUCT_VALUE_H (Struct, Type35, ManagementDeviceHandle);
      PRINT_STRUCT_VALUE_H (Struct, Type35, ComponentHandle);
      PRINT_STRUCT_VALUE_H (Struct, Type35, ThresholdHandle);
      break;

    //
    // Management Device Threshold Data (Type 36)
    //
    case 36:
      PRINT_STRUCT_VALUE_H (Struct, Type36, LowerThresholdNonCritical);
      PRINT_STRUCT_VALUE_H (Struct, Type36, UpperThresholdNonCritical);
      PRINT_STRUCT_VALUE_H (Struct, Type36, LowerThresholdCritical);
      PRINT_STRUCT_VALUE_H (Struct, Type36, UpperThresholdCritical);
      PRINT_STRUCT_VALUE_H (Struct, Type36, LowerThresholdNonRecoverable);
      PRINT_STRUCT_VALUE_H (Struct, Type36, UpperThresholdNonRecoverable);
      break;

    //
    // Memory Channel (Type 37)
    //
    case 37:
    {
      UINT8          Count;
      MEMORY_DEVICE  *Ptr;
      DisplayMemoryChannelType (Struct->Type37->ChannelType, Option);
      PRINT_STRUCT_VALUE_H (Struct, Type37, MaximumChannelLoad);
      PRINT_STRUCT_VALUE_H (Struct, Type37, MemoryDeviceCount);

      Count = Struct->Type37->MemoryDeviceCount;
      Ptr   = Struct->Type37->MemoryDevice;
      for (Index = 0; Index < Count; Index++) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MEM_DEVICE), gShellDebug1HiiHandle, Index + 1);
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DEV_LOAD), gShellDebug1HiiHandle, Ptr[Index].DeviceLoad);
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DEV_HANDLE), gShellDebug1HiiHandle, Ptr[Index].DeviceHandle);
      }

      break;
    }

    //
    // IPMI Device Information (Type 38)
    //
    case 38:
      DisplayIPMIDIBMCInterfaceType (Struct->Type38->InterfaceType, Option);
      PRINT_STRUCT_VALUE_H (Struct, Type38, IPMISpecificationRevision);
      PRINT_STRUCT_VALUE_H (Struct, Type38, I2CSlaveAddress);
      PRINT_STRUCT_VALUE_H (Struct, Type38, NVStorageDeviceAddress);
      PRINT_STRUCT_VALUE_LH (Struct, Type38, BaseAddress);
      break;

    //
    // System Power Supply (Type 39)
    //
    case 39:
      PRINT_STRUCT_VALUE_H (Struct, Type39, PowerUnitGroup);
      PRINT_PENDING_STRING (Struct, Type39, Location);
      PRINT_PENDING_STRING (Struct, Type39, DeviceName);
      PRINT_PENDING_STRING (Struct, Type39, Manufacturer);
      PRINT_PENDING_STRING (Struct, Type39, SerialNumber);
      PRINT_PENDING_STRING (Struct, Type39, AssetTagNumber);
      PRINT_PENDING_STRING (Struct, Type39, ModelPartNumber);
      PRINT_PENDING_STRING (Struct, Type39, RevisionLevel);
      PRINT_STRUCT_VALUE_H (Struct, Type39, MaxPowerCapacity);
      DisplaySPSCharacteristics (
        *(UINT16 *)&(Struct->Type39->PowerSupplyCharacteristics),
        Option
        );
      PRINT_STRUCT_VALUE_H (Struct, Type39, InputVoltageProbeHandle);
      PRINT_STRUCT_VALUE_H (Struct, Type39, CoolingDeviceHandle);
      PRINT_STRUCT_VALUE_H (Struct, Type39, InputCurrentProbeHandle);
      break;

    //
    // Additional Information (Type 40)
    //
    case 40:
    {
      UINT8                         NumberOfEntries;
      UINT8                         EntryLength;
      ADDITIONAL_INFORMATION_ENTRY  *Entries;

      EntryLength     = 0;
      Entries         = Struct->Type40->AdditionalInfoEntries;
      NumberOfEntries = Struct->Type40->NumberOfAdditionalInformationEntries;

      PRINT_STRUCT_VALUE_H (Struct, Type40, NumberOfAdditionalInformationEntries);

      for (Index = 0; Index < NumberOfEntries; Index++) {
        EntryLength = Entries->EntryLength;
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_ENTRYLEN), gShellDebug1HiiHandle, EntryLength);
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_REFERENCEDHANDLE), gShellDebug1HiiHandle, Entries->ReferencedHandle);
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_REFERENCEDOFFSET), gShellDebug1HiiHandle, Entries->ReferencedOffset);
        PRINT_SMBIOS_STRING (Struct, Entries->EntryString, String);
        PRINT_SMBIOS_BIT_FIELD (Struct, Entries->Value, Value, EntryLength - 5);
        Entries = (ADDITIONAL_INFORMATION_ENTRY *)((UINT8 *)Entries + EntryLength);
      }

      break;
    }

    //
    // Onboard Devices Extended Information (Type 41)
    //
    case 41:
      PRINT_PENDING_STRING (Struct, Type41, ReferenceDesignation);
      ShellPrintEx (-1, -1, (((Struct->Type41->DeviceType) & 0x80) != 0) ? L"Device Enabled\n" : L"Device Disabled\n");
      DisplayOnboardDeviceTypes ((Struct->Type41->DeviceType) & 0x7F, Option);
      PRINT_STRUCT_VALUE_H (Struct, Type41, DeviceTypeInstance);
      PRINT_STRUCT_VALUE_H (Struct, Type41, SegmentGroupNum);
      PRINT_STRUCT_VALUE_H (Struct, Type41, BusNum);
      PRINT_STRUCT_VALUE_H (Struct, Type41, DevFuncNum);
      break;

    //
    // Management Controller Host Interface (Type 42)
    //
    case 42:
      DisplayMCHostInterfaceType (Struct->Type42->InterfaceType, Option);
      if (AE_SMBIOS_VERSION (0x3, 0x2)) {
        PRINT_STRUCT_VALUE_H (Struct, Type42, InterfaceTypeSpecificDataLength);
        PRINT_BIT_FIELD (Struct, Type42, InterfaceTypeSpecificData, Struct->Type42->InterfaceTypeSpecificDataLength);
      }

      break;

    //
    // TPM Device (Type 43)
    //
    case 43:
      PRINT_BIT_FIELD (Struct, Type43, VendorID, 4);
      PRINT_STRUCT_VALUE_H (Struct, Type43, MajorSpecVersion);
      PRINT_STRUCT_VALUE_H (Struct, Type43, MinorSpecVersion);
      PRINT_STRUCT_VALUE_H (Struct, Type43, FirmwareVersion1);
      PRINT_STRUCT_VALUE_H (Struct, Type43, FirmwareVersion2);
      PRINT_PENDING_STRING (Struct, Type43, Description);
      DisplayTpmDeviceCharacteristics (ReadUnaligned64 ((UINT64 *)(UINTN)&(Struct->Type43->Characteristics)), Option);
      PRINT_STRUCT_VALUE_H (Struct, Type43, OemDefined);
      break;

    //
    // Processor Additional Information (Type 44)
    //
    case 44:
      DisplayProcessorArchitectureType (Struct->Type44->ProcessorSpecificBlock.ProcessorArchType, Option);
      break;

    //
    // Inactive (Type 126)
    //
    case 126:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INACTIVE_STRUCT), gShellDebug1HiiHandle);
      break;

    //
    // End-of-Table (Type 127)
    //
    case 127:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_THIS_STRUCT_END_TABLE), gShellDebug1HiiHandle);
      break;

    default:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_STRUCT_TYPE_UNDEFINED), gShellDebug1HiiHandle);
      break;
  }

  return EFI_SUCCESS;
}

/**
  Display BIOS Information (Type 0) information.

  @param[in] Chara    The information bits.
  @param[in] Option   The optional information.
**/
VOID
DisplayBiosCharacteristics (
  IN UINT64  Chara,
  IN UINT8   Option
  )
{
  //
  // Print header
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_CHAR), gShellDebug1HiiHandle);
  //
  // print option
  //
  PRINT_INFO_OPTION (Chara, Option);

  //
  // Check all the bits and print information
  // This function does not use Table because table of bits
  //   are designed not to deal with UINT64
  //
  if (BIT (Chara, 0) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED_BIT), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 1) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED_BIT), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 2) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN_BIT), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 3) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_CHAR_NOT_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 4) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ISA_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 5) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MSA_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 6) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_EISA_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 7) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PCI_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 8) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PC_CARD_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 9) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PLUG_PLAY_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 10) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_APM_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 11) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_UPGRADEABLE), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 12) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_SHADOWING), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 13) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VESA_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 14) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ECSD_SUPPORT), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 15) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BOOT_FROM_CD_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 16) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SELECTED_BOOT_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 17) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_ROM_SOCKETED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 18) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BOOT_FROM_PC_CARD), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 19) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_EDD_ENHANCED_DRIVER), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 20) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_JAPANESE_FLOPPY_NEC), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 21) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_JAPANESE_FLOPPY_TOSHIBA), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 22) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_FLOPPY_SERVICES_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 23) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ONE_POINT_TWO_MB), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 24) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_720_KB), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 25) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TWO_POINT_EIGHT_EIGHT_MB), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 26) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PRINT_SCREEN_SUPPORT), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 27) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_KEYBOARD_SERV_SUPPORT), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 28) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SERIAL_SERVICES_SUPPORT), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 29) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PRINTER_SERVICES_SUPPORT), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 30) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MONO_VIDEO_SUPPORT), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 31) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NEC_PC_98), gShellDebug1HiiHandle);
  }

  //
  // Just print the Reserved
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_32_47), gShellDebug1HiiHandle);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_48_64), gShellDebug1HiiHandle);
}

/**
  Display Bios Characteristice extensions1 information.

  @param[in] Byte1    The information.
  @param[in] Option   The optional information.
**/
VOID
DisplayBiosCharacteristicsExt1 (
  IN UINT8  Byte1,
  IN UINT8  Option
  )
{
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_CHAR_EXTENSION), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (Byte1, Option);

  //
  // check bit and print
  //
  if (BIT (Byte1, 0) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ACPI_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Byte1, 1) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_USB_LEGACY_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Byte1, 2) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AGP_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Byte1, 3) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_I2O_BOOT_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Byte1, 4) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_LS_120_BOOT_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Byte1, 5) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ATAPI_ZIP_DRIVE), gShellDebug1HiiHandle);
  }

  if (BIT (Byte1, 6) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_1394_BOOT_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Byte1, 7) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SMART_BATTERY_SUPPORTED), gShellDebug1HiiHandle);
  }
}

/**
  Display Bios Characteristice extensions2 information.

  @param[in] byte2    The information.
  @param[in] Option   The optional information.
**/
VOID
DisplayBiosCharacteristicsExt2 (
  IN UINT8  byte2,
  IN UINT8  Option
  )
{
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_CHAR_EXTENSION_2), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (byte2, Option);

  if (BIT (byte2, 0) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_BOOT_SPEC_SUPP), gShellDebug1HiiHandle);
  }

  if (BIT (byte2, 1) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_FUNCTION_KEY_INIT), gShellDebug1HiiHandle);
  }

  if (AE_SMBIOS_VERSION (0x2, 0x4)) {
    if (BIT (byte2, 2) != 0) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENABLE_TAR_CONT_DIST), gShellDebug1HiiHandle);
    }

    if (AE_SMBIOS_VERSION (0x2, 0x7)) {
      if (BIT (byte2, 3) != 0) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UEFI_SPEC_SUPPORT), gShellDebug1HiiHandle);
      }

      if (BIT (byte2, 4) != 0) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VIRTUAL_MACHINE), gShellDebug1HiiHandle);
      }

      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_RSVD_FOR_FUTURE), gShellDebug1HiiHandle, 5);
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_RSVD_FOR_FUTURE), gShellDebug1HiiHandle, 3);
    }
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_RSVD_FOR_FUTURE), gShellDebug1HiiHandle, 2);
  }
}

/**
  Display Processor Information (Type 4) information.

  @param[in] Family       The family value.
  @param[in] Option       The option value.
**/
VOID
DisplayProcessorFamily (
  UINT8  Family,
  UINT8  Option
  )
{
  //
  // Print prompt message
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PROCESSOR_FAMILY), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (Family, Option);

  //
  // Use switch to check
  //
  switch (Family) {
    case 0x01:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OTHER), gShellDebug1HiiHandle);
      break;

    case 0x02:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
      break;

    case 0x03:
      Print (L"8086\n");
      break;

    case 0x04:
      Print (L"80286\n");
      break;

    case 0x05:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL386_PROCESSOR), gShellDebug1HiiHandle);
      break;

    case 0x06:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL486_PROCESSOR), gShellDebug1HiiHandle);
      break;

    case 0x07:
      Print (L"8087\n");
      break;

    case 0x08:
      Print (L"80287\n");
      break;

    case 0x09:
      Print (L"80387\n");
      break;

    case 0x0A:
      Print (L"80487\n");
      break;

    case 0x0B:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_PROC_FAMILY), gShellDebug1HiiHandle);
      break;

    case 0x0C:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_PRO_PROC), gShellDebug1HiiHandle);
      break;

    case 0x0D:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_II_PROC), gShellDebug1HiiHandle);
      break;

    case 0x0E:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_PROC_MMX), gShellDebug1HiiHandle);
      break;

    case 0x0F:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CELERON_PROC), gShellDebug1HiiHandle);
      break;

    case 0x10:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_XEON_PROC), gShellDebug1HiiHandle);
      break;

    case 0x11:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_III_PROC), gShellDebug1HiiHandle);
      break;

    case 0x12:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_M1_FAMILY), gShellDebug1HiiHandle);
      break;

    case 0x13:
      Print (L"M2 Family\n");
      break;

    case 0x14:
      Print (L"Intel Celeron M\n");
      break;

    case 0x15:
      Print (L"Intel Pentium 4 HT\n");
      break;

    case 0x18:
      Print (L"AMD Duron\n");
      break;

    case 0x19:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_K5_FAMILY), gShellDebug1HiiHandle);
      break;

    case 0x1A:
      Print (L"K6 Family\n");
      break;

    case 0x1B:
      Print (L"K6-2\n");
      break;

    case 0x1C:
      Print (L"K6-3\n");
      break;

    case 0x1D:
      Print (L"AMD Althon Processor Family\n");
      break;

    case 0x1E:
      Print (L"AMD 29000 Family\n");
      break;

    case 0x1F:
      Print (L"K6-2+\n");
      break;

    case 0x20:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_PC_FAMILY), gShellDebug1HiiHandle);
      break;

    case 0x21:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_PC_601), gShellDebug1HiiHandle);
      break;

    case 0x22:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_PC_603), gShellDebug1HiiHandle);
      break;

    case 0x23:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_PC_603_PLUS), gShellDebug1HiiHandle);
      break;

    case 0x24:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_PC_604), gShellDebug1HiiHandle);
      break;

    case 0x25:
      Print (L"Power PC 620\n");
      break;

    case 0x26:
      Print (L"Power PC 704\n");
      break;

    case 0x27:
      Print (L"Power PC 750\n");
      break;

    case 0x28:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE_DUO), gShellDebug1HiiHandle);
      break;

    case 0x29:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE_DUO_MOBILE), gShellDebug1HiiHandle);
      break;

    case 0x2A:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE_SOLO_MOBILE), gShellDebug1HiiHandle);
      break;

    case 0x2B:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_ATOM), gShellDebug1HiiHandle);
      break;

    case 0x2C:
      Print (L"Intel(R) Core(TM) M processor\n");
      break;

    case 0x2D:
      Print (L"Intel(R) Core(TM) m3 processor\n");
      break;

    case 0x2E:
      Print (L"Intel(R) Core(TM) m5 processor\n");
      break;

    case 0x2F:
      Print (L"Intel(R) Core(TM) m7 processor\n");
      break;

    case 0x30:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ALPHA_FAMILY_2), gShellDebug1HiiHandle);
      break;

    case 0x31:
      Print (L"Alpha 21064\n");
      break;

    case 0x32:
      Print (L"Alpha 21066\n");
      break;

    case 0x33:
      Print (L"Alpha 21164\n");
      break;

    case 0x34:
      Print (L"Alpha 21164PC\n");
      break;

    case 0x35:
      Print (L"Alpha 21164a\n");
      break;

    case 0x36:
      Print (L"Alpha 21264\n");
      break;

    case 0x37:
      Print (L"Alpha 21364\n");
      break;

    case 0x38:
      Print (L"AMD Turion II Ultra Dual-Core Mobile M Processor Family\n");
      break;

    case 0x39:
      Print (L"AMD Turion II Dual-Core Mobile M Processor Family\n");
      break;

    case 0x3A:
      Print (L"AMD Althon II Dual-Core M Processor Family\n");
      break;

    case 0x3B:
      Print (L"AMD Opteron 6100 Series Processor\n");
      break;

    case 0x3C:
      Print (L"AMD Opteron 4100 Series Processor\n");
      break;

    case 0x3D:
      Print (L"AMD Opteron 6200 Series Processor\n");
      break;

    case 0x3E:
      Print (L"AMD Opteron 4200 Series Processor\n");
      break;

    case 0x3F:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_FX_SERIES), gShellDebug1HiiHandle);
      break;

    case 0x40:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MIPS_FAMILY), gShellDebug1HiiHandle);
      break;

    case 0x41:
      Print (L"MIPS R4000\n");
      break;

    case 0x42:
      Print (L"MIPS R4200\n");
      break;

    case 0x43:
      Print (L"MIPS R4400\n");
      break;

    case 0x44:
      Print (L"MIPS R4600\n");
      break;

    case 0x45:
      Print (L"MIPS R10000\n");
      break;

    case 0x46:
      Print (L"AMD C-Series Processor\n");
      break;

    case 0x47:
      Print (L"AMD E-Series Processor\n");
      break;

    case 0x48:
      Print (L"AMD A-Series Processor\n");
      break;

    case 0x49:
      Print (L"AMD G-Series Processor\n");
      break;

    case 0x4A:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_Z_SERIES), gShellDebug1HiiHandle);
      break;

    case 0x4B:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_R_SERIES), gShellDebug1HiiHandle);
      break;

    case 0x4C:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_OPTERON_4300_SERIES), gShellDebug1HiiHandle);
      break;

    case 0x4D:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_OPTERON_6300_SERIES), gShellDebug1HiiHandle);
      break;

    case 0x4E:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_OPTERON_3300_SERIES), gShellDebug1HiiHandle);
      break;

    case 0x4F:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_OPTERON_FIREPRO_SERIES), gShellDebug1HiiHandle);
      break;

    case 0x50:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SPARC_FAMILY), gShellDebug1HiiHandle);
      break;

    case 0x51:
      Print (L"SuperSparc\n");
      break;

    case 0x52:
      Print (L"microSparc II\n");
      break;

    case 0x53:
      Print (L"microSparc IIep\n");
      break;

    case 0x54:
      Print (L"UltraSparc\n");
      break;

    case 0x55:
      Print (L"UltraSparc II\n");
      break;

    case 0x56:
      Print (L"UltraSparcIIi\n");
      break;

    case 0x57:
      Print (L"UltraSparcIII\n");
      break;

    case 0x58:
      Print (L"UltraSparcIIIi\n");
      break;

    case 0x60:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_68040_FAMILY), gShellDebug1HiiHandle);
      break;

    case 0x61:
      Print (L"68xx\n");
      break;

    case 0x62:
      Print (L"68000\n");
      break;

    case 0x63:
      Print (L"68010\n");
      break;

    case 0x64:
      Print (L"68020\n");
      break;

    case 0x65:
      Print (L"68030\n");
      break;

    case 0x66:
      Print (L"AMD Athlon(TM) X4 Quad-Core Processor Family\n");
      break;

    case 0x67:
      Print (L"AMD Opteron(TM) X1000 Series Processor\n");
      break;

    case 0x68:
      Print (L"AMD Opteron(TM) X2000 Series APU\n");
      break;

    case 0x69:
      Print (L"AMD Opteron(TM) A-Series Processor\n");
      break;

    case 0x6A:
      Print (L"AMD Opteron(TM) X3000 Series APU\n");
      break;

    case 0x6B:
      Print (L"AMD Zen Processor Family\n");
      break;

    case 0x70:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_HOBBIT_FAMILY), gShellDebug1HiiHandle);
      break;

    case 0x78:
      Print (L"Crusoe TM5000\n");
      break;

    case 0x79:
      Print (L"Crusoe TM3000\n");
      break;

    case 0x7A:
      Print (L"Efficeon TM8000\n");
      break;

    case 0x80:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_WEITEK), gShellDebug1HiiHandle);
      break;

    case 0x82:
      Print (L"Itanium\n");
      break;

    case 0x83:
      Print (L"AMD Athlon64\n");
      break;

    case 0x84:
      Print (L"AMD Opteron\n");
      break;

    case 0x85:
      Print (L"AMD Sempron\n");
      break;

    case 0x86:
      Print (L"AMD Turion64 Mobile\n");
      break;

    case 0x87:
      Print (L"Dual-Core AMD Opteron\n");
      break;

    case 0x88:
      Print (L"AMD Athlon 64X2 DualCore\n");
      break;

    case 0x89:
      Print (L"AMD Turion 64X2 Mobile\n");
      break;

    case 0x8A:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_OPTERON_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0x8B:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_OPTERON_THIRD_GENERATION), gShellDebug1HiiHandle);
      break;

    case 0x8C:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_PHENOM_FX_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0x8D:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_PHENOM_X4_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0x8E:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_PHENOM_X2_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0x8F:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_ATHLON_X2_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0x90:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PA_RISC_FAMILY), gShellDebug1HiiHandle);
      break;

    case 0x91:
      Print (L"PA-RISC 8500\n");
      break;

    case 0x92:
      Print (L"PA-RISC 8000\n");
      break;

    case 0x93:
      Print (L"PA-RISC 7300LC\n");
      break;

    case 0x94:
      Print (L"PA-RISC 7200\n");
      break;

    case 0x95:
      Print (L"PA-RISC 7100LC\n");
      break;

    case 0x96:
      Print (L"PA-RISC 7100\n");
      break;

    case 0xA0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_V30_FAMILY), gShellDebug1HiiHandle);
      break;

    case 0xA1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_3200_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0xA2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_3000_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xA3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5300_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0xA4:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5100_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xA5:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5000_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xA6:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_LV_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xA7:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_ULV_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xA8:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7100_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xA9:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5400_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0xAA:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0xAB:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5200_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xAC:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7200_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xAD:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7300_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0xAE:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7400_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0xAF:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7400_SERIES_MULTI_CORE), gShellDebug1HiiHandle);
      break;

    case 0xB0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_III_XEON), gShellDebug1HiiHandle);
      break;

    case 0xB1:
      Print (L"Pentium III Processorwith Intel SpeedStep Technology\n");
      break;

    case 0xB2:
      Print (L"Pentium 4 processor\n");
      break;

    case 0xB3:
      Print (L"Intel Xeon Processor\n");
      break;

    case 0xB4:
      Print (L"AS400 Family\n");
      break;

    case 0xB5:
      Print (L"Intel Xeon processor MP\n");
      break;

    case 0xB6:
      Print (L"AMD Althon XP Processor Family\n");
      break;

    case 0xB7:
      Print (L"AMD Althon MP Promcessor Family\n");
      break;

    case 0xB8:
      Print (L"Intel Itanium 2 processor\n");
      break;

    case 0xB9:
      Print (L"Intel Penium M processor\n");
      break;

    case 0xBA:
      Print (L"Intel Celeron D processor\n");
      break;

    case 0xBB:
      Print (L"Intel Pentium D processor\n");
      break;

    case 0xBC:
      Print (L"Intel Pentium Processor Extreme Edition\n");
      break;

    case 0xBD:
      Print (L"Intel Core Solo Processor\n");
      break;

    case 0xBF:
      Print (L"Intel Core 2 Duo Processor\n");
      break;

    case 0xC0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE2_SOLO), gShellDebug1HiiHandle);
      break;

    case 0xC1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE2_EXTREME), gShellDebug1HiiHandle);
      break;

    case 0xC2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE2_QUAD), gShellDebug1HiiHandle);
      break;

    case 0xC3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE2_EXTREME), gShellDebug1HiiHandle);
      break;

    case 0xC4:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE2_DUO_MOBILE), gShellDebug1HiiHandle);
      break;

    case 0xC5:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE2_SOLO_MOBILE), gShellDebug1HiiHandle);
      break;

    case 0xC6:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE_I7), gShellDebug1HiiHandle);
      break;

    case 0xC7:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CELERON_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xC8:
      Print (L"IBM 390\n");
      break;

    case 0xC9:
      Print (L"G4\n");
      break;

    case 0xCA:
      Print (L"G5\n");
      break;

    case 0xCB:
      Print (L"G6\n");
      break;

    case 0xCC:
      Print (L"zArchitecture\n");
      break;

    case 0xCD:
      Print (L"Intel Core i5 processor\n");
      break;

    case 0xCE:
      Print (L"Intel Core i3 processor\n");
      break;

    case 0xCF:
      Print (L"Intel Core i9 processor\n");
      break;

    case 0xD2:
      Print (L"ViaC7M\n");
      break;

    case 0xD3:
      Print (L"ViaC7D\n");
      break;

    case 0xD4:
      Print (L"ViaC7\n");
      break;

    case 0xD5:
      Print (L"Eden\n");
      break;

    case 0xD6:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_MULTI_CORE), gShellDebug1HiiHandle);
      break;

    case 0xD7:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_3_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xD8:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_3_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0xDA:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xDB:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0xDD:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xDE:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0xDF:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7_SERIES_MULTI_CORE), gShellDebug1HiiHandle);
      break;

    case 0xE0:
      Print (L"Multi-Core Intel Xeon processor 3400 Series\n");
      break;

    case 0xE4:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_OPTERON_3000_SERIES), gShellDebug1HiiHandle);
      break;

    case 0xE5:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_SEMPRON_II), gShellDebug1HiiHandle);
      break;

    case 0xE6:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_EMBEDDED_OPTERON_QUAD_CORE), gShellDebug1HiiHandle);
      break;

    case 0xE7:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_PHENOM_TRIPLE_CORE), gShellDebug1HiiHandle);
      break;

    case 0xE8:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_TURION_ULTRA_DUAL_CORE_MOBILE), gShellDebug1HiiHandle);
      break;

    case 0xE9:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_TURION_DUAL_CORE_MOBILE), gShellDebug1HiiHandle);
      break;

    case 0xEA:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_ATHLON_DUAL_CORE), gShellDebug1HiiHandle);
      break;

    case 0xEB:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_SEMPRON_SI), gShellDebug1HiiHandle);
      break;

    case 0xEC:
      Print (L"AMD Phenom II Processor Family\n");
      break;

    case 0xED:
      Print (L"AMD Althon II Processor Family\n");
      break;

    case 0xEE:
      Print (L"Six-Core AMD Opteron Processor Family\n");
      break;

    case 0xEF:
      Print (L"AMD Sempron M Processor Family\n");
      break;

    case 0xFA:
      Print (L"i860\n");
      break;

    case 0xFB:
      Print (L"i960\n");
      break;

    default:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNDEFINED_PROC_FAMILY), gShellDebug1HiiHandle);
  }

  //
  // end switch
  //
}

/**
  Display processor family information.

  @param[in] Family2      The family value.
  @param[in] Option       The option value.
**/
VOID
DisplayProcessorFamily2 (
  IN UINT16  Family2,
  IN UINT8   Option
  )
{
  //
  // Print prompt message
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PROCESSOR_FAMILY), gShellDebug1HiiHandle);

  //
  // Print option
  //
  PRINT_INFO_OPTION (Family2, Option);

  //
  // Use switch to check
  //
  switch (Family2) {
    case 0x100:
      Print (L"ARMv7\n");
      break;

    case 0x101:
      Print (L"ARMv8\n");
      break;

    case 0x104:
      Print (L"SH-3\n");
      break;

    case 0x105:
      Print (L"SH-4\n");
      break;

    case 0x118:
      Print (L"ARM\n");
      break;

    case 0x119:
      Print (L"StrongARM\n");
      break;

    case 0x12C:
      Print (L"6x86\n");
      break;

    case 0x12D:
      Print (L"MediaGX\n");
      break;

    case 0x12E:
      Print (L"MII\n");
      break;

    case 0x140:
      Print (L"WinChip\n");
      break;

    case 0x15E:
      Print (L"DSP\n");
      break;

    case 0x1F4:
      Print (L"Video Processor\n");
      break;

    case 0x200:
      Print (L"RISC-V RV32\n");
      break;

    case 0x201:
      Print (L"RISC-V RV64\n");
      break;

    case 0x202:
      Print (L"RISC-V RV128\n");
      break;

    default:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNDEFINED_PROC_FAMILY), gShellDebug1HiiHandle);
  }
}

/**
  Display processor voltage information.

  @param[in] Voltage      The Voltage.
  Bit 7 Set to 0, indicating 'legacy' mode for processor voltage
  Bits 6:4  Reserved, must be zero
  Bits 3:0  Voltage Capability.
            A Set bit indicates that the voltage is supported.
    Bit 0 - 5V
    Bit 1 - 3.3V
    Bit 2 - 2.9V
    Bit 3 - Reserved, must be zero.

  Note:
    Setting of multiple bits indicates the socket is configurable
    If bit 7 is set to 1, the remaining seven bits of the field are set to
    contain the processor's current voltage times 10.
    For example, the field value for a processor voltage of 1.8 volts would be
    92h = 80h + (1.8 * 10) = 80h + 18 = 80h +12h.

  @param[in] Option       The option.
**/
VOID
DisplayProcessorVoltage (
  IN UINT8  Voltage,
  IN UINT8  Option
  )
{
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PROC_INFO), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (Voltage, Option);

  if (BIT (Voltage, 7) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PROC_CURRENT_VOLTAGE), gShellDebug1HiiHandle, (Voltage - 0x80));
  } else {
    if (BIT (Voltage, 0) != 0) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_5V_SUPOPRTED), gShellDebug1HiiHandle);
    }

    if (BIT (Voltage, 1) != 0) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_33V_SUPPORTED), gShellDebug1HiiHandle);
    }

    if (BIT (Voltage, 2) != 0) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_29V_SUPPORTED), gShellDebug1HiiHandle);
    }

    //
    // check the reserved zero bits:
    //
    if (BIT (Voltage, 3) != 0) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT3_NOT_ZERO), gShellDebug1HiiHandle);
    }

    if (BIT (Voltage, 4) != 0) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT4_NOT_ZERO), gShellDebug1HiiHandle);
    }

    if (BIT (Voltage, 5) != 0) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT5_NOT_ZERO), gShellDebug1HiiHandle);
    }

    if (BIT (Voltage, 6) != 0) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT6_NOT_ZERO), gShellDebug1HiiHandle);
    }
  }
}

/**
  Display processor information.

  @param[in] Status   The status.
Bit 7 Reserved, must be 0
Bit 6 CPU Socket Populated
  1 - CPU Socket Populated
  0 - CPU Socket Unpopulated
Bits 5:3 Reserved, must be zero
Bits 2:0 CPU Status
  0h - Unknown
  1h - CPU Enabled
  2h - CPU Disabled by User via BIOS Setup
  3h - CPU Disabled By BIOS (POST Error)
  4h - CPU is Idle, waiting to be enabled.
  5-6h - Reserved
  7h - Other

  @param[in] Option   The option
**/
VOID
DisplayProcessorStatus (
  IN UINT8  Status,
  IN UINT8  Option
  )
{
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PROC_STATUS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Status, Option);

  if (BIT (Status, 7) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT7_NOT_ZERO), gShellDebug1HiiHandle);
  } else if (BIT (Status, 5) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT5_NOT_ZERO), gShellDebug1HiiHandle);
  } else if (BIT (Status, 4) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT4_NOT_ZERO), gShellDebug1HiiHandle);
  } else if (BIT (Status, 3) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT3_NOT_ZERO), gShellDebug1HiiHandle);
  }

  //
  // Check BIT 6
  //
  if (BIT (Status, 6) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CPU_SOCKET_POPULATED), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CPU_SOCKET_UNPOPULATED), gShellDebug1HiiHandle);
  }

  //
  // Check BITs 2:0
  //
  switch (Status & 0x07) {
    case 0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
      break;

    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CPU_ENABLED), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CPU_DISABLED_BY_USER), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CPU_DIABLED_BY_BIOS), gShellDebug1HiiHandle);
      break;

    case 4:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CPU_IDLE), gShellDebug1HiiHandle);
      break;

    case 7:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OTHERS), gShellDebug1HiiHandle);
      break;

    default:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED), gShellDebug1HiiHandle);
  }
}

/**
  Display information about Memory Controller Information (Type 5).

  @param[in] Size     Memory size.
  @param[in] SlotNum  Which slot is this about.
  @param[in] Option   Option for the level of detail output required.
**/
VOID
DisplayMaxMemoryModuleSize (
  IN UINT8  Size,
  IN UINT8  SlotNum,
  IN UINT8  Option
  )
{
  UINTN  MaxSize;

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SIZE_LARGEST_MEM), gShellDebug1HiiHandle);
  //
  // MaxSize is determined by follow formula
  //
  MaxSize = (UINTN)1 << Size;
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ONE_VAR_MB), gShellDebug1HiiHandle, MaxSize);

  if (Option >= SHOW_DETAIL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MAX_AMOUNT_MEM), gShellDebug1HiiHandle);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ONE_VAR_MB), gShellDebug1HiiHandle, MaxSize, SlotNum, MaxSize * SlotNum);
  }
}

/**
  Display information about memory configuration handles.

  @param[in] Handles  The buffer of handles to output info on.
  @param[in] SlotNum  The number of handles in the above buffer.
  @param[in] Option   Option for the level of detail output required.
**/
VOID
DisplayMemoryModuleConfigHandles (
  IN UINT16  *Handles,
  IN UINT8   SlotNum,
  IN UINT8   Option
  )
{
  UINT8  Index;

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_HANDLES_CONTROLLED), gShellDebug1HiiHandle, SlotNum);

  if (Option >= SHOW_DETAIL) {
    //
    // No handle, Handles is INVALID.
    //
    if (SlotNum == 0) {
      return;
    }

    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_HANDLES_LIST_CONTROLLED), gShellDebug1HiiHandle);
    for (Index = 0; Index < SlotNum; Index++) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_HANDLE), gShellDebug1HiiHandle, Index + 1, Handles[Index]);
    }
  }
}

/**
  Display Memory Module Information (Type 6).

  @param[in] BankConnections
  @param[in] Option
**/
VOID
DisplayMmBankConnections (
  IN UINT8  BankConnections,
  IN UINT8  Option
  )
{
  UINT8  High;
  UINT8  Low;

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BANK_CONNECTIONS), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (BankConnections, Option);

  //
  // Divide it to high and low
  //
  High = (UINT8)(BankConnections & 0xF0);
  Low  = (UINT8)(BankConnections & 0x0F);
  if (High != 0xF) {
    if (Low != 0xF) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BANK_RAS), gShellDebug1HiiHandle, High, Low, High, Low);
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BANK_RAS_2), gShellDebug1HiiHandle, High, High);
    }
  } else {
    if (Low != 0xF) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BANK_RAS_2), gShellDebug1HiiHandle, Low, Low);
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NO_BANKS_CONNECTED), gShellDebug1HiiHandle);
    }
  }
}

/**
  Display memory informcation.

  Bits 0:6  Size (n),
      where 2**n is the size in MB with three special-case values:
      7Dh Not determinable (Installed Size only)
      7Eh Module is installed, but no memory has been enabled
      7Fh Not installed
  Bit  7  Defines whether the memory module has a single- (0)
          or double-bank (1) connection.

  @param[in] Size   - The size
  @param[in] Option - The option
**/
VOID
DisplayMmMemorySize (
  IN UINT8  Size,
  IN UINT8  Option
  )
{
  UINT8  Value;

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MEMORY_SIZE), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (Size, Option);

  //
  // Get the low bits(0-6 bit)
  //
  Value = (UINT8)(Size & 0x7F);
  if (Value == 0x7D) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MEM_SIZE_NOT_DETERMINABLE), gShellDebug1HiiHandle);
  } else if (Value == 0x7E) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MODULE_INSTALLED), gShellDebug1HiiHandle);
  } else if (Value == 0x7F) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NOT_INSTALLED), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MEM_SIZE), gShellDebug1HiiHandle, 1 << Value);
  }

  if (BIT (Size, 7) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MEM_MODULE_DOUBLE_BANK), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MEM_MODULE_SINGLE_BANK), gShellDebug1HiiHandle);
  }
}

/**
  Display Cache Configuration.

  @param[in] CacheConfiguration   Cache Configuration.
Bits 15:10 Reserved, must be 0
Bits 9:8 Operational Mode
  0h - Write Through
  1h - Write Back
  2h - Varies with Memory Address
  3h - Unknown
Bit 7 Enabled/Disabled
  1 - Enabled
  0 - Disabled
Bits 6:5 Location
  0h - Internal
  1h - External
  2h - Reserved
  3h - Unknown
Bit 4 Reserved, must be zero
Bit 3 Cache Socketed
  1 - Socketed
  0 - Unsocketed
Bits 2:0 Cache Level
  1 through 8 (For example, an L1 cache would
  use value 000b and an L3 cache would use 010b.)

  @param[in] Option   The option
**/
VOID
DisplayCacheConfiguration (
  IN UINT16  CacheConfiguration,
  IN UINT8   Option
  )
{
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CACHE_CONFIGURATION), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (CacheConfiguration, Option);

  if (BIT (CacheConfiguration, 15) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT15_NOT_ZERO), gShellDebug1HiiHandle);
  } else if (BIT (CacheConfiguration, 14) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT14_NOT_ZERO), gShellDebug1HiiHandle);
  } else if (BIT (CacheConfiguration, 13) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT13_NOT_ZERO), gShellDebug1HiiHandle);
  } else if (BIT (CacheConfiguration, 12) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT12_NOT_ZERO), gShellDebug1HiiHandle);
  } else if (BIT (CacheConfiguration, 11) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT11_NOT_ZERO), gShellDebug1HiiHandle);
  } else if (BIT (CacheConfiguration, 10) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT10_NOT_ZERO), gShellDebug1HiiHandle);
  } else if (BIT (CacheConfiguration, 4) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT4_NOT_ZERO), gShellDebug1HiiHandle);
  }

  //
  // Check BITs 9:8
  //
  switch ((CacheConfiguration & 0x300) >> 8) {
    case 0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CACHE_WRITE_THROUGH), gShellDebug1HiiHandle);
      break;

    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CACHE_WRITE_BACK), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CACHE_VARIES_WITH_MEM_ADDR), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
      break;
  }

  //
  // Check BIT 7
  //
  if (BIT (CacheConfiguration, 7) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENABLED), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DISABLED), gShellDebug1HiiHandle);
  }

  //
  // Check BITs 6:5
  //
  switch ((CacheConfiguration & 0x60) >> 5) {
    case 0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CACHE_INTERNAL), gShellDebug1HiiHandle);
      break;

    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CACHE_EXTERNAL), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
      break;
  }

  //
  // Check BIT 3
  //
  if (BIT (CacheConfiguration, 3) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CACHE_SOCKETED), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CACHE_NOT_SOCKETED), gShellDebug1HiiHandle);
  }

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CACHE_LEVEL), gShellDebug1HiiHandle, (CacheConfiguration & 0x07) + 1);
}

/**
  The Slot ID field of the System Slot structure provides a mechanism to
  correlate the physical attributes of the slot to its logical access method
  (which varies based on the Slot Type field).

  @param[in] SlotId   - The slot ID
  @param[in] SlotType - The slot type
  @param[in] Option   - The Option
**/
VOID
DisplaySystemSlotId (
  IN UINT16  SlotId,
  IN UINT8   SlotType,
  IN UINT8   Option
  )
{
  //
  // Display slot type first
  //
  DisplaySystemSlotType (SlotType, Option);

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SLOT_ID), gShellDebug1HiiHandle);
  //
  // print option
  //
  PRINT_INFO_OPTION (SlotType, Option);

  switch (SlotType) {
    //
    // Slot Type: MCA
    //
    case 0x04:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_LOGICAL_MICRO_CHAN), gShellDebug1HiiHandle);
      if ((SlotId > 0) && (SlotId < 15)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ONE_VAR_D), gShellDebug1HiiHandle, SlotId);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ERROR_NOT_1_15), gShellDebug1HiiHandle);
      }

      break;

    //
    // EISA
    //
    case 0x05:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_LOGICAL_EISA_NUM), gShellDebug1HiiHandle);
      if ((SlotId > 0) && (SlotId < 15)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ONE_VAR_D), gShellDebug1HiiHandle, SlotId);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ERROR_NOT_1_15), gShellDebug1HiiHandle);
      }

      break;

    //
    // Slot Type: PCI
    //
    case 0x06:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VALUE_PRESENT), gShellDebug1HiiHandle, SlotId);
      break;

    //
    // PCMCIA
    //
    case 0x07:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_IDENTIFIES_ADAPTER_NUM), gShellDebug1HiiHandle, SlotId);
      break;

    //
    // Slot Type: PCI-E
    //
    case 0xA5:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VALUE_PRESENT), gShellDebug1HiiHandle, SlotId);
      break;

    default:
      if (((SlotType >= 0x0E) && (SlotType <= 0x12)) || ((SlotType >= 0xA6) && (SlotType <= 0xB6))) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VALUE_PRESENT), gShellDebug1HiiHandle, SlotId);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNDEFINED_SLOT_ID), gShellDebug1HiiHandle);
      }
  }
}

/**
  Display System Boot Information (Type 32) information.

  @param[in] Parameter      The parameter.
  @param[in] Option         The options.
**/
VOID
DisplaySystemBootStatus (
  IN UINT8  Parameter,
  IN UINT8  Option
  )
{
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SYSTEM_BOOT_STATUS), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (Parameter, Option);

  //
  // Check value and print
  //
  if (Parameter == 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NO_ERRORS_DETECTED), gShellDebug1HiiHandle);
  } else if (Parameter == 1) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NO_BOOTABLE_MEDIA), gShellDebug1HiiHandle);
  } else if (Parameter == 2) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NORMAL_OP_SYSTEM), gShellDebug1HiiHandle);
  } else if (Parameter == 3) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_FIRMWARE_DETECTED), gShellDebug1HiiHandle);
  } else if (Parameter == 4) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OP_SYSTEM), gShellDebug1HiiHandle);
  } else if (Parameter == 5) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_USER_REQUESTED_BOOT), gShellDebug1HiiHandle);
  } else if (Parameter == 6) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SYSTEM_SECURITY_VIOLATION), gShellDebug1HiiHandle);
  } else if (Parameter == 7) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PREV_REQ_IMAGE), gShellDebug1HiiHandle);
  } else if (Parameter == 8) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_WATCHDOG_TIMER), gShellDebug1HiiHandle);
  } else if ((Parameter >= 9) && (Parameter <= 127)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RSVD_FUTURE_ASSIGNMENT), gShellDebug1HiiHandle);
  } else if ((Parameter >= 128) && (Parameter <= 191)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VENDOR_OEM_SPECIFIC), gShellDebug1HiiHandle);
  } else if (Parameter >= 192) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PRODUCT_SPEC_IMPLMENTATION), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ERROR_VALUE), gShellDebug1HiiHandle);
  }
}

/**
  Display Portable Battery (Type 22) information.

  The date the cell pack was manufactured, in packed format:
   Bits 15:9  Year, biased by 1980, in the range 0 to 127.
   Bits 8:5 Month, in the range 1 to 12.
   Bits 4:0 Date, in the range 1 to 31.
  For example, 01 February 2000 would be identified as
  0010 1000 0100 0001b (0x2841).

  @param[in] Date     The date
  @param[in] Option   The option
**/
VOID
DisplaySBDSManufactureDate (
  IN UINT16  Date,
  IN UINT8   Option
  )
{
  UINTN  Day;
  UINTN  Month;
  UINTN  Year;

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SBDS_MANUFACTURE_DATE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Date, Option);
  //
  // Print date
  //
  Day   = Date & 0x001F;
  Month = (Date & 0x01E0) >> 5;
  Year  = ((Date & 0xFE00) >> 9) + 1980;
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MONTH_DAY_YEAR), gShellDebug1HiiHandle, Day, Month, Year);
}

/**
  Display System Reset (Type 23) information.


Identifies the system-reset capabilities for the system.
 Bits 7:6 Reserved for future assignment via this specification, set to 00b.
 Bit 5  System contains a watchdog timer, either True (1) or False (0).
 Bits 4:3 Boot Option on Limit.
  Identifies the system action to be taken when the Reset Limit is reached, one of:
  00b Reserved, do not use.
  01b Operating system
  10b System utilities
  11b Do not rebootBits
 2:1  Boot Option.  Indicates the action to be taken following a watchdog reset, one of:
  00b Reserved, do not use.
  01b Operating system
  10b System utilities
  11b Do not reboot
 Bit 0  Status.
  1b The system reset is enabled by the user
  0b The system reset is not enabled by the user

  @param[in] Reset   Reset
  @param[in] Option  The option
**/
VOID
DisplaySystemResetCapabilities (
  IN UINT8  Reset,
  IN UINT8  Option
  )
{
  UINTN  Temp;

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SYSTEM_RESET_CAPABILITIES), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Reset, Option);

  //
  // Check reserved bits 7:6
  //
  if ((Reset & 0xC0) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_RESERVED_ZERO), gShellDebug1HiiHandle);
  }

  //
  // Watch dog
  //
  if (BIT (Reset, 5) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_WATCHDOG_TIMER_2), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SYSTEM_NOT_CONTAIN_TIMER), gShellDebug1HiiHandle);
  }

  //
  // Boot Option on Limit
  //
  Temp = (Reset & 0x18) >> 3;
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BOOT_OPTION_LIMIT), gShellDebug1HiiHandle);
  switch (Temp) {
    case 0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED), gShellDebug1HiiHandle);
      break;

    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OP_SYSTEM_2), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SYSTEM_UTIL), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DO_NOT_REBOOT), gShellDebug1HiiHandle);
      break;
  }

  //
  // Boot Option
  //
  Temp = (Reset & 0x06) >> 1;
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BOOT_OPTION), gShellDebug1HiiHandle);
  switch (Temp) {
    case 0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED), gShellDebug1HiiHandle);
      break;

    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OP_SYSTEM_2), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SYSTEM_UTIL), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DO_NOT_REBOOT), gShellDebug1HiiHandle);
      break;
  }

  //
  // Reset enable flag
  //
  if ((Reset & 0x01) != 0) {
    Print (L"The system reset is enabled by the user\n");
  } else {
    Print (L"The system reset is disabled by the user\n");
  }
}

/**
  Display Hardware Security (Type 24) information.


Identifies the password and reset status for the system:

Bits 7:6    Power-on Password Status, one of:
  00b Disabled
  01b Enabled
  10b Not Implemented
  11b Unknown
Bits 5:4    Keyboard Password Status, one of:
  00b Disabled
  01b Enabled
  10b Not Implemented
  11b Unknown
Bits 3:2    Administrator Password Status, one  of:
  00b Disabled
  01b Enabled
  10b Not Implemented
  11b Unknown
Bits 1:0    Front Panel Reset Status, one of:
  00b Disabled
  01b Enabled
  10b Not Implemented
  11b Unknown

  @param[in] Settings The device settings.
  @param[in] Option   The device options.
**/
VOID
DisplayHardwareSecuritySettings (
  IN UINT8  Settings,
  IN UINT8  Option
  )
{
  UINTN  Temp;

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_HARDWARE_SECURITY_SET), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Settings, Option);

  //
  // Power-on Password Status
  //
  Temp = (Settings & 0xC0) >> 6;
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_ON_PASSWORD), gShellDebug1HiiHandle);
  switch (Temp) {
    case 0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DISABLED), gShellDebug1HiiHandle);
      break;

    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENABLED_NEWLINE), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NOT_IMPLEMENTED), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
      break;
  }

  //
  // Keyboard Password Status
  //
  Temp = (Settings & 0x30) >> 4;
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_KEYBOARD_PASSWORD), gShellDebug1HiiHandle);
  switch (Temp) {
    case 0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DISABLED), gShellDebug1HiiHandle);
      break;

    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENABLED_NEWLINE), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NOT_IMPLEMENTED), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
      break;
  }

  //
  // Administrator Password Status
  //
  Temp = (Settings & 0x0C) >> 2;
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ADMIN_PASSWORD_STATUS), gShellDebug1HiiHandle);
  switch (Temp) {
    case 0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DISABLED), gShellDebug1HiiHandle);
      break;

    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENABLED_NEWLINE), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NOT_IMPLEMENTED), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
      break;
  }

  //
  // Front Panel Reset Status
  //
  Temp = Settings & 0x3;
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_FRONT_PANEL_RESET), gShellDebug1HiiHandle);
  switch (Temp) {
    case 0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DISABLED), gShellDebug1HiiHandle);
      break;

    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENABLED_NEWLINE), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NOT_IMPLEMENTED), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
      break;
  }
}

/**
  Display Out-of-Band Remote Access (Type 30) information.

  @param[in] Connections        The device characteristics.
  @param[in] Option             The device options.
**/
VOID
DisplayOBRAConnections (
  IN UINT8  Connections,
  IN UINT8  Option
  )
{
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CONNECTIONS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Connections, Option);

  //
  // Check reserved bits 7:2
  //
  if ((Connections & 0xFC) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_RESERVED_ZERO_2), gShellDebug1HiiHandle);
  }

  //
  // Outbound Connection
  //
  if (BIT (Connections, 1) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OUTBOUND_CONN_ENABLED), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OTUBOUND_CONN_DISABLED), gShellDebug1HiiHandle);
  }

  //
  // Inbound Connection
  //
  if (BIT (Connections, 0) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INBOIUND_CONN_ENABLED), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INBOUND_CONN_DISABLED), gShellDebug1HiiHandle);
  }
}

/**
  Display System Power Supply (Type 39) information.

  @param[in] Characteristics    The device characteristics.
  @param[in] Option             The device options.
**/
VOID
DisplaySPSCharacteristics (
  IN UINT16  Characteristics,
  IN UINT8   Option
  )
{
  UINTN  Temp;

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_CHAR), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Characteristics, Option);

  //
  // Check reserved bits 15:14
  //
  if ((Characteristics & 0xC000) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_15_14_RSVD), gShellDebug1HiiHandle);
  }

  //
  // Bits 13:10 - DMTF Power Supply Type
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TYPE), gShellDebug1HiiHandle);
  Temp = (Characteristics & 0x1C00) >> 10;
  switch (Temp) {
    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OTHER_SPACE), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_LINEAR), gShellDebug1HiiHandle);
      break;

    case 4:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SWITCHING), gShellDebug1HiiHandle);
      break;

    case 5:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BATTERY), gShellDebug1HiiHandle);
      break;

    case 6:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UPS), gShellDebug1HiiHandle);
      break;

    case 7:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CONVERTER), gShellDebug1HiiHandle);
      break;

    case 8:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_REGULATOR), gShellDebug1HiiHandle);
      break;

    default:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED_2), gShellDebug1HiiHandle);
  }

  //
  // Bits 9:7 - Status
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_STATUS_DASH), gShellDebug1HiiHandle);
  Temp = (Characteristics & 0x380) >> 7;
  switch (Temp) {
    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OTHER_SPACE), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OK), gShellDebug1HiiHandle);
      break;

    case 4:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NON_CRITICAL), gShellDebug1HiiHandle);
      break;

    case 5:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CRITICAL_POWER_SUPPLY), gShellDebug1HiiHandle);
      break;

    default:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNDEFINED), gShellDebug1HiiHandle);
  }

  //
  // Bits 6:3 - DMTF Input Voltage Range Switching
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INPUT_VOLTAGE_RANGE), gShellDebug1HiiHandle);
  Temp = (Characteristics & 0x78) >> 3;
  switch (Temp) {
    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OTHER_SPACE), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MANUAL), gShellDebug1HiiHandle);
      break;

    case 4:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AUTO_SWITCH), gShellDebug1HiiHandle);
      break;

    case 5:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_WIDE_RANGE), gShellDebug1HiiHandle);
      break;

    case 6:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NOT_APPLICABLE), gShellDebug1HiiHandle);
      break;

    default:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED_3), gShellDebug1HiiHandle);
      break;
  }

  //
  // Power supply is unplugged from the wall
  //
  if (BIT (Characteristics, 2) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_UNPLUGGED), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_PLUGGED), gShellDebug1HiiHandle);
  }

  //
  // Power supply is present
  //
  if (BIT (Characteristics, 1) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_PRESENT), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_NOT_PRESENT), gShellDebug1HiiHandle);
  }

  //
  // hot replaceable
  //
  if (BIT (Characteristics, 0) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_REPLACE), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_NOT_REPLACE), gShellDebug1HiiHandle);
  }
}

/**
  Display TPM Device (Type 43) Characteristics.

  @param[in] Chara    The information bits.
  @param[in] Option   The optional information.
**/
VOID
DisplayTpmDeviceCharacteristics (
  IN UINT64  Chara,
  IN UINT8   Option
  )
{
  //
  // Print header
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TPM_DEVICE_CHAR), gShellDebug1HiiHandle);
  //
  // print option
  //
  PRINT_INFO_OPTION (Chara, Option);

  //
  // Check all the bits and print information
  // This function does not use Table because table of bits
  //   are designed not to deal with UINT64
  //
  if (BIT (Chara, 0) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED_BIT), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 1) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED_BIT), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 2) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TPM_DEVICE_CHAR_NOT_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 3) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TPM_DEVICE_CONFIG_FWU), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 4) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TPM_DEVICE_CONFIG_PLAT_SW), gShellDebug1HiiHandle);
  }

  if (BIT (Chara, 5) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TPM_DEVICE_CONFIG_OEM), gShellDebug1HiiHandle);
  }

  //
  // Just print the Reserved
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_06_63), gShellDebug1HiiHandle);
}
