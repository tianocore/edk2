/** @file
  DBG2 table parser

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - Microsoft Debug Port Table 2 (DBG2) Specification - December 10, 2015.
**/

#include <IndustryStandard/DebugPort2Table.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiViewLog.h"

// Local variables pointing to the table fields
STATIC CONST UINT32* OffsetDbgDeviceInfo;
STATIC CONST UINT32* NumberDbgDeviceInfo;
STATIC CONST UINT16* DbgDevInfoLen;
STATIC CONST UINT8*  GasCount;
STATIC CONST UINT16* NameSpaceStringLength;
STATIC CONST UINT16* NameSpaceStringOffset;
STATIC CONST UINT16* OEMDataLength;
STATIC CONST UINT16* OEMDataOffset;
STATIC CONST UINT16* BaseAddrRegOffset;
STATIC CONST UINT16* AddrSizeOffset;
STATIC ACPI_DESCRIPTION_HEADER_INFO AcpiHdrInfo;

/**
  This function validates the NameSpace string length.

  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateNameSpaceStrLen (
  IN UINT8* Ptr,
  IN VOID*  Context
  )
{
  UINT16 NameSpaceStrLen = *(UINT16 *) Ptr;

  if (AssertConstraint (L"ACPI", NameSpaceStrLen > 1)) {
    AcpiInfo (L"With no namespace, NamespaceString[] must be a period '.'");
  }
}

/// An ACPI_PARSER array describing the ACPI DBG2 table.
STATIC CONST ACPI_PARSER Dbg2Parser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  {L"OffsetDbgDeviceInfo", 4, 36, L"0x%x", NULL,
   (VOID**)&OffsetDbgDeviceInfo, NULL, NULL},
  {L"NumberDbgDeviceInfo", 4, 40, L"%d", NULL,
   (VOID**)&NumberDbgDeviceInfo, NULL, NULL}
};

/// An ACPI_PARSER array describing the debug device information structure
/// header.
STATIC CONST ACPI_PARSER DbgDevInfoHeaderParser[] = {
  {L"Revision", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 2, 1, L"%d", NULL, (VOID**)&DbgDevInfoLen, NULL, NULL}
};

/// An ACPI_PARSER array describing the debug device information.
STATIC CONST ACPI_PARSER DbgDevInfoParser[] = {
  {L"Revision", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 2, 1, L"%d", NULL, NULL, NULL, NULL},

  {L"Generic Address Registers Count", 1, 3, L"0x%x", NULL,
   (VOID**)&GasCount, NULL, NULL},
  {L"NameSpace String Length", 2, 4, L"%d", NULL,
   (VOID**)&NameSpaceStringLength, ValidateNameSpaceStrLen, NULL},
  {L"NameSpace String Offset", 2, 6, L"0x%x", NULL,
   (VOID**)&NameSpaceStringOffset, NULL, NULL},
  {L"OEM Data Length", 2, 8, L"%d", NULL, (VOID**)&OEMDataLength,
   NULL, NULL},
  {L"OEM Data Offset", 2, 10, L"0x%x", NULL, (VOID**)&OEMDataOffset,
   NULL, NULL},

  {L"Port Type", 2, 12, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Port SubType", 2, 14, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Reserved", 2, 16, L"%x", NULL, NULL, NULL, NULL},

  {L"Base Address Register Offset", 2, 18, L"0x%x", NULL,
   (VOID**)&BaseAddrRegOffset, NULL, NULL},
  {L"Address Size Offset", 2, 20, L"0x%x", NULL,
   (VOID**)&AddrSizeOffset, NULL, NULL}
};

/**
  This function parses the debug device information structure.

  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  Length of the debug device information structure.
**/
STATIC
VOID
EFIAPI
DumpDbgDeviceInfo (
  IN UINT8* Ptr,
  IN UINT16 Length
  )
{
  UINT16  Index;
  UINT16  Offset;

  ParseAcpi (
    TRUE,
    2,
    "Debug Device Info",
    Ptr,
    Length,
    PARSER_PARAMS (DbgDevInfoParser)
    );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if ((GasCount == NULL)              ||
      (NameSpaceStringLength == NULL) ||
      (NameSpaceStringOffset == NULL) ||
      (OEMDataLength == NULL)         ||
      (OEMDataOffset == NULL)         ||
      (BaseAddrRegOffset == NULL)     ||
      (AddrSizeOffset == NULL)) {
    AcpiError (ACPI_ERROR_PARSE, L"Failed to parse DbgDevInfo Structure");
    return;
  }

  // GAS
  Offset = *BaseAddrRegOffset;
  for (Index = 0; Index < *GasCount; Index++) {
    if (AssertMemberIntegrity (Offset, 1, Ptr, Length)) {
      break;
    }

    PrintFieldName (4, L"BaseAddressRegister[%d]", Index);
    Offset += (UINT16)DumpGasStruct (Ptr + Offset, 4, Length - Offset);
  }

  // Make sure the array of address sizes corresponding to each GAS fit in the
  // Debug Device Information structure
  if (AssertMemberIntegrity (
        *AddrSizeOffset, *GasCount * sizeof (UINT32), Ptr, Length)) {
    return;
  }

  // Address Size
  Offset = *AddrSizeOffset;
  for (Index = 0; Index < *GasCount; Index++) {
    if (AssertMemberIntegrity (Offset, 1, Ptr, Length)) {
      break;
    }
    PrintFieldName (4, L"Address Size[%d]", Index);
    AcpiInfo (L"0x%x", ReadUnaligned32 ((CONST UINT32 *)(Ptr + Offset)));
    Offset += sizeof (UINT32);
  }

  // NameSpace String
  PrintFieldName (4, L"NameSpace String");
  if (!AssertMemberIntegrity (
        *NameSpaceStringOffset, *NameSpaceStringLength, Ptr, Length)) {
    AcpiInfo (L"%-.*a", *NameSpaceStringLength - 1, Ptr + *NameSpaceStringOffset);
  }

  // OEM Data
  if (*OEMDataOffset != 0) {
    AcpiInfo (L"OEM Data");
    DumpRaw (Ptr + *OEMDataOffset, *OEMDataLength);
  }
}

/**
  This function parses the ACPI DBG2 table.
  When trace is enabled this function parses the DBG2 table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiDbg2 (
  IN BOOLEAN Trace,
  IN UINT8*  Ptr,
  IN UINT32  AcpiTableLength,
  IN UINT8   AcpiTableRevision
  )
{
  UINT32 Offset;
  UINT32 Index;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "DBG2",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (Dbg2Parser)
             );

  // Check if the values used to control the parsing logic have been
  // successfully read.
  if ((OffsetDbgDeviceInfo == NULL) || (NumberDbgDeviceInfo == NULL)) {
    AcpiError (ACPI_ERROR_PARSE, L"Failed to parse DbgDevInfo array");
    return;
  }

  Offset = *OffsetDbgDeviceInfo;
  Index = 0;

  while (Index++ < *NumberDbgDeviceInfo) {
    // Parse the Debug Device Information Structure header to obtain Length
    ParseAcpi (
      FALSE,
      0,
      NULL,
      Ptr + Offset,
      AcpiTableLength - Offset,
      PARSER_PARAMS (DbgDevInfoHeaderParser)
      );

    // Check if the values used to control the parsing logic have been
    // successfully read.
    if (DbgDevInfoLen == NULL) {
      AcpiError (ACPI_ERROR_PARSE, L"Failed to parse DbgDevInfoLen");
      return;
    }

    // Validate Debug Device Information Structure length
    if (AssertConstraint (L"ACPI", *DbgDevInfoLen > 0)) {
      return;
    }

    if (AssertMemberIntegrity (Offset, *DbgDevInfoLen, Ptr, AcpiTableLength)) {
      return;
    }

    DumpDbgDeviceInfo (Ptr + Offset, *DbgDevInfoLen);
    Offset += *DbgDevInfoLen;
  }
}
