/** @file
  Module for clarifying the content of the smbios structure element info.

  Copyright (c) 2005 - 2019, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"
#include "PrintInfo.h"
#include "QueryTable.h"
#include "EventLogInfo.h"

/**
  Function to display system event log access information.

  @param[in] Key    Additional information to print.
  @param[in] Option Whether to print the additional information.
**/
VOID
DisplaySELAccessMethod (
  IN CONST UINT8  Key,
  IN CONST UINT8  Option
  )
{
  //
  // Print prompt
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_ACCESS_METHOD), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Key, Option);

  //
  // Print value info
  //
  switch (Key) {
    case 0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_ONE_EIGHT_BIT), gShellDebug1HiiHandle);
      break;

    case 1:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_TWO_EIGHT_BITS), gShellDebug1HiiHandle);
      break;

    case 2:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_ONE_SIXTEEN_BIT), gShellDebug1HiiHandle);
      break;

    case 3:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_MEM_MAPPED_PHYS), gShellDebug1HiiHandle);
      break;

    case 4:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_AVAIL_VIA_GENERAL), gShellDebug1HiiHandle);
      break;

    default:
      if (Key <= 0x7f) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_AVAIL_FOR_FUTURE_ASSIGN), gShellDebug1HiiHandle);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_BIOS_VENDOR_OEM), gShellDebug1HiiHandle);
      }
  }
}

/**
  Function to display system event log status information.

  @param[in] Key    Additional information to print.
  @param[in] Option Whether to print the additional information.
**/
VOID
DisplaySELLogStatus (
  UINT8  Key,
  UINT8  Option
  )
{
  //
  // Print prompt
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_LOG_STATUS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Key, Option);

  //
  // Print value info
  //
  if ((Key & 0x01) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_LOG_AREA_VALID), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_LOG_AREA_VALID), gShellDebug1HiiHandle);
  }

  if ((Key & 0x02) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_LOG_AREA_FULL), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_LOG_AREA_NOT_FULL), gShellDebug1HiiHandle);
  }

  if ((Key & 0xFC) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_RES_BITS_NOT_ZERO), gShellDebug1HiiHandle, Key & 0xFC);
  }
}

/**
  Function to display system event log header format information.

  @param[in] Key    Additional information to print.
  @param[in] Option Whether to print the additional information.
**/
VOID
DisplaySysEventLogHeaderFormat (
  UINT8  Key,
  UINT8  Option
  )
{
  //
  // Print prompt
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_LOG_HEADER_FORMAT), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Key, Option);

  //
  // Print value info
  //
  if (Key == 0x00) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_NO_HEADER), gShellDebug1HiiHandle);
  } else if (Key == 0x01) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_TYPE_LOG_HEADER), gShellDebug1HiiHandle);
  } else if (Key <= 0x7f) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_AVAIL_FOR_FUTURE), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_BIOS_VENDOR), gShellDebug1HiiHandle);
  }
}

/**
  Display the header information for SEL log items.

  @param[in] Key      The information key.
  @param[in] Option   The option index.
**/
VOID
DisplaySELLogHeaderLen (
  UINT8  Key,
  UINT8  Option
  )
{
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_LOG_HEADER_LEN), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Key, Option);

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_ONE_VAR_D), gShellDebug1HiiHandle, Key & 0x7F);

  //
  // The most-significant bit of the field specifies
  // whether (0) or not (1) the record has been read
  //
  if ((Key & 0x80) != 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_THIS_RECORD_READ), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_THIS_RECORD_NOT_READ), gShellDebug1HiiHandle);
  }
}

/**
  Display the header information for type 1 items.

  @param[in] LogHeader      The buffer with the information.
**/
VOID
DisplaySysEventLogHeaderType1 (
  IN UINT8  *LogHeader
  )
{
  LOG_HEADER_TYPE1_FORMAT  *Header;

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_SYSTEM_EVENT_LOG), gShellDebug1HiiHandle);

  //
  // Print Log Header Type1 Format info
  //
  Header = (LOG_HEADER_TYPE1_FORMAT *)(LogHeader);

  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_OEM_RESERVED),
    gShellDebug1HiiHandle,
    Header->OEMReserved[0],
    Header->OEMReserved[1],
    Header->OEMReserved[2],
    Header->OEMReserved[3],
    Header->OEMReserved[4]
    );
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_MULTIPLE_EVENT_TIME), gShellDebug1HiiHandle, Header->Metw);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_MULTIPLE_EVENT_COUNT), gShellDebug1HiiHandle, Header->Meci);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_PREBOOT_ADDRESS), gShellDebug1HiiHandle, Header->CMOSAddress);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_PREBOOT_INDEX), gShellDebug1HiiHandle, Header->CMOSBitIndex);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_CHECKSUM_STARTING_OFF), gShellDebug1HiiHandle, Header->StartingOffset);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_CHECKSUN_BYTE_COUNT), gShellDebug1HiiHandle, Header->ChecksumOffset);
  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_RESERVED),
    gShellDebug1HiiHandle,
    Header->OEMReserved[0],
    Header->OEMReserved[1],
    Header->OEMReserved[2]
    );
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_HEADER_REVISION), gShellDebug1HiiHandle, Header->HeaderRevision);
}

/**
  Function to display system event log header information.

  @param[in] LogHeaderFormat  Format identifier.
  @param[in] LogHeader        Format informcation.
**/
VOID
DisplaySysEventLogHeader (
  UINT8  LogHeaderFormat,
  UINT8  *LogHeader
  )
{
  //
  // Print prompt
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_LOG_HEADER), gShellDebug1HiiHandle);

  //
  // Print value info
  //
  if (LogHeaderFormat == 0x00) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_NO_HEADER), gShellDebug1HiiHandle);
  } else if (LogHeaderFormat == 0x01) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_TYPE_LOG_HEADER), gShellDebug1HiiHandle);
    DisplaySysEventLogHeaderType1 (LogHeader);
  } else if (LogHeaderFormat <= 0x7f) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_AVAIL_FUTURE_ASSIGN), gShellDebug1HiiHandle);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_BIOS_VENDOR), gShellDebug1HiiHandle);
  }
}

/**
  Display the El Vdf information.

  @param[in] ElVdfType    The information type.
  @param[in] VarData      The information buffer.
**/
VOID
DisplayElVdfInfo (
  UINT8  ElVdfType,
  UINT8  *VarData
  )
{
  UINT16  *Word;
  UINT32  *Dword;

  //
  // Display Type Name
  //
  DisplaySELVarDataFormatType (ElVdfType, SHOW_DETAIL);

  //
  // Display Type description
  //
  switch (ElVdfType) {
    case 0:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_NO_STD_FORMAT), gShellDebug1HiiHandle);
      break;

    case 1:
      Word = (UINT16 *)(VarData + 1);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_SMBIOS_STRUCT_ASSOC), gShellDebug1HiiHandle);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_STRUCT_HANDLE), gShellDebug1HiiHandle, *Word);
      break;

    case 2:
      Dword = (UINT32 *)(VarData + 1);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_MULT_EVENT_COUNTER), gShellDebug1HiiHandle, *Dword);
      break;

    case 3:
      Word = (UINT16 *)(VarData + 1);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_SMBIOS_STRUCT_ASSOC), gShellDebug1HiiHandle);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_STRUCT_HANDLE), gShellDebug1HiiHandle, *Word);
      //
      // Followed by a multiple-event counter
      //
      Dword = (UINT32 *)(VarData + 1);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_MULT_EVENT_COUNTER), gShellDebug1HiiHandle, *Dword);
      break;

    case 4:
      Dword = (UINT32 *)(VarData + 1);
      DisplayPostResultsBitmapDw1 (*Dword, SHOW_DETAIL);
      Dword++;
      DisplayPostResultsBitmapDw2 (*Dword, SHOW_DETAIL);
      break;

    case 5:
      Dword = (UINT32 *)(VarData + 1);
      DisplaySELSysManagementTypes (*Dword, SHOW_DETAIL);
      break;

    case 6:
      Dword = (UINT32 *)(VarData + 1);
      DisplaySELSysManagementTypes (*Dword, SHOW_DETAIL);
      //
      // Followed by a multiple-event counter
      //
      Dword = (UINT32 *)(VarData + 1);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_MULT_EVENT_COUNTER), gShellDebug1HiiHandle, *Dword);
      break;

    default:
      if (ElVdfType <= 0x7F) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_UNUSED_AVAIL_FOR_ASSIGN), gShellDebug1HiiHandle);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_AVAIL_FOR_SYSTEM), gShellDebug1HiiHandle);
      }
  }
}

/**
  Function to display system event log data.

  @param[in] LogData        The data information.
  @param[in] LogAreaLength  Length of the data.
**/
VOID
DisplaySysEventLogData (
  UINT8   *LogData,
  UINT16  LogAreaLength
  )
{
  LOG_RECORD_FORMAT  *Log;
  UINT8              ElVdfType;
  //
  // Event Log Variable Data Format Types
  //
  UINTN  Offset;

  //
  // Print prompt
  //
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_SYSTEM_EVENT_LOG_2), gShellDebug1HiiHandle);

  //
  // Print Log info
  //
  Offset = 0;
  Log    = (LOG_RECORD_FORMAT *)LogData;
  while (Log != NULL && Log->Type != END_OF_LOG && Offset < LogAreaLength) {
    if (Log != NULL) {
      //
      // Display Event Log Record Information
      //
      DisplaySELTypes (Log->Type, SHOW_DETAIL);
      DisplaySELLogHeaderLen (Log->Length, SHOW_DETAIL);

      Offset += Log->Length;
      //
      // Display Log Header Date/Time Fields
      // These fields contain the BCD representation of the date and time
      // (as read from CMOS) of the occurrence of the event
      // So Print as hex and represent decimal
      //
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_DATE), gShellDebug1HiiHandle);
      if ((Log != NULL) && (Log->Year >= 80) && (Log->Year <= 99)) {
        Print (L"19");
      } else if ((Log != NULL) && (Log->Year <= 79)) {
        Print (L"20");
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_ERROR), gShellDebug1HiiHandle);
        //
        // Get a Event Log Record
        //
        Log = (LOG_RECORD_FORMAT *)(LogData + Offset);
        continue;
      }

      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_SMBIOSVIEW_EVENTLOGINFO_TIME_SIX_VARS),
        gShellDebug1HiiHandle,
        Log->Year,
        Log->Month,
        Log->Day,
        Log->Hour,
        Log->Minute,
        Log->Second
        );

      //
      // Display Variable Data Format
      //
      if (Log->Length <= (sizeof (LOG_RECORD_FORMAT) - 1)) {
        //
        // Get a Event Log Record
        //
        Log = (LOG_RECORD_FORMAT *)(LogData + Offset);
        continue;
      }

      ElVdfType = Log->LogVariableData[0];
      DisplayElVdfInfo (ElVdfType, Log->LogVariableData);
      //
      // Get a Event Log Record
      //
      Log = (LOG_RECORD_FORMAT *)(LogData + Offset);
    }
  }
}
