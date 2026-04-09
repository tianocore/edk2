/** @file
  Firmware Performance Data Table (FPDT) table parser

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI 6.5 Specification - August 2022
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiViewConfig.h"

/**
  Parse the Firmware basic boot performance record.
  EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER
**/
VOID
EFIAPI
DumpFbbp (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  );

/**
  Parse the S3 performance table record.
  EFI_ACPI_6_5_FPDT_RECORD_TYPE_S3_PERFORMANCE_TABLE_POINTER

  @param [in] Format  Format string.
  @param [in] Ptr     Pointer to the start of the record.
  @param [in] Length  Length of the field.
**/
VOID
EFIAPI
DumpS3Ptp (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  );

// Local Variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  mAcpiHdrInfo;

/**
  An ACPI_PARSER array describing the ACPI FPDT performance record header.
**/
STATIC CONST ACPI_PARSER  mFpdtPerfRecordParser[] = {
  { L"Type",     2, 0, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Length",   1, 2, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Revision", 1, 3, L"0x%x", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT boot performance pointer record.
**/
STATIC CONST ACPI_PARSER  mFpdtBootPerfPointerParser[] = {
  { L"Type",                        2, 0, L"0x%x", NULL,     NULL, NULL, NULL },
  { L"Length",                      1, 2, L"0x%x", NULL,     NULL, NULL, NULL },
  { L"Revision",                    1, 3, L"0x%x", NULL,     NULL, NULL, NULL },
  { L"Reserved",                    4, 4, L"0x%x", NULL,     NULL, NULL, NULL },
  { L"BootPerformanceTablePointer", 8, 8, NULL,    DumpFbbp, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT S3 performance pointer record.
**/
STATIC CONST ACPI_PARSER  mFpdtS3PerfPointerParser[] = {
  { L"Type",                      2, 0, L"0x%x", NULL,      NULL, NULL, NULL },
  { L"Length",                    1, 2, L"0x%x", NULL,      NULL, NULL, NULL },
  { L"Revision",                  1, 3, L"0x%x", NULL,      NULL, NULL, NULL },
  { L"Reserved",                  4, 4, L"0x%x", NULL,      NULL, NULL, NULL },
  { L"S3PerformanceTablePointer", 8, 8, NULL,    DumpS3Ptp, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT firmware table header.
**/
STATIC CONST ACPI_PARSER  mAcpiFpdtHdrParser[] = {
  { L"Signature", 4, 0, NULL,    Dump4Chars, NULL, NULL, NULL },
  { L"Length",    4, 4, L"0x%x", NULL,       NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT firmware basic boot record.
**/
STATIC CONST ACPI_PARSER  mFpdtBasicBootRecordParser[] = {
  { L"Type",                    2, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",                  1, 2,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Revision",                1, 3,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Reserved",                4, 4,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"ResetEnd",                8, 8,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"OsLoaderLoadImageStart",  8, 16, L"0x%lx", NULL, NULL, NULL, NULL },
  { L"OsLoaderStartImageStart", 8, 24, L"0x%lx", NULL, NULL, NULL, NULL },
  { L"ExitBootServicesEntry",   8, 32, L"0x%lx", NULL, NULL, NULL, NULL },
  { L"ExitBootServicesExit",    8, 40, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT S3 resume record.
**/
STATIC CONST ACPI_PARSER  mFpdtS3ResumeRecordParser[] = {
  { L"Type",          2, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",        1, 2,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Revision",      1, 3,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"ResumeCount",   4, 4,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"FullResume",    8, 8,  L"0x%lx", NULL, NULL, NULL, NULL },
  { L"AverageResume", 8, 16, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT S3 suspend record.
**/
STATIC CONST ACPI_PARSER  mFpdtS3SuspendRecordParser[] = {
  { L"Type",         2, 0,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Length",       1, 2,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Revision",     1, 3,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"SuspendStart", 4, 4,  L"0x%x",  NULL, NULL, NULL, NULL },
  { L"SuspendEnd",   8, 12, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI FPDT Table.
**/
STATIC CONST ACPI_PARSER  mFpdtParser[] = {
  PARSE_ACPI_HEADER (&mAcpiHdrInfo)
};

/**
  Parse the S3 performance table record.
  EFI_ACPI_6_5_FPDT_RECORD_TYPE_S3_PERFORMANCE_TABLE_POINTER

  @param [in] Format  Format string.
  @param [in] Ptr     Pointer to the start of the record.
  @param [in] Length  Length of the field.
**/
VOID
EFIAPI
DumpS3Ptp (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER   *FpdtHdr;
  EFI_ACPI_6_5_FPDT_S3_RESUME_RECORD           *FpdtS3ResumeRec;
  EFI_ACPI_6_5_FPDT_S3_SUSPEND_RECORD          *FpdtS3SuspendRec;
  UINT32                                       RecOffset;
  UINT8                                        *RecPtr;
  EFI_ACPI_6_5_FPDT_PERFORMANCE_RECORD_HEADER  *FpdtRecHdr;
  BOOLEAN                                      InvalidRecord;

  InvalidRecord = FALSE;
  RecPtr        = (UINT8 *)(UINTN)(*(UINT64 *)Ptr);

  Print (
    (Format != NULL) ? Format : L"0x%lx\n",
    RecPtr
    );

  ParseAcpi (
    TRUE,
    4,
    "S3 Performance Table Pointer Record",
    RecPtr,
    sizeof (EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER),
    PARSER_PARAMS (mAcpiFpdtHdrParser)
    );

  RecOffset = sizeof (EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER);
  FpdtHdr   = (EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER *)(RecPtr);
  while (!InvalidRecord && (RecOffset < FpdtHdr->Length)) {
    FpdtRecHdr = (EFI_ACPI_6_5_FPDT_PERFORMANCE_RECORD_HEADER *)(RecPtr + RecOffset);
    switch (FpdtRecHdr->Type) {
      case EFI_ACPI_6_5_FPDT_RUNTIME_RECORD_TYPE_S3_RESUME:
        FpdtS3ResumeRec = (EFI_ACPI_6_5_FPDT_S3_RESUME_RECORD *)(RecPtr + RecOffset);
        ParseAcpi (
          TRUE,
          4,
          "FPDT S3 Resume Record",
          RecPtr + RecOffset,
          FpdtS3ResumeRec->Header.Length,
          PARSER_PARAMS (mFpdtS3ResumeRecordParser)
          );
        break;
      case EFI_ACPI_6_5_FPDT_RUNTIME_RECORD_TYPE_S3_SUSPEND:
        FpdtS3SuspendRec = (EFI_ACPI_6_5_FPDT_S3_SUSPEND_RECORD *)(RecPtr + RecOffset);
        ParseAcpi (
          TRUE,
          4,
          "FPDT S3 Suspend Record",
          RecPtr + RecOffset,
          FpdtS3SuspendRec->Header.Length,
          PARSER_PARAMS (mFpdtS3SuspendRecordParser)
          );
        break;
      default:
        IncrementErrorCount ();
        Print (
          L"\nError: Unknown FPDT S3 Performance Record Type: 0x%x\n",
          FpdtRecHdr->Type
          );
        InvalidRecord = TRUE;
        break;
    }

    RecOffset += FpdtRecHdr->Length;
  }
}

/**
  Parse the Firmware basic boot performance record.
  EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER

  @param [in] Format  Format string.
  @param [in] Ptr     Pointer to the start of the record.
  @param [in] Length  Length of the field.
**/
VOID
EFIAPI
DumpFbbp (
  IN CONST CHAR16  *Format OPTIONAL,
  IN UINT8         *Ptr,
  IN UINT32        Length
  )
{
  EFI_ACPI_6_5_FPDT_FIRMWARE_BASIC_BOOT_RECORD  *FpdtBasicBootRec;
  EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER    *FpdtHdr;
  UINT32                                        RecOffset;
  UINT8                                         *RecPtr;

  RecPtr = (UINT8 *)(UINTN)(*(UINT64 *)Ptr);

  Print (
    (Format != NULL) ? Format : L"0x%lx\n",
    RecPtr
    );

  ParseAcpi (
    TRUE,
    4,
    "Firmware Basic Boot Performance Table",
    RecPtr,
    sizeof (EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER),
    PARSER_PARAMS (mAcpiFpdtHdrParser)
    );

  RecOffset = sizeof (EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER);
  FpdtHdr   = (EFI_ACPI_6_5_FPDT_PERFORMANCE_TABLE_HEADER *)RecPtr;
  while (RecOffset < FpdtHdr->Length) {
    FpdtBasicBootRec = (EFI_ACPI_6_5_FPDT_FIRMWARE_BASIC_BOOT_RECORD *)(RecPtr + RecOffset);
    if (FpdtBasicBootRec->Header.Type != EFI_ACPI_6_5_FPDT_RUNTIME_RECORD_TYPE_FIRMWARE_BASIC_BOOT) {
      IncrementErrorCount ();
      Print (
        L"\nError: Unexpected FPDT Boot Performance Record Type: 0x%x\n",
        FpdtBasicBootRec->Header.Type
        );
      break;
    }

    ParseAcpi (
      TRUE,
      4,
      "FPDT Boot Performance Table",
      RecPtr + RecOffset,
      FpdtBasicBootRec->Header.Length,
      PARSER_PARAMS (mFpdtBasicBootRecordParser)
      );
    RecOffset += FpdtBasicBootRec->Header.Length;
  }
}

/**
  This function parses the ACPI FPDT table.
  This function parses the FPDT table and optionally traces the ACPI
  table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiFpdt (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  EFI_ACPI_6_5_FPDT_PERFORMANCE_RECORD_HEADER  *RecordHeader;
  UINT32                                       Offset;
  UINT8                                        *RecordPtr;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "FPDT",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (mFpdtParser)
             );

  RecordPtr = Ptr + Offset;

  while (Offset < AcpiTableLength) {
    RecordHeader = (EFI_ACPI_6_5_FPDT_PERFORMANCE_RECORD_HEADER *)RecordPtr;

    switch (RecordHeader->Type) {
      case EFI_ACPI_6_5_FPDT_RECORD_TYPE_FIRMWARE_BASIC_BOOT_POINTER:
        ParseAcpi (
          TRUE,
          2,
          "FPDT Boot Performance Pointer Record",
          RecordPtr,
          RecordHeader->Length,
          PARSER_PARAMS (mFpdtBootPerfPointerParser)
          );
        break;
      case EFI_ACPI_6_5_FPDT_RECORD_TYPE_S3_PERFORMANCE_TABLE_POINTER:
        ParseAcpi (
          TRUE,
          2,
          "FPDT S3 Performance Table Pointer Record",
          RecordPtr,
          RecordHeader->Length,
          PARSER_PARAMS (mFpdtS3PerfPointerParser)
          );
        break;
      default:
        /// Parse the reserved record types
        ParseAcpi (
          TRUE,
          2,
          "FPDT Performance Record",
          RecordPtr,
          RecordHeader->Length,
          PARSER_PARAMS (mFpdtPerfRecordParser)
          );
        break;
    }

    Offset    += RecordHeader->Length;
    RecordPtr += RecordHeader->Length;
  }
}
