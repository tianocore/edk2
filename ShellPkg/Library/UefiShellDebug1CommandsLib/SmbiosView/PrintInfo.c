/** @file
  Module for clarifying the content of the smbios structure element information.

  Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "../UefiShellDebug1CommandsLib.h"
#include "PrintInfo.h"
#include "LibSmbiosView.h"
#include "QueryTable.h"
#include "EventLogInfo.h"


//
// Get the certain bit of 'value'
//
#define BIT(value, bit) ((value) & ((UINT64) 1) << (bit))

//
//////////////////////////////////////////////////////////
//  Macros of print structure element, simplify coding.
//
#define PrintPendingString(pStruct, type, element) \
  do { \
    CHAR8 StringBuf[64]; \
    SetMem (StringBuf, sizeof (StringBuf), 0x00); \
    SmbiosGetPendingString ((pStruct), (pStruct->type->element), StringBuf); \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintEx(-1,-1,L": %a\n", StringBuf); \
  } while (0);

#define PrintSmbiosString(pStruct, stringnumber, element) \
  do { \
    CHAR8 StringBuf[64]; \
    SetMem (StringBuf, sizeof (StringBuf), 0x00); \
    SmbiosGetPendingString ((pStruct), (stringnumber), StringBuf); \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintEx(-1,-1,L": %a\n", StringBuf); \
  } while (0);

#define PrintStructValue(pStruct, type, element) \
  do { \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintEx(-1,-1,L": %d\n", (pStruct->type->element)); \
  } while (0);

#define PrintStructValueH(pStruct, type, element) \
  do { \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintEx(-1,-1,L": 0x%x\n", (pStruct->type->element)); \
  } while (0);

#define PrintBitField(pStruct, type, element, size) \
  do { \
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DUMP), gShellDebug1HiiHandle); \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SIZE), gShellDebug1HiiHandle, size); \
    DumpHex (0, 0, size, &(pStruct->type->element)); \
  } while (0);

#define PrintSmbiosBitField(pStruct, startaddress, element, size) \
  do { \
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DUMP), gShellDebug1HiiHandle); \
    ShellPrintEx(-1,-1,L"%a",#element); \
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SIZE), gShellDebug1HiiHandle, size); \
    DumpHex (0, 0, size, startaddress); \
  } while (0);

//
/////////////////////////////////////////
//
VOID
MemToString (
  IN OUT VOID   *Dest,
  IN VOID       *Src,
  IN UINTN      Length
  )
/*++

Routine Description:
  Copy Length of Src buffer to Dest buffer,
  add a NULL termination to Dest buffer.

Arguments:
  Dest      - Destination buffer head
  Src       - Source buffer head
  Length    - Length of buffer to be copied

Returns:
  None.

**/
{
  UINT8 *SrcBuffer;
  UINT8 *DestBuffer;
  SrcBuffer   = (UINT8 *) Src;
  DestBuffer  = (UINT8 *) Dest;
  //
  // copy byte by byte
  //
  while ((Length--)!=0) {
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
VOID
SmbiosPrintEPSInfo (
  IN  SMBIOS_STRUCTURE_TABLE  *SmbiosTable,
  IN  UINT8                   Option
  )
/*++

Routine Description:
  Print the info of EPS(Entry Point Structure)

Arguments:
  SmbiosTable    - Pointer to the SMBIOS table entry point
  Option         - Display option

Returns: None

**/
{
  UINT8 Anchor[5];
  UINT8 InAnchor[6];

  if (SmbiosTable == NULL) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SMBIOSTABLE_NULL), gShellDebug1HiiHandle);
    return ;
  }

  if (Option == SHOW_NONE) {
    return ;
  }

  if (Option >= SHOW_NORMAL) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENTRY_POINT_SIGN), gShellDebug1HiiHandle);
    MemToString (Anchor, SmbiosTable->AnchorString, 4);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ANCHOR_STR), gShellDebug1HiiHandle, Anchor);
    ShellPrintHiiEx(-1,-1,NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_EPS_CHECKSUM),
      gShellDebug1HiiHandle,
      SmbiosTable->EntryPointStructureChecksum
     );
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENTRY_POINT_LEN), gShellDebug1HiiHandle, SmbiosTable->EntryPointLength);
    ShellPrintHiiEx(-1,-1,NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VERSION),
      gShellDebug1HiiHandle,
      SmbiosTable->MajorVersion,
      SmbiosTable->MinorVersion
     );
    ShellPrintHiiEx(-1,-1,NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NUMBER_STRUCT),
      gShellDebug1HiiHandle,
      SmbiosTable->NumberOfSmbiosStructures
     );
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MAX_STRUCT_SIZE), gShellDebug1HiiHandle, SmbiosTable->MaxStructureSize);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TABLE_ADDR), gShellDebug1HiiHandle, SmbiosTable->TableAddress);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TABLE_LENGTH), gShellDebug1HiiHandle, SmbiosTable->TableLength);

  }
  //
  // If SHOW_ALL, also print followings.
  //
  if (Option >= SHOW_DETAIL) {
    ShellPrintHiiEx(-1,-1,NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENTRY_POINT_REVISION),
      gShellDebug1HiiHandle,
      SmbiosTable->EntryPointRevision
     );
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BCD_REVISION), gShellDebug1HiiHandle, SmbiosTable->SmbiosBcdRevision);
    //
    // Since raw data is not string, add a NULL terminater.
    //
    MemToString (InAnchor, SmbiosTable->IntermediateAnchorString, 5);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTER_ACHOR), gShellDebug1HiiHandle, InAnchor);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTER_CHECKSUM), gShellDebug1HiiHandle, SmbiosTable->IntermediateChecksum);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_FORMATTED_AREA), gShellDebug1HiiHandle);
    DumpHex (2, 0, 5, SmbiosTable->FormattedArea);
  }

  Print (L"\n");
}

EFI_STATUS
SmbiosPrintStructure (
  IN  SMBIOS_STRUCTURE_POINTER  *pStruct,
  IN  UINT8                     Option
  )
/*++

Routine Description:
  This function print the content of the structure pointed by pStruct

Arguments:
  pStruct     - point to the structure to be printed
  Option      - print option of information detail

Returns:
  EFI_SUCCESS             - Successfully Printing this function
  EFI_INVALID_PARAMETER   - Invalid Structure
  EFI_UNSUPPORTED         - Unsupported

**/
{
  UINT8 Index;
  UINT8 *Buffer;

  Buffer = (UINT8 *) (UINTN) (pStruct->Raw);

  if (pStruct == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Option == SHOW_NONE) {
    return EFI_SUCCESS;
  }
  //
  // Display structure header
  //
  DisplayStructureTypeInfo (pStruct->Hdr->Type, SHOW_DETAIL);
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_FORMAT_PART_LEN), gShellDebug1HiiHandle, pStruct->Hdr->Length);
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_STRUCT_HANDLE), gShellDebug1HiiHandle, pStruct->Hdr->Handle);

  if (Option == SHOW_OUTLINE) {
    return EFI_SUCCESS;
  }

  switch (pStruct->Hdr->Type) {
  //
  //
  //
  case 0:
    PrintPendingString (pStruct, Type0, Vendor);
    PrintPendingString (pStruct, Type0, BiosVersion);
    PrintStructValue (pStruct, Type0, BiosSegment);
    PrintPendingString (pStruct, Type0, BiosReleaseDate);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_SIZE), gShellDebug1HiiHandle, 64 * (pStruct->Type0->BiosSize + 1));

    if (Option < SHOW_DETAIL) {
      PrintStructValueH (pStruct, Type0, BiosCharacteristics);
    } else {
      DisplayBiosCharacteristics (pStruct->Type0->BiosCharacteristics, Option);

      //
      // The length of above format part is 0x12 bytes,
      // Ext bytes are following, size = 'len-0x12'.
      // If len-0x12 > 0, then
      //    there are extension bytes (byte1, byte2, byte3...)
      // And byte3 not stated in spec, so dump all extension bytes(1, 2, 3..)
      //
      if ((Buffer[1] - (CHAR8) 0x12) > 0) {
        DisplayBiosCharacteristicsExt1 (Buffer[0x12], Option);
      }

      if ((Buffer[1] - (CHAR8) 0x12) > 1) {
        DisplayBiosCharacteristicsExt2 (Buffer[0x13], Option);
      }

      if ((Buffer[1] - (CHAR8) 0x12) > 2) {
        PrintBitField (
          pStruct,
          Type0,
          BiosCharacteristics,
          Buffer[1] - (CHAR8) 0x12
         );
      }
    }
    break;

  //
  // System Information (Type 1)
  //
  case 1:
    PrintPendingString (pStruct, Type1, Manufacturer);
    PrintPendingString (pStruct, Type1, ProductName);
    PrintPendingString (pStruct, Type1, Version);
    PrintPendingString (pStruct, Type1, SerialNumber);
    PrintBitField (pStruct, Type1, Uuid, 16);
    DisplaySystemWakeupType (pStruct->Type1->WakeUpType, Option);
    break;

  case 2:
    PrintPendingString (pStruct, Type2, Manufacturer);
    PrintPendingString (pStruct, Type2, ProductName);
    PrintPendingString (pStruct, Type2, Version);
    PrintPendingString (pStruct, Type2, SerialNumber);
    break;

  //
  // System Enclosure (Type 3)
  //
  case 3:
    PrintPendingString (pStruct, Type3, Manufacturer);
    PrintStructValue (pStruct, Type3, Type);
    DisplaySystemEnclosureType (pStruct->Type3->Type, Option);
    PrintPendingString (pStruct, Type3, Version);
    PrintPendingString (pStruct, Type3, SerialNumber);
    PrintPendingString (pStruct, Type3, AssetTag);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BOOTUP_STATE), gShellDebug1HiiHandle);
    DisplaySystemEnclosureStatus (pStruct->Type3->BootupState, Option);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_STATE), gShellDebug1HiiHandle);
    DisplaySystemEnclosureStatus (pStruct->Type3->PowerSupplyState, Option);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_THERMAL_STATE), gShellDebug1HiiHandle);
    DisplaySystemEnclosureStatus (pStruct->Type3->ThermalState, Option);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SECURITY_STATUS), gShellDebug1HiiHandle);
    DisplaySESecurityStatus (pStruct->Type3->SecurityStatus, Option);
    PrintBitField (pStruct, Type3, OemDefined, 4);
    break;

  //
  // Processor Information (Type 4)
  //
  case 4:
    PrintStructValue (pStruct, Type4, Socket);
    DisplayProcessorType (pStruct->Type4->ProcessorType, Option);
    if ((SmbiosMajorVersion > 0x2 || (SmbiosMajorVersion == 0x2 && SmbiosMinorVersion >= 0x6)) &&
        (pStruct->Type4->ProcessorFamily == 0xFE)) {
      //
      // Get family from ProcessorFamily2 field
      //
      DisplayProcessorFamily2 (pStruct->Type4->ProcessorFamily2, Option);
    } else {
      DisplayProcessorFamily (pStruct->Type4->ProcessorFamily, Option);
    }
    PrintPendingString (pStruct, Type4, ProcessorManufacture);
    PrintBitField (pStruct, Type4, ProcessorId, 8);
    PrintPendingString (pStruct, Type4, ProcessorVersion);
    DisplayProcessorVoltage (pStruct->Type4->Voltage, Option);
    PrintStructValue (pStruct, Type4, ExternalClock);
    PrintStructValue (pStruct, Type4, MaxSpeed);
    PrintStructValue (pStruct, Type4, CurrentSpeed);
    DisplayProcessorStatus (pStruct->Type4->Status, Option);
    DisplayProcessorUpgrade (pStruct->Type4->ProcessorUpgrade, Option);
    PrintStructValueH (pStruct, Type4, L1CacheHandle);
    PrintStructValueH (pStruct, Type4, L2CacheHandle);
    PrintStructValueH (pStruct, Type4, L3CacheHandle);
    PrintPendingString (pStruct, Type4, SerialNumber);
    PrintPendingString (pStruct, Type4, AssetTag);
    PrintPendingString (pStruct, Type4, PartNumber);
    if (SmbiosMajorVersion > 0x2 || (SmbiosMajorVersion == 0x2 && SmbiosMinorVersion >= 0x5)) {
      PrintStructValue (pStruct, Type4, CoreCount);
      PrintStructValue (pStruct, Type4, EnabledCoreCount);
      PrintStructValue (pStruct, Type4, ThreadCount);
      PrintStructValueH (pStruct, Type4, ProcessorCharacteristics);
    }
    break;

  //
  // Memory Controller Information (Type 5)
  //
  case 5:
    {
      UINT8 SlotNum;
      SlotNum = pStruct->Type5->AssociatedMemorySlotNum;

      DisplayMcErrorDetectMethod (pStruct->Type5->ErrDetectMethod, Option);
      DisplayMcErrorCorrectCapability (pStruct->Type5->ErrCorrectCapability, Option);
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SUPOPRT), gShellDebug1HiiHandle);
      DisplayMcInterleaveSupport (pStruct->Type5->SupportInterleave, Option);
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CURRENT), gShellDebug1HiiHandle);
      DisplayMcInterleaveSupport (pStruct->Type5->CurrentInterleave, Option);
      DisplayMaxMemoryModuleSize (pStruct->Type5->MaxMemoryModuleSize, SlotNum, Option);
      DisplayMcMemorySpeeds (pStruct->Type5->SupportSpeed, Option);
      DisplayMmMemoryType (pStruct->Type5->SupportMemoryType, Option);
      DisplayMemoryModuleVoltage (pStruct->Type5->MemoryModuleVoltage, Option);
      PrintStructValue (pStruct, Type5, AssociatedMemorySlotNum);
      //
      // According to SMBIOS Specification, offset 0x0F
      //
      DisplayMemoryModuleConfigHandles ((UINT16 *) (&Buffer[0x0F]), SlotNum, Option);
      DisplayMcErrorCorrectCapability (Buffer[0x0F + 2 * SlotNum], Option);
    }
    break;

  //
  // Memory Module Information (Type 6)
  //
  case 6:
    PrintPendingString (pStruct, Type6, SocketDesignation);
    DisplayMmBankConnections (pStruct->Type6->BankConnections, Option);
    PrintStructValue (pStruct, Type6, CurrentSpeed);
    DisplayMmMemoryType (pStruct->Type6->CurrentMemoryType, Option);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INSTALLED), gShellDebug1HiiHandle);
    DisplayMmMemorySize (pStruct->Type6->InstalledSize, Option);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENABLED), gShellDebug1HiiHandle);
    DisplayMmMemorySize (pStruct->Type6->EnabledSize, Option);
    DisplayMmErrorStatus (pStruct->Type6->ErrorStatus, Option);
    break;

  //
  // Cache Information (Type 7)
  //
  case 7:
    PrintPendingString (pStruct, Type7, SocketDesignation);
    PrintStructValueH (pStruct, Type7, CacheConfiguration);
    PrintStructValueH (pStruct, Type7, MaximumCacheSize);
    PrintStructValueH (pStruct, Type7, InstalledSize);
    PrintStructValueH (pStruct, Type7, SupportedSRAMType);
    PrintStructValueH (pStruct, Type7, CurrentSRAMType);
    DisplayCacheSRAMType (pStruct->Type7->CurrentSRAMType, Option);
    PrintStructValueH (pStruct, Type7, CacheSpeed);
    DisplayCacheErrCorrectingType (pStruct->Type7->ErrorCorrectionType, Option);
    DisplayCacheSystemCacheType (pStruct->Type7->SystemCacheType, Option);
    DisplayCacheAssociativity (pStruct->Type7->Associativity, Option);
    break;

  //
  // Port Connector Information  (Type 8)
  //
  case 8:
    PrintPendingString (pStruct, Type8, InternalReferenceDesignator);
    Print (L"Internal ");
    DisplayPortConnectorType (pStruct->Type8->InternalConnectorType, Option);
    PrintPendingString (pStruct, Type8, ExternalReferenceDesignator);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_EXTERNAL), gShellDebug1HiiHandle);
    DisplayPortConnectorType (pStruct->Type8->ExternalConnectorType, Option);
    DisplayPortType (pStruct->Type8->PortType, Option);
    break;

  //
  // System Slots (Type 9)
  //
  case 9:
    PrintPendingString (pStruct, Type9, SlotDesignation);
    DisplaySystemSlotType (pStruct->Type9->SlotType, Option);
    DisplaySystemSlotDataBusWidth (pStruct->Type9->SlotDataBusWidth, Option);
    DisplaySystemSlotCurrentUsage (pStruct->Type9->CurrentUsage, Option);
    DisplaySystemSlotLength (pStruct->Type9->SlotLength, Option);
    DisplaySystemSlotId (
      pStruct->Type9->SlotID,
      pStruct->Type9->SlotType,
      Option
     );
    DisplaySlotCharacteristics1 (pStruct->Type9->SlotCharacteristics1, Option);
    DisplaySlotCharacteristics2 (pStruct->Type9->SlotCharacteristics2, Option);
    if (SmbiosMajorVersion > 0x2 || (SmbiosMajorVersion == 0x2 && SmbiosMinorVersion >= 0x6)) {
      PrintStructValueH (pStruct, Type9, SegmentGroupNum);
      PrintStructValueH (pStruct, Type9, BusNum);
      PrintStructValueH (pStruct, Type9, DevFuncNum);
    }
    break;

  //
  // On Board Devices Information (Type 10)
  //
  case 10:
    {
      UINTN NumOfDevice;
      NumOfDevice = (pStruct->Type10->Hdr.Length - sizeof (SMBIOS_HEADER)) / (2 * sizeof (UINT8));
      for (Index = 0; Index < NumOfDevice; Index++) {
        DisplayOnboardDeviceTypes (pStruct->Type10->Device[Index].DeviceType, Option);
        ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DESC_STRING), gShellDebug1HiiHandle);
        ShellPrintEx(-1,-1,L"%a",LibGetSmbiosString (pStruct, pStruct->Type10->Device[Index].DescriptionString));
      }
    }
    break;

  case 11:
    PrintStructValue (pStruct, Type11, StringCount);
    for (Index = 1; Index <= pStruct->Type11->StringCount; Index++) {
      ShellPrintEx(-1,-1,L"%a\n", LibGetSmbiosString (pStruct, Index));
    }
    break;

  case 12:
    PrintStructValue (pStruct, Type12, StringCount);
    for (Index = 1; Index <= pStruct->Type12->StringCount; Index++) {
      ShellPrintEx(-1,-1,L"%a\n", LibGetSmbiosString (pStruct, Index));
    }
    break;

  case 13:
    PrintStructValue (pStruct, Type13, InstallableLanguages);
    PrintStructValue (pStruct, Type13, Flags);
    PrintBitField (pStruct, Type13, reserved, 15);
    PrintPendingString (pStruct, Type13, CurrentLanguages);
    break;

  case 14:
    PrintPendingString (pStruct, Type14, GroupName);
    PrintStructValue (pStruct, Type14, ItemType);
    PrintStructValue (pStruct, Type14, ItemHandle);
    break;

  //
  // System Event Log (Type 15)
  //
  case 15:
    {
      EVENTLOGTYPE  *Ptr;
      UINT8         Count;
      UINT8         *AccessMethodAddress;

      PrintStructValueH (pStruct, Type15, LogAreaLength);
      PrintStructValueH (pStruct, Type15, LogHeaderStartOffset);
      PrintStructValueH (pStruct, Type15, LogDataStartOffset);
      DisplaySELAccessMethod (pStruct->Type15->AccessMethod, Option);
      PrintStructValueH (pStruct, Type15, AccessMethodAddress);
      DisplaySELLogStatus (pStruct->Type15->LogStatus, Option);
      PrintStructValueH (pStruct, Type15, LogChangeToken);
      DisplaySysEventLogHeaderFormat (pStruct->Type15->LogHeaderFormat, Option);
      PrintStructValueH (pStruct, Type15, NumberOfSupportedLogTypeDescriptors);
      PrintStructValueH (pStruct, Type15, LengthOfLogTypeDescriptor);

      Count = pStruct->Type15->NumberOfSupportedLogTypeDescriptors;
      if (Count > 0) {
        Ptr = pStruct->Type15->EventLogTypeDescriptors;

        //
        // Display all Event Log type descriptors supported by system
        //
        for (Index = 0; Index < Count; Index++, Ptr++) {
          ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SUPOPRTED_EVENT), gShellDebug1HiiHandle, Index + 1);
          DisplaySELTypes (Ptr->LogType, Option);
          DisplaySELVarDataFormatType (Ptr->DataFormatType, Option);
        }

        if (Option >= SHOW_DETAIL) {
          switch (pStruct->Type15->AccessMethod) {
          case 03:
            AccessMethodAddress = (UINT8 *) (UINTN) (pStruct->Type15->AccessMethodAddress);
            break;

          case 00:
          case 01:
          case 02:
          case 04:
          default:
            ShellPrintHiiEx(-1,-1,NULL,
              STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ACCESS_METHOD_NOT_SUPOPRTED),
              gShellDebug1HiiHandle,
              pStruct->Type15->AccessMethod
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
            pStruct->Type15->LogHeaderFormat,
            AccessMethodAddress + pStruct->Type15->LogHeaderStartOffset
           );

          //
          // Display all Event Log data
          //
          // Starting offset (or index) within the nonvolatile storage
          // of the event-log's first data byte, from the Access Method Address(0x14)
          //
          DisplaySysEventLogData (
            AccessMethodAddress + pStruct->Type15->LogDataStartOffset,
            (UINT16)
            (
            pStruct->Type15->LogAreaLength -
            (pStruct->Type15->LogDataStartOffset - pStruct->Type15->LogDataStartOffset)
           )
           );
        }

      }
    }
    break;

  //
  // Physical Memory Array (Type 16)
  //
  case 16:
    DisplayPMALocation (pStruct->Type16->Location, Option);
    DisplayPMAUse (pStruct->Type16->Use, Option);
    DisplayPMAErrorCorrectionTypes (
      pStruct->Type16->MemoryErrorCorrection,
      Option
     );
    PrintStructValueH (pStruct, Type16, MaximumCapacity);
    PrintStructValueH (pStruct, Type16, MemoryErrorInformationHandle);
    PrintStructValueH (pStruct, Type16, NumberOfMemoryDevices);
    break;

  //
  // Memory Device (Type 17)
  //
  case 17:
    PrintStructValueH (pStruct, Type17, MemoryArrayHandle);
    PrintStructValueH (pStruct, Type17, MemoryErrorInformationHandle);
    PrintStructValue (pStruct, Type17, TotalWidth);
    PrintStructValue (pStruct, Type17, DataWidth);
    PrintStructValue (pStruct, Type17, Size);
    DisplayMemoryDeviceFormFactor (pStruct->Type17->FormFactor, Option);
    PrintStructValueH (pStruct, Type17, DeviceSet);
    PrintPendingString (pStruct, Type17, DeviceLocator);
    PrintPendingString (pStruct, Type17, BankLocator);
    DisplayMemoryDeviceType (pStruct->Type17->MemoryType, Option);
    DisplayMemoryDeviceTypeDetail (pStruct->Type17->TypeDetail, Option);
    PrintStructValueH (pStruct, Type17, Speed);
    PrintPendingString (pStruct, Type17, Manufacturer);
    PrintPendingString (pStruct, Type17, SerialNumber);
    PrintPendingString (pStruct, Type17, AssetTag);
    PrintPendingString (pStruct, Type17, PartNumber);
    if (SmbiosMajorVersion > 0x2 || (SmbiosMajorVersion == 0x2 && SmbiosMinorVersion >= 0x6)) {
      PrintStructValueH (pStruct, Type17, Attributes);
    }
    break;

  //
  // 32-bit Memory Error Information (Type 18)
  //
  case 18:
    DisplayMemoryErrorType (pStruct->Type18->ErrorType, Option);
    DisplayMemoryErrorGranularity (
      pStruct->Type18->ErrorGranularity,
      Option
     );
    DisplayMemoryErrorOperation (pStruct->Type18->ErrorOperation, Option);
    PrintStructValueH (pStruct, Type18, VendorSyndrome);
    PrintStructValueH (pStruct, Type18, MemoryArrayErrorAddress);
    PrintStructValueH (pStruct, Type18, DeviceErrorAddress);
    PrintStructValueH (pStruct, Type18, ErrorResolution);
    break;

  //
  // Memory Array Mapped Address (Type 19)
  //
  case 19:
    PrintStructValueH (pStruct, Type19, StartingAddress);
    PrintStructValueH (pStruct, Type19, EndingAddress);
    PrintStructValueH (pStruct, Type19, MemoryArrayHandle);
    PrintStructValueH (pStruct, Type19, PartitionWidth);
    break;

  //
  // Memory Device Mapped Address  (Type 20)
  //
  case 20:
    PrintStructValueH (pStruct, Type20, StartingAddress);
    PrintStructValueH (pStruct, Type20, EndingAddress);
    PrintStructValueH (pStruct, Type20, MemoryDeviceHandle);
    PrintStructValueH (pStruct, Type20, MemoryArrayMappedAddressHandle);
    PrintStructValueH (pStruct, Type20, PartitionRowPosition);
    PrintStructValueH (pStruct, Type20, InterleavePosition);
    PrintStructValueH (pStruct, Type20, InterleavedDataDepth);
    break;

  //
  // Built-in Pointing Device  (Type 21)
  //
  case 21:
    DisplayPointingDeviceType (pStruct->Type21->Type, Option);
    DisplayPointingDeviceInterface (pStruct->Type21->Interface, Option);
    PrintStructValue (pStruct, Type21, NumberOfButtons);
    break;

  //
  // Portable Battery  (Type 22)
  //
  case 22:
    PrintPendingString (pStruct, Type22, Location);
    PrintPendingString (pStruct, Type22, Manufacturer);
    PrintPendingString (pStruct, Type22, ManufactureDate);
    PrintPendingString (pStruct, Type22, SerialNumber);
    PrintPendingString (pStruct, Type22, DeviceName);
    DisplayPBDeviceChemistry (
      pStruct->Type22->DeviceChemistry,
      Option
     );
    PrintStructValueH (pStruct, Type22, DeviceCapacity);
    PrintStructValueH (pStruct, Type22, DesignVoltage);
    PrintPendingString (pStruct, Type22, SBDSVersionNumber);
    PrintStructValueH (pStruct, Type22, MaximumErrorInBatteryData);
    PrintStructValueH (pStruct, Type22, SBDSSerialNumber);
    DisplaySBDSManufactureDate (
      pStruct->Type22->SBDSManufactureDate,
      Option
     );
    PrintPendingString (pStruct, Type22, SBDSDeviceChemistry);
    PrintStructValueH (pStruct, Type22, DesignCapacityMultiplier);
    PrintStructValueH (pStruct, Type22, OEMSpecific);
    break;

  case 23:
    DisplaySystemResetCapabilities (
      pStruct->Type23->Capabilities,
      Option
     );
    PrintStructValueH (pStruct, Type23, ResetCount);
    PrintStructValueH (pStruct, Type23, ResetLimit);
    PrintStructValueH (pStruct, Type23, TimerInterval);
    PrintStructValueH (pStruct, Type23, Timeout);
    break;

  case 24:
    DisplayHardwareSecuritySettings (
      pStruct->Type24->HardwareSecuritySettings,
      Option
     );
    break;

  case 25:
    PrintStructValueH (pStruct, Type25, NextScheduledPowerOnMonth);
    PrintStructValueH (pStruct, Type25, NextScheduledPowerOnDayOfMonth);
    PrintStructValueH (pStruct, Type25, NextScheduledPowerOnHour);
    PrintStructValueH (pStruct, Type25, NextScheduledPowerOnMinute);
    PrintStructValueH (pStruct, Type25, NextScheduledPowerOnSecond);
    break;

  case 26:
    PrintPendingString (pStruct, Type26, Description);
    DisplayVPLocation (pStruct->Type26->LocationAndStatus, Option);
    DisplayVPStatus (pStruct->Type26->LocationAndStatus, Option);
    PrintStructValueH (pStruct, Type26, MaximumValue);
    PrintStructValueH (pStruct, Type26, MinimumValue);
    PrintStructValueH (pStruct, Type26, Resolution);
    PrintStructValueH (pStruct, Type26, Tolerance);
    PrintStructValueH (pStruct, Type26, Accuracy);
    PrintStructValueH (pStruct, Type26, OEMDefined);
    PrintStructValueH (pStruct, Type26, NominalValue);
    break;

  case 27:
    PrintStructValueH (pStruct, Type27, TemperatureProbeHandle);
    DisplayCoolingDeviceStatus (pStruct->Type27->DeviceTypeAndStatus, Option);
    DisplayCoolingDeviceType (pStruct->Type27->DeviceTypeAndStatus, Option);
    PrintStructValueH (pStruct, Type27, CoolingUnitGroup);
    PrintStructValueH (pStruct, Type27, OEMDefined);
    PrintStructValueH (pStruct, Type27, NominalSpeed);
    break;

  case 28:
    PrintPendingString (pStruct, Type28, Description);
    DisplayTemperatureProbeStatus (pStruct->Type28->LocationAndStatus, Option);
    DisplayTemperatureProbeLoc (pStruct->Type28->LocationAndStatus, Option);
    PrintStructValueH (pStruct, Type28, MaximumValue);
    PrintStructValueH (pStruct, Type28, MinimumValue);
    PrintStructValueH (pStruct, Type28, Resolution);
    PrintStructValueH (pStruct, Type28, Tolerance);
    PrintStructValueH (pStruct, Type28, Accuracy);
    PrintStructValueH (pStruct, Type28, OEMDefined);
    PrintStructValueH (pStruct, Type28, NominalValue);
    break;

  case 29:
    PrintPendingString (pStruct, Type29, Description);
    DisplayECPStatus (pStruct->Type29->LocationAndStatus, Option);
    DisplayECPLoc (pStruct->Type29->LocationAndStatus, Option);
    PrintStructValueH (pStruct, Type29, MaximumValue);
    PrintStructValueH (pStruct, Type29, MinimumValue);
    PrintStructValueH (pStruct, Type29, Resolution);
    PrintStructValueH (pStruct, Type29, Tolerance);
    PrintStructValueH (pStruct, Type29, Accuracy);
    PrintStructValueH (pStruct, Type29, OEMDefined);
    PrintStructValueH (pStruct, Type29, NominalValue);
    break;

  case 30:
    PrintPendingString (pStruct, Type30, ManufacturerName);
    DisplayOBRAConnections (pStruct->Type30->Connections, Option);
    break;

  case 31:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_STRUCT_TYPE31), gShellDebug1HiiHandle);
    break;

  case 32:
    PrintBitField (pStruct, Type32, Reserved, 6);
    DisplaySystemBootStatus (pStruct->Type32->BootStatus[0], Option);
    break;

  case 33:
    DisplayMemoryErrorType (pStruct->Type33->ErrorType, Option);
    DisplayMemoryErrorGranularity (
      pStruct->Type33->ErrorGranularity,
      Option
     );
    DisplayMemoryErrorOperation (pStruct->Type33->ErrorOperation, Option);
    PrintStructValueH (pStruct, Type33, VendorSyndrome);
    PrintStructValueH (pStruct, Type33, MemoryArrayErrorAddress);
    PrintStructValueH (pStruct, Type33, DeviceErrorAddress);
    PrintStructValueH (pStruct, Type33, ErrorResolution);
    break;

  //
  // Management Device  (Type 34)
  //
  case 34:
    PrintPendingString (pStruct, Type34, Description);
    DisplayMDType (pStruct->Type34->Type, Option);
    PrintStructValueH (pStruct, Type34, Address);
    PrintStructValueH (pStruct, Type34, AddressType);
    break;

  case 35:
    PrintPendingString (pStruct, Type35, Description);
    PrintStructValueH (pStruct, Type35, ManagementDeviceHandle);
    PrintStructValueH (pStruct, Type35, ComponentHandle);
    PrintStructValueH (pStruct, Type35, ThresholdHandle);
    break;

  case 36:
    PrintStructValueH (pStruct, Type36, LowerThresholdNonCritical);
    PrintStructValueH (pStruct, Type36, UpperThresholdNonCritical);
    PrintStructValueH (pStruct, Type36, LowerThresholdCritical);
    PrintStructValueH (pStruct, Type36, UpperThresholdCritical);
    PrintStructValueH (pStruct, Type36, LowerThresholdNonRecoverable);
    PrintStructValueH (pStruct, Type36, UpperThresholdNonRecoverable);
    break;

  //
  // Memory Channel  (Type 37)
  //
  case 37:
    {
      UINT8         Count;
      MEMORYDEVICE  *Ptr;
      DisplayMemoryChannelType (pStruct->Type37->ChannelType, Option);
      PrintStructValueH (pStruct, Type37, MaximumChannelLoad);
      PrintStructValueH (pStruct, Type37, MemoryDeviceCount);

      Count = pStruct->Type37->MemoryDeviceCount;
      Ptr   = pStruct->Type37->MemoryDevice;
      for (Index = 0; Index < Count; Index++) {
        ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MEM_DEVICE), gShellDebug1HiiHandle, Index + 1);
        ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DEV_LOAD), gShellDebug1HiiHandle, Ptr->DeviceLoad);
        ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DEV_HANDLE), gShellDebug1HiiHandle, Ptr->DeviceHandle);
      }
    }
    break;

  //
  // IPMI Device Information  (Type 38)
  //
  case 38:
    DisplayIPMIDIBMCInterfaceType (pStruct->Type38->InterfaceType, Option);
    PrintStructValueH (pStruct, Type38, IPMISpecificationRevision);
    PrintStructValueH (pStruct, Type38, I2CSlaveAddress);
    PrintStructValueH (pStruct, Type38, NVStorageDeviceAddress);
    PrintStructValueH (pStruct, Type38, BaseAddress);
    break;

  //
  // System Power Supply (Type 39)
  //
  case 39:
    PrintStructValueH (pStruct, Type39, PowerUnitGroup);
    PrintPendingString (pStruct, Type39, Location);
    PrintPendingString (pStruct, Type39, DeviceName);
    PrintPendingString (pStruct, Type39, Manufacturer);
    PrintPendingString (pStruct, Type39, SerialNumber);
    PrintPendingString (pStruct, Type39, AssetTagNumber);
    PrintPendingString (pStruct, Type39, ModelPartNumber);
    PrintPendingString (pStruct, Type39, RevisionLevel);
    PrintStructValueH (pStruct, Type39, MaxPowerCapacity);
    DisplaySPSCharacteristics (
      pStruct->Type39->PowerSupplyCharacteristics,
      Option
     );
    PrintStructValueH (pStruct, Type39, InputVoltageProbeHandle);
    PrintStructValueH (pStruct, Type39, CoolingDeviceHandle);
    PrintStructValueH (pStruct, Type39, InputCurrentProbeHandle);
    break;

  //
  // Additional Information (Type 40)
  //
  case 40:
    {
      UINT8                          NumberOfEntries;
      UINT8                          EntryLength;
      ADDITIONAL_INFORMATION_ENTRY   *Entries;

      EntryLength     = 0;
      Entries         = pStruct->Type40->AdditionalInfoEntries;
      NumberOfEntries = pStruct->Type40->NumberOfAdditionalInformationEntries;

      PrintStructValueH (pStruct, Type40, NumberOfAdditionalInformationEntries);

      for (Index = 0; Index < NumberOfEntries; Index++) {
        EntryLength = Entries->EntryLength;
        ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_ENTRYLEN), gShellDebug1HiiHandle, EntryLength);
        ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_REFERENCEDHANDLE), gShellDebug1HiiHandle, Entries->ReferencedHandle);
        ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_SMBIOSVIEW_REFERENCEDOFFSET), gShellDebug1HiiHandle, Entries->ReferencedOffset);
        PrintSmbiosString (pStruct, Entries->EntryString, String);
        PrintSmbiosBitField (pStruct, Entries->Value, Value, EntryLength - 5);
        Entries = (ADDITIONAL_INFORMATION_ENTRY *) ((UINT8 *)Entries + EntryLength);
      }
    }
    break;

  //
  // Onboard Devices Extended Information (Type 41)
  //
  case 41:
    PrintPendingString (pStruct, Type41, ReferenceDesignation);
    PrintStructValueH (pStruct, Type41, DeviceType);
    PrintStructValueH (pStruct, Type41, DeviceTypeInstance);
    PrintStructValueH (pStruct, Type41, SegmentGroupNum);
    PrintStructValueH (pStruct, Type41, BusNum);
    PrintStructValueH (pStruct, Type41, DevFuncNum);
    break;

  case 126:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INACTIVE_STRUCT), gShellDebug1HiiHandle);
    break;

  case 127:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_THIS_STRUCT_END_TABLE), gShellDebug1HiiHandle);
    break;

  default:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_STRUCT_TYPE_UNDEFINED), gShellDebug1HiiHandle);
    break;
  }

  return EFI_SUCCESS;
}

VOID
DisplayBiosCharacteristics (
  UINT64  chara,
  UINT8   Option
  )
{
  //
  // Print header
  //
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_CHAR), gShellDebug1HiiHandle);
  //
  // print option
  //
  PRINT_INFO_OPTION (chara, Option);

  //
  // Check all the bits and print information
  // This function does not use Table because table of bits
  //   are designed not to deal with UINT64
  //
  if (BIT (chara, 0) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED_BIT), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 1) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED_BIT), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 2) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN_BIT), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 3) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_CHAR_NOT_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 4) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ISA_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 5) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MSA_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 6) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_EISA_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 7) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PCI_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 8) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PC_CARD_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 9) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PLUG_PLAY_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 10) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_APM_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 11) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_UPGRADEABLE), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 12) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_SHADOWING), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 13) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VESA_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 14) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ECSD_SUPPORT), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 15) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BOOT_FORM_CD_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 16) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SELECTED_BOOT_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 17) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_ROM_SOCKETED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 18) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BOOT_FROM_PC_CARD), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 19) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_EDD_ENHANCED_DRIVER), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 20) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_JAPANESE_FLOPPY_NEC), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 21) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_JAPANESE_FLOPPY_TOSHIBA), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 22) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_FLOPPY_SERVICES_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 23) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ONE_POINT_TWO_MB), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 24) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_720_KB), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 25) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TWO_POINT_EIGHT_EIGHT_MB), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 26) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PRINT_SCREEN_SUPPORT), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 27) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_KEYBOARD_SERV_SUPPORT), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 28) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SERIAL_SERVICES_SUPPORT), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 29) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PRINTER_SERVICES_SUPPORT), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 30) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MONO_VIDEO_SUPPORT), gShellDebug1HiiHandle);
  }

  if (BIT (chara, 31) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NEC_PC_98), gShellDebug1HiiHandle);
  }
  //
  // Just print the reserved
  //
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_32_47), gShellDebug1HiiHandle);
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_48_64), gShellDebug1HiiHandle);
}

VOID
DisplayBiosCharacteristicsExt1 (
  UINT8 byte1,
  UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_CHAR_EXTENSION), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (byte1, Option);

  //
  // check bit and print
  //
  if (BIT (byte1, 0) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ACPI_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (byte1, 1) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_USB_LEGACY_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (byte1, 2) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AGP_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (byte1, 3) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_I2O_BOOT_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (byte1, 4) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_LS_120_BOOT_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (byte1, 5) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ATAPI_ZIP_DRIVE), gShellDebug1HiiHandle);
  }

  if (BIT (byte1, 6) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_1394_BOOT_SUPPORTED), gShellDebug1HiiHandle);
  }

  if (BIT (byte1, 7) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SMART_BATTERY_SUPPORTED), gShellDebug1HiiHandle);
  }
}

VOID
DisplayBiosCharacteristicsExt2 (
  UINT8 byte2,
  UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_CHAR_EXTENSION_2), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (byte2, Option);

  if (BIT (byte2, 0) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIOS_BOOT_SPEC_SUPP), gShellDebug1HiiHandle);
  }

  if (BIT (byte2, 1) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_FUNCTION_KEY_INIT), gShellDebug1HiiHandle);
  }

  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_RSVD_FOR_FUTURE), gShellDebug1HiiHandle);
}

VOID
DisplayProcessorFamily (
  UINT8 Family,
  UINT8 Option
  )
{
  //
  // Print prompt message
  //
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PROCESSOR_FAMILY), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (Family, Option);

  //
  // Use switch to check
  //
  switch (Family) {
  case 0x01:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OTHER), gShellDebug1HiiHandle);
    break;

  case 0x02:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
    break;

  case 0x03:
    Print (L"8086\n");
    break;

  case 0x04:
    Print (L"80286\n");
    break;

  case 0x05:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL386_PROCESSOR), gShellDebug1HiiHandle);
    break;

  case 0x06:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL486_PROCESSOR), gShellDebug1HiiHandle);
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
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_PROC_FAMILY), gShellDebug1HiiHandle);
    break;

  case 0x0C:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_PRO_PROC), gShellDebug1HiiHandle);
    break;

  case 0x0D:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_II_PROC), gShellDebug1HiiHandle);
    break;

  case 0x0E:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_PROC_MMX), gShellDebug1HiiHandle);
    break;

  case 0x0F:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CELERON_PROC), gShellDebug1HiiHandle);
    break;

  case 0x10:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_XEON_PROC), gShellDebug1HiiHandle);
    break;

  case 0x11:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_III_PROC), gShellDebug1HiiHandle);
    break;

  case 0x12:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_M1_FAMILY), gShellDebug1HiiHandle);
    break;

  case 0x18:
    Print (L"AMD Duron\n");
    break;

  case 0x19:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_K5_FAMILY), gShellDebug1HiiHandle);
    break;

  case 0x20:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_PC_FAMILY), gShellDebug1HiiHandle);
    break;

  case 0x21:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_PC_601), gShellDebug1HiiHandle);
    break;

  case 0x22:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_PC_603), gShellDebug1HiiHandle);
    break;

  case 0x23:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_PC_603_PLUS), gShellDebug1HiiHandle);
    break;

  case 0x24:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_PC_604), gShellDebug1HiiHandle);
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
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE_DUO), gShellDebug1HiiHandle);
    break;

  case 0x29:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE_DUO_MOBILE), gShellDebug1HiiHandle);
    break;

  case 0x2A:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE_SOLO_MOBILE), gShellDebug1HiiHandle);
    break;

  case 0x2B:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_ATOM), gShellDebug1HiiHandle);
    break;

  case 0x30:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ALPHA_FAMILY_2), gShellDebug1HiiHandle);
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

  case 0x40:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MIPS_FAMILY), gShellDebug1HiiHandle);
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

  case 0x50:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SPARC_FAMILY), gShellDebug1HiiHandle);
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
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_68040_FAMILY), gShellDebug1HiiHandle);
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

  case 0x70:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_HOBBIT_FAMILY), gShellDebug1HiiHandle);
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
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_WEITEK), gShellDebug1HiiHandle);
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
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_OPTERON_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0x8B:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_OPTERON_THIRD_GENERATION), gShellDebug1HiiHandle);
    break;

  case 0x8C:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_PHENOM_FX_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0x8D:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_PHENOM_X4_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0x8E:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_PHENOM_X2_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0x8F:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_ATHLON_X2_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0x90:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PA_RISC_FAMILY), gShellDebug1HiiHandle);
    break;

  case 0xA0:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_V30_FAMILY), gShellDebug1HiiHandle);
    break;

  case 0xA1:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_3200_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0xA2:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_3000_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0xA3:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5300_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0xA4:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5100_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0xA5:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5000_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0xA6:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_LV_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0xA7:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_ULV_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0xA8:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7100_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0xA9:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5400_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0xAA:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0xAB:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5200_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0xAC:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7200_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0xAD:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7300_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0xAE:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7400_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0xAF:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7400_SERIES_MULTI_CORE), gShellDebug1HiiHandle);
    break;

  case 0xB0:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PENTIUM_III_XEON), gShellDebug1HiiHandle);
    break;

  case 0xC0:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE2_SOLO), gShellDebug1HiiHandle);
    break;

  case 0xC1:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE2_EXTREME), gShellDebug1HiiHandle);
    break;

  case 0xC2:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE2_QUAD), gShellDebug1HiiHandle);
    break;

  case 0xC3:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE2_EXTREME), gShellDebug1HiiHandle);
    break;

  case 0xC4:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE2_DUO_MOBILE), gShellDebug1HiiHandle);
    break;

  case 0xC5:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE2_SOLO_MOBILE), gShellDebug1HiiHandle);
    break;

  case 0xC6:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CORE_I7), gShellDebug1HiiHandle);
    break;

  case 0xC7:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_CELERON_DUAL_CORE), gShellDebug1HiiHandle);
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
    Print (L"zArchitectur\n");
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
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_MULTI_CORE), gShellDebug1HiiHandle);
    break;

  case 0xD7:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_3_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0xD8:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_3_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0xDA:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0xDB:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_5_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0xDD:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7_SERIES_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0xDE:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7_SERIES_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0xDF:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INTEL_XEON_7_SERIES_MULTI_CORE), gShellDebug1HiiHandle);
    break;

  case 0xE6:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_EMBEDDED_OPTERON_QUAD_CORE), gShellDebug1HiiHandle);
    break;

  case 0xE7:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_PHENOM_TRIPLE_CORE), gShellDebug1HiiHandle);
    break;

  case 0xE8:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_TURION_ULTRA_DUAL_CORE_MOBILE), gShellDebug1HiiHandle);
    break;

  case 0xE9:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_TURION_DUAL_CORE_MOBILE), gShellDebug1HiiHandle);
    break;

  case 0xEA:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_ATHLON_DUAL_CORE), gShellDebug1HiiHandle);
    break;

  case 0xEB:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AMD_SEMPRON_SI), gShellDebug1HiiHandle);
    break;

  case 0xFA:
    Print (L"i860\n");
    break;

  case 0xFB:
    Print (L"i960\n");
    break;

  default:
    //
    // In order to reduce code quality notice of
    // case & break not pair, so
    // move multiple case into the else part and
    // use if/else to check value.
    //
    if (Family >= 0x13 && Family <= 0x17) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RSVD_FOR_SPEC_M1), gShellDebug1HiiHandle);
    } else if (Family >= 0x1A && Family <= 0x1F) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RSVD_FOR_SPEC_K5), gShellDebug1HiiHandle);
    } else if (Family >= 0xB1 && Family <= 0xBF) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RSVD_FOR_SPEC_PENTIUM), gShellDebug1HiiHandle);
    } else {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNDEFINED_PROC_FAMILY), gShellDebug1HiiHandle);
    }
  }
  //
  // end switch
  //
}

VOID
DisplayProcessorFamily2 (
  UINT16 Family2,
  UINT8  Option
  )
{
  //
  // Print prompt message
  //
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PROCESSOR_FAMILY), gShellDebug1HiiHandle);

  //
  // Print option
  //
  PRINT_INFO_OPTION (Family2, Option);

  //
  // Use switch to check
  //
  switch (Family2) {
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

    default:
     ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNDEFINED_PROC_FAMILY), gShellDebug1HiiHandle);
  }

}

VOID
DisplayProcessorVoltage (
  UINT8 Voltage,
  UINT8 Option
  )
/*++
Routine Description:
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

Arguments:
  Voltage  - The Voltage
  Option   - The option

Returns:

**/
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PROC_INFO), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (Voltage, Option);

  if (BIT (Voltage, 7) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PROC_CURRENT_VOLTAGE), gShellDebug1HiiHandle, (Voltage - 0x80));
  } else {
    if (BIT (Voltage, 0) != 0) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_5V_SUPOPRTED), gShellDebug1HiiHandle);
    }

    if (BIT (Voltage, 1) != 0) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_33V_SUPPORTED), gShellDebug1HiiHandle);
    }

    if (BIT (Voltage, 2) != 0) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_29V_SUPPORTED), gShellDebug1HiiHandle);
    }
    //
    // check the reserved zero bits:
    //
    if (BIT (Voltage, 3) != 0) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT3_NOT_ZERO), gShellDebug1HiiHandle);
    }

    if (BIT (Voltage, 4) != 0) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT4_NOT_ZERO), gShellDebug1HiiHandle);
    }

    if (BIT (Voltage, 5) != 0) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT5_NOT_ZERO), gShellDebug1HiiHandle);
    }

    if (BIT (Voltage, 6) != 0) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BIT6_NOT_ZERO), gShellDebug1HiiHandle);
    }
  }
}

VOID
DisplayProcessorStatus (
  UINT8 Status,
  UINT8 Option
  )
/*++
Routine Description:

Bit 7 Reserved, must be 0
Bit 6   CPU Socket Populated
 1 - CPU Socket Populated
 0 - CPU Socket UnpopulatedBits
 5:3  Reserved, must be zero
 Bits 2:0 CPU Status
  0h - Unknown
  1h - CPU Enabled
  2h - CPU Disabled by User via BIOS Setup
  3h - CPU Disabled By BIOS (POST Error)
  4h - CPU is Idle, waiting to be enabled.
  5-6h - Reserved
  7h - Other

Arguments:
  Status  - The status
  Option  - The option

Returns:

**/
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PROC_STATUS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Status, Option);

  if (BIT (Status, 7) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ERROR_BIT7), gShellDebug1HiiHandle);
  } else if (BIT (Status, 5) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ERROR_BIT5), gShellDebug1HiiHandle);
  } else if (BIT (Status, 4) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ERROR_BIT4), gShellDebug1HiiHandle);
  } else if (BIT (Status, 3) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ERROR_BIT3), gShellDebug1HiiHandle);
  }
  //
  // Check BIT 6
  //
  if (BIT (Status, 6) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CPU_SOCKET_POPULATED), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CPU_SOCKET_UNPOPULATED), gShellDebug1HiiHandle);
  }
  //
  // Check BITs 2:0
  //
  switch (Status & 0x07) {
  case 0:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
    break;

  case 1:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CPU_ENABLED), gShellDebug1HiiHandle);
    break;

  case 2:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CPU_DISABLED_BY_USER), gShellDebug1HiiHandle);
    break;

  case 3:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CPU_DIABLED_BY_BIOS), gShellDebug1HiiHandle);
    break;

  case 4:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CPU_IDLE), gShellDebug1HiiHandle);
    break;

  case 7:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OTHERS), gShellDebug1HiiHandle);
    break;

  default:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED), gShellDebug1HiiHandle);
  }
}

VOID
DisplayMaxMemoryModuleSize (
  UINT8 Size,
  UINT8 SlotNum,
  UINT8 Option
  )
{
  UINTN MaxSize;
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SIZE_LARGEST_MEM), gShellDebug1HiiHandle);
  //
  // MaxSize is determined by follow formula
  //
  MaxSize = (UINTN) 1 << Size;
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ONE_VAR_MB), gShellDebug1HiiHandle, MaxSize);

  if (Option >= SHOW_DETAIL) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MAX_AMOUNT_MEM), gShellDebug1HiiHandle);
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ONE_VAR_MB), gShellDebug1HiiHandle, MaxSize, SlotNum, MaxSize * SlotNum);
  }
}

VOID
DisplayMemoryModuleConfigHandles (
  UINT16 *Handles,
  UINT8  SlotNum,
  UINT8  Option
  )
{
  UINT8 Index;
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_HANDLES_CONTROLLED), gShellDebug1HiiHandle, SlotNum);

  if (Option >= SHOW_DETAIL) {
    //
    // No handle, Handles is INVALID.
    //
    if (SlotNum == 0) {
      return ;
    }

    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_HANDLES_LIST_CONTROLLED), gShellDebug1HiiHandle);
    for (Index = 0; Index < SlotNum; Index++) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_HANDLE), gShellDebug1HiiHandle, Index + 1, Handles[Index]);
    }
  }
}
//
// Memory Module Information (Type 6)
//
VOID
DisplayMmBankConnections (
  UINT8 BankConnections,
  UINT8 Option
  )
{
  UINT8 High;
  UINT8 Low;

  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BANK_CONNECTIONS), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (BankConnections, Option);

  //
  // Divide it to high and low
  //
  High  = (UINT8) (BankConnections & 0xF0);
  Low   = (UINT8) (BankConnections & 0x0F);
  if (High != 0xF) {
    if (Low != 0xF) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BANK_RAS), gShellDebug1HiiHandle, High, Low, High, Low);
    } else {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BANK_RAS_2), gShellDebug1HiiHandle, High, High);
    }
  } else {
    if (Low != 0xF) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BANK_RAS_2), gShellDebug1HiiHandle, Low, Low);
    } else {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NO_BANKS_CONNECTED), gShellDebug1HiiHandle);
    }
  }
}

VOID
DisplayMmMemorySize (
  UINT8 Size,
  UINT8 Option
  )
/*++
Routine Description:
  Bits 0:6  Size (n),
      where 2**n is the size in MB with three special-case values:
      7Dh Not determinable (Installed Size only)
      7Eh Module is installed, but no memory has been enabled
      7Fh Not installed
  Bit  7  Defines whether the memory module has a single- (0)
          or double-bank (1) connection.

Arguments:
  Size   - The size
  Option - The option

Returns:

**/
{
  UINT8 Value;
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MEMORY_SIZE), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (Size, Option);

  //
  // Get the low bits(0-6 bit)
  //
  Value = (UINT8) (Size & 0x7F);
  if (Value == 0x7D) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MEM_SIZE_NOT_DETERMINABLE), gShellDebug1HiiHandle);
  } else if (Value == 0x7E) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MODULE_INSTALLED), gShellDebug1HiiHandle);
  } else if (Value == 0x7F) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NOT_INSTALLED), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MEM_SIZE), gShellDebug1HiiHandle, 1 << Value);
  }

  if (BIT (Size, 7) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MEM_MODULE_DOUBLE_BANK), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MEM_MODULE_SINGLE_BANK), gShellDebug1HiiHandle);
  }
}

VOID
DisplaySystemSlotId (
  UINT16  SlotId,
  UINT8   SlotType,
  UINT8   Option
  )
/*++
Routine Description:

  The Slot ID field of the System Slot structure provides a mechanism to
  correlate the physical attributes of the slot to its logical access method
  (which varies based on the Slot Type field).

Arguments:

  SlotId   - The slot ID
  SlotType - The slot type
  Option   - The Option

Returns:

**/
{
  //
  // Display slot type first
  //
  DisplaySystemSlotType (SlotType, Option);

  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SLOT_ID), gShellDebug1HiiHandle);
  //
  // print option
  //
  PRINT_INFO_OPTION (SlotType, Option);

  switch (SlotType) {
  //
  // Slot Type: MCA
  //
  case 0x04:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_LOGICAL_MICRO_CHAN), gShellDebug1HiiHandle);
    if (SlotId > 0 && SlotId < 15) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ONE_VAR_D), gShellDebug1HiiHandle, SlotId);
    } else {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ERROR_NOT_1_15), gShellDebug1HiiHandle);
    }
    break;

  //
  // EISA
  //
  case 0x05:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_LOGICAL_EISA_NUM), gShellDebug1HiiHandle);
    if (SlotId > 0 && SlotId < 15) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ONE_VAR_D), gShellDebug1HiiHandle, SlotId);
    } else {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ERROR_NOT_1_15), gShellDebug1HiiHandle);
    }
    break;

  //
  // Slot Type: PCI
  //
  case 0x06:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VALUE_PRESENT), gShellDebug1HiiHandle, SlotId);
    break;

  //
  // PCMCIA
  //
  case 0x07:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_IDENTIFIES_ADAPTER_NUM), gShellDebug1HiiHandle, SlotId);
    break;

  //
  // Slot Type: PCI-E
  //
  case 0xA5:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VALUE_PRESENT), gShellDebug1HiiHandle, SlotId);
    break;

  default:
    if (SlotType >= 0x0E && SlotType <= 0x12) {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VALUE_PRESENT), gShellDebug1HiiHandle, SlotId);
    } else {
      ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNDEFINED_SLOT_ID), gShellDebug1HiiHandle);
    }
  }
}

VOID
DisplaySystemBootStatus (
  UINT8 Parameter,
  UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SYSTEM_BOOT_STATUS), gShellDebug1HiiHandle);
  //
  // Print option
  //
  PRINT_INFO_OPTION (Parameter, Option);

  //
  // Check value and print
  //
  if (Parameter == 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NO_ERRORS_DETECTED), gShellDebug1HiiHandle);
  } else if (Parameter == 1) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NO_BOOTABLE_MEDIA), gShellDebug1HiiHandle);
  } else if (Parameter == 2) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NORMAL_OP_SYSTEM), gShellDebug1HiiHandle);
  } else if (Parameter == 3) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_FIRMWARE_DETECTED), gShellDebug1HiiHandle);
  } else if (Parameter == 4) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OP_SYSTEM), gShellDebug1HiiHandle);
  } else if (Parameter == 5) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_USER_REQUESTED_BOOT), gShellDebug1HiiHandle);
  } else if (Parameter == 6) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SYSTEM_SECURITY_VIOLATION), gShellDebug1HiiHandle);
  } else if (Parameter == 7) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PREV_REQ_IMAGE), gShellDebug1HiiHandle);
  } else if (Parameter == 8) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_WATCHDOG_TIMER), gShellDebug1HiiHandle);
  } else if (Parameter >= 9 && Parameter <= 127) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RSVD_FUTURE_ASSIGNMENT), gShellDebug1HiiHandle);
  } else if (Parameter >= 128 && Parameter <= 191) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_VENDOR_OEM_SPECIFIC), gShellDebug1HiiHandle);
  } else if (Parameter >= 192 && Parameter <= 255) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_PRODUCT_SPEC_IMPLMENTATION), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ERROR_VALUE), gShellDebug1HiiHandle);
  }
}
//
// Portable Battery (Type 22)
//
VOID
DisplaySBDSManufactureDate (
  UINT16  Date,
  UINT8   Option
  )
/*++
Routine Description:
  The date the cell pack was manufactured, in packed format:
   Bits 15:9  Year, biased by 1980, in the range 0 to 127.
   Bits 8:5 Month, in the range 1 to 12.
   Bits 4:0 Date, in the range 1 to 31.
  For example, 01 February 2000 would be identified as
  0010 1000 0100 0001b (0x2841).

Arguments:
  Date   - The date
  Option - The option

Returns:

**/
{
  UINTN Day;
  UINTN Month;
  UINTN Year;

  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SBDS_MANUFACTURE_DATE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Date, Option);
  //
  // Print date
  //
  Day   = Date & 0x001F;
  Month = (Date & 0x00E0) >> 5;
  Year  = ((Date & 0xFF00) >> 8) + 1980;
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MONTH_DAY_YEAR), gShellDebug1HiiHandle, Day, Month, Year);

}
//
// System Reset  (Type 23)
//
VOID
DisplaySystemResetCapabilities (
  UINT8 Reset,
  UINT8 Option
  )
/*++
Routine Description:
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

Arguments:
  Reset   - Reset
  Option  - The option

Returns:

**/
{
  UINTN Temp;

  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SYSTEM_RESET_CAPABILITIES), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Reset, Option);

  //
  // Check reserved bits 7:6
  //
  if ((Reset & 0xC0) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_RESERVED_ZERO), gShellDebug1HiiHandle);
  }
  //
  // Watch dog
  //
  if (BIT (Reset, 5) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_WATCHDOG_TIMER_2), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SYSTEM_NOT_CONTAIN_TIMER), gShellDebug1HiiHandle);
  }
  //
  // Boot Option on Limit
  //
  Temp = (Reset & 0x18) >> 3;
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BOOT_OPTION_LIMIT), gShellDebug1HiiHandle);
  switch (Temp) {
  case 0:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED), gShellDebug1HiiHandle);
    break;

  case 1:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OP_SYSTEM_2), gShellDebug1HiiHandle);
    break;

  case 2:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SYSTEM_UTIL), gShellDebug1HiiHandle);
    break;

  case 3:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DO_NOT_REBOOT_BITS), gShellDebug1HiiHandle);
    break;
  }
  //
  // Boot Option
  //
  Temp = (Reset & 0x06) >> 1;
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BOOT_OPTION), gShellDebug1HiiHandle);
  switch (Temp) {
  case 0:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED), gShellDebug1HiiHandle);
    break;

  case 1:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OP_SYSTEM_2), gShellDebug1HiiHandle);
    break;

  case 2:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SYSTEM_UTIL), gShellDebug1HiiHandle);
    break;

  case 3:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DO_NOT_REBOOT), gShellDebug1HiiHandle);
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
//
// Hardware Security (Type 24)
//
VOID
DisplayHardwareSecuritySettings (
  UINT8 Settings,
  UINT8 Option
  )
/*++
Routine Description:
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

Arguments:
  Settings    - The settings
  Option      - the option

Returns:

**/
{
  UINTN Temp;

  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_HARDWARE_SECURITY_SET), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Settings, Option);

  //
  // Power-on Password Status
  //
  Temp = (Settings & 0xC0) >> 6;
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_ON_PASSWORD), gShellDebug1HiiHandle);
  switch (Temp) {
  case 0:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DISABLED), gShellDebug1HiiHandle);
    break;

  case 1:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENABLED_NEWLINE), gShellDebug1HiiHandle);
    break;

  case 2:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NOT_IMPLEMENTED), gShellDebug1HiiHandle);
    break;

  case 3:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
    break;
  }
  //
  // Keyboard Password Status
  //
  Temp = (Settings & 0x30) >> 4;
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_KEYBOARD_PASSWORD), gShellDebug1HiiHandle);
  switch (Temp) {
  case 0:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DISABLED), gShellDebug1HiiHandle);
    break;

  case 1:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENABLED_NEWLINE), gShellDebug1HiiHandle);
    break;

  case 2:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NOT_IMPLEMENTED), gShellDebug1HiiHandle);
    break;

  case 3:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
    break;
  }
  //
  // Administrator Password Status
  //
  Temp = (Settings & 0x0C) >> 2;
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ADMIN_PASSWORD_STATUS), gShellDebug1HiiHandle);
  switch (Temp) {
  case 0:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DISABLED), gShellDebug1HiiHandle);
    break;

  case 1:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENABLED_NEWLINE), gShellDebug1HiiHandle);
    break;

  case 2:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NOT_IMPLEMENTED), gShellDebug1HiiHandle);
    break;

  case 3:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
    break;
  }
  //
  // Front Panel Reset Status
  //
  Temp = Settings & 0x3;
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_FRONT_PANEL_RESET), gShellDebug1HiiHandle);
  switch (Temp) {
  case 0:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_DISABLED), gShellDebug1HiiHandle);
    break;

  case 1:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_ENABLED_NEWLINE), gShellDebug1HiiHandle);
    break;

  case 2:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NOT_IMPLEMENTED), gShellDebug1HiiHandle);
    break;

  case 3:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
    break;
  }
}
//
// Out-of-Band Remote Access (Type 30)
//
VOID
DisplayOBRAConnections (
  UINT8   Connections,
  UINT8   Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CONNECTIONS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Connections, Option);

  //
  // Check reserved bits 7:2
  //
  if ((Connections & 0xFC) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_RESERVED_ZERO_2), gShellDebug1HiiHandle);
  }
  //
  // Outbound Connection
  //
  if (BIT (Connections, 1) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OUTBOUND_CONN_ENABLED), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OTUBOUND_CONN_DISABLED), gShellDebug1HiiHandle);
  }
  //
  // Inbound Connection
  //
  if (BIT (Connections, 0) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INBOIUND_CONN_ENABLED), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INBOUND_CONN_DISABLED), gShellDebug1HiiHandle);
  }
}
//
// System Power Supply (Type 39)
//
VOID
DisplaySPSCharacteristics (
  UINT16  Characteristics,
  UINT8   Option
  )
{
  UINTN Temp;

  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_CHAR), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Characteristics, Option);

  //
  // Check reserved bits 15:14
  //
  if ((Characteristics & 0xC000) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BITS_15_14_RSVD), gShellDebug1HiiHandle);
  }
  //
  // Bits 13:10 - DMTF Power Supply Type
  //
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_TYPE), gShellDebug1HiiHandle);
  Temp = (Characteristics & 0x1C00) << 10;
  switch (Temp) {
  case 1:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OTHER_SPACE), gShellDebug1HiiHandle);
    break;

  case 2:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
    break;

  case 3:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_LINEAR), gShellDebug1HiiHandle);
    break;

  case 4:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_SWITCHING), gShellDebug1HiiHandle);
    break;

  case 5:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_BATTERY), gShellDebug1HiiHandle);
    break;

  case 6:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UPS), gShellDebug1HiiHandle);
    break;

  case 7:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CONVERTER), gShellDebug1HiiHandle);
    break;

  case 8:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_REGULATOR), gShellDebug1HiiHandle);
    break;

  default:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED_2), gShellDebug1HiiHandle);
  }
  //
  // Bits 9:7 - Status
  //
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_STATUS_DASH), gShellDebug1HiiHandle);
  Temp = (Characteristics & 0x380) << 7;
  switch (Temp) {
  case 1:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OTHER_SPACE), gShellDebug1HiiHandle);
    break;

  case 2:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
    break;

  case 3:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OK), gShellDebug1HiiHandle);
    break;

  case 4:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NON_CRITICAL), gShellDebug1HiiHandle);
    break;

  case 5:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_CRITICAL_POWER_SUPPLY), gShellDebug1HiiHandle);
    break;

  default:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNDEFINED), gShellDebug1HiiHandle);
  }
  //
  // Bits 6:3 - DMTF Input Voltage Range Switching
  //
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_INPUT_VOLTAGE_RANGE), gShellDebug1HiiHandle);
  Temp = (Characteristics & 0x78) << 3;
  switch (Temp) {
  case 1:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_OTHER_SPACE), gShellDebug1HiiHandle);
    break;

  case 2:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_UNKNOWN), gShellDebug1HiiHandle);
    break;

  case 3:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_MANUAL), gShellDebug1HiiHandle);
    break;

  case 4:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_AUTO_SWITCH), gShellDebug1HiiHandle);
    break;

  case 5:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_WIDE_RANGE), gShellDebug1HiiHandle);
    break;

  case 6:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_NOT_APPLICABLE), gShellDebug1HiiHandle);
    break;

  default:
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_RESERVED_3), gShellDebug1HiiHandle);
    break;
  }
  //
  // Power supply is unplugged from the wall
  //
  if (BIT (Characteristics, 2) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_UNPLUGGED), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_PLUGGED), gShellDebug1HiiHandle);
  }
  //
  // Power supply is present
  //
  if (BIT (Characteristics, 1) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_PRESENT), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_NOT_PRESENT), gShellDebug1HiiHandle);
  }
  //
  // hot replaceable
  //
  if (BIT (Characteristics, 0) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_REPLACE), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_PRINTINFO_POWER_SUPPLY_NOT_REPLACE), gShellDebug1HiiHandle);
  }
}
