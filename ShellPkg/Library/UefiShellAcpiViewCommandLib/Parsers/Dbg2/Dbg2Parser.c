/** @file
  DBG2 table parser

  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - Microsoft Debug Port Table 2 (DBG2) Specification - December 10, 2015.
**/

#include <IndustryStandard/DebugPort2Table.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

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
  This function Validates the NameSpace string length.

  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Context Pointer to context specific information e.g. this
                      could be a pointer to the ACPI table header.
**/
STATIC
VOID
EFIAPI
ValidateNameSpaceStrLen (
  IN  UINT8* Ptr,
  IN  VOID*  Context
  );

/**
  This function parses the debug device information structure.

  @param [in]  Ptr     Pointer to the start of the buffer.
  @param [out] Length  Pointer in which the length of the debug
                       device information is returned.
**/
STATIC
VOID
EFIAPI
DumpDbgDeviceInfo (
  IN  UINT8*  Ptr,
  OUT UINT32* Length
  );

/// An ACPI_PARSER array describing the ACPI DBG2 table.
STATIC CONST ACPI_PARSER Dbg2Parser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  {L"OffsetDbgDeviceInfo", 4, 36, L"0x%x", NULL,
   (VOID**)&OffsetDbgDeviceInfo, NULL, NULL},
  {L"NumberDbgDeviceInfo", 4, 40, L"%d", NULL,
   (VOID**)&NumberDbgDeviceInfo, NULL, NULL}
};

/// An ACPI_PARSER array describing the debug device information.
STATIC CONST ACPI_PARSER DbgDevInfoParser[] = {
  {L"Revision", 1, 0, L"0x%x", NULL, NULL, NULL, NULL},
  {L"Length", 2, 1, L"%d", NULL, (VOID**)&DbgDevInfoLen, NULL, NULL},

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
  UINT16 NameSpaceStrLen;

  NameSpaceStrLen = *(UINT16*)Ptr;

  if (NameSpaceStrLen < 2) {
    IncrementErrorCount ();
    Print (
      L"\nERROR: NamespaceString Length = %d. If no Namespace device exists,\n"
       L"    then NamespaceString[] must contain a period '.'",
      NameSpaceStrLen
      );
  }
}

/**
  This function parses the debug device information structure.

  @param [in]  Ptr     Pointer to the start of the buffer.
  @param [out] Length  Pointer in which the length of the debug
                       device information is returned.
**/
STATIC
VOID
EFIAPI
DumpDbgDeviceInfo (
  IN  UINT8*  Ptr,
  OUT UINT32* Length
  )
{
  UINT16  Index;
  UINT8*  DataPtr;
  UINT32* AddrSize;

  // Parse the debug device info to get the Length
  ParseAcpi (
    FALSE,
    0,
    "Debug Device Info",
    Ptr,
    3,  // Length is 2 bytes starting at offset 1
    PARSER_PARAMS (DbgDevInfoParser)
    );

  ParseAcpi (
    TRUE,
    2,
    "Debug Device Info",
    Ptr,
    *DbgDevInfoLen,
    PARSER_PARAMS (DbgDevInfoParser)
    );

  // GAS and Address Size
  Index = 0;
  DataPtr = Ptr + (*BaseAddrRegOffset);
  AddrSize = (UINT32*)(Ptr + (*AddrSizeOffset));
  while (Index < (*GasCount)) {
    PrintFieldName (4, L"BaseAddressRegister");
    DumpGasStruct (DataPtr, 4);
    PrintFieldName (4, L"Address Size");
    Print (L"0x%x\n", AddrSize[Index]);
    DataPtr += GAS_LENGTH;
    Index++;
  }

  // NameSpace String
  Index = 0;
  DataPtr = Ptr + (*NameSpaceStringOffset);
  PrintFieldName (4, L"NameSpace String");
  while (Index < (*NameSpaceStringLength)) {
    Print (L"%c", DataPtr[Index++]);
  }
  Print (L"\n");

  // OEM Data
  Index = 0;
  DataPtr = Ptr + (*OEMDataOffset);
  PrintFieldName (4, L"OEM Data");
  while (Index < (*OEMDataLength)) {
    Print (L"%x ", DataPtr[Index++]);
    if ((Index & 7) == 0) {
      Print (L"\n%-*s   ", OUTPUT_FIELD_COLUMN_WIDTH, L"");
    }
  }

  *Length = *DbgDevInfoLen;
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
  UINT32 DbgDeviceInfoLength;
  UINT8* DevInfoPtr;

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
  DevInfoPtr = Ptr + Offset;

  while (Offset < AcpiTableLength) {
    DumpDbgDeviceInfo (
      DevInfoPtr,
      &DbgDeviceInfoLength
      );
    Offset += DbgDeviceInfoLength;
    DevInfoPtr += DbgDeviceInfoLength;
  }
}
